"""
ECU Dashboard Server  —  http://0.0.0.0:5000
=============================================

ESP8266 endpoints  (existing protocol, unchanged):
  GET  /version             → latest firmware version string
  GET  /firmware            → binary download
  POST /status              → ESP reports flash result  {version, status}
  POST /api/esp_register    → ESP registers its local IP on boot

Web UI endpoints:
  GET  /              → dashboard HTML
  POST /upload        → upload new .bin + version  (multipart)
  POST /api/command   → send control byte  {cmd, channel:"wire"|"wifi"|"both"}
  GET  /api/ports     → list serial ports
  POST /api/connect   → open serial port  {port, baud}
  POST /api/disconnect
  GET  /api/state     → current ECU state (JSON snapshot)
  WS   /socket.io     → real-time terminal + status
"""

import re
import socket
import threading
import time
import urllib.request
import urllib.parse
from pathlib import Path

import serial
import serial.tools.list_ports
from flask import Flask, request, jsonify, send_file
from flask_socketio import SocketIO, emit

TELEMETRY_UDP_PORT = 5001

# ──────────────────────────────────────────────────────────────────
#  Paths
# ──────────────────────────────────────────────────────────────────
BASE_DIR     = Path(__file__).parent
UPLOAD_DIR   = BASE_DIR / "uploads"
STATIC_DIR   = BASE_DIR / "static"
FIRMWARE_BIN = UPLOAD_DIR / "firmware.bin"
VERSION_FILE = UPLOAD_DIR / "version.txt"

UPLOAD_DIR.mkdir(exist_ok=True)

# ──────────────────────────────────────────────────────────────────
#  Helpers (defined first — used in state init below)
# ──────────────────────────────────────────────────────────────────
def _read_version():
    try:
        return VERSION_FILE.read_text().strip()
    except FileNotFoundError:
        return "–"


def _save_version(ver: str):
    VERSION_FILE.write_text(ver)
    state["server_version"] = ver


# ──────────────────────────────────────────────────────────────────
#  Flask + SocketIO
# ──────────────────────────────────────────────────────────────────
app      = Flask(__name__, static_folder=str(STATIC_DIR))
app.config["SECRET_KEY"] = "ecu-2024"
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# ──────────────────────────────────────────────────────────────────
#  Shared state
# ──────────────────────────────────────────────────────────────────
state = {
    "serial_port"   : "COM5",
    "baud"          : 115200,
    "connected"     : False,
    "ecu_version"   : "–",
    "server_version": _read_version(),
    "temperature"   : "–",
    "life_counter"  : "–",
    "rtc_time"      : "--:--:--",
    "last_flash"    : None,
    "esp_ip"        : None,    # set when ESP registers itself
}

ser      = None
ser_lock = threading.Lock()
_rx_run  = False

# ──────────────────────────────────────────────────────────────────
#  Serial: RX worker thread
# ──────────────────────────────────────────────────────────────────
def _parse_line(raw: str):
    """Extract structured values from UART2 output lines."""
    line = raw.strip()   # removes \r, leading/trailing whitespace

    m = re.search(r"Life counter:\s*(\d+)", line)
    if m:
        state["life_counter"] = m.group(1)

    m = re.search(r"Internal Temp:\s*(-?\d+)", line)
    if m:
        state["temperature"] = m.group(1) + " °C"

    m = re.search(r"(?:STM Application Version:|VER:)\s*([\d.]+)", line)
    if m:
        state["ecu_version"] = m.group(1)

    m = re.search(r"Hour:\s*(\d+).*?Minuts:\s*(\d+).*?Seconds:\s*(\d+)", line)
    if m:
        state["rtc_time"] = (
            f"{int(m.group(1)):02d}:{int(m.group(2)):02d}:{int(m.group(3)):02d}"
        )


def _rx_worker():
    global _rx_run
    buf = ""

    while _rx_run:
        if ser is None or not ser.is_open:
            time.sleep(0.1)
            continue
        try:
            waiting = ser.in_waiting or 1
            chunk   = ser.read(waiting)
            if chunk:
                text = chunk.decode("utf-8", errors="replace")
                socketio.emit("terminal", {"data": text})

                buf += text
                while "\n" in buf:
                    line, buf = buf.split("\n", 1)
                    _parse_line(line)

                socketio.emit("state", state)
        except serial.SerialException:
            state["connected"] = False
            socketio.emit("state", state)
            time.sleep(1)
        except Exception:
            time.sleep(0.05)


def _udp_worker():
    """Receive telemetry UDP packets from ESP8266 (no TCP overhead)."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", TELEMETRY_UDP_PORT))
    sock.settimeout(1.0)
    while True:
        try:
            data, _ = sock.recvfrom(512)
            line = data.decode("utf-8", errors="replace").strip()
            if line:
                _parse_line(line)
                socketio.emit("state", state)
                socketio.emit("terminal", {"data": "[WiFi] " + line + "\r\n"})
        except socket.timeout:
            continue
        except Exception:
            continue


def _open_serial(port: str, baud: int) -> bool:
    global ser, _rx_run

    _close_serial()

    try:
        with ser_lock:
            ser = serial.Serial(port, baud, timeout=0.1)
        state["serial_port"] = port
        state["baud"]        = baud
        state["connected"]   = True

        _rx_run = True
        threading.Thread(target=_rx_worker, daemon=True).start()

        # Request firmware version immediately after connecting
        def _query_version():
            time.sleep(0.5)   # let UART settle
            with ser_lock:
                if ser and ser.is_open:
                    ser.write(bytes([0xA1]))
        threading.Thread(target=_query_version, daemon=True).start()

        return True

    except serial.SerialException as e:
        state["connected"] = False
        print(f"[serial] Cannot open {port}: {e}")
        return False


def _close_serial():
    global ser, _rx_run

    _rx_run = False
    if ser and ser.is_open:
        try:
            ser.close()
        except Exception:
            pass
    ser = None
    state["connected"] = False


# ──────────────────────────────────────────────────────────────────
#  ESP WiFi channel
# ──────────────────────────────────────────────────────────────────
def _send_to_esp(byte_val: int):
    """Forward a command byte to the ESP's HTTP server (non-blocking)."""
    ip = state.get("esp_ip")
    if not ip:
        return
    try:
        body = urllib.parse.urlencode({"byte": format(byte_val, "02X")}).encode()
        req  = urllib.request.Request(
            f"http://{ip}/cmd", data=body, method="POST"
        )
        with urllib.request.urlopen(req, timeout=2):
            pass
        print(f"[esp]  0x{byte_val:02X} → {ip}")
    except Exception as e:
        print(f"[esp]  cmd to {ip} failed: {e}")
        state["esp_ip"] = None   # mark unreachable
        socketio.emit("state", state)


def _esp_cmd_async(byte_val: int):
    threading.Thread(target=_send_to_esp, args=(byte_val,), daemon=True).start()


# ──────────────────────────────────────────────────────────────────
#  Routes — ESP8266 protocol
# ──────────────────────────────────────────────────────────────────
@app.route("/version")
def route_version():
    return _read_version()


@app.route("/firmware")
def route_firmware():
    if not FIRMWARE_BIN.exists():
        return "No firmware available", 404
    return send_file(str(FIRMWARE_BIN), mimetype="application/octet-stream")


@app.route("/status", methods=["POST"])
def route_status():
    ver    = request.form.get("version", "?")
    result = request.form.get("status",  "?")
    state["last_flash"] = {"version": ver, "status": result}
    if result == "ok":
        state["ecu_version"] = ver
    socketio.emit("state", state)
    socketio.emit("flash_result", {"version": ver, "status": result})
    print(f"[flash] v{ver} → {result}")
    return "ok"


@app.route("/api/telemetry", methods=["POST"])
def route_telemetry():
    """ESP forwards raw STM32 telemetry lines here (wireless path)."""
    line = request.data.decode("utf-8", errors="replace").strip()
    if line:
        _parse_line(line)
        socketio.emit("state", state)
        socketio.emit("terminal", {"data": "[WiFi] " + line + "\r\n"})
    return "ok"


@app.route("/api/esp_register", methods=["POST"])
def route_esp_register():
    """ESP calls this on boot to tell the server its IP."""
    ip = request.form.get("ip", "").strip()
    if ip:
        state["esp_ip"] = ip
        socketio.emit("state", state)
        print(f"[esp]  registered at {ip}")
    return "ok"


# ──────────────────────────────────────────────────────────────────
#  Routes — Web UI
# ──────────────────────────────────────────────────────────────────
@app.route("/")
def route_index():
    return send_file(str(STATIC_DIR / "index.html"))


@app.route("/upload", methods=["POST"])
def route_upload():
    if "firmware" not in request.files:
        return jsonify(error="No file attached"), 400

    f       = request.files["firmware"]
    version = (request.form.get("version") or "").strip()
    if not version:
        return jsonify(error="Version is required (e.g. 1.0.6)"), 400

    f.save(str(FIRMWARE_BIN))
    _save_version(version)
    size = FIRMWARE_BIN.stat().st_size

    socketio.emit("state", state)
    print(f"[upload] v{version}  {size:,} bytes")
    return jsonify(ok=True, version=version, size=size)


COMMANDS = {
    "led_on"     : 0x01,
    "led_off"    : 0x02,
    "btld_jump"  : 0x03,
    "btld_update": 0x04,
    "run_time"   : 0x05,
    "sys_reset"  : 0x06,
    "get_version": 0xA1,
}


@app.route("/api/command", methods=["POST"])
def route_command():
    data    = request.get_json(silent=True) or request.form
    cmd     = data.get("cmd")
    channel = data.get("channel", "both")   # "wire" | "wifi" | "both"

    byte = COMMANDS.get(cmd)
    if byte is None:
        return jsonify(error=f"Unknown command '{cmd}'"), 400

    wire_ok  = False
    wifi_ok  = False
    wire_err = None
    wifi_err = None

    # ── Wire path (UART2 / COM5) ──────────────────────────────────
    if channel in ("wire", "both"):
        if ser and ser.is_open:
            with ser_lock:
                ser.write(bytes([byte]))
            wire_ok = True
        else:
            wire_err = "ECU not connected"

    # ── WiFi path (ESP HTTP server) ───────────────────────────────
    if channel in ("wifi", "both"):
        if state.get("esp_ip"):
            _esp_cmd_async(byte)
            wifi_ok = True
        else:
            wifi_err = "ESP not registered"

    # Return error only if the requested channel has nothing to send on
    if channel == "wire" and not wire_ok:
        return jsonify(error=wire_err), 503
    if channel == "wifi" and not wifi_ok:
        return jsonify(error=wifi_err), 503
    if channel == "both" and not wire_ok and not wifi_ok:
        return jsonify(error="No channel available"), 503

    return jsonify(
        ok=True, cmd=cmd, byte=hex(byte),
        wire=wire_ok, wifi=wifi_ok,
        wire_err=wire_err, wifi_err=wifi_err,
    )


@app.route("/api/ports")
def route_ports():
    ports = [{"port": p.device, "desc": p.description}
             for p in serial.tools.list_ports.comports()]
    return jsonify(ports)


@app.route("/api/connect", methods=["POST"])
def route_connect():
    data = request.get_json(silent=True) or {}
    port = data.get("port", "COM5")
    baud = int(data.get("baud", 115200))
    ok   = _open_serial(port, baud)
    socketio.emit("state", state)
    return jsonify(ok=ok, port=port, baud=baud)


@app.route("/api/disconnect", methods=["POST"])
def route_disconnect():
    _close_serial()
    socketio.emit("state", state)
    return jsonify(ok=True)


@app.route("/api/state")
def route_state():
    return jsonify(state)


# ──────────────────────────────────────────────────────────────────
#  SocketIO events
# ──────────────────────────────────────────────────────────────────
@socketio.on("connect")
def on_ws_connect():
    emit("state", state)


@socketio.on("send_raw")
def on_send_raw(data):
    """Send a raw hex byte from the terminal input box."""
    try:
        byte_val = int(str(data.get("byte", "0")), 16) & 0xFF
        channel  = data.get("channel", "wire")
        if channel in ("wire", "both") and ser and ser.is_open:
            with ser_lock:
                ser.write(bytes([byte_val]))
        if channel in ("wifi", "both"):
            _esp_cmd_async(byte_val)
    except (ValueError, TypeError):
        pass


# ──────────────────────────────────────────────────────────────────
#  Main
# ──────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    import sys
    public_url = next((a for a in sys.argv[1:] if a.startswith("http")), None)

    print("=" * 60)
    print("  ECU Dashboard  →  http://localhost:5000")
    print("  LAN access    →  http://192.168.1.2:5000")
    if public_url:
        print(f"  PUBLIC URL    →  {public_url}")
    print("=" * 60)

    threading.Thread(target=_udp_worker, daemon=True).start()
    print(f"  UDP telemetry  →  port {TELEMETRY_UDP_PORT}")

    _open_serial(state["serial_port"], state["baud"])

    socketio.run(app, host="0.0.0.0", port=5000, debug=False, allow_unsafe_werkzeug=True)

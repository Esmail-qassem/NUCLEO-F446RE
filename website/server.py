"""
ECU Dashboard Server  —  http://0.0.0.0:5000
"""

import re
import socket
import sqlite3
import subprocess
import threading
import time
import datetime
import shutil
import urllib.request
import urllib.parse
from pathlib import Path

import serial
import serial.tools.list_ports
from flask import Flask, request, jsonify, send_file
from flask_socketio import SocketIO, emit

# ──────────────────────────────────────────────────────────────────
#  Paths
# ──────────────────────────────────────────────────────────────────
BASE_DIR     = Path(__file__).parent
UPLOAD_DIR   = BASE_DIR / "uploads"
STATIC_DIR   = BASE_DIR / "static"
FIRMWARE_BIN = UPLOAD_DIR / "firmware.bin"
VERSION_FILE = UPLOAD_DIR / "version.txt"
DB_PATH      = BASE_DIR / "ecu_data.db"
APP_BTLD_BIN = BASE_DIR.parent / "APP/Tools/APP_BTLD.bin"

UPLOAD_DIR.mkdir(exist_ok=True)
TELEMETRY_UDP_PORT = 5001

# ──────────────────────────────────────────────────────────────────
#  Flask + SocketIO
# ──────────────────────────────────────────────────────────────────
app      = Flask(__name__, static_folder=str(STATIC_DIR))
app.config["SECRET_KEY"] = "ecu-2024"
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# Trust Cloudflare / reverse-proxy headers (X-Forwarded-Proto, etc.)
from werkzeug.middleware.proxy_fix import ProxyFix
app.wsgi_app = ProxyFix(app.wsgi_app, x_for=1, x_proto=1, x_host=1, x_prefix=1)

# ──────────────────────────────────────────────────────────────────
#  Database
# ──────────────────────────────────────────────────────────────────
_db_lock = threading.Lock()

def _db():
    conn = sqlite3.connect(str(DB_PATH), check_same_thread=False)
    conn.row_factory = sqlite3.Row
    return conn

def _db_init():
    with _db_lock, _db() as conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS firmware_history (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                version      TEXT    NOT NULL,
                uploaded_at  TEXT    NOT NULL,
                size         INTEGER,
                flash_result TEXT    DEFAULT 'pending',
                notes        TEXT    DEFAULT ''
            )
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS metrics (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                ts          TEXT    NOT NULL,
                temperature REAL,
                life_counter INTEGER
            )
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS boot_history (
                id      INTEGER PRIMARY KEY AUTOINCREMENT,
                ts      TEXT    NOT NULL,
                reason  TEXT    NOT NULL,
                version TEXT    DEFAULT '–'
            )
        """)
        conn.commit()

# ──────────────────────────────────────────────────────────────────
#  Version helpers
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
    "esp_ip"        : None,
    "pwm_duty"      : 0,
}

ser      = None
ser_lock = threading.Lock()
_rx_run  = False

# ──────────────────────────────────────────────────────────────────
#  Alert system
# ──────────────────────────────────────────────────────────────────
alert_cfg = {
    "temp_threshold"  : 55,    # °C
    "life_stall_s"    : 60,    # seconds before life-counter stall alert
    "esp_offline_s"   : 120,   # seconds before ESP offline alert
}

_alert_state = {
    "last_life_value" : None,
    "last_life_time"  : None,
    "last_esp_seen"   : None,
    "last_temp_alert" : 0,
    "last_life_alert" : 0,
    "last_esp_alert"  : 0,
}

def _emit_alert(kind: str, message: str):
    now = time.time()
    cooldown = 60  # don't repeat same alert type within 60 s
    key = f"last_{kind.lower()[:4]}_alert"
    if key in _alert_state and now - _alert_state.get(key, 0) < cooldown:
        return
    _alert_state[key] = now
    payload = {"kind": kind, "message": message,
               "ts": datetime.datetime.now().strftime("%H:%M:%S")}
    socketio.emit("alert", payload)
    print(f"[alert] {kind}: {message}")

def _check_alerts(temp_val=None, life_val=None):
    now = time.time()

    # Temperature alert
    if temp_val is not None:
        try:
            t = int(str(temp_val).replace(" °C", "").replace("°C", "").strip())
            if t > alert_cfg["temp_threshold"]:
                _emit_alert("HIGH_TEMP",
                    f"Temperature {t} °C exceeds threshold ({alert_cfg['temp_threshold']} °C)")
        except ValueError:
            pass

    # Life counter stall
    if life_val is not None:
        if _alert_state["last_life_value"] != life_val:
            _alert_state["last_life_value"] = life_val
            _alert_state["last_life_time"]  = now
        elif _alert_state["last_life_time"] and \
             (now - _alert_state["last_life_time"]) > alert_cfg["life_stall_s"]:
            _emit_alert("LIFE_STALL",
                "Life counter has not changed — ECU may be unresponsive")

    # ESP offline
    if state.get("esp_ip"):
        _alert_state["last_esp_seen"] = now
    elif _alert_state["last_esp_seen"] and \
         (now - _alert_state["last_esp_seen"]) > alert_cfg["esp_offline_s"]:
        _emit_alert("ESP_OFFLINE", "ESP8266 has gone offline (no registration received)")

# ──────────────────────────────────────────────────────────────────
#  Metrics storage (throttled to 1 insert / 60 s)
# ──────────────────────────────────────────────────────────────────
_last_metric_ts  = 0
_pending_metric  = {"temp": None, "life": None}

def _store_metric(temp=None, life=None):
    global _last_metric_ts, _pending_metric
    # Accumulate latest values — temp and life arrive on separate UART lines
    if temp is not None:
        _pending_metric["temp"] = temp
    if life is not None:
        _pending_metric["life"] = life
    now = time.time()
    if now - _last_metric_ts < 60:
        return
    _last_metric_ts = now
    ts = datetime.datetime.now().isoformat(timespec="seconds")
    try:
        with _db_lock, _db() as conn:
            conn.execute(
                "INSERT INTO metrics (ts, temperature, life_counter) VALUES (?,?,?)",
                (ts, _pending_metric["temp"], _pending_metric["life"])
            )
            conn.commit()
    except Exception as e:
        print(f"[db] metrics insert failed: {e}")

# ──────────────────────────────────────────────────────────────────
#  Serial: line parser
# ──────────────────────────────────────────────────────────────────
def _parse_line(raw: str):
    line = raw.strip()

    # MPU-6050 sensor data — $DATA:ax,ay,az,gx,gy,gz
    if line.startswith("$DATA:"):
        parts = line[6:].split(",")
        if len(parts) == 6:
            try:
                vals = [int(p) for p in parts]
                socketio.emit("mpu", {
                    "ax": vals[0], "ay": vals[1], "az": vals[2],
                    "gx": vals[3], "gy": vals[4], "gz": vals[5],
                })
            except ValueError:
                pass
        return

    temp_val = None
    life_val = None

    m = re.search(r"Life counter:\s*(\d+)", line)
    if m:
        state["life_counter"] = m.group(1)
        life_val = int(m.group(1))

    m = re.search(r"Internal Temp:\s*(-?\d+)", line)
    if m:
        state["temperature"] = m.group(1) + " °C"
        temp_val = int(m.group(1))

    m = re.search(r"(?:STM Application Version:|VER:)\s*([\d.]+)", line)
    if m:
        state["ecu_version"] = m.group(1)

    m = re.search(r"Hour:\s*(\d+).*?Minuts:\s*(\d+).*?Seconds:\s*(\d+)", line)
    if m:
        state["rtc_time"] = (
            f"{int(m.group(1)):02d}:{int(m.group(2)):02d}:{int(m.group(3)):02d}"
        )

    # Boot reason (#9)
    m = re.search(r"BOOT:(\w+)", line)
    if m:
        reason  = m.group(1)
        ts      = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        version = state.get("ecu_version", "–")
        try:
            with _db_lock, _db() as conn:
                conn.execute(
                    "INSERT INTO boot_history (ts, reason, version) VALUES (?,?,?)",
                    (ts, reason, version)
                )
                conn.commit()
        except Exception as e:
            print(f"[db] boot_history insert failed: {e}")
        socketio.emit("boot_history", _get_boot_history())
        print(f"[boot] reason={reason}  ver={version}")

    # CPU load — "$CPU:T0:2% T1:0% T2:24% T3:0% T4:0% T5:1%"
    m = re.search(r"\$CPU:\s*((?:T\d+:\d+%\s*)+)", line)
    if m:
        cpu = {}
        for hit in re.finditer(r"T(\d+):(\d+)%", m.group(1)):
            cpu[f"T{hit.group(1)}"] = int(hit.group(2))
        if cpu:
            socketio.emit("cpu_load", cpu)

    # Stack usage — "Stack: T0:5% T1:8% T2:3% T3:12% T4:7% T5:15%"
    m = re.search(r"Stack:\s*((?:T\d+:\d+%\s*)+)", line)
    if m:
        stack = {}
        for hit in re.finditer(r"T(\d+):(\d+)%", m.group(1)):
            stack[f"T{hit.group(1)}"] = int(hit.group(2))
        if stack:
            socketio.emit("stack_usage", stack)

    # Store metrics + check alerts
    if temp_val is not None or life_val is not None:
        _store_metric(temp=temp_val, life=life_val)
        threading.Thread(
            target=_check_alerts,
            kwargs={"temp_val": temp_val, "life_val": life_val},
            daemon=True
        ).start()

# ──────────────────────────────────────────────────────────────────
#  Serial: RX worker
# ──────────────────────────────────────────────────────────────────
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
        def _query_version():
            time.sleep(0.5)
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
#  UDP telemetry worker
# ──────────────────────────────────────────────────────────────────
def _udp_worker():
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

# ──────────────────────────────────────────────────────────────────
#  ESP WiFi channel
# ──────────────────────────────────────────────────────────────────
def _send_to_esp(byte_val: int):
    ip = state.get("esp_ip")
    if not ip:
        return
    try:
        body = urllib.parse.urlencode({"byte": format(byte_val, "02X")}).encode()
        req  = urllib.request.Request(f"http://{ip}/cmd", data=body, method="POST")
        with urllib.request.urlopen(req, timeout=2):
            pass
    except Exception as e:
        print(f"[esp]  cmd to {ip} failed: {e}")
        state["esp_ip"] = None
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
    # Update firmware history
    try:
        with _db_lock, _db() as conn:
            conn.execute(
                "UPDATE firmware_history SET flash_result=? WHERE version=? AND flash_result='pending'",
                (result, ver)
            )
            conn.commit()
    except Exception as e:
        print(f"[db] history update failed: {e}")
    socketio.emit("state", state)
    socketio.emit("flash_result", {"version": ver, "status": result})
    socketio.emit("firmware_history", _get_history())
    print(f"[flash] v{ver} → {result}")
    return "ok"

@app.route("/api/telemetry", methods=["POST"])
def route_telemetry():
    line = request.data.decode("utf-8", errors="replace").strip()
    if line:
        _parse_line(line)
        socketio.emit("state", state)
        socketio.emit("terminal", {"data": "[WiFi] " + line + "\r\n"})
    return "ok"

@app.route("/api/esp_register", methods=["POST"])
def route_esp_register():
    ip = request.form.get("ip", "").strip()
    if ip:
        state["esp_ip"] = ip
        _alert_state["last_esp_seen"] = time.time()
        socketio.emit("state", state)
        print(f"[esp]  registered at {ip}")
    return "ok"

# ──────────────────────────────────────────────────────────────────
#  Build — runs combined_image/build.sh via bash
# ──────────────────────────────────────────────────────────────────
BUILD_DIR    = BASE_DIR.parent / "combined_image"
_build_running = False

def _find_bash() -> str:
    # Prefer Git for Windows bash — avoids WSL path conversion (/mnt/c/...)
    for p in [
        r"C:\Program Files\Git\bin\bash.exe",
        r"C:\Program Files\Git\usr\bin\bash.exe",
        r"C:\Program Files (x86)\Git\bin\bash.exe",
    ]:
        if Path(p).exists():
            return p
    return shutil.which("bash") or "bash"

def _do_build():
    global _build_running

    def log(msg: str):
        socketio.emit("build_log", {
            "msg": msg,
            "ts" : datetime.datetime.now().strftime("%H:%M:%S")
        })
        print(f"[build] {msg}")

    tmp_script = BUILD_DIR / "_tmp_build.sh"
    try:
        bash_exe = _find_bash()
        log(f"bash: {bash_exe}")
        log(f"dir:  {BUILD_DIR}")

        # Strip Windows CRLF line endings so bash doesn't see \r as a command
        script_text = (BUILD_DIR / "build.sh").read_text(encoding="utf-8", errors="replace")
        script_text = script_text.replace("\r\n", "\n").replace("\r", "\n")
        tmp_script.write_text(script_text, encoding="utf-8")

        log("Running build.sh ...")
        proc = subprocess.Popen(
            [bash_exe, str(tmp_script)],
            cwd=str(BUILD_DIR),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )

        for line in proc.stdout:
            log(line.rstrip())

        proc.wait()
        ok = proc.returncode == 0
        log("Build OK" if ok else f"Build FAILED (exit {proc.returncode})")
        socketio.emit("build_done", {"ok": ok})

    except FileNotFoundError:
        log("ERROR: bash not found — install Git for Windows")
        socketio.emit("build_done", {"ok": False})
    except Exception as e:
        log(f"ERROR: {e}")
        socketio.emit("build_done", {"ok": False})
    finally:
        _build_running = False
        tmp_script.unlink(missing_ok=True)

@app.route("/api/build", methods=["POST"])
def route_build():
    global _build_running
    if _build_running:
        return jsonify(error="Build already in progress"), 409
    if not BUILD_DIR.exists():
        return jsonify(error=f"combined_image/ not found at {BUILD_DIR}"), 500
    _build_running = True
    threading.Thread(target=_do_build, daemon=True).start()
    return jsonify(ok=True)

# ──────────────────────────────────────────────────────────────────
#  UART Flash — direct UART2 firmware update
# ──────────────────────────────────────────────────────────────────
_uart_flash_running = False

def _do_uart_flash(payload: bytes):
    global _uart_flash_running

    def log(msg: str):
        socketio.emit("uart_flash_log", {
            "msg": msg,
            "ts" : datetime.datetime.now().strftime("%H:%M:%S")
        })
        print(f"[uart_flash] {msg}")

    try:
        total = len(payload)
        log(f"Payload: {total:,} bytes")

        log("Sending BTLD_JUMP (0x03) ...")
        with ser_lock:
            ser.write(bytes([0x03]))

        log("Waiting 3 s for bootloader to start ...")
        time.sleep(3)

        log("Sending sync 0x55 ...")
        with ser_lock:
            ser.write(b'\x55')

        log("Streaming firmware ...")
        CHUNK = 512
        for i in range(0, total, CHUNK):
            with ser_lock:
                ser.write(payload[i:i + CHUNK])
            sent = min(i + CHUNK, total)
            socketio.emit("uart_flash_progress", {
                "pct"  : int(sent / total * 100),
                "sent" : sent,
                "total": total,
            })
            time.sleep(0.005)

        log(f"Transfer done — {total:,} bytes sent")
        log("Waiting for CRC + verification (watch terminal) ...")
        socketio.emit("uart_flash_done", {"ok": True})

    except Exception as e:
        log(f"ERROR: {e}")
        socketio.emit("uart_flash_done", {"ok": False})
    finally:
        _uart_flash_running = False

@app.route("/api/uart_flash", methods=["POST"])
def route_uart_flash():
    global _uart_flash_running
    if _uart_flash_running:
        return jsonify(error="Flash already in progress"), 409
    if not (ser and ser.is_open):
        return jsonify(error="ECU not connected via UART"), 503

    if "firmware" in request.files:
        payload = request.files["firmware"].read()
    elif FIRMWARE_BIN.exists():
        payload = FIRMWARE_BIN.read_bytes()
    else:
        return jsonify(error="No firmware — upload one first or select a file"), 400

    if len(payload) == 0:
        return jsonify(error="Firmware file is empty"), 400

    _uart_flash_running = True
    threading.Thread(target=_do_uart_flash, args=(payload,), daemon=True).start()
    return jsonify(ok=True, size=len(payload))

@app.route("/api/app_btld_info")
def route_app_btld_info():
    if not APP_BTLD_BIN.exists():
        return jsonify(exists=False, path=str(APP_BTLD_BIN))
    stat = APP_BTLD_BIN.stat()
    return jsonify(
        exists   = True,
        size     = stat.st_size,
        modified = datetime.datetime.fromtimestamp(stat.st_mtime).strftime("%Y-%m-%d %H:%M:%S"),
        path     = "APP/Tools/APP_BTLD.bin",
    )

@app.route("/api/uart_flash_local", methods=["POST"])
def route_uart_flash_local():
    global _uart_flash_running
    if _uart_flash_running:
        return jsonify(error="Flash already in progress"), 409
    if not (ser and ser.is_open):
        return jsonify(error="ECU not connected via UART"), 503
    if not APP_BTLD_BIN.exists():
        return jsonify(error=f"APP_BTLD.bin not found at {APP_BTLD_BIN}"), 404
    payload = APP_BTLD_BIN.read_bytes()
    if len(payload) == 0:
        return jsonify(error="APP_BTLD.bin is empty"), 400
    _uart_flash_running = True
    threading.Thread(target=_do_uart_flash, args=(payload,), daemon=True).start()
    return jsonify(ok=True, size=len(payload), path="APP/Tools/APP_BTLD.bin")

# ──────────────────────────────────────────────────────────────────
#  Routes — Firmware History
# ──────────────────────────────────────────────────────────────────
def _get_history():
    try:
        with _db_lock, _db() as conn:
            rows = conn.execute(
                "SELECT * FROM firmware_history ORDER BY id DESC LIMIT 50"
            ).fetchall()
            return [dict(r) for r in rows]
    except Exception:
        return []

@app.route("/api/firmware_history")
def route_firmware_history():
    return jsonify(_get_history())

def _get_boot_history():
    try:
        with _db_lock, _db() as conn:
            rows = conn.execute(
                "SELECT * FROM boot_history ORDER BY id DESC LIMIT 50"
            ).fetchall()
            return [dict(r) for r in rows]
    except Exception:
        return []

@app.route("/api/boot_history")
def route_boot_history():
    return jsonify(_get_boot_history())

# ──────────────────────────────────────────────────────────────────
#  Route — PWM duty control (#5)
# ──────────────────────────────────────────────────────────────────
@app.route("/api/pwm", methods=["POST"])
def route_pwm():
    data    = request.get_json(silent=True) or {}
    duty    = max(0, min(100, int(data.get("duty", 0))))
    channel = data.get("channel", "both")

    wire_ok = False
    if channel in ("wire", "both") and ser and ser.is_open:
        with ser_lock:
            ser.write(bytes([0x07, duty]))   # 2-byte command
        wire_ok = True

    wifi_ok = False
    if channel in ("wifi", "both") and state.get("esp_ip"):
        # ESP /cmd only supports single bytes; send 0x07 then duty as two calls
        _esp_cmd_async(0x07)
        _esp_cmd_async(duty)
        wifi_ok = True

    if not wire_ok and not wifi_ok:
        return jsonify(error="No channel available"), 503

    state["pwm_duty"] = duty
    socketio.emit("state", state)
    return jsonify(ok=True, duty=duty)

@app.route("/api/rollback/<version>", methods=["POST"])
def route_rollback(version):
    versioned = UPLOAD_DIR / f"firmware_v{version}.bin"
    if not versioned.exists():
        return jsonify(error=f"No stored binary for v{version}"), 404
    shutil.copy(str(versioned), str(FIRMWARE_BIN))
    _save_version(version)
    socketio.emit("state", state)
    socketio.emit("firmware_history", _get_history())
    print(f"[rollback] restored v{version}")
    return jsonify(ok=True, version=version)

# ──────────────────────────────────────────────────────────────────
#  Routes — OLED frame data
# ──────────────────────────────────────────────────────────────────
@app.route("/api/oled_frames")
def route_oled_frames():
    h_file = BASE_DIR.parent / "APP/HAL/OLED/OLED_NyanCat.h"
    if not h_file.exists():
        return jsonify(error="OLED_NyanCat.h not found"), 404
    try:
        text = h_file.read_text(encoding="utf-8", errors="replace")
        frames = []
        for m in re.finditer(r'NyanCat_Frame\d+\[512\]\s*=\s*\{([^}]+)\}', text):
            nums = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]{2}', m.group(1))]
            if len(nums) == 512:
                frames.append(nums)
        return jsonify(frames=frames, width=128, height=32, pages=4,
                       total_height=64, y_offset_pages=2)
    except Exception as e:
        return jsonify(error=str(e)), 500

# ──────────────────────────────────────────────────────────────────
#  Routes — Cloudflare tunnel URL
# ──────────────────────────────────────────────────────────────────
@app.route("/api/tunnel_url")
def route_tunnel_url():
    f = BASE_DIR / "tunnel_url.txt"
    url = f.read_text().strip() if f.exists() else ""
    return jsonify(url=url)

# ──────────────────────────────────────────────────────────────────
#  Routes — Metrics
# ──────────────────────────────────────────────────────────────────
@app.route("/api/metrics")
def route_metrics():
    hours = int(request.args.get("hours", 24))
    since = (datetime.datetime.now() - datetime.timedelta(hours=hours)).isoformat()
    try:
        with _db_lock, _db() as conn:
            rows = conn.execute(
                "SELECT ts, temperature, life_counter FROM metrics WHERE ts >= ? ORDER BY id ASC",
                (since,)
            ).fetchall()
            return jsonify([dict(r) for r in rows])
    except Exception as e:
        return jsonify(error=str(e)), 500

# ──────────────────────────────────────────────────────────────────
#  Routes — Alerts config
# ──────────────────────────────────────────────────────────────────
@app.route("/api/alert_config", methods=["GET"])
def route_alert_config_get():
    return jsonify(alert_cfg)

@app.route("/api/alert_config", methods=["POST"])
def route_alert_config_set():
    data = request.get_json(silent=True) or {}
    if "temp_threshold" in data:
        alert_cfg["temp_threshold"] = int(data["temp_threshold"])
    if "life_stall_s" in data:
        alert_cfg["life_stall_s"] = int(data["life_stall_s"])
    return jsonify(alert_cfg)

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
        return jsonify(error="Version is required"), 400

    f.save(str(FIRMWARE_BIN))
    # Also save versioned copy for rollback
    versioned = UPLOAD_DIR / f"firmware_v{version}.bin"
    shutil.copy(str(FIRMWARE_BIN), str(versioned))
    _save_version(version)
    size = FIRMWARE_BIN.stat().st_size

    # Insert into firmware history
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    try:
        with _db_lock, _db() as conn:
            conn.execute(
                "INSERT INTO firmware_history (version, uploaded_at, size) VALUES (?,?,?)",
                (version, ts, size)
            )
            conn.commit()
    except Exception as e:
        print(f"[db] history insert failed: {e}")

    socketio.emit("state", state)
    socketio.emit("firmware_history", _get_history())
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
    channel = data.get("channel", "both")
    byte    = COMMANDS.get(cmd)
    if byte is None:
        return jsonify(error=f"Unknown command '{cmd}'"), 400

    wire_ok = wire_err = wifi_ok = wifi_err = None

    if channel in ("wire", "both"):
        if ser and ser.is_open:
            with ser_lock:
                ser.write(bytes([byte]))
            wire_ok = True
        else:
            wire_err = "ECU not connected"

    if channel in ("wifi", "both"):
        if state.get("esp_ip"):
            _esp_cmd_async(byte)
            wifi_ok = True
        else:
            wifi_err = "ESP not registered"

    if channel == "wire"  and not wire_ok: return jsonify(error=wire_err), 503
    if channel == "wifi"  and not wifi_ok: return jsonify(error=wifi_err), 503
    if channel == "both"  and not wire_ok and not wifi_ok:
        return jsonify(error="No channel available"), 503

    return jsonify(ok=True, cmd=cmd, byte=hex(byte),
                   wire=wire_ok, wifi=wifi_ok,
                   wire_err=wire_err, wifi_err=wifi_err)

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
    emit("firmware_history", _get_history())
    emit("boot_history", _get_boot_history())

@socketio.on("send_raw")
def on_send_raw(data):
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

    _db_init()

    print("=" * 60)
    print("  ECU Dashboard  →  http://localhost:5000")
    print("  LAN access    →  http://192.168.1.2:5000")
    if public_url:
        print(f"  PUBLIC URL    →  {public_url}")
    print(f"  UDP telemetry →  port {TELEMETRY_UDP_PORT}")
    print("=" * 60)

    threading.Thread(target=_udp_worker, daemon=True).start()
    _open_serial(state["serial_port"], state["baud"])
    socketio.run(app, host="0.0.0.0", port=5000, debug=False, allow_unsafe_werkzeug=True)

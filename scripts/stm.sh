#!/bin/bash
# ~/scripts/stm_lib.sh
# STM32 bash command library - Windows Git Bash version
# Usage: source ~/scripts/stm_lib.sh

# ─── Config ───────────────────────────────────────────────────────────────────
PORT="COM5"       # change to your actual COM port
BAUD="115200"
STM_PROJECT_ROOT="$HOME/Desktop/NUCLEO-F446RE"

# ─── Python serial helper ─────────────────────────────────────────────────────
_stm_send_byte() {
    local byte="$1"
    python - <<EOF
import serial, sys
try:
    ser = serial.Serial('$PORT', $BAUD, timeout=1)
    ser.write(bytes([int('$byte', 16)]))
    ser.close()
except Exception as e:
    print(f"[STM] Serial error: {e}", file=sys.stderr)
    sys.exit(1)
EOF
}

_stm_send_file() {
    local filepath="$1"
    python - <<EOF
import serial, sys
try:
    ser = serial.Serial('$PORT', $BAUD, timeout=1)
    with open(r'$filepath', 'rb') as f:
        data = f.read()
    ser.write(b'\x55')
    ser.write(data)
    ser.close()
    print(f"[BTLD] Sent {len(data)} bytes")
except Exception as e:
    print(f"[STM] Serial error: {e}", file=sys.stderr)
    sys.exit(1)
EOF
}
# ─── Build ────────────────────────────────────────────────────────────────────
_stm_build() {
    local target="${1:-all}"  # default: all
    local build_script
    build_script=$(cygpath -u "$STM_PROJECT_ROOT/combined_image/build.sh" 2>/dev/null \
                   || echo "$STM_PROJECT_ROOT/combined_image/build.sh")

    if [ ! -f "$build_script" ]; then
        printf "[STM] Error: build.sh not found at: $build_script\n"
        return 1
    fi

    printf "[STM] Building target: $target\n"
    bash "$build_script" "$target"

    if [ $? -eq 0 ]; then
        printf "[STM] Build OK\n"
    else
        printf "[STM] Build FAILED\n"
        return 1
    fi
}

# ─── Flash ────────────────────────────────────────────────────────────────────
_stm_flash() {
    local target="${1:-all}"  # default: all
    local flash_script
    flash_script=$(cygpath -u "$STM_PROJECT_ROOT/combined_image/FLASH.sh" 2>/dev/null \
                   || echo "$STM_PROJECT_ROOT/combined_image/FLASH.sh")

    if [ ! -f "$flash_script" ]; then
        printf "[STM] Error: FLASH.sh not found at: $flash_script\n"
        return 1
    fi

    printf "[STM] Flashing target: $target\n"
    bash "$flash_script" "$target"

    if [ $? -eq 0 ]; then
        printf "[STM] Flash OK\n"
    else
        printf "[STM] Flash FAILED\n"
        return 1
    fi
}

# ─── UART connect ─────────────────────────────────────────────────────────────
_stm_uart_connect() {
    python - <<EOF
import serial, sys
try:
    ser = serial.Serial('$PORT', $BAUD, timeout=1)
    ser.close()
    print("Connected to $PORT at $BAUD")
except Exception as e:
    print(f"[STM] Could not open port: {e}", file=sys.stderr)
    sys.exit(1)
EOF
}

# ─── Monitor ──────────────────────────────────────────────────────────────────
_stm_monitor_close() {
    if [ -z "$STM_MONITOR_PID" ]; then
        printf "[STM] No monitor running\n"
        return
    fi
    kill "$STM_MONITOR_PID" 2>/dev/null
    STM_MONITOR_PID=""
    printf "\n[STM] Monitor closed\n"
}

_stm_monitor_open() {
    if [ -n "$STM_MONITOR_PID" ] && kill -0 "$STM_MONITOR_PID" 2>/dev/null; then
        printf "[STM] Monitor already running (PID $STM_MONITOR_PID)\n"
        return
    fi

    printf "[STM] Monitor opened — press Ctrl+C to stop\n"

    python - <<EOF &
import serial, sys
try:
    ser = serial.Serial('$PORT', $BAUD, timeout=1)
    while True:
        data = ser.readline()
        if data:
            print(data.decode('utf-8', errors='replace'), end='', flush=True)
except KeyboardInterrupt:
    pass
except Exception as e:
    print(f"[STM] Monitor error: {e}", file=sys.stderr)
finally:
    ser.close()
EOF

    STM_MONITOR_PID=$!
    trap '_stm_monitor_close' INT TSTP
    wait "$STM_MONITOR_PID"
    trap - INT TSTP
}

# ─── Game input (WASD) ───────────────────────────────────────────────────────
_stm_game_input() {
    printf "[STM] Game input — w/s/a/d to move, r to reset, i/j for options, q to quit\n"
    while IFS= read -r -s -n1 key; do
        case "$key" in
            w|W) _stm_send_byte "0x77" && printf "  ↑\r" ;;
            s|S) _stm_send_byte "0x73" && printf "  ↓\r" ;;
            a|A) _stm_send_byte "0x61" && printf "  ←\r" ;;
            d|D) _stm_send_byte "0x64" && printf "  →\r" ;;
            r|R) _stm_send_byte "0x72" && printf "  Reset\r" ;;
            i|I) _stm_send_byte "0x69" && printf "  [i]\r" ;;
            j|J) _stm_send_byte "0x6A" && printf "  [j]\r" ;;
            q|Q) printf "\n[STM] Game input closed\n"; break ;;
        esac
    done
}

# ─── LED ──────────────────────────────────────────────────────────────────────
_stm_led_on() {
    _stm_send_byte "0x01" && printf "[STM] LED ON\n"
}

_stm_led_off() {
    _stm_send_byte "0x02" && printf "[STM] LED OFF\n"
}

# ─── Bootloader ───────────────────────────────────────────────────────────────
_BTLD_JUMP() {
    _stm_send_byte "0x03" && printf "[BTLD] Jump sent\n"
}
_stm_sw_version() {
    _stm_send_byte "0xA1" && printf "[STM] Requested software version\n"
}
_stm_time() {
    _stm_send_byte "0x05" && printf "[STM] Requested current time\n"
}
_BTLD_UPDATE() {
    local bin="$1"
    local bin_win
    bin_win=$(cygpath -w "$bin" 2>/dev/null || echo "$bin")
    local bin_unix
    bin_unix=$(cygpath -u "$bin_win" 2>/dev/null || echo "$bin")

    if [ -z "$bin" ] || [ ! -f "$bin_unix" ]; then
        printf "[BTLD] Error: binary not found: '$bin'\n"
        printf "Usage: stm bootloader update <path/to/file.bin>\n"
        return 1
    fi

    printf "[BTLD] Triggering update mode...\n"
    _stm_send_byte "0x04"

    sleep 0.5

    printf "[BTLD] Sending binary: $bin_win\n"
    _stm_send_file "$bin_win"

    printf "[BTLD] Transfer done\n"
}

# ─── Clean ────────────────────────────────────────────────────────────────────
_stm_clean() {
    if [ -z "$STM_PROJECT_ROOT" ]; then
        printf "[STM] Error: STM_PROJECT_ROOT is not set\n"
        return 1
    fi

    if [ ! -d "$STM_PROJECT_ROOT" ]; then
        printf "[STM] Error: '$STM_PROJECT_ROOT' is not a valid directory\n"
        return 1
    fi

    printf "[STM] About to clean artifacts in: $STM_PROJECT_ROOT\n"
    read -p "Are you sure? (y/N): " confirm
    [ "$confirm" != "y" ] && printf "Aborted\n" && return 0

    find "$STM_PROJECT_ROOT/APP/Build"  \( -name "*.o" -o -name "*.d" \) -delete
    find "$STM_PROJECT_ROOT/BM/Build"   \( -name "*.o" -o -name "*.d" \) -delete
    find "$STM_PROJECT_ROOT/BTLD/Build" \( -name "*.o" -o -name "*.d" \) -delete

    rm -f "$STM_PROJECT_ROOT/APP/Tools/"*.bin  \
          "$STM_PROJECT_ROOT/APP/Tools/"*.elf  \
          "$STM_PROJECT_ROOT/APP/Tools/"*.hex  \
          "$STM_PROJECT_ROOT/APP/Tools/"*.map  \
          "$STM_PROJECT_ROOT/APP/Tools/"*.txt  \
          "$STM_PROJECT_ROOT/BM/Tools/"*.bin   \
          "$STM_PROJECT_ROOT/BM/Tools/"*.elf   \
          "$STM_PROJECT_ROOT/BM/Tools/"*.hex   \
          "$STM_PROJECT_ROOT/BM/Tools/"*.map   \
          "$STM_PROJECT_ROOT/BM/Tools/"*.txt   \
          "$STM_PROJECT_ROOT/BTLD/Tools/"*.bin \
          "$STM_PROJECT_ROOT/BTLD/Tools/"*.elf \
          "$STM_PROJECT_ROOT/BTLD/Tools/"*.hex \
          "$STM_PROJECT_ROOT/BTLD/Tools/"*.map \
          "$STM_PROJECT_ROOT/BTLD/Tools/"*.txt

    printf "[STM] Clean done\n"
}

# ─── Exit ─────────────────────────────────────────────────────────────────────
_stm_exit() {
    _stm_monitor_close
    printf "Disconnected\n"
}

# ─── Main dispatcher ──────────────────────────────────────────────────────────
stm() {
    local cmd="$1"
    local sub="$2"
    local arg="$3"

    case "$cmd" in
        led)
            case "$sub" in
                on)  _stm_led_on ;;
                off) _stm_led_off ;;
                *)   printf "Usage: stm led [on|off]\n" ;;
            esac
            ;;
        connect)
            _stm_uart_connect
            ;;
        monitor)
            case "$sub" in
                open)  _stm_monitor_open ;;
                close) _stm_monitor_close ;;
                *)     printf "Usage: stm monitor [open|close]\n" ;;
            esac
            ;;
        bootloader)
            case "$sub" in
                jump)   _BTLD_JUMP ;;
                update) _BTLD_UPDATE "$arg" ;;
                *)      printf "Usage: stm bootloader [jump|update <file.bin>]\n" ;;
            esac
            ;;
        build)
            _stm_build "$sub"
            ;;
        flash)
            _stm_flash "$sub"
            ;;   
        version)
            _stm_sw_version
            ;;
        time)
            _stm_time
            ;;        
        game)
            _stm_game_input
            ;;
        send)
            if [ -z "$sub" ]; then
                printf "Usage: stm send <0xHH>\n"
            else
                _stm_send_byte "$sub" && printf "[STM] Sent $sub\n"
            fi
            ;;
        clean)
            _stm_clean
            ;;
        exit)
            _stm_exit
            ;;
        *)
            printf "Usage: stm [led|connect|monitor|bootloader|build|flash|send|clean|exit]\n"
            printf "\n"
            printf "  led        [on|off]\n"
            printf "  connect\n"
            printf "  monitor    [open|close]\n"
            printf "  bootloader [jump|update <file.bin>]\n"
            printf "  build      [all|app|bm|btld]\n"
            printf "  flash      [all|app|bm|btld]\n"
            printf "  game               (interactive WASD input)\n"
            printf "  send       <0xHH>  (raw byte to UART2)\n"
            printf "  clean\n"
            printf "  exit\n"
            ;;
    esac
}
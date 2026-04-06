import serial
import time

# PORT  = '/dev/ttyACM0'
PORT  = 'COM5' 
BAUD  = 115200
INPUT = '../APP/Tools/APP_BTLD.bin'  # bin created by the other script

# ─── Read binary ──────────────────────────────────────────
with open(INPUT, 'rb') as f:
    payload = f.read()

# ─── Send over UART ───────────────────────────────────────
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(0.5)

print(f"Sending {len(payload)} bytes...")
ser.write(b'\x55')  
ser.write(payload)
print("Done!")
ser.close()
import serial
import time
import sys

PORT = '/dev/ttyACM0'
BAUD = 115200
FILE = sys.argv[1]

with open(FILE, 'rb') as f:
    data = f.read()

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)  # ← wait for BTLD to initialize!

print(f"Sending {len(data)} bytes...")
ser.write(data)
print("Done!")
ser.close()
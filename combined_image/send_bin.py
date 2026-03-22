import serial
import time
import struct
import zlib

PORT = '/dev/ttyACM0'
BAUD = 115200
# ─── Config ───────────────────────────────────────────────
INPUT  = '../APP/Tools/application.bin'   # <-- change this
OUTPUT = '../APP/Tools/application_with_header.bin'    # <-- saved in current folder

with open(INPUT, 'rb') as f:
    app_data = f.read()


# --- Build 4-byte header ---
MAGIC     = 0xDEAD       # 2 bytes: identify valid firmware
APP_SIZE  = len(app_data) # 2 bytes: size of the app
crc = zlib.crc32(app_data) & 0xFFFFFFFF
## HH 2 bytes each, little-endian
## II 4 bytes each, little-endian

header = struct.pack('<III', MAGIC, crc , APP_SIZE)  # little-endian: [MAGIC(2)] [SIZE(2)]

# --- Merge header + binary ---
payload = header + app_data

print(f"Header : MAGIC=0x{MAGIC:04X},CRC=0x{crc:08X}, SIZE={APP_SIZE} bytes")
print(f"Payload: {len(payload)} bytes (header + app)")


with open(OUTPUT, 'wb') as f:
    f.write(payload)
# --- Send over UART ---
ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)  # wait for bootloader to initialize

print(f"Sending {len(payload)} bytes...")
ser.write(payload)
print("Done!")
ser.close()
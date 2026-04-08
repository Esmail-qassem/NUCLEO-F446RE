# STM32F446RE Bare-Metal Firmware Stack

A complete bare-metal embedded firmware stack for the **ST NUCLEO-F446RE** development board, built on the **STM32F446RE (ARM Cortex-M4)** microcontroller.

This project demonstrates a full embedded firmware architecture from scratch — no vendor HAL or Cube libraries. Every peripheral is driven through direct register-level programming. The firmware is split into three independent images: a **Boot Manager (BM)**, a **Bootloader (BTLD)**, and an **Application (APP)**, each residing in a dedicated flash region.

---

## ✨ Highlights

- ✅ Full bare-metal firmware stack — no HAL, no CubeMX
- ✅ Direct register-level peripheral drivers
- ✅ Three-stage boot architecture: BM → BTLD → APP
- ✅ Custom bootloader supporting `.bin` and `.hex` over UART
- ✅ Structured 12-byte firmware header (MAGIC + CRC32 + SIZE)
- ✅ zlib-compatible CRC32 verification with lookup table
- ✅ Sync byte handshake (`0x55`) — rejects noise before transfer
- ✅ Wired OTA firmware update over UART at 115200 baud
- ✅ **Wireless OTA** via ESP8266 + RPi4 server — no PC required!
- ✅ Firmware version embedded in binary — auto-detected on upload
- ✅ Custom bare-metal preemptive RTOS with real context switching (PendSV + SVC)
- ✅ Modular MCAL + HAL driver architecture
- ✅ Makefile-based build system with custom linker scripts
- ✅ Full Linux development workflow (build, flash, debug)

---

## 🧠 Firmware Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Flash Memory                      │
├──────────────┬──────────────┬───────────────────────┤
│  BM (16 KB)  │ BTLD (16 KB) │     APP (480 KB)      │
│ 0x08000000   │ 0x08004000   │     0x08008000        │
└──────────────┴──────────────┴───────────────────────┘
│                   RAM (128 KB)                      │
│              0x20000000 - 0x20020000                │
└─────────────────────────────────────────────────────┘
```

---

## 🔄 Boot Flow

```
Power ON / SW Reset
        │
        ▼
┌───────────────────────────────────────┐
│         BM (Boot Manager)             │
│         @ 0x08000000                  │
│                                       │
│  Reads RCC reset flags:               │
│  • PowerReset → jump to APP           │
│  • SwReset    → jump to APP           │
│  • PinReset   → jump to BTLD          │
└───────────────────────────────────────┘
        │                    │
        │ PinReset            │ PowerReset / SwReset
        ▼                    ▼
┌──────────────┐    ┌─────────────────────────────────┐
│     BTLD     │    │            APP                  │
│ @ 0x08004000 │    │        @ 0x08008000             │
│              │    │                                 │
│ • Waits for  │    │ • Sets VTOR = 0x08008000        │
│   0x55 sync  │    │ • Starts RTOS scheduler         │
│ • Validates  │    │ • Runs application tasks        │
│   MAGIC+CRC  │    │ • UART logging @ 115200         │
│ • Erases     │    │ • ESP WiFi communication        │
│   APP flash  │    │ • Responds to 0xA1 version cmd  │
│ • Writes fw  │    └─────────────────────────────────┘
│ • CRC verify │
│ • SW Reset   │──► BM sees SwReset ──► jumps to APP ✅
└──────────────┘
```

### Reset Flag Logic

| Reset Type | Cause | BM Action |
|------------|-------|-----------|
| `PowerReset` | Power on / unplug-replug | Jump to APP |
| `SwReset` | Software reset (SYSRESETREQ) | Jump to APP |
| `PinReset` | NRST pin / reset button | Jump to BTLD |

> **Note:** `st-flash --reset` triggers a **pin reset** → goes to BTLD.
> OpenOCD `reset run` triggers a **SW reset** → goes to APP.
> This is why pressing F5 in VS Code works correctly.

---

## 📦 BTLD Firmware Protocol

The bootloader uses a structured protocol for reliable firmware transfer over UART.

### 12-Byte Header Format

Every firmware image sent to BTLD must include a 12-byte header prepended to the raw binary:

```
┌─────────────┬─────────────┬─────────────┬──────────────────┐
│  MAGIC      │  CRC32      │  SIZE       │  Firmware Data   │
│  4 bytes    │  4 bytes    │  4 bytes    │  N bytes         │
│  0x0000DEAD │  zlib CRC32 │  app size   │  raw .bin        │
└─────────────┴─────────────┴─────────────┴──────────────────┘
  little-endian throughout
```

| Field | Size | Value | Description |
|-------|------|-------|-------------|
| MAGIC | 4 bytes | `0x0000DEAD` | Identifies valid firmware packet |
| CRC32 | 4 bytes | zlib CRC32 | Checksum of raw firmware data only |
| SIZE | 4 bytes | N bytes | Size of raw firmware in bytes |

### Transfer Protocol

```
Sender                          BTLD
  │                               │
  │── 0x55 (sync byte) ─────────► │  All bytes before 0x55 ignored ✅
  │── MAGIC  (4 bytes) ─────────► │  Validates 0x0000DEAD
  │── CRC32  (4 bytes) ─────────► │  Stores expected CRC
  │── SIZE   (4 bytes) ─────────► │  Knows exact transfer size
  │── Firmware data    ─────────► │  Writes to flash 4 bytes at a time
  │                               │  Calculates CRC32 after last byte
  │                               │  CRC match → Verification → SW reset
  │                               │  BM → APP ✅
```

### BTLD Command Bytes

| Byte | Sender | Handled by | Description |
|------|--------|-----------|-------------|
| `0x55` | PC / ESP | BTLD | Sync byte — starts firmware transfer |
| `0xA1` | ESP | APP | Version request — ignored by BTLD |

### CRC32 Implementation

- Algorithm: CRC32 with polynomial `0xEDB88320`
- Compatible with Python's `zlib.crc32()`
- Implemented as software lookup table (256 entries) — ~8x faster than bit-by-bit
- Calculated over raw firmware only (not the 12-byte header)

### Generate Header (Python)

```python
import struct, zlib, re

with open('application.bin', 'rb') as f:
    app_data = f.read()

MAGIC    = 0xDEAD
crc      = zlib.crc32(app_data) & 0xFFFFFFFF
APP_SIZE = len(app_data)

# Auto-detect version from binary
matches = re.findall(rb'\d+\.\d+\.\d+', app_data)
VERSION = matches[0].decode() if matches else "1.0.0"

header  = struct.pack('<III', MAGIC, crc, APP_SIZE)
payload = header + app_data

with open('APP_BTLD.bin', 'wb') as f:
    f.write(payload)

print(f"Version : {VERSION}")
print(f"CRC32   : 0x{crc:08X}")
print(f"Size    : {APP_SIZE} bytes")
```

---

## 🗂️ Repository Structure

```
NUCLEO-F446RE/
│
├── BM/                         # Boot Manager
│   ├── Build/                  # Makefile, makeconfig, Linker/
│   ├── Inc/                    # Header files
│   ├── MCAL/                   # Peripheral drivers
│   ├── Src/                    # BM main source
│   ├── Startup/                # Startup code & vector table
│   └── Tools/                  # Output: BM.bin, BM.elf
│
├── BTLD/                       # Bootloader
│   ├── Build/                  # Makefile, makeconfig, Linker/
│   ├── Inc/                    # Header files
│   ├── MCAL/                   # Peripheral drivers
│   ├── Parse/                  # Firmware parser module
│   │   └── src/Parse.c         # Header validation, CRC32, flash write
│   ├── Src/                    # BTLD main source
│   ├── Startup/                # Startup code & vector table
│   └── Tools/                  # Output: BTLD.bin, BTLD.elf
│
├── APP/                        # Application
│   ├── Build/                  # Makefile, makeconfig, Linker/
│   ├── Inc/                    # Header files
│   ├── MCAL/                   # Peripheral drivers
│   │   ├── RCC/
│   │   ├── GPIO/
│   │   ├── NVIC/
│   │   ├── SysTick/
│   │   ├── UART/
│   │   ├── SPI/
│   │   ├── I2C/
│   │   ├── DMA/
│   │   ├── FLASH/
│   │   ├── BASIC_TIMER/
│   │   ├── GP_TIMER/
│   │   ├── IWDG/
│   │   ├── EXTI/
│   │   ├── AFIO/
│   │   └── PwrMD/
│   ├── HAL/
│   │   ├── ESP/                # ESP WiFi module
│   │   └── OLED/               # OLED display
│   ├── P1/
│   │   └── FLAPPY_Bird/        # Flappy Bird game on OLED
│   ├── RTOS/                   # Custom bare-metal RTOS scheduler
│   ├── Src/                    # Application main source
│   ├── Startup/                # Startup code & vector table
│   └── Tools/                  # Output: application.bin, application.elf
│
├── combined_image/             # Combined image tools
│   ├── build.sh                # Build BM + BTLD + APP
│   ├── FLASH.sh                # Flash combined image to board
│   ├── D.sh                    # Clean all build outputs
│   ├── full_image.hex          # Generated combined hex
│   └── send_bin.py             # Generates header binary + sends over UART
│
├── ESP_OTA/
│   └── ESP_OTA.ino             # ESP8266 wireless OTA client
│
├── upload_firmware.sh          # Upload firmware to RPi4 server
│
├── state_machine.pdf           # RTOS task state machine diagram
├── uml.pdf                     # RTOS architecture + UML sequence diagram
│
├── .vscode/
│   ├── launch.json             # Cortex-Debug configuration
│   └── tasks.json              # VS Code build task
│
└── README.md
```

---

## ⚙️ Custom Preemptive RTOS

The application runs on a custom bare-metal RTOS built from scratch on Cortex-M4 hardware primitives. It is **preemptive** — tasks are switched by the CPU itself using PendSV, not called as regular functions.

### How It Works

```
SysTick (1ms)
    │
    └─► Scheduler()
            │  counts down each task's remaining_ticks
            │  when ticks == 0 → state = READY
            └─► Trigger_PendSV()
                    │
                    └─► PendSV_Handler()   (lowest-priority ISR — fires after SysTick exits)
                            │  STMDB  R4-R11  onto CurrentTask's PSP  (save context)
                            │  LDMIA  R4-R11  from NextTask's PSP     (load context)
                            └─► BX LR  →  CPU jumps to NextTask's PC  (context switched)
```

### Task States

```
CreateTask()
    │
    ▼
SUSPENDED ──(ticks==0)──► READY ──(PendSV)──► [RUNNING]
    ▲                                               │
    │◄──────────── TaskFunc() returns ──────────────┘
    │              (trampoline suspends task,
    │               yields to Idle via PendSV)
    │
    ├── WaitEvent()  ──► WAITING ──(ResumeTask())──► SUSPENDED
    └── DeleteTask() ──► REMOVED
```

### Key Design Decisions

| Feature | Detail |
|---------|--------|
| **Context saved** | R4–R11 manually by PendSV; R0–R3, R12, LR, PC, xPSR by CPU hardware |
| **Stack per task** | 128 words (512 bytes) private stack in RAM — tasks fully isolated |
| **Initial stack frame** | Fake exception frame built at `CreateTask()` — SVC/PendSV can "return" into a task that never ran |
| **Task trampoline** | `RTOS_TaskEntry()` wraps every user function — catches return, suspends task safely |
| **First task launch** | `SVC #0` → SVC_Handler sets PSP, switches CONTROL to Thread/PSP, EXC_RETURNs into idle |
| **Idle task** | Always runnable — `NOP` loop with its own stack; runs when all user tasks are suspended |
| **Scheduler in ISR** | Runs directly inside SysTick ISR — no polling loop, no missed ticks |
| **Priority = index** | `SysTask[0]` = highest priority (runs first when multiple tasks expire together) |

### Stack Memory Layout

```
HIGH ADDRESS ──► stack[127]
┌───────────┐
│  xPSR     │  0x01000000 (Thumb bit)       ┐
│  PC       │  → RTOS_TaskEntry             │  Hardware frame
│  LR       │  0xFFFFFFFD (EXC_RETURN)      │  auto-saved/restored by CPU
│  R12      │  0                            │  on exception entry/exit
│  R3–R0    │  0, 0, 0, priority            ┘
├───────────┤  ← CPU boundary
│  R11–R4   │  0, 0, 0, 0, 0, 0, 0, 0      ← Software frame
├───────────┤  ← stack_pointer after CreateTask()
│  (free)   │  ← grows down as task calls functions
LOW ADDRESS ──► stack[0]
```

### RTOS Files

| File | Role |
|------|------|
| `APP/RTOS/RTOS.h` | Types (`TCB_t`, `Task_t`, states), public API |
| `APP/RTOS/RTOS.c` | Kernel: scheduler, context switch, SVC/PendSV handlers, trampoline |
| `APP/MCAL/PendSV/` | `PendSV_Init()` (sets lowest priority), `Trigger_PendSV()` (sets SCB_ICSR bit) |

### Diagrams

See the included PDFs at the root of this repo:

- **[state_machine.pdf](state_machine.pdf)** — Full task state machine with all transitions and triggers
- **[uml.pdf](uml.pdf)** — 2-page UML: system architecture (component diagram) + boot-to-tasks sequence diagram

---

## 🛠️ Prerequisites

### Tools

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch
sudo apt install make stlink-tools openocd srecord picocom
pip install pyserial --break-system-packages
```

### USB Permissions (run once)

```bash
sudo usermod -aG dialout $USER
sudo usermod -aG plugdev $USER
sudo nano /etc/udev/rules.d/49-stlink.rules
```

Add:
```
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="0666", GROUP="plugdev"
```

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Log out and back in.

### VS Code Extensions

- **Cortex-Debug** by marus25
- **C/C++** by Microsoft
- **Serial Monitor** by Microsoft

---

## 🚀 Getting Started

```bash
# Clone
git clone https://github.com/Esmail-qassem/NUCLEO-F446RE.git
cd NUCLEO-F446RE

# Make scripts executable
chmod +x flash.sh
chmod +x upload_firmware.sh
chmod +x combined_image/FULL_IMAGE.sh
chmod +x combined_image/FLASH.sh
chmod +x combined_image/clean_all.sh
chmod +x BM/Build/m.sh
chmod +x BTLD/Build/m.sh
chmod +x APP/Build/m.sh

# Open in VS Code
code .
```

---

## 🔨 Build

```bash
# Build all (BM + BTLD + APP + combined image)
./combined_image/FULL_IMAGE.sh

# Build individual image
cd APP/Build  && ./m.sh
cd BM/Build   && ./m.sh
cd BTLD/Build && ./m.sh

# Clean all
./combined_image/clean_all.sh
```

---

## ⚡ Flash

```bash
# Flash full system (BM + BTLD + APP)
./combined_image/FLASH.sh

# Update individual image
./flash.sh update_app       # flashes APP  @ 0x08008000
./flash.sh update_btld      # flashes BTLD @ 0x08004000
./flash.sh update_bm        # flashes BM   @ 0x08000000

# Test a single image standalone
./flash.sh test_app         # flash APP alone  @ 0x08000000
./flash.sh test_btld        # flash BTLD alone @ 0x08000000
./flash.sh test_bm          # flash BM alone   @ 0x08000000

# Erase chip
./flash.sh erase
```

---

## 📡 Wired OTA Firmware Update

### How it works

```
PC                                      STM32
│                                          │
│  1. Press reset button                   │
│  ───────────────────────────────────►    │ BM: PinReset → BTLD
│                                          │ BTLD: erases APP sector
│                                          │ BTLD: waits for 0x55 sync
│                                          │
│  2. python3 send_bin.py                  │
│     0x55 + header(12B) + firmware ────► │ BTLD: validates MAGIC
│                                          │ BTLD: reads CRC + SIZE
│                                          │ BTLD: writes to flash
│                                          │ BTLD: CRC32 verify ✅
│                                          │ BTLD: SW reset → APP ✅
```

### Wired OTA Usage

```bash
# Step 1 — Build APP
cd APP/Build && ./m.sh

# Step 2 — Generate header binary (auto-detects version)
python3 combined_image/BTLD_APP_IMAGE.py

# Step 3 — Press reset button on board, then run again to send
python3 combined_image/send_bin.py
```

**Expected BTLD output:**
```
BTLD
CRC OK!
Verification OK!
```

---

## 📡 Wireless OTA Firmware Update

### System Architecture

```
┌──────────┐   HTTP POST    ┌──────────────┐   HTTP GET    ┌──────────────┐
│  Your PC │ ─────────────► │  RPi4 Server │ ◄──────────── │  ESP8266     │
│          │  upload_       │  Flask API   │               │  NodeMCU     │
│          │  firmware.sh   │  port 5000   │               │              │
└──────────┘                └──────────────┘               └──────┬───────┘
                                                                  │ UART 115200
                                                                  │ NRST pin
                                                                  ▼
                                                           ┌──────────────┐
                                                           │  STM32F446   │
                                                           │  BTLD → APP  │
                                                           └──────────────┘
```

### Wireless OTA Flow

```
1. Build APP → generate APP_BTLD.bin (with header)
2. Run upload_firmware.sh → uploads to RPi4
3. ESP polls RPi4 every 30s → GET /version
4. ESP sends 0xA1 to STM32 APP → gets version string
5. Version mismatch → ESP downloads firmware to RAM
6. ESP pulls NRST LOW → pin reset → BM → BTLD
7. ESP sends: 0x55 + header + firmware over UART
8. BTLD: validates MAGIC → writes flash → CRC verify → SW reset
9. BM → APP runs with new firmware ✅
10. ESP confirms version → notifies RPi4
```

### ESP8266 Hardware Connections

| ESP8266 Pin | Connects to | Purpose |
|-------------|-------------|---------|
| `TX` | STM32 UART1 RX (PA10) | Send firmware + commands |
| `RX` | STM32 UART1 TX (PA9) | Receive version string |
| `D1` (GPIO5) | STM32 NRST | Trigger pin reset → BTLD |
| `GND` | STM32 GND | Common ground |
| `VIN` | 5V | Power supply |

> ⚠️ All signal pins are 3.3V — no level shifter needed.
> ⚠️ NodeMCU EN pin is internally pulled up — do not connect externally.
> ⚠️ Disconnect TX/RX when flashing new ESP firmware via USB.

### Firmware Version in APP

```c
// In APP/Src/main.c
const char FIRMWARE_VERSION[] = "2.0.0";

// Responds to ESP version request
void UART1_ISR(uint8 num)
{
    if (num == 0xA1)
        UART_SendSyncBuffer(UART1, (uint8*)FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION));
}
```

### RPi4 Server Setup

```bash
pip3 install flask --break-system-packages
mkdir ~/ota_server && cd ~/ota_server
python3 server.py  # runs on port 5000
```

### RPi4 Server API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/version` | GET | Returns current firmware version |
| `/upload` | POST | Upload new `APP_BTLD.bin` + version |
| `/firmware` | GET | Download latest firmware |
| `/status` | POST | ESP reports update result |

### Upload New Firmware

```bash
# Step 1 — Build + generate header
cd APP/Build && ./m.sh
python3 combined_image/BTLD_APP_IMAGE.py

# Step 2 — Upload to RPi4
./upload_firmware.sh
```

```
Enter server IP: 192.168.1.xx
Auto-detected version: 2.1.0
Uploading v2.1.0 to http://192.168.1.xx:5000...
Firmware v2.1.0 uploaded successfully!
Done! ESP will pick it up within 30 seconds.
```

### ESP8266 Configuration

```cpp
#define WIFI_SSID        "your_wifi_name"
#define WIFI_PASSWORD    "your_password"
#define SERVER_IP        "192.168.1.xx"   // RPi4 IP
#define POLL_INTERVAL_MS 30000           // poll every 30s
#define RESET_PIN        D1              // connected to STM32 NRST
#define CMD_GET_VERSION  0xA1            // version request command
```

---

## 🐛 Debug

```bash
./combined_image/FLASH.sh
```

Press **F5** in VS Code to build, flash and attach debugger.

| Key | Action |
|-----|--------|
| `F5` | Continue |
| `F10` | Step over |
| `F11` | Step into |
| `Shift+F11` | Step out |
| `Shift+F5` | Stop |

---

## 📺 Serial Monitor

| Interface | Baud Rate | Purpose |
|-----------|-----------|---------|
| UART1 | 115200 | ESP communication + BTLD receive |
| UART2 | 115200 | Debug output |

```bash
picocom -b 115200 /dev/ttyACM0
```

Exit: `Ctrl+A` then `Ctrl+X`

---

## 🔧 Build System

Compiler flags:
```
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
-O0 -g3 -Wall -std=gnu11 -ffunction-sections -fdata-sections
```

---

## 👤 Author

**Esmail Qassem Gomma**
Embedded Software Engineer
GitHub: [@Esmail-qassem](https://github.com/Esmail-qassem/NUCLEO-F446RE)

---

## 📄 License

This project is open source.
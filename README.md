# STM32F446RE Bare-Metal Firmware Stack

A complete bare-metal embedded firmware stack for the **ST NUCLEO-F446RE** development board, built on the **STM32F446RE (ARM Cortex-M4)** microcontroller.

This project demonstrates a full embedded firmware architecture from scratch — no vendor HAL or Cube libraries. Every peripheral is driven through direct register-level programming. The firmware is split into three independent images: a **Boot Manager (BM)**, a **Bootloader (BTLD)**, and an **Application (APP)**, each residing in a dedicated flash region.

---

## ✨ Highlights

- ✅ Full bare-metal firmware stack — no HAL, no CubeMX
- ✅ Direct register-level peripheral drivers
- ✅ Three-stage boot architecture: BM → BTLD → APP
- ✅ Custom bootloader supporting `.bin` and `.hex` over UART
- ✅ OTA firmware update over UART at 115200 baud (~1.17 seconds)
- ✅ Custom bare-metal RTOS scheduler
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
┌──────────────┐    ┌───────────────────────────────────┐
│     BTLD     │    │            APP                    │
│ @ 0x08004000 │    │        @ 0x08008000               │
│              │    │                                   │
│ • Receives   │    │ • Sets VTOR = 0x08008000          │
│   .bin/.hex  │    │ • Starts RTOS scheduler           │
│   over UART  │    │ • Runs application tasks          │
│ • Erases     │    │ • UART logging @ 921600 or 115200 │
│   APP flash  │    │ • ESP WiFi communication          │
│ • Writes new │    │ • OLED display                    │
│   firmware   │    └───────────────────────────────────┘
│ • Verifies   │
│ • SW Reset   │──► BM sees SwReset ──► jumps to APP
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
│   ├── Parse/                  # .bin / .hex parser module
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
│   ├── FULL_IMAGE.sh           # Build BM + BTLD + APP
│   ├── FLASH.sh                # Flash combined image to board
│   ├── clean_all.sh            # Clean all build outputs
│   ├── full_image.hex          # Generated combined hex
│   └── send_bin.py             # OTA firmware update script
│
├── .vscode/
│   ├── launch.json             # Cortex-Debug configuration
│   └── tasks.json              # VS Code build task
│
└── README.md
```

---

## 🛠️ Prerequisites

### Tools

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch
sudo apt install make stlink-tools openocd srecord picocom
pip install pyserial --break-system-packages
```


## 🚀 Getting Started

```bash
# Clone
git clone https://github.com/Esmail-qassem/NUCLEO-F446RE.git
cd NUCLEO-F446RE

# Make scripts executable
chmod +x flash.sh
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

# Update individual image (full system already on chip)
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

## 📡 OTA Firmware Update

The bootloader supports receiving a new `.bin` firmware image over UART and flashing it to the APP region — **no ST-Link required**.

### How it works

```
PC                              STM32
│                                  │
│  1. Press reset button           │
│  ───────────────────────────►    │ BM: PinReset → jump to BTLD
│                                  │
│  2. python3 send_bin.py app.bin  │
│  ───────────────────────────►    │ BTLD: receive bytes @ 115200
│                                  │ BTLD: write 4 bytes at a time to flash
│                                  │ BTLD: verify stack ptr + reset handler
│                                  │ BTLD: SW reset
│                                  │ BM: SwReset → jump to APP ✅
```

### Verification

After receiving the full image, BTLD verifies the first 8 bytes of the APP region:

| Address | Expected value | Meaning |
|---------|---------------|---------|
| `0x08008000` | `0x20000000` – `0x20020000` | Valid stack pointer in RAM |
| `0x08008004` | `0x08008000` – `0x08080000` | Valid reset handler in APP flash |

If both pass → soft reset → APP runs ✅
If either fails → BTLD stays waiting for retry ❌

### Send firmware

```bash
# Install pyserial (once)
pip install pyserial --break-system-packages

# Press reset button on board to enter BTLD, then:
python3 send_bin.py APP/Tools/application.bin
```

**Expected output:**
```
Sending 13532 bytes...
Done!
```

**Serial monitor (115200 baud):**
```
BTLD
Verification OK!
```

Transfer time: **~1.17 seconds** at 115200 baud.

---

## 🐛 Debug

1. Flash the full system:
```bash
./combined_image/FLASH.sh
```

2. Open VS Code and press **F5** — it will build, flash and attach the debugger at `main()`

| Key | Action |
|-----|--------|
| `F5` | Continue |
| `F10` | Step over |
| `F11` | Step into |
| `Shift+F11` | Step out |
| `Shift+F5` | Stop |

To watch a variable: open **Run & Debug** sidebar → **WATCH** panel → click `+` → type variable name.

> **Note:** F5 uses OpenOCD which triggers SW reset → BM jumps to APP correctly.
> `st-flash --reset` triggers pin reset → BM jumps to BTLD instead.

---

## 📺 Serial Monitor

| Interface | Baud Rate | Purpose |
|-----------|-----------|---------|
| UART2 | 921600 | APP logging / debug output |
| UART2 | 115200 | BTLD firmware receive |

```bash
# Monitor APP output
picocom -b 921600 /dev/ttyACM0

# Monitor BTLD output
picocom -b 115200 /dev/ttyACM0
```

Exit: `Ctrl+A` then `Ctrl+X`

---

## 🔧 Build System

Each image (BM, BTLD, APP) has its own:
- `Makefile` — build rules, source/include paths, compiler flags
- `makeconfig` — toolchain and path configuration
- `Linker/LinkerScript.ld` — custom linker script with correct flash origin
- `m.sh` — build wrapper script

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

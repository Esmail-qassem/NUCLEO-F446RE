# STM32F446RE Bare-Metal Firmware Stack

A complete bare-metal embedded firmware stack for the **ST NUCLEO-F446RE** development board, built on the **STM32F446RE (ARM Cortex-M4)** microcontroller.

This project demonstrates a full embedded firmware architecture from scratch — no vendor HAL or Cube libraries. Every peripheral is driven through direct register-level programming. The firmware is split into three independent images: a **Boot Manager (BM)**, a **Bootloader (BTLD)**, and an **Application (APP)**, each residing in a dedicated flash region.

---

## ✨ Highlights

- ✅ Full bare-metal firmware stack — no HAL, no CubeMX
- ✅ Direct register-level peripheral drivers
- ✅ Three-stage boot architecture: BM → BTLD → APP
- ✅ Custom bootloader supporting `.bin` and `.hex` over UART
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
```

### Boot Flow

```
Reset
  │
  ▼
BM (Boot Manager) @ 0x08000000
  │  • Checks power reset flag
  │  • Decides: firmware update or run application
  │
  ├──[Update requested]──► BTLD (Bootloader) @ 0x08004000
  │                           │  • Receives .bin or .hex over UART
  │                           │  • Erases target flash region
  │                           │  • Writes and verifies new image
  │                           │  • Jumps to APP after update
  │
  └──[No update]────────► APP (Application) @ 0x08008000
                              │  • Sets VTOR to 0x08008000
                              │  • Starts custom RTOS scheduler
                              │  • Runs application tasks
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
│   └── full_image.hex          # Generated combined hex
│
├── flash.sh                    # Master flash script (all modes)
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
```

### USB Permissions (run once)

```bash
sudo usermod -aG dialout $USER
sudo usermod -aG plugdev $USER
sudo nano /etc/udev/rules.d/49-stlink.rules
```
### VS Code Extensions

- **Cortex-Debug** by marus25
- **Serial Monitor** by Microsoft

---

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

---

## 📡 Serial Monitor

UART2 @ 921600 baud** on `/dev/ttyACM0`

```bash
picocom -b 921600 /dev/ttyACM0
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

**Esmail Qassem Qassem Gomma  
Embedded Software Engineer  
GitHub: [@Esmail-qassem](https://github.com/Esmail-qassem/NUCLEO-F446RE)

---

## 📄 License

This project is open source.

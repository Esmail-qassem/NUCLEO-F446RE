#!/bin/bash

COMBINED_DIR=$(dirname $(realpath $0))

echo "Flashing combined image..."
#  st-flash --format ihex write $COMBINED_DIR/full_image.hex

#  echo "Resetting via SWD..."
#  openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
#      -c "init" \
#      -c "arm semihosting enable" \
#      -c "reset run" \
#      -c "exit"

"C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe" -c port=swd -d "C:\Users\s_a_a\Desktop\NUCLEO-F446RE\combined_image\full_image.hex" 0x08000000 -v -rst

echo "✅ Done!"

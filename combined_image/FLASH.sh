#!/bin/bash

COMBINED_DIR=$(dirname $(realpath $0))

echo "Flashing combined image..."
st-flash --format ihex write $COMBINED_DIR/full_image.hex

echo "Resetting via SWD..."
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "init" \
    -c "arm semihosting enable" \
    -c "reset run" \
    -c "exit"

echo "✅ Done!"
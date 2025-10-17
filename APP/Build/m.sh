#!/bin/bash

# Add ARM toolchain to PATH
export PATH="$PATH:/opt/gcc-arm-none-eabi-10-2021.10/bin"  # Adjust if installed elsewhere

# Handle command-line arguments
case "$1" in
    -j)
        make clean
        make all
        ;;
    clean)
        make clean
        ;;
    flash)
        make flash
        ;;
    erase)
        make erase
        ;;
    -a)
        export APPLICATION_ONLY=1
        make clean
        make all
        ;;
    *)
        echo "Usage: $0 [-j | clean | flash | erase | -a]"
        ;;
esac



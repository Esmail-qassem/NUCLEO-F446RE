#!/bin/bash
ROOT=$(dirname $(realpath $0))/..
COMBINED_DIR=$(dirname $(realpath $0))

printf "Building BM...\n"
cd $ROOT/BM/Build/
./m.sh -j

printf "Building BTLD...\n"
cd $ROOT/BTLD/Build/
./m.sh -j

printf "Building APP...\n"
cd $ROOT/APP/Build/
./m.sh -j

printf "Creating the Btld+App image...\n"
cd $ROOT/combined_image
python BTLD_APP_IMAGE.py

printf "Generating combined hex...\n"
cd $COMBINED_DIR
srec_cat \
    $ROOT/BM/Tools/BM.bin           -Binary -offset 0x08000000 \
    $ROOT/BTLD/Tools/BTLD.bin       -Binary -offset 0x08004000 \
    $ROOT/APP/Tools/application.bin -Binary -offset 0x08008000 \
    -o full_image.hex -Intel

printf "✅ Hex generated!"
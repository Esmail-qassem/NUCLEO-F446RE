ROOT=$(dirname $(realpath $0))/..
COMBINED_DIR=$(dirname $(realpath $0))

echo "Building BM..."
cd $ROOT/BM/Build/
./m.sh -j

echo "Building BTLD..."
cd $ROOT/BTLD/Build/
./m.sh -j

echo "Building APP..."
cd $ROOT/APP/Build/
./m.sh -j

echo "Generating combined hex..."
cd $COMBINED_DIR
srec_cat \
    $ROOT/BM/Tools/BM.bin           -Binary -offset 0x08000000 \
    $ROOT/BTLD/Tools/BTLD.bin       -Binary -offset 0x08004000 \
    $ROOT/APP/Tools/application.bin -Binary -offset 0x08008000 \
    -o full_image.hex -Intel

echo "✅ Hex generated!"
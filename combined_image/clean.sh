#!/bin/bash

ROOT=$(dirname $(realpath $0))/..

echo "Cleaning BM..."
cd $ROOT/BM/Build/
./m.sh clean

echo "Cleaning BTLD..."
cd $ROOT/BTLD/Build/
./m.sh clean

echo "Cleaning APP..."
cd $ROOT/APP/Build/
./m.sh clean

echo "✅ All cleaned!"

#!/bin/bash

BIN=~/Desktop/NUCLEO-F446RE/APP/Tools/application.bin

read -p "Enter version: " VERSION

echo "Uploading v$VERSION to http://192.168.1.12:5000..."

curl -X POST \
  -F "version=$VERSION" \
  -F "firmware=@$BIN" \
  http://192.168.1.12:5000/upload

echo ""
echo "Done! ESP will pick it up within 30 seconds."

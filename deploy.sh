#!/bin/bash
# Deploy dashboard to Raspberry Pi
# Usage: ./deploy.sh [pi_user]  (default user: pi)

RPI_IP="192.168.1.19"
RPI_USER="${1:-pi}"
RPI_PATH="~/ecu-dashboard"

echo "[DEPLOY] Copying dashboard to $RPI_USER@$RPI_IP:$RPI_PATH ..."

scp website/server.py                  "$RPI_USER@$RPI_IP:$RPI_PATH/server.py"
scp website/static/index.html          "$RPI_USER@$RPI_IP:$RPI_PATH/static/index.html"
scp website/static/app.js              "$RPI_USER@$RPI_IP:$RPI_PATH/static/app.js"
scp website/static/style.css           "$RPI_USER@$RPI_IP:$RPI_PATH/static/style.css"
scp website/static/socket.io.min.js    "$RPI_USER@$RPI_IP:$RPI_PATH/static/socket.io.min.js"
scp website/requirements.txt           "$RPI_USER@$RPI_IP:$RPI_PATH/requirements.txt"

if [ $? -ne 0 ]; then
  echo "[ERROR] Transfer failed — check SSH access to $RPI_IP"
  exit 1
fi

echo ""
echo "[DEPLOY] Restarting server on RPi..."
ssh "$RPI_USER@$RPI_IP" "pkill -f server.py; sleep 1; cd $RPI_PATH && nohup python3 server.py > server.log 2>&1 &"

echo "[DEPLOY] Done! Dashboard at http://$RPI_IP:5000"

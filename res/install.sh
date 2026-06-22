#!/bin/sh
# sms-forward device install script
# Run this on the r200 device after extracting the release package.

set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="/home/root/sms"

mkdir -p "$BIN_DIR"

cp "$DIR/sms-forward" "$BIN_DIR/sms-forward"
chmod +x "$BIN_DIR/sms-forward"

cp "$DIR/sms-forward-reload" "$BIN_DIR/sms-forward-reload"
chmod +x "$BIN_DIR/sms-forward-reload"

cp "$DIR/sms-forward.service" /lib/systemd/system/sms-forward.service

if [ ! -f "$BIN_DIR/config.json" ]; then
  cat > "$BIN_DIR/config.json" << 'CONFIG'
{
  "push": {
    "backendName": "wxpusher",
    "wxpusher": {
      "mode": "standard",
      "appToken": "",
      "uids": []
    }
  }
}
CONFIG
  echo "Created $BIN_DIR/config.json"
fi

systemctl daemon-reload
systemctl enable sms-forward
systemctl restart sms-forward

echo "sms-forward installed"

#!/bin/sh
# sms-forward release packager
# Run on the build machine. Produces build/sms-forward-YYYYMMDD.tar.gz.

set -e

cd "$(dirname "$0")"

echo "Building..."
xmake f
xmake clean
xmake

VERSION=$(date +%Y%m%d)
OUTDIR="build/sms-forward-release"

rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"

cp build/linux/armv7/release/sms-forward "$OUTDIR/"
cp res/sms-forward-reload "$OUTDIR/"
cp res/sms-forward.service "$OUTDIR/"
cp res/install.sh "$OUTDIR/"

chmod +x "$OUTDIR/sms-forward"
chmod +x "$OUTDIR/sms-forward-reload"
chmod +x "$OUTDIR/install.sh"

cat > "$OUTDIR/config.json" << 'CONFIG'
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

cd build
tar czf "sms-forward-${VERSION}.tar.gz" -C sms-forward-release .
rm -rf sms-forward-release

echo "Package: build/sms-forward-${VERSION}.tar.gz"

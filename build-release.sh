#!/bin/sh
# sms-forward release packager
# Run on the build machine. Produces build/sms-forward-x.x.x.tar.gz.

set -e

cd "$(dirname "$0")"

echo "Building..."
xmake f
xmake clean
xmake

VERSION=$(cat res/version.txt)
OUTDIR="build/sms-forward-release"

rm -rf "$OUTDIR"
mkdir -p "$OUTDIR"

cp build/linux/armv7/release/sms-forward "$OUTDIR/"
cp res/sms-forward-daemon "$OUTDIR/"
cp res/sms-forward-reload "$OUTDIR/"
cp res/sms-forward.service "$OUTDIR/"
cp res/install.sh "$OUTDIR/"
cp res/uninstall.sh "$OUTDIR/"

chmod +x "$OUTDIR/sms-forward"
chmod +x "$OUTDIR/sms-forward-daemon"
chmod +x "$OUTDIR/sms-forward-reload"
chmod +x "$OUTDIR/install.sh"
chmod +x "$OUTDIR/uninstall.sh"

cd build
tar czf "sms-forward-${VERSION}.tar.gz" -C sms-forward-release .
rm -rf sms-forward-release

echo "Package: build/sms-forward-${VERSION}.tar.gz"

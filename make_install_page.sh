#!/bin/sh
# HADAL-1's browser installer, from a release build (./build.sh --release 0.6), served on this computer.
set -e
cd "$(dirname "$0")"
V="${1:-1.0}"
mkdir -p app/build/emu app/site
[ -f app/build/emu/app.wasm ] || : > app/build/emu/app.wasm
[ -f app/site/landing.md ] || printf '# HADAL-1\n\nDark worlds that never repeat.\n\n[Install](install/)\n' > app/site/landing.md
python3 base/tools/make_site.py app "app/build/hadal-$V.fwsc" "$V" app/build/site
rm -f app/build/site/firmware/m-bootloader-*.fwsc
echo
echo "Open this in Chrome or Edge on Windows:  http://localhost:8765/install/"
echo "Press Ctrl+C here when the install has finished."
exec python3 -m http.server 8765 --bind 127.0.0.1 --directory app/build/site

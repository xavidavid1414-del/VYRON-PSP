#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ ! -f vyron_psp.elf || ! -f vyron_psp.3dsx ]]; then
  echo "Run 'make' before packaging the CIA." >&2
  exit 1
fi

mkdir -p assets build/cia-tools
python3 tools/generate_cia_assets.py

TOOLS_BIN="${DEVKITPRO:-/opt/devkitpro}/tools/bin"
mkdir -p "$TOOLS_BIN"

if ! command -v bannertool >/dev/null 2>&1; then
  echo "Building bannertool from its upstream source..."
  if [[ ! -d build/cia-tools/bannertool ]]; then
    git clone --depth 1 --recurse-submodules https://github.com/diasurgical/bannertool.git build/cia-tools/bannertool
  fi
  make -C build/cia-tools/bannertool
  BANNERTOOL_BIN="$(find build/cia-tools/bannertool/output -type f -name bannertool -perm -111 -print -quit)"
  if [[ -z "$BANNERTOOL_BIN" ]]; then
    echo "bannertool build did not produce an executable." >&2
    exit 1
  fi
  install -m 755 "$BANNERTOOL_BIN" "$TOOLS_BIN/bannertool"
fi

if ! command -v makerom >/dev/null 2>&1; then
  echo "Building makerom from its upstream source..."
  if [[ ! -d build/cia-tools/Project_CTR ]]; then
    git clone --depth 1 --recurse-submodules https://github.com/3DSGuy/Project_CTR.git build/cia-tools/Project_CTR
  fi
  MBEDTLS_CONFIG="build/cia-tools/Project_CTR/makerom/deps/libmbedtls/include/mbedtls/config.h"
  if [[ -f "$MBEDTLS_CONFIG" ]] && ! grep -q '^#define MBEDTLS_NO_PLATFORM_ENTROPY' "$MBEDTLS_CONFIG"; then
    python3 - "$MBEDTLS_CONFIG" <<'PY'
import sys
path = sys.argv[1]
with open(path, "r", encoding="utf-8") as handle:
    content = handle.read()
guard = "#define MBEDTLS_CONFIG_H"
if guard in content:
    content = content.replace(guard, guard + "\n#define MBEDTLS_NO_PLATFORM_ENTROPY", 1)
else:
    content = "#define MBEDTLS_NO_PLATFORM_ENTROPY\n" + content
with open(path, "w", encoding="utf-8") as handle:
    handle.write(content)
PY
  fi
    export CC=/usr/bin/gcc CXX=/usr/bin/g++ LD=/usr/bin/gcc AR=/usr/bin/ar
  make -C build/cia-tools/Project_CTR/makerom clean
  make -C build/cia-tools/Project_CTR/makerom deps
  make -C build/cia-tools/Project_CTR/makerom
  MAKEROM_BIN="$(find build/cia-tools/Project_CTR/makerom -type f -name makerom -perm -111 -print -quit)"
  if [[ -z "$MAKEROM_BIN" ]]; then
    echo "makerom build did not produce an executable." >&2
    exit 1
  fi
  install -m 755 "$MAKEROM_BIN" "$TOOLS_BIN/makerom"
fi

export PATH="$TOOLS_BIN:$PATH"
command -v bannertool >/dev/null
command -v makerom >/dev/null

bannertool makesmdh \
  -s "VYRON PSP" \
  -l "Experimental PSP emulator for Nintendo 3DS" \
  -p "VYRON PSP Project" \
  -i assets/vyron_icon.png \
  -o vyron_psp.icn

bannertool makebanner \
  -i assets/vyron_banner.png \
  -a assets/silence.wav \
  -o vyron_psp.bnr

makerom -f cia -o vyron_psp.cia \
  -DAPP_ENCRYPTED=false \
  -rsf config/vyron_psp.rsf \
  -target t -exefslogo -desc app:4 \
  -elf vyron_psp.elf \
  -icon vyron_psp.icn \
  -banner vyron_psp.bnr

test -s vyron_psp.cia
echo "CIA package created: vyron_psp.cia"

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
  install -m 755 build/cia-tools/bannertool/output/bannertool "$TOOLS_BIN/bannertool"
fi

if ! command -v makerom >/dev/null 2>&1; then
  echo "Building makerom from its upstream source..."
  if [[ ! -d build/cia-tools/Project_CTR ]]; then
    git clone --depth 1 --recurse-submodules https://github.com/3DSGuy/Project_CTR.git build/cia-tools/Project_CTR
  fi
  make -C build/cia-tools/Project_CTR/makerom deps
  make -C build/cia-tools/Project_CTR/makerom
  install -m 755 build/cia-tools/Project_CTR/makerom/makerom "$TOOLS_BIN/makerom"
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
  -target t -exefslogo \
  -elf vyron_psp.elf \
  -icon vyron_psp.icn \
  -banner vyron_psp.bnr

test -s vyron_psp.cia
echo "CIA package created: vyron_psp.cia"

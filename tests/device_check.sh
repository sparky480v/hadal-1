#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The whole device unity build, type-checked with any clang: it finds what the simulator can't, like a clash with base code.
set -eu
cd "$(dirname "$0")/.."
INC="-Iapp -Iapp/dsp -Iapp/build/gen -Ibase/include -Ibase/device -Ibase/lib $(find base/bsp -type d | sed 's/^/-I/')"
clang --target=arm-none-eabi -fsyntax-only -std=gnu99 -w -Werror=implicit-function-declaration -ferror-limit=0 \
  -DFM1B_CPU2=1 -DFM1_CPU1=1 -DFM1B_BUILD=1u -DFM1_CPU1_FALLBACK=1 -DFELUCCA_UAC_MIDI_LAST=1 $INC base/device/unity.c
echo "device check ok"

#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The browser emulator without Emscripten: clang's wasm32 target and wasm-ld. The real app on fm1-base's simulator.
set -e
cd "$(dirname "$0")"
G=app/build/gen; O=app/build/emu; mkdir -p $O
FL="--target=wasm32 -O2 -ffp-contract=off -std=gnu99 -nostdlib -w -DPLAT_HOST -DPLAT_WEB -DFM1B_BLE=0 -DFM1B_CPU2=1 -DFM1B_FONTS=1 -Itools/emu_shim -Ibase/host -Ibase/device -Ibase/include -Ibase/lib -Iapp -Iapp/dsp -I$G"
clang $FL -c app/dsp/hadal.c -o $O/hadal.o
clang $FL -c app/dsp/vis.c -o $O/vis.o
clang $FL -fno-builtin -O1 -c tools/emu_shim/libc.c -o $O/libc.o
clang $FL -Wl,--no-entry -Wl,--export-dynamic -Wl,--initial-memory=33554432 -Wl,-z,stack-size=1048576 -o $O/app.wasm base/web/emu/emu_web.c $O/hadal.o $O/vis.o $O/libc.o
ls -la $O/app.wasm

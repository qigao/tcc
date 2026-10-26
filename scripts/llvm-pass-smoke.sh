#!/usr/bin/env bash
set -euo pipefail
: "${LLVM_OBF_PLUGIN:?Provide the plugin built for exactly the same LLVM version as opt}"
CLANG=${CLANG:-clang-14}
OPT=${OPT:-opt-14}
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT=${OUT:-"$ROOT/build-llvm-obf"}
mkdir -p "$OUT"
"$CLANG" --version | head -n 1
"$OPT" --version | head -n 2
# -disable-O0-optnone permits opt's explicit new-PM function pass to run.
"$CLANG" -O0 -Xclang -disable-O0-optnone -S -emit-llvm \
    "$ROOT/tests/obfuscation/llvm_fixture.c" -o "$OUT/baseline.ll"
export LLVM_OBF_SEED=0xA04252B187478C00A40BC6D81D1A8A52
"$OPT" -load-pass-plugin "$LLVM_OBF_PLUGIN" -passes='function(flattening)' \
    -verify-each -S "$OUT/baseline.ll" -o "$OUT/obfuscated.ll"
if cmp -s "$OUT/baseline.ll" "$OUT/obfuscated.ll"; then
    echo 'LLVM pass produced identical IR; this is not a qualified obfuscation' >&2
    exit 5
fi
if ! grep -q 'switchVar\|loopEntry' "$OUT/obfuscated.ll"; then
    echo 'LLVM pass changed IR but expected flattening markers are missing' >&2
    exit 6
fi
"$CLANG" -O0 "$OUT/baseline.ll" -o "$OUT/baseline"
"$CLANG" -O0 "$OUT/obfuscated.ll" -o "$OUT/obfuscated"
for input in -501 -1 0 1 7 31 1024; do
    "$OUT/baseline" "$input" > "$OUT/plain.stdout"
    "$OUT/obfuscated" "$input" > "$OUT/obfuscated.stdout"
    diff -u "$OUT/plain.stdout" "$OUT/obfuscated.stdout"
done
printf 'LLVM pass: IR transformed and 7-input semantic parity PASS\n'

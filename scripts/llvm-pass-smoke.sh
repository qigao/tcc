#!/usr/bin/env bash
set -euo pipefail
: "${LLVM_OBF_PLUGIN:?Set LLVM_OBF_PLUGIN to a plugin built for the exact host Clang/LLVM version}"
CC=${CC:-clang}
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
mkdir -p "$root/build-llvm-obf"
"$CC" --version | head -n 1
"$CC" -O2 "$root/tests/obfuscation/semantic_smoke.c" -o "$root/build-llvm-obf/plain"
"$CC" -O2 -fpass-plugin="$LLVM_OBF_PLUGIN" "$root/tests/obfuscation/semantic_smoke.c" -o "$root/build-llvm-obf/transformed"
"$root/build-llvm-obf/plain" > "$root/build-llvm-obf/plain.stdout"
"$root/build-llvm-obf/transformed" > "$root/build-llvm-obf/transformed.stdout"
diff -u "$root/build-llvm-obf/plain.stdout" "$root/build-llvm-obf/transformed.stdout"
echo 'LLVM host pass smoke: semantic parity PASS (not proof that an obfuscation pass executed)'

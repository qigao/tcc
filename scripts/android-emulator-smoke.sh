#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
ABI="${ANDROID_TEST_ABI:-x86_64}"
case "$ABI" in
  x86_64) TRIPLE=x86_64-linux-android; ELF_MACHINE='Advanced Micro Devices X86-64'; GUEST_MACHINE=x86_64 ;;
  arm64-v8a) TRIPLE=aarch64-linux-android; ELF_MACHINE='AArch64'; GUEST_MACHINE=aarch64
    # Host ARM64 is only needed for the Apple-Silicon emulator case.
    # A real Android ARM64 device may be attached to a Linux x86-64 host.
    if [[ "${ANDROID_ARM64_HARDWARE:-0}" != 1 ]]; then
      test "$(uname -m)" = arm64 || { echo 'ARM64 Apple Silicon emulator host required' >&2; exit 2; }
    fi ;;
  *) echo "Unsupported Android ABI $ABI" >&2; exit 2 ;;
esac
OUT="build-android-$ABI"
mkdir -p "$OUT"
NDK="${ANDROID_NDK_HOME:-}"
if [[ ! -d "$NDK" ]]; then NDK="${ANDROID_HOME:?}/ndk/27.3.13750724"; fi
test -d "$NDK" || { echo "Android NDK missing: $NDK" >&2; exit 2; }
case "$(uname -s)" in
  Linux) BIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin" ;;
  Darwin) BIN="$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin"
    test -d "$BIN" || BIN="$NDK/toolchains/llvm/prebuilt/darwin-arm64/bin" ;;
  *) exit 2 ;;
esac
CC="$BIN/${TRIPLE}29-clang"
READELF="$BIN/llvm-readelf"
test -x "$CC" && test -x "$READELF"
BASE=/data/local/tmp/tcc_poc
adb shell getprop ro.product.cpu.abi | tr -d '\r' | tee "$OUT/adb-abi.txt"
adb shell uname -m | tr -d '\r' | tee "$OUT/adb-machine.txt"
adb shell getprop ro.build.version.sdk | tr -d '\r' | tee "$OUT/adb-api.txt"
grep -qx "$ABI" "$OUT/adb-abi.txt"
grep -qx "$GUEST_MACHINE" "$OUT/adb-machine.txt"
adb shell "mkdir -p $BASE"
for asset in tcc libtcc.so android_smoke; do
  test -s "$OUT/$asset"
  adb push "$OUT/$asset" "$BASE/$asset"
done
adb push tests/platform/android_tcc_object.c "$BASE/android_tcc_object.c"
adb push scripts/android-guest-smoke.sh "$BASE/guest.sh"
adb shell "sh $BASE/guest.sh" | tee "$OUT/adb-run.txt"
adb pull "$BASE/android_tcc_object.o" "$OUT/android_tcc_object.o"
file "$OUT/android_tcc_object.o"
"$READELF" -h "$OUT/android_tcc_object.o" | tee "$OUT/adb-tcc-object.txt"
grep -q "$ELF_MACHINE" "$OUT/adb-tcc-object.txt"
"$CC" -std=c11 -O0 tests/platform/android_object_client.c \
  "$OUT/android_tcc_object.o" -o "$OUT/android_tcc_consumer"
adb push "$OUT/android_tcc_consumer" "$BASE/android_tcc_consumer"
adb shell "chmod 700 $BASE/android_tcc_consumer && cd $BASE && ./android_tcc_consumer" |
  tee "$OUT/adb-consumer.txt"
grep -q 'Android: TinyCC native ELF object interoperated with NDK Clang PASS' "$OUT/adb-consumer.txt"
echo "Android $ABI native guest TinyCC -> ELF -> NDK Clang -> execution PASS"

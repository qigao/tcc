#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
NDK="${ANDROID_NDK_HOME:?}"
BIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
CC="$BIN/x86_64-linux-android29-clang"
BASE=/data/local/tmp/tcc_poc
mkdir -p build-android-x86_64
echo "=== DEVICE ==="
adb shell getprop ro.product.cpu.abi | tee build-android-x86_64/adb-abi.txt
adb shell getprop ro.build.version.sdk | tee build-android-x86_64/adb-api.txt
if ! grep -q x86_64 build-android-x86_64/adb-abi.txt; then
    echo 'Wrong emulator ABI: require actual x86_64, no architecture translation' >&2
    exit 2
fi
adb shell "mkdir -p $BASE"
for asset in tcc libtcc.so android_smoke; do
    adb push "build-android-x86_64/$asset" "$BASE/$asset"
done
adb push tests/platform/android_tcc_object.c "$BASE/android_tcc_object.c"
adb push scripts/android-guest-smoke.sh "$BASE/guest.sh"
adb shell "sh $BASE/guest.sh" | tee build-android-x86_64/adb-run.txt
# The object below must have been emitted by TinyCC *on Android*.
adb pull "$BASE/android_tcc_object.o" build-android-x86_64/android_tcc_object.o
file build-android-x86_64/android_tcc_object.o
"$BIN/llvm-readelf" -h build-android-x86_64/android_tcc_object.o |
    tee build-android-x86_64/adb-tcc-object.txt
"$CC" -std=c11 -O0 tests/platform/android_object_client.c \
    build-android-x86_64/android_tcc_object.o \
    -o build-android-x86_64/android_tcc_consumer
adb push build-android-x86_64/android_tcc_consumer "$BASE/android_tcc_consumer"
adb shell "chmod 700 $BASE/android_tcc_consumer && cd $BASE && ./android_tcc_consumer" |
    tee build-android-x86_64/adb-consumer.txt
grep -q 'Android: TinyCC native ELF object interoperated with NDK Clang PASS' \
    build-android-x86_64/adb-consumer.txt

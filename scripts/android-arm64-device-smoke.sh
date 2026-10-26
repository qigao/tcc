#!/usr/bin/env bash
# Trusted, isolated Android ARM64 physical-device acceptance gate.
# Requires USB debugging for a dedicated test device, not a personal phone.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
: "${ANDROID_NDK_HOME:?Pinned NDK is required}"
command -v adb >/dev/null
# Never silently select a random connected device.
if [[ -z "${ANDROID_SERIAL:-}" ]]; then
  mapfile -t devices < <(adb devices | awk 'NR > 1 && $2 == "device" {print $1}')
  if (( ${#devices[@]} != 1 )); then
    echo "Require exactly one authorized USB debugging device" >&2
    exit 2
  fi
  export ANDROID_SERIAL="${devices[0]}"
fi
if [[ ! "$ANDROID_SERIAL" =~ ^[A-Za-z0-9._:-]+$ ]] ||
   [[ "$ANDROID_SERIAL" == emulator-* ]]; then
  echo "Expected authorized real Android test device" >&2
  exit 2
fi
adb get-state | tr -d '\r' | grep -qx device
abi=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
machine=$(adb shell uname -m | tr -d '\r')
sdk=$(adb shell getprop ro.build.version.sdk | tr -d '\r')
emulator=$(adb shell getprop ro.kernel.qemu | tr -d '\r')
bootemu=$(adb shell getprop ro.boot.qemu | tr -d '\r')
if [[ "$abi" != arm64-v8a || "$machine" != aarch64 ||
      ! "$sdk" =~ ^[0-9]+$ || "$sdk" -lt 29 ||
      "$emulator" == 1 || "$bootemu" == 1 ]]; then
  echo "Reject ARM64 ABI mismatch, unsupported SDK or emulator" >&2
  exit 3
fi
# Avoid publishing serial numbers, IMEI, device names or device logcat.
echo "Accepted genuine ARM64 Android device, API $sdk"
test -s build-android-arm64-v8a/tcc
test -s build-android-arm64-v8a/libtcc.so
test -s android/apk-poc/app/build/outputs/apk/debug/app-debug.apk
BIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"
"$BIN/llvm-readelf" -h build-android-arm64-v8a/tcc |
    tee build-android-arm64-v8a/adb-tcc-provider-header.txt
grep -q 'Machine:.*AArch64' build-android-arm64-v8a/adb-tcc-provider-header.txt
export ANDROID_TEST_ABI=arm64-v8a
export ANDROID_ARM64_HARDWARE=1
bash scripts/android-emulator-smoke.sh
bash scripts/android-apk-sandbox-smoke.sh
echo "Android ARM64 physical-device TinyCC ELF + targetSdk35 JNI ET_REL PASS"

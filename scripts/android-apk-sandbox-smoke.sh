#!/usr/bin/env bash
set -euo pipefail
ABI="${ANDROID_TEST_ABI:-x86_64}"
APP=com.qigao.tccpoc
APK=android/apk-poc/app/build/outputs/apk/debug/app-debug.apk
OUT="build-android-$ABI"
case "$ABI" in
  x86_64) MACHINE='X86-64'; GUEST=x86_64 ;;
  arm64-v8a) MACHINE='AArch64'; GUEST=aarch64 ;;
  *) echo "unsupported ABI $ABI" >&2; exit 2 ;;
esac
test -s "$APK"
adb shell getprop ro.product.cpu.abi | tr -d '\r' | grep -qx "$ABI"
adb shell uname -m | tr -d '\r' | grep -qx "$GUEST"
adb install -r "$APK" | tee "$OUT/adb-apk-install.txt"
adb shell am start -W -n "$APP/.SmokeActivity" | tee "$OUT/adb-apk-launch.txt"
verdict=""
for i in $(seq 1 25); do
  verdict=$(adb shell run-as "$APP" cat files/status.txt 2>/dev/null | tr -d '\r' || true)
  if [[ "$verdict" == PASS:APP_SANDBOX_ELF_OBJECT ]]; then break; fi
  if [[ "$verdict" == FAIL:* ]]; then echo "$verdict" >&2; exit 1; fi
  sleep 1
done
printf '%s\n' "$verdict" | tee "$OUT/adb-apk-status.txt"
test "$verdict" = PASS:APP_SANDBOX_ELF_OBJECT
adb exec-out run-as "$APP" cat files/native_tcc.o > "$OUT/android_apk_native_tcc.o"
test -s "$OUT/android_apk_native_tcc.o"
NDK="${ANDROID_NDK_HOME:-}"
if [[ ! -d "$NDK" ]]; then NDK="${ANDROID_HOME:?}/ndk/27.3.13750724"; fi
case "$(uname -s)" in
  Linux) BIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin" ;;
  Darwin)
    BIN="$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin"
    test -d "$BIN" || BIN="$NDK/toolchains/llvm/prebuilt/darwin-arm64/bin"
    ;;
  *) exit 2 ;;
esac
"$BIN/llvm-readelf" -h "$OUT/android_apk_native_tcc.o" | tee "$OUT/adb-apk-object.txt"
grep -q 'Type:.*REL' "$OUT/adb-apk-object.txt"
grep -q "Machine:.*$MACHINE" "$OUT/adb-apk-object.txt"
adb shell dumpsys package "$APP" > "$OUT/adb-apk-package.txt"
grep -m1 'targetSdk=35' "$OUT/adb-apk-package.txt" | tee "$OUT/adb-apk-targetsdk.txt"
echo "Android $ABI targetSdk35 APK JNI libtcc ET_REL app sandbox PASS"

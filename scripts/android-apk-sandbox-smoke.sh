#!/usr/bin/env bash
set -euo pipefail
APP=com.qigao.tccpoc
APK=android/apk-poc/app/build/outputs/apk/debug/app-debug.apk
OUT=build-android-x86_64
test -s "$APK"
adb shell getprop ro.product.cpu.abi | tr -d '\r' | grep -qx x86_64
adb install -r "$APK" | tee "$OUT/adb-apk-install.txt"
adb shell am start -W -n "$APP/.SmokeActivity" | tee "$OUT/adb-apk-launch.txt"
verdict=""
for i in $(seq 1 25); do
    verdict=$(adb shell run-as "$APP" cat files/status.txt 2>/dev/null |
        tr -d '\r' || true)
    if [[ "$verdict" == PASS:APP_SANDBOX_ELF_OBJECT ]]; then break; fi
    if [[ "$verdict" == FAIL:* ]]; then echo "$verdict" >&2; exit 1; fi
    sleep 1
done
printf '%s\n' "$verdict" | tee "$OUT/adb-apk-status.txt"
test "$verdict" = PASS:APP_SANDBOX_ELF_OBJECT
adb exec-out run-as "$APP" cat files/native_tcc.o > "$OUT/android_apk_native_tcc.o"
test -s "$OUT/android_apk_native_tcc.o"
"${ANDROID_NDK_HOME:?}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf" \
    -h "$OUT/android_apk_native_tcc.o" | tee "$OUT/adb-apk-object.txt"
grep -q 'Type:.*REL' "$OUT/adb-apk-object.txt"
grep -q 'Machine:.*X86-64' "$OUT/adb-apk-object.txt"
adb shell dumpsys package "$APP" | grep -m1 'targetSdk=35' |
    tee "$OUT/adb-apk-targetsdk.txt"
echo 'Android APK targetSdk35 JNI libtcc compiled ET_REL in app sandbox PASS'

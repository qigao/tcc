# Android ARM64 real-device admission

This tests a dedicated ARM64 Android device and does **not** bypass the
Android app sandbox. The required Android x86-64 emulator APK/JNI gate
remains independent. Mac arm64 hosted runners have no supported nested
virtualization for accelerated Android ARM64 guests; software QEMU/TCG
is **opt-in diagnostic**, never a substitute for physical device execution.

## Runner configuration

1. Prepare an isolated Linux x86-64 GitHub Actions self-hosted runner
   with labels `self-hosted`, `linux`, `x64`, and
   `android-arm64-device`. Do **not** attach a personal phone or expose
   a runner holding secrets to untrusted PR code.
2. Attach one authorized dedicated ARM64 Android debug device (USB adb).
   Do not change SELinux enforcement, system images, app signing policy or
   Android W^X restrictions.
3. Set repository variable `TCC_ANDROID_ARM64_DEVICE_ENABLED=true`
   only after the runner and device are ready. Optionally specify
   `TCC_ANDROID_ARM64_SERIAL` as a repository secret; otherwise exactly
   one connected authorized device is required. Keep device serials and
   logcat out of uploaded artifacts.
4. On a trusted, repository-owned Draft PR branch
   `poc/tcc-llvm-mingw-atomic-threads-atl`, the conditional
   `android-arm64-hardware` CI job will use the same commit's pinned
   NDK ARM64 TinyCC payload and targetSdk35 multi-ABI APK artifacts.

## Required acceptance

- ADB reports `arm64-v8a`, `uname -m=aarch64`, and
  Android API level **35 or newer**. Reject `emulator-*` serials,
  `ro.kernel.qemu=1`, `ro.hardware=ranchu/goldfish` and ABI mismatches.
  Require three successive ELF/NDK and targetSdk35 JNI test iterations.
- Native Android ARM64/Bionic `tcc` executes, compiles a C source
  into an ELF64 AArch64 ET_REL object on-device; NDK Clang links it
  into a native Bionic executable, which is executed on the device.
- The `targetSdk35` APK installs and loads its packaged ARM64
  `libtcc.so` through JNI, compiles a separate ET_REL object in
  app-private storage and verifies the ARM64 object header via
  `run-as`. Generated app-private binaries are never executed.
- Fail-close on build, ABI, ELF, ownership, loader or JNI errors.

The job is **skipped by default**. A skipped job does not qualify
Android ARM64 runtime. The Mac software-TCG diagnostic can be requested
separately via `workflow_dispatch` input `run_arm64_tcg`; success
there would still be emulated Android ARM64, not native hardware.

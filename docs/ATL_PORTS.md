# Canonical ATL/WTL integration

Official WTL10 + pinned OpenATL/llvm-mingw is the sole WTL path.
See [WTL10_OPENATL.md](WTL10_OPENATL.md) for the required
Windows x64/native ARM64 ABI runtime and Linux/macOS cross-host
full SDI + .rc link acceptance.

The previous historical WTL8 Winx ATL port and compatibility patches,
x86/x64/ARM64 WTL8 tests and W^X thunk adapters have been removed.
ReactOS RATL survives only as a separate TinyCC compiler C ABI fixture;
it is not a WTL alternative, and no headers or objects are mixed.

The WTL10 ZIP remains external and licensed by MS-PL.
Pinned OpenATL still ships an RWX thunk. The POC-only patch
(`scripts/openatl-wx-hardening.py`) changes it to RW → RX for
the required native Windows x64/ARM64 hidden-window and resource-dialog
tests; it is **not** production hardening or an upstream fix.
CFG/CET, concurrent callbacks, teardown and failure injection remain open.

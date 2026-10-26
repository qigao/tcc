#!/usr/bin/env bash
# RATL = isolated conformance reference, never a fallback WTL implementation.
# Windows execution is required in the separate native WTL10 workflow.
set -euo pipefail
BIN="$(dirname "$(find "$PWD/tools/wtl10-host/llvm" -name x86_64-w64-mingw32-clang++ -print -quit)")"
test -d "$BIN"
OPENATL="$PWD/tools/wtl10-host/openatl"
RATL="$PWD/tools/wtl10-host/ratl-com"
OPENPIN=7b587f3e10f3f8d93a01522033de80007368bad5
RATLPIN=2e2efa182d7c6ee9037454d23780991fb8543084
test "$(git -C "$OPENATL" rev-parse HEAD)" = "$OPENPIN"
git clone --quiet https://github.com/katahiromz/RATL.git "$RATL"
git -C "$RATL" checkout --quiet --detach "$RATLPIN"
test "$(git -C "$RATL" rev-parse HEAD)" = "$RATLPIN"
test -f "$RATL/include/atl/atlcom.h"
test -f "$OPENATL/include/atlcom.h"

for cpu in x86_64 aarch64; do
  CXX="$BIN/$cpu-w64-mingw32-clang++"
  test -x "$CXX"
  OPENEXE="build-wtl10-host/atl_com_openatl_$cpu.exe"
  RATLEXE="build-wtl10-host/atl_com_ratl_$cpu.exe"
  "$CXX" -std=c++17 -O0 -DNDEBUG -DUNICODE -D_UNICODE \
    -D_ATL_APARTMENT_THREADED -D_WIN32_WINNT=0x0601 \
    -DATL_CONFORMANCE_OPENATL=1 "-I$OPENATL/include" \
    tests/atl_conformance/com_p0.cpp -o "$OPENEXE" \
    -static -lole32 -loleaut32 -luuid -lrpcrt4 -lshlwapi \
    -ladvapi32 -lgdi32 -luser32 -lkernel32
  python3 tests/wtl10/verify_pe_machine.py "$OPENEXE" "$cpu"
  "$CXX" -std=c++17 -O0 -DNDEBUG -DUNICODE -D_UNICODE \
    -D_ATL_APARTMENT_THREADED -D_WIN32_WINNT=0x0601 \
    -DATL_CONFORMANCE_RATL=1 -D__RATL__ -DINITGUID -D_USE_PSEH3 \
    -D_CRT_NON_CONFORMING_SWPRINTFS "-I$RATL/include/atl" "-I$RATL/compat" \
    tests/atl_conformance/com_p0.cpp -o "$RATLEXE" \
    -static -lole32 -loleaut32 -luuid -lrpcrt4 -lshlwapi \
    -ladvapi32 -lgdi32 -luser32 -lkernel32
  python3 tests/wtl10/verify_pe_machine.py "$RATLEXE" "$cpu"
done
echo "ATL COM P0 separate OpenATL and RATL PE cross-build x64+ARM64 PASS (no Windows runtime)"

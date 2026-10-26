param(
  [Parameter(Mandatory=$true)][string]$LLVMRoot,
  [Parameter(Mandatory=$true)][string]$TinyCCExe,
  [Parameter(Mandatory=$true)][string]$RATLRoot,
  [Parameter(Mandatory=$true)][string]$ObfusHeader,
  [string]$OutDir = "build-interop"
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$bin = Join-Path $root $OutDir
New-Item -ItemType Directory -Force -Path $bin | Out-Null
$clang = Join-Path $LLVMRoot 'x86_64-w64-mingw32-clang.exe'
$clangxx = Join-Path $LLVMRoot 'x86_64-w64-mingw32-clang++.exe'
$inc = Join-Path $root 'include'
function Invoke-Checked([string]$program, [string[]]$argv) {
  Write-Host "Running $program $($argv -join ' ')"
  & $program @argv
  if ($LASTEXITCODE -ne 0) { throw "$program failed with exit $LASTEXITCODE" }
}
if (-not (Test-Path $clang) -or -not (Test-Path $TinyCCExe)) { throw 'Toolchain missing' }
Push-Location $bin
try {
  # C DLL by llvm-mingw; Windows TinyCC calls it through GetProcAddress.
  Invoke-Checked $clang @('-shared', '-O0', "-I$inc", (Join-Path $root 'tests/bridge.c'), '-o', 'poc_bridge.dll')
  Invoke-Checked $TinyCCExe @("-I$inc", (Join-Path $root 'src/atomic/win_interlocked.c'),
    (Join-Path $root 'tests/atomic_thread.c'), '-o', 'atomic_thread_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'atomic_thread_tcc.exe') @() }
  Invoke-Checked $TinyCCExe @("-I$inc", (Join-Path $root 'src/threads/threads.c'),
    (Join-Path $root 'src/atomic/win_interlocked.c'),
    (Join-Path $root 'tests/thread_contract.c'), '-o', 'thread_contract_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'thread_contract_tcc.exe') @() }
  Invoke-Checked $TinyCCExe @("-I$inc", (Join-Path $root 'src/threads/threads.c'),
    (Join-Path $root 'tests/thread_shutdown_overlap.c'), '-o', 'thread_shutdown_overlap_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'thread_shutdown_overlap_tcc.exe') @() }
  # llvm-mingw COFF objects cannot be linked directly by this TinyCC PE
  # distribution. Preserve a verified, versioned C ABI across a DLL boundary.
  Invoke-Checked $clang @('-c', (Join-Path $root 'src/atomic/win_x64.S'), '-o', 'poc_atomic64_win.obj')
  Invoke-Checked $clang @('-shared', "-I$inc", (Join-Path $root 'src/atomic/win64_dll_exports.c'),
    'poc_atomic64_win.obj', '-o', 'poc_atomic64_win.dll')
  Invoke-Checked $clang @('-shared', (Join-Path $root 'tests/win_x64/bad_abi.c'),
    '-o', 'poc_atomic64_badabi.dll')
  Invoke-Checked $TinyCCExe @("-I$inc", '-DPOC_ATOMIC64_EXTERNAL_DLL=1',
    (Join-Path $root 'src/atomic/win64_dll_client.c'),
    (Join-Path $root 'src/threads/threads.c'),
    (Join-Path $root 'tests/atomic_small_thread.c'), '-o', 'atomic_small_thread_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'atomic_small_thread_tcc.exe') @() }
  Invoke-Checked $TinyCCExe @("-I$inc", '-DPOC_ATOMIC64_EXTERNAL_DLL=1',
    (Join-Path $root 'src/atomic/win64_dll_client.c'),
    (Join-Path $root 'src/atomic/ptr64.c'), (Join-Path $root 'src/threads/threads.c'),
    (Join-Path $root 'tests/atomic64_thread.c'), '-o', 'atomic64_thread_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'atomic64_thread_tcc.exe') @() }
  Invoke-Checked $TinyCCExe @((Join-Path $root 'tests/win_api.c'), '-o', 'win_api_tcc.exe')
  Invoke-Checked (Join-Path $bin 'win_api_tcc.exe') @()
  Invoke-Checked $TinyCCExe @("-I$inc", (Join-Path $root 'tests/bridge_client.c'), '-o', 'bridge_client_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'bridge_client_tcc.exe') @() }
  # llvm-mingw C++/RATL internals, exported stable C function only.
  $rinc = Join-Path $RATLRoot 'include/atl'
  $rcompat = Join-Path $RATLRoot 'compat'
  Invoke-Checked $clangxx @('-shared', '-std=c++17', '-O0', '-D__RATL__', '-DINITGUID',
    '-D_USE_PSEH3', '-D_CRT_NON_CONFORMING_SWPRINTFS', "-I$rinc", "-I$rcompat",
    # The llvm-mingw CRT has native excpt.h/sal.h. Do not shadow them with
    # RATL fallback headers: compat/excpt lacks EXCEPTION_DISPOSITION.
    (Join-Path $root 'tests/ratl/bridge.cpp'), '-o', 'poc_ratl_bridge.dll',
    '-static', '-lole32', '-loleaut32', '-luuid', '-lrpcrt4', '-lshlwapi', '-lgdi32', '-ladvapi32', '-lkernel32')
  Invoke-Checked $TinyCCExe @((Join-Path $root 'tests/ratl/client.c'), '-o', 'ratl_client_tcc.exe')
  for ($i = 1; $i -le 5; ++$i) { Invoke-Checked (Join-Path $bin 'ratl_client_tcc.exe') @() }
  # Optional deterministic semantic-only test; no anti-debug/fake signatures.
  if (-not (Test-Path $ObfusHeader)) { throw 'Pinned obfus.h header missing' }
  $obfinc = Split-Path -Parent $ObfusHeader
  Invoke-Checked $TinyCCExe @((Join-Path $root 'tests/obfuscation/semantic_smoke.c'), '-o', 'obfus_plain.exe')
  $plain = & (Join-Path $bin 'obfus_plain.exe'); if ($LASTEXITCODE -ne 0) { throw 'plain failed' }
  foreach ($seed in @(0, 123456789)) {
    Invoke-Checked $TinyCCExe @("-I$obfinc", '-DPOC_ENABLE_OBFUS_H=1',
      "-DOBFH_SEED=${seed}u", (Join-Path $root 'tests/obfuscation/semantic_smoke.c'),
      '-o', "obfus_protected_$seed.exe")
    $protected = & (Join-Path $bin "obfus_protected_$seed.exe")
    if ($LASTEXITCODE -ne 0) { throw "obfus seed $seed failed" }
    if ($plain -ne $protected) { throw "obfuscation seed $seed changed program output" }
    Write-Host "obfus.h seed $seed semantic parity PASS (no anti-debugging/fake signatures)"
  }
} finally { Pop-Location }

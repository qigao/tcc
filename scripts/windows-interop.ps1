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
  Invoke-Checked (Join-Path $bin 'atomic_thread_tcc.exe') @()
  Invoke-Checked $TinyCCExe @((Join-Path $root 'tests/win_api.c'), '-o', 'win_api_tcc.exe')
  Invoke-Checked (Join-Path $bin 'win_api_tcc.exe') @()
  Invoke-Checked $TinyCCExe @("-I$inc", (Join-Path $root 'tests/bridge_client.c'), '-o', 'bridge_client_tcc.exe')
  Invoke-Checked (Join-Path $bin 'bridge_client_tcc.exe') @()
  # llvm-mingw C++/RATL internals, exported stable C function only.
  $rinc = Join-Path $RATLRoot 'include/atl'
  $rcompat = Join-Path $RATLRoot 'compat'
  Invoke-Checked $clangxx @('-shared', '-std=c++17', '-O0', '-D__RATL__', '-DINITGUID',
    '-D_USE_PSEH3', '-D_CRT_NON_CONFORMING_SWPRINTFS', "-I$rinc", "-I$rcompat",
    "-I$(Join-Path $rcompat 'sal')", "-I$(Join-Path $rcompat 'excpt')",
    (Join-Path $root 'tests/ratl/bridge.cpp'), '-o', 'poc_ratl_bridge.dll',
    '-static', '-lole32', '-loleaut32', '-luuid', '-lrpcrt4', '-lshlwapi', '-lgdi32', '-ladvapi32', '-lkernel32')
  Invoke-Checked $TinyCCExe @((Join-Path $root 'tests/ratl/client.c'), '-o', 'ratl_client_tcc.exe')
  Invoke-Checked (Join-Path $bin 'ratl_client_tcc.exe') @()
  # Optional deterministic semantic-only test; no anti-debug/fake signatures.
  if (Test-Path $ObfusHeader) {
    $obfinc = Split-Path -Parent $ObfusHeader
    Invoke-Checked $TinyCCExe @((Join-Path $root 'tests/obfuscation/semantic_smoke.c'), '-o', 'obfus_plain.exe')
    Invoke-Checked $TinyCCExe @("-I$obfinc", '-DPOC_ENABLE_OBFUS_H=1',
      (Join-Path $root 'tests/obfuscation/semantic_smoke.c'), '-o', 'obfus_protected.exe')
    $plain = & (Join-Path $bin 'obfus_plain.exe'); if ($LASTEXITCODE -ne 0) { throw 'plain failed' }
    $protected = & (Join-Path $bin 'obfus_protected.exe'); if ($LASTEXITCODE -ne 0) { throw 'obfus failed' }
    if ($plain -ne $protected) { throw 'obfuscation changed program output' }
    Write-Host 'obfus.h semantic parity passed; no reverse-engineering assurance asserted'
  }
} finally { Pop-Location }

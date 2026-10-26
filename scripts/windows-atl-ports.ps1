param(
  [Parameter(Mandatory=$true)][string]$LLVMRoot,
  [Parameter(Mandatory=$true)][string]$TinyCC64,
  [Parameter(Mandatory=$true)][string]$TinyCC32,
  [Parameter(Mandatory=$true)][string]$OpenATLRoot,
  [Parameter(Mandatory=$true)][string]$WTL4MinGWRoot,
  [string]$OutDir = 'build-atl-ports',
  [ValidateSet('openatl','wtl4mingw','both')][string]$Port='both'
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$bin = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force $bin | Out-Null
function Invoke-Checked([string]$exe, [string[]]$argv) {
  Write-Host "Run $exe $($argv -join ' ')"
  & $exe @argv
  if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE" }
}
$cc64 = Join-Path $LLVMRoot 'x86_64-w64-mingw32-clang++.exe'
$cc32 = Join-Path $LLVMRoot 'i686-w64-mingw32-clang++.exe'
foreach ($need in @($cc64, $cc32, $TinyCC64, $TinyCC32)) {
  if (-not (Test-Path $need)) { throw "Missing compiler $need" }
}
$source = Join-Path $repo 'tests/atl_ports'
# Isolate the two ports' mutually conflicting atlbase.h and atlwin.h
# include trees. WTL4MinGW documents x86 only; don't guess x64 parity.
Push-Location $bin
try {
  if ($Port -eq 'both' -or $Port -eq 'openatl') {
  $openInc = Join-Path $OpenATLRoot 'include'
  Invoke-Checked $cc64 @('-shared','-std=c++17','-O0','-DUNICODE','-D_UNICODE',
    "-I$openInc", (Join-Path $source 'openatl_bridge.cpp'),
    '-o','poc_openatl.dll','-static',
    '-loleaut32','-lole32','-lcomctl32','-lshlwapi','-luuid','-luser32')
  Invoke-Checked $TinyCC64 @((Join-Path $source 'client.c'), '-o','openatl_client_tcc.exe')
  for ($i=1; $i -le 5; ++$i) {
    Invoke-Checked (Join-Path $bin 'openatl_client_tcc.exe') @('poc_openatl.dll')
  }
  Write-Host 'OpenATL pinned llvm-mingw C++ -> TinyCC x64 C ABI PASS'
  }
  if ($Port -eq 'both' -or $Port -eq 'wtl4mingw') {
  $atlport = Join-Path $WTL4MinGWRoot 'atlport/include'
  $wtlinc = Join-Path $WTL4MinGWRoot 'wtl/include'
  $stdext = Join-Path $WTL4MinGWRoot 'stdext/include'
  Invoke-Checked $cc32 @('-shared','-std=c++11','-O0','-include','algorithm',
    '-D_WIN32_WINNT=0x0601','-DWINVER=0x0601','-D_WIN32_IE=0x0601',
    '-fms-extensions', "-I$atlport", "-I$stdext", "-I$wtlinc",
    (Join-Path $source 'wtl4mingw_bridge.cpp'),
    '-o','poc_wtl4mingw.dll','-static',
    '-lole32','-loleaut32','-lcomctl32','-lshlwapi',
    '-luuid','-lgdi32','-luser32','-ladvapi32')
  Invoke-Checked $TinyCC32 @((Join-Path $source 'client.c'), '-o','wtl4mingw_client_tcc.exe')
  for ($i=1; $i -le 5; ++$i) {
    Invoke-Checked (Join-Path $bin 'wtl4mingw_client_tcc.exe') @('poc_wtl4mingw.dll')
  }
  Write-Host 'WTL4MinGW legacy x86 C++ -> TinyCC x86 C ABI PASS'
  }
} finally { Pop-Location }

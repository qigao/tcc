param(
  [Parameter(Mandatory=$true)][string]$LLVMRoot,
  [Parameter(Mandatory=$true)][string]$TinyCC64,
  [Parameter(Mandatory=$true)][string]$TinyCCArm,
  [string]$Root = '.',
  [string]$OutDir = 'build-wtl10'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Checked([string]$executable, [string[]]$arguments) {
  & $executable @arguments
  if ($LASTEXITCODE -ne 0) {
    throw "$executable exited $LASTEXITCODE"
  }
}
$repo = (Resolve-Path $Root).Path
Push-Location $repo
try {
  New-Item -ItemType Directory -Force $OutDir | Out-Null
  New-Item -ItemType Directory -Force tools/wtl10 | Out-Null
  $openATL = '7b587f3e10f3f8d93a01522033de80007368bad5'
  $testHead = '63ba6a992cb3d79d56be66cd283f6615c6646b3d'
  $wtlMirror = '065ef5597383c1231bd4892cf8e94331b29bdb97'
  $wtlBlob = 'a9f9d84495aac2155b0fdb00fec0e421b0e75c90'
  git clone --quiet https://github.com/kkhaike/openatl.git tools/wtl10-openatl
  if ($LASTEXITCODE -ne 0) { throw 'OpenATL clone failed' }
  Invoke-Checked git @('-C','tools/wtl10-openatl','checkout','--detach',$openATL)
  if ((git -C tools/wtl10-openatl rev-parse HEAD).Trim() -ne $openATL) {
    throw 'OpenATL head mismatch'
  }
  git clone --quiet https://github.com/kkhaike/wtltest.git tools/wtl10-test
  if ($LASTEXITCODE -ne 0) { throw 'WTL10 demo clone failed' }
  Invoke-Checked git @('-C','tools/wtl10-test','checkout','--detach',$testHead)
  if ((git -C tools/wtl10-test rev-parse HEAD).Trim() -ne $testHead) {
    throw 'WTL10 demo head mismatch'
  }
  $url = "https://raw.githubusercontent.com/Win32-WTL/WTL/$wtlMirror/Releases/WTL10_10320_Final.zip"
  Invoke-WebRequest -Uri $url -OutFile "$OutDir/wtl10.zip"
  $got = (git hash-object "$OutDir/wtl10.zip").Trim()
  if ($LASTEXITCODE -ne 0 -or $got -ne $wtlBlob) {
    throw "WTL10 pinned git blob mismatch: $got"
  }
  $sha = (Get-FileHash "$OutDir/wtl10.zip" -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($sha -ne '13de5cd7ed41454b8f287e9bcc4525234c6ac8c85905e4d4fe1a0776709fd78f') {
    throw "WTL10 official release SHA256 mismatch: $sha"
  }
  Write-Host "WTL 10.0.10320 release SHA256=$sha, git-blob=$got"
  Expand-Archive "$OutDir/wtl10.zip" -DestinationPath tools/wtl10 -Force
  $header = Get-ChildItem tools/wtl10 -Recurse -File -Filter atlapp.h |
    Where-Object { $_.FullName -match '(?i)[\\/]include[\\/]atlapp\.h$' } |
    Select-Object -First 1
  if (-not $header) { throw 'Pinned WTL10 zip does not contain Include/atlapp.h' }
  $txt = Get-Content $header.FullName -TotalCount 130
  if (-not ($txt -match 'WTL version 10\.0') -or
      -not ($txt -match '_WTL_VER')) {
    throw 'Unexpected WTL version; refuse WTL8/9 headers'
  }
  $wtlIncludes = $header.Directory.FullName
  $atlIncludes = (Resolve-Path tools/wtl10-openatl/include).Path
  $fixture = (Resolve-Path tests/wtl10/openatl_wtl10_bridge.cpp).Path
  $client = (Resolve-Path tests/atl_ports/client.c).Path
  foreach ($arch in @('x86_64','aarch64')) {
    $cxx = Join-Path $LLVMRoot "$arch-w64-mingw32-clang++.exe"
    $tcc = if ($arch -eq 'x86_64') { $TinyCC64 } else { $TinyCCArm }
    $suffix = if ($arch -eq 'x86_64') { 'x64' } else { 'arm64' }
    $dll = "$OutDir/poc_openatl_wtl10_$suffix.dll"
    $exe = "$OutDir/wtl10_client_$suffix.exe"
    Invoke-Checked $cxx @('-shared','-std=c++17','-O0','-DUNICODE',
      '-D_UNICODE','-D_WIN32_WINNT=0x0601','-DWINVER=0x0601',
      '-D_WIN32_IE=0x0601',"-I$atlIncludes","-I$wtlIncludes",
      $fixture,'-o',$dll,'-static','-loleaut32','-lole32','-luuid',
      '-lcomctl32','-lgdi32','-luser32','-lshell32','-lshlwapi',
      '-lcomdlg32','-luxtheme','-lmsimg32')
    Invoke-Checked $tcc @($client,'-o',$exe)
    # Git host compiler targets MUST NOT be substituted or emulated.
    Write-Host "WTL10 generated $dll and $exe"
  }
  Push-Location $OutDir
  try {
    for ($i=1; $i -le 5; ++$i) {
      Invoke-Checked (Join-Path (Get-Location).Path 'wtl10_client_x64.exe') @('poc_openatl_wtl10_x64.dll')
    }
  } finally { Pop-Location }
  Write-Host 'OpenATL + WTL10 llvm-mingw x64 DLL → TinyCC C 5/5 PASS'
  $manifest = @{}
  Get-ChildItem $OutDir -File -Include *.dll,*.exe -Recurse | ForEach-Object {
    $manifest[$_.Name] = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  }
  $manifest | ConvertTo-Json -Depth 3 | Set-Content "$OutDir/manifest.json" -Encoding utf8
  # The optional comprehensive upstream SDI sample is built separately,
  # never used as evidence of runtime unless it actually executes.
  Write-Host "WTL10_INCLUDE=$wtlIncludes"
} finally { Pop-Location }

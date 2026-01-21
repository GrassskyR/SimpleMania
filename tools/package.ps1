$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectRoot "cmake-build-release-mania"
$distDir = Join-Path $projectRoot "dist"
$msysBin = "C:\msys64\mingw64\bin"

if (!(Test-Path $buildDir)) {
    Write-Host "Build directory not found: $buildDir"
    Write-Host "Run Release build first."
    exit 1
}

New-Item -ItemType Directory -Force -Path $distDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $distDir "assets") | Out-Null

$exeFiles = @(
    "simplemania.exe",
    "chartgen.exe"
)

foreach ($exe in $exeFiles) {
    $src = Join-Path $buildDir $exe
    if (Test-Path $src) {
        Copy-Item $src $distDir -Force
    } else {
        Write-Host "Missing executable: $src"
    }
}

$dlls = @(
    "SDL2.dll",
    "SDL2_mixer.dll",
    "libstdc++-6.dll",
    "libgcc_s_seh-1.dll",
    "libwinpthread-1.dll",
    "zlib1.dll",
    "libFLAC.dll",
    "libogg-0.dll",
    "libopusfile-0.dll",
    "libvorbis-0.dll",
    "libvorbisfile-3.dll",
    "libmpg123-0.dll",
    "libwavpack-1.dll",
    "libxmp.dll"
)

foreach ($dll in $dlls) {
    $src = Join-Path $msysBin $dll
    if (Test-Path $src) {
        Copy-Item $src $distDir -Force
    }
}

$assetsSrc = Join-Path $projectRoot "assets"
if (Test-Path $assetsSrc) {
    Copy-Item -Path (Join-Path $assetsSrc "*") -Destination (Join-Path $distDir "assets") -Recurse -Force
}

Write-Host "Packaged to: $distDir"

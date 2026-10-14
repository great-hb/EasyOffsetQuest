param([switch]$Bootstrap)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location -LiteralPath $projectRoot
try {
    if ($Bootstrap) {
        & python tools/bootstrap.py
        if ($LASTEXITCODE -ne 0) { throw 'Dependency setup failed' }
    }
    $taskNdk = Join-Path $projectRoot 'toolchain/android-ndk-r27c'
    $taskNinja = Join-Path $projectRoot 'toolchain/ninja/ninja.exe'
    if (-not (Test-Path -LiteralPath $taskNdk) -or -not (Test-Path -LiteralPath $taskNinja) -or -not (Test-Path -LiteralPath 'extern.cmake')) {
        throw 'Run python tools/bootstrap.py first, or build.ps1 -Bootstrap'
    }
    & cmake -S . -B build -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$taskNdk/build/cmake/android.toolchain.cmake" "-DCMAKE_MAKE_PROGRAM=$taskNinja" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-29 -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    & cmake --build build --parallel 4
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
    & python tools/package.py
    if ($LASTEXITCODE -ne 0) { throw 'Packaging failed' }
} finally { Pop-Location }

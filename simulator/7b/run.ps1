param(
    [switch]$Build,
    [string]$Compiler = 'C:\msys64\mingw32\bin\gcc.exe',
    [string]$CMake = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    [string]$Ninja = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
)
$ErrorActionPreference = 'Stop'
$buildDir = Join-Path $PSScriptRoot 'build'
$exe = Join-Path $buildDir 'vesc_7b_simulator.exe'
if ($Build -or !(Test-Path -LiteralPath $exe)) {
    foreach ($tool in @($Compiler, $CMake, $Ninja)) {
        if (!(Test-Path -LiteralPath $tool)) { throw "Tool not found: $tool. Supply its path as a script parameter." }
    }
    $previousPath = $env:PATH
    try {
        $env:PATH = (Split-Path $Compiler) + ';' + $env:PATH
        & $CMake -S $PSScriptRoot -B $buildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_C_COMPILER=$Compiler" -DCMAKE_BUILD_TYPE=Release
        if ($LASTEXITCODE -ne 0) { throw 'Simulator configuration failed.' }
        & $CMake --build $buildDir -j 8
        if ($LASTEXITCODE -ne 0) { throw 'Simulator build failed. Close the running simulator before rebuilding.' }
    } finally { $env:PATH = $previousPath }
}
# This is the visible interactive application requested by the user.
Start-Process -FilePath $exe -WorkingDirectory $PSScriptRoot

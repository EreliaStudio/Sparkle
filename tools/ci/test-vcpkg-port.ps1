[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE): $Arguments" }
}

$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$root = Join-Path $repo 'build/vcpkg-port-check'
$overlay = Join-Path $repo 'ports'
$toolchain = "$env:VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake"
Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $root | Out-Null

function Assert-VcpkgLicenseFiles([string]$Installed) {
    $tripletRoot = Join-Path $Installed 'x64-windows-static'
    foreach ($relative in @(
        'share/sparkle/LICENSE',
        'share/sparkle/THIRD_PARTY_NOTICES.md',
        'share/sparkle/LICENSE-Liberation.txt',
        'share/erelia-sparkle/copyright'
    )) {
        $path = Join-Path $tripletRoot $relative
        if (-not (Test-Path $path)) { throw "vcpkg package is missing $path" }
    }
}

function Test-ManifestConsumer([string]$Name, [string]$SourcePath) {
    $build = Join-Path $root $Name
    $installed = Join-Path $root "$Name-installed"
    $arguments = @(
        '-S', $SourcePath, '-B', $build, '-G', 'Ninja',
        '-DCMAKE_CXX_COMPILER=clang-cl',
        "-DCMAKE_BUILD_TYPE=$Configuration",
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>',
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
        '-DVCPKG_TARGET_TRIPLET=x64-windows-static',
        "-DVCPKG_INSTALLED_DIR=$installed",
        "-DVCPKG_OVERLAY_PORTS=$overlay",
        '-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF',
        '-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF'
    )
    Invoke-Checked cmake $arguments
    Invoke-Checked cmake @('--build', $build, '--parallel', '4')
    Invoke-Checked ctest @('--test-dir', $build, '--output-on-failure', '--no-tests=error')
    Assert-VcpkgLicenseFiles $installed
}

Test-ManifestConsumer 'default-consumer' (Join-Path $repo 'tests/package-default')
Test-ManifestConsumer 'test-library-consumer' (Join-Path $repo 'tests/package-consumer')
'Base erelia-sparkle port and test-library feature both passed through vcpkg manifest consumption.' |
    Set-Content (Join-Path $root 'verification.log')

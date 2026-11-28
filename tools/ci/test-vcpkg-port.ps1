[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('All', 'Core', 'Graphics', 'TestLibrary')][string]$Component = 'All'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE): $Arguments" }
}

$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$root = Join-Path $repo 'build/vcpkg-port-check'
$overlay = Join-Path $repo 'ports'
# The port references the local checkout. Its ABI must depend on every
# packaged input, but not unrelated CI/test/documentation changes.
if ($env:GITHUB_ACTIONS -eq 'true') {
    $fingerprint = & "$PSScriptRoot/package-source-fingerprint.ps1"
    Set-Content -LiteralPath (Join-Path $overlay 'erelia-sparkle/ci-source-tree.txt') -Value $fingerprint -Encoding ascii
}
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

function Assert-CoreOnlyInstall([string]$Installed) {
    $tripletRoot = Join-Path $Installed 'x64-windows-static'
    foreach ($relative in @(
        'lib/sparkle.lib',
        'debug/lib/sparkled.lib',
        'share/glew',
        'share/opengl',
        'share/stb'
    )) {
        $path = Join-Path $tripletRoot $relative
        if (Test-Path $path) { throw "Core-only vcpkg install unexpectedly contains $path" }
    }
}

function Test-ManifestConsumer(
    [string]$Name,
    [string]$SourcePath,
    [bool]$CoreOnly = $false
) {
    $build = Join-Path $root $Name
    $installed = Join-Path $root "$Name-installed"
    $arguments = @(
        '-S', $SourcePath, '-B', $build, '-G', 'Ninja',
        '-DCMAKE_CXX_COMPILER=clang-cl',
        "-DCMAKE_BUILD_TYPE=$Configuration",
        '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>',
        '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded',
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
    if ($CoreOnly) { Assert-CoreOnlyInstall $installed }
}

if ($Component -in @('All', 'Core')) {
    Test-ManifestConsumer 'core-consumer' (Join-Path $repo 'tests/Packaging/CoreConsumer') $true
}
if ($Component -in @('All', 'Graphics')) {
    Test-ManifestConsumer 'graphics-consumer' (Join-Path $repo 'tests/Packaging/GraphicsConsumer')
}
if ($Component -in @('All', 'TestLibrary')) {
    Test-ManifestConsumer 'test-library-consumer' (Join-Path $repo 'tests/Packaging/TestLibraryConsumer')
}
"erelia-sparkle $Component configuration(s) passed through vcpkg manifest consumption." |
    Set-Content (Join-Path $root 'verification.log')

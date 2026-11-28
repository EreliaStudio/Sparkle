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
# The overlay reads this checkout directly. Include its exact source tree in the port ABI.
# Only CI enables persistent overlay caching; local dirty-tree runs keep the default behavior.
if ($env:GITHUB_ACTIONS -eq 'true') {
    $tree = (git -C $repo rev-parse 'HEAD^{tree}').Trim()
    if ($LASTEXITCODE -ne 0 -or $tree -notmatch '^[0-9a-f]{40,64}$') { throw 'Cannot fingerprint overlay source tree' }
    Set-Content -LiteralPath (Join-Path $overlay 'erelia-sparkle/ci-source-tree.txt') -Value $tree -Encoding ascii
    # Do not let a local edit reuse a cached package for HEAD.
    if (git -C $repo diff --name-only HEAD -- src include resources CMakeLists.txt cmake tools/resource_builder tests/SparkleTestLibrary) { throw 'Overlay source tree contains uncommitted changes' }
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

Test-ManifestConsumer 'core-consumer' (Join-Path $repo 'tests/Packaging/CoreConsumer') $true
Test-ManifestConsumer 'graphics-consumer' (Join-Path $repo 'tests/Packaging/GraphicsConsumer')
Test-ManifestConsumer 'test-library-consumer' (Join-Path $repo 'tests/Packaging/TestLibraryConsumer')
'Core-only, graphics and test-library erelia-sparkle configurations passed through vcpkg manifest consumption.' |
    Set-Content (Join-Path $root 'verification.log')

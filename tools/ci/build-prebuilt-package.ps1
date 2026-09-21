[CmdletBinding()]
param(
    [string]$Version = '',
    [string]$OutputDirectory = 'build/prebuilt-package/output'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE): $Arguments" }
}

$repo = (Resolve-Path "$PSScriptRoot/../..").Path
if ([string]::IsNullOrWhiteSpace($Version)) {
    $manifest = Get-Content (Join-Path $repo 'vcpkg.json') -Raw | ConvertFrom-Json
    $Version = $manifest.version
}
if ([string]::IsNullOrWhiteSpace($Version)) { throw 'Unable to resolve the Sparkle version' }

$revision = (& git -C $repo rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') { throw 'Unable to resolve the Sparkle revision' }

$packageName = "sparkle-$Version-g$revision-windows-x64-clangcl-static-release"
$root = Join-Path $repo 'build/prebuilt-package'
$output = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $repo $OutputDirectory }
$source = Join-Path $root 'source'
$build = Join-Path $root 'sparkle-build'
$dependencies = Join-Path $root 'dependencies'
$staging = Join-Path $root 'staging'
$relocated = Join-Path $root 'relocated'
$triplets = Join-Path $root 'triplets'
$installed = Join-Path $staging $packageName
$relocatedPackage = Join-Path $relocated $packageName

Remove-Item $source, $build, $dependencies, $staging, $relocated, $triplets,
    "$root/core-consumer", "$root/core-consumer-build", "$root/test-consumer", "$root/test-consumer-build",
    "$root/source.zip" -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $root, $output | Out-Null

Invoke-Checked git @('-C', $repo, 'archive', '--format=zip', '-o', "$root/source.zip", 'HEAD')
Expand-Archive "$root/source.zip" $source -Force
Copy-Item "$source/tests/package-core-consumer" "$root/core-consumer" -Recurse -Force
Copy-Item "$source/tests/package-consumer" "$root/test-consumer" -Recurse -Force
Copy-Item "$source/tools/ci/triplets" $triplets -Recurse -Force

$toolchain = "$env:VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake"
$common = @(
    '-G', 'Ninja',
    '-DCMAKE_C_COMPILER=clang-cl',
    '-DCMAKE_CXX_COMPILER=clang-cl',
    '-DCMAKE_BUILD_TYPE=Release',
    '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static-release',
    "-DVCPKG_OVERLAY_TRIPLETS=$triplets",
    "-DVCPKG_INSTALLED_DIR=$dependencies"
)

Invoke-Checked cmake (@(
    '-S', $source, '-B', $build,
    '-DSPARKLE_BUILD_TEST_LIBRARY=ON',
    '-DSPARKLE_BUILD_TESTS=OFF',
    "-DCMAKE_INSTALL_PREFIX=$installed"
) + $common)
Invoke-Checked cmake @('--build', $build, '--parallel', '4')
Invoke-Checked cmake @('--install', $build, '--config', 'Release')

$required = @(
    "$installed/include",
    "$installed/lib/sparkle.lib",
    "$installed/lib/SparkleTestLibrary.lib",
    "$installed/lib/cmake/sparkle/sparkleConfig.cmake",
    "$installed/lib/cmake/sparkle/sparkleConfigVersion.cmake",
    "$installed/lib/cmake/sparkle/sparkleTargets.cmake",
    "$installed/lib/cmake/sparkle/sparkleTargets-release.cmake",
    "$installed/lib/cmake/sparkle/sparkleTestTargets.cmake"
)
foreach ($path in $required) {
    if (-not (Test-Path $path)) { throw "Installed package is missing $path" }
}
if (Get-ChildItem "$installed/lib" -File -Filter '*d.lib') { throw 'The Release-only package contains a Debug library' }

$metadata = [ordered]@{
    name = 'sparkle'
    version = $Version
    revision = $revision
    platform = 'windows'
    architecture = 'x64'
    compiler = 'clang-cl'
    abi = 'msvc-compatible'
    crt = 'static'
    linkage = 'static'
    configuration = 'Release'
    components = @('TestLibrary')
}
$metadata | ConvertTo-Json | Set-Content "$installed/package-metadata.json"

$cmakeFiles = Get-ChildItem "$installed/lib/cmake/sparkle" -File -Filter '*.cmake'
foreach ($forbidden in @($source, $build)) {
    $matches = $cmakeFiles | Select-String -SimpleMatch $forbidden
    if ($matches) {
        $matches | ForEach-Object { Write-Error "$($_.Path):$($_.LineNumber): $($_.Line)" }
        throw "Installed CMake metadata embeds $forbidden"
    }
}

New-Item -ItemType Directory -Force $relocated | Out-Null
Move-Item $installed $relocatedPackage
Remove-Item $source, $build, $staging -Recurse -Force

$consumerCommon = @(
    '-G', 'Ninja',
    '-DCMAKE_C_COMPILER=clang-cl',
    '-DCMAKE_CXX_COMPILER=clang-cl',
    '-DCMAKE_BUILD_TYPE=Release',
    '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static-release',
    "-DVCPKG_OVERLAY_TRIPLETS=$triplets",
    "-DVCPKG_INSTALLED_DIR=$dependencies",
    '-DVCPKG_MANIFEST_MODE=OFF',
    '-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF',
    '-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF',
    "-DCMAKE_PREFIX_PATH=$relocatedPackage"
)

foreach ($consumer in @('core-consumer', 'test-consumer')) {
    $consumerSource = Join-Path $root $consumer
    $consumerBuild = Join-Path $root "$consumer-build"
    Invoke-Checked cmake (@('-S', $consumerSource, '-B', $consumerBuild) + $consumerCommon)
    Invoke-Checked cmake @('--build', $consumerBuild, '--parallel', '4')
    Invoke-Checked ctest @('--test-dir', $consumerBuild, '--output-on-failure', '--no-tests=error')
}

$archive = Join-Path $output "$packageName.zip"
$checksum = "$archive.sha256"
Remove-Item $archive, $checksum -Force -ErrorAction SilentlyContinue
Compress-Archive -Path $relocatedPackage -DestinationPath $archive -CompressionLevel Optimal
$hash = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $packageName.zip" | Set-Content $checksum

if ($env:GITHUB_OUTPUT) {
    "package_name=$packageName" | Out-File $env:GITHUB_OUTPUT -Append
    "archive=$archive" | Out-File $env:GITHUB_OUTPUT -Append
    "checksum=$checksum" | Out-File $env:GITHUB_OUTPUT -Append
    "sha256=$hash" | Out-File $env:GITHUB_OUTPUT -Append
    "release_tag=sparkle-v$Version-g$revision" | Out-File $env:GITHUB_OUTPUT -Append
}

Write-Host "Created $archive"
Write-Host "SHA-256: $hash"

[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed ($LASTEXITCODE): $Arguments" }
}

$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$root = Join-Path $repo 'build/package-check'
New-Item -ItemType Directory -Force $root | Out-Null
$source = Join-Path $root 'source'
# Build an isolated copy so relocation can remove the original source AND build trees.
Invoke-Checked git @('-C', $repo, 'archive', '--format=zip', '-o', "$root/source.zip", 'HEAD')
Expand-Archive "$root/source.zip" $source -Force
Copy-Item "$source/tests/package-consumer" "$root/consumer" -Recurse
Copy-Item "$source/tests/package-default" "$root/default-consumer" -Recurse
$dependencies = Join-Path $root 'dependencies'
$common = @('-G', 'Ninja', '-DCMAKE_CXX_COMPILER=clang-cl', "-DCMAKE_BUILD_TYPE=$Configuration",
    '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>',
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake",
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static', "-DVCPKG_INSTALLED_DIR=$dependencies")
$consumerOptions = $common + @('-DVCPKG_MANIFEST_MODE=OFF', '-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF',
    '-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF')

function Test-Consumer([string]$SourcePath, [string]$BuildPath, [string]$Prefix) {
    Invoke-Checked cmake (@('-S', $SourcePath, '-B', $BuildPath, "-DCMAKE_PREFIX_PATH=$Prefix") + $consumerOptions)
    Invoke-Checked cmake @('--build', $BuildPath, '--parallel', '4')
    Invoke-Checked ctest @('--test-dir', $BuildPath, '--output-on-failure', '--no-tests=error', '--output-junit', 'results.xml')
}

# Leave SPARKLE_BUILD_TEST_LIBRARY unspecified: verify its actual default.
$normal = Join-Path $root 'normal-install'
Invoke-Checked cmake (@('-S', $source, '-B', "$root/normal-build", '-DSPARKLE_BUILD_TESTS=OFF',
    '-DCMAKE_DISABLE_FIND_PACKAGE_GTest=TRUE', "-DCMAKE_INSTALL_PREFIX=$normal") + $common)
Invoke-Checked cmake @('--build', "$root/normal-build", '--parallel', '4')
Invoke-Checked cmake @('--install', "$root/normal-build")
Test-Consumer "$root/default-consumer" "$root/default-build" $normal
if (Test-Path "$normal/include/sparkle_test.hpp") { throw 'Optional headers installed by default' }
if (@(Get-ChildItem "$normal/lib" -Recurse -Filter '*TestLibrary*').Count -gt 0) { throw 'Optional library installed by default' }
& cmake -S "$root/consumer" -B "$root/missing-component" "-DCMAKE_PREFIX_PATH=$normal" @consumerOptions *> "$root/missing-component.log"
$missingExit = $LASTEXITCODE
Get-Content "$root/missing-component.log"
if ($missingExit -eq 0) { throw 'Required TestLibrary unexpectedly configured against default install' }
if (-not (Select-String -Path "$root/missing-component.log" -Pattern 'sparkle_FOUND.*FALSE|sparkle_FOUND to FALSE' -Quiet)) {
    throw 'Negative configuration failed for an unexpected reason; inspect missing-component.log'
}

$installed = Join-Path $root 'install'
Invoke-Checked cmake (@('-S', $source, '-B', "$root/library-build", '-DSPARKLE_BUILD_TESTS=OFF',
    '-DSPARKLE_BUILD_TEST_LIBRARY=ON', '-DCMAKE_DISABLE_FIND_PACKAGE_GTest=TRUE',
    "-DCMAKE_INSTALL_PREFIX=$installed") + $common)
Invoke-Checked cmake @('--build', "$root/library-build", '--parallel', '4')
Invoke-Checked cmake @('--install', "$root/library-build")
Test-Consumer "$root/consumer" "$root/consumer-build" $installed

$relocated = Join-Path $root 'relocated install'
Move-Item $installed $relocated
Remove-Item $source, "$root/library-build", "$root/normal-build", "$root/consumer-build" -Recurse -Force
Test-Consumer "$root/consumer" "$root/relocated-build" $relocated
'Optional component absence, installed consumer and relocated consumer passed.' | Set-Content "$root/verification.log"

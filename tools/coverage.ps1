<#
.SYNOPSIS
Builds Sparkle with LLVM coverage instrumentation and generates an HTML report.

.DESCRIPTION
Configures a dedicated Clang coverage build, runs the complete SparkleTestSuite
once, merges every generated raw profile, and writes an llvm-cov HTML report.

.PARAMETER Clean
Deletes the dedicated build/coverage directory before configuring.

.PARAMETER NoOpen
Does not open the generated index.html in the default browser.

.EXAMPLE
./tools/coverage.ps1

.EXAMPLE
./tools/coverage.ps1 -Clean -NoOpen
#>

[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$NoOpen
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$coverageBuildDirectory = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot "build/coverage"))
$expectedBuildPrefix = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot "build")) + [System.IO.Path]::DirectorySeparatorChar

if (-not $coverageBuildDirectory.StartsWith($expectedBuildPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use coverage directory outside the repository build directory: $coverageBuildDirectory"
}

foreach ($command in @("cmake", "clang++", "llvm-profdata", "llvm-cov")) {
    if ($null -eq (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Required command '$command' was not found in PATH. Install LLVM and CMake/Ninja before running coverage."
    }
}

if ([string]::IsNullOrWhiteSpace($env:VCPKG_ROOT)) {
    throw "VCPKG_ROOT is not set. Set it to your vcpkg installation directory before running coverage."
}

$toolchainFile = Join-Path $env:VCPKG_ROOT "scripts/buildsystems/vcpkg.cmake"
if (-not (Test-Path -LiteralPath $toolchainFile -PathType Leaf)) {
    throw "The vcpkg toolchain file was not found: $toolchainFile"
}

if ($Clean -and (Test-Path -LiteralPath $coverageBuildDirectory)) {
    Write-Host "Removing previous coverage build: $coverageBuildDirectory"
    Remove-Item -LiteralPath $coverageBuildDirectory -Recurse -Force
}

$runIdentifier = Get-Date -Format "yyyyMMdd-HHmmss-fff"
$runDirectory = Join-Path $coverageBuildDirectory "reports/$runIdentifier"
$profileDirectory = Join-Path $runDirectory "profiles"
$htmlDirectory = Join-Path $runDirectory "html"
$profileDataPath = Join-Path $runDirectory "sparkle.profdata"
$indexPath = Join-Path $htmlDirectory "index.html"

New-Item -ItemType Directory -Path $profileDirectory -Force | Out-Null

$configureArguments = @(
    "-S", $repositoryRoot,
    "-B", $coverageBuildDirectory,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DSPARKLE_BUILD_TESTS=ON",
    "-DVCPKG_MANIFEST_FEATURES=tests",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile",
    "-DCMAKE_CXX_COMPILER=clang++",
    "-DCMAKE_CXX_FLAGS=-fprofile-instr-generate -fcoverage-mapping",
    "-DCMAKE_EXE_LINKER_FLAGS=-fprofile-instr-generate"
)

Write-Host "Configuring coverage build..."
& cmake @configureArguments
if ($LASTEXITCODE -ne 0) {
    throw "Coverage configuration failed with exit code $LASTEXITCODE."
}

Write-Host "Building SparkleTestSuite..."
& cmake --build $coverageBuildDirectory --target SparkleTestSuite
if ($LASTEXITCODE -ne 0) {
    throw "Coverage build failed with exit code $LASTEXITCODE."
}

$testExecutable = Join-Path $coverageBuildDirectory "tests/TU/TestSuite/SparkleTestSuite.exe"
if (-not (Test-Path -LiteralPath $testExecutable -PathType Leaf)) {
    throw "The test executable was not produced: $testExecutable"
}

$previousProfilePattern = $env:LLVM_PROFILE_FILE
$testExitCode = 0
try {
    $env:LLVM_PROFILE_FILE = Join-Path $profileDirectory "sparkle-%p-%m.profraw"
    Write-Host "Running the complete test suite..."
    & $testExecutable --gtest_brief=1
    $testExitCode = $LASTEXITCODE
}
finally {
    if ($null -eq $previousProfilePattern) {
        Remove-Item Env:\LLVM_PROFILE_FILE -ErrorAction SilentlyContinue
    }
    else {
        $env:LLVM_PROFILE_FILE = $previousProfilePattern
    }
}

$profilePaths = @(Get-ChildItem -LiteralPath $profileDirectory -Filter "*.profraw" -File | Select-Object -ExpandProperty FullName)
if ($profilePaths.Count -eq 0) {
    throw "The tests produced no LLVM coverage profiles."
}

Write-Host "Merging $($profilePaths.Count) coverage profile(s)..."
& llvm-profdata merge -sparse @profilePaths "-o=$profileDataPath"
if ($LASTEXITCODE -ne 0) {
    throw "Coverage profile merge failed with exit code $LASTEXITCODE."
}

$ignorePattern = "(tests|build|vcpkg_installed)[\\/]"

Write-Host "Coverage summary:"
& llvm-cov report $testExecutable "-instr-profile=$profileDataPath" "-ignore-filename-regex=$ignorePattern"
if ($LASTEXITCODE -ne 0) {
    throw "Coverage summary generation failed with exit code $LASTEXITCODE."
}

Write-Host "Generating HTML report..."
& llvm-cov show $testExecutable `
    "-instr-profile=$profileDataPath" `
    -format=html `
    "-output-dir=$htmlDirectory" `
    -show-line-counts-or-regions `
    -show-branches=count `
    "-ignore-filename-regex=$ignorePattern"
if ($LASTEXITCODE -ne 0) {
    throw "HTML coverage generation failed with exit code $LASTEXITCODE."
}

if (-not (Test-Path -LiteralPath $indexPath -PathType Leaf)) {
    throw "llvm-cov completed without producing the expected report: $indexPath"
}

Write-Host "Coverage report: $indexPath"

if (-not $NoOpen) {
    Start-Process -FilePath $indexPath
}

if ($testExitCode -ne 0) {
    throw "SparkleTestSuite failed with exit code $testExitCode. The partial coverage report is available at: $indexPath"
}

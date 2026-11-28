<#
.SYNOPSIS
Builds Sparkle with LLVM coverage instrumentation and generates an HTML report.

.DESCRIPTION
Configures a dedicated Clang coverage build, runs every registered test, batching only audited pure math cases and retaining individual CTest isolation elsewhere, merges every generated raw profile, and writes an llvm-cov HTML report.

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
    [switch]$NoOpen,
    [string]$Triplet = "x64-windows"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$coverageBuildDirectory = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot "build/coverage"))
$expectedBuildPrefix = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot "build")) + [System.IO.Path]::DirectorySeparatorChar

if (-not $coverageBuildDirectory.StartsWith($expectedBuildPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use coverage directory outside the repository build directory: $coverageBuildDirectory"
}

foreach ($command in @("cmake", "ctest", "clang++", "llvm-profdata", "llvm-cov")) {
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
    "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded",
    "-DSPARKLE_BUILD_TESTS=ON",
    "-DVCPKG_MANIFEST_FEATURES=tests",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile",
    "-DCMAKE_CXX_COMPILER=clang++",
    "-DVCPKG_TARGET_TRIPLET=$Triplet",
    "-DCMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE=PRE_TEST",
    "-DCMAKE_CXX_FLAGS=-fprofile-instr-generate -fcoverage-mapping",
    "-DCMAKE_EXE_LINKER_FLAGS=-fprofile-instr-generate"
)

if ($Triplet.EndsWith("-static")) {
    $configureArguments += '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>'
}

Write-Host "Configuring coverage build..."
& cmake @configureArguments
if ($LASTEXITCODE -ne 0) {
    throw "Coverage configuration failed with exit code $LASTEXITCODE."
}

Write-Host "Building Core and Graphics test suites..."
& cmake --build $coverageBuildDirectory --target SparkleCoreTestSuite SparkleGraphicsTestSuite --parallel 4
if ($LASTEXITCODE -ne 0) {
    throw "Coverage build failed with exit code $LASTEXITCODE."
}

$coreTestExecutable = Join-Path $coverageBuildDirectory "tests/Core/SparkleCoreTestSuite.exe"
$graphicsTestExecutable = Join-Path $coverageBuildDirectory "tests/Graphics/SparkleGraphicsTestSuite.exe"
$clipboardTestExecutable = Join-Path $coverageBuildDirectory "tests/Graphics/SparkleClipboardTestSuite.exe"
foreach ($testExecutable in @($coreTestExecutable, $graphicsTestExecutable, $clipboardTestExecutable)) {
    if (-not (Test-Path -LiteralPath $testExecutable -PathType Leaf)) {
        throw "The test executable was not produced: $testExecutable"
    }
}

$previousProfilePattern = $env:LLVM_PROFILE_FILE
$testExitCode = 0
try {
    $env:LLVM_PROFILE_FILE = Join-Path $profileDirectory "sparkle-%p-%m.profraw"
    Write-Host "Running the complete test suite..."
    # Batch only explicitly audited pure math cases. The full standard CI
    # still runs every case in an isolated CTest process with its own timeout.
    $batchExitCode = 0
    $batchReport = Join-Path $runDirectory 'results-batch.xml'
    try {
        $json = & ctest --test-dir $coverageBuildDirectory -L '^CoverageBatch$' --show-only=json-v1
        if ($LASTEXITCODE -ne 0) { throw 'Coverage batch discovery failed' }
        $cases = @(($json -join [Environment]::NewLine | ConvertFrom-Json).tests)
        if ($cases.Count -eq 0) { throw 'Empty coverage batch' }
        $names = @($cases | ForEach-Object {
            if ($_.name -notmatch '^Core\.[A-Za-z0-9_]+\.[A-Za-z0-9_]+$') {
                throw "Unexpected coverage batch case: $($_.name)"
            }
            $_.name.Substring(5)
        })
        if (@($names | Sort-Object -Unique).Count -ne $names.Count) { throw 'Duplicate coverage batch cases' }

        $info = [System.Diagnostics.ProcessStartInfo]::new()
        $info.FileName = $coreTestExecutable
        $info.WorkingDirectory = Split-Path -Parent $coreTestExecutable
        $info.UseShellExecute = $false
        $info.ArgumentList.Add("--gtest_filter=$($names -join ':')")
        $info.ArgumentList.Add("--gtest_output=xml:$batchReport")
        $process = [System.Diagnostics.Process]::Start($info)
        try {
            # Stronger aggregate timeout: every case together gets at most 120 seconds.
            if (-not $process.WaitForExit(120000)) {
                $process.Kill($true)
                $process.WaitForExit()
                throw 'Coverage batch exceeded its 120 second timeout'
            }
            $batchExitCode = $process.ExitCode
        }
        finally { $process.Dispose() }

        $report = [xml](Get-Content -LiteralPath $batchReport -Raw)
        $executed = @($report.SelectNodes('//testcase[@status="run"]') | ForEach-Object {
            "$($_.classname).$($_.name)"
        } | Sort-Object)
        $expected = @($names | Sort-Object)
        if ($executed.Count -ne $expected.Count -or @(Compare-Object $expected $executed -SyncWindow 0).Count -ne 0) {
            throw 'Coverage batch did not execute exactly its discovered cases'
        }
        Write-Host "Coverage batch executed $($executed.Count) cases"
    }
    catch {
        Write-Warning $_
        $batchExitCode = 1
        # A failed batch must not cause any registered case to disappear.
        & ctest --test-dir $coverageBuildDirectory -L '^CoverageBatch$' --output-on-failure --no-tests=error --timeout 120 --parallel 2 --output-junit $batchReport
    }

    # The other Fast cases and the non-Fast complement remain isolated.
    & ctest --test-dir $coverageBuildDirectory -L '^Fast$' -LE '^CoverageBatch$' --output-on-failure --no-tests=error --timeout 120 --parallel 2 --output-junit (Join-Path $runDirectory "results-fast.xml")
    $fastExitCode = $LASTEXITCODE
    & ctest --test-dir $coverageBuildDirectory -LE '^Fast$' --output-on-failure --no-tests=error --timeout 120 --parallel 1 --output-junit (Join-Path $runDirectory "results-serial.xml")
    $serialExitCode = $LASTEXITCODE
    $testExitCode = if ($batchExitCode -ne 0) { $batchExitCode } elseif ($fastExitCode -ne 0) { $fastExitCode } else { $serialExitCode }
}
finally {
    if ($null -eq $previousProfilePattern) {
        Remove-Item Env:\LLVM_PROFILE_FILE -ErrorAction SilentlyContinue
    }
    else {
        $env:LLVM_PROFILE_FILE = $previousProfilePattern
    }
}

# Merge JUnit cases without dropping failures from either phase.
$combined = [xml] '<testsuites/>'
foreach ($name in @('results-batch.xml', 'results-fast.xml', 'results-serial.xml')) {
    $path = Join-Path $runDirectory $name
    if (Test-Path -LiteralPath $path) {
        $part = [xml](Get-Content -LiteralPath $path -Raw)
        $suites = if ($part.DocumentElement.Name -eq 'testsuite') { @($part.DocumentElement) } else { @($part.DocumentElement.SelectNodes('testsuite')) }
        foreach ($suite in $suites) {
            [void]$combined.DocumentElement.AppendChild($combined.ImportNode($suite, $true))
        }
    }
}
$combined.Save((Join-Path $runDirectory 'results.xml'))

$profilePaths = @(Get-ChildItem -LiteralPath $profileDirectory -Filter "*.profraw" -File | Select-Object -ExpandProperty FullName)
if ($profilePaths.Count -eq 0) {
    throw "The tests produced no LLVM coverage profiles."
}

Write-Host "Merging $($profilePaths.Count) coverage profile(s)..."
& llvm-profdata merge -sparse $profileDirectory "-o=$profileDataPath"
if ($LASTEXITCODE -ne 0) {
    throw "Coverage profile merge failed with exit code $LASTEXITCODE."
}

$ignorePattern = "(tests|build|vcpkg_installed)[\\/]"

$coverageObjects = @(
    "-object=$coreTestExecutable",
    "-object=$clipboardTestExecutable"
)

Write-Host "Coverage summary:"
& llvm-cov report $graphicsTestExecutable @coverageObjects "-instr-profile=$profileDataPath" "-ignore-filename-regex=$ignorePattern"
if ($LASTEXITCODE -ne 0) {
    throw "Coverage summary generation failed with exit code $LASTEXITCODE."
}

Write-Host "Generating HTML report..."
& llvm-cov show $graphicsTestExecutable `
    @coverageObjects `
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
    throw "Sparkle test suites failed with exit code $testExitCode. The partial coverage report is available at: $indexPath"
}

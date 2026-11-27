# Export only the already compiled Release executable and generated CTest registrations.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$testDirectory = 'build/core-test/tests/TU/Core'
$bundle = 'build/core-stress-bundle'
New-Item -ItemType Directory -Force $bundle | Out-Null

# PRE_TEST discovery creates the registrations even when the previous test selection failed.
& ctest --test-dir build/core-test -N -L '^Stress$' --no-tests=error
if ($LASTEXITCODE -ne 0) { throw 'Core stress discovery failed' }
$executable = if ($IsWindows) { 'SparkleCoreTestSuite.exe' } else { 'SparkleCoreTestSuite' }
Copy-Item -LiteralPath (Join-Path $testDirectory $executable) -Destination $bundle
$registrations = @(Get-ChildItem -LiteralPath $testDirectory -Filter 'SparkleCoreTestSuite*_tests.cmake' -File)
if ($registrations.Count -eq 0) { throw 'Missing generated Core test registrations' }
foreach ($registration in $registrations) {
    Copy-Item -LiteralPath $registration.FullName -Destination $bundle
}
$registrations | ForEach-Object { 'include("' + $_.Name + '")' } |
    Set-Content (Join-Path $bundle 'CTestTestfile.cmake') -Encoding utf8
[ordered]@{
    workspace = (Resolve-Path .).Path
    commit = $env:GITHUB_SHA
    os = $env:RUNNER_OS
    architecture = $env:RUNNER_ARCH
    executable = $executable
} | ConvertTo-Json | Set-Content (Join-Path $bundle 'core-reuse.json') -Encoding utf8

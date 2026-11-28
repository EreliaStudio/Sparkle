[CmdletBinding()]
param([Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Configuration)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$build = 'build/standard-test'
$bundles = 'build/ci-bundles'
$compiler = 'clang-cl'
$compilerVersion = (& $compiler --version | Out-String).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Could not identify the compiler' }
$origin = [ordered]@{
    workspace = (Resolve-Path .).Path
    commit = $env:GITHUB_SHA
    os = $env:RUNNER_OS
    architecture = $env:RUNNER_ARCH
    image = $env:ImageVersion
    configuration = $Configuration
    compiler = $compiler
    compilerVersion = $compilerVersion
    triplet = 'x64-windows-static'
    runtime = 'static MSVC'
}

function Export-TestSuite([string]$Suite, [string[]]$Executables) {
    $relative = "tests/$Suite"
    $source = Join-Path $build $relative
    $bundle = Join-Path $bundles $Suite
    $destination = Join-Path $bundle $relative
    New-Item -ItemType Directory -Force $destination | Out-Null

    # Force PRE_TEST discovery on the producer. The consumer never needs CMake or discovery.
    $inventoryJson = & ctest --test-dir $source --show-only=json-v1
    if ($LASTEXITCODE -ne 0) { throw "$Suite discovery failed" }
    $inventory = ($inventoryJson -join [Environment]::NewLine) | ConvertFrom-Json
    $names = @($inventory.tests | ForEach-Object { $_.name } | Sort-Object)
    if ($names.Count -eq 0 -or @($names | Sort-Object -Unique).Count -ne $names.Count -or
        @($names | Where-Object { $_ -notlike "$Suite.*" -or $_ -match 'NOT_BUILT' }).Count -ne 0) {
        throw "$Suite discovery produced an empty, duplicate or invalid inventory"
    }

    foreach ($executable in $Executables) {
        Copy-Item -LiteralPath (Join-Path $source $executable) -Destination $destination
    }
    $registrations = @(Get-ChildItem -LiteralPath $source -Filter 'Sparkle*Suite*_tests.cmake' -File | Sort-Object Name)
    if ($registrations.Count -eq 0) { throw "Missing $Suite registrations" }
    foreach ($registration in $registrations) {
        Copy-Item -LiteralPath $registration.FullName -Destination $destination
    }
    $registrations | ForEach-Object { 'include("' + $_.Name + '")' } |
        Set-Content (Join-Path $destination 'CTestTestfile.cmake') -Encoding utf8
    ('subdirs("' + $relative + '")') |
        Set-Content (Join-Path $bundle 'CTestTestfile.cmake') -Encoding utf8
    $metadata = [ordered]@{}
    foreach ($key in $origin.Keys) { $metadata[$key] = $origin[$key] }
    $metadata['suite'] = $Suite
    $metadata['executables'] = $Executables
    $metadata['tests'] = $names
    $metadata | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $bundle 'build-origin.json') -Encoding utf8
    Write-Host "Exported $Suite ($($names.Count) tests)"
}

Export-TestSuite -Suite Graphics -Executables @('SparkleGraphicsTestSuite.exe', 'SparkleClipboardTestSuite.exe')

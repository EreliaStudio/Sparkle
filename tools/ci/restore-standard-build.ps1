[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Graphics')][string]$Suite,
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Configuration
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$build = 'build/standard-test'
$origin = Get-Content (Join-Path $build 'build-origin.json') -Raw | ConvertFrom-Json
# Compiled resource paths and frozen CTest commands refer to the original workspace.
if ($origin.workspace -ne (Resolve-Path .).Path -or
    $origin.commit -ne $env:GITHUB_SHA -or
    $origin.os -ne $env:RUNNER_OS -or
    $origin.architecture -ne $env:RUNNER_ARCH -or
    $origin.configuration -ne $Configuration -or $origin.suite -ne $Suite) {
    throw 'The downloaded test bundle is incompatible with this checkout or configuration'
}
if ($origin.image -ne $env:ImageVersion) {
    Write-Warning "Runner image changed from $($origin.image) to $env:ImageVersion; validating executable inventory"
}
foreach ($name in $origin.executables) {
    $executable = Join-Path $build "tests/$Suite/$name"
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Missing $name" }
}
$inventoryJson = & ctest --test-dir $build --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'Downloaded CTest registration is invalid' }
$inventory = ($inventoryJson -join [Environment]::NewLine) | ConvertFrom-Json
$names = @($inventory.tests | ForEach-Object { $_.name } | Sort-Object)
$expected = @($origin.tests | Sort-Object)
if ($names.Count -eq 0 -or $names.Count -ne $expected.Count -or
    @(Compare-Object $expected $names -SyncWindow 0).Count -ne 0) {
    throw "Downloaded test inventory differs from the producer ($($names.Count) versus $($expected.Count))"
}
Write-Host "Verified $Suite $Configuration bundle: $($names.Count) tests; $($origin.compilerVersion)"

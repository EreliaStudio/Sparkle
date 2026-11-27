# Generated CTest commands and resource paths are absolute; reject incompatible runners.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$directory = 'build/core-test/tests/TU/Core'
$origin = Get-Content (Join-Path $directory 'core-reuse.json') -Raw | ConvertFrom-Json
if ($origin.workspace -ne (Resolve-Path .).Path -or
    $origin.commit -ne $env:GITHUB_SHA -or
    $origin.os -ne $env:RUNNER_OS -or
    $origin.architecture -ne $env:RUNNER_ARCH) {
    throw 'The downloaded Core test executable is not compatible with this checkout and runner'
}
$executable = Join-Path $directory $origin.executable
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw 'Missing Core test executable' }
if (-not $IsWindows) {
    & chmod +x $executable
    if ($LASTEXITCODE -ne 0) { throw 'Could not restore executable permission' }
    & ldd $executable
    if ($LASTEXITCODE -ne 0) { throw 'Core runtime dependency inspection failed' }
}
& ctest --test-dir $directory -N -L '^Stress$' --no-tests=error
if ($LASTEXITCODE -ne 0) { throw 'Downloaded stress registration is invalid' }

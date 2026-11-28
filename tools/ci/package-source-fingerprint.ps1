[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
# All inputs to CMake, resource embedding, installed headers, optional
# TestLibrary, vcpkg feature definitions and license installation.
$inputs = @('src', 'include', 'resources', 'cmake', 'ports', 'tools/resource_builder',
    'tests/SparkleTestLibrary', 'CMakeLists.txt', 'vcpkg.json', 'vcpkg-configuration.json',
    'LICENSE', 'THIRD_PARTY_NOTICES.md')

$dirty = @(git -C $repo status --porcelain --untracked-files=all --ignored -- @inputs)
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect package inputs' }
if ($dirty.Count -ne 0) { throw "Package inputs differ from the committed source: $dirty" }

$entries = @(git -C $repo ls-tree HEAD -- @inputs)
if ($LASTEXITCODE -ne 0 -or $entries.Count -eq 0) { throw 'Cannot enumerate package inputs' }
$hash = ($entries | git -C $repo hash-object --stdin).Trim()
if ($LASTEXITCODE -ne 0 -or $hash -notmatch '^[0-9a-f]{40,64}$') { throw 'Cannot fingerprint package inputs' }
$hash

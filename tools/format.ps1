param (
    [Parameter(Mandatory=$true)]
    [string]$Folder
)

$ExpectedVersion = "21.1.0"
$VersionOutput = clang-format --version
if ($LASTEXITCODE -ne 0) {
    throw "Unable to execute clang-format. Ensure LLVM clang-format $ExpectedVersion is installed and available on PATH."
}
if ($VersionOutput -notmatch "clang-format version $([regex]::Escape($ExpectedVersion))") {
    throw "Sparkle requires clang-format $ExpectedVersion. Found: $VersionOutput"
}

# Recursively find all C/C++ source/header files and apply clang-format in place
Get-ChildItem -Path $Folder -Recurse -Include *.cpp, *.hpp, *.c, *.h, *.cc, *.hh |
    ForEach-Object {
        clang-format -i $_.FullName
        Write-Output "Formatted: $($_.FullName)"
    }

Write-Output "Formatting applied to all source files in $Folder"

Get-ChildItem -Path $PSScriptRoot -Filter "*unittest*.exe" | ForEach-Object {
    Write-Output "Executing $($_.Name)..."
    Invoke-Expression $_.FullName
	# If ($process.ExitCode -ne 0) {
    #     Write-Output "Tests failed in $($_.Name). Stopping script."
    #     Exit
    # }
}

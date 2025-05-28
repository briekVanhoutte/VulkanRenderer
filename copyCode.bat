@echo off
SETLOCAL EnableDelayedExpansion

:: Set the base directory to the folder where this script is located
set "BaseDir=%~dp0"

:: Remove trailing backslash if present
if "%BaseDir:~-1%"=="\" set "BaseDir=%BaseDir:~0,-1%"

:: Default output file name
set "OutputFile=ConsolidatedCode.txt"

:: Set default extensions (edit if needed)
set "Extensions=*.h"

:: Call PowerShell to do the work
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$baseDir = '%BaseDir%';" ^
    "$outputFile = Join-Path $baseDir '%OutputFile%';" ^
    "$extensions = '%Extensions%'.Split(',');" ^
    "$folders = @('src', 'shaders','project');" ^
    "Set-Content -Path $outputFile -Value \"Consolidated Code from '$baseDir'`n`n\";" ^
    "foreach ($folder in $folders) {" ^
        "$fullPath = Join-Path $baseDir $folder;" ^
        "if (-Not (Test-Path $fullPath)) { continue };" ^
        "foreach ($ext in $extensions) {" ^
            "Get-ChildItem -Path $fullPath -Recurse -Filter $ext | ForEach-Object {" ^
                "$filePath = $_.FullName;" ^
                "Add-Content -Path $outputFile -Value \"`n----- Start of file: $filePath -----`n\";" ^
                "Get-Content -Path $filePath | Add-Content -Path $outputFile;" ^
                "Add-Content -Path $outputFile -Value \"`n----- End of file: $filePath -----`n\";" ^
            "}" ^
        "}" ^
    "};" ^
    "Write-Host \"Consolidation complete. Output saved to '$outputFile'.\""

pause

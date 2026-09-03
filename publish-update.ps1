param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version,

    [Parameter(Mandatory = $false)]
    [string]$Notes = "Backrooms Offical update."
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$Status = git status --porcelain
if ($Status) {
    throw "The repository has uncommitted changes. Commit or stash them first."
}

$Parts = $Version.Split(".")
$Major = [int]$Parts[0]
$Minor = [int]$Parts[1]
$Patch = [int]$Parts[2]

function Replace-Text {
    param(
        [string]$Path,
        [string]$Pattern,
        [string]$Replacement
    )

    $Content = Get-Content $Path -Raw
    $Updated = [regex]::Replace($Content, $Pattern, $Replacement)

    if ($Updated -eq $Content) {
        throw "Version pattern was not found in $Path"
    }

    Set-Content -Path $Path -Value $Updated -NoNewline
}

Replace-Text "src/Core/Version.h" 'inline constexpr const char\* Text = "\d+\.\d+\.\d+";' ('inline constexpr const char* Text = "' + $Version + '";')
Replace-Text "CMakeLists.txt" 'project\(BackroomsOffical VERSION \d+\.\d+\.\d+ LANGUAGES C CXX\)' ('project(BackroomsOffical VERSION ' + $Version + ' LANGUAGES C CXX)')

$Rc = Get-Content "src/Platform/Windows/Backrooms.rc" -Raw
$Rc = [regex]::Replace($Rc, 'FILEVERSION \d+,\d+,\d+,\d+', "FILEVERSION $Major,$Minor,$Patch,0")
$Rc = [regex]::Replace($Rc, 'PRODUCTVERSION \d+,\d+,\d+,\d+', "PRODUCTVERSION $Major,$Minor,$Patch,0")
$Rc = [regex]::Replace($Rc, 'VALUE "FileVersion", "\d+\.\d+\.\d+\\0"', ('VALUE "FileVersion", "' + $Version + '\0"'))
$Rc = [regex]::Replace($Rc, 'VALUE "ProductVersion", "\d+\.\d+\.\d+\\0"', ('VALUE "ProductVersion", "' + $Version + '\0"'))
Set-Content "src/Platform/Windows/Backrooms.rc" $Rc -NoNewline

Set-Content "update/release_notes.txt" $Notes -NoNewline

git add "src/Core/Version.h" "CMakeLists.txt" "src/Platform/Windows/Backrooms.rc" "update/release_notes.txt"
git commit -m "Release Backrooms Offical V$Version"
git push origin main

Write-Host ""
Write-Host "V$Version pushed."
Write-Host "GitHub is now building the public Windows installer."
Write-Host "The updater manifest is changed only after that installer is published successfully."
Write-Host ""
Write-Host "Public download:"
Write-Host "https://github.com/spongebobtdgameplay-prog/Backrooms-Server-For-Downloadiing/releases/download/latest-windows/Backrooms-Offical-Windows-x64.exe"

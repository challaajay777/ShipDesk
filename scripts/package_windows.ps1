# ShipDesk Windows Packaging & Deployment Script
param (
    [string]$BuildDir = "build",
    [string]$DistDir = "dist/ShipDesk",
    [string]$Version = "0.1.0",
    [switch]$CreateDesktopShortcut
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path "$PSScriptRoot\.."
Set-Location $ProjectRoot

Write-Output "=== Packaging ShipDesk v$Version ==="

if (-not (Test-Path "$BuildDir\shipdesk.exe")) {
    throw "Build artifact '$BuildDir\shipdesk.exe' not found. Compile first with cmake --build $BuildDir"
}

if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

Copy-Item "$BuildDir\shipdesk.exe" "$DistDir\"
Copy-Item "LICENSE" "$DistDir\"
Copy-Item "README.md" "$DistDir\"
Copy-Item "SECURITY.md" "$DistDir\"
Copy-Item "PRIVACY.md" "$DistDir\"

# Deploy Qt dependencies using windeployqt
$windeployqt = (Get-Command windeployqt6.exe, windeployqt.exe -ErrorAction SilentlyContinue | Select-Object -First 1).Source
if ($windeployqt) {
    Write-Output "Running $windeployqt on $DistDir\shipdesk.exe..."
    & $windeployqt --no-translations --compiler-runtime "$DistDir\shipdesk.exe"
} else {
    Write-Warning "windeployqt not found; copy Qt runtime DLLs manually."
}

# Copy application icon to dist
Copy-Item "$ProjectRoot\resources\icons\shipdesk.ico" "$DistDir\shipdesk.ico" -Force

# Create ZIP distribution
$ZipName = "ShipDesk-v$Version-windows-x64.zip"
if (Test-Path $ZipName) { Remove-Item -Force $ZipName }
Compress-Archive -Path "$DistDir\*" -DestinationPath $ZipName -CompressionLevel Optimal

$Hash = (Get-FileHash -Algorithm SHA256 $ZipName).Hash
Set-Content -Path "$ZipName.sha256" -Value "$Hash  $ZipName"

Write-Output "=== Distribution Package Created ==="
Write-Output "Archive: $ZipName"
Write-Output "Size: $((Get-Item $ZipName).Length) bytes"
Write-Output "SHA-256: $Hash"

if ($CreateDesktopShortcut) {
    $InstallDir = "$env:LOCALAPPDATA\ShipDesk"
    if (Test-Path $InstallDir) { Remove-Item -Recurse -Force $InstallDir }
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
    Copy-Item -Recurse -Force "$DistDir\*" $InstallDir

    $Desktops = @(
        [Environment]::GetFolderPath('Desktop'),
        "$env:USERPROFILE\Desktop",
        "$env:USERPROFILE\OneDrive\Desktop"
    ) | Select-Object -Unique

    $WshShell = New-Object -ComObject WScript.Shell
    foreach ($d in $Desktops) {
        if (Test-Path $d) {
            $ShortcutPath = Join-Path $d "ShipDesk.lnk"
            $Shortcut = $WshShell.CreateShortcut($ShortcutPath)
            $Shortcut.TargetPath = "$InstallDir\shipdesk.exe"
            $Shortcut.WorkingDirectory = "$InstallDir"
            $Shortcut.IconLocation = "$InstallDir\shipdesk.ico,0"
            $Shortcut.Description = "ShipDesk - Native Desktop Release Workspace"
            $Shortcut.Save()
            Write-Output "Created Desktop Shortcut: $ShortcutPath"
        }
    }
}

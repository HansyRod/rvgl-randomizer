$ErrorActionPreference = "Stop"

$configPath = Join-Path $PSScriptRoot "..\\src-tauri\\tauri.conf.json"
$releaseDir = Join-Path $PSScriptRoot "..\\src-tauri\\target\\release"
$bundleDir = Join-Path $releaseDir "bundle"

$config = Get-Content $configPath | ConvertFrom-Json
$version = $config.version

$exePath = Join-Path $releaseDir "rvgl-randomizer.exe"
$resourcesPath = Join-Path $releaseDir "resources"
$zipPath = Join-Path $bundleDir ("rvgl-randomizer-{0}.zip" -f $version)

if (-not (Test-Path $exePath)) {
    throw "Portable packaging failed: executable not found at $exePath"
}

if (-not (Test-Path $resourcesPath)) {
    throw "Portable packaging failed: resources directory not found at $resourcesPath"
}

New-Item -ItemType Directory -Path $bundleDir -Force | Out-Null

if (Test-Path $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

Compress-Archive -Path $exePath, $resourcesPath -DestinationPath $zipPath -Force
Write-Host "Created portable archive: $zipPath"

<#
.SYNOPSIS
    Builds all Simple Spaces plugins.

.DESCRIPTION
    Configures and builds all 12 plugins using CMake and MSVC, then validates
    plugin manifests.  Outputs one DLL + plugin.json per plugin under the
    build output directory.

    Requires:
      - CMake 3.20 or later on PATH (https://cmake.org)
      - Visual Studio 2019 or 2022 with the "Desktop development with C++" workload
      - The Spaces host SDK path (directory containing 'extensions/')

.PARAMETER HostSdkPath
    [Required] Path to the Spaces host SDK directory that contains
    the 'extensions/' header folder.

.PARAMETER BuildDir
    Directory where CMake build files and intermediate objects are written.
    Defaults to 'build' inside the repo root.

.PARAMETER Configuration
    Build configuration: Debug or Release.  Defaults to Release.

.PARAMETER InstallDir
    Optional.  If provided, all built plugin DLLs and their plugin.json files
    are copied here after a successful build (e.g. the host app's plugins folder).

.PARAMETER Clean
    If specified, removes and recreates the build directory before configuring.

.EXAMPLE
    .\build.ps1 -HostSdkPath "C:\Dev\Spaces\sdk"

.EXAMPLE
    .\build.ps1 -HostSdkPath "C:\Dev\Spaces\sdk" -Configuration Debug -Clean

.EXAMPLE
    .\build.ps1 -HostSdkPath "C:\Dev\Spaces\sdk" `
                -InstallDir "C:\Dev\Spaces\bin\plugins" `
                -Clean
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$HostSdkPath,

    [string]$BuildDir = "build",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [string]$InstallDir = "",

    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── Helpers ─────────────────────────────────────────────────────────────────

function Write-Step([string]$text) {
    Write-Host ""
    Write-Host "==> $text" -ForegroundColor Cyan
}

function Write-Ok([string]$text) {
    Write-Host "    $text" -ForegroundColor Green
}

function Fail([string]$text) {
    Write-Host ""
    Write-Host "ERROR: $text" -ForegroundColor Red
    exit 1
}

# ── Resolve paths ────────────────────────────────────────────────────────────

$repoRoot  = $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDir

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║        Simple Spaces Plugin Builder              ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host "  Repo root    : $repoRoot"
Write-Host "  Build dir    : $buildPath"
Write-Host "  Configuration: $Configuration"

# Validate HostSdkPath
if (-not (Test-Path $HostSdkPath -PathType Container)) {
    Fail "HostSdkPath does not exist: $HostSdkPath"
}
$sdkPath = (Resolve-Path $HostSdkPath).Path
if (-not (Test-Path (Join-Path $sdkPath "extensions") -PathType Container)) {
    Fail "HostSdkPath does not contain an 'extensions/' subfolder.`n  Checked: $sdkPath\extensions`n  Make sure HostSdkPath points to the Spaces host SDK root."
}
Write-Host "  Host SDK     : $sdkPath"

if ($InstallDir -ne "") {
    Write-Host "  Install dir  : $InstallDir"
}

# ── Prerequisite checks ──────────────────────────────────────────────────────

Write-Step "Checking prerequisites"

# CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "cmake not found on PATH.`n  Install CMake from https://cmake.org and ensure it is added to PATH."
}
$cmakeVersion = (cmake --version | Select-Object -First 1)
Write-Ok "cmake: $cmakeVersion"

# Find Visual Studio via vswhere
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
}

$vsGenerator = $null
$vsDisplayName = $null

if (Test-Path $vswhere) {
    $vsInstalls = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json 2>$null | ConvertFrom-Json

    if ($vsInstalls -and $vsInstalls.Count -gt 0) {
        $vs = $vsInstalls[0]
        $vsMajor = [int]($vs.installationVersion -split '\.')[0]

        switch ($vsMajor) {
            17 { $vsGenerator = "Visual Studio 17 2022"; $vsDisplayName = "Visual Studio 2022" }
            16 { $vsGenerator = "Visual Studio 16 2019"; $vsDisplayName = "Visual Studio 2019" }
            15 { $vsGenerator = "Visual Studio 15 2017"; $vsDisplayName = "Visual Studio 2017" }
            default { Fail "Unsupported Visual Studio version $vsMajor. Install VS 2019 or VS 2022 with Desktop C++ workload." }
        }

        Write-Ok "${vsDisplayName} detected (v$($vs.installationVersion))"
    }
}

if (-not $vsGenerator) {
    # Fallback: try common generators in descending order
    $candidates = @("Visual Studio 17 2022", "Visual Studio 16 2019")
    foreach ($gen in $candidates) {
        $testOut = cmake -G $gen --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            $vsGenerator = $gen
            Write-Ok "Generator (fallback detection): $vsGenerator"
            break
        }
    }
}

if (-not $vsGenerator) {
    Fail "No compatible Visual Studio installation found.`n  Install Visual Studio 2019 or 2022 with the 'Desktop development with C++' workload."
}

# ── Optional clean ────────────────────────────────────────────────────────────

if ($Clean -and (Test-Path $buildPath)) {
    Write-Step "Cleaning build directory"
    Remove-Item -Recurse -Force $buildPath
    Write-Ok "Removed: $buildPath"
}

if (-not (Test-Path $buildPath)) {
    New-Item -ItemType Directory -Path $buildPath | Out-Null
}

# ── Validate manifests (fast pre-check) ──────────────────────────────────────

Write-Step "Validating plugin manifests"
& "$repoRoot\scripts\validate-plugin-manifests.ps1"
if ($LASTEXITCODE -ne 0) {
    Fail "Manifest validation failed — fix errors above before building."
}
Write-Ok "All 12 manifests valid."

# ── CMake configure ───────────────────────────────────────────────────────────

Write-Step "Configuring with CMake ($vsGenerator)"

Push-Location $buildPath
try {
    & cmake "$repoRoot" `
        -G "$vsGenerator" `
        -A x64 `
        "-DHOST_SDK_PATH=$sdkPath"

    if ($LASTEXITCODE -ne 0) {
        Fail "CMake configuration failed (exit code $LASTEXITCODE)."
    }
}
finally {
    Pop-Location
}

Write-Ok "Configuration complete."

# ── CMake build ───────────────────────────────────────────────────────────────

Write-Step "Building ($Configuration) — this may take a moment"

Push-Location $buildPath
try {
    & cmake --build . --config $Configuration --parallel

    if ($LASTEXITCODE -ne 0) {
        Fail "Build failed (exit code $LASTEXITCODE)."
    }
}
finally {
    Pop-Location
}

Write-Ok "Build complete."

# ── Collect output ────────────────────────────────────────────────────────────

$outputRoot = Join-Path $buildPath "output\plugins"

Write-Step "Build output"
if (Test-Path $outputRoot) {
    $dlls = Get-ChildItem -Path $outputRoot -Recurse -Filter "*.dll"
    foreach ($dll in $dlls) {
        $rel = $dll.FullName.Substring($outputRoot.Length).TrimStart("\")
        Write-Ok $rel
    }
    Write-Host ""
    Write-Host "  Output root: $outputRoot" -ForegroundColor White
} else {
    Write-Host "  (No output directory found — build may have placed files elsewhere)" -ForegroundColor Yellow
}

# ── Optional install ─────────────────────────────────────────────────────────

if ($InstallDir -ne "") {
    Write-Step "Installing plugins to: $InstallDir"

    if (-not (Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }

    $pluginDirs = Get-ChildItem -Path $outputRoot -Directory -ErrorAction SilentlyContinue
    $count = 0
    foreach ($dir in $pluginDirs) {
        $destDir = Join-Path $InstallDir $dir.Name
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir | Out-Null
        }

        Get-ChildItem -Path $dir.FullName -File | ForEach-Object {
            Copy-Item $_.FullName (Join-Path $destDir $_.Name) -Force
            Write-Ok "$($dir.Name)\$($_.Name)"
            $count++
        }
    }

    Write-Ok "Installed $count files to: $InstallDir"
}

# ── Done ──────────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║   Build succeeded.  All plugins ready.           ║" -ForegroundColor Green
Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""

# PowerShell Build Script for NeuroDB C++ Database Engine

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Building NeuroDB C++ Core Engine Binaries  " -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

# Detect MinGW g++ compiler
$gppPath = "g++"
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    $codeblocksGpp = "C:\Program Files\CodeBlocks\MinGW\bin\g++.exe"
    if (Test-Path $codeblocksGpp) {
        $env:PATH = "C:\Program Files\CodeBlocks\MinGW\bin;" + $env:PATH
        $gppPath = $codeblocksGpp
        Write-Host "Using CodeBlocks MinGW GCC compiler at: $codeblocksGpp" -ForegroundColor Green
    } else {
        Write-Error "Error: g++ compiler not found in PATH or CodeBlocks directory!"
        exit 1
    }
}

$rootDir = Split-Path -Parent $PSScriptRoot

Write-Host "Compiling neurodb.exe..." -ForegroundColor Yellow
& $gppPath -std=c++17 -I"$rootDir\include" "$rootDir\src\main.cpp" -o "$rootDir\neurodb.exe"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation of neurodb.exe failed!"
    exit $LASTEXITCODE
}
Write-Host "✓ Successfully built neurodb.exe" -ForegroundColor Green

Write-Host "Compiling setup_test_data.exe..." -ForegroundColor Yellow
& $gppPath -std=c++17 -I"$rootDir\include" "$rootDir\src\setup_test_data.cpp" -o "$rootDir\setup_test_data.exe"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation of setup_test_data.exe failed!"
    exit $LASTEXITCODE
}
Write-Host "✓ Successfully built setup_test_data.exe" -ForegroundColor Green

Write-Host "Seeding initial database tables..." -ForegroundColor Yellow
Set-Location -Path $rootDir
& "$rootDir\setup_test_data.exe"

Write-Host "`n=============================================" -ForegroundColor Cyan
Write-Host "      Build & Setup Complete! Ready!        " -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

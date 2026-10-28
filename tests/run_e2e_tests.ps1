# E2E Test Suite Runner Script for RBI Banking Management System
[CmdletBinding()]
param (
    [string]$Tier = "all",
    [switch]$BuildOnly = $false,
    [switch]$SkipMainBuild = $false
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$RootDir = (Get-Item $ScriptDir).Parent.FullName

Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "   RBI Banking System - E2E Test Suite Runner Script   " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "Project Root: $RootDir"
Write-Host "Tests Dir:    $ScriptDir"

# Step 1: Verify g++ compiler
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host "[ERROR] g++ compiler not found in PATH! Please ensure MinGW / g++ is installed." -ForegroundColor Red
    exit 1
}

# Step 2: Build main project executable if needed
$MainExe = Join-Path $RootDir "rbi_banking_system.exe"
if (-not $SkipMainBuild) {
    Write-Host "`n[1/3] Building main project executable (rbi_banking_system.exe)..." -ForegroundColor Yellow
    Push-Location $RootDir
    try {
        Get-Process -Name "rbi_banking_system", "test_runner" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
        $cppFiles = Get-ChildItem -Path $RootDir -Filter "*.cpp" | Select-Object -ExpandProperty FullName
        if ($cppFiles.Count -eq 0) {
            Write-Host "[ERROR] No C++ source files found in $RootDir" -ForegroundColor Red
            exit 1
        }
        $buildCmd = "g++ -std=c++17 -Wall -Wextra -Iinclude " + ($cppFiles -join " ") + " -o `"$MainExe`""
       Write-Host "Executing: g++ -std=c++17 -Wall -Wextra -Iinclude *.cpp -o rbi_banking_system.exe" -ForegroundColor Gray
        $LASTEXITCODE = 0
        Invoke-Expression $buildCmd
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path $MainExe)) {
            Write-Host "[ERROR] Failed to compile main executable $MainExe (Exit Code: $LASTEXITCODE)" -ForegroundColor Red
            exit 1
        }
        Write-Host "[SUCCESS] Main executable compiled successfully." -ForegroundColor Green
    }
    finally {
        Pop-Location
    }
} else {
    Write-Host "`n[1/3] Skipping main project build (using existing executable)." -ForegroundColor Gray
}

# Step 3: Discover test source files across Tiers 1-4 and tests directory
Write-Host "`n[2/3] Discovering and compiling E2E test suite executables..." -ForegroundColor Yellow

$TiersToRun = @()
if ($Tier -eq "all") {
    $TiersToRun = @("tier1", "tier2", "tier3", "tier4")
} else {
    $TiersToRun = @($Tier.ToLower())
}

$TestRunnerExe = Join-Path $ScriptDir "test_runner.exe"
$MainTestRunnerCpp = Join-Path $ScriptDir "main_test_runner.cpp"

# Collect all test cpp files from tiers and main_test_runner.cpp
$AllTestCppFiles = @()
if (Test-Path $MainTestRunnerCpp) {
    $AllTestCppFiles += $MainTestRunnerCpp
}

# Collect implementation files (excluding main.cpp)
$implFiles = Get-ChildItem -Path $RootDir -Filter "*.cpp" | Where-Object { $_.Name -ne "main.cpp" } | Select-Object -ExpandProperty FullName
$AllTestCppFiles += $implFiles

foreach ($t in @("tier1", "tier2", "tier3", "tier4")) {
    $tierDir = Join-Path $ScriptDir $t
    if (Test-Path $tierDir) {
        $files = Get-ChildItem -Path $tierDir -Filter "*.cpp" -Recurse | Select-Object -ExpandProperty FullName
        $AllTestCppFiles += $files
    }
}

Write-Host "Found $($AllTestCppFiles.Count) test source files." -ForegroundColor Gray

Push-Location $RootDir
try {
    $includeFlags = "-I `"$ScriptDir`" -I `"$ScriptDir/include`" -I `"$RootDir`" -I `"$RootDir/include`""
    $testBuildCmd = "g++ -std=c++17 $includeFlags " + (($AllTestCppFiles | ForEach-Object { "`"$_`"" }) -join " ") + " -o `"$TestRunnerExe`""
    Write-Host "Compiling test runner executable..." -ForegroundColor Gray
    $LASTEXITCODE = 0
    Invoke-Expression $testBuildCmd
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $TestRunnerExe)) {
        Write-Host "[ERROR] Failed to compile test runner executable $TestRunnerExe (Exit Code: $LASTEXITCODE)" -ForegroundColor Red
        exit 1
    }
    Write-Host "[SUCCESS] Test runner compiled successfully: $TestRunnerExe" -ForegroundColor Green
}
finally {
    Pop-Location
}

if ($BuildOnly) {
    Write-Host "`n[Build-Only Mode] Build complete." -ForegroundColor Green
    exit 0
}

# Step 4: Run test executables and track pass/fail per Tier
Write-Host "`n[3/3] Executing E2E Test Suite..." -ForegroundColor Yellow

$OverallSuccess = $true
$TierSummary = @()

foreach ($t in $TiersToRun) {
    Write-Host "`n>>> Running $t Tests <<<" -ForegroundColor Cyan
    Push-Location $RootDir
    $startTime = Get-Date
    try {
        $pinfo = New-Object System.Diagnostics.ProcessStartInfo
        $pinfo.FileName = $TestRunnerExe
        $pinfo.Arguments = "--tier $t"
        $pinfo.UseShellExecute = $false
        $pinfo.RedirectStandardOutput = $true
        $pinfo.RedirectStandardError = $true
        $pinfo.WorkingDirectory = $RootDir

        $p = New-Object System.Diagnostics.Process
        $p.StartInfo = $pinfo
        $p.Start() | Out-Null
        $stdout = $p.StandardOutput.ReadToEnd()
        $stderr = $p.StandardError.ReadToEnd()
        $p.WaitForExit()
        $exitCode = $p.ExitCode
        $endTime = Get-Date
        $duration = ($endTime - $startTime).TotalMilliseconds

        Write-Host $stdout
        if ($stderr) {
            Write-Host $stderr -ForegroundColor Red
        }

        $passed = ($exitCode -eq 0)
        if (-not $passed) {
            $OverallSuccess = $false
        }

        $TierSummary += [PSCustomObject]@{
            Tier = $t
            Status = if ($passed) { "PASS" } else { "FAIL" }
            DurationMs = [math]::Round($duration, 2)
            ExitCode = $exitCode
        }
    }
    finally {
        Pop-Location
    }
}

# Print Summary Table
Write-Host "`n=======================================================" -ForegroundColor Cyan
Write-Host "                E2E TIER SUMMARY TABLE                " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan

foreach ($row in $TierSummary) {
    $color = if ($row.Status -eq "PASS") { "Green" } else { "Red" }
    Write-Host ("[{0}] Tier: {1,-8} | Time: {2,8} ms" -f $row.Status, $row.Tier, $row.DurationMs) -ForegroundColor $color
}

Write-Host "-------------------------------------------------------" -ForegroundColor Cyan
if ($OverallSuccess) {
    Write-Host "OVERALL RESULT: ALL TIERS PASSED SUCCESSFULLY" -ForegroundColor Green
    exit 0
} else {
    Write-Host "OVERALL RESULT: ONE OR MORE TIERS FAILED" -ForegroundColor Red
    exit 1
}

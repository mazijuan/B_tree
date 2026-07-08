<#
Runs all test_*.exe files found in the db-engine folder and its parent.
Usage:
  PowerShell -ExecutionPolicy Bypass -File .\run_all_tests.ps1
#>

param(
    [string]$Root = $PSScriptRoot
)

if (-not $Root) { $Root = Get-Location }

$dirs = @()
$dirs += (Resolve-Path -Path $Root -ErrorAction SilentlyContinue).ProviderPath
$parent = Split-Path -Path $Root -Parent
if ($parent) { $dirs += (Resolve-Path -Path $parent -ErrorAction SilentlyContinue).ProviderPath }

$dirs = $dirs | Where-Object { $_ } | Get-Unique

$exes = @()
foreach ($d in $dirs) {
    $found = Get-ChildItem -Path $d -Filter 'test_*.exe' -File -ErrorAction SilentlyContinue
    if ($found) { $exes += $found }
}

if (-not $exes -or $exes.Count -eq 0) {
    Write-Host "No test executables (test_*.exe) found in" $dirs -ForegroundColor Yellow
    exit 1
}

$failCount = 0
foreach ($exe in $exes | Sort-Object Name) {
    Write-Host '--------------------------------------------------'
    Write-Host "Running $($exe.FullName)"
    Write-Host '--------------------------------------------------'
    & "$($exe.FullName)"
    $code = $LASTEXITCODE
    if ($code -ne 0) {
        Write-Host "Exit code: $code" -ForegroundColor Red
        $failCount++
    } else {
        Write-Host "Exit code: $code" -ForegroundColor Green
    }
    Write-Host ""
}

if ($failCount -gt 0) {
    Write-Host "$failCount test(s) failed." -ForegroundColor Red
    exit 2
}

Write-Host "All tests finished successfully." -ForegroundColor Green
exit 0

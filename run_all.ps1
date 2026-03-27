$root = "C:\Users\akikf\programing\omnikarai\omniwin"
Set-Location $root

# Clean all scratch outputs
Get-ChildItem "tests\scratch" -Filter "*.txt" | Remove-Item -Force
Get-ChildItem "tests\scratch" -Filter "*.ps1" | Remove-Item -Force
Remove-Item "tests\scratch\t_*.ok" -ErrorAction SilentlyContinue

Set-Location "$root\tests"
& powershell -File run_tests.ps1
Write-Host ""
& powershell -File run_stress.ps1

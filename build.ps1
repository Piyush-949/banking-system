g++ -std=c++17 -Wall -Wextra -Iinclude *.cpp -o rbi_banking_system.exe

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild successful!`n" -ForegroundColor Green
    .\rbi_banking_system.exe
}
else {
    Write-Host "`nBuild failed.`n" -ForegroundColor Red
}
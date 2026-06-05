$extensions = @("*.gn", "*.gni", "*.py", "*.ts", "*.js", "*.html", "*.css", "*.mojom")
$directories = @(
    "n:\chromium\src\ui\webui\resources",
    "n:\chromium\src\tools\typescript"
)
$count = 0

foreach ($dir in $directories) {
    foreach ($ext in $extensions) {
        $files = Get-ChildItem -Recurse -Include $ext -Path $dir -ErrorAction SilentlyContinue
        foreach ($file in $files) {
            $content = Get-Content $file.FullName -Raw -ErrorAction SilentlyContinue
            if ($content -and $content.Contains("chrome://")) {
                $newContent = $content -replace '(?<![\w-])chrome://', 'neovex://'
                if ($newContent -ne $content) {
                    Set-Content -Path $file.FullName -Value $newContent -NoNewline
                    $count++
                    Write-Host "Updated: $($file.FullName)"
                }
            }
        }
    }
}
Write-Host "Done! Updated $count files."

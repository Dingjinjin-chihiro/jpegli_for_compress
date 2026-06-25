# 第二轮：修复遗漏的符号（namespace 声明、CamelCase 类型名）

$root = "C:\Code\Compress_Module_Optimizer\jpegli"
$excludeDirs = @("third_party", "build", "testdata", ".git", ".github", "debian")

Write-Host "=== 第二轮替换：修复遗漏符号 ===" -ForegroundColor Cyan

$files = Get-ChildItem -Path $root -Recurse -Include "*.h", "*.cc", "*.c", "*.cmake", "*.txt" |
    Where-Object {
        $inExcluded = $false
        foreach ($ex in $excludeDirs) {
            if ($_.FullName -match "\\$ex\\") { $inExcluded = $true; break }
        }
        -not $inExcluded
    }

$modifiedFiles = 0
$totalReplacements = 0

foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    if (-not $content) { continue }
    $original = $content

    # namespace jpegli -> namespace pdfcore（非文件路径）
    $content = $content -creplace '(?<!/)(?<![.\w])namespace jpegli\b', 'namespace pdfcore'

    # Jpegli -> Pdfcore 开头的类型/函数（仅当后面有大写字母，且前面没有 pdfcore_）
    $content = $content -creplace '(?<!Pdfcore)(?<!pdfcore_)Jpegli(?=[A-Z])', 'Pdfcore'
    
    if ($content -ne $original) {
        $count = [regex]::Matches($original, '\bjpegli\b').Count - [regex]::Matches($content, '\bjpegli\b').Count
        if ($count -le 0) { $count = 1 }
        Set-Content $file.FullName $content -NoNewline -Force
        $modifiedFiles++
        $totalReplacements += $count
        Write-Host "  $($file.FullName.Substring($root.Length+1))" -ForegroundColor Gray
    }
}

Write-Host "=== 完成 ===" -ForegroundColor Green
Write-Host "修改文件: $modifiedFiles" -ForegroundColor Yellow

<#
.SYNOPSIS
    用 Authenticode 对 QueMusic 的 exe/dll 做代码签名（SHA-256 + RFC3161 时间戳）。

.DESCRIPTION
    签名对象应包括：主程序、Qt 运行库、插件 dll、安装包。
    只签安装包不够：用户直接运行解压后的 exe 时，SmartScreen 看的是那个 exe 的签名与信誉。

    证书两种来源：
      * PFX 文件      -PfxPath .\cert.pfx -PfxPassword ***
      * 本机证书存储  -Thumbprint <SHA1 指纹>

    示例：
      .\sign.ps1 -Path ..\..\build\bin -PfxPath .\cert.pfx -PfxPassword ***
      .\sign.ps1 -Path ..\..\dist\QueMusic-0.6.0-win64-setup.exe -Thumbprint ABC...

    时间戳服务器：
      DigiCert  http://timestamp.digicert.com
      Sectigo   http://timestamp.sectigo.com
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [string]$PfxPath,
    [string]$PfxPassword,
    [string]$Thumbprint,

    [string]$TimestampUrl = 'http://timestamp.digicert.com',

    [switch]$NoVerify
)

$ErrorActionPreference = 'Stop'

if (-not $PfxPath -and -not $Thumbprint) {
    throw '必须提供 -PfxPath（配合 -PfxPassword）或 -Thumbprint'
}
if ($PfxPath -and -not (Test-Path $PfxPath)) {
    throw "找不到证书文件：$PfxPath"
}

function Find-SignTool {
    $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $roots = @(
        (Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Windows Kits\10\bin'),
        (Join-Path $env:ProgramFiles 'Windows Kits\10\bin')
    )
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $tool = Get-ChildItem $root -Recurse -Filter signtool.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\' } |
            Sort-Object FullName -Descending | Select-Object -First 1
        if ($tool) { return $tool.FullName }
    }
    throw '找不到 signtool.exe，请安装 Windows SDK。'
}

function Test-PeFile([string]$File) {
    $stream = [IO.File]::OpenRead($File)
    try {
        return ($stream.ReadByte() -eq 0x4D -and $stream.ReadByte() -eq 0x5A)
    } finally {
        $stream.Dispose()
    }
}

$target = Get-Item -LiteralPath $Path
if ($target.PSIsContainer) {
    $files = Get-ChildItem -LiteralPath $target.FullName -Recurse -File -Include *.exe, *.dll
} else {
    $files = @($target)
}
$files = @($files | Where-Object { Test-PeFile $_.FullName })

if ($files.Count -eq 0) {
    throw "在 $Path 下没有找到 PE 文件（.exe / .dll）"
}

$signTool = Find-SignTool
Write-Host "signtool: $signTool"
Write-Host "待签名文件数: $($files.Count)"

$baseArgs = @('sign', '/fd', 'SHA256', '/td', 'SHA256', '/tr', $TimestampUrl)
if ($PfxPath) {
    $baseArgs += @('/f', (Resolve-Path $PfxPath).Path)
    if ($PfxPassword) { $baseArgs += @('/p', $PfxPassword) }
} else {
    $baseArgs += @('/sha1', $Thumbprint)
}

$failed = 0
foreach ($file in $files) {
    Write-Host "  -> $($file.FullName)"
    & $signTool @baseArgs $file.FullName
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  !! 签名失败: $($file.FullName)" -ForegroundColor Red
        $failed++
    }
}

if ($failed -gt 0) {
    throw "$failed 个文件签名失败"
}

if (-not $NoVerify) {
    foreach ($file in $files) {
        & $signTool verify /pa /q $file.FullName
        if ($LASTEXITCODE -ne 0) {
            throw "签名验证失败: $($file.FullName)"
        }
    }
}

Write-Host "完成：$($files.Count) 个文件已签名并验证。" -ForegroundColor Green

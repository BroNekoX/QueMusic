<#
.SYNOPSIS
    把 QueMusic 的 windeployqt 输出打成可分发的 MSIX。

.DESCRIPTION
    前置条件：
      * 已用 Qt 6.10.3 完成 Release 构建（build\...\bin\QueMusic.exe 存在）
      * Windows SDK（makeappx.exe / signtool.exe）
      * 本地安装测试需要自签名证书，Subject 必须与 -Publisher 一致

    示例（一行）：
      .\build-msix.ps1 -QtBin C:\Qt\6.10.3\llvm-mingw_64\bin -Publisher "CN=BroNekoX" -Version 0.6.0.0

    上传 Microsoft Store 用 .msixupload；本地测试用 .msix。
#>
[CmdletBinding()]
param(
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\..\build\Desktop_Qt_6_10_3_llvm_mingw_64_bit_Release'),
    [string]$QtBin = 'C:\Qt\6.10.3\llvm-mingw_64\bin',
    [string]$OutputDir = (Join-Path $PSScriptRoot '..\..\dist\msix'),
    [string]$IdentityName = 'BroNekoX.QueMusic',
    [string]$Publisher = 'CN=BroNekoX',
    [string]$PublisherName = 'BroNekoX',
    [string]$DisplayName = 'QueMusic',
    [string]$Description = 'A high-performance music player built with Qt 6 and QML',
    [string]$Version = '0.6.0.0',
    [string]$CertificatePath = '',
    [string]$CertificatePassword = ''
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function Get-SdkTool {
    param([string]$Name)

    $pf86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $roots = @(
        (Join-Path $pf86 'Windows Kits\10\bin'),
        (Join-Path $env:ProgramFiles 'Windows Kits\10\bin')
    ) | Where-Object { Test-Path $_ }

    foreach ($root in $roots) {
        $tool = Get-ChildItem $root -Recurse -Filter $Name -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\' } |
            Sort-Object FullName -Descending | Select-Object -First 1
        if ($tool) { return $tool.FullName }
    }
    throw "找不到 $Name，请安装 Windows SDK。"
}

function Export-Icon {
    param([string]$Source, [string]$Destination, [int]$Size)

    $image = [System.Drawing.Image]::FromFile($Source)
    try {
        $bitmap = New-Object System.Drawing.Bitmap $Size, $Size
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.DrawImage($image, 0, 0, $Size, $Size)
            } finally { $graphics.Dispose() }
            $bitmap.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $bitmap.Dispose() }
    } finally { $image.Dispose() }
}

$exe = Join-Path $BuildDir 'bin\QueMusic.exe'
if (-not (Test-Path $exe)) { throw "找不到 $exe，请先完成 Release 构建。" }
if (-not (Test-Path (Join-Path $QtBin 'windeployqt.exe'))) { throw "找不到 windeployqt：$QtBin" }

$staging = Join-Path $OutputDir 'staging'
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Path $staging, (Join-Path $staging 'Assets') -Force | Out-Null

Write-Host '==> 拷贝构建产物…'
Copy-Item (Join-Path $BuildDir 'bin\*') $staging -Recurse -Force

Write-Host '==> windeployqt…'
Push-Location $staging
try {
    $qmlRoot = [string](Resolve-Path (Join-Path $PSScriptRoot '..\..'))
    & (Join-Path $QtBin 'windeployqt.exe') --release --no-opengl-sw --qmldir $qmlRoot (Join-Path $staging 'QueMusic.exe')
    if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败（$LASTEXITCODE）" }
} finally { Pop-Location }

Write-Host '==> 生成 MSIX 图标…'
$iconSource = Join-Path $PSScriptRoot '..\..\resources\app\rainbowMusicIcon.png'
Export-Icon $iconSource (Join-Path $staging 'Assets\Square44x44Logo.png') 44
Export-Icon $iconSource (Join-Path $staging 'Assets\Square150x150Logo.png') 150
Export-Icon $iconSource (Join-Path $staging 'Assets\Square310x310Logo.png') 310
Export-Icon $iconSource (Join-Path $staging 'Assets\StoreLogo.png') 50

# 宽图 310x150 只能单独画：透明底 + 居中 logo
$wide = New-Object System.Drawing.Bitmap 310, 150
try {
    $g = [System.Drawing.Graphics]::FromImage($wide)
    try {
        $g.Clear([System.Drawing.Color]::Transparent)
        $img = [System.Drawing.Image]::FromFile($iconSource)
        try {
            $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $g.DrawImage($img, 85, 5, 140, 140)
        } finally { $img.Dispose() }
    } finally { $g.Dispose() }
    $wide.Save((Join-Path $staging 'Assets\Wide310x150Logo.png'), [System.Drawing.Imaging.ImageFormat]::Png)
} finally { $wide.Dispose() }

Write-Host '==> 生成 AppxManifest.xml…'
$template = Get-Content (Join-Path $PSScriptRoot 'AppxManifest.xml') -Raw
$template = $template.Replace('{{IDENTITY}}', $IdentityName)
$template = $template.Replace('{{PUBLISHER}}', $Publisher)
$template = $template.Replace('{{PUBLISHERNAME}}', $PublisherName)
$template = $template.Replace('{{DISPLAYNAME}}', $DisplayName)
$template = $template.Replace('{{DESCRIPTION}}', $Description)
$template = $template.Replace('{{VERSION}}', $Version)
Set-Content (Join-Path $staging 'AppxManifest.xml') $template -Encoding UTF8

$msix = Join-Path $OutputDir ("QueMusic-" + $Version + "-x64.msix")
Write-Host "==> makeappx pack -> $msix"
& (Get-SdkTool 'makeappx.exe') pack /d $staging /p $msix /o
if ($LASTEXITCODE -ne 0) { throw "makeappx 失败（$LASTEXITCODE）" }

if ($CertificatePath) {
    Write-Host '==> 签名…'
    $signArgs = @('sign', '/fd', 'SHA256', '/f', $CertificatePath)
    if ($CertificatePassword) { $signArgs += @('/p', $CertificatePassword) }
    $signArgs += $msix
    & (Get-SdkTool 'signtool.exe') @signArgs
    if ($LASTEXITCODE -ne 0) { throw "signtool 失败（$LASTEXITCODE）" }
}

$upload = Join-Path $OutputDir ("QueMusic-" + $Version + "-x64.msixupload")
$tempZip = Join-Path $OutputDir ("QueMusic-" + $Version + "-x64.zip")
if (Test-Path $upload) { Remove-Item $upload -Force }
if (Test-Path $tempZip) { Remove-Item $tempZip -Force }
Compress-Archive -Path $msix -DestinationPath $tempZip -Force
Move-Item $tempZip $upload -Force

Write-Host ""
Write-Host "完成："
Write-Host "  MSIX      : $msix"
Write-Host "  MSIXUPLOAD: $upload"
Write-Host "本地安装测试：Add-AppxPackage -Path '$msix'（需先信任自签名证书）"

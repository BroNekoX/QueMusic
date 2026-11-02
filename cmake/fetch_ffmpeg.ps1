# 维护脚本：重建 <repo>/ffmpeg/{include,lib}
#
# 正常构建**不需要**运行本脚本——这批文件随仓库提供、构建无需联网。
# 运行库 dll 不参与入库，构建时直接用 Qt 安装目录里的同名文件。
# 仅在更换 FFmpeg 版本或需重新生成导入库时使用。脚本本身全程不下载：
#   1) lib/      用 gendef + llvm-dlltool 从 Qt 自带 DLL 的导出表生成导入库
#   2) include/  仅换版本时指定 -SourceDir：复制本地源码头文件并补齐 configure 生成的两个头
#
# 用法：
#   powershell -ExecutionPolicy Bypass -File cmake/fetch_ffmpeg.ps1
#   powershell -ExecutionPolicy Bypass -File cmake/fetch_ffmpeg.ps1 -SourceDir C:\ffmpeg-7.1.3
param(
    [string]$QtBin,       # Qt 的 bin 目录（含 avcodec-61.dll 等）
    [string]$Toolchain,   # llvm-mingw 的 bin（含 gendef.exe / llvm-dlltool.exe）
    [string]$SourceDir    # 可选的 FFmpeg 源码目录（已解压）
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$dest = Join-Path $root "ffmpeg"
$ffver = "7.1.3"
$modules = @{
    "avcodec-61"   = "libavcodec"
    "avformat-61"  = "libavformat"
    "avutil-59"    = "libavutil"
    "swresample-5" = "libswresample"
}

if (-not $QtBin) {
    $found = Get-ChildItem "C:\Qt" -Recurse -Depth 4 -Filter "avcodec-61.dll" -ErrorAction SilentlyContinue |
             Select-Object -First 1
    if ($found) { $QtBin = $found.DirectoryName }
}
if (-not $Toolchain) {
    $found = Get-ChildItem "C:\Qt\Tools" -Recurse -Depth 3 -Filter "llvm-dlltool.exe" -ErrorAction SilentlyContinue |
             Select-Object -First 1
    if ($found) { $Toolchain = $found.DirectoryName }
}
if (-not $QtBin)     { throw "未找到 Qt 的 FFmpeg 共享库（avcodec-61.dll），请用 -QtBin 指定" }
if (-not $Toolchain) { throw "未找到 llvm-dlltool，请用 -Toolchain 指定 llvm-mingw 的 bin 目录" }

New-Item -ItemType Directory -Force -Path (Join-Path $dest "lib") | Out-Null

$tmp = Join-Path $env:TEMP "quemusic-ffmpeg-def"
Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
Push-Location $tmp
foreach ($k in $modules.Keys) {
    & (Join-Path $Toolchain "gendef.exe") (Join-Path $QtBin "$k.dll") | Out-Null
    & (Join-Path $Toolchain "llvm-dlltool.exe") -d "$k.def" -D "$k.dll" `
        -l (Join-Path $dest "lib\$($modules[$k]).dll.a")
}
Pop-Location
Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue

if ($SourceDir) {
    foreach ($d in @("libavcodec", "libavformat", "libavutil", "libswresample")) {
        $target = Join-Path $dest "include\$d"
        New-Item -ItemType Directory -Force -Path $target | Out-Null
        Get-ChildItem (Join-Path $SourceDir "$d\*.h") | Copy-Item -Destination $target -Force
    }
    Copy-Item (Join-Path $SourceDir "COPYING.LGPLv2.1") (Join-Path $dest "LICENSE.txt") -Force
    # 这两个头由 configure 生成，源码包里没有
    Set-Content -Path (Join-Path $dest "include\libavutil\avconfig.h") -Encoding ASCII -Value @"
#ifndef AVUTIL_AVCONFIG_H
#define AVUTIL_AVCONFIG_H
#define AV_HAVE_BIGENDIAN 0
#define AV_HAVE_FAST_UNALIGNED 1
#endif
"@
    Set-Content -Path (Join-Path $dest "include\libavutil\ffversion.h") -Encoding ASCII `
        -Value "#define FFMPEG_VERSION `"$ffver`""
}

Write-Host "完成：$dest"

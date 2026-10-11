# Memory soak test for the JIZURA plugin.
#
#   pwsh -File tools/soak.ps1 -Seconds 180
#
# Runs the plugin through tests/harness.qml and samples the process working set
# every few seconds, so a leak in the recorder caches, the QML pools or a pack's
# own caching shows up as a rising curve instead of a crash after ten minutes.

param(
    [int]$Seconds = 180,
    [int]$SampleEvery = 10,
    [string]$Quality = "auto",
    [string]$QmlExe = "C:\Qt\6.10.3\llvm-mingw_64\bin\qml.exe"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$tests = Join-Path $root "tests"
$env:PATH = (Split-Path -Parent $QmlExe) + ";" + $env:PATH

Write-Host "soak: $Seconds s, quality=$Quality, sampling every ${SampleEvery}s"
$log = Join-Path $env:TEMP ("jizura-soak-" + [guid]::NewGuid().ToString("N") + ".log")
$proc = Start-Process -FilePath $QmlExe -ArgumentList @("harness.qml", "--", "--run", "$Seconds", "--quality", $Quality) `
                      -WorkingDirectory $tests -PassThru -RedirectStandardOutput $log -RedirectStandardError "$log.err" -NoNewWindow

$samples = @()
$t0 = Get-Date
while (-not $proc.HasExited) {
    Start-Sleep -Seconds $SampleEvery
    try { $proc.Refresh() } catch { break }
    if ($proc.HasExited) { break }
    $ws = [math]::Round($proc.WorkingSet64 / 1MB, 1)
    $pv = [math]::Round($proc.PrivateMemorySize64 / 1MB, 1)
    $samples += [pscustomobject]@{ t = [int]((Get-Date) - $t0).TotalSeconds; wsMB = $ws; privateMB = $pv }
    Write-Host ("  t={0,4}s  working set {1,8} MB   private {2,8} MB" -f $samples[-1].t, $ws, $pv)
}

if ($proc.ExitCode -ne 0 -and $proc.ExitCode -ne $null) {
    Write-Host "process exit code: $($proc.ExitCode)  (qml.exe returns 1 when the window closes early; check the log)"
}

Write-Host "`n--- harness output ---"
if (Test-Path $log) { Get-Content $log | Where-Object { $_ -notmatch "usedbeforedeclared" } | Select-Object -Last 6 }
if ((Test-Path "$log.err") -and (Get-Item "$log.err").Length -gt 0) {
    $err = Get-Content "$log.err" | Where-Object { $_ -notmatch "usedbeforedeclared|^所在位置|^\+|CategoryInfo|FullyQualifiedErrorId" }
    if ($err) { Write-Host "--- stderr ---"; $err | Select-Object -First 10 }
}

if ($samples.Count -ge 3) {
    $first = ($samples | Select-Object -First 3 | Measure-Object wsMB -Average).Average
    $last = ($samples | Select-Object -Last 3 | Measure-Object wsMB -Average).Average
    $delta = $last - $first
    Write-Host ("`nworking set: {0:N1} MB -> {1:N1} MB  (delta {2:+0.0;-0.0} MB over {3}s)" -f $first, $last, $delta, $Seconds)
    if ($delta -gt 40) { Write-Host "VERDICT: GROWING - investigate" }
    else { Write-Host "VERDICT: stable" }
} else {
    Write-Host "`nnot enough samples"
}
Remove-Item $log, "$log.err" -ErrorAction SilentlyContinue

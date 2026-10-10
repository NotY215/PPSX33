# Restores src/gui/MainForm.cs from MainForm.cs.gz.b64
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if (-not (Test-Path (Join-Path $PSScriptRoot '..\src\gui'))) { $root = (Get-Location).Path }
$b64path = Join-Path $root 'src\gui\MainForm.cs.gz.b64'
$outpath = Join-Path $root 'src\gui\MainForm.cs'
if (-not (Test-Path $b64path)) { Write-Error "Missing $b64path"; exit 1 }
$bytes = [Convert]::FromBase64String((Get-Content -Raw $b64path))
$ms = New-Object System.IO.MemoryStream(,$bytes)
$gz = New-Object System.IO.Compression.GzipStream($ms, [System.IO.Compression.CompressionMode]::Decompress)
$out = New-Object System.IO.MemoryStream
$gz.CopyTo($out)
[IO.File]::WriteAllBytes($outpath, $out.ToArray())
Write-Host "Restored $outpath ($($out.Length) bytes)"

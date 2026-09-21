param(
    [ValidateSet('Adaptive', 'Original', 'Overlap', 'Compare')]
    [string]$Mode = 'Adaptive',
    [string]$RuntimeDirectory = (Join-Path $env:LOCALAPPDATA 'OFXR Bridge\RuntimeLayer\v116')
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $RuntimeDirectory -PathType Container)) {
    throw "Runtime directory does not exist: $RuntimeDirectory"
}
$modes = @{ Adaptive = 0; Original = 1; Overlap = 2; Compare = 3 }
$path = Join-Path $RuntimeDirectory 'ofxr-scheduling.ini'
$contents = "[scheduling]`r`nmode=$($modes[$Mode])`r`n"
[IO.File]::WriteAllText($path, $contents, [Text.Encoding]::ASCII)
Write-Output "Next OpenXR session: $Mode ($path)"
if ($Mode -eq 'Compare') {
    Write-Output 'Keep the scene, DLSS mode and Cheeky state fixed for about three minutes.'
    Write-Output '30 s warm-up, then 30 s each: Original, Overlap, Original, Overlap; then Adaptive.'
}

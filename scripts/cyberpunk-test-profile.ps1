param([ValidateSet('Apply','Restore')][string]$Mode='Apply')
$ErrorActionPreference='Stop'
function Set-TestProfile([string]$Text) {
 $values=[ordered]@{backend='nvidia';nvidia_preset='medium';nvidia_input_scale='50';nvidia_bidirectional='0';triple_frame_gen='0';hybrid='0';hybrid_depth_search='0';deep_pipeline='0';single_swapchain_rings='0';capture_at_end_frame='1';panel_frame_generation='0';independent_synthesis_queues='1';diagnostics='1'}
 $lines=[Collections.Generic.List[string]]::new()
 $inTray=$false; $seen=$false
 foreach($line in ($Text -split '\r?\n')) {
  if($line -match '^\s*\[([^\]]+)\]\s*$'){
   $inTray=$Matches[1] -ieq 'tray'
   if($inTray){
    if($seen){throw 'Duplicate [tray] sections; settings left unchanged.'}
    $seen=$true; $lines.Add('[tray]')
    foreach($key in $values.Keys){$lines.Add("$key=$($values[$key])")}
    continue
   }
  }
  if($inTray -and $line -match '^\s*([^=]+?)\s*='){
   if($values.Contains($Matches[1].Trim().ToLowerInvariant())){continue}
  }
  $lines.Add($line)
 }
 if(!$seen){
  $lines.Add('[tray]')
  foreach($key in $values.Keys){$lines.Add("$key=$($values[$key])")}
 }
 return ($lines -join "`r`n")
}
# Dot sourcing exposes the pure transformer for fixture tests only.
if($MyInvocation.InvocationName -eq '.'){return}
try {
 if(Get-Process Cyberpunk2077,OFXRBridgeTray -ErrorAction SilentlyContinue){throw 'Close Cyberpunk and Exit all OFXR trays before applying/restoring settings.'}
 $package=Split-Path $PSScriptRoot -Parent
 $tray=Join-Path $package 'OFXRBridgeTray.exe'
 if(!(Test-Path -LiteralPath $tray -PathType Leaf)){throw 'Extract the entire release ZIP before running this helper.'}
 $settingsRoot=Join-Path $env:LOCALAPPDATA 'OFXR Bridge'
 $settings=Join-Path $settingsRoot 'tray.ini'
 $backup=Join-Path $settingsRoot 'cyberpunk-sbs-v557-rc1-profile-backup'
 $marker=Join-Path $backup 'state.txt'
 if($Mode -eq 'Restore'){
  if(!(Test-Path -LiteralPath $marker)){throw 'No RC1 profile backup found. Nothing changed.'}
  $state=[IO.File]::ReadAllText($marker).Trim()
  if($state -eq 'existing'){Copy-Item -LiteralPath (Join-Path $backup 'tray.ini') -Destination $settings -Force}
  elseif($state -eq 'absent'){if(Test-Path -LiteralPath $settings){Remove-Item -LiteralPath $settings}}
  else{throw 'Unrecognized backup state; settings left unchanged.'}
  Write-Host 'Previous settings restored. Launch your previous OFXR tray from its original folder.'
  exit 0
 }
 $original=if(Test-Path -LiteralPath $settings){[IO.File]::ReadAllText($settings)}else{''}
 $updated=Set-TestProfile $original
 New-Item -ItemType Directory -Force $settingsRoot | Out-Null
 if(!(Test-Path -LiteralPath $marker)){
  New-Item -ItemType Directory -Force $backup | Out-Null
  if(Test-Path -LiteralPath $settings){Copy-Item -LiteralPath $settings -Destination (Join-Path $backup 'tray.ini') -Force; $state='existing'}else{$state='absent'}
  [IO.File]::WriteAllText($marker,$state)
 }
 [IO.File]::WriteAllText($settings,$updated,[Text.UTF8Encoding]::new($false))
 Write-Host "Applied Cyberpunk test profile. Previous settings: $backup"
 Start-Process -FilePath $tray -WorkingDirectory $package
 Write-Host 'OFXR started. Keep built-in port framegen OFF; launch Cyberpunk normally.'
 exit 0
}catch{Write-Host "STOPPED: $($_.Exception.Message)" -ForegroundColor Red;exit 1}

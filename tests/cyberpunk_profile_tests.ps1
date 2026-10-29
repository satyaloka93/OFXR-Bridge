$ErrorActionPreference='Stop'
. "$PSScriptRoot/../scripts/cyberpunk-test-profile.ps1"
$inputText="[tray]`r`nbackend=fidelityfx`r`nindependent_synthesis_queues=0`r`noverlay_size=big`r`n[other]`r`nbackend=keep"
$output=Set-TestProfile $inputText
if($output -notmatch 'backend=nvidia' -or $output -notmatch 'independent_synthesis_queues=1' -or $output -notmatch 'overlay_size=big' -or $output -notmatch 'backend=keep'){throw 'Profile merge failed'}
if((Set-TestProfile $output) -ne $output){throw 'Profile is not idempotent'}
if((Set-TestProfile '') -notmatch 'panel_frame_generation=0'){throw 'Fresh profile failed'}
$rejected=$false
try {Set-TestProfile "[tray]`n[tray]" | Out-Null}catch{$rejected=$true}
if(!$rejected){throw 'Duplicate section not rejected'}
Write-Output 'Profile fixtures passed: merge, unrelated settings, idempotence, fresh install, duplicate rejection.'

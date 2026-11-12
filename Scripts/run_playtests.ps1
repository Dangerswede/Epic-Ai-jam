# Runs the automated playtests (Retake.AutoTest) headless for every scene.
# Usage:  powershell -File Scripts\run_playtests.ps1 [-Scenes Scene01_Bridge,...] [-Scenarios solution,wrong]
# Needs the editor target built. Uses a separate save slot (-RetakeTestSave), never your real progress.
param(
    [string[]]$Scenes = @("Scene01_Bridge", "Scene02_Heist", "Scene03_Chase", "Scene04_Finale", "Scene05_TheGap"),
    [string[]]$Scenarios = @("solution", "wrong"),
    [string]$Engine = "E:\Unreal engine\UE_5.9"
)
$project = Join-Path $PSScriptRoot "..\EpicAiJam.uproject" | Resolve-Path
$logDir = Join-Path $PSScriptRoot "..\Saved\Playtests"
New-Item -ItemType Directory -Force $logDir | Out-Null
$save = Join-Path $PSScriptRoot "..\Saved\SaveGames\RetakeProgress_Test.sav"
$results = @()
foreach ($scene in $Scenes) {
    foreach ($scenario in $Scenarios) {
        # Fresh test save per run, pre-unlocked up to this scene.
        Remove-Item $save -ErrorAction SilentlyContinue
        $log = Join-Path $logDir "$scene-$scenario.log"
        & "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project "/Game/Retake/Maps/$scene" -game -nullrhi -nosound -unattended `
            -benchmark -fps=60 -RetakeTestSave -RetakeTestQuit "-ExecCmds=Retake.AutoTest $scenario" "-abslog=$log" 2>&1 | Out-Null
        $line = Select-String $log -Pattern "RETAKE_TEST .*: (PASS|FAIL)" | Select-Object -Last 1
        if (-not $line) { $line = "RETAKE_TEST $scene ${scenario}: FAIL - no result (crash or never started), see $log" } else { $line = $line.Line -replace '^.*?RETAKE_TEST', 'RETAKE_TEST' }
        $results += $line
        Write-Output $line
    }
}
if ($results -match "FAIL") { exit 1 } else { exit 0 }

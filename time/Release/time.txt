@echo off
setlocal

set "TASK_NAME=TimeSyncOnLoginAndEvery6Hours"
set "EXE_PATH=%~dp0time.exe"

:: 1. 작업 등록 (로그온 시 + 6시간마다)
schtasks /Create ^
 /TN "%TASK_NAME%" ^
 /TR "\"%EXE_PATH%\"" ^
 /SC HOURLY ^
 /MO 6 ^
 /RL HIGHEST ^
 /F ^
 /RU "" ^
 /IT

:: 2. 옵션 수정: 요청 시 실행 비활성화 + 예약된 시간 놓치면 가능한 빨리 실행
powershell -Command " \
$task = Get-ScheduledTask -TaskName '%TASK_NAME%'; \
$task.Settings.AllowDemandStart = $false; \
$task.Settings.StartWhenAvailable = $true; \
$task.Triggers += New-ScheduledTaskTrigger -AtLogOn; \
Set-ScheduledTask -TaskName '%TASK_NAME%' -Settings $task.Settings -Triggers $task.Triggers \
"

echo 작업 등록 완료:
echo - 로그인 시 자동 실행
echo - 6시간마다 반복 실행
echo - 요청 시 수동 실행: 비활성화
echo - 예약된 시간 놓치면 가능한 빨리 실행: 활성화
pause

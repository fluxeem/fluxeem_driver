@echo off
rem SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
rem SPDX-License-Identifier: Apache-2.0
rem
rem Windows USB 驱动卸载脚本

set base_dir=%~dp0
%base_dir:~0,2%
pushd %base_dir%

set wintemp=%SYSTEMROOT%\temp\FluxeemInstall
mkdir %wintemp% 2>nul
if not exist "%wintemp%" (
	set wintemp=%TEMP%\FluxeemInstall
	mkdir %wintemp% 2>nul
)
set _LOG_FILE=%wintemp%\FluxeemDriverLog.log

echo. >> %_LOG_FILE%
echo [%date% %time%] >> %_LOG_FILE%
echo %cd% >> %_LOG_FILE%

net session >nul 2>&1
if errorlevel 1 (
	echo running as admin: no >> %_LOG_FILE%
	echo [ERROR] Administrator privileges are required. >> %_LOG_FILE%
	popd
	exit /b 740
)
echo running as admin: yes >> %_LOG_FILE%

rem Resolve pnputil/PowerShell path: NSIS is 32-bit, WOW64 redirects System32 -> SysWOW64.
rem pnputil.exe only exists as a 64-bit binary. Use Sysnative to bypass redirection.
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" (
	set PS_EXE="%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
	echo [INFO] Using Sysnative PowerShell ^(32-bit host process^) >> %_LOG_FILE%
) else (
	set PS_EXE=powershell
	echo [INFO] Using PATH PowerShell ^(64-bit host process^) >> %_LOG_FILE%
)
echo [UNINSTALL] generating uninstall script ... >> %_LOG_FILE%
(
	echo $targets = @^(
	echo     @{name='ApexVision-S1';                    pat='VID_04B4.PID_0101'},
	echo     @{name='EVK4';                    pat='VID_04B4.PID_00F5'},
	echo     @{name='RDK3';                    pat='VID_04B4.PID_00F4'},
	echo     @{name='DVSLume';                 pat='VID_04B5.PID_0001'},
	echo     @{name='SENSING_USB3_EVS_CAMERA'; pat='VID_04B4.PID_00C4'}
	echo ^)
	echo $oem = $null
	echo foreach ^($line in ^(pnputil /enum-drivers^)^) {
	echo     if ^($line -match 'Published Name:\s+^(\S+^)'^) { $oem = $Matches[1] }
	echo     elseif ^($oem -and ^($line -match 'Signer Name'^)^) {
	echo         foreach ^($t in $targets^) {
	echo             if ^($line -match $t.pat^) {
	echo                 Write-Host ^("[UNINSTALL] removing " + $t.name + " ^(" + $oem + "^)"^)
	echo                 pnputil /delete-driver $oem /uninstall /force
	echo                 $oem = $null; break
	echo             }
	echo         }
	echo     }
	echo }
) > "%wintemp%\uninstall_drivers.ps1"
echo [UNINSTALL] searching and removing Fluxeem USB drivers ... >> %_LOG_FILE%
%PS_EXE% -NoProfile -ExecutionPolicy Bypass -File "%wintemp%\uninstall_drivers.ps1" >> %_LOG_FILE% 2>&1
set PNPUTIL_EXIT_CODE=%ERRORLEVEL%
if "%PNPUTIL_EXIT_CODE%"=="3010" (
	echo [PNPUTIL] reboot required ^(code 3010^), treated as success. >> %_LOG_FILE%
	set PNPUTIL_EXIT_CODE=0
)
echo [UNINSTALL] exit code: %PNPUTIL_EXIT_CODE% >> %_LOG_FILE%
@ping 127.0.0.1 -n 2 >nul
echo driver uninstall completed >> %_LOG_FILE%
echo. >> %_LOG_FILE%

popd
exit /b %PNPUTIL_EXIT_CODE%

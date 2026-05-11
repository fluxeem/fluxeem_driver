@echo off
rem SPDX-FileCopyrightText: 2024-present Fluxeem Technologies Co., Ltd.
rem SPDX-License-Identifier: Apache-2.0
rem
rem Windows USB 驱动安装脚本 - 安装证书并注册 WinUSB 驱动

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

rem Resolve pnputil path: NSIS is 32-bit; from a 32-bit process %SystemRoot%\System32
rem is redirected to SysWOW64 where pnputil.exe does not exist.
rem Sysnative bypasses WOW64 redirection and reaches the real 64-bit System32.
if exist "%SystemRoot%\Sysnative\pnputil.exe" (
	set PNPUTIL=%SystemRoot%\Sysnative\pnputil.exe
	echo [INFO] Using Sysnative pnputil ^(32-bit host process^) >> %_LOG_FILE%
) else (
	set PNPUTIL=pnputil
	echo [INFO] Using PATH pnputil ^(64-bit host process^) >> %_LOG_FILE%
)
echo driver is installing, please wait for a few minutes ... >> %_LOG_FILE%
echo adding certificates to TrustedPublisher and Root ... >> %_LOG_FILE%

echo [CERT] SENSING_USB3_EVS_CAMERA.cer -> TrustedPublisher >> %_LOG_FILE%
certutil.exe -f -addstore "TrustedPublisher" "%base_dir%usb_driver\SENSING_USB3_EVS_CAMERA.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%
echo [CERT] SENSING_USB3_EVS_CAMERA.cer -> Root >> %_LOG_FILE%
certutil.exe -f -addstore "Root" "%base_dir%usb_driver\SENSING_USB3_EVS_CAMERA.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%

echo [CERT] DVSLume.cer -> TrustedPublisher >> %_LOG_FILE%
certutil.exe -f -addstore "TrustedPublisher" "%base_dir%usb_driver\DVSLume.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%
echo [CERT] DVSLume.cer -> Root >> %_LOG_FILE%
certutil.exe -f -addstore "Root" "%base_dir%usb_driver\DVSLume.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%

echo [CERT] RDK3.cer -> TrustedPublisher >> %_LOG_FILE%
certutil.exe -f -addstore "TrustedPublisher" "%base_dir%usb_driver\RDK3.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%
echo [CERT] RDK3.cer -> Root >> %_LOG_FILE%
certutil.exe -f -addstore "Root" "%base_dir%usb_driver\RDK3.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%

echo [CERT] EVK4.cer -> TrustedPublisher >> %_LOG_FILE%
certutil.exe -f -addstore "TrustedPublisher" "%base_dir%usb_driver\EVK4.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%
echo [CERT] EVK4.cer -> Root >> %_LOG_FILE%
certutil.exe -f -addstore "Root" "%base_dir%usb_driver\EVK4.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%

echo [CERT] ApexVision-S1.cer -> TrustedPublisher >> %_LOG_FILE%
certutil.exe -f -addstore "TrustedPublisher" "%base_dir%usb_driver\ApexVision-S1.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%
echo [CERT] ApexVision-S1.cer -> Root >> %_LOG_FILE%
certutil.exe -f -addstore "Root" "%base_dir%usb_driver\ApexVision-S1.cer" >> %_LOG_FILE% 2>&1
echo [CERT] exit code: %ERRORLEVEL% >> %_LOG_FILE%

%PNPUTIL% /scan-devices >> %_LOG_FILE% 2>&1
echo [SCAN] exit code: %ERRORLEVEL% >> %_LOG_FILE%

echo installing drivers ... >> %_LOG_FILE%
%PNPUTIL% /add-driver "%base_dir%usb_driver\*.inf" /install >> %_LOG_FILE% 2>&1
set PNPUTIL_EXIT_CODE=%ERRORLEVEL%
if "%PNPUTIL_EXIT_CODE%"=="3010" (
	echo [PNPUTIL] reboot required ^(code 3010^), treated as success. >> %_LOG_FILE%
	set PNPUTIL_EXIT_CODE=0
)
echo [PNPUTIL] exit code: %PNPUTIL_EXIT_CODE% >> %_LOG_FILE%
@ping 127.0.0.1 -n 2 >nul
echo driver install completed (exit code: %PNPUTIL_EXIT_CODE%) >> %_LOG_FILE%
echo. >> %_LOG_FILE%

popd
exit /b %PNPUTIL_EXIT_CODE%

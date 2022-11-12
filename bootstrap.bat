@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

set VSWHERE_PATH="C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
rem vs2019-vs2022
set VSMIN=16.0
set VSMAX=18.0

set CURRENTDIR=%cd%
cd /D "%CURRENTDIR%"

set PREMAK5_PATH=%CURRENTDIR%/Premake/premake5.exe
echo Generating...

if not exist %VSWHERE_PATH% (
	echo ERROR: Could not find vswhere.exe, please check the path: !VSWHERE_PATH!.
) else (
	rem Try to search a particular version of visual studio.
	set VsWhereCmdLine="!VSWHERE_PATH! -nologo -latest -version [%VSMIN%,%VSMAX%) -property catalog.productLineVersion"
	
	for /f "usebackq delims=" %%i in (`!VsWhereCmdLine!`) do (
		echo Successfully find the latest visual studio : %%i
		echo.
		
		echo Premake5: Processing...
		call %PREMAK5_PATH% vs%%i || goto error
		echo.
		
		echo Script complete
		pause
		exit /b 0
	)
	
	echo ERROR: Could not find vs2019 or vs2022, please make sure you installed it.
)

echo Errors encountered during execution.
pause
exit /b 1

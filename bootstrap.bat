@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

set VSWHERE_PATH="C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"

set VSMIN=17.0
set VSMAX=30.0

set CURRENTDIR=%cd%
cd /D %CURRENTDIR%

set PREMAK5_PATH="%CURRENTDIR%/Premake/premake5.exe"
set VsVersion=%~1

echo Generating...
echo.

if not "%VsVersion%"=="" (
	call :GenerateVsProject

	pause
	exit /b 0
)

echo Try to search a particular version of visual studio.

if not exist %VSWHERE_PATH% (
	echo ERROR: Could not find vswhere.exe, please check the path: %VSWHERE_PATH%.
) else (
	set VsWhereCmdLine="!VSWHERE_PATH! -nologo -latest -version [%VSMIN%,%VSMAX%) -property catalog.productLineVersion"
	
	for /f "usebackq delims=" %%i in (`!VsWhereCmdLine!`) do (
		echo Successfully find the eligible visual studio
		echo.
		
		set VsVersion=vs2022
		call :GenerateVsProject

		pause
		exit /b 0
	)
	
	echo ERROR: Could not find vs2022 or later, please make sure you installed it.
)

:GenerateVsProject
echo Premake5: Processing...
call %PREMAK5_PATH% %VsVersion% || goto error
echo.	
echo Premake5 complete
pause
exit /b 0


:error
echo.
echo Errors encountered during execution!!!!!
pause
exit /b 1

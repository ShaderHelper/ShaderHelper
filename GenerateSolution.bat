@echo off

set CURRENTDIR=%cd%
cd /D "%CURRENTDIR%"

set PREMAK5_PATH=%CURRENTDIR%/Premake/premake5.exe
ECHO Premake5: Processing...

call %PREMAK5_PATH% vs2022

if %ERRORLEVEL% == 0 goto :endofscript
echo "Errors encountered during execution.  Exited with status: %errorlevel%"

:endofscript
echo "Script complete"

pause
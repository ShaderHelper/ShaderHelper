@echo off
set CURRENTDIR=%cd%

echo Donwloading UE-Src...
	set UE-Src_Dir=%CURRENTDIR%/Src
	set UE-Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c0c6af00cb2edf593ac76e9ca9135e2f/UE-Src.zip"
	cd /D %UE-Src_Dir%
	curl -LO %UE-Src_Url% || goto error
	tar -zxf UE-Src.zip || goto error
	del UE-Src.zip
echo.


echo Script complete
pause
exit /b 0

:error
echo.
echo Errors encountered during execution.
pause
exit /b 1

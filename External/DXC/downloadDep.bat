@echo off
set CURRENTDIR=%~dp0

echo Downloading DXC...
	set DXC_Dir=%CURRENTDIR%/Lib
	set DXC_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/1d895d024c8df6731cdba946b387bff5/DXC.zip"
	cd /D %DXC_Dir% || goto error
	curl -LO %DXC_Url% || goto error
	C:/Windows/System32/tar.exe -zxf DXC.zip || goto error
	del DXC.zip
echo.
exit /b 0

:error
exit /b 1

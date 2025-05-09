@echo off
setlocal 
set CURRENTDIR=%~dp0

echo Downloading DXC...
	set DXC_Dir=%CURRENTDIR%/Lib
	set DXC_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/DXC.zip"
	pushd "%DXC_Dir%" || goto error
	curl -LO %DXC_Url% || goto error
	C:/Windows/System32/tar.exe -zxf DXC.zip || goto error
	del DXC.zip
	popd
echo.
exit /b 0

:error
exit /b 1

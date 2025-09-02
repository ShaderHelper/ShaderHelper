@echo off
setlocal
set CURRENTDIR=%~dp0

echo Downloading built-in plugins...
	set Plugin_Dir=%CURRENTDIR%
	set Plugin_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/Plugins.zip"
	pushd "%Plugin_Dir=%" || goto error
	curl -LO %Plugin_Url% || goto error
	C:/Windows/System32/tar.exe -zxf Plugins.zip || goto error
	del Plugins.zip
	popd
echo.
exit /b 0
 
:error
exit /b 1

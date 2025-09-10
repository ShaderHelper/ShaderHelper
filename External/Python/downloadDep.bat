@echo off
setlocal
set CURRENTDIR=%~dp0

echo Downloading Python...
	set Python_Dir=%CURRENTDIR%
	set Python_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/Python-Win.zip"
	pushd "%Python_Dir%" || goto error
	curl -LO %Python_Url% || goto error
	C:/Windows/System32/tar.exe -zxf Python-Win.zip || goto error
	del Python-Win.zip
	popd
echo.
exit /b 0
 
:error
exit /b 1

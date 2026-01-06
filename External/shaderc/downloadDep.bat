@echo off
setlocal 
set CURRENTDIR=%~dp0

echo Downloading shaderc...
	set shaderc_Dir=%CURRENTDIR%/Lib
	set shaderc_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/shaderc-Win.zip"
	pushd "%shaderc_Dir%" || goto error
	curl -LO %shaderc_Url% || goto error
	C:/Windows/System32/tar.exe -zxf shaderc-Win.zip || goto error
	del shaderc-Win.zip
	popd
echo.
exit /b 0

:error
exit /b 1

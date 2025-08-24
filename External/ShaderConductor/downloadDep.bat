@echo off
setlocal 
set CURRENTDIR=%~dp0

echo Downloading ShaderConductor...
	set ShaderConductor_Dir=%CURRENTDIR%/Lib
	set ShaderConductor_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/ShaderConductor-Win.zip"
	pushd "%ShaderConductor_Dir%" || goto error
	curl -LO %ShaderConductor_Url% || goto error
	C:/Windows/System32/tar.exe -zxf ShaderConductor-Win.zip || goto error
	del ShaderConductor-Win.zip
	popd
echo.
exit /b 0

:error
exit /b 1

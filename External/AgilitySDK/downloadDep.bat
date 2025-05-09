@echo off
setlocal
set CURRENTDIR=%~dp0

echo Downloading AgilitySDK...
	set AgilitySDK_Dir=%CURRENTDIR%/Lib
	set AgilitySDK_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/AgilitySDK.zip"
	pushd "%AgilitySDK_Dir%" || goto error
	curl -LO %AgilitySDK_Url% || goto error
	C:/Windows/System32/tar.exe -zxf AgilitySDK.zip || goto error
	del AgilitySDK.zip
	popd
echo.
exit /b 0
 
:error
exit /b 1

@echo off
set CURRENTDIR=%~dp0

echo Downloading AgilitySDK...
	set AgilitySDK_Dir=%CURRENTDIR%/Lib
	set AgilitySDK_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/a17e85d32bedc04b54791a65cce43b15/AgilitySDK.zip"
	cd /D %AgilitySDK_Dir% || goto error
	curl -LO %AgilitySDK_Url% || goto error
	C:/Windows/System32/tar.exe -zxf AgilitySDK.zip || goto error
	del AgilitySDK.zip
echo.
exit /b 0

:error
exit /b 1

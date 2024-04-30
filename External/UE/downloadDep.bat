@echo off
set CURRENTDIR=%~dp0

echo Downloading UE...
	set UE-lib_Dir=%CURRENTDIR%/Lib
	set UE-lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/4e2354808ceedab7178517b424dcd61f/UE-Lib-Win.zip"
	cd /D %UE-lib_Dir% || goto error
	curl -LO %UE-lib_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Lib-Win.zip || goto error
	del UE-Lib-Win.zip
	
	set UE-Src_Dir=%CURRENTDIR%/Src
	set UE-Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/1607e8c444f77ead08ee852853c530fe/UE-Src.zip"
	cd /D %UE-Src_Dir% || goto error
	curl -LO %UE-Src_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Src.zip || goto error
	del UE-Src.zip
echo.
exit /b 0

:error
exit /b 1

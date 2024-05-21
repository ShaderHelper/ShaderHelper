@echo off
setlocal
set CURRENTDIR=%~dp0

echo Downloading UE...
	set UE-lib_Dir=%CURRENTDIR%/Lib
	set UE-lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/914856048d1b2fcdee05c18953c0477f/UE-Lib-Win.zip"
	pushd "%UE-lib_Dir%" || goto error
	curl -LO %UE-lib_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Lib-Win.zip || goto error
	del UE-Lib-Win.zip
	popd
	
	set UE-Src_Dir=%CURRENTDIR%/Src
	set UE-Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/739e5f8ff7a70fa9d523fe7ca5c0b0ab/UE-Src.zip"
	pushd "%UE-Src_Dir%" || goto error
	curl -LO %UE-Src_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Src.zip || goto error
	del UE-Src.zip
	popd
echo.
exit /b 0

:error
exit /b 1

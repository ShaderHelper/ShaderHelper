@echo off
setlocal
set CURRENTDIR=%~dp0

echo Downloading UE...
	set UE-lib_Dir=%CURRENTDIR%/Lib
	set UE-lib_Url="https://gitlab.com/-/project/55718299/uploads/b95841417a47555e55bf09f754167e4f/UE-Lib-Win.zip"
	pushd "%UE-lib_Dir%" || goto error
	curl -LO %UE-lib_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Lib-Win.zip || goto error
	del UE-Lib-Win.zip
	popd
	
	set UE-Src_Dir=%CURRENTDIR%/Src
	set UE-Src_Url="https://gitlab.com/-/project/55718299/uploads/144c0bc8e5d41f321f9fcf0d1b75ac68/UE-Src.tar.gz"
	pushd "%UE-Src_Dir%" || goto error
	curl -LO %UE-Src_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Src.tar.gz || goto error
	del UE-Src.tar.gz
	popd
echo.
exit /b 0

:error
exit /b 1

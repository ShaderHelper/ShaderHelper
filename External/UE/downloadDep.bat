@echo off
setlocal
set CURRENTDIR=%~dp0

echo Downloading UE...
	set UE-lib_Dir=%CURRENTDIR%/Lib
	set UE-lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/65921c99a2f27dfa82c4b263feb8d194/UE-Lib-Win.zip"
	pushd "%UE-lib_Dir%" || goto error
	curl -LO %UE-lib_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Lib-Win.zip || goto error
	del UE-Lib-Win.zip
	popd
	
	set UE-Src_Dir=%CURRENTDIR%/Src
	set UE-Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c54de905c2350d3e380d3464a4a885df/UE-Src.tar"
	pushd "%UE-Src_Dir%" || goto error
	curl -LO %UE-Src_Url% || goto error
	C:/Windows/System32/tar.exe -zxf UE-Src.tar || goto error
	del UE-Src.tar
	popd
echo.
exit /b 0

:error
exit /b 1

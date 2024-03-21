@echo off
set CURRENTDIR=%cd%

echo Donwloading AgilitySDK...
	set AgilitySDK_Dir=%CURRENTDIR%/External/AgilitySDK/Lib
	set AgilitySDK_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/7019fbcba88d580ff459b4665092211c/AgilitySDK.zip"
	cd /D %AgilitySDK_Dir%
	curl -LO %AgilitySDK_Url% || goto error
	tar -zxf AgilitySDK.zip || goto error
	del AgilitySDK.zip
echo.

echo Donwloading DXC...
	set DXC_Dir=%CURRENTDIR%/External/DXC/Lib
	set DXC_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/1d895d024c8df6731cdba946b387bff5/DXC.zip"
	cd /D %DXC_Dir%
	curl -LO %DXC_Url% || goto error
	tar -zxf DXC.zip || goto error
	del DXC.zip
echo.

echo Donwloading UE...
	set UE-lib_Dir=%CURRENTDIR%/External/UE/Lib
	set UE-lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/a073247dcbe52e672722609b215df293/UE-Lib-Win.zip"
	cd /D %UE-lib_Dir%
	curl -LO %UE-lib_Url% || goto error
	tar -zxf UE-Lib-Win.zip || goto error
	del UE-Lib-Win.zip
	
	set UE-Src_Dir=%CURRENTDIR%/External/UE/Src
	set UE-Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/5b7a7f9b02abe11002315e44c9adc7ce/UE-Src.zip"
	cd /D %UE-Src_Dir%
	curl -LO %UE-Src_Url% || goto error
	tar -zxf UE-Src.zip || goto error
	del UE-Src.zip
echo.

echo Script complete
pause
exit /b 0

:error
echo.
echo Errors encountered during execution.
pause
exit /b 1

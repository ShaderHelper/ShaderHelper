@echo off
set CURRENTDIR=%cd%

echo Donwloading AgilitySDK...
	set AgilitySDK_Dir=%CURRENTDIR%/External/AgilitySDK/Lib
	set AgilitySDK_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/a17e85d32bedc04b54791a65cce43b15/AgilitySDK.zip"
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
	set UE-lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c0407aa653ea3f3625690b32b4635b36/UE-Lib-Win.zip"
	cd /D %UE-lib_Dir%
	curl -LO %UE-lib_Url% || goto error
	tar -zxf UE-Lib-Win.zip || goto error
	del UE-Lib-Win.zip
	
	set UE-Src_Dir=%CURRENTDIR%/External/UE/Src
	set UE-Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/073096f75c69486222f4295105e8729e/UE-Src.zip"
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

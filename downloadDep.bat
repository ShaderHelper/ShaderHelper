@echo off
echo #!/bin/sh > .git/hooks/post-merge
echo ./External/checkDependency.sh >> .git/hooks/post-merge

call "%~dp0External/AgilitySDK/downloadDep.bat" || goto error
call "%~dp0External/DXC/downloadDep.bat" || goto error
call "%~dp0External/UE/downloadDep.bat" || goto error

echo Script complete
pause
exit /b 0

:error
echo.
echo Errors encountered during execution.
pause
exit /b 1

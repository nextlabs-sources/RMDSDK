@ECHO OFF

SETLOCAL

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RMCAPI.sln /Clean "Release|x86"
IF ERRORLEVEL 1 GOTO END

cd scripts

:END
ENDLOCAL

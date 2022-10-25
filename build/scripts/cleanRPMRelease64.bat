@ECHO OFF

SETLOCAL

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RPM.sln /Clean "Release|x64"
IF ERRORLEVEL 1 GOTO END

cd scripts

:END
ENDLOCAL

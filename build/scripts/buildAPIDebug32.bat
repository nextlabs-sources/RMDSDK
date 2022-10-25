@ECHO OFF

SETLOCAL

CALL importExternalArtifacts.bat x86 debug
CALL importCoreArtifacts.bat Win32_Debug

CALL setVersion.bat

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RMCAPI.sln /Build "Debug|x86"
IF ERRORLEVEL 1 GOTO END

cd scripts

:END
ENDLOCAL

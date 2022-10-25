@ECHO OFF

SETLOCAL

CALL importExternalArtifacts.bat x64 debug
CALL importCoreArtifacts.bat x64_Debug

CALL setVersion.bat

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RMCAPI.sln /Build "Debug|x64"
IF ERRORLEVEL 1 GOTO END

cd scripts

:END
ENDLOCAL

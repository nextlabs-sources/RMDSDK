@ECHO OFF

SETLOCAL

CALL importExternalArtifacts.bat x64 release
CALL importCoreArtifacts.bat x64_Release

CALL setVersion.bat

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RMCAPI.sln /Build "Release|x64"
IF ERRORLEVEL 1 GOTO END

cd scripts

:END
ENDLOCAL

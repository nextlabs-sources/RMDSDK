@ECHO OFF

SETLOCAL

CALL importExternalArtifacts.bat x86 release
CALL importCoreArtifacts.bat Win32_Release

CALL setVersion.bat

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RMCAPI.sln /Build "Release|x86"
IF ERRORLEVEL 1 GOTO END

cd scripts

:END
ENDLOCAL

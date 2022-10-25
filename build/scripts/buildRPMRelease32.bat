@ECHO OFF

SETLOCAL

CALL importExternalArtifacts.bat x86 release

CALL setVersion.bat

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
devenv.com RPM.sln /Build "Release|x86"
IF ERRORLEVEL 1 GOTO END

cd bin\Win32\Release
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\..\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO END
)
cd ..\..\..

cd bin_SancDir\Win32\Release
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\..\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO END
)
cd ..\..\..

cd scripts

:END
ENDLOCAL

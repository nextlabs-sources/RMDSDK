@ECHO OFF

SETLOCAL

cd ..

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat"

REM
REM For C# modules, we cannot control the directories for the intermediate
REM files during compilation using .csproj files.  (For C++ modules, we
REM control this using <IntDir> in .vcxproj files.)  So we cannot assign
REM different intermediate directories for different versions of the same
REM module even though we have version-specific .csproj files for the
REM module.
REM
REM Therefore we need to build different versions of C# modules using
REM separate .sln files in sequence.  We also need to use /Rebuild instead
REM of /Build.  This way:
REM 1. Different versions of the same module are compiled in sequence
REM    instead of in parallel.
REM 2. The intermediate files for each version are re-built from scratch.
REM

devenv.com RPM_CS.sln /Rebuild "Debug|Any CPU"
IF ERRORLEVEL 1 GOTO END

devenv.com RPM_CS_SancDir.sln /Rebuild "Debug|Any CPU"
IF ERRORLEVEL 1 GOTO END

cd bin\Debug
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO END
)
cd ..\..

cd bin_SancDir\Debug
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO END
)
cd ..\..

cd scripts

:END
ENDLOCAL

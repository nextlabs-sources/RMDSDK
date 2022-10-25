@ECHO OFF

SETLOCAL

IF "%BUILD_NUMBER%"=="" (
  echo Error: BUILD_NUMBER is not set!
  GOTO :EOF
)

REM
REM Check line-end in .INF files.  Make sure that they have Windows line-end.
REM Otherwise, Visual Studio will fail to add timestamps to the .inf files
REM without any error messages.
REM 

REM
REM Check line-end in .BAT files.  Make sure that they have Windows line-end.
REM Otherwise, CALL commands which call labels will not be able to find the
REM labels.
REM
echo Checking %0 ...
..\..\sources\tools\lineendchecker\output\x86_release\lineendchecker.exe /win %0
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Configure various build variables.
REM
CALL configure.bat



REM
REM Release build
REM
CALL buildAPIRelease32.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildAPIRelease64.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildRPMRelease32.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildRPMRelease64.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildRPM_CS_Release.bat
IF ERRORLEVEL 1 GOTO :EOF

REM
REM Release RPM installer
REM
CALL buildRPMInstallersByConfig.bat Release
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Debug build
REM
CALL buildAPIDebug32.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildAPIDebug64.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildRPMDebug32.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildRPMDebug64.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL buildRPM_CS_Debug.bat
IF ERRORLEVEL 1 GOTO :EOF

REM
REM Debug RPM installer
REM
CALL buildRPMInstallersByConfig.bat Debug
IF ERRORLEVEL 1 GOTO :EOF



REM
REM SDK Installer
REM
CALL buildInstaller.bat
IF ERRORLEVEL 1 GOTO :EOF
CALL publishInstaller.bat
IF ERRORLEVEL 1 GOTO :EOF



SET RES_PUBLISH_TO_BODA=false
if "%PUBLISH_TO_BODA%" == "Yes" set RES_PUBLISH_TO_BODA=true
if "%PUBLISH_TO_BODA%" == "yes" set RES_PUBLISH_TO_BODA=true
if "%PUBLISH_TO_BODA%" == "YES" set RES_PUBLISH_TO_BODA=true
if "%PUBLISH_TO_BODA%" == "1" set RES_PUBLISH_TO_BODA=true
if "%RES_PUBLISH_TO_BODA%"=="true" (
REM
REM Artifacts
REM
CALL publishArtifacts.bat
)

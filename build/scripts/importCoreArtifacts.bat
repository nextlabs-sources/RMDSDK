@ECHO OFF

SETLOCAL

IF "%1"=="" GOTO USAGE

set CONFIG=%1

set CODEBASE=Doom
set CODEBASE_LOWERCASE=doom
set TARGET_OS=Windows

CALL setVersion.bat

cd ..\..

set S_DRIVE=\\nextlabs.com\share\data

IF "%CONFIG_TYPE%"=="feature_smdc" (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\Core\%VERSION_MAJMIN%.%BRANCH_ID%
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\Core\%VERSION_MAJMIN%.%BRANCH_ID%
) ELSE (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\Core\%VERSION_MAJMIN%
)

IF "%CONFIG_TYPE%"=="feature_smdc" (
  set TYPE=feature
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set TYPE=feature
) ELSE IF "%CONFIG_TYPE%"=="develop" (
  set TYPE=develop
) ELSE (
  set TYPE=release
)

set ARTIFACT_FILE_PREFIX=%VERSION_DIR%\%TARGET_OS%\last_stable\%CODEBASE_LOWERCASE%-Core-%VERSION_MAJMIN%

FOR %%f IN (%ARTIFACT_FILE_PREFIX%.*-%TYPE%-bin.zip) DO (
  set FIRST_F=%%f
  GOTO AFTER_FIRST_F
)
:AFTER_FIRST_F

unzip.exe -o %FIRST_F% sources/include*.*

md sources\include\common\brain
pushd sources\include\common\brain
unzip.exe -o -j %FIRST_F% sources/common/brain/*
popd

md libs\RMCCore\windows\%CONFIG%
pushd libs\RMCCore\windows\%CONFIG%
unzip.exe -o -j %FIRST_F% build/build.msvc/%CONFIG%/*
popd

cd build\scripts

GOTO :EOF



:USAGE
echo Usage: %0 config
echo config can be "Win32_Debug", "Win32_Release", "x64_Debug", or "x64_Release"

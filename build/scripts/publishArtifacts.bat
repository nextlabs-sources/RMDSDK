@ECHO OFF

SETLOCAL

set CODEBASE=Doom
set CODEBASE_LOWERCASE=doom
set TARGET_OS=Windows

CALL setVersion.bat

IF "%BUILD_NUMBER%"=="" (
  echo Error: BUILD_NUMBER is not set!
  GOTO :EOF
)

set S_DRIVE=\\nextlabs.com\share\data
set S_DRIVE_POSIX=//nextlabs.com/share/data

IF "%CONFIG_TYPE%"=="feature_smdc" (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\SDK\%VERSION_MAJMIN%.%BRANCH_ID%
  set VERSION_DIR_POSIX=%S_DRIVE_POSIX%/%OUTPUT_REPOSITORY_ROOT_POSIX%/artifacts/%CODEBASE%/SDK/%VERSION_MAJMIN%.%BRANCH_ID%
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\SDK\%VERSION_MAJMIN%.%BRANCH_ID%
  set VERSION_DIR_POSIX=%S_DRIVE_POSIX%/%OUTPUT_REPOSITORY_ROOT_POSIX%/artifacts/%CODEBASE%/SDK/%VERSION_MAJMIN%.%BRANCH_ID%
) ELSE (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\SDK\%VERSION_MAJMIN%
  set VERSION_DIR_POSIX=%S_DRIVE_POSIX%/%OUTPUT_REPOSITORY_ROOT_POSIX%/artifacts/%CODEBASE%/SDK/%VERSION_MAJMIN%
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

set ARTIFACT_FILE_PREFIX_POSIX=%VERSION_DIR_POSIX%/%TARGET_OS%/%VERSION_BUILD_SHORT%/%CODEBASE_LOWERCASE%-SDK-%VERSION_MAJMIN%.%VERSION_BUILD_SHORT%-%TYPE%

IF NOT EXIST %VERSION_DIR% md %VERSION_DIR%
IF ERRORLEVEL 1 GOTO END
IF NOT EXIST %VERSION_DIR%\%TARGET_OS% md %VERSION_DIR%\%TARGET_OS%
IF ERRORLEVEL 1 GOTO END
IF NOT EXIST %VERSION_DIR%\%TARGET_OS%\%VERSION_BUILD_SHORT% md %VERSION_DIR%\%TARGET_OS%\%VERSION_BUILD_SHORT%
IF ERRORLEVEL 1 GOTO END

xcopy /s /k /i /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers\*.sys ..\bin_HLKSignedDrivers
xcopy /s /k /i /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers\*.pdb ..\bin_HLKSignedDrivers
xcopy /s /k /i /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers_SancDir\*.sys ..\bin_SancDir_HLKSignedDrivers
xcopy /s /k /i /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers_SancDir\*.pdb ..\bin_SancDir_HLKSignedDrivers

cd ..\..

zip -D -r -9 %ARTIFACT_FILE_PREFIX_POSIX%-bin.zip sources/include/SDWL build/build.msvc*/*/SDWRmcLib*.{lib,dll,exe,pdb} build/build.msvc*/*/nxrmtool.{exe,pdb} build/bin*/*/*/*.{sys,inf,cat,dll,exe,pdb} build/bin/*/*.{dll,exe,tlb,pdb}
IF ERRORLEVEL 1 GOTO END
echo INFO: Created %ARTIFACT_FILE_PREFIX_POSIX%-bin.zip

zip -D -r -9 %ARTIFACT_FILE_PREFIX_POSIX%-install.zip install -x "install/build/output/Product Configuration 1/Media_MSI/DiskImages/DISK1/SkyDRM Client SDK.msi"
IF ERRORLEVEL 1 GOTO END
echo INFO: Created %ARTIFACT_FILE_PREFIX_POSIX%-install.zip

cd build\scripts

:END
ENDLOCAL

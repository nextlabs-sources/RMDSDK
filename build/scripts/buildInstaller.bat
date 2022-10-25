@ECHO ON

SETLOCAL

set CODEBASE=Doom
set TARGET_OS=Windows

set S_DRIVE=\\nextlabs.com\share\data

CALL setVersion.bat

IF "%BUILD_NUMBER%"=="" (
  set buildNumber=0
) ELSE (
  set buildNumber=%BUILD_NUMBER%
)

IF "%CONFIG_TYPE%"=="feature_smdc" (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\SDK\%VERSION_MAJMIN%.%BRANCH_ID%
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\SDK\%VERSION_MAJMIN%.%BRANCH_ID%
) ELSE (
  set VERSION_DIR=%S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\SDK\%VERSION_MAJMIN%
)

set version="%VERSION_MAJMIN%.%buildNumber%"

CALL c:\windows\syswow64\cscript ISAutoGUIDVersion.js ..\..\install\RMDSDK.ism %version%

xcopy /s /k /i /y ..\..\sources\include\SDWL ..\..\install\build\data\include\SDWL
IF ERRORLEVEL 1 GOTO :EOF
xcopy /s /k /i /y %S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\external\%TARGET_OS%\boost\lib ..\..\install\build\data\bin\boost
IF ERRORLEVEL 1 GOTO :EOF
xcopy /s /k /i /y ..\..\libs\RMCCore\%TARGET_OS%\libeay32.lib* ..\..\install\build\data\bin\SDWL
IF ERRORLEVEL 1 GOTO :EOF
xcopy /s /k /i /y ..\build.msvc\SDWRmcLib.lib* ..\..\install\build\data\bin\SDWL
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /y ..\bin\Release\*.tlb ..\..\install\build\data\include
IF ERRORLEVEL 1 GOTO :EOF
xcopy /s /k /i /y %VERSION_DIR%\%TARGET_OS%_src\redist ..\..\install\build\data\redist
IF ERRORLEVEL 1 GOTO :EOF
xcopy /s /k /i /y %VERSION_DIR%\%TARGET_OS%_src\doc ..\..\install\build\data\doc
IF ERRORLEVEL 1 GOTO :EOF

xcopy /s /y ..\..\install\themes "C:\Program Files (x86)\InstallShield\2014 SAB\Support\Themes"
IF ERRORLEVEL 1 GOTO :EOF

cd ..\..\install

"C:\Program Files (x86)\InstallShield\2014 SAB\System\IsCmdBld.exe" -x -p RMDSDK.ism
IF ERRORLEVEL 1 GOTO :EOF

cd build\output\Product Configuration 1\Media_MSI\DiskImages\DISK1
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\..\..\..\..\..\build\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO :EOF
)
cd ..\..\..\..\..\..

cd ..\build\scripts

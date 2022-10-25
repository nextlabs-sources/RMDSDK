@ECHO OFF

SETLOCAL

cd ..\..\install

CALL :RUN_IS_BY_EDITION
IF ERRORLEVEL 1 GOTO :EOF
CALL :RUN_IS_BY_EDITION SancDir
IF ERRORLEVEL 1 GOTO :EOF

cd ..\build\scripts

GOTO :EOF



REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
REM % RUN_IS_BY_EDITION
REM %
REM % Invoke InstallShield for one product edition
REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:RUN_IS_BY_EDITION

SETLOCAL

IF "%1"=="" (
  set EDITION=""
) ELSE (
  set EDITION="_%1"
)

"C:\Program Files (x86)\InstallShield\2014 SAB\System\IsCmdBld.exe" -x -p RPM_x86_Release.ism -r Media_EXE -a Release_32bit%EDITION%
IF ERRORLEVEL 1 GOTO :EOF

cd build_RPM\output\Release_32bit%EDITION%\Media_EXE\DiskImages\DISK1
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\..\..\..\..\..\build\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO :EOF
)
cd ..\..\..\..\..\..

"C:\Program Files (x86)\InstallShield\2014 SAB\System\IsCmdBld.exe" -x -p RPM_x86_Release.ism -r Media_MSI -a Release_32bit%EDITION%
IF ERRORLEVEL 1 GOTO :EOF

cd build_RPM\output\Release_32bit%EDITION%\Media_MSI\DiskImages\DISK1
IF NOT "%SKIP_SIGNING%"=="Yes" (
  CALL ..\..\..\..\..\..\..\build\scripts\signModulesByServer256Only.bat
  IF ERRORLEVEL 1 GOTO :EOF
)
cd ..\..\..\..\..\..

ENDLOCAL

GOTO :EOF

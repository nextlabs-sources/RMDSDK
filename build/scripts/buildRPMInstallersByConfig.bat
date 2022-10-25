@ECHO ON

SETLOCAL

IF "%1"=="" GOTO USAGE

IF "%1"=="Debug" (
  set CONFIG=Debug
) ELSE IF "%1"=="Release" (
  set CONFIG=Release
) ELSE (
  GOTO USAGE
)

set CODEBASE=Doom
set TARGET_OS=Windows

set WINKIT10_PATH="C:\Program Files (x86)\Windows Kits\10"
set VC2015_REDIST_PATH="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist"
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



REM
REM Generate ProductCode and set ProductVersion in .ism files
REM
CALL c:\windows\syswow64\cscript ISAutoGUIDVersion.js ..\..\install\RPM_x86_%CONFIG%.ism %version%
CALL c:\windows\syswow64\cscript ISAutoGUIDVersion.js ..\..\install\RPM_x64_%CONFIG%.ism %version%



REM
REM Copy C++ modules
REM
CALL :COPY_CPP_MODULES_BY_EDITION
IF ERRORLEVEL 1 GOTO :EOF
CALL :COPY_CPP_MODULES_BY_EDITION SancDir
IF ERRORLEVEL 1 GOTO :EOF

xcopy /s /k /i /y ..\..\sources\SDWL\RPM\app\nxrmserv\src\res\Nxl-folder.ico ..\..\install\build_RPM\data
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Copy C# modules
REM
xcopy /i /y %NLGITEXTERNALDIR%\system.data.sqlite.core\1.0.109.2\build\net45\x64\SQLite.Interop.dll ..\..\install\build_RPM\data\x64
IF ERRORLEVEL 1 GOTO :EOF
xcopy /i /y %NLGITEXTERNALDIR%\system.data.sqlite.core\1.0.109.2\build\net45\x86\SQLite.Interop.dll ..\..\install\build_RPM\data\Win32
IF ERRORLEVEL 1 GOTO :EOF

CALL :COPY_CS_MODULES_BY_EDITION
IF ERRORLEVEL 1 GOTO :EOF
CALL :COPY_CS_MODULES_BY_EDITION SancDir
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Copy latest Microsoft HLK-signed drivers, if any.  Ignore error.
REM
CALL :COPY_HLK_SIGNED_DRIVERS_BY_EDITION
CALL :COPY_HLK_SIGNED_DRIVERS_BY_EDITION SancDir



REM
REM Copy PC files
REM
unzip -o -d ..\..\install\build_RPM\data\Win32\PC %VERSION_DIR%\%TARGET_OS%_src\PC\PC_x86.zip
IF ERRORLEVEL 1 GOTO :EOF
unzip -o -d ..\..\install\build_RPM\data\x64\PC %VERSION_DIR%\%TARGET_OS%_src\PC\PC_x64.zip
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Copy external library files
REM
copy %S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\external\%TARGET_OS%\openssl\x86\lib\%CONFIG%\libeay32.dll ..\..\install\build_RPM\data\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
copy %S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\external\%TARGET_OS%\openssl\x64\lib\%CONFIG%\libeay32.dll ..\..\install\build_RPM\data\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF



REM
REM For Win32 libeay32.dll, use a non-FIPS one from standard OpenSSL instead of the FIPS one we built in-house
REM
copy %NLGITEXTERNALDIR%\OpenSSL\openssl-1.0.2o-x32-VC2017\openssl-1.0.2o-x32-VC2017\libeay32.dll ..\..\install\build_RPM\data\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Copy Chromium Embedded Framework and CefSharp files
REM
md ..\..\install\build_RPM\data\Win32\CEF\locales
md ..\..\install\build_RPM\data\x64\CEF\locales

FOR %%f IN (
  chrome_100_percent.pak
  chrome_200_percent.pak
  chrome_elf.dll
  d3dcompiler_47.dll
  icudtl.dat
  libcef.dll
  libEGL.dll
  libGLESv2.dll
  LICENSE.txt
  resources.pak
  locales\en-US.pak
  snapshot_blob.bin
  v8_context_snapshot.bin
  vk_swiftshader.dll
  vk_swiftshader_icd.json
  vulkan-1.dll
) DO (
  copy %NLGITEXTERNALDIR%\ChromiumEmbeddedFramework\cef.redist.x86.96.0.18\CEF\%%f ..\..\install\build_RPM\data\Win32\CEF\%%f
  IF ERRORLEVEL 1 GOTO :EOF
  copy %NLGITEXTERNALDIR%\ChromiumEmbeddedFramework\cef.redist.x64.96.0.18\CEF\%%f ..\..\install\build_RPM\data\x64\CEF\%%f
  IF ERRORLEVEL 1 GOTO :EOF
)

FOR %%f IN (
  CefSharp.BrowserSubprocess.Core.dll
  CefSharp.BrowserSubprocess.exe
  CefSharp.Core.Runtime.dll
  CefSharp.Core.Runtime.xml
) DO (
  copy %NLGITEXTERNALDIR%\CefSharp\cefsharp.common.96.0.180\CefSharp\x86\%%f ..\..\install\build_RPM\data\Win32\CEF
  IF ERRORLEVEL 1 GOTO :EOF
  copy %NLGITEXTERNALDIR%\CefSharp\cefsharp.common.96.0.180\CefSharp\x64\%%f ..\..\install\build_RPM\data\x64\CEF
  IF ERRORLEVEL 1 GOTO :EOF
)

FOR %%f IN (
  CefSharp.Core.dll
  CefSharp.dll
) DO (
  copy %NLGITEXTERNALDIR%\CefSharp\cefsharp.common.96.0.180\lib\net452\%%f ..\..\install\build_RPM\data\Win32\CEF
  IF ERRORLEVEL 1 GOTO :EOF
  copy %NLGITEXTERNALDIR%\CefSharp\cefsharp.common.96.0.180\lib\net452\%%f ..\..\install\build_RPM\data\x64\CEF
  IF ERRORLEVEL 1 GOTO :EOF
)



REM
REM Copy Windows Kits 10 Redistributable files
REM
FOR %%f IN (
  api-ms-win-core-file-l1-2-0.dll
  api-ms-win-core-file-l2-1-0.dll
  api-ms-win-core-localization-l1-2-0.dll
  api-ms-win-core-processthreads-l1-1-1.dll
  api-ms-win-core-synch-l1-2-0.dll
  api-ms-win-core-timezone-l1-1-0.dll
  api-ms-win-crt-convert-l1-1-0.dll
  api-ms-win-crt-environment-l1-1-0.dll
  api-ms-win-crt-filesystem-l1-1-0.dll
  api-ms-win-crt-heap-l1-1-0.dll
  api-ms-win-crt-locale-l1-1-0.dll
  api-ms-win-crt-math-l1-1-0.dll
  api-ms-win-crt-multibyte-l1-1-0.dll
  api-ms-win-crt-runtime-l1-1-0.dll
  api-ms-win-crt-stdio-l1-1-0.dll
  api-ms-win-crt-string-l1-1-0.dll
  api-ms-win-crt-time-l1-1-0.dll
  api-ms-win-crt-utility-l1-1-0.dll
) DO (
  copy %WINKIT10_PATH%\Redist\ucrt\DLLs\x86\%%f ..\..\install\build_RPM\data\Win32
  IF ERRORLEVEL 1 GOTO :EOF
  copy %WINKIT10_PATH%\Redist\ucrt\DLLs\x64\%%f ..\..\install\build_RPM\data\x64
  IF ERRORLEVEL 1 GOTO :EOF
)

IF "%CONFIG%"=="Debug" (
  copy %WINKIT10_PATH%\bin\x86\ucrt\ucrtbased.dll ..\..\install\build_RPM\data\Win32\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
  copy %WINKIT10_PATH%\bin\x64\ucrt\ucrtbased.dll ..\..\install\build_RPM\data\x64\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
) ELSE (
  copy %WINKIT10_PATH%\Redist\ucrt\DLLs\x86\ucrtbase.dll ..\..\install\build_RPM\data\Win32\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
  copy %WINKIT10_PATH%\Redist\ucrt\DLLs\x64\ucrtbase.dll ..\..\install\build_RPM\data\x64\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
)



REM
REM Copy Visual C++ 2015 Redistributable files
REM
REM Copy the versions of redistributables as follow for the reasons below:
REM - Debug 32-bit:
REM   - msvcp140d.dll:          14.0.24210      (I cannot find 14.30.30704 version.)
REM   - msvcp140.dll:           14.30.30704     (CEF modules need Release version.)
REM   - vcruntime140d.dll:      14.0.24210      (I cannot find 14.30.30704 version.)
REM   - vcruntime140.dll:       14.30.30704     (CEF modules need Release version.)
REM   - vcruntime140_1.dll:     N/A             (There is no 32-bit version.)
REM - Debug 64-bit:
REM   - msvcp140d.dll:          14.0.24210      (I cannot find 14.30.30704 version.)
REM   - msvcp140.dll:           14.30.30704     (CEF modules need Release version.)
REM   - vcruntime140d.dll:      14.0.24210      (I cannot find 14.30.30704 version.)
REM   - vcruntime140.dll:       14.30.30704     (CEF modules need Release version.)
REM   - vcruntime140_1.dll:     14.30.30704     (CEF modules need Release version.)
REM - Release 32-bit:
REM   - msvcp140.dll:           14.30.30704
REM   - vcruntime140.dll:       14.30.30704
REM   - vcruntime140_1.dll:     N/A             (There is no 32-bit version.)
REM - Release 64-bit:
REM   - msvcp140.dll:           14.30.30704
REM   - vcruntime140.dll:       14.30.30704
REM   - vcruntime140_1.dll:     14.30.30704
REM
FOR %%f IN (msvcp140 vcruntime140) DO (
  IF "%CONFIG%"=="Debug" (
    copy %VC2015_REDIST_PATH%\debug_nonredist\x86\Microsoft.VC140.DebugCRT\%%fd.dll ..\..\install\build_RPM\data\Win32\%CONFIG%
    IF ERRORLEVEL 1 GOTO :EOF
    copy %VC2015_REDIST_PATH%\debug_nonredist\x64\Microsoft.VC140.DebugCRT\%%fd.dll ..\..\install\build_RPM\data\x64\%CONFIG%
    IF ERRORLEVEL 1 GOTO :EOF
  )

  copy %NLGITEXTERNALDIR%\VisualStudio\VC_redist\14.30.30704\x86\%%f.dll ..\..\install\build_RPM\data\Win32\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
  copy %NLGITEXTERNALDIR%\VisualStudio\VC_redist\14.30.30704\x64\%%f.dll ..\..\install\build_RPM\data\x64\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
)

FOR %%f IN (vcruntime140_1) DO (
  copy %NLGITEXTERNALDIR%\VisualStudio\VC_redist\14.30.30704\x64\%%f.dll ..\..\install\build_RPM\data\x64\%CONFIG%
  IF ERRORLEVEL 1 GOTO :EOF
)



REM
REM Copy SkyDRM theme files for InstallShield
REM
xcopy /s /y ..\..\install\themes "C:\Program Files (x86)\InstallShield\2014 SAB\Support\Themes"
IF ERRORLEVEL 1 GOTO :EOF



REM
REM Invoke InstallShield
REM
CALL buildRPM%CONFIG%32Installer.bat
IF ERRORLEVEL 1 GOTO :EOF

CALL buildRPM%CONFIG%64Installer.bat
IF ERRORLEVEL 1 GOTO :EOF

GOTO :EOF



:USAGE
echo Usage: %0 config
echo config can be either "Debug" or "Release"
GOTO :EOF



REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
REM % COPY_CPP_MODULES_BY_EDITION
REM %
REM % Copy C++ modules for one product edition
REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:COPY_CPP_MODULES_BY_EDITION

SETLOCAL

IF "%1"=="" (
  set EDITION=""
) ELSE (
  set EDITION="_%1"
)

xcopy /k /i /y ..\bin%EDITION%\Win32\%CONFIG%\*.sys ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\Win32\%CONFIG%\*.inf ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\Win32\%CONFIG%\*.cat ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\Win32\%CONFIG%\*.dll ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\Win32\%CONFIG%\*.exe ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\Win32\%CONFIG%\*.res ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\build.msvc%EDITION%\Win32_%CONFIG%\nxrmtool.exe ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\x64\%CONFIG%\*.sys ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\x64\%CONFIG%\*.inf ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\x64\%CONFIG%\*.cat ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\x64\%CONFIG%\*.dll ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\x64\%CONFIG%\*.exe ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\x64\%CONFIG%\*.res ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\build.msvc%EDITION%\x64_%CONFIG%\nxrmtool.exe ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF

ENDLOCAL

GOTO :EOF



REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
REM % COPY_CS_MODULES_BY_EDITION
REM %
REM % Copy C# modules for one product edition
REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:COPY_CS_MODULES_BY_EDITION

SETLOCAL

IF "%1"=="" (
  set EDITION=""
  set BIN_EDITION=""
) ELSE (
  set EDITION="_%1"
  set BIN_EDITION="bin_%1\"
)

xcopy /k /i /y ..\..\sources\SDWL\RPM\app\nxrmtray\%BIN_EDITION%%CONFIG%\*.dll ..\..\install\build_RPM\data%EDITION%\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\..\sources\SDWL\RPM\app\nxrmtray\%BIN_EDITION%%CONFIG%\*.xml ..\..\install\build_RPM\data%EDITION%\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\..\sources\SDWL\RPM\app\nxrmtray\%BIN_EDITION%%CONFIG%\*.config ..\..\install\build_RPM\data%EDITION%\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\%CONFIG%\*.dll ..\..\install\build_RPM\data%EDITION%\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF
xcopy /k /i /y ..\bin%EDITION%\%CONFIG%\*.exe ..\..\install\build_RPM\data%EDITION%\%CONFIG%
IF ERRORLEVEL 1 GOTO :EOF

ENDLOCAL

GOTO :EOF



REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
REM % COPY_HLK_SIGNED_DRIVERS_BY_EDITION
REM %
REM % Copy Microsoft HLK-signed drivers for one product edition
REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:COPY_HLK_SIGNED_DRIVERS_BY_EDITION

SETLOCAL

IF "%1"=="" (
  set EDITION=""
) ELSE (
  set EDITION="_%1"
)

xcopy /k /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers%EDITION%\Win32\%CONFIG%\*.sys ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
xcopy /k /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers%EDITION%\Win32\%CONFIG%\*.inf ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
xcopy /k /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers%EDITION%\Win32\%CONFIG%\*.cat ..\..\install\build_RPM\data%EDITION%\Win32\%CONFIG%
xcopy /k /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers%EDITION%\x64\%CONFIG%\*.sys ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
xcopy /k /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers%EDITION%\x64\%CONFIG%\*.inf ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%
xcopy /k /y /c %VERSION_DIR%\%TARGET_OS%_src\HLKSignedDrivers%EDITION%\x64\%CONFIG%\*.cat ..\..\install\build_RPM\data%EDITION%\x64\%CONFIG%

ENDLOCAL

GOTO :EOF

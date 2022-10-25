REM
REM signModules256Only.bat
REM
REM Sign all supported files in the current directory.
REM
REM If there are multiple matching SHA256 certificates on the
REM machine, you must set the environment variable
REM CERT_HASH_SHA256 to the SHA1 thumbprint of the certificate you want
REM to use respectively before running this script.
REM



SETLOCAL ENABLEDELAYEDEXPANSION

SET TIMESTAMP_SERVER=timestamp.digicert.com

IF "%PROCESSOR_ARCHITECTURE%"=="x86" (
  SET SIGNTOOL="C:\Program Files\Windows Kits\8.1\bin\x86\signtool.exe"
) ELSE (
  SET SIGNTOOL="C:\Program Files (x86)\Windows Kits\8.1\bin\x86\signtool.exe"
)
SET SIGNTOOL_ARGS=sign /n "NextLabs Inc."

SET SIGNTOOL_ARGS_SHA256=/i "DigiCert Trusted G4 Code Signing RSA4096 SHA384 2021 CA1" /fd SHA256 /t http://%TIMESTAMP_SERVER%
IF DEFINED CERT_HASH_SHA256 SET SIGNTOOL_ARGS_SHA256=%SIGNTOOL_ARGS_SHA256% /sha1 %CERT_HASH_SHA256%

set FILE_LIST=
FOR %%i IN (*.dll *.exe *.sys *.cat *.cab *.msi) DO set FILE_LIST=!FILE_LIST! "%%i"
IF NOT DEFINED FILE_LIST (
  ECHO No supported files are found!
  EXIT /b 1
)

CALL :SIGN_ONE %SIGNTOOL_ARGS_SHA256%
IF ERRORLEVEL 1 GOTO :EOF

ENDLOCAL
GOTO :EOF



:SIGN_ONE

SET /a SIGNCOUNT=1

:SIGNAGAIN

%SIGNTOOL% %SIGNTOOL_ARGS% %* %FILE_LIST%
IF %ERRORLEVEL% EQU 0 GOTO AFTERSIGN

ECHO ERROR: Signing attempt #%SIGNCOUNT% failed!
ping www.google.com
ping %TIMESTAMP_SERVER%

IF EXIST C:\cygwin\bin\sleep.exe (
  ECHO Sleeping for 10 seconds before retrying ......
  C:\cygwin\bin\sleep.exe 10
) ELSE IF EXIST C:\cygwin64\bin\sleep.exe (
  ECHO Sleeping for 10 seconds before retrying ......
  C:\cygwin64\bin\sleep.exe 10
)

SET /a SIGNCOUNT=%SIGNCOUNT% + 1
IF %SIGNCOUNT% LEQ 10 GOTO SIGNAGAIN

:AFTERSIGN
GOTO :EOF

REM
REM signHlkxs.bat
REM
REM Sign all .hlkx files in the current directory.
REM
REM You must set the environment variable CERT_HASH_SHA256 to the SHA1
REM thumbprint of the certificate you want to use before running this
REM script.
REM



SETLOCAL ENABLEDELAYEDEXPANSION

SET SIGNTOOL_ARGS=%CERT_HASH_SHA256%

set FILE_LIST=
FOR %%i IN (*.hlkx) DO set FILE_LIST=!FILE_LIST! "%%i"
IF NOT DEFINED FILE_LIST (
  ECHO No supported files are found!
  EXIT /b 1
)

CALL :SIGN_ONE
IF ERRORLEVEL 1 GOTO :EOF

ENDLOCAL
GOTO :EOF



:SIGN_ONE

SET /a SIGNCOUNT=1

:SIGNAGAIN

%SIGNTOOL% %SIGNTOOL_ARGS% %FILE_LIST%
IF %ERRORLEVEL% EQU 0 GOTO AFTERSIGN

ECHO ERROR: Signing attempt #%SIGNCOUNT% failed!

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

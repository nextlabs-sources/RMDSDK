@ECHO OFF

SETLOCAL

set SIGNING_SHARE_DIR=C:\signingForHlkx
set SIGNING_REQ_FILENAME=signingReq.txt
set SIGNING_DONE_FILENAME=signingDone.txt

set SIGNFILES_BAT=%CD%\signHlkxs.bat
SET SIGNTOOL=%CD%\SignHLKX.exe



REM
REM Sign a dummy file in the current directory, so that if SafeNet
REM Authentication Client Tools needs to ask the console user to enter
REM the token password, it will ask right now instead of when a client
REM machine submits files for signing later.
REM
CALL "%SIGNFILES_BAT%"
IF ERRORLEVEL 1 (
    ECHO Signing of dummy file failed!
    GOTO :EOF
)



:AGAIN

FOR /R "%SIGNING_SHARE_DIR%" %%P IN (.) DO (
    IF EXIST "%%P\%SIGNING_REQ_FILENAME%" (
        ECHO Signing files in "%%P"
        del "%%P\%SIGNING_REQ_FILENAME%"
        pushd "%%P"
        CALL "%SIGNFILES_BAT%"
        popd
        copy nul "%%P\%SIGNING_DONE_FILENAME%"
    )
)

IF EXIST C:\cygwin\bin\sleep.exe (
    C:\cygwin\bin\sleep.exe 10
) ELSE IF EXIST C:\cygwin64\bin\sleep.exe (
    C:\cygwin64\bin\sleep.exe 10
) ELSE IF EXIST .\Sleep.exe (
    .\Sleep.exe 10000
)

GOTO AGAIN

ENDLOCAL

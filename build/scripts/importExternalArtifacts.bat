@ECHO OFF

SETLOCAL

IF "%2"=="" GOTO USAGE
IF "%NLGITEXTERNALDIR%"=="" GOTO USAGE

set PLATFORM=%1
set CONFIG=%2

set CODEBASE=Doom
set TARGET_OS=Windows

cd ..\..

set S_DRIVE=\\nextlabs.com\share\data

REM
REM Import Boost.
REM
xcopy /s /i /d /y %NLGITEXTERNALDIR%\Boost\boost_1_65_1\boost external\boost\boost
xcopy /s /i /d /y %S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\external\%TARGET_OS%\boost\lib\%PLATFORM%\%CONFIG% external\boost\lib\%PLATFORM%\%CONFIG%

REM
REM Import OpenSSL.
REM
xcopy /s /i /d /y %S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\external\%TARGET_OS%\openssl\%PLATFORM%\inc32 external\openssl\%PLATFORM%\inc32
xcopy /s /i /d /y %S_DRIVE%\%OUTPUT_REPOSITORY_ROOT%\artifacts\%CODEBASE%\external\%TARGET_OS%\openssl\%PLATFORM%\lib\%CONFIG% external\openssl\%PLATFORM%\lib\%CONFIG%

REM
REM Import JSON for Modern C++.
REM
xcopy /s /i /d /y "%NLGITEXTERNALDIR%\JSONforModernC++\3.7.3" "external\jsonformodernc++"

cd build\scripts

GOTO :EOF



:USAGE
echo Usage: %0 platform config
echo platform can be "x86" or "x64".
echo config can be "debug" or "release".
echo The environment variable NLGITEXTERNALDIR must be defined.

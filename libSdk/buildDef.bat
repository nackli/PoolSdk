echo off
setlocal EnableDelayedExpansion
cd %~dp0
if exist %1\objects.txt% (
 del /f/q "%1\objects.txt"
)
for /f "delims=" %%a in ('dir /b /s *.obj') do echo;%%~a>>%1/objects.txt

cmake.exe -E __create_def %~dp0/%1/exports.def %~dp0/%1/objects.txt
if %errorlevel% neq 0 goto :cmEnd
:cmEnd
endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone
:cmErrorLevel
exit /b %1
:cmDone
if %errorlevel% neq 0 goto :VCEnd
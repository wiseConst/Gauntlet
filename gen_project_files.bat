@echo off

call format_files.bat
call vendor\premake\premake5.exe vs2022

IF %ERRORLEVEL% NEQ 0 (
    PAUSE
) 

cd Assets/Shaders
call CompileShaders.bat
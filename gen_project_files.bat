@echo off

call format_files.bat
call vendor\premake\premake5.exe vs2022

IF %ERRORLEVEL% NEQ 0 (
    PAUSE
) 

cd Forge/Resources/

if not exist Cached ( mkdir Cached )
cd Cached
if not exist Shaders (mkdir Shaders)
if not exist Pipelines (mkdir Pipelines)

cd ../Shaders

call CompileShaders.bat
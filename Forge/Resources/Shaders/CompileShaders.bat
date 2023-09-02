::@echo off

set SourceDir=%cd%

cd ../Cached/Shaders
set TargetDir=%cd%
cd ../../Shaders

:: Compile shaders
for /r %%i in (*.frag, *.vert) do %VULKAN_SDK%/Bin/glslc.exe %%i -o %%i.spv

:: Then move them

move "%SourceDir%\*.spv" "%TargetDir%"

PAUSE
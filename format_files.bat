@echo off
setlocal EnableDelayedExpansion

set "exclude_folder=vendor"

for /R %%f in (*.cpp *.h) do (
    set "filename=%%~f"
  
    if "!filename:%exclude_folder%=!" neq "!filename!" (
        echo "No need to format Third-party libs.
    ) else (
        echo Formatted: "%%f"
        clang-format -i "%%f" 
    )
)

::@PAUSE
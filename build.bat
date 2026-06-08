@echo off
REM Build script for dpi_counter (raw-input mouse count tester)
REM Tries MSVC (cl.exe) first, falls back to MinGW (gcc) if not found

echo Building dpi_counter.exe...

where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using MSVC compiler...
    cl.exe /nologo /O2 /W4 dpi_counter.c ^
        /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ^
        /OUT:dpi_counter.exe
    del *.obj 2>nul
    goto :done
)

where gcc.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using MinGW gcc compiler...
    gcc -O2 -Wall -o dpi_counter.exe dpi_counter.c -luser32 -lgdi32 -mwindows
    goto :done
)

echo ERROR: No compiler found ^(need cl.exe or gcc^).
exit /b 1

:done
if %ERRORLEVEL% EQU 0 (echo Done: dpi_counter.exe) else (echo Build failed %ERRORLEVEL%)

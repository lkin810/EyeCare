@echo off
REM =============================================================
REM  EyeCare v6.4 Build Script (MSVC)
REM
REM  Usage:
REM    1. Open "x64 Native Tools Command Prompt for VS 2022"
REM    2. cd to this directory
REM    3. Run build.bat
REM
REM  Output: EyeCare.exe
REM
REM  Note: MSVC build reduces antivirus false positives
REM  MinGW builds are more likely flagged by Windows Defender
REM =============================================================

echo ============================================
echo   EyeCare v6.4 - Building...
echo ============================================

REM Step 1: Compile resource script (VERSIONINFO)
echo [1/2] Compiling resources...
rc EyeCare.rc
if %errorlevel% neq 0 (
    echo.
    echo Resource compilation failed! Check:
    echo   - Are you running in VS Developer Command Prompt?
    echo   - Is Windows SDK installed?
    echo.
    echo Trying without resources...
    set RES_LIB=
) else (
    set RES_LIB=EyeCare.res
)

REM Step 2: Compile and link
REM /O2  = Max optimization
REM /GL  = Whole program optimization
REM /DUNICODE /D_UNICODE = Unicode macros
REM /SUBSYSTEM:WINDOWS = Windows GUI subsystem
echo [2/2] Compiling main program...
cl /O2 /GL /EHsc /utf-8 /DUNICODE /D_UNICODE /MD EyeCare.cpp %RES_LIB% /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib advapi32.lib shell32.lib comctl32.lib winmm.lib gdiplus.lib ole32.lib shfolder.lib

if %errorlevel% equ 0 (
    echo.
    echo ============================================
    echo   Build successful!
    echo ============================================
    for %%A in (EyeCare.exe) do echo Size: %%~zA bytes
    REM Clean up intermediate files
    del /q *.obj 2>nul
) else (
    echo.
    echo ============================================
    echo   Build failed! Check error messages above.
    echo.
    echo   Common issues:
    echo   1. Run in VS Developer Command Prompt
    echo   2. Install "C++ Desktop Development" workload
    echo   3. Install Windows SDK if rc fails
    echo ============================================
)

del /q *.res 2>nul
pause

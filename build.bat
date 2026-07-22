@echo off
setlocal enabledelayedexpansion

REM ******************************
REM Customer only need to modify following 2 line
set EATSDKDIR=E:\OpenSDK\Anjay\SDK\SIMCOM_SDK_SET

REM ******************************

set PROJECTDIR=%CD%

del %PROJECTDIR%\out\*.*   /s /q
del %EATSDKDIR%\output\*.* /s /q
del %EATSDKDIR%\sc_app\*.*  /s /q
REM Clean anjay build directory to avoid stale .a files and Windows ar race condition
REM (arm-none-eabi-ar on Windows has race conditions with parallel builds)
if exist "%EATSDKDIR%\sc_app\anjay_build" rmdir /s /q "%EATSDKDIR%\sc_app\anjay_build"
XCOPY  %PROJECTDIR%\sc_app  %EATSDKDIR%\sc_app\  /S/E/Y
COPY  %PROJECTDIR%\CMakeLists.txt  %EATSDKDIR%\CMakeLists.txt
COPY  %PROJECTDIR%\.config  %EATSDKDIR%\config\.config
cd %EATSDKDIR%

GNUmake clean

REM ============================================================================
REM Check if SELECTED_MODULE is already defined by user
REM ============================================================================
if defined SELECTED_MODULE (
    echo Using pre-defined module: !SELECTED_MODULE!
    echo Running: gnumake !SELECTED_MODULE!
    REM BUILD=ninja -j1: limit ninja parallelism to avoid arm-none-eabi-ar race condition on Windows
    gnumake "BUILD=ninja -j1" !SELECTED_MODULE!
    goto COPY_OUTPUT
)

REM ============================================================================
REM Module extraction logic (only executed if SELECTED_MODULE not defined)
REM ============================================================================
set "found=0"
set "COUNT=0"

for /f "delims=" %%a in ('gnumake') do (
    if "!found!"=="1" (
        echo %%a | findstr /b /c:"-" >nul
        if !errorlevel! == 0 (
            set "line=%%a"
            set "line=!line:- =!"
            for /f "tokens=* delims= " %%b in ("!line!") do (
                set "modulename=%%b"
            )
            if "!modulename!"=="" (
                set "found=0"
            ) else if "!modulename!"=="-" (
                set "found=0"
            ) else (
                echo !modulename! | findstr ":" >nul
                if !errorlevel! == 0 (
                    set "found=0"
                ) else (
                    set /a COUNT+=1
                    set "MODULE_NAME_!COUNT!=!modulename!"
                )
            )
        ) else (
            set "found=0"
        )
    )
    echo %%a | findstr /c:"module list:" >nul
    if !errorlevel! == 0 (
        set "found=1"
    )
)

echo Module count: !COUNT!

if !COUNT! == 1 (
    set "SELECTED_MODULE=!MODULE_NAME_1!"
    echo Only one module found: !SELECTED_MODULE!
) else if !COUNT! == 0 (
    echo No valid modules found.
    exit /b 1
) else (
    echo Multiple modules found:
    for /l %%i in (1,1,!COUNT!) do (
        call echo   %%i. %%MODULE_NAME_%%i%%
    )
    
    :GET_CHOICE
    set /p "CHOICE=Please enter the number of the module to use (1-!COUNT!): "
    
    REM Clean input by using FOR to parse it
    set "CLEANED="
    for /f "tokens=* delims= " %%c in ("!CHOICE!") do set "CLEANED=%%c"
    
    if "!CLEANED!"=="" (
        echo No input. Please try again.
        goto GET_CHOICE
    )
    
    REM Try to convert to number - this validates it's a valid integer
    set /a TEST=!CLEANED! 2>nul
    if errorlevel 1 (
        echo Invalid input: '!CLEANED!' is not a valid number.
        goto GET_CHOICE
    )
    
    REM Check range
    if !TEST! lss 1 (
        echo Number must be at least 1.
        goto GET_CHOICE
    )
    
    if !TEST! gtr !COUNT! (
        echo Number must be between 1 and !COUNT!.
        goto GET_CHOICE
    )
    
    set "CHOICE=!TEST!"
    for %%i in (!CHOICE!) do (
        set "SELECTED_MODULE=!MODULE_NAME_%%i!"
    )
    
    echo You selected: [!SELECTED_MODULE!]
    
    if "!SELECTED_MODULE!"=="" (
        echo Invalid selection. Exiting.
        exit /b 1
    )
)

REM ============================================================================
REM Build the selected module
REM ============================================================================
echo Running: gnumake !SELECTED_MODULE!
REM BUILD=ninja -j1: limit ninja parallelism to avoid arm-none-eabi-ar race condition on Windows
gnumake "BUILD=ninja -j1" !SELECTED_MODULE!

:COPY_OUTPUT
REM ============================================================================
REM Copy output files (executed for both pre-defined and selected modules)
REM ============================================================================
if exist "%EATSDKDIR%\out\!SELECTED_MODULE!\*.zip" (
    COPY "%EATSDKDIR%\out\!SELECTED_MODULE!\*.zip" "%PROJECTDIR%\output\"
) else (
    echo WARNING: No zip files found at %EATSDKDIR%\out\!SELECTED_MODULE!\
    echo Looking for zip files in %EATSDKDIR%\out\...
    if exist "%EATSDKDIR%\out\*.zip" (
        COPY "%EATSDKDIR%\out\*.zip" "%PROJECTDIR%\output\"
        echo Zip files copied from output root.
    )
)

cd "%PROJECTDIR%"

endlocal
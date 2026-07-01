@echo off
setlocal enabledelayedexpansion

echo Checking for C compiler...

where gcc >nul 2>&1
if !errorlevel! equ 0 (
    set CC=gcc
    goto compile
)

where cl >nul 2>&1
if !errorlevel! equ 0 (
    set CC=cl
    goto compile_msvc
)

where clang >nul 2>&1
if !errorlevel! equ 0 (
    set CC=clang
    goto compile
)

echo No C compiler found. Please install GCC, Clang, or MSVC.
pause
exit /b 1

:compile
echo Found %CC% compiler
echo.
echo Compiling disk_manager.c...
%CC% -c src/disk_manager.c -o src/disk_manager.o -Wall -Wextra -g -I./src

if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Creating library...
ar rcs libdbengine.a src/disk_manager.o

if !errorlevel! neq 0 (
    echo Library creation failed!
    pause
    exit /b 1
)

echo.
echo Compiling test...
%CC% -o test_disk_manager tests/test_disk_manager.c libdbengine.a -Wall -Wextra -g -I./src

if !errorlevel! neq 0 (
    echo Test compilation failed!
    pause
    exit /b 1
)

echo.
echo Running tests...
echo.
test_disk_manager.exe

echo.
echo Build completed!
pause
exit /b 0

:compile_msvc
echo Found MSVC compiler
echo.
echo Compiling disk_manager.c...
cl /c /W3 /Zi src/disk_manager.c /I.\src

if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Creating library...
lib /OUT:libdbengine.lib src/disk_manager.obj

if !errorlevel! neq 0 (
    echo Library creation failed!
    pause
    exit /b 1
)

echo.
echo Compiling test...
cl /Fe:test_disk_manager.exe tests/test_disk_manager.c libdbengine.lib /I.\src /W3 /Zi

if !errorlevel! neq 0 (
    echo Test compilation failed!
    pause
    exit /b 1
)

echo.
echo Running tests...
echo.
test_disk_manager.exe

echo.
echo Build completed!
pause
exit /b 0

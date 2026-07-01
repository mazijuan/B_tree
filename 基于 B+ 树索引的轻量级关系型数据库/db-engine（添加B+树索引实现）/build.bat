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
echo Compiling bplus_tree.c...
%CC% -c src/bplus_tree.c -o src/bplus_tree.o -Wall -Wextra -g -I./src

if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Creating library...
ar rcs libdbengine.a src/disk_manager.o src/bplus_tree.o

if !errorlevel! neq 0 (
    echo Library creation failed!
    pause
    exit /b 1
)

echo.
echo Compiling disk_manager test...
%CC% -o test_disk_manager tests/test_disk_manager.c libdbengine.a -Wall -Wextra -g -I./src

if !errorlevel! neq 0 (
    echo Disk test compilation failed!
    pause
    exit /b 1
)

echo.
echo Compiling B+Tree test...
%CC% -o test_bpt tests/test_bpt.c libdbengine.a -Wall -Wextra -g -I./src

if !errorlevel! neq 0 (
    echo B+Tree test compilation failed!
    pause
    exit /b 1
)

echo.
echo Running disk_manager tests...
echo.
test_disk_manager.exe

echo.
echo Running B+Tree tests...
echo.
test_bpt.exe

echo.
echo Build completed!
pause
exit /b 0

:compile_msvc
echo Found MSVC compiler
echo.
echo Compiling disk_manager.c...
cl /c /W3 /Zi /source-charset:utf-8 src/disk_manager.c /I.\src

if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Compiling bplus_tree.c...
cl /c /W3 /Zi /source-charset:utf-8 src/bplus_tree.c /I.\src

if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Creating library...
lib /OUT:libdbengine.lib src/disk_manager.obj src/bplus_tree.obj

if !errorlevel! neq 0 (
    echo Library creation failed!
    pause
    exit /b 1
)

echo.
echo Compiling disk_manager test...
cl /Fe:test_disk_manager.exe tests/test_disk_manager.c libdbengine.lib /I.\src /W3 /Zi /source-charset:utf-8

if !errorlevel! neq 0 (
    echo Disk test compilation failed!
    pause
    exit /b 1
)

echo.
echo Compiling B+Tree test...
cl /Fe:test_bpt.exe tests/test_bpt.c libdbengine.lib /I.\src /W3 /Zi /source-charset:utf-8

if !errorlevel! neq 0 (
    echo B+Tree test compilation failed!
    pause
    exit /b 1
)

echo.
echo Running disk_manager tests...
echo.
test_disk_manager.exe

echo.
echo Running B+Tree tests...
echo.
test_bpt.exe

echo.
echo Build completed!
pause
exit /b 0

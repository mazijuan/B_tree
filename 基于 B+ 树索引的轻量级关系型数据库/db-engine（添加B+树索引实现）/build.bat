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
echo Compiling buffer_pool.c...
%CC% -c src/buffer_pool.c -o src/buffer_pool.o -Wall -Wextra -g -I./src
if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Creating library...
ar rcs libdbengine.a src/disk_manager.o src/bplus_tree.o src/buffer_pool.o
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
echo Compiling B+Tree Delete test...
%CC% -o test_bpt_delete tests/test_bpt_delete.c libdbengine.a -Wall -Wextra -g -I./src
if !errorlevel! neq 0 (
    echo B+Tree Delete test compilation failed!
    pause
    exit /b 1
)

echo.
echo Compiling Buffer Pool test...
%CC% -o test_buffer_pool tests/test_buffer_pool.c libdbengine.a -Wall -Wextra -g -I./src
if !errorlevel! neq 0 (
    echo Buffer Pool test compilation failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Running disk_manager tests...
echo ========================================
echo.
test_disk_manager.exe

echo.
echo ========================================
echo Running B+Tree tests...
echo ========================================
echo.
test_bpt.exe

echo.
echo ========================================
echo Running B+Tree Delete tests...
echo ========================================
echo.
test_bpt_delete.exe

echo.
echo ========================================
echo Running Buffer Pool tests...
echo ========================================
echo.
test_buffer_pool.exe

echo.
echo Build and tests completed!
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
echo Compiling buffer_pool.c...
cl /c /W3 /Zi /source-charset:utf-8 src/buffer_pool.c /I.\src
if !errorlevel! neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Creating library...
lib /OUT:libdbengine.lib src/disk_manager.obj src/bplus_tree.obj src/buffer_pool.obj
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
echo Compiling B+Tree Delete test...
cl /Fe:test_bpt_delete.exe tests/test_bpt_delete.c libdbengine.lib /I.\src /W3 /Zi /source-charset:utf-8
if !errorlevel! neq 0 (
    echo B+Tree Delete test compilation failed!
    pause
    exit /b 1
)

echo.
echo Compiling Buffer Pool test...
cl /Fe:test_buffer_pool.exe tests/test_buffer_pool.c libdbengine.lib /I.\src /W3 /Zi /source-charset:utf-8
if !errorlevel! neq 0 (
    echo Buffer Pool test compilation failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Running disk_manager tests...
echo ========================================
echo.
test_disk_manager.exe

echo.
echo ========================================
echo Running B+Tree tests...
echo ========================================
echo.
test_bpt.exe

echo.
echo ========================================
echo Running B+Tree Delete tests...
echo ========================================
echo.
test_bpt_delete.exe

echo.
echo ========================================
echo Running Buffer Pool tests...
echo ========================================
echo.
test_buffer_pool.exe

echo.
echo Build and tests completed!
pause
exit /b 0

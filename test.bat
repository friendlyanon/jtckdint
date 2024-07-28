@echo off

if not "%jtckdint_build_mode%" == "" goto :build_with_mode

call :self mingw
if not %errorlevel% == 0 exit /b %errorlevel%

call :self llvm
if not %errorlevel% == 0 exit /b %errorlevel%

call :self msvc
exit /b %errorlevel%

:build_with_mode
setlocal
call :%jtckdint_build_mode%
set code=%errorlevel%
if not %code% == 0 echo Exited with code: %code%>&2
endlocal & exit /b %code%

:self
setlocal
set jtckdint_build_mode=%1
call "%~f0"
endlocal & set code=%errorlevel%
for %%g in (o obj ilk pdb) do if exist test.%%g del test.%%g
if %code% == 0 if exist test.exe del test.exe
exit /b %code%

:mingw
setlocal

call mingw.bat
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=gcc.exe -isystem . -Wall -Wextra -Wno-type-limits -Werror -o test.exe

call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -pedantic-errors -std=c11
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O0
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O0 -pedantic-errors -std=c11
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O3
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O3 -pedantic-errors -std=c11

endlocal & exit /b %errorlevel%

:llvm
setlocal

if "%CLANG_VERSION_PREFIX%" == "" set CLANG_VERSION_PREFIX=C:\Program Files\LLVM\lib\clang\18

set comp=clang.exe -isystem . -Weverything -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -D_CRT_SECURE_NO_WARNINGS=1 -o test.exe

call :build -g -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -g -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=c11 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O0 -l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O0 -std=c11 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O3 -l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O3 -std=c11 -D__STRICT_ANSI__=1

endlocal & exit /b %errorlevel%

:msvc
setlocal

call vcvars.bat
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=cl.exe /nologo /Wall /WX /D_CRT_SECURE_NO_WARNINGS=1 /diagnostics:caret /external:I . /external:W0

:: call :build /Od
:: if not %errorlevel% == 0 exit /b %errorlevel%
call :build /Od /std:c11
if not %errorlevel% == 0 exit /b %errorlevel%

:: call :build /O2
:: if not %errorlevel% == 0 exit /b %errorlevel%
call :build /O2 /wd4710 /wd4711 /wd4883 /std:c11

endlocal & exit /b %errorlevel%

:build
echo ^< %comp% %* test.c
%comp% %* test.c
if not %errorlevel% == 0 exit /b %errorlevel%

echo ^> test.exe
test.exe
exit /b %errorlevel%

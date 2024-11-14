@echo off

if not "%jtckdint_build_mode%" == "" goto :build_with_mode

call :self mingw
if not %errorlevel% == 0 exit /b %errorlevel%

call :self llvm
if not %errorlevel% == 0 exit /b %errorlevel%

call :self msvc
exit /b %errorlevel%

:build_with_mode
call :%jtckdint_build_mode%
if not %errorlevel% == 0 >&2 echo ! Exited with code: [%errorlevel%]
exit /b %errorlevel%

:self
setlocal
set jtckdint_build_mode=%1
call "%~f0"
endlocal & set code=%errorlevel%
for %%g in (o obj ilk pdb) do if exist test.%%g del test.%%g
for %%g in (o obj ilk pdb) do if exist other.%%g del other.%%g
if %code% == 0 if exist test.exe del test.exe
exit /b %code%

:mingw
setlocal

call mingw.bat
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=gcc.exe -isystem . -Wall -Wextra -Wconversion -Werror -o test.exe

call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=gnu11
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -Wpedantic -std=c11
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O0 -std=gnu11
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O0 -Wpedantic -std=c11
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O3 -std=gnu11
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O3 -Wpedantic -std=c11
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=g++.exe -isystem . -Wall -Wextra -Wconversion -Wpedantic -Werror -o test.exe -x c++

call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=c++14
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O0 -std=c++14
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O3 -std=c++14

exit /b %errorlevel%

:llvm
setlocal

if "%CLANG_VERSION_PREFIX%" == "" set CLANG_VERSION_PREFIX=C:\Program Files\LLVM\lib\clang\19

set comp=clang.exe -isystem . -Weverything -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wno-pre-c11-compat -Werror -D_CRT_SECURE_NO_WARNINGS=1 -o test.exe

call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=gnu11 -l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=c11 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O0 -std=gnu11 -l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O0 -std=c11 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O3 -std=gnu11 -l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
if not %errorlevel% == 0 exit /b %errorlevel%
call :build -O3 -std=c11 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=clang++.exe -isystem . -Weverything -Wno-unsafe-buffer-usage -Wno-c++98-compat -Wno-c++98-compat-pedantic -Werror -D_CRT_SECURE_NO_WARNINGS=1 -o test.exe -x c++

call :build -Os -fsanitize=undefined -fsanitize-undefined-trap-on-error -std=c++14 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O0 -std=c++14 -D__STRICT_ANSI__=1
if not %errorlevel% == 0 exit /b %errorlevel%

call :build -O3 -std=c++14 -D__STRICT_ANSI__=1

exit /b %errorlevel%

:msvc
setlocal

call vcvars.bat
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=cl.exe /nologo /Wall /wd4710 /WX /D_CRT_SECURE_NO_WARNINGS=1 /diagnostics:caret /external:I . /external:W0 /permissive- /Zc:inline /Zc:preprocessor

call :build /Od /d2Obforceinline /std:c11
if not %errorlevel% == 0 exit /b %errorlevel%

call :build /O2 /wd4711 /wd4883 /std:c11
if not %errorlevel% == 0 exit /b %errorlevel%

set comp=%comp% /EHsc /TP /Zc:__cplusplus

call :build /Od /d2Obforceinline
if not %errorlevel% == 0 exit /b %errorlevel%

call :build /O2 /wd4711 /wd4883

exit /b %errorlevel%

:build
echo ^< %comp% %* test.c other.c
%comp% %* test.c other.c
if not %errorlevel% == 0 exit /b %errorlevel%

echo ^> test.exe
test.exe
exit /b %errorlevel%

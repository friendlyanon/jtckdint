@echo off

if not "%jtckdint_build_mode%" == "" goto :build_with_mode

call :self mingw32 || goto :exit
call :self mingw64 || goto :exit
call :self llvm || goto :exit
call :self msvc32 || goto :exit
call :self msvc64

:exit
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

:mingw32
setlocal
set arch=x86
call :mingw
exit /b %errorlevel%

:mingw64
setlocal
set arch=amd64
call :mingw
exit /b %errorlevel%

:mingw
setlocal

echo ? arch=%arch%

call mingw.bat %arch% || goto :exit

set ubsan=-fsanitize=undefined -fsanitize-undefined-trap-on-error
set flags=-isystem . -Wall -Wextra -Wconversion -Werror -Wfatal-errors -DJTCKDINT_OPTION_STDCKDINT=2 -o test.exe
set comp=gcc.exe %flags%

call :build -Os %ubsan% -std=gnu11 || goto :exit
call :build -Os %ubsan% -Wpedantic -std=c11 || goto :exit

call :build -O0 -std=gnu11 || goto :exit
call :build -O0 -Wpedantic -std=c11 || goto :exit

call :build -O3 -std=gnu11 || goto :exit
call :build -O3 -Wpedantic -std=c11 || goto :exit

set comp=g++.exe %flags% -Wpedantic -x c++

call :build -Os %ubsan% -std=c++14 || goto :exit

call :build -O0 -std=c++14 || goto :exit

call :build -O3 -std=c++14

exit /b %errorlevel%

:llvm
setlocal

echo ? arch=amd64

if "%CLANG_VERSION_PREFIX%" == "" set CLANG_VERSION_PREFIX=C:\Program Files\LLVM\lib\clang\19

set ubsan=-fsanitize=undefined -fsanitize-undefined-trap-on-error
set builtins=-l "%CLANG_VERSION_PREFIX%\lib\windows\clang_rt.builtins-x86_64.lib"
set flags=-fno-ms-compatibility -isystem . -Weverything  -Wno-unsafe-buffer-usage -Werror -Wfatal-errors -D_CRT_SECURE_NO_WARNINGS=1 -DJTCKDINT_OPTION_STDCKDINT=2 -o test.exe
set comp=clang.exe %flags% -Wno-declaration-after-statement -Wno-pre-c11-compat

call :build -Os %ubsan% -std=gnu11 %builtins% || goto :exit
call :build -Os %ubsan% -std=c11 || goto :exit

call :build -O0 -std=gnu11 %builtins% || goto :exit
call :build -O0 -std=c11 || goto :exit

call :build -O3 -std=gnu11 %builtins% || goto :exit
call :build -O3 -std=c11 || goto :exit

set comp=clang++.exe %flags% -Wno-c++98-compat -Wno-c++98-compat-pedantic -x c++

call :build -Os %ubsan% -std=c++14 || goto :exit

call :build -O0 -std=c++14 || goto :exit

call :build -O3 -std=c++14

exit /b %errorlevel%

:msvc32
setlocal
set arch=x86
call :msvc
exit /b %errorlevel%

:msvc64
setlocal
set arch=amd64
call :msvc
exit /b %errorlevel%

:msvc
setlocal

echo ? arch=%arch%

call vcvars.bat -arch=%arch% -host_arch=amd64 -no_logo || goto :exit

set comp=cl.exe /nologo /Wall /wd4710 /WX /D_CRT_SECURE_NO_WARNINGS=1 /DJTCKDINT_OPTION_STDCKDINT=2 /diagnostics:caret /external:I . /external:W0 /permissive- /Zc:inline /Zc:preprocessor

call :build /Od /d2Obforceinline /std:c11 || goto :exit

call :build /O2 /wd4711 /wd4883 /std:c11 || goto :exit

set comp=%comp% /EHsc /TP /Zc:__cplusplus

call :build /Od /d2Obforceinline || goto :exit

call :build /O2 /wd4711 /wd4883

exit /b %errorlevel%

:build
echo ^< %comp% %* test.c other.c
%comp% %* test.c other.c || goto :exit

echo ^> test.exe
test.exe
exit /b %errorlevel%

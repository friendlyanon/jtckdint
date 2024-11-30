$arch = $args[0]
$compiler = $args[1]

if ($compiler -eq 'mingw') {
  $tag = (Invoke-RestMethod https://api.github.com/repos/friendlyanon/w64devkit/releases/latest -Method Get).tag_name
  Invoke-WebRequest "https://github.com/friendlyanon/w64devkit/releases/download/$tag/mingw-$arch.exe" -OutFile "mingw-$arch.exe"
  Start-Process "mingw-$arch.exe" -ArgumentList '-y', '-o.' -Wait
  Set-Content mingw.bat "@echo off`r`nset CC=gcc.exe`r`nset CXX=g++.exe`r`nset `"PATH=%cd%\w64devkit\bin;%PATH%`""
  .\w64devkit\bin\gcc.exe -dumpmachine
} elseif ($compiler -eq 'llvm') {
  choco.exe install llvm --version=19.1.3 --no-progress
} else {
  Set-Content vcvars.bat "@echo off`r`nfor /F `"delims=`" %%g in ('`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe`" -Property InstallationPath') do set dir=%%g`r`ncall `"%dir%\Common7\Tools\vsdevcmd.bat`" -arch=$arch -host_arch=amd64 -no_logo"
}

$arch = if ($compiler -eq 'llvm') { '' } elseif ($arch -eq 'x86') { '32' } else { '64' }
Add-Content $env:GITHUB_ENV "jtckdint_build_mode=$compiler$arch"

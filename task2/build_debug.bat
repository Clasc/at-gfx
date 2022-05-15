@echo OFF
setlocal

echo Compiling RRT Debug...
for /f "usebackq delims=" %%i in (`vswhere.exe -prerelease -latest -property installationPath`) do (
  if exist "%%i\Common7\Tools\vsdevcmd.bat" (
    call "%%i\Common7\Tools\vsdevcmd.bat" -arch=amd64 -host_arch=amd64
  )
)

set commonCompilerOptions=-nologo -GR- -EHsc -WX -W4 -wd4003 -wd4281 -wd4100 -wd4189 -wd4706 -wd4127 -wd4505 -I ..\..\external -I ..\..\external\SDL -FC -Z7 -std:c++17
set debugLinkerOptions= /link /ignore:4099 /LIBPATH:../../libraries/debug /INCREMENTAL:NO /SUBSYSTEM:CONSOLE SDL2d.lib SDL2maind.lib glew32.lib opengl32.lib

IF NOT EXIST "./binaries/x64_debug" mkdir "./binaries/x64_debug"
pushd "./binaries/x64_debug"
cl -MDd -Od -DDEBUG %commonCompilerOptions% ..\..\source\main.cpp %debugLinkerOptions% /OUT:rrt_debug.exe


echo.
echo Copying external libraries...
xcopy "..\..\libraries\debug\*.dll" /y
popd

endlocal
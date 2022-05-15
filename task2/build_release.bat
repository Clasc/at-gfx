@echo OFF
setlocal

echo Compiling RRT...
for /f "usebackq delims=" %%i in (`vswhere.exe -prerelease -latest -property installationPath`) do (
  if exist "%%i\Common7\Tools\vsdevcmd.bat" (
    call "%%i\Common7\Tools\vsdevcmd.bat" -arch=amd64 -host_arch=amd64
  )
)

set commonCompilerOptions=-nologo -GR- -EHsc -WX -W4 -wd4003 -wd4281 -wd4100 -wd4189 -wd4706 -wd4127 -wd4505 -I ..\..\external -I ..\..\external\SDL -FC -Z7 
set releaseLinkerOptions= /link /ignore:4099 /LIBPATH:../../libraries/release /INCREMENTAL:NO /SUBSYSTEM:CONSOLE SDL2.lib SDL2main.lib glew32.lib opengl32.lib

IF NOT EXIST "./binaries/x64_release" mkdir "./binaries/x64_release"
pushd "./binaries/x64_release"
cl -MD -Ox -Oi -GS- %commonCompilerOptions% ..\..\source\main.cpp %releaseLinkerOptions% /OUT:rrt_release.exe

echo.
echo Copying external libraries...
xcopy "..\..\libraries\release\*.dll" /y
popd


endlocal
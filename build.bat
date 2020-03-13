@echo off


pushd build

rem -DEDITOR_SLOW

set compilerFlags=-DEDITOR_SLOW -Od -MTd -nologo -Oi -GR- -EHa- -WX -W4 -wd4101 -wd4702 -wd4005 -wd4505 -wd4456 -wd4201 -wd4100 -wd4189 -wd4204 -wd4459 -Zi -FC

set linkerFlags=-incremental:no -opt:ref OpenGL32.lib Winmm.lib user32.lib Gdi32.lib



cl %compilerFlags% ..\code\main.c /link %linkerFlags%

popd
pushd data
rem ..\build\main.exe
popd
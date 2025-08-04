@echo off

IF NOT EXIST .\build mkdir .\build
pushd .\build
cl -MT -Gm- -nologo -GR- -EHa- -Oi -WX /W4 /wd4201 /wd4100 /wd4189 -DHANDMADE_DEV=1 -DHANDMADE_LINT=1 -DHANDMADE_WIN32=1 -FC -Zi -Fm ..\src\win32_game.cpp /link -opt:ref -subsystem:windows user32.lib gdi32.lib
popd

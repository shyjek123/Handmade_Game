@echo off
mkdir .\build
pushd .\build
cl /FC /Zi ..\src\win32_game.cpp user32.lib gdi32.lib

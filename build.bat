@echo off
mkdir .\build
pushd .\build
cl /FC /Zi ..\src\win32_game.cpp user32.lib gdi32.lib

echo debug exe? 
set /p debug=

if /i "%debug%"=="y" (
  devenv .\win32_game.exe
) 

if /i "%debug%"=="yes" (
  devenv 
)

popd


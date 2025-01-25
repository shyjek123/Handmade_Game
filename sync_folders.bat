@echo off

SET SOURCE_DIR="C:\Users\sebas\source\projects\Handmade_Game"
SET TARGET_DIR="C:\Users\sebas\source\github\accounts\shyjek123\Handmade_Game"

xcopy /E /I /H /Y "%SOURCE_DIR%\*" "%TARGET_DIR%\"

echo Files copied from %SOURCE_DIR% to %TARGET_DIR%


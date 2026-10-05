@echo off
rem Launches the latest Release build with the game data in bin\UFO.
rem OXCE keeps its own user folder (saves, options, logs) so it never mixes with the vanilla build.
set "OXCE_USER=%USERPROFILE%\OneDrive\Documents\OpenXcom Extended"
if not exist "%OXCE_USER%" mkdir "%OXCE_USER%"
cd /d "%~dp0bin\x64"
start "" "Release\OpenXcom.exe" -data ../ -user "%OXCE_USER%" -config "%OXCE_USER%"

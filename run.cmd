@echo off
rem Launches the latest Release build with the game data in bin\UFO.
cd /d "%~dp0bin\x64"
start "" "Release\OpenXcom.exe" -data ../

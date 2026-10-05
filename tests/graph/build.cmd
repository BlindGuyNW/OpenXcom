@echo off
rem Builds and runs the graph kernel conformance tests. The tests need C++20 (the harness uses
rem a requires-expression); the kernel itself is C++17 and builds into the game unchanged.
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set OUT=%~dp0..\..\build\graphtests
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /std:c++20 /EHsc /W3 /O2 /utf-8 /I"%~dp0..\..\src\Access" /Fo"%OUT%\\" /Fe"%OUT%\GraphTests.exe" "%~dp0*.cpp" "%~dp0..\..\src\Access\Graph\*.cpp" || exit /b 1
"%OUT%\GraphTests.exe"

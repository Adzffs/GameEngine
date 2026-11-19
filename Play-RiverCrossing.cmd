@echo off
setlocal

if not "%~2"=="" goto usage
if not "%~1"=="" if /i not "%~1"=="--build-only" goto usage

set "repoRoot=%~dp0."
set "buildRoot=%~dp0build-river-crossing"
set "worldFile=%~dp0content\worlds\river-crossing.blockout.json"
if not defined GAMEENGINE_UCRT64_BIN set "GAMEENGINE_UCRT64_BIN=C:\msys64\ucrt64\bin"

if not exist "%worldFile%" goto missing_source
if not exist "%~dp0src\Core\GameLaunchOptions.cpp" goto missing_source
for %%T in (cmake.exe ninja.exe g++.exe SDL3.dll) do if not exist "%GAMEENGINE_UCRT64_BIN%\%%T" goto missing_toolchain

rem Select the matching compiler and DLLs for this process only.
set "PATH=%GAMEENGINE_UCRT64_BIN%;%PATH%"
echo Building River Crossing from: "%repoRoot%"
"%GAMEENGINE_UCRT64_BIN%\cmake.exe" -S "%repoRoot%" -B "%buildRoot%" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="%GAMEENGINE_UCRT64_BIN%\g++.exe" -DCMAKE_MAKE_PROGRAM="%GAMEENGINE_UCRT64_BIN%\ninja.exe" -DCMAKE_PREFIX_PATH="%GAMEENGINE_UCRT64_BIN%\.." -DBUILD_TESTING=OFF -DGAMEENGINE_BUILD_CLIENT=ON -DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF -DGAMEENGINE_BUILD_CONTENT_TOOLS=OFF
if errorlevel 1 goto failed
"%GAMEENGINE_UCRT64_BIN%\cmake.exe" --build "%buildRoot%" --target GameEngine --parallel 6
if errorlevel 1 goto failed
if /i "%~1"=="--build-only" exit /b 0

echo Starting a fresh River Crossing character with temporary progress.
pushd "%repoRoot%"
if errorlevel 1 goto failed
"%buildRoot%\GameEngine.exe" --world "%worldFile%" --profile fresh --no-save
set "gameExit=%ERRORLEVEL%"
popd
if not "%gameExit%"=="0" goto failed
exit /b 0

:missing_source
echo This checkout does not contain the current River Crossing game source.
echo Update this checkout from master, including the content directory, then run this launcher again.
goto failed

:missing_toolchain
echo Missing UCRT64 compiler, CMake, Ninja or SDL3 in: "%GAMEENGINE_UCRT64_BIN%"
echo Install the MSYS2 UCRT64 dependencies described in docs\embervale\06-development\GETTING_STARTED.md.
echo For another installation, set GAMEENGINE_UCRT64_BIN to its ucrt64\bin directory.
goto failed

:usage
echo Usage: Play-RiverCrossing.cmd [--build-only]
exit /b 2

:failed
echo River Crossing could not be built or started. See the error above.
if /i not "%~1"=="--build-only" pause
exit /b 1

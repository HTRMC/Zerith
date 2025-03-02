@echo off
echo Building Zerith Vulkan Triangle project...

rem Compile shaders
call compile_shaders.bat

rem Create build directory if it doesn't exist
if not exist "build" mkdir build

rem Navigate to build directory
cd build

rem Generate build files
cmake ..

rem Build the project
cmake --build . --config Release

echo Build complete!
echo Run the application with: build\Release\Zerith.exe
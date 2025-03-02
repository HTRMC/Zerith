@echo off
echo Compiling shaders...

rem Create shader directory if it doesn't exist
if not exist "shaders" mkdir shaders

rem Path to glslc compiler - adjust this to your Vulkan SDK path
set GLSLC="%VULKAN_SDK%\Bin\glslc.exe"

rem Compile vertex shader
echo Compiling vertex shader...
%GLSLC% -o shaders/vert.spv shaders/shader.vert

rem Compile fragment shader
echo Compiling fragment shader...
%GLSLC% -o shaders/frag.spv shaders/shader.frag

echo Shader compilation complete!
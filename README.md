# Zerith

![License](https://img.shields.io/badge/License-All\_Rights\_Reserved-blue?style=flat-square&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-23-red?style=flat-square&logo=cplusplus&logoColor=white)
![Vulkan](https://img.shields.io/badge/Vulkan-1.4-purple?style=flat-square&logo=vulkan&logoColor=white)
![Status](https://img.shields.io/badge/Status-In\_Development-green?style=flat-square&logoColor=white)

A 3D voxel-based game engine built with Vulkan. Starting as a Minecraft-inspired foundation, Zerith aims to evolve into a unique game with custom mechanics, world-building, and visual aesthetics.

> **Mesh Shader Rewrite**  
> Zerith is a complete rewrite of the [original engine](https://github.com/HTRMC/Zerith-Old), now utilizing **mesh shaders** instead of the traditional vertex/fragment pipeline.  
> This modern approach allows for significantly more efficient and scalable rendering—especially suited for voxel-based environments.

## Current Features
- Vulkan rendering pipeline
- Basic triangle rendering (foundation for 3D graphics)

## Planned Features
- Procedural terrain generation
- Voxel-based world editing
- Custom block types and textures
- Physics and collision system
- Unique gameplay mechanics
- Multiplayer support

## Requirements

- C++23 compatible compiler
- CMake 3.30 or higher
- Vulkan SDK 1.4.304.1 or higher
- (Optional) vcpkg for dependency management

## Tested Environments

| OS | Compiler | CMake | Vulkan SDK | Status |
|----|----------|-------|------------|--------|
| Windows 11 | MSVC 19.38 | 3.30.0 | 1.4.304.1 | ✅ Working |
| Windows 10 | MSVC 19.37 | 3.30.0 | 1.4.304.1 | ⚠️ Untested |
| Ubuntu 22.04 | GCC 12.3 | 3.30.0 | 1.4.304.1 | ⚠️ Untested |
| macOS 14 | AppleClang 15.0 | 3.30.0 | MoltenVK 1.2.0 | ⚠️ Untested |

## Building from Source

### Prerequisites
1. Install [Vulkan SDK](https://vulkan.lunarg.com/) (version 1.4.304.1 or higher)
2. Install [CMake](https://cmake.org/download/) (version 3.30 or higher)
3. Install a C++23 compatible compiler:
    - Windows: Visual Studio 2022 or newer
    - Linux: GCC 12 or newer
    - macOS: Xcode 15 or newer

### Windows

#### Using Visual Studio
1. Clone the repository:
   ```
   git clone https://github.com/HTRMC/Zerith.git
   cd Zerith
   ```

2. Generate build files:
   ```
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project:
   ```
   cmake --build . --config Release
   ```

4. Run the application:
   ```
   .\Release\Zerith.exe
   ```

#### Using CLion
1. Clone the repository:
   ```
   git clone https://github.com/HTRMC/Zerith.git
   ```

2. Open the project in CLion

3. Set CMake options in CLion:
    - Go to File > Settings > Build, Execution, Deployment > CMake
    - Add the following to "CMake options":
      ```
      -DCMAKE_TOOLCHAIN_FILE=C:/Users/YOURUSERNAME/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
      ```
    - Adjust the path to your vcpkg installation

4. Build and run the project from CLion

### Linux

1. Clone the repository:
   ```
   git clone https://github.com/HTRMC/Zerith.git
   cd Zerith
   ```

2. Generate build files:
   ```
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project:
   ```
   cmake --build . --config Release
   ```

4. Run the application:
   ```
   ./Zerith
   ```

### macOS

1. Clone the repository:
   ```
   git clone https://github.com/HTRMC/Zerith.git
   cd Zerith
   ```

2. Generate build files:
   ```
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project:
   ```
   cmake --build . --config Release
   ```

4. Run the application:
   ```
   ./Zerith
   ```

## Troubleshooting

### Shader Compilation Issues

If you encounter shader compilation errors:

1. Make sure shader files (`shader.vert` and `shader.frag`) are in the `shaders` directory
2. Make sure the Vulkan SDK is properly installed and the `VULKAN_SDK` environment variable is set
3. Run `compile_shaders.bat` manually to see detailed error messages

### Validation Layer Errors

If you see validation layer errors:

1. Make sure you're using a compatible GPU with proper Vulkan support
2. Update your GPU drivers to the latest version
3. Check that the Vulkan SDK is installed correctly

## Project Structure

```
Zerith/
├── CMakeLists.txt           # CMake configuration
├── build.bat                # Build script for Windows
├── compile_shaders.bat      # Script to compile GLSL shaders to SPIR-V
├── shaders/                 # Directory for shader files
│   ├── shader.vert          # Vertex shader source (GLSL)
│   ├── shader.frag          # Fragment shader source (GLSL)
│   ├── vert.spv             # Compiled vertex shader (SPIR-V)
│   └── frag.spv             # Compiled fragment shader (SPIR-V)
└── src/                     # Source code directory
├── main.cpp             # Application entry point
├── VulkanApp.cpp        # Vulkan application implementation
└── VulkanApp.hpp        # Vulkan application header
```

## Contributing

This project follows conventional commit format:
```
feat(scope): Description
```

Example: `feat(physics): Added player collisions with blocks`

## License

_This project is licensed under All Rights Reserved - see the LICENSE file for details._

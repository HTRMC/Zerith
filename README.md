# Installation

## Windows

### Prerequisites
1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with C++ development tools
2. Install [CMake](https://cmake.org/download/)
3. Install [Vulkan SDK (1.3.296)](https://vulkan.lunarg.com/sdk/home)

### Building
```powershell
mkdir build\Release
cd build\Release
cmake ..\.. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cpack
```

### Installation
1. Run the generated installer in `build\Release\Zerith\Zerith-0.0.1-win64.exe`

## Linux

### Prerequisites
```bash
sudo apt update
sudo apt install build-essential cmake vulkan-tools libvulkan-dev libxcb1-dev glslang-tools
```

### Building
```bash
mkdir -p build/Release
cd build/Release
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Installation
```bash
cpack -G DEB
sudo dpkg -i Zerith/Zerith-0.0.1-Linux.deb

# Fix missing dependencies if needed
sudo apt-get install -f
```

> [!NOTE]  
> Ensure all prerequisites are installed before building.
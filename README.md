# Zerith

# 1. Install dependencies
```sudo apt update```

```sudo apt install build-essential cmake vulkan-tools libvulkan-dev libxcb1-dev glslang-tools```

# 2. Build project
```mkdir -p build/Release```

```cd build/Release```

```cmake ../.. -DCMAKE_BUILD_TYPE=Release```

```make -j$(nproc)```

# 3. Create installer
```cd ../..```

```cd build/Release```

```cpack -G DEB```

# 4. Install package
```sudo dpkg -i Zerith/Zerith-0.0.1-Linux.deb```

If any dependencies missing

```sudo apt-get install -f```  

cmake_minimum_required(VERSION 3.20)
project(Zerith)

# Enable compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(glfw3 3.4 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

# Add GLAD library
add_library(glad STATIC "C:/Users/HTRMC/Dev/Libraries/glad/src/glad.c")
target_include_directories(glad PUBLIC "C:/Users/HTRMC/Dev/Libraries/glad/include")

# Create executable
add_executable(${PROJECT_NAME} main.cpp)

# Include directories for main executable
target_include_directories(${PROJECT_NAME} PRIVATE "C:/Users/HTRMC/Dev/Libraries/glad/include")

# Link all libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        glad
        glfw
        glm::glm
        OpenGL::GL
)
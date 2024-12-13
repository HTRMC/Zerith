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
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "src/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Copy shader directory to build directory
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        ${CMAKE_BINARY_DIR}/shaders
)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/Zerith.png
        ${CMAKE_BINARY_DIR}/Zerith.png
)

# Link all libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glm::glm
        OpenGL::GL
        nlohmann_json::nlohmann_json
        glad::glad
)

# Add the resource file
if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE resources.rc)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi)
endif()
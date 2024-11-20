cmake_minimum_required(VERSION 3.20)
project(Zerith)

# Enable compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# Find GLFW
find_package(glfw3 REQUIRED)

find_package(glm REQUIRED)

# Shader compilation
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/bin)

# Compile shaders
file(GLOB SHADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv
            COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME}"
    )
    list(APPEND SPV_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
endforeach()

add_custom_target(shaders DEPENDS ${SPV_SHADERS})

# Create executable
add_executable(${PROJECT_NAME} main.cpp)
add_dependencies(${PROJECT_NAME} shaders)

# Link Vulkan SDK
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw)
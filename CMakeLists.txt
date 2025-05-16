cmake_minimum_required(VERSION 3.31)
project(MeshShader)

set(CMAKE_CXX_STANDARD 23)

# Find packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Find glslangValidator for shader compilation
find_program(GLSLANG_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/bin)

# Shader compilation function
function(compile_shader TARGET SHADER_FILE SHADER_TYPE OUTPUT_FILE)
    add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_FILE} -S ${SHADER_TYPE} -o ${OUTPUT_FILE} --target-env vulkan1.3
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_TYPE} shader ${SHADER_FILE}"
    )
    add_custom_target(${TARGET} DEPENDS ${OUTPUT_FILE})
endfunction()

# Compile shaders
compile_shader(
        task_shader
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/task_shader.glsl
        "task"
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/task_shader.spv
)

compile_shader(
        mesh_shader
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_shader.glsl
        "mesh"
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/mesh_shader.spv
)

compile_shader(
        fragment_shader
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fragment_shader.glsl
        "frag"
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/fragment_shader.spv
)

# Include directories
include_directories(${Vulkan_INCLUDE_DIRS})

# Add executable
add_executable(MeshShader main.cpp)

# Add shader dependencies
add_dependencies(MeshShader task_shader mesh_shader fragment_shader)

# Link libraries
target_link_libraries(MeshShader ${Vulkan_LIBRARIES} glfw)

# Create a directory for shader output
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
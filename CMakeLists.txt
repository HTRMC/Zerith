cmake_minimum_required(VERSION 3.31)
project(MeshShader)

set(CMAKE_CXX_STANDARD 23)

# Find packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(SPNG CONFIG REQUIRED)

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
include_directories(include)

# Add executable
add_executable(MeshShader 
    main.cpp
    src/chunk.cpp
    src/chunk_mesh_generator.cpp
    src/chunk_manager.cpp
    src/texture_array.cpp
    src/logger.cpp
)

# Add shader dependencies
add_dependencies(MeshShader task_shader mesh_shader fragment_shader)

# Link libraries
target_link_libraries(MeshShader PRIVATE ${Vulkan_LIBRARIES} glfw $<IF:$<TARGET_EXISTS:spng::spng>,spng::spng,spng::spng_static>)

# Create directories for shader output and assets
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)

# Copy assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
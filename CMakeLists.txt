cmake_minimum_required(VERSION 3.31)
project(Zerith)

set(CMAKE_CXX_STANDARD 23)

# Find packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(SPNG CONFIG REQUIRED)

# Add FastNoise2 as subdirectory
add_subdirectory(third_party/FastNoise2)

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

compile_shader(
        aabb_mesh_shader
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/aabb_mesh_shader.glsl
        "mesh"
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/aabb_mesh_shader.spv
)

compile_shader(
        aabb_fragment_shader
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/aabb_fragment_shader.glsl
        "frag"
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/aabb_fragment_shader.spv
)

# Include directories
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(include)

# Add executable
add_executable(Zerith
    main.cpp
    src/chunk.cpp
    src/chunk_mesh_generator.cpp
    src/chunk_manager.cpp
    src/texture_array.cpp
    src/texture_atlas.cpp
    src/logger.cpp
    src/aabb.cpp
    src/player.cpp
    src/terrain_generator.cpp
    src/raycast.cpp
    src/indirect_draw.cpp
    src/block_properties.cpp
    src/block_face_bounds.cpp
    src/octree.cpp
    src/sparse_octree.cpp
    src/chunk_octree.cpp
)

# Add shader dependencies
add_dependencies(Zerith 
    task_shader 
    mesh_shader 
    fragment_shader 
    aabb_mesh_shader 
    aabb_fragment_shader
)

if(MSVC)
    target_compile_options(Zerith PRIVATE
            # Always enable AVX2 SIMD instructions
            /arch:AVX2

            # Debug config: no optimization, runtime checks
            $<$<CONFIG:Debug>:/Od /RTC1>

            # Release config: optimize aggressively + intrinsics + whole program optimization
            $<$<CONFIG:Release>:/O2 /Oi /GL>
    )
    target_link_options(Zerith PRIVATE
            $<$<CONFIG:Release>:/LTCG>
    )
else()
    target_compile_options(Zerith PRIVATE
            # Debug config: no optimization + debug info
            $<$<CONFIG:Debug>:-O0 -g>

            # Release config: optimize + CPU native + loop unrolling
            $<$<CONFIG:Release>:-march=native -O3 -funroll-loops>
    )
endif()

# Link libraries
target_link_libraries(Zerith PRIVATE ${Vulkan_LIBRARIES} glfw $<IF:$<TARGET_EXISTS:spng::spng>,spng::spng,spng::spng_static> FastNoise2)

# Create directories for shader output and assets
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)

# Copy assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
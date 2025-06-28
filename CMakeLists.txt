cmake_minimum_required(VERSION 3.31)
project(Zerith)

set(CMAKE_CXX_STANDARD 23)

# Find packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(SPNG CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)

# Set C++ standard globally before adding subdirectories
set(CMAKE_CXX_STANDARD 23)          
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add FastNoise2 as subdirectory
add_subdirectory(third_party/FastNoise2)

# Add ImGui as subdirectory
add_subdirectory(third_party/imgui)

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
include_directories(third_party/imgui)
include_directories(third_party/imgui/backends)

# Add executable
add_executable(Zerith
    main.cpp
        src/world/chunk.cpp
        src/rendering/chunk_mesh_generator.cpp
        src/rendering/voxel_ao.cpp
        src/world/chunk_manager.cpp
        src/rendering/texture_array.cpp
        src/rendering/texture_atlas.cpp
        src/utils/logger.cpp
        src/core/aabb.cpp
        src/core/player.cpp
        src/world/terrain_generator.cpp
        src/core/raycast.cpp
        src/rendering/indirect_draw.cpp
        src/blocks/block_properties.cpp
        src/blocks/block_face_bounds.cpp
        src/world/sparse_octree.cpp
        src/world/chunk_octree.cpp
        src/utils/translation_manager.cpp
        src/serialization/binary_chunk_data.cpp
        src/serialization/binary_mesh_converter.cpp
        src/utils/thread_pool.cpp
        src/blocks/blocks.cpp
        src/blocks/block_behavior.cpp
        src/blocks/water_block.cpp
        src/blocks/fluid_block.cpp
        src/ui/imgui_integration.cpp
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_vulkan.cpp
)

# Add shader dependencies
add_dependencies(Zerith 
    task_shader 
    mesh_shader 
    fragment_shader 
    aabb_mesh_shader 
    aabb_fragment_shader
)

# Tracy profiler configuration
option(ENABLE_TRACY "Enable Tracy profiler" OFF)
if(ENABLE_TRACY)
    target_compile_definitions(Zerith PRIVATE TRACY_ENABLE)
endif()

if(MSVC)
    target_compile_options(Zerith PRIVATE
            # Always enable AVX2 SIMD instructions
            /arch:AVX2

            # Debug config: no optimization, runtime checks
            $<$<CONFIG:Debug>:/Od /RTC1>

            # Release config: optimize aggressively + intrinsics
            $<$<CONFIG:Release>:/O2 /Oi>
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
target_link_libraries(Zerith PRIVATE ${Vulkan_LIBRARIES} glfw $<IF:$<TARGET_EXISTS:spng::spng>,spng::spng,spng::spng_static> FastNoise2 spdlog::spdlog Tracy::TracyClient imgui)

# Create directories for shader output and assets
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)

# Copy assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
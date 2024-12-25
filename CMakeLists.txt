cmake_minimum_required(VERSION 3.29)
project(Zerith)

set(CMAKE_CXX_STANDARD 23)

# Find Vulkan
find_package(Vulkan REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.hpp")

add_executable(Zerith ${SOURCES} ${HEADERS})

# Link Vulkan
target_link_libraries(Zerith Vulkan::Vulkan)

# Shader Compilation
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(BUILD_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)

    # Output filename must be unique
    set(SHADER_OUTPUT ${BUILD_SHADER_DIR}/${SHADER_NAME}${SHADER_EXT}.spv)

    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_SHADER_DIR}
            COMMAND glslangValidator -V ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER} to ${SHADER_OUTPUT}"
    )

    list(APPEND SPV_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(CompileShaders ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Forcing shader recompilation"
        DEPENDS ${SPV_SHADERS}
)

# Ensure the shaders are ready before the main build
add_dependencies(Zerith CompileShaders)

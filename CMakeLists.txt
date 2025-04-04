cmake_minimum_required(VERSION 3.30)
project(Zerith)

set(CMAKE_CXX_STANDARD 23)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Find GLM package
find_package(glm REQUIRED)

# Find nlohmann_json package
find_package(nlohmann_json REQUIRED)

# Find stb package
find_package(Stb REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Header files
file(GLOB_RECURSE HEADERS src/*.hpp)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set the src directory as the project's include directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
    ${Stb_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    glm::glm
    XInput
    nlohmann_json::nlohmann_json
)

# Copy shaders directory to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)
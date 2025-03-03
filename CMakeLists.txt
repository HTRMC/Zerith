cmake_minimum_required(VERSION 3.30)
project(Zerith)

set(CMAKE_CXX_STANDARD 23)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Find GLM package
find_package(glm REQUIRED)

# Find nlohmann_json package
find_package(nlohmann_json REQUIRED)

# Source files
set(SOURCES
        src/main.cpp
        src/VulkanApp.cpp
        src/ModelLoader.cpp
)

# Header files
set(HEADERS
        src/VulkanApp.hpp
        src/ModelLoader.hpp
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})

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

# Create resources directory in build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

# Copy model resources to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)
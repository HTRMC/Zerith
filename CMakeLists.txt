cmake_minimum_required(VERSION 3.30)
project(Zerith)

set(CMAKE_CXX_STANDARD 23)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Find GLM package
find_package(glm REQUIRED)

# Source files
set(SOURCES
        src/main.cpp
        src/VulkanApp.cpp
)

# Header files
set(HEADERS
        src/VulkanApp.hpp
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
)

# Copy shaders directory to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)
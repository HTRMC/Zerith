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
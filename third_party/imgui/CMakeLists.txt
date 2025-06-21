cmake_minimum_required(VERSION 3.10)

project(imgui)

set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

# Set C++ standard
target_compile_features(imgui PUBLIC cxx_std_11)

# Platform specific settings
if(WIN32)
    target_compile_definitions(imgui PUBLIC IMGUI_API=)
endif()
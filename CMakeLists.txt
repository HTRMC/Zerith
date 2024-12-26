cmake_minimum_required(VERSION 3.28)
project(Zerith VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Zerith)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Zerith)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Zerith)

# Find Vulkan
find_package(Vulkan REQUIRED)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.hpp")

# Copy resources to output directory
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(WIN32)
    enable_language(RC)
    set(WIN_RESOURCES ${CMAKE_SOURCE_DIR}/resources/resources.rc)
endif()

if(WIN32)
    add_executable(Zerith WIN32 ${SOURCES} ${HEADERS} ${WIN_RESOURCES})
else()
    add_executable(Zerith ${SOURCES} ${HEADERS} ${WIN_RESOURCES})
endif()

if(UNIX)
    target_link_libraries(Zerith Vulkan::Vulkan xcb)
else()
    target_link_libraries(Zerith Vulkan::Vulkan)
endif()

# Shader Compilation
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(BUILD_SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

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

#=======================================================
# Installation Configuration
#=======================================================

# Install binary to /usr/bin
install(TARGETS Zerith
        RUNTIME DESTINATION bin
        RENAME zerith)  # Lowercase for Linux convention

# Install desktop file
install(FILES "${CMAKE_SOURCE_DIR}/resources/zerith.desktop"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        DESTINATION share/applications)

# Install icon
install(FILES "${CMAKE_SOURCE_DIR}/resources/Zerith.png"
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME zerith.png)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
        DESTINATION bin
        COMPONENT application)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
        DESTINATION bin
        COMPONENT application)

#=======================================================
# CPack Configuration
#=======================================================

set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/Zerith")

set(CPACK_PACKAGE_NAME "Zerith")
set(CPACK_PACKAGE_VENDOR "HTRMC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Zerith - Vulkan Application")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Zerith")

if(WIN32)
    # Windows NSIS configuration
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "Zerith")
    set(CPACK_NSIS_DISPLAY_NAME "Zerith")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/x256.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/x256.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\Zerith.exe")
    set(CPACK_NSIS_HELP_LINK "https://github.com/HTRMC/Zerith")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/HTRMC/Zerith")
    set(CPACK_NSIS_CONTACT "support@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

    # Create Start Menu shortcuts
    set(CPACK_NSIS_MENU_LINKS
            "bin/Zerith.exe" "Zerith"
            "https://github.com/HTRMC/Zerith" "Zerith Website")

    # Desktop shortcut
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
            "CreateShortCut '$DESKTOP\\\\Zerith.lnk' '$INSTDIR\\\\bin\\\\Zerith.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
            "Delete '$DESKTOP\\\\Zerith.lnk'")
else()
    # Linux configuration
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libvulkan1, libxcb1")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "HTRMC")
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libvulkan1")

    configure_file("${CMAKE_SOURCE_DIR}/resources/zerith.desktop"
            "${CMAKE_BINARY_DIR}/zerith.desktop" @ONLY)
    install(FILES "${CMAKE_BINARY_DIR}/zerith.desktop"
            DESTINATION share/applications)

    install(FILES "${CMAKE_SOURCE_DIR}/resources/Zerith.png"
            DESTINATION share/icons/hicolor/256x256/apps
            RENAME zerith.png
            COMPONENT application)
endif()

include(CPack)
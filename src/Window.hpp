// Window.hpp
#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#else
#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <cstring>
#endif

#include <string>
#include <vulkan/vulkan.h>

class Window {
public:
    Window(int width, int height);
    ~Window();

    bool shouldClose() const { return windowShouldClose; }
    VkSurfaceKHR createSurface(VkInstance instance);
    void pollEvents();
    void setIcon(const std::string& iconPath);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    bool windowShouldClose = false;

#ifdef _WIN32
    HINSTANCE hInstance;
    HWND hwnd;
    HICON hIcon;
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#else
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    void setWindowIcon(const uint32_t* iconData, int width, int height);
#endif
};

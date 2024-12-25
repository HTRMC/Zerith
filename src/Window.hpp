// Window.hpp
#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#else
#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#endif

#include <vulkan/vulkan.h>

class Window {
public:
    Window(int width, int height);
    ~Window();

    bool shouldClose() const { return windowShouldClose; }
    VkSurfaceKHR createSurface(VkInstance instance);
    void pollEvents();

private:
    int width;
    int height;
    bool windowShouldClose = false;

#ifdef _WIN32
    HINSTANCE hInstance;
    HWND hwnd;
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#else
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
#endif
};

// Window.hpp
#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#undef min
#undef max
#include <windowsx.h>
#else
#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <cstring>
#endif

#include <string>
#include <vulkan/vulkan.h>

#include "InputManager.hpp"

class Window {
private:
    int width;
    int height;
    bool windowShouldClose = false;
    InputManager inputManager;
    bool isCenteringCursor = false;
    POINT lastCursorPos = {0, 0};

public:
    Window(int width, int height);
    ~Window();

    bool shouldClose() const { return windowShouldClose; }
    VkSurfaceKHR createSurface(VkInstance instance);
    void pollEvents();
    void setIcon(const std::string& iconPath);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isKeyPressed(KeyCode key) const { return inputManager.isKeyPressed(key); }
    bool isKeyHeld(KeyCode key) const { return inputManager.isKeyHeld(key); }
    float getMouseDeltaX() const { return inputManager.getMouseDeltaX(); }
    float getMouseDeltaY() const { return inputManager.getMouseDeltaY(); }
    void resetMouseDeltas() { inputManager.resetMouseDeltas(); }
    void setCaptureMouse(bool capture);
    void centerCursor();

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

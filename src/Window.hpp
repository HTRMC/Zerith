// Window.hpp
#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#include <windowsx.h>
#else
#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <cstring>
#endif

#include <string>
#include <vulkan/vulkan.h>

class Window {
private:
    int width;
    int height;
    bool windowShouldClose = false;

    struct {
        bool w = false;
        bool a = false;
        bool s = false;
        bool d = false;
        bool shift = false;
        bool space = false;
    } keys;

    struct {
        float x = 0.0f;
        float y = 0.0f;
        float deltaX = 0.0f;
        float deltaY = 0.0f;
        bool firstMouse = true;
    } mouse;

public:
    Window(int width, int height);
    ~Window();

    bool shouldClose() const { return windowShouldClose; }
    VkSurfaceKHR createSurface(VkInstance instance);
    void pollEvents();
    void setIcon(const std::string& iconPath);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isKeyPressed(char key) const {
        switch(key) {
            case 'W': return keys.w;
            case 'A': return keys.a;
            case 'S': return keys.s;
            case 'D': return keys.d;
            default: return false;
        }
    }

    bool isShiftPressed() const { return keys.shift; }
    bool isSpacePressed() const { return keys.space; }

    float getMouseDeltaX() const { return mouse.deltaX; }
    float getMouseDeltaY() const { return mouse.deltaY; }
    void resetMouseDeltas() { mouse.deltaX = 0.0f; mouse.deltaY = 0.0f; }
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

// Window.cpp
#include "Window.hpp"

#ifdef _WIN32
void Window::setCaptureMouse(bool capture) {
    if (capture) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        ClientToScreen(hwnd, (POINT*)&rect.left);
        ClientToScreen(hwnd, (POINT*)&rect.right);
        ClipCursor(&rect);
        ShowCursor(FALSE);
        centerCursor();
    } else {
        ClipCursor(NULL);
        ShowCursor(TRUE);
    }
}

void Window::centerCursor() {
    RECT rect;
    GetClientRect(hwnd, &rect);
    POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
    ClientToScreen(hwnd, &center);
    SetCursorPos(center.x, center.y);
    mouse.x = static_cast<float>(rect.right - rect.left) / 2;
    mouse.y = static_cast<float>(rect.bottom - rect.top) / 2;
}

Window::Window(int width, int height) : width(width), height(height) {
    hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ZerithWindow";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassEx(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    hwnd = CreateWindowEx(
        0,
        "ZerithWindow",
        "Zerith",
        WS_OVERLAPPEDWINDOW,
        posX, posY,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    ShowWindow(hwnd, SW_SHOW);
}

void Window::setIcon(const std::string& iconPath) {
    // Load icon from file
    hIcon = (HICON)LoadImage(
        NULL,
        iconPath.c_str(),
        IMAGE_ICON,
        256, 256,  // Use actual size
        LR_LOADFROMFILE
    );

    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
}


Window::~Window() {
    if (hIcon) {
        DestroyIcon(hIcon);
    }
    DestroyWindow(hwnd);
    UnregisterClass("ZerithWindow", hInstance);
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_CREATE: {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            return 0;
        }

        case WM_KEYDOWN:
            if (window) {
                switch (wParam) {
                    case 'W': window->keys.w = true; break;
                    case 'A': window->keys.a = true; break;
                    case 'S': window->keys.s = true; break;
                    case 'D': window->keys.d = true; break;
                    case VK_SHIFT: window->keys.shift = true; break;
                    case VK_SPACE: window->keys.space = true; break;
                }
            }
        return 0;

        case WM_KEYUP:
            if (window) {
                switch (wParam) {
                    case 'W': window->keys.w = false; break;
                    case 'A': window->keys.a = false; break;
                    case 'S': window->keys.s = false; break;
                    case 'D': window->keys.d = false; break;
                    case VK_SHIFT: window->keys.shift = false; break;
                    case VK_SPACE: window->keys.space = false; break;
                }
            }
        return 0;

        case WM_MOUSEMOVE: {
            if (window) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);

                if (window->mouse.firstMouse) {
                    window->mouse.x = static_cast<float>(x);
                    window->mouse.y = static_cast<float>(y);
                    window->mouse.firstMouse = false;
                }

                window->mouse.deltaX = static_cast<float>(x) - window->mouse.x;
                window->mouse.deltaY = static_cast<float>(y) - window->mouse.y;
                window->mouse.x = static_cast<float>(x);
                window->mouse.y = static_cast<float>(y);
                window->centerCursor();
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Window::pollEvents() {
    MSG msg{};
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            windowShouldClose = true;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

VkSurfaceKHR Window::createSurface(VkInstance instance) {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = hwnd;
    createInfo.hinstance = hInstance;

    VkSurfaceKHR surface;
    vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
    return surface;
}

#else
void Window::setCaptureMouse(bool capture) {
    // For XCB we would need to handle mouse capture differently
    // This is a simplified version
    xcb_grab_pointer(connection, 0, window,
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
        window, XCB_NONE, XCB_CURRENT_TIME);
    if (capture) {
        centerCursor();
    }
}

void Window::centerCursor() {
    xcb_warp_pointer(connection, XCB_NONE, window,
        0, 0, 0, 0,
        width / 2, height / 2);
    xcb_flush(connection);
    mouse.x = static_cast<float>(width) / 2;
    mouse.y = static_cast<float>(height) / 2;
}

Window::Window(int width, int height) : width(width), height(height) {
    connection = xcb_connect(nullptr, nullptr);
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    window = xcb_generate_id(connection);

    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[2] = {
        screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS
    };

    int posX = (screen->width_in_pixels - width) / 2;
    int posY = (screen->height_in_pixels - height) / 2;

    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        window,
        screen->root,
        posX, posY,
        width, height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        value_mask,
        value_list
    );

    xcb_map_window(connection, window);
    xcb_flush(connection);
}

void Window::setWindowIcon(const uint32_t* iconData, int width, int height) {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
        connection,
        0,
        strlen("_NET_WM_ICON"),
        "_NET_WM_ICON"
    );

    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(
        connection,
        cookie,
        nullptr
    );

    if (reply) {
        xcb_change_property(
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            reply->atom,
            XCB_ATOM_CARDINAL,
            32,
            width * height + 2,
            iconData
        );

        free(reply);
    }
}

void Window::setIcon(const std::string& iconPath) {
    // TODO: Implement icon setting for XCB
}

Window::~Window() {
    xcb_disconnect(connection);
}

void Window::pollEvents() {
    xcb_generic_event_t* event;
    while ((event = xcb_poll_for_event(connection))) {
        switch (event->response_type & ~0x80) {
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* key_event = (xcb_key_press_event_t*)event;
                switch (key_event->detail) {
                    case 25: keys.w = true; break; // W
                    case 38: keys.a = true; break; // A
                    case 39: keys.s = true; break; // S
                    case 40: keys.d = true; break; // D
                    case 50: keys.shift = true; break; // Left Shift
                    case 65: keys.space = true; break; // Space
                }
                break;
            }
            case XCB_KEY_RELEASE: {
                xcb_key_release_event_t* key_event = (xcb_key_release_event_t*)event;
                switch (key_event->detail) {
                    case 25: keys.w = false; break; // W
                    case 38: keys.a = false; break; // A
                    case 39: keys.s = false; break; // S
                    case 40: keys.d = false; break; // D
                    case 50: keys.shift = false; break; // Left Shift
                    case 65: keys.space = false; break; // Space
                }
                break;
            }
            case XCB_MOTION_NOTIFY: {
                xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
                if (mouse.firstMouse) {
                    mouse.x = static_cast<float>(motion->event_x);
                    mouse.y = static_cast<float>(motion->event_y);
                    mouse.firstMouse = false;
                }

                mouse.deltaX = static_cast<float>(motion->event_x) - mouse.x;
                mouse.deltaY = static_cast<float>(motion->event_y) - mouse.y;

                centerCursor();
                break;
            }
            case XCB_CLIENT_MESSAGE:
                windowShouldClose = true;
                break;
        }
        free(event);
    }
}

VkSurfaceKHR Window::createSurface(VkInstance instance) {
    VkXcbSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.connection = connection;
    createInfo.window = window;

    VkSurfaceKHR surface;
    vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &surface);
    return surface;
}
#endif
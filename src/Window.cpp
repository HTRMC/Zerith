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
    inputManager.setMousePosition(static_cast<float>(rect.right - rect.left) / 2,
                                static_cast<float>(rect.bottom - rect.top) / 2);
}

Window::Window(int width, int height) : width(width), height(height), inputManager() {
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
        windowWidth, windowHeight,
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
        case WM_SYSKEYDOWN:
            if (window) {
                // Let Alt+F4 pass through
                if (wParam == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) {
                    return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }

                // Handle Shift keys specifically
                if (wParam == VK_SHIFT) {
                    // Use GetKeyState to determine which shift key was pressed
                    if (GetKeyState(VK_LSHIFT) < 0) {
                        window->inputManager.updateKeyState(KeyCode::SHIFT_LEFT, true);
                    }
                    if (GetKeyState(VK_RSHIFT) < 0) {
                        window->inputManager.updateKeyState(KeyCode::SHIFT_RIGHT, true);
                    }
                    return 0;
                }
                KeyCode keyCode = InputManager::windowsKeyCodeToKeyCode(wParam);
                window->inputManager.updateKeyState(keyCode, true);
            }
            return 0;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (window) {
                // Handle Shift keys specifically
                if (wParam == VK_SHIFT) {
                    // Check both shift keys and update their states
                    if (!(GetKeyState(VK_LSHIFT) & 0x8000)) {
                        window->inputManager.updateKeyState(KeyCode::SHIFT_LEFT, false);
                    }
                    if (!(GetKeyState(VK_RSHIFT) & 0x8000)) {
                        window->inputManager.updateKeyState(KeyCode::SHIFT_RIGHT, false);
                    }
                    return 0;
                }
                KeyCode keyCode = InputManager::windowsKeyCodeToKeyCode(wParam);
                window->inputManager.updateKeyState(keyCode, false);
            }
            return 0;

        case WM_LBUTTONDOWN:
            if (window) {
                window->inputManager.updateKeyState(KeyCode::MOUSE_LEFT, true);
            }
            return 0;

        case WM_LBUTTONUP:
            if (window) {
                window->inputManager.updateKeyState(KeyCode::MOUSE_LEFT, false);
            }
            return 0;

        case WM_RBUTTONDOWN:
            if (window) {
                window->inputManager.updateKeyState(KeyCode::MOUSE_RIGHT, true);
            }
            return 0;

        case WM_RBUTTONUP:
            if (window) {
                window->inputManager.updateKeyState(KeyCode::MOUSE_RIGHT, false);
            }
            return 0;

        case WM_MBUTTONDOWN:
            if (window) {
                window->inputManager.updateKeyState(KeyCode::MOUSE_MIDDLE, true);
            }
            return 0;

        case WM_MBUTTONUP:
            if (window) {
                window->inputManager.updateKeyState(KeyCode::MOUSE_MIDDLE, false);
            }
            return 0;

        case WM_MOUSEMOVE: {
            if (window) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                float newX = static_cast<float>(x);
                float newY = static_cast<float>(y);
                float oldX = window->inputManager.getMouseX();
                float oldY = window->inputManager.getMouseY();

                if (oldX != 0.0f || oldY != 0.0f) {
                    window->inputManager.setMouseDelta(newX - oldX, newY - oldY);
                }

                window->inputManager.setMousePosition(newX, newY);
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
    // Update input manager state
    inputManager.update();
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
    if (capture) {
        // Set up XCB pointer grab
        xcb_grab_pointer(
            connection,
            0,  // owner_events
            window,
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION,
            XCB_GRAB_MODE_ASYNC,
            XCB_GRAB_MODE_ASYNC,
            window,
            XCB_NONE,
            XCB_CURRENT_TIME
        );
        centerCursor();
    } else {
        xcb_ungrab_pointer(connection, XCB_CURRENT_TIME);
    }
}

void Window::centerCursor() {
    xcb_warp_pointer(connection, XCB_NONE, window,
        0, 0, 0, 0,
        width / 2, height / 2);
    xcb_flush(connection);
    inputManager.setMousePosition(static_cast<float>(width) / 2,
                                static_cast<float>(height) / 2);
}

Window::Window(int width, int height) : width(width), height(height), inputManager() {
    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection)) {
        throw std::runtime_error("Failed to connect to X server");
    }

    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    window = xcb_generate_id(connection);

    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[2] = {
        screen->black_pixel,
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_EXPOSURE
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

    // Set window title
    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE,
        window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        6,
        "Zerith"
    );

    // Set up WM_DELETE_WINDOW protocol
    xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(
        connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(
        connection, 0, 16, "WM_DELETE_WINDOW");

    xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(
        connection, protocols_cookie, nullptr);
    xcb_intern_atom_reply_t* delete_reply = xcb_intern_atom_reply(
        connection, delete_cookie, nullptr);

    if (protocols_reply && delete_reply) {
        xcb_change_property(
            connection,
            XCB_PROP_MODE_REPLACE,
            window,
            protocols_reply->atom,
            XCB_ATOM_ATOM,
            32,
            1,
            &delete_reply->atom
        );
    }

    free(protocols_reply);
    free(delete_reply);

    xcb_map_window(connection, window);
    xcb_flush(connection);
}

Window::~Window() {
    xcb_disconnect(connection);
}

void Window::setIcon(const std::string& iconPath) {
    // XCB icon setting would go here
    // This is more complex in XCB and might require additional libraries
    // like Imlib2 to load the icon file
}

void Window::pollEvents() {
    xcb_generic_event_t* event;
    while ((event = xcb_poll_for_event(connection))) {
        switch (event->response_type & ~0x80) {
            case XCB_KEY_PRESS: {
                auto* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(event);
                KeyCode keyCode = InputManager::xcbKeyCodeToKeyCode(keyEvent->detail);
                inputManager.updateKeyState(keyCode, true);
                break;
            }

            case XCB_KEY_RELEASE: {
                auto* keyEvent = reinterpret_cast<xcb_key_release_event_t*>(event);
                KeyCode keyCode = InputManager::xcbKeyCodeToKeyCode(keyEvent->detail);
                inputManager.updateKeyState(keyCode, false);
                break;
            }

            case XCB_BUTTON_PRESS: {
                auto* buttonEvent = reinterpret_cast<xcb_button_press_event_t*>(event);
                switch (buttonEvent->detail) {
                    case XCB_BUTTON_1: inputManager.updateKeyState(KeyCode::MOUSE_LEFT, true); break;
                    case XCB_BUTTON_2: inputManager.updateKeyState(KeyCode::MOUSE_MIDDLE, true); break;
                    case XCB_BUTTON_3: inputManager.updateKeyState(KeyCode::MOUSE_RIGHT, true); break;
                    case XCB_BUTTON_4: inputManager.updateKeyState(KeyCode::MOUSE_4, true); break;
                    case XCB_BUTTON_5: inputManager.updateKeyState(KeyCode::MOUSE_5, true); break;
                }
                break;
            }

            case XCB_BUTTON_RELEASE: {
                auto* buttonEvent = reinterpret_cast<xcb_button_release_event_t*>(event);
                switch (buttonEvent->detail) {
                    case XCB_BUTTON_1: inputManager.updateKeyState(KeyCode::MOUSE_LEFT, false); break;
                    case XCB_BUTTON_2: inputManager.updateKeyState(KeyCode::MOUSE_MIDDLE, false); break;
                    case XCB_BUTTON_3: inputManager.updateKeyState(KeyCode::MOUSE_RIGHT, false); break;
                    case XCB_BUTTON_4: inputManager.updateKeyState(KeyCode::MOUSE_4, false); break;
                    case XCB_BUTTON_5: inputManager.updateKeyState(KeyCode::MOUSE_5, false); break;
                }
                break;
            }

            case XCB_MOTION_NOTIFY: {
                auto* motionEvent = reinterpret_cast<xcb_motion_notify_event_t*>(event);
                float newX = static_cast<float>(motionEvent->event_x);
                float newY = static_cast<float>(motionEvent->event_y);
                float oldX = inputManager.getMouseX();
                float oldY = inputManager.getMouseY();

                if (oldX != 0.0f || oldY != 0.0f) {
                    inputManager.setMouseDelta(newX - oldX, newY - oldY);
                }

                inputManager.setMousePosition(newX, newY);
                centerCursor();
                break;
            }

            case XCB_CLIENT_MESSAGE:
                windowShouldClose = true;
                break;

            case XCB_DESTROY_NOTIFY:
                windowShouldClose = true;
                break;
        }
        free(event);
    }

    // Update input manager state
    inputManager.update();
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

#endif
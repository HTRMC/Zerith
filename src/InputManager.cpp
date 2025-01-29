// InputManager.cpp
#include "InputManager.hpp"

#include "Application.hpp"

InputManager::InputManager()
    : mouseX(0.0f), mouseY(0.0f), mouseDeltaX(0.0f), mouseDeltaY(0.0f) {
    // Initialize all keys to released state
    keyStates.fill(KeyState::RELEASED);
    prevKeyStates.fill(KeyState::RELEASED);
    initializePlatformKeyMaps();
}

void InputManager::updateKeyState(KeyCode key, bool pressed) {
    KeyState& state = keyStates[static_cast<size_t>(key)];
    KeyState prevState = state;

    if (pressed) {
        if (prevState == KeyState::RELEASED || prevState == KeyState::JUST_RELEASED) {
            state = KeyState::JUST_PRESSED;
        } else {
            state = KeyState::HELD;
        }
    } else {
        if (prevState == KeyState::JUST_PRESSED || prevState == KeyState::HELD) {
            state = KeyState::JUST_RELEASED;
        } else {
            state = KeyState::RELEASED;
        }
    }
}

bool InputManager::isKeyPressed(KeyCode key) const {
    KeyState state = keyStates[static_cast<size_t>(key)];
    return state == KeyState::JUST_PRESSED || state == KeyState::HELD;
}

bool InputManager::isKeyJustPressed(KeyCode key) const {
    return keyStates[static_cast<size_t>(key)] == KeyState::JUST_PRESSED;
}

bool InputManager::isKeyHeld(KeyCode key) const {
    return keyStates[static_cast<size_t>(key)] == KeyState::HELD;
}

bool InputManager::isKeyJustReleased(KeyCode key) const {
    return keyStates[static_cast<size_t>(key)] == KeyState::JUST_RELEASED;
}

void InputManager::setMousePosition(float x, float y) {
    mouseX = x;
    mouseY = y;
}

void InputManager::setMouseDelta(float deltaX, float deltaY) {
    mouseDeltaX = deltaX;
    mouseDeltaY = deltaY;
}

void InputManager::update() {
    // Update previous key states
    prevKeyStates = keyStates;

    // Update key states
    for (size_t i = 0; i < static_cast<size_t>(KeyCode::KEY_COUNT); i++) {
        if (keyStates[i] == KeyState::JUST_PRESSED) {
            keyStates[i] = KeyState::HELD;
        } else if (keyStates[i] == KeyState::JUST_RELEASED) {
            keyStates[i] = KeyState::RELEASED;
        }
    }
}

#ifdef _WIN32
std::unordered_map<unsigned int, KeyCode> InputManager::windowsKeyMap;

KeyCode InputManager::windowsKeyCodeToKeyCode(unsigned int windowsKeyCode) {
    auto it = windowsKeyMap.find(windowsKeyCode);
    if (it != windowsKeyMap.end()) {
        return it->second;
    }
    // Return a default value if key not found
    return KeyCode::KEY_COUNT;
}

void InputManager::initializePlatformKeyMaps() {
    // Windows Virtual Key Codes (VK_*)
    windowsKeyMap[0x41] = KeyCode::A; // A key
    windowsKeyMap[0x42] = KeyCode::B;
    windowsKeyMap[0x43] = KeyCode::C;
    windowsKeyMap[0x44] = KeyCode::D;
    windowsKeyMap[0x45] = KeyCode::E;
    windowsKeyMap[0x46] = KeyCode::F;
    windowsKeyMap[0x47] = KeyCode::G;
    windowsKeyMap[0x48] = KeyCode::H;
    windowsKeyMap[0x49] = KeyCode::I;
    windowsKeyMap[0x4A] = KeyCode::J;
    windowsKeyMap[0x4B] = KeyCode::K;
    windowsKeyMap[0x4C] = KeyCode::L;
    windowsKeyMap[0x4D] = KeyCode::M;
    windowsKeyMap[0x4E] = KeyCode::N;
    windowsKeyMap[0x4F] = KeyCode::O;
    windowsKeyMap[0x50] = KeyCode::P;
    windowsKeyMap[0x51] = KeyCode::Q;
    windowsKeyMap[0x52] = KeyCode::R;
    windowsKeyMap[0x53] = KeyCode::S;
    windowsKeyMap[0x54] = KeyCode::T;
    windowsKeyMap[0x55] = KeyCode::U;
    windowsKeyMap[0x56] = KeyCode::V;
    windowsKeyMap[0x57] = KeyCode::W;
    windowsKeyMap[0x58] = KeyCode::X;
    windowsKeyMap[0x59] = KeyCode::Y;
    windowsKeyMap[0x5A] = KeyCode::Z;

    // Numbers
    windowsKeyMap[0x30] = KeyCode::NUM_0;
    windowsKeyMap[0x31] = KeyCode::NUM_1;
    windowsKeyMap[0x32] = KeyCode::NUM_2;
    windowsKeyMap[0x33] = KeyCode::NUM_3;
    windowsKeyMap[0x34] = KeyCode::NUM_4;
    windowsKeyMap[0x35] = KeyCode::NUM_5;
    windowsKeyMap[0x36] = KeyCode::NUM_6;
    windowsKeyMap[0x37] = KeyCode::NUM_7;
    windowsKeyMap[0x38] = KeyCode::NUM_8;
    windowsKeyMap[0x39] = KeyCode::NUM_9;

    // Function keys
    windowsKeyMap[VK_F1] = KeyCode::F1;
    windowsKeyMap[VK_F2] = KeyCode::F2;
    windowsKeyMap[VK_F3] = KeyCode::F3;
    windowsKeyMap[VK_F4] = KeyCode::F4;
    windowsKeyMap[VK_F5] = KeyCode::F5;
    windowsKeyMap[VK_F6] = KeyCode::F6;
    windowsKeyMap[VK_F7] = KeyCode::F7;
    windowsKeyMap[VK_F8] = KeyCode::F8;
    windowsKeyMap[VK_F9] = KeyCode::F9;
    windowsKeyMap[VK_F10] = KeyCode::F10;
    windowsKeyMap[VK_F11] = KeyCode::F11;
    windowsKeyMap[VK_F12] = KeyCode::F12;

    // Special keys
    windowsKeyMap[VK_ESCAPE] = KeyCode::ESCAPE;
    windowsKeyMap[VK_TAB] = KeyCode::TAB;
    windowsKeyMap[VK_CAPITAL] = KeyCode::CAPS_LOCK;
    windowsKeyMap[VK_LSHIFT] = KeyCode::SHIFT_LEFT;
    windowsKeyMap[VK_RSHIFT] = KeyCode::SHIFT_RIGHT;
    windowsKeyMap[VK_LCONTROL] = KeyCode::CONTROL_LEFT;
    windowsKeyMap[VK_RCONTROL] = KeyCode::CONTROL_RIGHT;
    windowsKeyMap[VK_LMENU] = KeyCode::ALT_LEFT;
    windowsKeyMap[VK_RMENU] = KeyCode::ALT_RIGHT;
    windowsKeyMap[VK_SPACE] = KeyCode::SPACE;
    windowsKeyMap[VK_RETURN] = KeyCode::ENTER;
    windowsKeyMap[VK_BACK] = KeyCode::BACKSPACE;
    windowsKeyMap[VK_DELETE] = KeyCode::DELETE_KEY;
    windowsKeyMap[VK_INSERT] = KeyCode::INSERT;

    // Arrow keys
    windowsKeyMap[VK_UP] = KeyCode::UP;
    windowsKeyMap[VK_DOWN] = KeyCode::DOWN;
    windowsKeyMap[VK_LEFT] = KeyCode::LEFT;
    windowsKeyMap[VK_RIGHT] = KeyCode::RIGHT;

    // Navigation keys
    windowsKeyMap[VK_HOME] = KeyCode::HOME;
    windowsKeyMap[VK_END] = KeyCode::END;
    windowsKeyMap[VK_PRIOR] = KeyCode::PAGE_UP;
    windowsKeyMap[VK_NEXT] = KeyCode::PAGE_DOWN;

    // Numpad
    windowsKeyMap[VK_NUMPAD0] = KeyCode::NUMPAD_0;
    windowsKeyMap[VK_NUMPAD1] = KeyCode::NUMPAD_1;
    windowsKeyMap[VK_NUMPAD2] = KeyCode::NUMPAD_2;
    windowsKeyMap[VK_NUMPAD3] = KeyCode::NUMPAD_3;
    windowsKeyMap[VK_NUMPAD4] = KeyCode::NUMPAD_4;
    windowsKeyMap[VK_NUMPAD5] = KeyCode::NUMPAD_5;
    windowsKeyMap[VK_NUMPAD6] = KeyCode::NUMPAD_6;
    windowsKeyMap[VK_NUMPAD7] = KeyCode::NUMPAD_7;
    windowsKeyMap[VK_NUMPAD8] = KeyCode::NUMPAD_8;
    windowsKeyMap[VK_NUMPAD9] = KeyCode::NUMPAD_9;
    windowsKeyMap[VK_MULTIPLY] = KeyCode::NUMPAD_MULTIPLY;
    windowsKeyMap[VK_ADD] = KeyCode::NUMPAD_ADD;
    windowsKeyMap[VK_SUBTRACT] = KeyCode::NUMPAD_SUBTRACT;
    windowsKeyMap[VK_DECIMAL] = KeyCode::NUMPAD_DECIMAL;
    windowsKeyMap[VK_DIVIDE] = KeyCode::NUMPAD_DIVIDE;

    // Mouse buttons
    windowsKeyMap[VK_LBUTTON] = KeyCode::MOUSE_LEFT;
    windowsKeyMap[VK_RBUTTON] = KeyCode::MOUSE_RIGHT;
    windowsKeyMap[VK_MBUTTON] = KeyCode::MOUSE_MIDDLE;
    windowsKeyMap[VK_XBUTTON1] = KeyCode::MOUSE_4;
    windowsKeyMap[VK_XBUTTON2] = KeyCode::MOUSE_5;
}

#else
std::unordered_map<unsigned int, KeyCode> InputManager::xcbKeyMap;

KeyCode InputManager::xcbKeyCodeToKeyCode(unsigned int xcbKeyCode) {
    auto it = xcbKeyMap.find(xcbKeyCode);
    if (it != xcbKeyMap.end()) {
        return it->second;
    }
    // Return a default value if key not found
    return KeyCode::KEY_COUNT;
}

void InputManager::initializePlatformKeyMaps() {
    // XCB key codes - You'll need to map these correctly for your system
    // Letter keys
    xcbKeyMap[38] = KeyCode::A;  // a
    xcbKeyMap[56] = KeyCode::B;  // b
    xcbKeyMap[54] = KeyCode::C;  // c
    xcbKeyMap[40] = KeyCode::D;  // d
    xcbKeyMap[26] = KeyCode::E;  // e
    xcbKeyMap[41] = KeyCode::F;  // f
    xcbKeyMap[42] = KeyCode::G;  // g
    xcbKeyMap[43] = KeyCode::H;  // h
    xcbKeyMap[31] = KeyCode::I;  // i
    xcbKeyMap[44] = KeyCode::J;  // j
    xcbKeyMap[45] = KeyCode::K;  // k
    xcbKeyMap[46] = KeyCode::L;  // l
    xcbKeyMap[58] = KeyCode::M;  // m
    xcbKeyMap[57] = KeyCode::N;  // n
    xcbKeyMap[32] = KeyCode::O;  // o
    xcbKeyMap[33] = KeyCode::P;  // p
    xcbKeyMap[24] = KeyCode::Q;  // q
    xcbKeyMap[27] = KeyCode::R;  // r
    xcbKeyMap[39] = KeyCode::S;  // s
    xcbKeyMap[28] = KeyCode::T;  // t
    xcbKeyMap[30] = KeyCode::U;  // u
    xcbKeyMap[55] = KeyCode::V;  // v
    xcbKeyMap[25] = KeyCode::W;  // w
    xcbKeyMap[53] = KeyCode::X;  // x
    xcbKeyMap[29] = KeyCode::Y;  // y
    xcbKeyMap[52] = KeyCode::Z;  // z

    // Number keys (top row)
    xcbKeyMap[19] = KeyCode::NUM_0;  // 0
    xcbKeyMap[10] = KeyCode::NUM_1;  // 1
    xcbKeyMap[11] = KeyCode::NUM_2;  // 2
    xcbKeyMap[12] = KeyCode::NUM_3;  // 3
    xcbKeyMap[13] = KeyCode::NUM_4;  // 4
    xcbKeyMap[14] = KeyCode::NUM_5;  // 5
    xcbKeyMap[15] = KeyCode::NUM_6;  // 6
    xcbKeyMap[16] = KeyCode::NUM_7;  // 7
    xcbKeyMap[17] = KeyCode::NUM_8;  // 8
    xcbKeyMap[18] = KeyCode::NUM_9;  // 9

    // Function keys
    xcbKeyMap[67] = KeyCode::F1;
    xcbKeyMap[68] = KeyCode::F2;
    xcbKeyMap[69] = KeyCode::F3;
    xcbKeyMap[70] = KeyCode::F4;
    xcbKeyMap[71] = KeyCode::F5;
    xcbKeyMap[72] = KeyCode::F6;
    xcbKeyMap[73] = KeyCode::F7;
    xcbKeyMap[74] = KeyCode::F8;
    xcbKeyMap[75] = KeyCode::F9;
    xcbKeyMap[76] = KeyCode::F10;
    xcbKeyMap[95] = KeyCode::F11;
    xcbKeyMap[96] = KeyCode::F12;

    // Special keys
    xcbKeyMap[9]  = KeyCode::ESCAPE;     // Escape
    xcbKeyMap[23] = KeyCode::TAB;        // Tab
    xcbKeyMap[66] = KeyCode::CAPS_LOCK;  // Caps Lock
    xcbKeyMap[50] = KeyCode::SHIFT_LEFT; // Left Shift
    xcbKeyMap[62] = KeyCode::SHIFT_RIGHT;// Right Shift
    xcbKeyMap[37] = KeyCode::CONTROL_LEFT;  // Left Control
    xcbKeyMap[105] = KeyCode::CONTROL_RIGHT;// Right Control
    xcbKeyMap[64] = KeyCode::ALT_LEFT;   // Left Alt
    xcbKeyMap[108] = KeyCode::ALT_RIGHT; // Right Alt
    xcbKeyMap[65] = KeyCode::SPACE;      // Space
    xcbKeyMap[36] = KeyCode::ENTER;      // Enter
    xcbKeyMap[22] = KeyCode::BACKSPACE;  // Backspace
    xcbKeyMap[119] = KeyCode::DELETE_KEY;    // Delete
    xcbKeyMap[118] = KeyCode::INSERT;    // Insert

    // Arrow keys
    xcbKeyMap[111] = KeyCode::UP;    // Up arrow
    xcbKeyMap[116] = KeyCode::DOWN;  // Down arrow
    xcbKeyMap[113] = KeyCode::LEFT;  // Left arrow
    xcbKeyMap[114] = KeyCode::RIGHT; // Right arrow

    // Navigation keys
    xcbKeyMap[110] = KeyCode::HOME;     // Home
    xcbKeyMap[115] = KeyCode::END;      // End
    xcbKeyMap[112] = KeyCode::PAGE_UP;  // Page Up
    xcbKeyMap[117] = KeyCode::PAGE_DOWN;// Page Down

    // Numpad
    xcbKeyMap[90] = KeyCode::NUMPAD_0;
    xcbKeyMap[87] = KeyCode::NUMPAD_1;
    xcbKeyMap[88] = KeyCode::NUMPAD_2;
    xcbKeyMap[89] = KeyCode::NUMPAD_3;
    xcbKeyMap[83] = KeyCode::NUMPAD_4;
    xcbKeyMap[84] = KeyCode::NUMPAD_5;
    xcbKeyMap[85] = KeyCode::NUMPAD_6;
    xcbKeyMap[79] = KeyCode::NUMPAD_7;
    xcbKeyMap[80] = KeyCode::NUMPAD_8;
    xcbKeyMap[81] = KeyCode::NUMPAD_9;
    xcbKeyMap[63] = KeyCode::NUMPAD_MULTIPLY;
    xcbKeyMap[86] = KeyCode::NUMPAD_ADD;
    xcbKeyMap[82] = KeyCode::NUMPAD_SUBTRACT;
    xcbKeyMap[91] = KeyCode::NUMPAD_DECIMAL;
    xcbKeyMap[106] = KeyCode::NUMPAD_DIVIDE;
    xcbKeyMap[104] = KeyCode::NUMPAD_ENTER;

    // Mouse buttons - Note: These are typically handled differently in XCB
    xcbKeyMap[1] = KeyCode::MOUSE_LEFT;
    xcbKeyMap[3] = KeyCode::MOUSE_RIGHT;
    xcbKeyMap[2] = KeyCode::MOUSE_MIDDLE;
    xcbKeyMap[8] = KeyCode::MOUSE_4;
    xcbKeyMap[9] = KeyCode::MOUSE_5;
}
#endif
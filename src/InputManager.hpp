#pragma once
#include <array>
#include <unordered_map>
#include <string>

// InputManager.hpp
enum class KeyCode {
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    NUM_0, NUM_1, NUM_2, NUM_3, NUM_4,
    NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,
    
    // Function keys
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,
    
    // Special keys
    ESCAPE, TAB, CAPS_LOCK, SHIFT_LEFT, SHIFT_RIGHT,
    CONTROL_LEFT, CONTROL_RIGHT, ALT_LEFT, ALT_RIGHT,
    SPACE, ENTER, BACKSPACE, DELETE_KEY, INSERT,
    
    // Arrow keys
    UP, DOWN, LEFT, RIGHT,
    
    // Navigation
    HOME, END, PAGE_UP, PAGE_DOWN,
    
    // Numpad
    NUMPAD_0, NUMPAD_1, NUMPAD_2, NUMPAD_3, NUMPAD_4,
    NUMPAD_5, NUMPAD_6, NUMPAD_7, NUMPAD_8, NUMPAD_9,
    NUMPAD_MULTIPLY, NUMPAD_ADD, NUMPAD_SUBTRACT,
    NUMPAD_DECIMAL, NUMPAD_DIVIDE, NUMPAD_ENTER,
    
    // Mouse buttons
    MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_4, MOUSE_5,
    
    // Total count (keep this last)
    KEY_COUNT
};

enum class KeyState {
    RELEASED,
    JUST_PRESSED,
    HELD,
    JUST_RELEASED
};

class InputManager {
public:
    InputManager();
    
    // Key state management
    void updateKeyState(KeyCode key, bool pressed);
    bool isKeyPressed(KeyCode key) const;
    bool isKeyJustPressed(KeyCode key) const;
    bool isKeyHeld(KeyCode key) const;
    bool isKeyJustReleased(KeyCode key) const;
    
    // Mouse position management
    void setMousePosition(float x, float y);
    void setMouseDelta(float deltaX, float deltaY);
    float getMouseDeltaX() const { return mouseDeltaX; }
    float getMouseDeltaY() const { return mouseDeltaY; }
    float getMouseX() const { return mouseX; }
    float getMouseY() const { return mouseY; }
    void resetMouseDeltas() { mouseDeltaX = 0.0f; mouseDeltaY = 0.0f; }

    // Update cycle
    void update();
    
    // Platform-specific key code conversion
#ifdef _WIN32
    static KeyCode windowsKeyCodeToKeyCode(unsigned int windowsKeyCode);
#else
    static KeyCode xcbKeyCodeToKeyCode(unsigned int xcbKeyCode);
#endif

private:
    std::array<KeyState, static_cast<size_t>(KeyCode::KEY_COUNT)> keyStates;
    std::array<KeyState, static_cast<size_t>(KeyCode::KEY_COUNT)> prevKeyStates;
    
    // Mouse state
    float mouseX;
    float mouseY;
    float mouseDeltaX;
    float mouseDeltaY;

    // Maps for platform-specific key code conversion
#ifdef _WIN32
    static std::unordered_map<unsigned int, KeyCode> windowsKeyMap;
#else
    static std::unordered_map<unsigned int, KeyCode> xcbKeyMap;
#endif

    void initializePlatformKeyMaps();
};
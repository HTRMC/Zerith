// GUIRenderer.h
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "blocks/TextureManager.h"

// First, add these members to the GUIRenderer class in GUIRenderer.h
class GUIRenderer {
private:
    unsigned int textVAO = 0;
    unsigned int textVBO = 0;
    unsigned int fontTexture;
    unsigned int VAO, VBO;
    unsigned int crosshairTexture;
    int screenWidth, screenHeight;
    const int GUI_TEXTURE_UNIT = 15;
    const float CROSSHAIR_SIZE = 0.03f;
    bool isPauseMenuOpen = false;
    unsigned int buttonVAO, buttonVBO;
    const float BUTTON_WIDTH = 0.4f;  // 40% of screen width
    const float BUTTON_HEIGHT = 0.1f; // 10% of screen height
    const float BUTTON_SPACING = 0.05f; // Space between buttons

    struct Button {
        float x, y, width, height;
        std::string text;
        bool isHovered;
    };

    std::vector<Button> pauseMenuButtons;

    void initButtons() {
        // Create resume button
        Button resumeButton;
        resumeButton.width = BUTTON_WIDTH;
        resumeButton.height = BUTTON_HEIGHT;
        resumeButton.x = 0.0f;
        resumeButton.y = 0.2f;  // Position above center
        resumeButton.text = "Resume Game";
        resumeButton.isHovered = false;

        // Create quit button
        Button quitButton;
        quitButton.width = BUTTON_WIDTH;
        quitButton.height = BUTTON_HEIGHT;
        quitButton.x = 0.0f;
        quitButton.y = -0.2f;  // Position below center
        quitButton.text = "Quit Game";
        quitButton.isHovered = false;

        pauseMenuButtons = {resumeButton, quitButton};

        // Initialize button rendering data - using a unit quad
        float vertices[] = {
            // positions        // texture coords
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,  // bottom left
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f,  // bottom right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f,  // top right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f,  // top right
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,  // top left
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f   // bottom left
        };

        glGenVertexArrays(1, &buttonVAO);
        glGenBuffers(1, &buttonVBO);

        glBindVertexArray(buttonVAO);
        glBindBuffer(GL_ARRAY_BUFFER, buttonVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    struct CharInfo {
        float x;      // X position in texture (in pixels)
        float y;      // Y position in texture (in pixels)
        float width;  // Width of character (normally 8)
    };

    std::unordered_map<char, CharInfo> charMap;

    void initCharMap() {
        // Structure to define character ranges
        struct CharRange {
            char start;
            char end;
            float baseX;
            float baseY;
        };

        // Define all character ranges
        const CharRange ranges[] = {
            {' ', '/', 0.0f, 16.0f},    // symbols
            {'0', '?', 0.0f, 24.0f},    // numbers and symbols
            {'@', 'O', 0.0f, 32.0f},    // @ and first half of uppercase
            {'P', '_', 0.0f, 40.0f},    // second half of uppercase and symbols
            {'`', 'o', 0.0f, 48.0f},    // backtick and first half of lowercase
            {'p', '~', 0.0f, 56.0f}     // second half of lowercase and symbols
        };

        // Initialize all ranges
        for (const auto& range : ranges) {
            float x = range.baseX;
            for (char c = range.start; c <= range.end; ++c) {
                charMap[c] = {x, range.baseY, 8.0f};
                x += 8.0f;
            }
        }
    }

public:
    // Add these to the constructor initialization list
    GUIRenderer(int width, int height) : screenWidth(width), screenHeight(height) {
        initQuad();
        initButtons();
        initCharMap();
        crosshairTexture = TextureManager::getTexture("gui/sprites/hud/crosshair");
        fontTexture = TextureManager::getTexture("font/ascii");
    }

    void setPauseMenuOpen(bool open) {
        isPauseMenuOpen = open;
    }

    bool isPaused() const {
        return isPauseMenuOpen;
    }

    void initQuad() {
        // Calculate aspect ratio correction
        float aspectRatio = static_cast<float>(screenWidth) / screenHeight;
        float width = CROSSHAIR_SIZE;
        float height = CROSSHAIR_SIZE * aspectRatio;

        // Create a properly sized and aspect-ratio corrected quad
        float vertices[] = {
            // positions        // texture coords
            -width, -height, 0.0f,  0.0f, 0.0f,  // bottom left
             width, -height, 0.0f,  1.0f, 0.0f,  // bottom right
             width,  height, 0.0f,  1.0f, 1.0f,  // top right

             width,  height, 0.0f,  1.0f, 1.0f,  // top right
            -width,  height, 0.0f,  0.0f, 1.0f,  // top left
            -width, -height, 0.0f,  0.0f, 0.0f   // bottom left
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texture coord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void renderCrosshair(unsigned int shaderProgram) {
        glUseProgram(shaderProgram);

        // Save current OpenGL state
        GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

        // Setup for GUI rendering
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Set uniforms
        glUniform1i(glGetUniformLocation(shaderProgram, "isButton"), 0);
        glUniform4f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 1.0f, 1.0f);

        // Bind texture
        glActiveTexture(GL_TEXTURE0 + GUI_TEXTURE_UNIT);
        glBindTexture(GL_TEXTURE_2D, crosshairTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "guiTexture"), GUI_TEXTURE_UNIT);

        // Render quad
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Restore OpenGL state
        if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    }

    void renderPauseMenu(unsigned int shaderProgram) {
        if (!isPauseMenuOpen) return;

        glUseProgram(shaderProgram);

        // Save current OpenGL state
        GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLint currentVAO;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

        // Setup for GUI rendering
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw semi-transparent background using the buttonVAO
        glBindVertexArray(buttonVAO);

        // Draw full-screen background
        glm::mat4 bgModel = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(bgModel));
        glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 0.0f, 0.0f, 0.5f);
        glUniform1i(glGetUniformLocation(shaderProgram, "isButton"), 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw buttons
        for (const auto& button : pauseMenuButtons) {
            // Draw the individual button background
            glUniform1i(glGetUniformLocation(shaderProgram, "isButton"), 1);

            glm::mat4 buttonModel = glm::mat4(1.0f);
            buttonModel = glm::translate(buttonModel, glm::vec3(button.x, button.y, 0.0f));
            buttonModel = glm::scale(buttonModel, glm::vec3(button.width, button.height, 1.0f));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(buttonModel));

            if (button.isHovered) {
                glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.4f, 0.4f, 0.4f, 1.0f);
            } else {
                glUniform4f(glGetUniformLocation(shaderProgram, "color"), 0.2f, 0.2f, 0.2f, 1.0f);
            }

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Render button text
            // Save current OpenGL state
            GLint currentVAO;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

            // Calculate the text position to center it on the button
            float textScale = 0.05f;
            float textWidth = button.text.length() * textScale * 0.5f; // Approximate width of text
            float textX = button.x - textWidth / 2.0f;
            float textY = button.y - textScale / 2.0f;
            renderText(shaderProgram, button.text, textX, textY, textScale);

            // Restore OpenGL state
            glBindVertexArray(currentVAO);
        }

        // Restore OpenGL state
        if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
        if (!blendWasEnabled) glDisable(GL_BLEND);
    }

    bool handlePauseMenuClick(double xpos, double ypos) {
        if (!isPauseMenuOpen) return false;

        // Convert screen coordinates to normalized device coordinates
        float ndcX = (2.0f * xpos) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * ypos) / screenHeight;

        for (size_t i = 0; i < pauseMenuButtons.size(); i++) {
            const auto& button = pauseMenuButtons[i];
            if (ndcX >= button.x && ndcX <= button.x + button.width &&
                ndcY >= button.y && ndcY <= button.y + button.height) {
                return true;
            }
        }
        return false;
    }

    int getClickedButton(double xpos, double ypos) {
        if (!isPauseMenuOpen) return -1;

        float ndcX = (2.0f * xpos) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * ypos) / screenHeight;

        for (size_t i = 0; i < pauseMenuButtons.size(); i++) {
            const auto& button = pauseMenuButtons[i];
            float halfWidth = button.width / 2.0f;
            float halfHeight = button.height / 2.0f;

            // Check if the point is within the button's bounds
            if (ndcX >= button.x - halfWidth && ndcX <= button.x + halfWidth &&
                ndcY >= button.y - halfHeight && ndcY <= button.y + halfHeight) {
                return i;
                }
        }
        return -1;
    }

    void updateButtonHover(double xpos, double ypos) {
        if (!isPauseMenuOpen) return;

        float ndcX = (2.0f * xpos) / screenWidth - 1.0f;
        float ndcY = 1.0f - (2.0f * ypos) / screenHeight;

        for (auto& button : pauseMenuButtons) {
            float halfWidth = button.width / 2.0f;
            float halfHeight = button.height / 2.0f;

            // Check if the mouse is within the button's bounds
            button.isHovered = (ndcX >= button.x - halfWidth && ndcX <= button.x + halfWidth &&
                               ndcY >= button.y - halfHeight && ndcY <= button.y + halfHeight);
        }
    }

    void updateScreenSize(int width, int height) {
        screenWidth = width;
        screenHeight = height;
    }

    unsigned int getButtonVAO() const {
        return buttonVAO;
    }

    void renderText(unsigned int shaderId, const std::string& text, float x, float y, float scale) {
        if (textVAO == 0) {
            glGenVertexArrays(1, &textVAO);
            glGenBuffers(1, &textVBO);

            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0 + GUI_TEXTURE_UNIT);
        glBindTexture(GL_TEXTURE_2D, fontTexture);
        glUniform1i(glGetUniformLocation(shaderId, "guiTexture"), GUI_TEXTURE_UNIT);
        glUniform1i(glGetUniformLocation(shaderId, "isButton"), 0);

        float aspectRatio = static_cast<float>(screenWidth) / screenHeight;
        float widthScale = scale / aspectRatio;
        const float texSize = 128.0f;

        float currentX = x;

        for(char c : text) {
            auto it = charMap.find(c);
            if(it == charMap.end()) continue;  // Skip unknown characters

            const CharInfo& charInfo = it->second;

            float u0 = charInfo.x / texSize;
            float u1 = (charInfo.x + charInfo.width) / texSize;
            float v1 = 1.0f - (charInfo.y / texSize);
            float v0 = 1.0f - ((charInfo.y + charInfo.width) / texSize);

            float vertices[6][5] = {
                { currentX,              y + scale,     0.0f,  u0, v1 },    // top left
                { currentX + widthScale, y,             0.0f,  u1, v0 },    // bottom right
                { currentX,              y,             0.0f,  u0, v0 },    // bottom left

                { currentX,              y + scale,     0.0f,  u0, v1 },    // top left
                { currentX + widthScale, y + scale,     0.0f,  u1, v1 },    // top right
                { currentX + widthScale, y,             0.0f,  u1, v0 }     // bottom right
            };

            glBindVertexArray(textVAO);
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            currentX += widthScale;
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
};
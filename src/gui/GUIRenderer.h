// GUIRenderer.h
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "blocks/TextureManager.h"

class GUIRenderer {
private:
    unsigned int VAO, VBO;
    unsigned int crosshairTexture;
    int screenWidth, screenHeight;
    const int GUI_TEXTURE_UNIT = 15;
    const float CROSSHAIR_SIZE = 0.03f;

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

public:
    GUIRenderer(int width, int height) : screenWidth(width), screenHeight(height) {
        initQuad();
        crosshairTexture = TextureManager::getTexture("gui/sprites/hud/crosshair");

        // Debug prints
        std::cout << "Initializing GUI Renderer. Screen size: " << width << "x" << height << std::endl;
        std::cout << "Crosshair texture loaded: " << crosshairTexture << std::endl;
    }

    ~GUIRenderer() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void renderCrosshair(unsigned int shaderProgram) {
        glUseProgram(shaderProgram);

        // Save current OpenGL state
        GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        GLint previousBlendSrcFactor, previousBlendDstFactor;
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrcFactor);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDstFactor);

        // Setup for GUI rendering
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Bind texture
        glActiveTexture(GL_TEXTURE0 + GUI_TEXTURE_UNIT);
        glBindTexture(GL_TEXTURE_2D, crosshairTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "guiTexture"), GUI_TEXTURE_UNIT);

        // Render quad
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Restore previous OpenGL state
        if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
        glBlendFunc(previousBlendSrcFactor, previousBlendDstFactor);
        glActiveTexture(GL_TEXTURE0);
    }

    void updateScreenSize(int width, int height) {
        screenWidth = width;
        screenHeight = height;
    }
};
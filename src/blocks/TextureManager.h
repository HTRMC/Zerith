// TextureManager.h
#pragma once
#include <string>
#include <map>
#include <glad/glad.h>
#include <iostream>

class TextureManager {
public:
    static unsigned int loadTexture(const char *path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

        if (data) {
            GLenum internalFormat = nrChannels == 1 ? GL_RED : (nrChannels == 3 ? GL_RGB : GL_RGBA);
            GLenum format = internalFormat; // Match internal format

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            if (nrChannels == 1) {
                // For grayscale textures, replicate R channel to RGB
                float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            std::cout << "Failed to load texture: " << path << std::endl;
        }

        stbi_image_free(data);
        return textureID;
    }

    static unsigned int getTexture(const std::string &name) {
        static std::map<std::string, unsigned int> textureCache;

        auto it = textureCache.find(name);
        if (it != textureCache.end()) {
            return it->second;
        }

        std::string path = "assets/minecraft/textures/" + name + ".png";
        unsigned int texture = loadTexture(path.c_str());
        textureCache[name] = texture;
        return texture;
    }
};

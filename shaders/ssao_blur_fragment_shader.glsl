#version 450

// Input from vertex shader
layout(location = 0) in vec2 texCoord;

// Input SSAO texture
layout(binding = 0) uniform sampler2D ssaoInput;

// Output
layout(location = 0) out float fragAO;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;
    
    // Simple 4x4 box blur
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, texCoord + offset).r;
        }
    }
    
    fragAO = result / 16.0;
}
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat int fragTextureIndex;
layout(location = 3) in flat int fragRenderLayer; // New input for render layer

layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) out vec4 outColor;

// Define render layer constants to match our C++ enum
const int RENDER_LAYER_OPAQUE = 0;
const int RENDER_LAYER_CUTOUT = 1;
const int RENDER_LAYER_TRANSLUCENT = 2;

void main() {
    // Sample the texture
    vec4 texColor = texture(texSampler, vec3(fragTexCoord, fragTextureIndex));

    // Handle different render layers
    if (fragRenderLayer == RENDER_LAYER_CUTOUT) {
        // Alpha testing for cutout transparency
        if (texColor.a == 0) {
            discard;
        }
    }
    else if (fragRenderLayer == RENDER_LAYER_TRANSLUCENT) {
        // For translucent blocks, we'll use the alpha value directly
        // Alpha blending is handled by the pipeline
        // No special handling needed here
    }

    // For OPAQUE or fragments that passed the alpha test
    outColor = texColor;
}
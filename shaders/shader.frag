#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat int fragTextureIndex;

layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    // Mix color and texture
    vec4 texColor = texture(texSampler, vec3(fragTexCoord, fragTextureIndex));

    // Discard fragments with very low alpha
    if (texColor.a == 0) {
        discard;
    }

    outColor = texColor;
}
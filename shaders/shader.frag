// shader.frag
#version 450

layout(binding = 1) uniform sampler2DArray texArray;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in float fragTextureIndex;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texArray, vec3(fragTexCoord, fragTextureIndex));

    // Handle grass side overlay
    if (fragTextureIndex == 2.0) { // If this is a grass side texture
                                   vec4 overlayColor = texture(texArray, vec3(fragTexCoord, 3.0)); // Get overlay texture
                                   // Blend the base texture with the overlay using overlay's alpha
                                   outColor = vec4(mix(texColor.rgb, overlayColor.rgb, overlayColor.a), texColor.a);
    } else {
        outColor = texColor;
    }
}
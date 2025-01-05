// shader.frag
#version 450

layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in flat uint fragTextureID;

layout(location = 0) out vec4 outColor;

void main() {
    // For grass block sides, we need to blend the side texture with the overlay
    if (fragTextureID == 2) {  // If it's a grass block side
                               vec4 sideTexture = texture(texSampler, vec3(fragTexCoord, 2));  // grass_block_side
                               vec4 overlayTexture = texture(texSampler, vec3(fragTexCoord, 3));  // grass_block_side_overlay

                               // Blend the textures using the overlay's alpha channel
                               outColor = vec4(mix(sideTexture.rgb, overlayTexture.rgb, overlayTexture.a), sideTexture.a);
    } else {
        // For all other blocks, just use the texture directly
        outColor = texture(texSampler, vec3(fragTexCoord, fragTextureID));
    }
}

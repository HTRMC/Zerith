// fragment_shader.glsl
#version 330 core
in vec3 vertexColor;
in vec2 TexCoord;
in vec3 fragPos;
flat in float faceIndex;
flat in float textureIndex;

out vec4 FragColor;

uniform sampler2D blockTextures[16];
uniform bool isHighlighted;
uniform bool useTexture;
uniform vec3 highlightedBlockPos;

void main()
{
    float faceBrightness;
    if (faceIndex == 0.0) faceBrightness = 1.0;      // Top
    else if (faceIndex == 1.0) faceBrightness = 0.4; // Bottom
    else if (faceIndex == 2.0) faceBrightness = 0.8; // Front
    else if (faceIndex == 3.0) faceBrightness = 0.8; // Back
    else if (faceIndex == 4.0) faceBrightness = 0.6; // Right
    else if (faceIndex == 5.0) faceBrightness = 0.6; // Left
    else faceBrightness = 1.0;

    vec4 texColor;
    if (useTexture) {
        int texIndex = int(textureIndex);
        texColor = texture(blockTextures[texIndex], TexCoord);

        vec3 grassTint = vec3(0.7, 0.9, 0.5);

        // Handle grass block top
        if (texIndex == 3) { // grass_block_top
            texColor.rgb *= grassTint;
        }
        // Handle grass block side overlay
        else if (texIndex == 2) { // grass_block_side
            vec4 overlayColor = texture(blockTextures[7], TexCoord);
            overlayColor.rgb *= grassTint;
            texColor.rgb = mix(texColor.rgb, overlayColor.rgb, overlayColor.a);
        }
    } else {
        texColor = vec4(vertexColor, 1.0);
    }

    texColor.rgb *= faceBrightness;

    if (isHighlighted &&
    floor(fragPos.x) == highlightedBlockPos.x &&
    floor(fragPos.y) == highlightedBlockPos.y &&
    floor(fragPos.z) == highlightedBlockPos.z) {
        vec3 highlightColor = vec3(0.0, 1.0, 0.0);
        FragColor = vec4(mix(texColor.rgb, highlightColor, 0.3), texColor.a);
    } else {
        FragColor = texColor;
    }
}
// fragment_shader.glsl
#version 330 core
in vec3 vertexColor;
in vec2 TexCoord;
in vec3 fragPos;
flat in float faceIndex;

out vec4 FragColor;

uniform sampler2D blockTexture;
uniform bool isHighlighted;
uniform bool useTexture;

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
        texColor = texture(blockTexture, TexCoord) * vec4(vertexColor, 1.0);
    } else {
        texColor = vec4(vertexColor, 1.0);
    }

    texColor.rgb *= faceBrightness;

    if (isHighlighted) {
        vec3 highlightColor = vec3(0.0, 1.0, 0.0);
        FragColor = vec4(mix(texColor.rgb, highlightColor, 0.3), texColor.a);
    } else {
        FragColor = texColor;
    }
}

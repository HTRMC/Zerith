// fragment_shader.glsl
#version 330 core
in vec3 vertexColor;
in vec2 TexCoord;
in vec3 fragPos;

out vec4 FragColor;

uniform sampler2D blockTexture;
uniform bool isHighlighted;
uniform bool useTexture;

void main()
{
    vec4 texColor;
    if (useTexture) {
        texColor = texture(blockTexture, TexCoord) * vec4(vertexColor, 1.0);
    } else {
        texColor = vec4(vertexColor, 1.0);
    }

    if (isHighlighted) {
        vec3 highlightColor = vec3(0.0, 1.0, 0.0);
        FragColor = vec4(mix(texColor.rgb, highlightColor, 0.3), texColor.a);
    } else {
        FragColor = texColor;
    }
}
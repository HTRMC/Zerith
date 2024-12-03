// fragment_shader.glsl
#version 330 core
in vec3 vertexColor;
in vec3 fragPos;

out vec4 FragColor;

uniform bool isHighlighted;

void main()
{
    if (isHighlighted) {
        // Mix the original color with neon green
        vec3 highlightColor = vec3(0.0, 1.0, 0.0);  // Neon green
        FragColor = vec4(mix(vertexColor, highlightColor, 0.5), 1.0);
    } else {
        FragColor = vec4(vertexColor, 1.0);
    }
}
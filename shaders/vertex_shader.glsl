// vertex_shader.glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;
out vec3 fragPos;  // Add this

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 blockColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = blockColor;
    fragPos = vec3(model * vec4(aPos, 1.0));
}
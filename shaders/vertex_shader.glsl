// vertex_shader.glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aFaceIndex;
layout (location = 4) in float aTextureIndex;

out vec3 vertexColor;
out vec2 TexCoord;
out vec3 fragPos;
flat out float faceIndex;
flat out float textureIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 blockColor;

void main()
{
    vec4 worldPosition = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPosition;
    vertexColor = blockColor;
    TexCoord = aTexCoord;
    fragPos = worldPosition.xyz;
    faceIndex = aFaceIndex;
    textureIndex = aTextureIndex;
}

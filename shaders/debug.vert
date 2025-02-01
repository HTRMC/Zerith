// debug.vert
#version 450

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = push.viewProj * vec4(inPosition, 1.0);
    fragColor = inColor;
}

// shader.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

// Per-vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Face transforms stored in storage buffer
layout(binding = 2) readonly buffer FaceTransforms {
    mat4 transforms[6];
} faceTransforms;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    mat4 faceTransform = faceTransforms.transforms[gl_InstanceIndex];
    gl_Position = ubo.proj * ubo.view * push.model * faceTransform * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
// shader.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(binding = 2) readonly buffer InstanceTransforms {
    mat4 transforms[];
} instanceTransforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inTextureID;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat uint fragTextureID;

void main() {
    // Get the transform for this instance
    mat4 instanceTransform = instanceTransforms.transforms[gl_InstanceIndex];

    // Combine all transforms
    mat4 finalTransform = ubo.proj * ubo.view * push.model * instanceTransform;
    gl_Position = finalTransform * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    fragTextureID = inTextureID;
}
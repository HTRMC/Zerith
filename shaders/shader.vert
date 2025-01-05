// shader.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inTextureID;

layout(binding = 2) readonly buffer FaceTransforms {
    mat4 transforms[6];
} faceTransforms;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat uint fragTextureID;

void main() {
    // Calculate cube instance ID (which cube in the 16x16x16 grid)
    int cubeID = gl_InstanceIndex / 6;  // Divide by 6 because each cube has 6 faces

    // Calculate which face of the cube we're drawing (0-5)
    int faceIndex = gl_InstanceIndex % 6;  // This gives us which face of the current cube

    // Calculate grid position
    int x = cubeID % 16;
    int y = (cubeID / 16) % 16;
    int z = cubeID / (16 * 16);

    // Create translation matrix for this cube instance
    mat4 translation = mat4(1.0);
    translation[3].x = float(x);
    translation[3].y = float(y);
    translation[3].z = float(z);

    // Combine all transforms
    mat4 finalTransform = ubo.proj * ubo.view * push.model * translation * faceTransforms.transforms[faceIndex];
    gl_Position = finalTransform * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    fragTextureID = inTextureID;
}
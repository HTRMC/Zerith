// sky.vert
#version 450

layout(binding = 0) uniform SkyUBO {
    mat4 view;
    vec4 topColor;
    vec4 bottomColor;
} skyUBO;

layout(location = 0) out vec3 fragViewDir;

vec2 positions[6] = vec2[](
vec2(-1.0, -1.0),
vec2( 1.0, -1.0),
vec2(-1.0,  1.0),
vec2(-1.0,  1.0),
vec2( 1.0, -1.0),
vec2( 1.0,  1.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos.x, pos.y, 1.0, 1.0);

    // Convert screen-space coordinates to view direction in world space
    vec3 viewDir = vec3(pos.x, -pos.y, -1.0);
    viewDir = normalize(viewDir);

    // Transform the view direction by the inverse view matrix
    // We only need to use the rotational part of the view matrix
    mat3 viewRotation = mat3(skyUBO.view);
    fragViewDir = transpose(viewRotation) * viewDir;
}
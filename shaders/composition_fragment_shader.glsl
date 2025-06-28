#version 450

// Input from vertex shader
layout(location = 0) in vec2 texCoord;

// Input textures
layout(binding = 0) uniform sampler2D gAlbedo;
layout(binding = 1) uniform sampler2D ssaoTexture;

// Push constants
layout(push_constant) uniform PushConstants {
    float aoStrength;
} pc;

// Output
layout(location = 0) out vec4 fragColor;

void main() {
    // Get albedo color
    vec4 albedo = texture(gAlbedo, texCoord);
    
    // Get ambient occlusion
    float ao = texture(ssaoTexture, texCoord).r;
    
    // Apply ambient occlusion
    vec3 ambient = vec3(0.3) * albedo.rgb * mix(1.0, ao, pc.aoStrength);
    vec3 diffuse = vec3(0.7) * albedo.rgb; // Simple diffuse lighting
    
    fragColor = vec4(ambient + diffuse, albedo.a);
}
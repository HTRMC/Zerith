#version 450

// Input from vertex shader
layout(location = 0) in vec2 texCoord;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
    mat4 projection;
    mat4 view;
    vec4 samples[64]; // Kernel samples in tangent space
    vec4 noiseScale;  // (screenWidth/4.0, screenHeight/4.0, 0, 0)
} ubo;

// Textures
layout(binding = 1) uniform sampler2D gPosition;  // View-space position
layout(binding = 2) uniform sampler2D gNormal;    // View-space normal
layout(binding = 3) uniform sampler2D gDepth;     // Depth buffer
layout(binding = 4) uniform sampler2D noiseTexture; // 4x4 rotation noise

// SSAO parameters
layout(push_constant) uniform PushConstants {
    float radius;
    float bias;
    float intensity;
    uint kernelSize;
} pc;

// Output
layout(location = 0) out float fragAO;

void main() {
    // Get G-buffer data
    vec3 fragPos = texture(gPosition, texCoord).xyz;
    vec3 normal = normalize(texture(gNormal, texCoord).xyz);
    float depth = texture(gDepth, texCoord).r;
    
    // Early exit for sky/background
    if (depth >= 1.0) {
        fragAO = 1.0;
        return;
    }
    
    // Get random rotation vector from noise texture
    vec3 randomVec = texture(noiseTexture, texCoord * ubo.noiseScale.xy).xyz;
    
    // Create TBN matrix
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // SSAO calculation
    float occlusion = 0.0;
    for (uint i = 0u; i < pc.kernelSize; ++i) {
        // Get sample position in view space
        vec3 samplePos = TBN * ubo.samples[i].xyz;
        samplePos = fragPos + samplePos * pc.radius;
        
        // Project sample position to screen space
        vec4 offset = vec4(samplePos, 1.0);
        offset = ubo.projection * offset;
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;
        
        // Flip Y coordinate for Vulkan
        offset.y = 1.0 - offset.y;
        
        // Get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z;
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, pc.radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + pc.bias ? 1.0 : 0.0) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(pc.kernelSize));
    fragAO = pow(occlusion, pc.intensity);
}
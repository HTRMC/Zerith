#version 450

// Input from mesh shader
layout(location = 0) in PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
    flat uint textureLayer;
    vec3 worldPos;
    vec3 normal;
} v_in;

// Push constant for render layer
layout(push_constant) uniform PushConstants {
    uint renderLayer; // 0=OPAQUE, 1=CUTOUT, 2=TRANSLUCENT
} pc;

// Uniforms
layout(binding = 0) uniform UniformBufferObject {
    float time;
    mat4 view;
    mat4 proj;
    uint faceCount;
    vec4 cameraPos;
    vec4 frustumPlanes[6];
} ubo;

// Texture array sampler
layout(binding = 1) uniform sampler2DArray texArraySampler;

// Grass texture layer indices
layout(binding = 4) uniform GrassTextureIndices {
    uint grassTopLayer;
    uint grassSideLayer;
    uint grassOverlayLayer;
} grassTextures;

// Transparency texture layer indices
layout(binding = 5) uniform TransparencyTextureIndices {
    uint waterLayer;
    uint glassLayer;
    uint leavesLayer;
} transparencyTextures;

// G-buffer outputs
layout(location = 0) out vec4 gPosition; // View-space position
layout(location = 1) out vec4 gNormal;   // View-space normal
layout(location = 2) out vec4 gAlbedo;   // Albedo color

void main() {
    vec4 texColor;
    
    // Check if this is the missing texture layer
    if (v_in.textureLayer == 0xFFFFFFFFu) {
        vec2 gridPos = floor(v_in.texCoord * 2.0);
        bool isCheckerboard = mod(gridPos.x + gridPos.y, 2.0) == 0.0;
        texColor = vec4(isCheckerboard ? vec3(0.0, 0.0, 0.0) : vec3(1.0, 0.0, 1.0), 1.0);
    } else {
        vec3 texCoords = vec3(v_in.texCoord, float(v_in.textureLayer));
        texColor = texture(texArraySampler, texCoords);
    }
    
    // Apply grass color
    vec3 grassColor = vec3(121.0/255.0, 192.0/255.0, 90.0/255.0);
    if (v_in.textureLayer == grassTextures.grassTopLayer) {
        texColor.rgb *= grassColor;
    }
    
    // Handle grass block sides
    if (v_in.textureLayer == grassTextures.grassSideLayer && v_in.faceIndex >= 2u) {
        vec3 overlayCoords = vec3(v_in.texCoord, float(grassTextures.grassOverlayLayer));
        vec4 overlayColor = texture(texArraySampler, overlayCoords);
        overlayColor.rgb *= grassColor;
        texColor.rgb = mix(texColor.rgb, overlayColor.rgb, overlayColor.a);
    }
    
    // Alpha test
    if (texColor.a == 0.0) {
        discard;
    }
    
    // Apply render layer filtering
    bool shouldRender = false;
    
    if (pc.renderLayer == 0u) { // OPAQUE
        if (v_in.textureLayer != transparencyTextures.waterLayer && 
            v_in.textureLayer != transparencyTextures.glassLayer && 
            v_in.textureLayer != transparencyTextures.leavesLayer) {
            shouldRender = true;
        }
    } else if (pc.renderLayer == 1u) { // CUTOUT
        if (v_in.textureLayer == transparencyTextures.leavesLayer) {
            shouldRender = true;
            if (texColor.a < 0.5) {
                discard;
            }
        }
    } else if (pc.renderLayer == 2u) { // TRANSLUCENT
        if (v_in.textureLayer == transparencyTextures.waterLayer || 
            v_in.textureLayer == transparencyTextures.glassLayer) {
            shouldRender = true;
        }
    }
    
    if (!shouldRender) {
        discard;
    }
    
    // Transform to view space
    vec4 viewPos = ubo.view * vec4(v_in.worldPos, 1.0);
    vec3 viewNormal = mat3(ubo.view) * v_in.normal;
    
    // Output G-buffer data
    gPosition = vec4(viewPos.xyz, 1.0);
    gNormal = vec4(normalize(viewNormal), 1.0);
    gAlbedo = texColor;
}
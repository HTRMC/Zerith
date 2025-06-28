#version 450

// Input from mesh shader
layout(location = 0) in PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
    flat uint textureLayer;
    float ao; // Per-vertex ambient occlusion
} v_in;

// Push constant for render layer
layout(push_constant) uniform PushConstants {
    uint renderLayer; // 0=OPAQUE, 1=CUTOUT, 2=TRANSLUCENT
} pc;

// Texture array sampler
layout(binding = 1) uniform sampler2DArray texArraySampler;

// Grass texture layer indices (passed as uniforms)
layout(binding = 4) uniform GrassTextureIndices {
    uint grassTopLayer;      // Layer index for grass_block_top
    uint grassSideLayer;     // Layer index for grass_block_side
    uint grassOverlayLayer;  // Layer index for grass_block_side_overlay
} grassTextures;

// Transparency texture layer indices (passed as uniforms)
layout(binding = 5) uniform TransparencyTextureIndices {
    uint waterLayer;         // Layer index for water texture
    uint glassLayer;         // Layer index for glass texture
    uint leavesLayer;        // Layer index for oak_leaves texture
} transparencyTextures;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor;
    
    // Check if this is the missing texture layer (0xFFFFFFFF)
    if (v_in.textureLayer == 0xFFFFFFFFu) {
        // Generate procedural checkerboard pattern: X = Black, O = Magenta
        // [X][O]
        // [O][X]
        vec2 gridPos = floor(v_in.texCoord * 2.0);
        bool isCheckerboard = mod(gridPos.x + gridPos.y, 2.0) == 0.0;
        texColor = vec4(isCheckerboard ? vec3(0.0, 0.0, 0.0) : vec3(1.0, 0.0, 1.0), 1.0);
    } else {
        // Sample from the texture array using the layer index
        vec3 texCoords = vec3(v_in.texCoord, float(v_in.textureLayer));
        texColor = texture(texArraySampler, texCoords);
    }
    
    // Define grass color for multiplication
    vec3 grassColor = vec3(121.0/255.0, 192.0/255.0, 90.0/255.0);
    
    // Apply color multiplication for grass top texture
    if (v_in.textureLayer == grassTextures.grassTopLayer) {
        texColor.rgb *= grassColor;
    }
    
    // Special handling for grass block sides
    // Check if this is a grass block side face (not top/bottom)
    if (v_in.textureLayer == grassTextures.grassSideLayer && v_in.faceIndex >= 2u) {
        // Sample the overlay texture
        vec3 overlayCoords = vec3(v_in.texCoord, float(grassTextures.grassOverlayLayer));
        vec4 overlayColor = texture(texArraySampler, overlayCoords);
        
        // Apply color multiplication to the overlay
        overlayColor.rgb *= grassColor;
        
        // Blend overlay on top of base texture using alpha blending
        texColor.rgb = mix(texColor.rgb, overlayColor.rgb, overlayColor.a);
    }
    
    // Alpha handling based on render layer:
    // - OPAQUE: No alpha testing needed (alpha always 1.0)
    // - CUTOUT: Binary alpha test - discard if below threshold (leaves)
    // - TRANSLUCENT: Keep all alpha values for blending (glass, water)
    
    // Cutout alpha test threshold for leaves and similar blocks
    float alphaTestThreshold = 0.5;
    
    // Discard fragments with 0 alpha (for complete transparency)
    if (texColor.a == 0.0) {
        discard;
    }
    
    // Apply render layer specific processing based on texture layer
    bool shouldRender = false;
    
    if (pc.renderLayer == 0u) { // OPAQUE layer
        // Only render non-transparent textures (exclude water, glass, and leaves)
        if (v_in.textureLayer != transparencyTextures.waterLayer && 
            v_in.textureLayer != transparencyTextures.glassLayer && 
            v_in.textureLayer != transparencyTextures.leavesLayer) {
            shouldRender = true;
        }
    } else if (pc.renderLayer == 1u) { // CUTOUT layer  
        // Only render leaves (texture layer from uniform)
        if (v_in.textureLayer == transparencyTextures.leavesLayer) {
            shouldRender = true;
            // Apply alpha testing for cutout materials
            if (texColor.a < alphaTestThreshold) {
                discard;
            }
        }
    } else if (pc.renderLayer == 2u) { // TRANSLUCENT layer
        // Only render water and glass (texture layers from uniforms)
        if (v_in.textureLayer == transparencyTextures.waterLayer || 
            v_in.textureLayer == transparencyTextures.glassLayer) {
            shouldRender = true;
            // Keep all alpha values for blending
        }
    }
    
    // Discard fragments that don't belong to this render layer
    if (!shouldRender) {
        discard;
    }
    
    // Apply ambient occlusion
    texColor.rgb *= v_in.ao;
    
    // Output texture color
    outColor = texColor;
    
    // Debug UV coordinates with a pattern
    // Uncomment these lines to see a UV debug pattern instead of the texture
    // float checkerSize = 8.0;
    // bool isCheckerPattern = (mod(floor(v_in.texCoord.x * checkerSize), 2.0) == mod(floor(v_in.texCoord.y * checkerSize), 2.0));
    // outColor = vec4(isCheckerPattern ? vec3(1.0) : vec3(0.0, 0.0, 0.0), 1.0);
}
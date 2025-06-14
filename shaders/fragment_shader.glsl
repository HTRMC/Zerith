#version 450

// Input from mesh shader
layout(location = 0) in PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
    flat uint textureLayer;
} v_in;

// Texture array sampler
layout(binding = 1) uniform sampler2DArray texArraySampler;

// Grass texture layer indices (passed as uniforms)
layout(binding = 4) uniform GrassTextureIndices {
    uint grassTopLayer;      // Layer index for grass_block_top
    uint grassSideLayer;     // Layer index for grass_block_side
    uint grassOverlayLayer;  // Layer index for grass_block_side_overlay
} grassTextures;

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
    
    // Discard fragments with 0 alpha (for transparency)
    if (texColor.a == 0.0) {
        discard;
    }
    
    // Output texture color
    outColor = texColor;
    
    // Debug UV coordinates with a pattern
    // Uncomment these lines to see a UV debug pattern instead of the texture
    // float checkerSize = 8.0;
    // bool isCheckerPattern = (mod(floor(v_in.texCoord.x * checkerSize), 2.0) == mod(floor(v_in.texCoord.y * checkerSize), 2.0));
    // outColor = vec4(isCheckerPattern ? vec3(1.0) : vec3(0.0, 0.0, 0.0), 1.0);
}
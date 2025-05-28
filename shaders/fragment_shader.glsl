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

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample from the texture array using the layer index
    vec3 texCoords = vec3(v_in.texCoord, float(v_in.textureLayer));
    vec4 texColor = texture(texArraySampler, texCoords);
    
    // Define grass color for multiplication
    vec3 grassColor = vec3(121.0/255.0, 192.0/255.0, 90.0/255.0);
    
    // Apply color multiplication for grass top texture (layer 3)
    if (v_in.textureLayer == 3u) {
        texColor.rgb *= grassColor;
    }
    
    // Special handling for grass block sides (texture layer 4)
    // Check if this is a grass block side face (not top/bottom)
    if (v_in.textureLayer == 4u && v_in.faceIndex >= 2u) {
        // Sample the overlay texture (layer 5)
        vec3 overlayCoords = vec3(v_in.texCoord, 5.0);
        vec4 overlayColor = texture(texArraySampler, overlayCoords);
        
        // Apply color multiplication to the overlay
        overlayColor.rgb *= grassColor;
        
        // Blend overlay on top of base texture using alpha blending
        texColor.rgb = mix(texColor.rgb, overlayColor.rgb, overlayColor.a);
    }
    
    // Output texture color
    outColor = texColor;
    
    // Debug UV coordinates with a pattern
    // Uncomment these lines to see a UV debug pattern instead of the texture
    // float checkerSize = 8.0;
    // bool isCheckerPattern = (mod(floor(v_in.texCoord.x * checkerSize), 2.0) == mod(floor(v_in.texCoord.y * checkerSize), 2.0));
    // outColor = vec4(isCheckerPattern ? vec3(1.0) : vec3(0.0, 0.0, 0.0), 1.0);
}
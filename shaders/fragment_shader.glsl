#version 450

// Input from mesh shader
layout(location = 0) in PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
} v_in;

// Add texture sampler
layout(binding = 1) uniform sampler2D texSampler;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample texture
    vec4 texColor = texture(texSampler, v_in.texCoord);
    
    // Mix with face color to allow for face identification
    // Use face color as a subtle tint (20%)
    vec3 finalColor = mix(texColor.rgb, v_in.color, 0.2);
    
    // Output final color
    outColor = vec4(finalColor, 1.0);
}
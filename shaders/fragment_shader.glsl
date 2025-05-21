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
    
    // Just output the texture color without mixing with face color
    // Original code with color mixing (commented out):
    // vec3 finalColor = mix(texColor.rgb, v_in.color, 0.2);
    
    // Output final color directly from texture, preserving alpha
    outColor = texColor;
    
    // Debug UV coordinates with a pattern
    // Uncomment these lines to see a UV debug pattern instead of the texture
    // float checkerSize = 8.0;
    // bool isCheckerPattern = (mod(floor(v_in.texCoord.x * checkerSize), 2.0) == mod(floor(v_in.texCoord.y * checkerSize), 2.0));
    // outColor = vec4(isCheckerPattern ? vec3(1.0) : vec3(0.0, 0.0, 0.0), 1.0);
}
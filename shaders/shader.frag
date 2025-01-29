// shader.frag
#version 450

layout(binding = 1) uniform sampler2DArray texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in flat uint fragTextureID;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    // Light properties
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.8)); // Light direction
    vec3 lightColor = vec3(1.0, 1.0, 0.9);          // Slightly warm light
    float ambientStrength = 0.3;                     // Ambient light intensity

    // Grass tint color (#91BD59)
    vec3 grassTint = vec3(145.0/255.0, 189.0/255.0, 89.0/255.0);

    // Get base color from texture
    vec4 texColor;

    // Special case for grass block sides (texture ID 2)
    if (fragTextureID == 2) {
        // Get base grass side texture
        vec4 baseColor = texture(texSampler, vec3(fragTexCoord, 2));
        // Get overlay texture
        vec4 overlayColor = texture(texSampler, vec3(fragTexCoord, 3));
        overlayColor.rgb *= grassTint;
        // Pre-multiply alpha and blend
        vec4 blendedColor;
        blendedColor.rgb = mix(baseColor.rgb, overlayColor.rgb * overlayColor.a, overlayColor.a);
        blendedColor.a = baseColor.a;  // Keep base alpha
        texColor = blendedColor;
    }
    else if (fragTextureID == 1) {
        // Get base grass top texture
        vec4 baseColor = texture(texSampler, vec3(fragTexCoord, 1));
        baseColor.rgb *= grassTint;
        texColor = baseColor;
    } else {
        texColor = texture(texSampler, vec3(fragTexCoord, fragTextureID));
    }

    // Calculate diffuse lighting
    float diff = max(dot(normalize(fragNormal), lightDir), 0.0);

    // Combine ambient and diffuse
    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 lighting = ambient + diffuse;

    // Apply lighting to texture color
    outColor = vec4(texColor.rgb * lighting, texColor.a);
}

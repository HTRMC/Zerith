// sky.frag
#version 450

layout(binding = 0) uniform SkyUBO {
    mat4 view;
    vec4 topColor;
    vec4 bottomColor;
} skyUBO;

layout(location = 0) in vec3 fragViewDir;
layout(location = 0) out vec4 outColor;

void main() {
    // Calculate height factor based on normalized view direction
    float height = normalize(fragViewDir).z;
    height = (height + 1.0) * 0.5; // Map from [-1, 1] to [0, 1]

    // Apply a power function to create sharper transition near horizon
    height = pow(height, 0.5);

    // Create stronger horizon glow
    float horizonGlow = 1.0 - abs(normalize(fragViewDir).z);
    horizonGlow = pow(horizonGlow, 8.0); // Sharper horizon glow
    vec4 horizonColor = vec4(1.0, 1.0, 1.0, 1.0);

    // Mix colors based on height
    vec4 baseColor = mix(skyUBO.bottomColor, skyUBO.topColor, height);

    // Add horizon glow
    outColor = mix(baseColor, horizonColor, horizonGlow * 0.3);

    // Add atmospheric scattering
    float scatterStrength = pow(1.0 - height, 3.0) * 0.2;
    outColor.rgb += vec3(scatterStrength);
}
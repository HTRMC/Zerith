// sky.frag
#version 450

layout(binding = 0) uniform SkyColors {
    vec4 topColor;
    vec4 bottomColor;
} skyColors;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = mix(skyColors.bottomColor, skyColors.topColor, fragUV.y);
}
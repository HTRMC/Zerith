// gui_fragment_shader.glsl
#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D guiTexture;
uniform vec4 color;
uniform bool isButton;

void main() {
    if (isButton) {
        FragColor = color;
    } else {
        vec4 texColor = texture(guiTexture, TexCoord);
        if (texColor.a < 0.1) {
            discard;
        }
        FragColor = texColor;
    }
}
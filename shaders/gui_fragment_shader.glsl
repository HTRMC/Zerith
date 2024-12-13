// gui_fragment_shader.glsl
#version 460 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D guiTexture;

void main() {
    FragColor = texture(guiTexture, TexCoord);
}

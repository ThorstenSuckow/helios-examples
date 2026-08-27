#version 410 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec4 color;
uniform sampler2D tex;

flat in float vNormalizedAge;

void main() {

    float alpha = 1.0 - clamp(vNormalizedAge, 0.0, 1.0);

    FragColor = vec4(texture(tex, TexCoord).rgb, alpha);

}
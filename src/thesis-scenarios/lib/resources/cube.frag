#version 410 core

out vec4 FragColor;

uniform vec4 color;

flat in float vNormalizedAge;

void main() {

    float alpha = 1.0 - clamp(vNormalizedAge, 0.0, 1.0);

    FragColor = vec4(color.rgb, color.a * alpha);
}
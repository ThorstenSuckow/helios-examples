#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location=4) in mat4 instanceMatrix;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 TexCoord;

void main() {
    gl_Position = projectionMatrix * viewMatrix * instanceMatrix * vec4(aPos, 1.0);
    TexCoord = aTexCoords;
}
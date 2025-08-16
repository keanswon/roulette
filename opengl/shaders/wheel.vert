#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
out vec3 vColor;

uniform float uAngle;

void main() {
    float c = cos(uAngle), s = sin(uAngle);
    vec2 p = vec2(aPos.x*c - aPos.y*s, aPos.x*s + aPos.y*c);

    gl_Position = vec4(p, aPos.z, 1.0);
    vColor = aColor;
}
#version 330 core

uniform mat4 m4;

in vec4 v4;
out vec2 xy;

void main() {
    gl_Position = m4 * vec4(v4.xy, 0.0, 1.0);
    xy = v4.zw;
}

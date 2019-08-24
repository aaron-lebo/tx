#version 330 core

uniform sampler2D tx;

in vec2 xy;
out vec4 rgba;

const float edge = 0.5;

const float spread = 4.0;
const float scale = 2.0;
const float smoothing = 0.25 / spread * scale;

void main() {
    float distance = texture2D(tx, xy).a;
    float a = smoothstep(edge - smoothing, edge + smoothing, distance);
    rgba = vec4(1.0, 1.0, 1.0, a);
}

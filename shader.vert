#version 430 core

layout(location = 0) in vec2 position;

layout(std430, binding = 0) buffer ParticleBuffer {
    vec4 particles[];
};

uniform vec2 iResolution;
uniform vec2 mousePos;
uniform int isCursor;

void main() {
    if (isCursor == 1) {
        vec2 cursorPos = (mousePos / iResolution) * 2.0 - 1.0;
        gl_Position = vec4(cursorPos, 0.0, 1.0);
    } else {
        vec2 particlePos = particles[gl_InstanceID].xy;
        vec2 screenPos = particlePos * 0.5 + 0.5;
        vec2 pixelPos = screenPos * iResolution;
        vec2 normalizedPos = (pixelPos / iResolution) * 2.0 - 1.0;
        gl_Position = vec4(normalizedPos, 0.0, 1.0);
    }
    // gl_PointSize is now set in the C code
}
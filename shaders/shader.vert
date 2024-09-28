#version 430 core

layout(location = 0) in vec2 position;

layout(std430, binding = 0) buffer ParticleBuffer {
    vec4 particles[];
};

uniform vec2 iResolution;
uniform vec2 mousePos;
uniform int isCursor;

void main() {
    vec2 pos;
    if (isCursor == 1) {
        pos = mousePos;
    } else {
        pos = particles[gl_InstanceID].xy;
    }
    
    // Transform from screen space to clip space
    vec2 clipPos = (pos / iResolution) * 2.0 - 1.0;
    clipPos.y = -clipPos.y; // Flip Y coordinate for OpenGL
    gl_Position = vec4(clipPos, 0.0, 1.0);
}
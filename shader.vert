#version 430 core

layout(location = 0) in vec2 position;

layout(std430, binding = 0) buffer ParticleBuffer {
    vec4 particles[];
};

uniform vec2 iResolution;

void main() {
    vec2 particlePos = particles[gl_InstanceID].xy;
    vec2 screenPos = particlePos * 0.5 + 0.5; // Convert from [-1, 1] to [0, 1]
    vec2 pixelPos = screenPos * iResolution;
    vec2 normalizedPos = (pixelPos / iResolution) * 2.0 - 1.0;
    gl_Position = vec4(normalizedPos, 0.0, 1.0);
    gl_PointSize = 1.0;
}
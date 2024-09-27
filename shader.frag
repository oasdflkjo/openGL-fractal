#version 430 core

uniform float iTime;
uniform vec2 iResolution;

layout(std430, binding = 0) buffer ParticleBuffer {
    vec4 particles[];
};

out vec4 fragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec3 color = vec3(0.0);

    for (int i = 0; i < 10000; i++) { // Make sure this matches NUM_PARTICLES
        vec2 particlePos = particles[i].xy * 0.5 + 0.5; // Convert from -1,1 to 0,1 range
        float distance = length(uv - particlePos);
        if (distance < 0.005) {
            color += vec3(1.0, 0.5, 0.2) * (1.0 - distance / 0.005);
        }
    }

    // Add a subtle background glow
    color += vec3(0.1, 0.05, 0.2) * (1.0 - length(uv - 0.5));

    fragColor = vec4(color, 1.0);
}

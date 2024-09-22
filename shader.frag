#version 430 core  // Some GPUs may need 4.30 or higher for double support

uniform float iTime;
uniform vec2 iResolution;

out vec4 FragColor;

void main() {
    vec2 uv = (gl_FragCoord.xy - iResolution * 0.5) / iResolution.y;

    // Zoom factor, now with float precision
    float zoom = exp(iTime * 0.3); // 2x zoom speed

    // Float precision center for the fractal
    vec2 center = vec2(-0.74543, 0.11301); // Known detailed region
    vec2 c = center + uv / zoom;    // Adjust for zoom

    vec2 z = vec2(0.0);
    float iterations = 0.0;
    const float maxIterations = 100.0; // Reduced from 200.0

    for (float i = 0.0; i < maxIterations; i++) {
        z = vec2(
            z.x * z.x - z.y * z.y + c.x,
            2.0 * z.x * z.y + c.y
        );
        if (length(z) > 2.0) break;
        iterations = i;
    }

    float color = iterations / maxIterations;
    FragColor = vec4(vec3(color), 1.0);
}

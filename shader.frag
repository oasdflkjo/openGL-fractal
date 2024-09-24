#version 430 core

uniform float iTime;
uniform vec2 iResolution;

out vec4 FragColor;

const float ZOOM_SPEED = 0.5;
const vec2 MANDELBROT_CENTER = vec2(-0.74543, 0.11301);
const float ESCAPE_RADIUS = 2.0;
const float MAX_ITERATIONS = 300.0;

void main() {
    vec2 uv = (gl_FragCoord.xy - iResolution * 0.5) / iResolution.y;

    float zoom = exp(iTime * ZOOM_SPEED);

    vec2 c = MANDELBROT_CENTER + uv / zoom;

    vec2 z = vec2(0.0);
    float iterations = 0.0;

    for (float i = 0.0; i < MAX_ITERATIONS; i++) {
        z = vec2(
            z.x * z.x - z.y * z.y + c.x,
            2.0 * z.x * z.y + c.y
        );
        if (length(z) > ESCAPE_RADIUS) break;
        iterations = i;
    }

    float color = iterations / MAX_ITERATIONS;
    FragColor = vec4(vec3(color), 1.0);
}

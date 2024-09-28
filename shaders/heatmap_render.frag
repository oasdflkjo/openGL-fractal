#version 430

uniform sampler2D densityMap;
uniform vec2 iResolution;

out vec4 fragColor;

vec3 heatmapColorGradient(float t) {
    const vec3 c0 = vec3(0, 0, 0);
    const vec3 c1 = vec3(0, 0, 1);
    const vec3 c2 = vec3(0, 1, 1);
    const vec3 c3 = vec3(1, 1, 0);
    const vec3 c4 = vec3(1, 0, 0);

    float r = t * 4.0;
    if (r < 1.0) return mix(c0, c1, r);
    r -= 1.0;
    if (r < 1.0) return mix(c1, c2, r);
    r -= 1.0;
    if (r < 1.0) return mix(c2, c3, r);
    r -= 1.0;
    return mix(c3, c4, r);
}

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution;
    float density = texture(densityMap, uv).r;
    
    // Apply a threshold to remove very low density values
    float threshold = 0.05;
    if (density < threshold) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0); // Black background
    } else {
        // Adjust the density range for better visualization
        density = (density - threshold) / (1.0 - threshold);
        density = pow(density, 0.5); // Apply gamma correction for better contrast
        
        vec3 heatmapColor = heatmapColorGradient(density);
        fragColor = vec4(heatmapColor, 1.0);
    }
}
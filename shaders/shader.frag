#version 430 core

out vec4 fragColor;

uniform int isCursor;

void main() {
    if (isCursor == 1) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red cursor
    } else {
        fragColor = vec4(1.0, 1.0, 1.0, 1.0); // White particles
    }
}

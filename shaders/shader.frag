#version 430 core

out vec4 fragColor;

uniform int isCursor; // Add this line to determine if we're drawing the cursor

void main() {
    if (isCursor == 1) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color for cursor
    } else {
        fragColor = vec4(1.0, 0.5, 0.0, 0.5); // Bright orange color for particles
    }
}

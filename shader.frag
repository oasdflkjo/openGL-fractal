#version 430 core

out vec4 fragColor;

uniform int isCursor; // Add this line to determine if we're drawing the cursor

void main() {
    if (isCursor == 1) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color for cursor
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 0.5); // Black color for particles
    }
}

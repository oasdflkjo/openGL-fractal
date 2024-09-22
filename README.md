# OpenGL Fractal Shader Demo

This project demonstrates a simple OpenGL application that renders a Mandelbrot fractal using a fragment shader. It's designed to run on Windows and provides a fullscreen, interactive fractal visualization.

## Features

- Fullscreen OpenGL rendering
- Dynamic fractal zoom based on time
- GLSL fragment shader for fractal rendering
- Vsync support for smooth animation
- Simple Windows API integration

## Prerequisites

To build and run this project, you'll need:

- Windows operating system
- GCC compiler (MinGW-w64 recommended)
- OpenGL headers and libraries

Note: While Windows includes some OpenGL support by default, you may need to install additional OpenGL development files. The exact headers and libraries required (like glext.h and wglext.h) are not always included in the default Windows installation.

## Setting up OpenGL Development Files

1. Download the latest OpenGL headers from the Khronos Group's OpenGL Registry:
   - glext.h: https://www.khronos.org/registry/OpenGL/api/GL/glext.h
   - wglext.h: https://www.khronos.org/registry/OpenGL/api/GL/wglext.h

2. Place these header files in your compiler's include directory, typically:
   `C:\MinGW\include\GL\` (adjust the path according to your MinGW installation)

3. Ensure your compiler's include path is set up correctly to find these headers.

## Building the Project

1. Ensure you have GCC installed and added to your system PATH.
2. Open a command prompt in the project directory.
3. Run the following command to compile the project:

gcc -std=c11 -Os -s -mwindows -Wall demo.c -o demo.exe -lopengl32 -lgdi32 -lwinmm -lglu32
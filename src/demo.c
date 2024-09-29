#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "shader_utils.h"
#include "gl_loader.h"

// Global variable declarations
GLint iTimeLocation;
GLint iResolutionLocation;

// Function pointer declarations
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);

// Add these global variables
GLuint computeProgram;
GLint mousePositionLocation;
GLint deltaTimeLocation;
const int NUM_PARTICLES = 15000000;
const int WORK_GROUP_SIZE = 256;
GLuint iResolutionLocationCompute;
GLuint vertexArray;
GLuint vertexBuffer;
float mouseX = 0.0f, mouseY = 0.0f;

// Pixel format descriptor
PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0, 0, 0, 0,
    24,
    0,
    0,
    PFD_MAIN_PLANE,
    0,
    0, 0, 0
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE || (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// New function declarations
void SetupFullscreenWindow(HWND *hwnd, HDC *hdc, int *screenWidth, int *screenHeight, HINSTANCE hInstance);
void InitializeOpenGL(HDC hdc, HGLRC *hglrc);
void InitializeParticles(GLuint *particleBuffers, int screenWidth, int screenHeight);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    int screenWidth, screenHeight;

    SetupFullscreenWindow(&hwnd, &hdc, &screenWidth, &screenHeight, hInstance);
    InitializeOpenGL(hdc, &hglrc);

    LoadOpenGLFunctions();
    LoadShaderUtilsFunctions();

    ShowCursor(FALSE);

    // Initialize random seed
    srand(time(NULL));

    // Set up particle buffers (ping-pong)
    GLuint particleBuffers[2];
    InitializeParticles(particleBuffers, screenWidth, screenHeight);

    // Set up vertex array and buffer for instanced rendering
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    float point[] = {0.0f, 0.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(point), point, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glVertexAttribDivisor(0, 0);

    // Update this function call to include the shader directory
    char *fragmentShaderSource = LoadShader("shaders/shader.frag");
    if (!fragmentShaderSource) {
        return -1;
    }
    GLuint shader = CompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);

    // Update this function call as well
    char *vertexShaderSource = LoadShader("shaders/shader.vert");
    if (!vertexShaderSource) {
        return -1;
    }
    GLuint vertexShader = CompileShader(vertexShaderSource, GL_VERTEX_SHADER);
    glAttachShader(program, vertexShader);

    glLinkProgram(program);
    CheckProgramLinkStatus(program);
    glUseProgram(program);
    iTimeLocation = glGetUniformLocation(program, "iTime");
    iResolutionLocation = glGetUniformLocation(program, "iResolution");
    // startTime = GetTickCount() / 1000.0f;
    free(fragmentShaderSource);
    free(vertexShaderSource);

    // Load compute shader
    // Update this function call for the compute shader
    char *computeShaderSource = LoadShader("shaders/particle_update.comp");
    if (!computeShaderSource) {
        return -1;
    }
    GLuint computeShader = CompileShader(computeShaderSource, GL_COMPUTE_SHADER);
    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    mousePositionLocation = glGetUniformLocation(computeProgram, "mousePosition");
    deltaTimeLocation = glGetUniformLocation(computeProgram, "deltaTime");
    iResolutionLocationCompute = glGetUniformLocation(computeProgram, "iResolution");
    free(computeShaderSource);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MSG msg;
    DWORD lastTime = GetTickCount();
    int currentBuffer = 0;  // Add this line to declare currentBuffer
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        DWORD currentTime = GetTickCount();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Update particles
        glUseProgram(computeProgram);
        
        // Get mouse position
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hwnd, &mousePos);
        mouseX = (float)mousePos.x;
        mouseY = (float)(screenHeight - mousePos.y);
        glUniform2f(mousePositionLocation, mouseX, mouseY);
        
        glUniform1f(deltaTimeLocation, deltaTime);
        glUniform2f(iResolutionLocationCompute, (float)screenWidth, (float)screenHeight);
        glUniform1f(glGetUniformLocation(computeProgram, "aspectRatio"), (float)screenWidth / screenHeight);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers[currentBuffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleBuffers[1-currentBuffer]);

        // Calculate the number of work groups needed
        int numWorkGroups = (NUM_PARTICLES + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        glDispatchCompute(numWorkGroups, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Render particles
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Set clear color to grey
        glUseProgram(program);
        glUniform1f(iTimeLocation, deltaTime);
        glUniform2f(iResolutionLocation, (float)screenWidth, (float)screenHeight);
        glUniform1f(glGetUniformLocation(program, "aspectRatio"), (float)screenWidth / screenHeight);
        glBindVertexArray(vertexArray);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers[1-currentBuffer]);

        GLint mousePosLoc = glGetUniformLocation(program, "mousePos");
        GLint isCursorLoc = glGetUniformLocation(program, "isCursor");
        glUniform2f(mousePosLoc, mouseX, mouseY);

        // Draw particles first
        glUniform1i(isCursorLoc, 0);
        glDrawArraysInstanced(GL_POINTS, 0, 1, NUM_PARTICLES);

        // Then draw cursor (red dot) on top
        glUniform1i(isCursorLoc, 1);
        glPointSize(10.0f);  // Set a larger point size for the cursor
        glDrawArraysInstanced(GL_POINTS, 0, 1, 1);
        glPointSize(1.0f);  // Reset point size for particles in the next frame

        SwapBuffers(hdc);
        currentBuffer = 1 - currentBuffer;

        // Check for OpenGL errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            printf("OpenGL error: %d\n", err);
        }
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    ChangeDisplaySettings(NULL, 0);
    ShowCursor(TRUE);
    ExitProcess(0);
}

void SetupFullscreenWindow(HWND *hwnd, HDC *hdc, int *screenWidth, int *screenHeight, HINSTANCE hInstance) {
    DEVMODE screenSettings = {0};
    screenSettings.dmSize = sizeof(screenSettings);
    *screenWidth = GetSystemMetrics(SM_CXSCREEN);
    *screenHeight = GetSystemMetrics(SM_CYSCREEN);
    screenSettings.dmPelsWidth = *screenWidth;
    screenSettings.dmPelsHeight = *screenHeight;
    screenSettings.dmBitsPerPel = 32;
    screenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);

    *hwnd = CreateWindowEx(0, (LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, *screenWidth, *screenHeight, 0, 0, hInstance, 0);
    *hdc = GetDC(*hwnd);
}

void InitializeOpenGL(HDC hdc, HGLRC *hglrc) {
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    *hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, *hglrc);
}

void InitializeParticles(GLuint *particleBuffers, int screenWidth, int screenHeight) {
    glGenBuffers(2, particleBuffers);
    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffers[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, particleBuffers[i]);
    }

    float *initialParticleData = (float*)malloc(NUM_PARTICLES * 4 * sizeof(float));
    float ySpacing = 2.0f;
    int currentParticle = 0;

    for (float y = 0; currentParticle < NUM_PARTICLES; y += ySpacing) {
        for (float x = 0; x < screenWidth && currentParticle < NUM_PARTICLES; x += 2.0f) {
            initialParticleData[currentParticle*4] = x;
            initialParticleData[currentParticle*4+1] = screenHeight - y;
            initialParticleData[currentParticle*4+2] = 0.0f;
            initialParticleData[currentParticle*4+3] = 0.0f;
            currentParticle++;
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffers[0]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * 4 * sizeof(float), initialParticleData);
    free(initialParticleData);
}
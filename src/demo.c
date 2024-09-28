#include <windows.h>
#include <windowsx.h>  // Add this for GET_X_LPARAM and GET_Y_LPARAM
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>  // Add this for bool type and true/false constants
#include <wow64apiset.h>

#include "shader_utils.h"
#include "gl_loader.h"

// Add this declaration near the top of the file, after the other function declarations
const char* GetErrorLocation(void);

// Global variable declarations
GLint iTimeLocation;
GLint iResolutionLocation;

// Function pointer declarations
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);

// Add these global variables
GLuint computeProgram;
GLint mousePositionLocation;
GLint deltaTimeLocation;
const int NUM_PARTICLES = 1000000;
const int WORK_GROUP_SIZE = 16;
GLuint iResolutionLocationCompute;
GLuint vertexArray;
GLuint vertexBuffer;
float mouseX = 0.0f, mouseY = 0.0f;
GLuint densityTexture;
GLuint heatmapRenderProgram;
GLuint quadVAO;

// Add these global variables for mouse polling
POINT currentMousePos;
bool mousePositionUpdated = false;

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

// Add these global variables near the top of the file
bool mouseInWindow = false;

GLuint clearHeatmapProgram;

// Add these declarations near the top of the file with other global variables
const int screenWidth = 3440;
const int screenHeight = 1440;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_MOUSEMOVE:
            mouseInWindow = true;
            break;
        case WM_MOUSELEAVE:
            mouseInWindow = false;
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Add this function to generate a random heatmap
void generateRandomHeatmap(float* heatmapData, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        heatmapData[i] = (float)rand() / RAND_MAX;
    }
}

// Add this function before the main loop
void clearDensityTexture() {
    glUseProgram(clearHeatmapProgram);
    glBindImageTexture(0, densityTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    glDispatchCompute((screenWidth + 15) / 16, (screenHeight + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    BOOL isWow64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &isWow64) && isWow64) {
        Wow64DisableWow64FsRedirection(NULL);
    }

    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
        printf("Current working directory: %s\n", cwd);
    } else {
        printf("Failed to get current working directory\n");
    }

    DEVMODE screenSettings = {0};
    screenSettings.dmSize = sizeof(screenSettings);
    screenSettings.dmPelsWidth = screenWidth;
    screenSettings.dmPelsHeight = screenHeight;
    screenSettings.dmBitsPerPel = 32;
    screenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);

    HWND hwnd = CreateWindowEx(0, (LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, screenWidth, screenHeight, 0, 0, hInstance, 0);

    // Track mouse events
    TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
    TrackMouseEvent(&tme);

    // Enable raw input for the mouse
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; 
    rid.usUsage = 0x02; 
    rid.dwFlags = 0;
    rid.hwndTarget = hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        // Handle error
        printf("Failed to register raw input device.\n");
    }

    HDC hdc = GetDC(hwnd);

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    LoadOpenGLFunctions();
    LoadShaderUtilsFunctions();

    ShowCursor(FALSE);

    // Initialize random seed
    srand(time(NULL));

    // Set up particle buffers (ping-pong)
    GLuint particleBuffers[2];
    glGenBuffers(2, particleBuffers);
    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffers[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, particleBuffers[i]);
    }

    // Initialize particles
    float *initialParticleData = (float*)malloc(NUM_PARTICLES * 4 * sizeof(float));
    for (int i = 0; i < NUM_PARTICLES; i++) {
        initialParticleData[i*4] = (float)(i % screenWidth);
        initialParticleData[i*4+1] = (float)((i / screenWidth) % screenHeight);
        initialParticleData[i*4+2] = (float)rand() / RAND_MAX * 2.0f - 1.0f; // Random vx between -1 and 1
        initialParticleData[i*4+3] = (float)rand() / RAND_MAX * 2.0f - 1.0f; // Random vy between -1 and 1
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffers[0]);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * 4 * sizeof(float), initialParticleData);
    free(initialParticleData);

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
    GLuint shader = CompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER, "shaders/shader.frag");
    if (shader == 0) {
        free(fragmentShaderSource);
        return -1;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);

    // Update this function call as well
    char *vertexShaderSource = LoadShader("shaders/shader.vert");
    if (!vertexShaderSource) {
        return -1;
    }
    GLuint vertexShader = CompileShader(vertexShaderSource, GL_VERTEX_SHADER, "shaders/shader.vert");
    if (vertexShader == 0) {
        free(vertexShaderSource);
        return -1;
    }
    glAttachShader(program, vertexShader);

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        char *infoLog = (char*)malloc(logLength);
        if (infoLog) {
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            glGetProgramInfoLog(program, logLength, NULL, infoLog);
            printf("Main program linking failed: %s\n", infoLog);
            free(infoLog);
        }
        return -1;
    }
    CheckProgramLinkStatus(program);
    glUseProgram(program);
    iTimeLocation = glGetUniformLocation(program, "iTime");
    iResolutionLocation = glGetUniformLocation(program, "iResolution");
    // startTime = GetTickCount() / 1000.0f;
    free(fragmentShaderSource);
    free(vertexShaderSource);

    // After compiling and linking the main program
    printf("Main program compiled and linked. Program ID: %u\n", program);
    printf("iTimeLocation: %d\n", iTimeLocation);
    printf("iResolutionLocation: %d\n", iResolutionLocation);

    // Load compute shader
    char *computeShaderSource = LoadShader("shaders/particle_update_and_heatmap.comp");
    if (!computeShaderSource) {
        return -1;
    }

    // After loading the compute shader source
    printf("Compute Shader Source:\n%s\n", computeShaderSource);

    // Replace the existing compute shader compilation and linking code with this:
    GLuint computeShader = CompileShader(computeShaderSource, GL_COMPUTE_SHADER, "shaders/particle_update_and_heatmap.comp");
    if (computeShader == 0) {
        printf("Failed to compile compute shader.\n");
        return -1;
    }

    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);

    GLint computeLinkStatus;
    glGetProgramiv(computeProgram, GL_LINK_STATUS, &computeLinkStatus);
    if (computeLinkStatus == GL_FALSE) {
        GLint logLength;
        glGetProgramiv(computeProgram, GL_INFO_LOG_LENGTH, &logLength);
        char* infoLog = (char*)malloc(logLength);
        glGetProgramInfoLog(computeProgram, logLength, NULL, infoLog);
        printf("Compute program linking failed: %s\n", infoLog);
        free(infoLog);
        return -1;
    }

    printf("Compute program compiled and linked successfully. Program ID: %u\n", computeProgram);

    // Add error checking for uniform locations
    mousePositionLocation = glGetUniformLocation(computeProgram, "mousePosition");
    deltaTimeLocation = glGetUniformLocation(computeProgram, "deltaTime");
    iResolutionLocationCompute = glGetUniformLocation(computeProgram, "iResolution");

    printf("Uniform locations:\n");
    printf("mousePosition: %d\n", mousePositionLocation);
    printf("deltaTime: %d\n", deltaTimeLocation);
    printf("iResolution: %d\n", iResolutionLocationCompute);

    if (mousePositionLocation == -1 || deltaTimeLocation == -1 || iResolutionLocationCompute == -1) {
        printf("Warning: Some uniform locations not found. This may be intentional.\n");
        printf("mousePosition: %d\n", mousePositionLocation);
        printf("deltaTime: %d\n", deltaTimeLocation);
        printf("iResolution: %d\n", iResolutionLocationCompute);
    }

    printf("Successfully retrieved uniform locations for compute shader.\n");

    free(computeShaderSource);

    // Create density texture
    glGenTextures(1, &densityTexture);
    glBindTexture(GL_TEXTURE_2D, densityTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, screenWidth, screenHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generate random heatmap data
    float* heatmapData = (float*)malloc(screenWidth * screenHeight * sizeof(float));
    generateRandomHeatmap(heatmapData, screenWidth, screenHeight);

    // Upload heatmap data to GPU
    glBindTexture(GL_TEXTURE_2D, densityTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RED, GL_FLOAT, heatmapData);
    free(heatmapData);

    // Load heatmap render shader
    char *heatmapVertexShaderSource = LoadShader("shaders/fullscreen_quad.vert");
    char *heatmapFragmentShaderSource = LoadShader("shaders/heatmap_render.frag");
    if (!heatmapVertexShaderSource || !heatmapFragmentShaderSource) {
        return -1;
    }
    GLuint heatmapVertexShader = CompileShader(heatmapVertexShaderSource, GL_VERTEX_SHADER, "shaders/fullscreen_quad.vert");
    GLuint heatmapFragmentShader = CompileShader(heatmapFragmentShaderSource, GL_FRAGMENT_SHADER, "shaders/heatmap_render.frag");
    heatmapRenderProgram = glCreateProgram();
    glAttachShader(heatmapRenderProgram, heatmapVertexShader);
    glAttachShader(heatmapRenderProgram, heatmapFragmentShader);
    glLinkProgram(heatmapRenderProgram);
    free(heatmapVertexShaderSource);
    free(heatmapFragmentShaderSource);

    // After compiling and linking the heatmap render program
    printf("Heatmap render program compiled and linked. Program ID: %u\n", heatmapRenderProgram);

    // Create a fullscreen quad for rendering the heatmap
    float quadVertices[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MSG msg;
    DWORD lastTime = GetTickCount();
    int currentBuffer = 0;
    float renderMouseX = 0.5f, renderMouseY = 0.5f;

    // Add these debug print statements after linking each shader program
    printf("Main program link status: %d\n", success);
    printf("Compute program link status: %d\n", success);
    printf("Heatmap render program link status: %d\n", success);

    // Load and compile the clear heatmap compute shader
    const char* clearHeatmapSource = LoadShader("shaders/clear_heatmap.comp");
    GLuint clearHeatmapShader = CompileShader(clearHeatmapSource, GL_COMPUTE_SHADER, "shaders/clear_heatmap.comp");
    clearHeatmapProgram = glCreateProgram();
    glAttachShader(clearHeatmapProgram, clearHeatmapShader);
    glLinkProgram(clearHeatmapProgram);
    CheckProgramLinkStatus(clearHeatmapProgram);
    glDeleteShader(clearHeatmapShader);

    while (!GetAsyncKeyState(VK_ESCAPE)) {
        // Process all pending messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Poll mouse position once per frame
        POINT newMousePos;
        if (GetCursorPos(&newMousePos) && ScreenToClient(hwnd, &newMousePos)) {
            currentMousePos = newMousePos;
            mousePositionUpdated = true;
        }

        DWORD currentTime = GetTickCount();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Update mouse position if it has changed
        if (mousePositionUpdated) {
            mouseX = (float)currentMousePos.x;
            mouseY = (float)currentMousePos.y;
            mousePositionUpdated = false;

            // Smoothly update render mouse position
            renderMouseX = renderMouseX * 0.8f + mouseX * 0.2f;
            renderMouseY = renderMouseY * 0.8f + mouseY * 0.2f;

            printf("Mouse position: (%.2f, %.2f)\n", renderMouseX, renderMouseY);
        }

        // Clear the density texture at the start of each frame
        clearDensityTexture();

        // Add this declaration before the error checking loops
        GLenum err;

        // Particle update
        glUseProgram(computeProgram);
        while ((err = glGetError()) != GL_NO_ERROR) {
            printf("OpenGL error after using compute program: %d, Location: %s\n", err, GetErrorLocation());
        }

        if (mousePositionLocation != -1) glUniform2f(mousePositionLocation, renderMouseX, renderMouseY);
        if (deltaTimeLocation != -1) glUniform1f(deltaTimeLocation, deltaTime);
        if (iResolutionLocationCompute != -1) glUniform2f(iResolutionLocationCompute, (float)screenWidth, (float)screenHeight);
        while ((err = glGetError()) != GL_NO_ERROR) {
            printf("OpenGL error after setting uniforms in compute program: %d, Location: %s\n", err, GetErrorLocation());
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers[currentBuffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleBuffers[1-currentBuffer]);
        glBindImageTexture(2, densityTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
        while ((err = glGetError()) != GL_NO_ERROR) {
            printf("OpenGL error after binding buffers and textures: %d, Location: %s\n", err, GetErrorLocation());
        }

        glDispatchCompute((NUM_PARTICLES + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render heatmap
        glUseProgram(heatmapRenderProgram);
        glUniform2f(glGetUniformLocation(heatmapRenderProgram, "iResolution"), (float)screenWidth, (float)screenHeight);
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Render particles (optional, for debugging)
        glUseProgram(program);
        glUniform2f(iResolutionLocation, (float)screenWidth, (float)screenHeight);
        glBindVertexArray(vertexArray);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers[1-currentBuffer]);
        glPointSize(1.0f);
        glDrawArraysInstanced(GL_POINTS, 0, 1, NUM_PARTICLES);

        // Render cursor
        glUniform2f(glGetUniformLocation(program, "mousePos"), renderMouseX, renderMouseY);
        glUniform1i(glGetUniformLocation(program, "isCursor"), 1);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, 1);
        glPointSize(1.0f);

        while ((err = glGetError()) != GL_NO_ERROR) {
            printf("OpenGL error after rendering: %d, Location: %s\n", err, GetErrorLocation());
        }

        SwapBuffers(hdc);
        currentBuffer = 1 - currentBuffer;
        printf("Swapped particle buffers. Current buffer: %d\n", currentBuffer);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    ChangeDisplaySettings(NULL, 0);
    ShowCursor(TRUE);
    ExitProcess(0);
}

// Add this helper function at the end of the file
const char* GetErrorLocation(void) {
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "Line %d", __LINE__);
    return buffer;
}
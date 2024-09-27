#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <stdlib.h>
#include <time.h>

// Global variable declarations
GLint iTimeLocation;
GLint iResolutionLocation;
float startTime;
LARGE_INTEGER frequency;
LARGE_INTEGER lastTime;
LARGE_INTEGER currentTime;
const float targetFPS = 100.0f;
const float targetFrameTime = 1.0f / targetFPS;

// Function pointer declarations
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hDC, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);

// OpenGL function pointers
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLDISPATCHCOMPUTEPROC glDispatchCompute;
PFNGLMEMORYBARRIERPROC glMemoryBarrier;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

// Function declarations
void LoadOpenGLFunctions();
GLuint CompileShader(const char *source, GLenum type);
void CheckShaderCompileStatus(GLuint shader);
void CheckProgramLinkStatus(GLuint program);
char* LoadShader(const char *filename);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

// Add these global variables
GLuint computeProgram;
GLuint particleBuffer;
GLint mousePositionLocation;
GLint deltaTimeLocation;
const int NUM_PARTICLES = 10000000; // We'll start with 1 million and scale up
const int WORK_GROUP_SIZE = 256;
GLuint iResolutionLocationCompute;
GLuint vertexArray;
GLuint vertexBuffer;

void LoadOpenGLFunctions() {
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1); // Enable vsync
    }
    
    // Add these new function pointers
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)wglGetProcAddress("glDispatchCompute");
    glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)wglGetProcAddress("glMemoryBarrier");
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
    glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
    glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)wglGetProcAddress("glDrawArraysInstanced");
    glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)wglGetProcAddress("glVertexAttribDivisor");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
}

char* LoadShader(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for shader file: %s\n", filename);
        fclose(file);
        return NULL;
    }
    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CLOSE || (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    DEVMODE screenSettings = {0};
    screenSettings.dmSize = sizeof(screenSettings);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    screenSettings.dmPelsWidth = screenWidth;
    screenSettings.dmPelsHeight = screenHeight;
    screenSettings.dmBitsPerPel = 32;
    screenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);

    HWND hwnd = CreateWindowEx(0, (LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, screenWidth, screenHeight, 0, 0, hInstance, 0);
    HDC hdc = GetDC(hwnd);

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    LoadOpenGLFunctions();

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
        initialParticleData[i*4] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        initialParticleData[i*4+1] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        initialParticleData[i*4+2] = 0.0f;
        initialParticleData[i*4+3] = 0.0f;
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

    char *fragmentShaderSource = LoadShader("shader.frag");
    if (!fragmentShaderSource) {
        return -1;
    }
    GLuint shader = CompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);

    char *vertexShaderSource = LoadShader("shader.vert");
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
    startTime = GetTickCount() / 1000.0f;
    free(fragmentShaderSource);
    free(vertexShaderSource);

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    // Load compute shader
    char *computeShaderSource = LoadShader("particle_update.comp");
    if (!computeShaderSource) {
        return -1;
    }
    GLuint computeShader = CompileShader(computeShaderSource, GL_COMPUTE_SHADER);
    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    CheckProgramLinkStatus(computeProgram);
    mousePositionLocation = glGetUniformLocation(computeProgram, "mousePosition");
    deltaTimeLocation = glGetUniformLocation(computeProgram, "deltaTime");
    iResolutionLocationCompute = glGetUniformLocation(computeProgram, "iResolution");
    free(computeShaderSource);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MSG msg;
    int frameCount = 0;
    int currentBuffer = 0;
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        QueryPerformanceCounter(&currentTime);
        float deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

        // Update particles
        glUseProgram(computeProgram);
        
        // Get mouse position
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hwnd, &mousePos);
        glUniform2f(mousePositionLocation, (float)mousePos.x, (float)(screenHeight - mousePos.y));
        
        glUniform1f(deltaTimeLocation, deltaTime);
        glUniform2f(iResolutionLocationCompute, (float)screenWidth, (float)screenHeight);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers[currentBuffer]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleBuffers[1-currentBuffer]);

        // Calculate the number of work groups needed
        int numWorkGroups = (NUM_PARTICLES + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        glDispatchCompute(numWorkGroups, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Render particles
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set clear color to black
        glUseProgram(program);
        float currentTimeSeconds = GetTickCount() / 1000.0f - startTime;
        glUniform1f(iTimeLocation, currentTimeSeconds);
        glUniform2f(iResolutionLocation, (float)screenWidth, (float)screenHeight);
        glBindVertexArray(vertexArray);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffers[1-currentBuffer]);
        glDrawArraysInstanced(GL_POINTS, 0, 1, NUM_PARTICLES);

        SwapBuffers(hdc);
        currentBuffer = 1 - currentBuffer;

        lastTime = currentTime;
        frameCount++;

        // Debug output
        if (frameCount % 60 == 0) {
            printf("Frame: %d, Mouse: (%ld, %ld), DeltaTime: %f\n", frameCount, mousePos.x, mousePos.y, deltaTime);
        }

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

GLuint CompileShader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    CheckShaderCompileStatus(shader);
    return shader;
}

void CheckShaderCompileStatus(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
}

void CheckProgramLinkStatus(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
}

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
// #include <GL/glext.h> // Comment out this line
#include <stdio.h>

// Add these includes
#include <GL/glext.h>
#include <GL/wglext.h>

// Add these global variable declarations
GLint iTimeLocation;
GLint iResolutionLocation;
float startTime;
FILETIME lastTime;
FILETIME currentTime;
const float targetFPS = 60.0f;
const float targetFrameTime = 1.0f / targetFPS;

// Add these function pointer declarations
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hDC, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);

// Add these near the top of the file
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif

#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif

// Add these function declarations near the top of the file, after the other declarations
void CheckShaderCompileStatus(GLuint shader);
void CheckProgramLinkStatus(GLuint program);

// Add these OpenGL function pointers
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;

// OpenGL function pointers
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
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;

// Function to load OpenGL functions
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
}

// Function declarations
void PlayMidiNote();
GLuint CompileShader(const char *source, GLenum type);
// Pixel format descriptor
PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,                          // Version number
    PFD_DRAW_TO_WINDOW |        // Support window
    PFD_SUPPORT_OPENGL |        // Support OpenGL
    PFD_DOUBLEBUFFER,           // Double buffered
    PFD_TYPE_RGBA,              // RGBA type
    32,                         // 32-bit color depth
    0, 0, 0, 0, 0, 0,           // Ignore color bits
    0,                          // No alpha buffer
    0,                          // Ignore shift bit
    0,                          // No accumulation buffer
    0, 0, 0, 0,                 // Ignore accumulation bits
    24,                         // 24-bit z-buffer
    0,                          // No stencil buffer
    0,                          // No auxiliary buffer
    PFD_MAIN_PLANE,             // Main layer
    0,                          // Reserved
    0, 0, 0                     // Layer masks ignored
};

// Simple file loader for the shader
char* LoadShader(const char *filename) {
    FILE *file = fopen(filename, "r");
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = 0;
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
    // Set fullscreen
    DEVMODE screenSettings;
    memset(&screenSettings, 0, sizeof(screenSettings));
    screenSettings.dmSize = sizeof(screenSettings);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    screenSettings.dmPelsWidth = screenWidth;
    screenSettings.dmPelsHeight = screenHeight;
    screenSettings.dmBitsPerPel = 32;
    screenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);

    // Create window
    HWND hwnd = CreateWindowEx(0, (LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, screenWidth, screenHeight, 0, 0, hInstance, 0);
    HDC hdc = GetDC(hwnd);
    

    // Set pixel format
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    // Create OpenGL context
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    // Load OpenGL functions
    LoadOpenGLFunctions();

    // Hide cursor
    ShowCursor(FALSE);

    // Load and compile shader
    char *fragmentShaderSource = LoadShader("shader.frag");
    GLuint shader = CompileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    CheckProgramLinkStatus(program);
    glUseProgram(program);
    iTimeLocation = glGetUniformLocation(program, "iTime");
    iResolutionLocation = glGetUniformLocation(program, "iResolution");
    startTime = GetTickCount() / 1000.0f;
    free(fragmentShaderSource);

    // Play MIDI note
    //PlayMidiNote();

    // Initialize timer
    GetSystemTimeAsFileTime(&lastTime);

    // Main loop
    MSG msg;
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Calculate delta time
        GetSystemTimeAsFileTime(&currentTime);
        ULARGE_INTEGER ul1, ul2;
        ul1.LowPart = lastTime.dwLowDateTime;
        ul1.HighPart = lastTime.dwHighDateTime;
        ul2.LowPart = currentTime.dwLowDateTime;
        ul2.HighPart = currentTime.dwHighDateTime;
        float deltaTime = (float)(ul2.QuadPart - ul1.QuadPart) / 10000000.0f; // Convert 100-nanosecond intervals to seconds

        if (deltaTime >= targetFrameTime) {
            // Update uniforms
            float currentTimeSeconds = GetTickCount() / 1000.0f - startTime;
            glUniform1f(iTimeLocation, currentTimeSeconds);
            glUniform2f(iResolutionLocation, (float)screenWidth, (float)screenHeight);

            // Render simple shader effect

            glClear(GL_COLOR_BUFFER_BIT);
            glRects(-1, -1, 1, 1);  // Placeholder for shader rendering
            SwapBuffers(hdc);

            // Update lastTime
            lastTime = currentTime;
        }
    }

    // Clean up
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    ChangeDisplaySettings(NULL, 0);  // Restore screen settings
    ShowCursor(TRUE);
    ExitProcess(0);
}

// Compile shader function
GLuint CompileShader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    CheckShaderCompileStatus(shader);
    return shader;
}

// MIDI Note Playback
void PlayMidiNote() {
    HMIDIOUT hMidiOut;
    midiOutOpen(&hMidiOut, 0, 0, 0, CALLBACK_NULL);
    midiOutShortMsg(hMidiOut, 0x007F3C90);  // Example MIDI note
}

// Add these new functions
void CheckShaderCompileStatus(GLuint shader) {
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
}

void CheckProgramLinkStatus(GLuint program) {
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
}

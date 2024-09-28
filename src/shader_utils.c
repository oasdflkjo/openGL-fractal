#include "shader_utils.h"
#include <stdio.h>
#include <stdlib.h>

void LoadShaderUtilsFunctions() {
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");  // Add this line
}

char* LoadShader(const char *filename) {
    char fullPath[MAX_PATH];
    GetFullPathNameA(filename, MAX_PATH, fullPath, NULL);
    printf("Attempting to open shader file: %s\n", fullPath);

    FILE *file = fopen(filename, "rb");  // Open in binary mode
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", fullPath);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for shader file: %s\n", fullPath);
        fclose(file);
        return NULL;
    }
    size_t read = fread(buffer, 1, length, file);
    if (read != length) {
        fprintf(stderr, "Failed to read entire shader file: %s\nExpected %ld bytes, read %zu bytes\n", 
                fullPath, length, read);
        // Print the content that was read
        printf("Content read:\n");
        for (size_t i = 0; i < read; i++) {
            printf("%c", buffer[i]);
        }
        printf("\n");
        // Print the hex values of the content
        printf("Hex values:\n");
        for (size_t i = 0; i < read; i++) {
            printf("%02X ", (unsigned char)buffer[i]);
        }
        printf("\n");
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[read] = '\0';  // Null-terminate the string
    fclose(file);

    printf("Successfully read shader file %s (%zu bytes):\n%s\n", fullPath, read, buffer);

    return buffer;
}

GLuint CompileShader(const char *source, GLenum type, const char *filename) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        
        char *infoLog = (char*)malloc(logLength);
        if (infoLog) {
            glGetShaderInfoLog(shader, logLength, NULL, infoLog);
            const char *shaderTypeName = 
                (type == GL_VERTEX_SHADER) ? "VERTEX" :
                (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" :
                (type == GL_COMPUTE_SHADER) ? "COMPUTE" : "UNKNOWN";
            fprintf(stderr, "ERROR::%s_SHADER::COMPILATION_FAILED\nFile: %s\n%s\n", shaderTypeName, filename, infoLog);
            free(infoLog);
        } else {
            fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\nFile: %s\nFailed to allocate memory for error message\n", filename);
        }
        
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void CheckShaderCompileStatus(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        
        char *infoLog = (char*)malloc(logLength);
        if (infoLog) {
            glGetShaderInfoLog(shader, logLength, NULL, infoLog);
            fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
            free(infoLog);
        } else {
            fprintf(stderr, "ERROR::SHADER::COMPILATION_FAILED\nFailed to allocate memory for error message\n");
        }
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
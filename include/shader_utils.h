#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

// Declare OpenGL function pointers as extern
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;

void LoadShaderUtilsFunctions();
char* LoadShader(const char *filename);
GLuint CompileShader(const char *source, GLenum type);
void CheckShaderCompileStatus(GLuint shader);
void CheckProgramLinkStatus(GLuint program);

#endif // SHADER_UTILS_H
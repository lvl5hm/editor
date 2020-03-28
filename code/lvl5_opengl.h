#ifndef LVL5_OPENGL_H

#include "lvl5_string.h"

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

#ifndef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)
#endif


#include <GL/gl.h>
#include <KHR/glext.h>






typedef void FNGLCLEARPROC(GLuint buffer_bit);
typedef void FNGLCLEARCOLORPROC(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
typedef void FNGLDRAWARRAYSPROC(GLenum mode, GLint first, GLsizei count);
typedef void FNGLDRAWELEMENTSPROC(GLenum mode, GLsizei count, GLenum type, GLvoid *indices);

typedef void FNGLTEXPARAMETERI(GLenum thing, GLenum param, GLint value);
typedef void FNGLTEXPARAMETERFV(GLenum thing, GLenum param, GLfloat *value);
typedef void FNGLGENTEXTURES(GLint count, GLuint *id);
typedef void FNGLBINDTEXTURE(GLenum type, GLuint id);
typedef void FNGLTEXIMAGE2D(GLenum target,
                            GLint level,
                            GLint internalFormat,
                            GLsizei width,
                            GLsizei height,
                            GLint border,
                            GLenum format,
                            GLenum type,
                            const GLvoid *data);

typedef void FNGLGENERATEMIPMAPPROC(GLenum thing);
typedef void FNGLENABLEPROC(GLenum thing);
typedef void FNGLDISABLEPROC(GLenum thing);
typedef void FNGLBLENDFUNCPROC(GLenum src, GLenum dst);
typedef void FNGLDELETETEXTURESPROC(GLsizei n, GLuint *textures);
typedef void FNGLVIEWPORTPROC(GLint x, GLint y, GLsizei width, GLsizei height);



typedef struct {
  PFNGLVERTEXATTRIBIPOINTERPROC VertexAttribIPointer;
  PFNGLGENBUFFERSPROC GenBuffers;
  PFNGLBINDBUFFERPROC BindBuffer;
  PFNGLBUFFERDATAPROC BufferData;
  PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
  PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
  PFNGLCREATESHADERPROC CreateShader;
  PFNGLSHADERSOURCEPROC ShaderSource;
  PFNGLCOMPILESHADERPROC CompileShader;
  PFNGLGETSHADERIVPROC GetShaderiv;
  PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
  PFNGLCREATEPROGRAMPROC CreateProgram;
  PFNGLATTACHSHADERPROC AttachShader;
  PFNGLLINKPROGRAMPROC LinkProgram;
  PFNGLVALIDATEPROGRAMPROC ValidateProgram;
  PFNGLDELETESHADERPROC DeleteShader;
  PFNGLUSEPROGRAMPROC UseProgram;
  PFNGLUSESHADERPROGRAMEXTPROC UseShaderProgramEXT;
  PFNGLDEBUGMESSAGECALLBACKPROC DebugMessageCallback;
  PFNGLENABLEIPROC Enablei;
  PFNGLDEBUGMESSAGECONTROLPROC DebugMessageControl;
  PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
  PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
  PFNGLBINDVERTEXARRAYPROC BindVertexArray;
  PFNGLDELETEBUFFERSPROC DeleteBuffers;
  PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
  PFNGLVERTEXATTRIBDIVISORPROC VertexAttribDivisor;
  PFNGLDRAWARRAYSINSTANCEDPROC DrawArraysInstanced;
  
  PFNGLUNIFORM4FPROC Uniform4f;
  PFNGLUNIFORM3FPROC Uniform3f;
  PFNGLUNIFORM2FPROC Uniform2f;
  PFNGLUNIFORM1FPROC Uniform1f;
  PFNGLUNIFORMMATRIX2FVPROC UniformMatrix2fv;
  PFNGLUNIFORMMATRIX3FVPROC UniformMatrix3fv;
  PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;
  
  FNGLDISABLEPROC *Disable;
  FNGLCLEARCOLORPROC *ClearColor;
  FNGLCLEARPROC *Clear;
  FNGLDRAWARRAYSPROC *DrawArrays;
  FNGLDRAWELEMENTSPROC *DrawElements;
  FNGLTEXPARAMETERI *TexParameteri;
  FNGLGENTEXTURES *GenTextures;
  FNGLBINDTEXTURE *BindTexture;
  FNGLTEXIMAGE2D *TexImage2D;
  FNGLTEXPARAMETERFV *TexParameterfv;
  FNGLGENERATEMIPMAPPROC *GenerateMipmap;
  FNGLENABLEPROC *Enable;
  FNGLBLENDFUNCPROC *BlendFunc;
  FNGLDELETETEXTURESPROC *DeleteTextures;
  FNGLVIEWPORTPROC *Viewport;
} gl_Funcs;

typedef struct {
  String vertex;
  String fragment;
} gl_Parse_Result;

gl_Parse_Result gl_parse_glsl(String src) {
  String vertex_header = const_string("#shader vertex");
  i64 vertex_index = find_index(src, vertex_header, 0);
  assert(vertex_index != -1);
  String frag_header = const_string("#shader fragment");
  i64 frag_index = find_index(src, frag_header, 0);
  assert(frag_index != -1);
  
  gl_Parse_Result result;
  result.vertex = make_string(src.data + vertex_index + vertex_header.count, 
                              frag_index - vertex_index - vertex_header.count);
  result.fragment = make_string(src.data + frag_index + frag_header.count,
                                src.count - frag_index - frag_header.count);
  
  return result;
}

GLuint gl_compile_shader(gl_Funcs gl, u32 type, String src) {
  GLuint id = gl.CreateShader(type);
  gl.ShaderSource(id, 1, &src.data, (i32 *)&src.count);
  gl.CompileShader(id);
  
  i32 compile_status;
  gl.GetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
  if (compile_status == GL_FALSE)
  {
    i32 length;
    gl.GetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    char *message = scratch_push_array(char, length);
    gl.GetShaderInfoLog(id, length, &length, message);
    assert(false);
  }
  
  return id;
}

GLuint gl_create_shader(gl_Funcs gl, String vertex_src, String fragment_src) {
  GLuint program = gl.CreateProgram();
  GLuint vertex_shader = gl_compile_shader(gl, GL_VERTEX_SHADER, vertex_src);
  GLuint fragment_shader = gl_compile_shader(gl, GL_FRAGMENT_SHADER, fragment_src);
  
  gl.AttachShader(program, vertex_shader);
  gl.AttachShader(program, fragment_shader);
  gl.LinkProgram(program);
  gl.ValidateProgram(program);
  
  gl.DeleteShader(vertex_shader);
  gl.DeleteShader(fragment_shader);
  
  return program;
}

GLuint gl_create_shader_from_file(gl_Funcs gl, String (*os_read_entire_file)(String), String file_name) {
  String shader_file = os_read_entire_file(file_name);
  gl_Parse_Result shader_src = gl_parse_glsl(shader_file);
  GLuint shader = gl_create_shader(gl, shader_src.vertex, shader_src.fragment);
  return shader;
}

void gl_set_uniform_m3(gl_Funcs gl, GLint program,
                       char *name, M3 *value, u32 count) {
  GLint uniform_loc = gl.GetUniformLocation(program, name);
  gl.UniformMatrix3fv(uniform_loc, count, GL_TRUE, (GLfloat *)value);
}


void gl_set_uniform_m4(gl_Funcs gl, GLint program,
                       char *name, M4 *value, u32 count) {
  GLint uniform_loc = gl.GetUniformLocation(program, name);
  gl.UniformMatrix4fv(uniform_loc, count, GL_TRUE, (GLfloat *)value);
}


void gl_set_uniform_v4(gl_Funcs gl, GLint program, char *name, V4 value) {
  GLint uniform_loc = gl.GetUniformLocation(program, name);
  gl.Uniform4f(uniform_loc, value.x, value.y, value.z, value.w);
}

#define LVL5_OPENGL_H
#endif
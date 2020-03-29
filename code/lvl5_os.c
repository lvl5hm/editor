#ifndef LVL5_OS

#include "lvl5_string.h"
#include "lvl5_os.h"
#include "lvl5_opengl.h"
#include "lvl5_stretchy_buffer.h"

#ifdef _MSC_VER
#define os_WIN32 1
#endif


#if os_WIN32
#include <Windows.h>
#include <Windowsx.h>
#include <KHR/wglext.h>


bool os_char_is_delimeter(char c) {
  bool result = c == '/' || c == '\\';
  return result;
}

String os_get_file_name_from_path(String path) {
  u64 index = path.count - 1;
  while (index > 0) {
    if (os_char_is_delimeter(path.data[index])) 
    {
      break;
    }
    index--;
  }
  
  u64 count = path.count - index;
  String result = alloc_string(path.data + index + 1, count);
  return result;
}

String os_get_file_ext(String path) {
  u64 index = path.count - 1;
  while (index > 0) {
    if (path.data[index] == '.') 
    {
      break;
    }
    index--;
  }
  
  u64 count = path.count - index;
  String result = make_string(path.data + index + 1, count);
  return result;
}

String os_get_file_base(String path) {
  String name = os_get_file_name_from_path(path);
  u64 index = 0;
  while (index < name.count) {
    if (name.data[index] == '.') {
      break;
    }
    index++;
  }
  String result = alloc_string(name.data, index);
  return result;
}

String os_get_parent_dir(String path) {
  u64 index = path.count - 1;
  while (index > 0) {
    if (os_char_is_delimeter(path.data[index])) 
    {
      break;
    }
    index--;
  }
  
  u64 count = path.count - index;
  String result = alloc_string(path.data, index);
  return result;
}

os_File os_open_file(String file_name) {
  HANDLE handle = CreateFileA((LPCSTR)to_c_string(file_name),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              0);
  
  assert(handle != INVALID_HANDLE_VALUE);
  return (os_File)handle;
}

// the array and all file names inside must be freed at some point
String *os_get_file_names(String path) {
  String *result = sb_new(String, 16);
  sb_push(result, alloc_string("..", 2));
  
  WIN32_FIND_DATAA findData;
  String wildcard = concat(path, const_string("\\*.*"));
  char *wildcard_c = to_c_string(wildcard);
  HANDLE file = FindFirstFileA(wildcard_c, &findData);
  
  while (file != INVALID_HANDLE_VALUE) {
    if (!c_string_compare(findData.cFileName, ".") &&
        !c_string_compare(findData.cFileName, "..")) {
      
      char *src = findData.cFileName;
      i32 name_length = c_string_length(src);
      char *dst = (char *)alloc(name_length);
      copy_memory_slow(dst, src, name_length);
      
      sb_push(result, make_string(dst, name_length));
    }
    
    b32 next_file_found = FindNextFileA(file, &findData);
    if (!next_file_found) {
      break;
    }
  }
  
  return result;
}

u64 os_get_file_size(os_File file) {
  u64 result = 0;
  LARGE_INTEGER file_size_li;
  
  if (GetFileSizeEx((HANDLE)file, &file_size_li))
  {
    u64 file_size = file_size_li.QuadPart;
    result = file_size;
  }
  return result;
}

void os_close_file(os_File file) {
  CloseHandle((HANDLE)file);
}

void os_read_file(os_File file, void *dst, u64 offset, u64 size) {
  OVERLAPPED overlapped = {0};
  overlapped.Offset = (u32)((offset >> 0) & 0xFFFFFFFF);
  overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);
  
  DWORD bytes_read;
  ReadFile(
    (HANDLE)file,
    dst,
    (u32)size,
    &bytes_read,
    &overlapped);
  assert(bytes_read == size);
}

void os_write_file(os_File file, void *data, u64 offset, u64 size) {
  DWORD pos = SetFilePointer((HANDLE)file, (u32)offset, null, FILE_BEGIN);
  assert(pos == offset);
  
  DWORD bytes_written;
  BOOL written = WriteFile((HANDLE)file, data, (u32)size, &bytes_written, null);
  assert(written);
  assert(bytes_written == size);
}

os_Dll os_load_dll(String name) {
  HMODULE dll = LoadLibraryA(to_c_string(name));
  assert(dll);
  return (os_Dll)dll;
}

void *os_load_function(os_Dll dll, String name) {
  void *result = (void *)GetProcAddress(dll, to_c_string(name));
  assert(result);
  return result;
}

void os_free_dll(os_Dll dll) {
  FreeLibrary(dll);
}

void *get_any_gl_func_address(const char *name) {
  void *p = (void *)wglGetProcAddress(name);
  if(p == 0 ||
     (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
     (p == (void*)-1)) {
    HMODULE module = LoadLibraryA("opengl32.dll");
    p = (void *)GetProcAddress(module, name);
  }
  
  return p;
}

PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

gl_Funcs gl_load_functions() {
  gl_Funcs funcs = {0};
  
#define load_opengl_proc(name) *(Mem_Size *)&(funcs.name) = (Mem_Size)get_any_gl_func_address("gl"#name)
  
  load_opengl_proc(VertexAttribIPointer);
  load_opengl_proc(BindBuffer);
  load_opengl_proc(GenBuffers);
  load_opengl_proc(BufferData);
  load_opengl_proc(VertexAttribPointer);
  load_opengl_proc(EnableVertexAttribArray);
  load_opengl_proc(CreateShader);
  load_opengl_proc(ShaderSource);
  load_opengl_proc(CompileShader);
  load_opengl_proc(GetShaderiv);
  load_opengl_proc(GetShaderInfoLog);
  load_opengl_proc(CreateProgram);
  load_opengl_proc(AttachShader);
  load_opengl_proc(LinkProgram);
  load_opengl_proc(ValidateProgram);
  load_opengl_proc(DeleteShader);
  load_opengl_proc(UseProgram);
  load_opengl_proc(DebugMessageCallback);
  load_opengl_proc(Enablei);
  load_opengl_proc(DebugMessageControl);
  load_opengl_proc(GetUniformLocation);
  load_opengl_proc(GenVertexArrays);
  load_opengl_proc(BindVertexArray);
  load_opengl_proc(DeleteBuffers);
  load_opengl_proc(DeleteVertexArrays);
  load_opengl_proc(VertexAttribDivisor);
  load_opengl_proc(DrawArraysInstanced);
  
  load_opengl_proc(Uniform4f);
  load_opengl_proc(Uniform3f);
  load_opengl_proc(Uniform2f);
  load_opengl_proc(Uniform1f);
  
  load_opengl_proc(UniformMatrix2fv);
  load_opengl_proc(UniformMatrix3fv);
  load_opengl_proc(UniformMatrix4fv);
  
  load_opengl_proc(ClearColor);
  load_opengl_proc(Clear);
  load_opengl_proc(DrawArrays);
  load_opengl_proc(DrawElements);
  load_opengl_proc(TexParameteri);
  load_opengl_proc(GenTextures);
  load_opengl_proc(BindTexture);
  load_opengl_proc(TexImage2D);
  load_opengl_proc(GenerateMipmap);
  load_opengl_proc(BlendFunc);
  load_opengl_proc(Enable);
  load_opengl_proc(DeleteTextures);
  load_opengl_proc(Viewport);
  load_opengl_proc(Disable);
  
  return funcs;
}


void APIENTRY opengl_debug_callback(GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar* message,
                                    const void* userParam) {
#if 1
  OutputDebugStringA(message);
  __debugbreak();
#endif
}

typedef struct {
  HWND window;
  WNDCLASSA window_class;
} win32_Window;

win32_Window win32_window_create(HINSTANCE instance, WNDPROC WindowProc, bool visible, i32 width, i32 height) {
  WNDCLASSA window_class = {0};
  window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = instance;
  window_class.hCursor = LoadCursor((HINSTANCE)0, IDC_ARROW);
  window_class.lpszClassName = "test_window_class";
  
  ATOM window_class_name = RegisterClassA(&window_class);
  assert(window_class_name);
  
  HWND window = CreateWindowA(window_class.lpszClassName,
                              "test_window_name",
                              WS_OVERLAPPEDWINDOW|(visible ? WS_VISIBLE : 0),
                              CW_USEDEFAULT, // x
                              CW_USEDEFAULT, // y
                              width,
                              height,
                              0,
                              0,
                              instance,
                              0);
  assert(window);
  
  win32_Window result;
  result.window = window;
  result.window_class = window_class;
  return result;
}

void win32_window_destroy(win32_Window window, HINSTANCE instance) {
  DestroyWindow(window.window);
  b32 window_class_unregistered = UnregisterClassA(window.window_class.lpszClassName, instance);
  assert(window_class_unregistered);
}

HWND win32_init_opengl(gl_Funcs *gl, HINSTANCE instance, WNDPROC WindowProc, i32 width, i32 height) {
  // NOTE(lvl5): create a fake context
  win32_Window fake_window = win32_window_create(instance, WindowProc, false, width, height);
  HGLRC fake_opengl_context;
  HDC fake_device_context = GetDC(fake_window.window);
  {
    PIXELFORMATDESCRIPTOR pixel_format_descriptor = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,
      PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,    // Flags
      PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
      32,                   // Colordepth of the framebuffer.
      0, 0, 0, 0, 0, 0,
      0,
      0,
      0,
      0, 0, 0, 0,
      24,                   // Number of bits for the depthbuffer
      8,                    // Number of bits for the stencilbuffer
      0,                    // Number of Aux buffers in the framebuffer.
      PFD_MAIN_PLANE,
      0,
      0, 0, 0
    };
    
    int pixel_format = ChoosePixelFormat(fake_device_context,
                                         &pixel_format_descriptor);
    assert(pixel_format);
    
    b32 pixel_format_set = SetPixelFormat(fake_device_context, pixel_format, 
                                          &pixel_format_descriptor);
    assert(pixel_format_set);
    
    fake_opengl_context = wglCreateContext(fake_device_context);
    assert(fake_opengl_context);
    
    b32 context_made_current = wglMakeCurrent(fake_device_context, 
                                              fake_opengl_context);
    assert(context_made_current);
  }
  
  // NOTE(lvl5): load functions
  *gl = gl_load_functions();
  wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
  wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
  wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
  
  // NOTE(lvl5): delete fake window and context
  b32 fake_device_context_released = ReleaseDC(fake_window.window, 
                                               fake_device_context);
  assert(fake_device_context_released);
  wglDeleteContext(fake_opengl_context);
  win32_window_destroy(fake_window, instance);
  
  // NOTE(lvl5): now we need to create an actual context
  win32_Window window = win32_window_create(instance, WindowProc, true, width, height);
  HDC device_context = GetDC(window.window);
  
  int attributes[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_DEPTH_BITS_ARB, 24,
    WGL_STENCIL_BITS_ARB, 8,
    WGL_SAMPLE_BUFFERS_ARB, 1, // Number of buffers (must be 1)
    WGL_SAMPLES_ARB, 4,        // Number of samples
    0
  };
  
  i32 pixel_format;
  u32 num_formats;
  wglChoosePixelFormatARB(device_context, attributes, null, 1, &pixel_format,
                          &num_formats);
  assert(num_formats);
  
  b32 pixel_format_set = SetPixelFormat(device_context, pixel_format, null);
  assert(pixel_format_set);
  
  int context_attributes[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0,
  };
  HGLRC opengl_context = wglCreateContextAttribsARB(device_context,
                                                    null, context_attributes);
  assert(opengl_context);
  b32 context_made_current = wglMakeCurrent(device_context, opengl_context);
  assert(context_made_current);
  b32 interval_set = wglSwapIntervalEXT(1);
  
  glEnable(GL_DEBUG_OUTPUT);
  gl->DebugMessageCallback(opengl_debug_callback, 0);
  GLuint unusedIds = 0;
  gl->DebugMessageControl(GL_DONT_CARE,
                          GL_DONT_CARE,
                          GL_DONT_CARE,
                          0,
                          &unusedIds,
                          true);
  
  
  SetProcessDPIAware();
  return window.window;
}


#endif


// TODO(lvl5): make an event stack per each window
#define os_MAX_EVENT_COUNT 256
typedef struct os_State {
  os_Event events[os_MAX_EVENT_COUNT];
  i32 event_count;
  HDC device_context;
} os_State;

os_State __os_global_state = {0};

void os_push_event(os_Event event) {
  assert(__os_global_state.event_count < os_MAX_EVENT_COUNT);
  __os_global_state.events[__os_global_state.event_count++] = event;
}

b32 os_pop_event(os_Event *event) {
  b32 result = false;
  if (__os_global_state.event_count > 0) {
    result = true;
    *event = __os_global_state.events[--__os_global_state.event_count];
  }
  return result;
}


#if os_WIN32

os_File_Info os_get_file_info(String file_name) {
  WIN32_FIND_DATAA find_data;
  HANDLE file_handle = FindFirstFileA(
    to_c_string(file_name),
    &find_data);
  
  os_File_Info info = {0};
  
  if (file_handle != INVALID_HANDLE_VALUE) {
    info.exists = true;
    FindClose(file_handle);
    FILETIME write_time = find_data.ftLastWriteTime;
    
    info.write_time = ((u64)write_time.dwLowDateTime << 32) |
      write_time.dwLowDateTime;
  }
  
  return info;
}

bool os_copy_file(String dst_str, String src_str) {
  u64 mark = scratch_get_mark();
  
  char *src = to_c_string(src_str);
  char *dst = to_c_string(dst_str);
  b32 copy_success = CopyFileA(src, dst, false);
  
  scratch_set_mark(mark);
  return (bool)copy_success;
}

String os_get_build_dir() {
  String full_path;
  full_path.data = scratch_push_array(char, MAX_PATH);
  full_path.count = GetModuleFileNameA(0, full_path.data, MAX_PATH);
  assert(full_path.count);
  
  i64 last_slash_index = find_last_index(full_path, const_string("\\"), full_path.count);
  String result = substring(full_path, 0, last_slash_index + 1);
  
  return result;
}


String os_get_work_dir() {
  String result = const_string("..\\data\\");
  return result;
}

String os_read_entire_file(String file_name) {
  String path = os_get_work_dir();
  String full_name = concat(path, file_name);
  char *c_file_name = to_c_string(full_name);
  HANDLE file = CreateFileA(c_file_name,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            0);
  
  assert(file != INVALID_HANDLE_VALUE);
  
  LARGE_INTEGER file_size_li;
  GetFileSizeEx(file, &file_size_li);
  u64 file_size = file_size_li.QuadPart;
  
  char *buffer = (char *)malloc(file_size);
  u32 bytes_read;
  ReadFile(file, buffer, (DWORD)file_size, (LPDWORD)&bytes_read, 0);
  assert(bytes_read == file_size);
  
  CloseHandle(file);
  
  String result;
  result.data = buffer;
  result.count = file_size;
  
  return result;
}


LRESULT CALLBACK os_window_proc(HWND   window,
                                UINT   message,
                                WPARAM w_param,
                                LPARAM l_param) 
{
  LRESULT result = 0;
  switch (message) {
    case WM_SIZE: {
      os_push_event((os_Event){ .type = os_Event_Type_RESIZE });
    } break;
    
    case WM_CLOSE:
    case WM_QUIT: {
      os_push_event((os_Event){ .type = os_Event_Type_CLOSE });
    } break;
    
    default: result = DefWindowProc(window, message, w_param, l_param);
  }
  
  return result;
}

// TODO: leaking the window handle!!
os_Window os_create_window(gl_Funcs *gl, i32 width, i32 height) {
  win32_Window *window = alloc_struct(win32_Window);
  HINSTANCE instance = GetModuleHandle(null);
  window->window = win32_init_opengl(gl, instance, os_window_proc, width, height);
  __os_global_state.device_context = GetDC(window->window);
  return (os_Window *)window;
}


void os_collect_messages(os_Window _window, os_Input *input) {
  for (i32 key_index = 0; key_index < os_Keycode_count; key_index++) {
    os_Button *key = input->keys + key_index;
    key->went_up = false;
    key->went_down = false;
    key->pressed = false;
  }
  
  {
    os_Button *key = &input->mouse.left;
    key->went_up = false;
    key->went_down = false;
    key->pressed = false;
  }
  {
    os_Button *key = &input->mouse.right;
    key->went_up = false;
    key->went_down = false;
    key->pressed = false;
  }
  
  input->char_count = 0;
  win32_Window *window = (win32_Window *)_window;
  
  MSG message;
  while (PeekMessage(&message, window->window, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    switch (message.message) {
      case WM_LBUTTONDOWN: {
        os_Button *key = &input->mouse.left;
        if (key && !key->is_down) {
          key->went_down = true;
          key->is_down = true;
        }
      } break;
      case WM_RBUTTONDOWN: {
        os_Button *key = &input->mouse.right;
        if (key && !key->is_down) {
          key->went_down = true;
          key->is_down = true;
        }
      } break;
      
      case WM_LBUTTONUP: {
        os_Button *key = &input->mouse.left;
        if (key && key->is_down) {
          key->went_up = true;
          key->is_down = false;
        }
      } break;
      case WM_RBUTTONUP: {
        os_Button *key = &input->mouse.right;
        if (key && key->is_down) {
          key->went_up = true;
          key->is_down = false;
        }
      } break;
      
      case WM_MOUSEMOVE: {
        i32 x = GET_X_LPARAM(message.lParam);
        i32 y = GET_Y_LPARAM(message.lParam);
        
        input->mouse.p = v2_i(x, y);
      } break;
      
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP: {
#define KEY_IS_UP_BIT 1 << 31
#define SCAN_CODE_MASK 0x0000FF00
        bool key_is_down = !(message.lParam & KEY_IS_UP_BIT);
        
        // TODO(lvl5): map winodws keycode to custom keycode
        WPARAM keycode = message.wParam;
        os_Button *key = input->keys + keycode;
        
        if (key->is_down && !key_is_down) {
          key->went_up = true;
        } else if (!key->is_down && key_is_down) {
          key->went_down = true;
        }
        key->is_down = key_is_down;
        key->pressed = key_is_down;
        
        if (key->pressed) {
          u32 scan_code = message.lParam & SCAN_CODE_MASK;
          byte keyboard_state[256];
          GetKeyboardState(keyboard_state);
          u16 char_code_u16[2];
          i32 got_keycode = ToAscii((UINT)keycode, scan_code,
                                    keyboard_state, char_code_u16, 0);
          
          // TODO: multiple presses on same frame not handled
          // TODO: unicode
          if (got_keycode > 0) {
            assert(input->char_count < MAX_CHARS_PER_FRAME);
            input->chars[input->char_count++] = (char)char_code_u16[0];
          }
        }
        
        os_Event event = {0};
        event.type = os_Event_Type_BUTTON;
        event.button.keycode = keycode;
        os_push_event(event);
        
        if (key->went_down) {
          switch (keycode) {
            case VK_SHIFT: {
              input->shift = true;
            } break;
            case VK_MENU: {
              input->alt = true;
            } break;
            case VK_CONTROL: {
              input->ctrl = true;
            } break;
            case VK_NUMLOCK: {
              input->num_lock = !input->num_lock;
            } break;
            case VK_CAPITAL: {
              input->caps_lock = !input->caps_lock;
            } break;
          }
        } else if (key->went_up) {
          switch (keycode) {
            case VK_SHIFT: {
              input->shift = false;
            } break;
            case VK_MENU: {
              input->alt = false;
            } break;
            case VK_CONTROL: {
              input->ctrl = false;
            } break;
          }
        }
      } break;
      
      default: {
        os_window_proc(window->window,
                       message.message,
                       message.wParam,
                       message.lParam);
      } break;
    }
  }
}

void os_blit_to_screen() {
  SwapBuffers(__os_global_state.device_context);
}

void os_log(String str) {
  char *cstring = to_c_string(str);
  OutputDebugStringA(cstring);
}

V2 os_get_window_size(os_Window window) {
  RECT rect;
  b32 got_rect = GetClientRect(((win32_Window *)window)->window, &rect);
  assert(got_rect);
  V2 result = v2((f32)(rect.right - rect.left),
                 (f32)(rect.bottom - rect.top));
  return result;
}


#define os_entry_point() int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)






Font os_load_font(String file_name, String font_name_str, i32 font_size) {
  Font font = {0};
  
  {
    HDC device_context = CreateCompatibleDC(GetDC(null));
    char *font_name = to_c_string(font_name_str);
    b32 font_added = AddFontResourceExA(to_c_string(file_name), FR_PRIVATE, 0);
    assert(font_added);
    
    HFONT win_font = CreateFontA(
      font_size, 0,
      0, 0,
      FW_NORMAL, //weight
      FALSE, //italic
      FALSE, //underline
      FALSE, // strikeout
      DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,
      CLIP_DEFAULT_PRECIS,
      ANTIALIASED_QUALITY,
      DEFAULT_PITCH|FF_DONTCARE,
      font_name);
    
    u32 *font_buffer_pixels = null;
    
    i32 font_buffer_width = 256;
    i32 font_buffer_height = 256;
    
    BITMAPINFO font_buffer_bi = {0};
    font_buffer_bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    font_buffer_bi.bmiHeader.biWidth = font_buffer_width;
    font_buffer_bi.bmiHeader.biHeight = font_buffer_height;
    font_buffer_bi.bmiHeader.biPlanes = 1;
    font_buffer_bi.bmiHeader.biBitCount = 32;
    font_buffer_bi.bmiHeader.biCompression = BI_RGB;
    font_buffer_bi.bmiHeader.biSizeImage = 0;
    font_buffer_bi.bmiHeader.biXPelsPerMeter = 0;
    font_buffer_bi.bmiHeader.biYPelsPerMeter = 0;
    font_buffer_bi.bmiHeader.biClrUsed = 0;
    font_buffer_bi.bmiHeader.biClrImportant = 9;
    
    HBITMAP font_buffer = CreateDIBSection(
      device_context,
      &font_buffer_bi,
      DIB_RGB_COLORS,
      (void **)&font_buffer_pixels,
      0, 0);
    SelectObject(device_context, font_buffer);
    SelectObject(device_context, win_font);
    
    
    b32 metrics_bytes = GetOutlineTextMetrics(device_context, 0, null);
    OUTLINETEXTMETRIC *metric = (OUTLINETEXTMETRIC *)scratch_push_array(byte, metrics_bytes);
    GetOutlineTextMetrics(device_context, metrics_bytes, metric);
    
    
    
    SetBkColor(device_context, RGB(0, 0, 0));
    
    char first_codepoint = 0;
    char last_codepoint = '~';
    
    i32 codepoint_count = last_codepoint - first_codepoint + 1;
    Bitmap *codepoint_bitmaps = scratch_push_array(Bitmap, codepoint_count + 1);
    // 1 extra bitmap for white pixel
    
    font = (Font){
      .first_codepoint = first_codepoint,
      .advance = alloc_array(i8, codepoint_count),
      .kerning = alloc_array(i8, codepoint_count*codepoint_count),
      .origins = alloc_array(V2, codepoint_count),
      .codepoint_count = codepoint_count,
    };
    zero_memory_slow(font.advance, sizeof(i8)*codepoint_count);
    zero_memory_slow(font.kerning, sizeof(i8)*codepoint_count*codepoint_count);
    
    ABC *abcs = scratch_push_array(ABC, font.codepoint_count);
    GetCharABCWidthsW(device_context, font.first_codepoint,
                      font.first_codepoint + codepoint_count, abcs);
    
    for (char codepoint_index = 0;
         codepoint_index < codepoint_count;
         codepoint_index++)
    {
      wchar_t codepoint = first_codepoint + codepoint_index;
      
      b32 bg_blitted = PatBlt(device_context, 
                              0, 0, font_buffer_width, font_buffer_height, BLACKNESS);
      assert(bg_blitted);
      SetTextColor(device_context, RGB(255, 255, 255));
      b32 written_text = TextOutW(device_context, 0, 0, &codepoint, 1);
      
      i32 min_x = 10000;
      i32 min_y = 10000;
      i32 max_x = -10000;
      i32 max_y = -10000;
      
      
      u32 *pixel = font_buffer_pixels;
      for (i32 y = 0; y < font_buffer_height; y++) {
        for (i32 x = 0; x < font_buffer_width; x++) {
          u32 color_ref = *(pixel++);
          if (color_ref != 0) {
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
          }
        }
      }
      
      if (min_x == 10000) {
        min_x = 0;
        min_y = 0;
        max_x = 0;
        max_y = 0;
      } else {
        min_x--;
        min_y--;
        max_x++;
        max_y++;
      }
      
      
      Bitmap bmp = make_empty_bitmap(max_x - min_x, max_y - min_y);
      
      for (i32 y = 0; y < bmp.height; y++) {
        for (i32 x = 0; x < bmp.width; x++) {
          u32 src_pixel = font_buffer_pixels[(min_y + y)*font_buffer_width + min_x + x];
          u8 intensity = (u8)((src_pixel & 0x00FF0000) >> 16);
          u32 new_pixel = color_u32(0xFF, 0xFF, 0xFF, intensity);
          bmp.data[y*bmp.width + x] = new_pixel;
        }
      }
      
      codepoint_bitmaps[codepoint_index] = bmp;
      
      ABC abc = abcs[codepoint_index];
      i8 total_width = (i8)(abc.abcA + abc.abcB + abc.abcC);
      font.advance[codepoint_index] = total_width;
      font.origins[codepoint_index] = v2((f32)min_x, (f32)min_y - font_buffer_height);
    }
    
    font.advance[first_codepoint] = font.advance[' ' - first_codepoint];
    
    Bitmap white_bitmap = make_empty_bitmap(2, 2);
    white_bitmap.data[0] = 0xFFFFFFFF;
    white_bitmap.data[1] = 0xFFFFFFFF;
    white_bitmap.data[2] = 0xFFFFFFFF;
    white_bitmap.data[3] = 0xFFFFFFFF;
    codepoint_bitmaps[codepoint_count] = white_bitmap;
    
    font.line_spacing = (i8)(metric->otmLineGap + metric->otmAscent - metric->otmDescent);
    font.line_height = (i8)metric->otmTextMetrics.tmHeight;
    font.descent = (i8)metric->otmTextMetrics.tmDescent;
    font.atlas = texture_atlas_make_from_bitmaps(codepoint_bitmaps,
                                                 codepoint_count+1,
                                                 512);
    
    DWORD kerning_pair_count = GetKerningPairs(device_context, I32_MAX, null);
    KERNINGPAIR *kerning_pairs = scratch_push_array(KERNINGPAIR, 
                                                    kerning_pair_count);
    GetKerningPairs(device_context, kerning_pair_count, kerning_pairs);
    for (DWORD i = 0; i < kerning_pair_count; i++) {
      KERNINGPAIR pair = kerning_pairs[i];
      assert(pair.wFirst >= font.first_codepoint);
      
      char first = (char)pair.wFirst - font.first_codepoint;
      char second = (char)pair.wSecond - font.first_codepoint;
      assert(pair.iKernAmount < I8_MAX);
      assert(pair.iKernAmount > I8_MIN);
      font.kerning[first*codepoint_count + second] += (i8)pair.iKernAmount;
    }
  }
  
  return font;
}

#endif




b32 os_input_is_upper(os_Input input) {
  b32 result = (input.caps_lock && !input.shift) ||
    (!input.caps_lock && input.shift);
  return result;
}


#define LVL5_OS
#endif
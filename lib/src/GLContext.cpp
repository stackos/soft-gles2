/*
* soft-gles2
* Copyright 2014-2018 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#define GL_APICALL __declspec(dllexport)

#include "GLES2/gl2.h"
#include "math/Mathf.h"
#include <memory>

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "container/Vector.h"
#include <Windows.h>
#include <string>

using namespace Viry3D;

void exec_cmd(const std::string& path, const std::string& exe, const std::string& param)
{
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "\"" + path + "\\" + exe + "\" " + param;

    if (CreateProcess(
        NULL,
        (LPSTR) cmd.c_str(),    // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi))
    {
        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles. 
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

class GLContext
{
public:
    void SetDefaultBuffers(void* color_buffer, void* depth_buffer, int width, int height)
    {
        m_default_color_buffer = (unsigned char*) color_buffer;
        m_default_depth_buffer = (float*) depth_buffer;
        m_default_buffer_width = width;
        m_default_buffer_height = height;
    }

    void Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
    {
        m_viewport_x = Mathf::Clamp(x, 0, m_default_buffer_width - 1);
        m_viewport_y = Mathf::Clamp(y, 0, m_default_buffer_height - 1);
        m_viewport_width = Mathf::Clamp(width, 0, m_default_buffer_width - m_viewport_x);
        m_viewport_height = Mathf::Clamp(height, 0, m_default_buffer_height - m_viewport_y);
    }

    void ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
    {
        m_clear_color_red = Mathf::Clamp01(red);
        m_clear_color_green = Mathf::Clamp01(green);
        m_clear_color_blue = Mathf::Clamp01(blue);
        m_clear_color_alpha = Mathf::Clamp01(alpha);
    }

    void ClearDepthf(GLfloat d)
    {
        m_clear_depth = Mathf::Clamp01(d);
    }

    void Clear(GLbitfield mask)
    {
        int x = 0;
        int width = m_default_buffer_width;
        int y = 0;
        int height = m_default_buffer_height;

        if (m_viewport_x >= 0)
        {
            x = m_viewport_x;
            y = m_viewport_y;
            width = m_viewport_width;
            height = m_viewport_height;
        }

        if (mask & GL_COLOR_BUFFER_BIT)
        {
            unsigned char r = FloatToColorByte(m_clear_color_red);
            unsigned char g = FloatToColorByte(m_clear_color_green);
            unsigned char b = FloatToColorByte(m_clear_color_blue);
            unsigned char a = FloatToColorByte(m_clear_color_alpha);

            for (int i = y; i < y + height; ++i)
            {
                for (int j = x; j < x + width; ++j)
                {
                    m_default_color_buffer[i * m_default_buffer_width * 4 + j * 4 + 0] = r;
                    m_default_color_buffer[i * m_default_buffer_width * 4 + j * 4 + 1] = g;
                    m_default_color_buffer[i * m_default_buffer_width * 4 + j * 4 + 2] = b;
                    m_default_color_buffer[i * m_default_buffer_width * 4 + j * 4 + 3] = a;
                }
            }
        }

        if (mask & GL_DEPTH_BUFFER_BIT)
        {
            for (int i = y; i < y + height; ++i)
            {
                for (int j = x; j < x + width; ++j)
                {
                    m_default_depth_buffer[i * m_default_buffer_width + j] = m_clear_depth;
                }
            }
        }
    }

    void SetPixelTest(int x, int y, float r, float g, float b, float a)
    {
        m_default_color_buffer[y * m_default_buffer_width * 4 + x * 4 + 0] = FloatToColorByte(r);
        m_default_color_buffer[y * m_default_buffer_width * 4 + x * 4 + 1] = FloatToColorByte(g);
        m_default_color_buffer[y * m_default_buffer_width * 4 + x * 4 + 2] = FloatToColorByte(b);
        m_default_color_buffer[y * m_default_buffer_width * 4 + x * 4 + 3] = FloatToColorByte(a);
    }

    float ProjToScreenX(float x)
    {
        return (x * 0.5f + 0.5f) * m_default_buffer_width;
    }

    float ProjToScreenY(float y)
    {
        return (y * 0.5f + 0.5f) * m_default_buffer_height;
    }

    bool IsTopLeftEdge(int x1, int y1, int x2, int y2)
    {
        return ((y2 > y1) || (y1 == y2 && x1 > x2));
    }

    struct Vector2i
    {
        int x;
        int y;
    };

    Vector<Vector2i> TriangleEdge(int x1, int y1, int x2, int y2)
    {
        Vector<Vector2i> line;

        if (x1 == x2)
        {
            int y = y1;
            int step = y2 > y1 ? 1 : -1;
            while (y != y2)
            {
                line.Add({ x1, y });
                y += step;
            }
            line.Add({ x2, y2 });
        }
        else
        {
            float k = fabs((y2 - y1) / (float) (x2 - x1));

            if (k < 1) // x step 1
            {
                int x = x1;
                int y = y1;
                int x_step = x2 > x1 ? 1 : -1;
                int y_step = y2 > y1 ? 1 : -1;
                float d = 0;
                while (x != x2)
                {
                    if (d >= 0.5f)
                    {
                        line.Add({ x, y + y_step });
                    }
                    else
                    {
                        line.Add({ x, y });
                    }
                    
                    x += x_step;
                    d += k;
                    if (d >= 1.0f)
                    {
                        d -= 1.0f;
                        y += y_step;
                    }
                }
                line.Add({ x2, y2 });
            }
            else // y step 1
            {
                int x = x1;
                int y = y1;
                int x_step = x2 > x1 ? 1 : -1;
                int y_step = y2 > y1 ? 1 : -1;
                float d = 0;
                while (y != y2)
                {
                    if (d >= 0.5f)
                    {
                        line.Add({ x + x_step, y });
                    }
                    else
                    {
                        line.Add({ x, y });
                    }

                    y += y_step;
                    d += k;
                    if (d >= 1.0f)
                    {
                        d -= 1.0f;
                        x += x_step;
                    }
                }
                line.Add({ x2, y2 });
            }
        }

        return line;
    }

    void DrawTriangleTest(const Vector4* pos, const Vector2* uv, const Vector4* color)
    {
        Vector4 a = pos[0];
        Vector4 b = pos[1];
        Vector4 c = pos[2];

        Vector4 pa = a / a.w;
        Vector4 pb = b / b.w;
        Vector4 pc = c / c.w;

        if (pa.y > pb.y)
        {
            std::swap(a, b);
            std::swap(pa, pb);
        }
        if (pb.y > pc.y)
        {
            std::swap(b, c);
            std::swap(pb, pc);
        }
        if (pa.y > pb.y)
        {
            std::swap(a, b);
            std::swap(pa, pb);
        }

        /*
                     pc
 
            pb     pd

                pa
        */

        Vector2 pd;
        pd.x = pc.x - (pc.y - pb.y) * (pc.x - pa.x) / (pc.y - pa.y);
        pd.y = pb.y;

        // fill cbd
        {
            int x1 = (int) ProjToScreenX(pc.x);
            int y1 = (int) ProjToScreenY(pc.y);
            int x2 = (int) ProjToScreenX(pd.x);
            int y2 = (int) ProjToScreenY(pd.y);
            int x3 = (int) ProjToScreenX(pb.x);
            int y3 = (int) ProjToScreenY(pb.y);

            Vector<Vector2i> e1 = TriangleEdge(x1, y1, x2, y2);
            Vector<Vector2i> e2 = TriangleEdge(x1, y1, x3, y3);

            int i1 = 0;
            int i2 = 0;
            int y = y1;
            while (y != y2)
            {
                int minX = 0x7fffffff;
                int maxX = -1;

                for (int i = i1; i < e1.Size(); ++i)
                {
                    if (e1[i].y != y)
                    {
                        i1 = i;
                        break;
                    }
                    else
                    {
                        minX = Mathf::Min(minX, e1[i].x);
                        maxX = Mathf::Max(maxX, e1[i].x);
                    }
                }

                for (int i = i2; i < e2.Size(); ++i)
                {
                    if (e2[i].y != y)
                    {
                        i2 = i;
                        break;
                    }
                    else
                    {
                        minX = Mathf::Min(minX, e2[i].x);
                        maxX = Mathf::Max(maxX, e2[i].x);
                    }
                }

                for (int i = minX; i <= maxX; ++i)
                {
                    // todo:
                    // apply top left side check
                    SetPixelTest(i, y, 1, 1, 1, 1);
                }

                y--;
            }
        }
        
        // fill abd
        {
            int x1 = (int) ProjToScreenX(pa.x);
            int y1 = (int) ProjToScreenY(pa.y);
            int x2 = (int) ProjToScreenX(pd.x);
            int y2 = (int) ProjToScreenY(pd.y);
            int x3 = (int) ProjToScreenX(pb.x);
            int y3 = (int) ProjToScreenY(pb.y);

            Vector<Vector2i> e1 = TriangleEdge(x2, y2, x1, y1);
            Vector<Vector2i> e2 = TriangleEdge(x3, y3, x1, y1);

            int i1 = 0;
            int i2 = 0;
            int y = y3;
            while (y != y1)
            {
                int minX = 0x7fffffff;
                int maxX = -1;

                for (int i = i1; i < e1.Size(); ++i)
                {
                    if (e1[i].y != y)
                    {
                        i1 = i;
                        break;
                    }
                    else
                    {
                        minX = Mathf::Min(minX, e1[i].x);
                        maxX = Mathf::Max(maxX, e1[i].x);
                    }
                }

                for (int i = i2; i < e2.Size(); ++i)
                {
                    if (e2[i].y != y)
                    {
                        i2 = i;
                        break;
                    }
                    else
                    {
                        minX = Mathf::Min(minX, e2[i].x);
                        maxX = Mathf::Max(maxX, e2[i].x);
                    }
                }

                for (int i = minX; i <= maxX; ++i)
                {
                    // todo:
                    // apply top left side check
                    SetPixelTest(i, y, 1, 1, 1, 1);
                }

                y--;
            }
        }

        /*
        int TopLeftEdge(int x1, int y1, int x2, int y2)
        {
            return ((y2 > y1) || (y1 == y2 && x1 > x2)) ? 0 : -1;
        }

        int EdgeEquation(int x, int y, int x1, int y1, int x2, int y2)
        {
            return (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1) + TopLeftEdge(x1, y1, x2, y2);
        }

        void DrawTriangleTest(const Vector4* pos, const Vector2* uv, const Vector4* color)
        {
            Vector4 v1 = pos[0] / pos[0].w;
            Vector4 v2 = pos[1] / pos[1].w;
            Vector4 v3 = pos[2] / pos[2].w;

            int x1 = (int) ProjToScreenX(v1.x);
            int x2 = (int) ProjToScreenX(v2.x);
            int x3 = (int) ProjToScreenX(v3.x);

            int y1 = (int) ProjToScreenY(v1.y);
            int y2 = (int) ProjToScreenY(v2.y);
            int y3 = (int) ProjToScreenY(v3.y);

            int minx = Mathf::Min(Mathf::Min(x1, x2), x3);
            int maxx = Mathf::Max(Mathf::Max(x1, x2), x3);
            int miny = Mathf::Min(Mathf::Min(y1, y2), y3);
            int maxy = Mathf::Max(Mathf::Max(y1, y2), y3);

            for (int y = miny; y < maxy; y++)
            {
                for (int x = minx; x < maxx; x++)
                {
                    int w1 = EdgeEquation(x, y, x1, y1, x2, y2);
                    int w2 = EdgeEquation(x, y, x2, y2, x3, y3);
                    int w3 = EdgeEquation(x, y, x3, y3, x1, y1);

                    if (w1 >= 0 && w2 >= 0 && w3 >= 0)
                    {
                        SetPixelTest(x, y, 1, 1, 1, 1);
                    }
                }
            }
        }*/
    }

    HMODULE dll = nullptr;

    void DrawTest()
    {
        if (dll == nullptr)
        {
            //std::string vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community";
            std::string vs_path = "D:\\Program\\VS2017";

            std::string cl_dir = vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\Hostx64\\x64";
            exec_cmd(cl_dir, "cl.exe", "/c test.vs.cpp test.ps.cpp "
                "/I \"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\include\" "
                "/I \"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.16299.0\\ucrt\"");
            exec_cmd(cl_dir, "link.exe", "/dll test.vs.obj test.ps.obj /OUT:test.dll "
                "/LIBPATH:\"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\lib\\x64\" "
                "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\um\\x64\" "
                "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\ucrt\\x64\"");

            dll = LoadLibrary("test.dll");
        }
        
        if (dll)
        {
            struct Vertex
            {
                Vector3 pos;
                Vector2 uv;
                Vector4 color;
            };

            Vertex vertices[3] = {
                { Vector3(-1, 0, 2), Vector2(0, 0), Vector4(1, 0, 0, 1) },
                { Vector3(-1, 0, 1), Vector2(0, 1), Vector4(0, 1, 0, 1) },
                { Vector3(1, 0, 0), Vector2(1, 1), Vector4(0, 0, 1, 1) },
            };
            unsigned short indices[3] = { 0, 1, 2 };

            Matrix4x4 view = Matrix4x4::LookTo(
                Vector3(0, 1, -2),
                Vector3(0, 0, 1),
                Vector3(0, 1, 0));
            Matrix4x4 proj = Matrix4x4::Perspective(60, 1280 / 720.0f, 0.3f, 1000);
            Matrix4x4 vp = proj * view;

            Vector4 gl_position[3];
            Vector2 v_uv[3];
            Vector4 v_color[3];

            typedef void*(*VarGetter)();
            typedef void(*VarSetter)(void*, int);
            typedef void(*Main)();

            VarSetter set_a_position = (VarSetter) GetProcAddress(dll, "set_a_position");
            VarSetter set_a_uv = (VarSetter) GetProcAddress(dll, "set_a_uv");
            VarSetter set_a_color = (VarSetter) GetProcAddress(dll, "set_a_color");
            Main vs_main = (Main) GetProcAddress(dll, "vs_main");
            VarGetter get_gl_Position = (VarGetter) GetProcAddress(dll, "get_gl_Position");
            VarGetter get_v_uv = (VarGetter) GetProcAddress(dll, "get_v_uv");
            VarGetter get_v_color = (VarGetter) GetProcAddress(dll, "get_v_color");

            if (vs_main)
            {
                for (int i = 0; i < 3; ++i)
                {
                    unsigned short index = indices[i];

                    Vector4 pos = vp * Vector4(vertices[index].pos, 1);

                    set_a_position(&pos, sizeof(Vector4));
                    set_a_uv(&vertices[index].uv, sizeof(Vector2));
                    set_a_color(&vertices[index].color, sizeof(Vector4));

                    vs_main();

                    gl_position[i] = *(Vector4*) get_gl_Position();
                    v_uv[i] = *(Vector2*) get_v_uv();
                    v_color[i] = *(Vector4*) get_v_color();
                }
            }

            VarSetter set_u_tex = (VarSetter) GetProcAddress(dll, "set_u_tex");
            VarSetter set_v_uv = (VarSetter) GetProcAddress(dll, "set_v_uv");
            VarSetter set_v_color = (VarSetter) GetProcAddress(dll, "set_v_color");
            Main ps_main = (Main) GetProcAddress(dll, "ps_main");
            VarGetter get_gl_FragColor = (VarGetter) GetProcAddress(dll, "get_gl_FragColor");

            if (ps_main)
            {
                DrawTriangleTest(gl_position, v_uv, v_color);
            }
        }
    }

    GLContext():
        m_default_color_buffer(nullptr),
        m_default_depth_buffer(nullptr),
        m_default_buffer_width(0),
        m_default_buffer_height(0),
        m_viewport_x(-1),
        m_viewport_y(-1),
        m_viewport_width(-1),
        m_viewport_height(-1),
        m_clear_color_red(1),
        m_clear_color_green(1),
        m_clear_color_blue(1),
        m_clear_color_alpha(1),
        m_clear_depth(1)
    {
    }

    ~GLContext()
    {
        if (dll)
        {
            FreeLibrary(dll);
        }
    }

private:
    unsigned char FloatToColorByte(float f)
    {
        return (unsigned char) (Mathf::Clamp01(f) * 255);
    }

private:
    unsigned char* m_default_color_buffer;
    float* m_default_depth_buffer;
    int m_default_buffer_width;
    int m_default_buffer_height;

    int m_viewport_x;
    int m_viewport_y;
    int m_viewport_width;
    int m_viewport_height;
    float m_clear_color_red;
    float m_clear_color_green;
    float m_clear_color_blue;
    float m_clear_color_alpha;
    float m_clear_depth;
};

static GLContext gl;

__declspec(dllexport)
void set_default_buffers(void* color_buffer, void* depth_buffer, int width, int height)
{
    gl.SetDefaultBuffers(color_buffer, depth_buffer, width, height);
}

void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    gl.Viewport(x, y, width, height);
}

void GL_APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    gl.ClearColor(red, green, blue, alpha);
}

void GL_APIENTRY glClearDepthf(GLfloat d)
{
    gl.ClearDepthf(d);
}

void GL_APIENTRY glClear(GLbitfield mask)
{
    gl.Clear(mask);

    gl.DrawTest();
}

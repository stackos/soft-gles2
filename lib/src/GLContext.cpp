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
#include "GLES2/gl2ext.h"
#include "GLObject.h"
#include "GLFramebuffer.h"
#include "GLRenderbuffer.h"

#include "math/Mathf.h"
#include "container/Map.h"

#include <functional>

using namespace Viry3D;

/*
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "container/Vector.h"
#include "io/File.h"
#include "graphics/Image.h"
#include <Windows.h>
#include <string>
#include <memory>

std::string vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community";
//std::string vs_path = "D:\\Program\\VS2017";

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
*/

namespace sgl
{
    class GLContext
    {
    public:
        void SetDefaultBuffers(void* color_buffer, void* depth_buffer, int width, int height)
        {
            m_default_color_buffer = (unsigned char*) color_buffer;
            m_default_depth_buffer = (float*) depth_buffer;
            m_default_buffer_width = width;
            m_default_buffer_height = height;

            if (m_viewport_x < 0)
            {
                m_viewport_x = 0;
                m_viewport_y = 0;
                m_viewport_width = width;
                m_viewport_height = height;
            }
        }

        template<class T>
        void GenObjects(GLsizei n, GLuint* objs)
        {
            for (int i = 0; i < n; ++i)
            {
                GLuint id = ++m_gen_id;
                m_objects.Add(id, RefMake<T>());
                objs[i] = id;
            }
        }

        typedef std::function<void(const Ref<GLObject>&)> OnRemoveObject;

        template<class T>
        void DeleteObjects(GLsizei n, const GLuint* objs, OnRemoveObject on_remove)
        {
            for (int i = 0; i < n; ++i)
            {
                Ref<GLObject>* find;
                GLuint id = objs[i];
                if (m_objects.TryGet(id, &find))
                {
                    Ref<T> obj = RefCast<T>(*find);
                    if (obj)
                    {
                        m_objects.Remove(id);

                        on_remove(obj);
                    }
                }
            }
        }

        template<class T>
        GLboolean ObjectIs(GLuint obj)
        {
            Ref<GLObject>* find;
            if (m_objects.TryGet(obj, &find))
            {
                if (RefCast<T>(*find))
                {
                    return GL_TRUE;
                }
            }

            return GL_FALSE;
        }

        template<class T>
        Ref<T> ObjectGet(GLuint obj)
        {
            Ref<GLObject>* find;
            if (m_objects.TryGet(obj, &find))
            {
                return RefCast<T>(*find);
            }

            return Ref<T>();
        }

        void GenFramebuffers(GLsizei n, GLuint* framebuffers)
        {
            GenObjects<GLFrameFbuffer>(n, framebuffers);
        }

        void DeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
        {
            DeleteObjects<GLFrameFbuffer>(n, framebuffers, [this](const Ref<GLObject>& obj) {
                if (!m_current_fbo.expired() && m_current_fbo.lock() == obj)
                {
                    m_current_fbo.reset();
                }
            });
        }

        GLboolean IsFramebuffer(GLuint framebuffer)
        {
            return ObjectIs<GLFrameFbuffer>(framebuffer);
        }

        void BindFramebuffer(GLenum target, GLuint framebuffer)
        {
            if (target == GL_FRAMEBUFFER)
            {
                if (framebuffer == 0)
                {
                    m_current_fbo.reset();
                }
                else
                {
                    Ref<GLFrameFbuffer> fbo = ObjectGet<GLFrameFbuffer>(framebuffer);
                    if (fbo)
                    {
                        m_current_fbo = fbo;
                    }
                }
            }
        }

        void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
        {
            if (target == GL_FRAMEBUFFER)
            {
                if (renderbuffertarget == GL_RENDERBUFFER)
                {
                    if (!m_current_fbo.expired())
                    {
                        Ref<GLFrameFbuffer> fbo = m_current_fbo.lock();
                        Ref<GLRenderbuffer> rbo;
                        if (renderbuffer > 0)
                        {
                            rbo = ObjectGet<GLRenderbuffer>(renderbuffer);
                        }

                        switch (attachment)
                        {
                            case GL_COLOR_ATTACHMENT0:
                                fbo->SetAttachment(GLFrameFbuffer::Attachment::Color0, rbo);
                                break;
                            case GL_DEPTH_ATTACHMENT:
                                fbo->SetAttachment(GLFrameFbuffer::Attachment::Depth, rbo);
                                break;
                            case GL_STENCIL_ATTACHMENT:
                                fbo->SetAttachment(GLFrameFbuffer::Attachment::Stencil, rbo);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }

        void GenRenderbuffers(GLsizei n, GLuint* renderbuffers)
        {
            GenObjects<GLRenderbuffer>(n, renderbuffers);
        }

        void DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
        {
            DeleteObjects<GLRenderbuffer>(n, renderbuffers, [this](const Ref<GLObject>& obj) {
                if (!m_current_rbo.expired() && m_current_rbo.lock() == obj)
                {
                    m_current_rbo.reset();
                }
            });
        }

        GLboolean IsRenderbuffer(GLuint renderbuffer)
        {
            return ObjectIs<GLRenderbuffer>(renderbuffer);
        }

        void BindRenderbuffer(GLenum target, GLuint renderbuffer)
        {
            if (target == GL_RENDERBUFFER)
            {
                if (renderbuffer == 0)
                {
                    m_current_rbo.reset();
                }
                else
                {
                    Ref<GLRenderbuffer> rbo = ObjectGet<GLRenderbuffer>(renderbuffer);
                    if (rbo)
                    {
                        m_current_rbo = rbo;
                    }
                }
            }
        }

        void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
        {
            if (target == GL_RENDERBUFFER)
            {
                if (!m_current_rbo.expired())
                {
                    Ref<GLRenderbuffer> rbo = m_current_rbo.lock();
                    rbo->Storage(internalformat, width, height);
                }
            }
        }

        void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params)
        {
            if (target == GL_RENDERBUFFER)
            {
                if (!m_current_rbo.expired())
                {
                    Ref<GLRenderbuffer> rbo = m_current_rbo.lock();

                    switch (pname)
                    {
                        case GL_RENDERBUFFER_WIDTH:
                            *params = rbo->GetWidth();
                            break;
                        case GL_RENDERBUFFER_HEIGHT:
                            *params = rbo->GetHeight();
                            break;
                        case GL_RENDERBUFFER_INTERNAL_FORMAT:
                            *params = rbo->GetInternalFormat();
                            break;
                        case GL_RENDERBUFFER_RED_SIZE:
                            *params = rbo->GetRedComponentSize();
                            break;
                        case GL_RENDERBUFFER_GREEN_SIZE:
                            *params = rbo->GetGreenComponentSize();
                            break;
                        case GL_RENDERBUFFER_BLUE_SIZE:
                            *params = rbo->GetBlueComponentSize();
                            break;
                        case GL_RENDERBUFFER_ALPHA_SIZE:
                            *params = rbo->GetAlphaComponentSize();
                            break;
                        case GL_RENDERBUFFER_DEPTH_SIZE:
                            *params = rbo->GetDepthSize();
                            break;
                        case GL_RENDERBUFFER_STENCIL_SIZE:
                            *params = rbo->GetStencilSize();
                            break;
                        default:
                            *params = 0;
                            break;
                    }
                }
            }
        }

        void Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
        {
            m_viewport_x = x;
            m_viewport_y = y;
            m_viewport_width = width;
            m_viewport_height = height;
        }

        void ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
        {
            m_clear_color_red = red;
            m_clear_color_green = green;
            m_clear_color_blue = blue;
            m_clear_color_alpha = alpha;
        }

        void ClearDepthf(GLfloat d)
        {
            m_clear_depth = d;
        }

        void ClearStencil(GLint s)
        {
            m_clear_stencil = s;
        }

        void Clear(GLbitfield mask)
        {
            int x = m_viewport_x;
            int y = m_viewport_y;
            int width = m_viewport_width;
            int height = m_viewport_height;

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

            if (mask & GL_STENCIL_BUFFER_BIT)
            {
                
            }
        }

        /*
        struct Vector2i
        {
            int x;
            int y;

            Vector2i(int x = 0, int y = 0): x(x), y(y) { }

            Vector2i operator -(const Vector2i& right) const
            {
                return Vector2i(x - right.x, y - right.y);
            }

            static int Dot(const Vector2i& left, const Vector2i& right)
            {
                return left.x * right.x + left.y * right.y;
            }

            static int Cross(const Vector2i& left, const Vector2i& right)
            {
                return left.x * right.y - left.y * right.x;
            }
        };

        struct Color
        {
            float r;
            float g;
            float b;
            float a;

            Color(float r = 1, float g = 1, float b = 1, float a = 1): r(r), g(g), b(b), a(a) { }

            Color operator +(const Color& right) const
            {
                return Color(r + right.r, g + right.g, b + right.b, a + right.a);
            }

            Color operator *(float right) const
            {
                return Color(r * right, g * right, b * right, a * right);
            }
        };

        void SetPixelTest(const Vector2i& p, const Color& c)
        {
            m_default_color_buffer[p.y * m_default_buffer_width * 4 + p.x * 4 + 0] = FloatToColorByte(c.r);
            m_default_color_buffer[p.y * m_default_buffer_width * 4 + p.x * 4 + 1] = FloatToColorByte(c.g);
            m_default_color_buffer[p.y * m_default_buffer_width * 4 + p.x * 4 + 2] = FloatToColorByte(c.b);
            m_default_color_buffer[p.y * m_default_buffer_width * 4 + p.x * 4 + 3] = FloatToColorByte(c.a);
        }

        float ProjToScreenX(float x)
        {
            return m_viewport_x + (x * 0.5f + 0.5f) * m_viewport_width;
        }

        float ProjToScreenY(float y)
        {
            return m_viewport_y + (y * 0.5f + 0.5f) * m_viewport_height;
        }

        bool IsTopLeftEdge(const Vector2i& p0, const Vector2i& p1)
        {
            return ((p1.y > p0.y) || (p0.y == p1.y && p0.x > p1.x));
        }

        int EdgeEquation(const Vector2i& p, const Vector2i& p0, const Vector2i& p1)
        {
            return (p1.x - p0.x) * (p.y - p0.y) - (p1.y - p0.y) * (p.x - p0.x) + IsTopLeftEdge(p0, p1) ? 0 : -1;
        }

        Vector<Vector2i> TriangleEdge(const Vector2i& p0, const Vector2i& p1)
        {
            Vector<Vector2i> line;

            if (p0.x == p1.x)
            {
                int y = p0.y;
                int step = p1.y > p0.y ? 1 : -1;
                while (y != p1.y)
                {
                    line.Add({ p0.x, y });
                    y += step;
                }
                line.Add({ p1.x, p1.y });
            }
            else
            {
                float k = fabs((p1.y - p0.y) / (float) (p1.x - p0.x));

                if (k < 1) // x step 1
                {
                    int x = p0.x;
                    int y = p0.y;
                    int x_step = p1.x > p0.x ? 1 : -1;
                    int y_step = p1.y > p0.y ? 1 : -1;
                    float d_step = k;
                    float d = 0;
                    while (x != p1.x)
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
                        d += d_step;
                        if (d >= 1.0f)
                        {
                            d -= 1.0f;
                            y += y_step;
                        }
                    }
                    line.Add({ p1.x, p1.y });
                }
                else // y step 1
                {
                    int x = p0.x;
                    int y = p0.y;
                    int x_step = p1.x > p0.x ? 1 : -1;
                    int y_step = p1.y > p0.y ? 1 : -1;
                    float d_step = 1.0f / k;
                    float d = 0;
                    while (y != p1.y)
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
                        d += d_step;
                        if (d >= 1.0f)
                        {
                            d -= 1.0f;
                            x += x_step;
                        }
                    }
                    line.Add({ p1.x, p1.y });
                }
            }

            return line;
        }

        class Texture
        {
        public:
            int width;
            int height;
            int bpp;
            ByteBuffer data;
        };

        class Sampler2D
        {
        public:
            typedef Color(*Sample)(Texture*, const Vector2*);
            Texture* texture;
            Sample sample_func = Sampler2D::SampleTexture;

            static Color SampleTexture(Texture* tex, const Vector2* uv)
            {
                int x = (int) ((tex->width - 1) * Mathf::Clamp01(uv->x));
                int y = (int) ((tex->height - 1) * Mathf::Clamp01(uv->y));

                int r = tex->data[y * tex->width * 4 + x * 4 + 0];
                int g = tex->data[y * tex->width * 4 + x * 4 + 1];
                int b = tex->data[y * tex->width * 4 + x * 4 + 2];
                int a = tex->data[y * tex->width * 4 + x * 4 + 3];

                return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
            }
        };

        Color CallFragmentShader(const Vector2& uv, const Color& color)
        {
            typedef void*(*VarGetter)();
            typedef void(*VarSetter)(void*, int);
            typedef void(*Main)();

            VarSetter set_v_uv = (VarSetter) GetProcAddress(dll, "set_v_uv");
            VarSetter set_v_color = (VarSetter) GetProcAddress(dll, "set_v_color");
            Main ps_main = (Main) GetProcAddress(dll, "ps_main");
            VarGetter get_gl_FragColor = (VarGetter) GetProcAddress(dll, "get_gl_FragColor");

            set_v_uv((void*) &uv, sizeof(Vector2));
            set_v_color((void*) &color, sizeof(Color));
            ps_main();
            return *(Color*) get_gl_FragColor();
        }

        void DrawScanLineTest(int y, int min_x, int max_x, const Vector2i& p0, const Vector2i& p1, const Vector2i& p2,
            const Vector4* pos, const Vector2* uv, const Color* color)
        {
            // 重心坐标插值
            float one_div_area = 1.0f / fabs(Vector2i::Cross(p0 - p1, p2 - p1) / 2.0f);
            // 透视校正
            float one_div_ws[3] = {
                1.0f / pos[0].w,
                1.0f / pos[1].w,
                1.0f / pos[2].w,
            };
        
            for (int i = min_x; i <= max_x; ++i)
            {
                Vector2i p(i, y);
                int w1 = EdgeEquation(p, p0, p1);
                int w2 = EdgeEquation(p, p1, p2);
                int w3 = EdgeEquation(p, p2, p0);

                if (w1 >= 0 && w2 >= 0 && w3 >= 0)
                {
                    float a01 = fabs(Vector2i::Cross(p1 - p, p0 - p) / 2.0f) * one_div_area;
                    float a12 = fabs(Vector2i::Cross(p2 - p, p1 - p) / 2.0f) * one_div_area;
                    float a20 = 1.0f - a01 - a12;

                    float w = 1.0f / (a01 * one_div_ws[2] + a12 * one_div_ws[0] + a20 * one_div_ws[1]);
                    Vector2 v_uv = (uv[2] * a01 * one_div_ws[2] + uv[0] * a12 * one_div_ws[0] + uv[1] * a20 * one_div_ws[1]) * w;
                    Color v_color = (color[2] * a01 * one_div_ws[2] + color[0] * a12 * one_div_ws[0] + color[1] * a20 * one_div_ws[1]) * w;

                    Color c = CallFragmentShader(v_uv, v_color);

                    SetPixelTest(p, c);
                }
            }
        }

        void DrawHalfTriangleTest(
            const Vector<Vector2i>& e1, const Vector<Vector2i>& e2,
            int y_top, int y_bottom,
            const Vector2i& p0, const Vector2i& p1, const Vector2i& p2,
            const Vector4* pos, const Vector2* uv, const Color* color)
        {
            int i1 = 0;
            int i2 = 0;
            int y = y_top;
            while (y != y_bottom)
            {
                int min_x = 0x7fffffff;
                int max_x = -1;

                for (int i = i1; i < e1.Size(); ++i)
                {
                    if (e1[i].y != y)
                    {
                        i1 = i;
                        break;
                    }
                    else
                    {
                        min_x = Mathf::Min(min_x, e1[i].x);
                        max_x = Mathf::Max(max_x, e1[i].x);
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
                        min_x = Mathf::Min(min_x, e2[i].x);
                        max_x = Mathf::Max(max_x, e2[i].x);
                    }
                }

                DrawScanLineTest(y, min_x, max_x, p0, p1, p2, pos, uv, color);

                y--;
            }
        }

        void DrawTriangleTest(const Vector4* pos, const Vector2* uv, const Color* color)
        {
            Vector2i p0;
            Vector2i p1;
            Vector2i p2;
            p0.x = (int) ProjToScreenX(pos[0].x / pos[0].w);
            p1.x = (int) ProjToScreenX(pos[1].x / pos[1].w);
            p2.x = (int) ProjToScreenX(pos[2].x / pos[2].w);
            p0.y = (int) ProjToScreenY(pos[0].y / pos[0].w);
            p1.y = (int) ProjToScreenY(pos[1].y / pos[1].w);
            p2.y = (int) ProjToScreenY(pos[2].y / pos[2].w);

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

            //             pc
            //
            //    pb     pd
            //
            //        pa

            Vector2 pd;
            pd.x = pc.x - (pc.y - pb.y) * (pc.x - pa.x) / (pc.y - pa.y);
            pd.y = pb.y;

            {
                Vector2i sa, sb, sc, sd;
                sa.x = (int) ProjToScreenX(pa.x);
                sa.y = (int) ProjToScreenY(pa.y);
                sb.x = (int) ProjToScreenX(pb.x);
                sb.y = (int) ProjToScreenY(pb.y);
                sc.x = (int) ProjToScreenX(pc.x);
                sc.y = (int) ProjToScreenY(pc.y);
                sd.x = (int) ProjToScreenX(pd.x);
                sd.y = (int) ProjToScreenY(pd.y);

                Vector<Vector2i> e1 = TriangleEdge(sc, sd);
                Vector<Vector2i> e2 = TriangleEdge(sc, sb);

                DrawHalfTriangleTest(e1, e2, sc.y, sd.y, p0, p1, p2, pos, uv, color);

                e1 = TriangleEdge(sd, sa);
                e2 = TriangleEdge(sb, sa);
                DrawHalfTriangleTest(e1, e2, sd.y, sa.y, p0, p1, p2, pos, uv, color);
            }
        }

        HMODULE dll = nullptr;
        Texture* pTex = nullptr;

        void DrawTest()
        {
            if (dll == nullptr)
            {
                std::string cl_dir = vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\Hostx64\\x64";
                exec_cmd(cl_dir, "cl.exe", "/c test.vs.cpp test.ps.cpp "
                    "/I \"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\include\" "
                    "/I \"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.16299.0\\ucrt\"");
                exec_cmd(cl_dir, "link.exe", "/dll test.vs.obj test.ps.obj /OUT:test.dll "
                    "/LIBPATH:\"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\lib\\x64\" "
                    "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\um\\x64\" "
                    "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\ucrt\\x64\"");

                dll = LoadLibrary("test.dll");

                pTex = new Texture();
                pTex->data = Image::LoadPNG(File::ReadAllBytes("Assets/texture/girl.png"), pTex->width, pTex->height, pTex->bpp);
            }
        
            if (dll)
            {
                struct Vertex
                {
                    Vector3 pos;
                    Vector2 uv;
                    Color color;
                };

                Vertex vertices[4] = {
                    { Vector3(-0.45f, 0, 4), Vector2(0, 0), Color(1, 0, 0, 1) },
                    { Vector3(-0.45f, -0.9f, 0), Vector2(0, 1), Color(0, 1, 0, 1) },
                    { Vector3(0.45f, -0.9f, 0), Vector2(1, 1), Color(0, 0, 1, 1) },
                    { Vector3(0.45f, 0, 4), Vector2(1, 0), Color(1, 0, 1, 1) },
                };
                unsigned short indices[6] = { 0, 1, 2, 0, 2, 3 };

                Matrix4x4 view = Matrix4x4::LookTo(
                    Vector3(0, 0, -2),
                    Vector3(0, 0, 1),
                    Vector3(0, 1, 0));
                Matrix4x4 proj = Matrix4x4::Perspective(60, 1280 / 720.0f, 0.3f, 1000);
                Matrix4x4 vp = proj * view;

                typedef void*(*VarGetter)();
                typedef void(*VarSetter)(void*, int);
                typedef void(*Main)();

                VarSetter set_u_tex = (VarSetter) GetProcAddress(dll, "set_u_tex");
                VarSetter set_a_position = (VarSetter) GetProcAddress(dll, "set_a_position");
                VarSetter set_a_uv = (VarSetter) GetProcAddress(dll, "set_a_uv");
                VarSetter set_a_color = (VarSetter) GetProcAddress(dll, "set_a_color");
                Main vs_main = (Main) GetProcAddress(dll, "vs_main");
                VarGetter get_gl_Position = (VarGetter) GetProcAddress(dll, "get_gl_Position");
                VarGetter get_v_uv = (VarGetter) GetProcAddress(dll, "get_v_uv");
                VarGetter get_v_color = (VarGetter) GetProcAddress(dll, "get_v_color");

                Sampler2D tex;
                tex.texture = pTex;
                set_u_tex(&tex, sizeof(Sampler2D));

                for (int i = 0; i < 2; ++i)
                {
                    Vector4 gl_position[3];
                    Vector2 v_uv[3];
                    Color v_color[3];

                    for (int j = 0; j < 3; ++j)
                    {
                        unsigned short index = indices[i * 3 + j];

                        Vector4 pos = vp * Vector4(vertices[index].pos, 1);

                        set_a_position(&pos, sizeof(Vector4));
                        set_a_uv(&vertices[index].uv, sizeof(Vector2));
                        set_a_color(&vertices[index].color, sizeof(Color));

                        vs_main();

                        gl_position[j] = *(Vector4*) get_gl_Position();
                        v_uv[j] = *(Vector2*) get_v_uv();
                        v_color[j] = *(Color*) get_v_color();
                    }

                    DrawTriangleTest(gl_position, v_uv, v_color);
                }
            }
        }
        */

        GLContext():
            m_default_color_buffer(nullptr),
            m_default_depth_buffer(nullptr),
            m_default_buffer_width(0),
            m_default_buffer_height(0),
            m_gen_id(0),
            m_viewport_x(-1),
            m_viewport_y(-1),
            m_viewport_width(-1),
            m_viewport_height(-1),
            m_clear_color_red(1.0f),
            m_clear_color_green(1.0f),
            m_clear_color_blue(1.0f),
            m_clear_color_alpha(1.0f),
            m_clear_depth(1.0f),
            m_clear_stencil(0)
        {
        }

        ~GLContext()
        {
            m_objects.Clear();

            /*if (dll)
            {
                FreeLibrary(dll);
            }
            if (pTex)
            {
                delete pTex;
            }*/
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

        Map<GLuint, Ref<GLObject>> m_objects;
        GLuint m_gen_id;
        WeakRef<GLFrameFbuffer> m_current_fbo;
        WeakRef<GLRenderbuffer> m_current_rbo;
        int m_viewport_x;
        int m_viewport_y;
        int m_viewport_width;
        int m_viewport_height;
        float m_clear_color_red;
        float m_clear_color_green;
        float m_clear_color_blue;
        float m_clear_color_alpha;
        float m_clear_depth;
        int m_clear_stencil; 
    };
}

static Ref<sgl::GLContext> gl;

__declspec(dllexport) void create_gl_context()
{
    gl = RefMake<sgl::GLContext>();
}

__declspec(dllexport) void destroy_gl_context()
{
    gl.reset();
}

__declspec(dllexport) void set_gl_context_default_buffers(void* color_buffer, void* depth_buffer, int width, int height)
{
    gl->SetDefaultBuffers(color_buffer, depth_buffer, width, height);
}

#define IMPLEMENT_VOID_GL_FUNC_1(func, t1) \
    void GL_APIENTRY gl##func(t1 p1) { \
        gl->func(p1); \
    }
#define IMPLEMENT_VOID_GL_FUNC_2(func, t1, t2) \
    void GL_APIENTRY gl##func(t1 p1, t2 p2) { \
        gl->func(p1, p2); \
    }
#define IMPLEMENT_VOID_GL_FUNC_3(func, t1, t2, t3) \
    void GL_APIENTRY gl##func(t1 p1, t2 p2, t3 p3) { \
        gl->func(p1, p2, p3); \
    }
#define IMPLEMENT_VOID_GL_FUNC_4(func, t1, t2, t3, t4) \
    void GL_APIENTRY gl##func(t1 p1, t2 p2, t3 p3, t4 p4) { \
        gl->func(p1, p2, p3, p4); \
    }
#define IMPLEMENT_GL_FUNC_1(ret, func, t1) \
    ret GL_APIENTRY gl##func(t1 p1) { \
        return gl->func(p1); \
    }

// Framebuffer
IMPLEMENT_VOID_GL_FUNC_2(GenFramebuffers, GLsizei, GLuint*)
IMPLEMENT_VOID_GL_FUNC_2(DeleteFramebuffers, GLsizei, const GLuint*)
IMPLEMENT_GL_FUNC_1(GLboolean, IsFramebuffer, GLuint)
IMPLEMENT_VOID_GL_FUNC_2(BindFramebuffer, GLenum, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(FramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint)

// Renderbuffer
IMPLEMENT_VOID_GL_FUNC_2(GenRenderbuffers, GLsizei, GLuint*)
IMPLEMENT_VOID_GL_FUNC_2(DeleteRenderbuffers, GLsizei, const GLuint*)
IMPLEMENT_GL_FUNC_1(GLboolean, IsRenderbuffer, GLuint)
IMPLEMENT_VOID_GL_FUNC_2(BindRenderbuffer, GLenum, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(RenderbufferStorage, GLenum, GLenum, GLsizei, GLsizei)
IMPLEMENT_VOID_GL_FUNC_3(GetRenderbufferParameteriv, GLenum, GLenum, GLint*)

// Viewport
IMPLEMENT_VOID_GL_FUNC_4(Viewport, GLint, GLint, GLsizei, GLsizei)

// Clear
IMPLEMENT_VOID_GL_FUNC_4(ClearColor, GLfloat, GLfloat, GLfloat, GLfloat)
IMPLEMENT_VOID_GL_FUNC_1(ClearDepthf, GLfloat)
IMPLEMENT_VOID_GL_FUNC_1(ClearStencil, GLint)
IMPLEMENT_VOID_GL_FUNC_1(Clear, GLbitfield)

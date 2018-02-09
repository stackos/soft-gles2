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
#include "Debug.h"
#include "math/Mathf.h"
#include "container/Map.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "GLObject.h"
#include "GLFramebuffer.h"
#include "GLRenderbuffer.h"
#include "GLTexture.h"
#include "GLShader.h"
#include "GLProgram.h"
#include "GLBuffer.h"
#include "GLRasterizer.h"
#include <functional>

using namespace Viry3D;

const char* g_vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community";
//const char* g_vs_path = "D:\\Program\\VS2017";

namespace sgl
{
    class GLContext
    {
    public:
        struct VertexAttribArray
        {
            bool enable;
            GLuint index;
            GLint size;
            GLenum type;
            GLboolean normalized;
            GLsizei stride;
            const GLvoid* pointer;
        };

        void SetDefaultBuffers(void* color_buffer, void* depth_buffer, void* stencil_buffer, int width, int height)
        {
            m_default_color_buffer = (unsigned char*) color_buffer;
            m_default_depth_buffer = (float*) depth_buffer;
            m_default_stencil_buffer = (unsigned char*) stencil_buffer;
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
                m_objects.Add(id, RefMake<T>(id));
                objs[i] = id;
            }
        }

        typedef std::function<void(const Ref<GLObject>&)> OnRemoveObject;

        template<class T>
        void DeleteObjects(GLsizei n, const GLuint* objs, OnRemoveObject on_remove = nullptr)
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

                        if (on_remove)
                        {
                            on_remove(obj);
                        }
                    }
                }
            }
        }

        template<class T>
        GLboolean ObjectIs(GLuint obj)
        {
            if (obj > 0)
            {
                Ref<GLObject>* find;
                if (m_objects.TryGet(obj, &find))
                {
                    if (RefCast<T>(*find))
                    {
                        return GL_TRUE;
                    }
                }
            }

            return GL_FALSE;
        }

        template<class T>
        Ref<T> ObjectGet(GLuint obj)
        {
            if (obj > 0)
            {
                Ref<GLObject>* find;
                if (m_objects.TryGet(obj, &find))
                {
                    return RefCast<T>(*find);
                }
            }

            return Ref<T>();
        }

        void GenFramebuffers(GLsizei n, GLuint* framebuffers)
        {
            this->GenObjects<GLFramebuffer>(n, framebuffers);
        }

        void DeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
        {
            this->DeleteObjects<GLFramebuffer>(n, framebuffers, [this](const Ref<GLObject>& obj) {
                if (!m_current_fb.expired() && m_current_fb.lock() == obj)
                {
                    m_current_fb.reset();
                }
            });
        }

        GLboolean IsFramebuffer(GLuint framebuffer)
        {
            return this->ObjectIs<GLFramebuffer>(framebuffer);
        }

        void BindFramebuffer(GLenum target, GLuint framebuffer)
        {
            if (target == GL_FRAMEBUFFER)
            {
                Ref<GLFramebuffer> fb = this->ObjectGet<GLFramebuffer>(framebuffer);
                if (fb)
                {
                    m_current_fb = fb;
                }
                else
                {
                    m_current_fb.reset();
                }
            }
        }

        void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
        {
            if (target == GL_FRAMEBUFFER)
            {
                if (renderbuffertarget == GL_RENDERBUFFER)
                {
                    if (!m_current_fb.expired())
                    {
                        Ref<GLFramebuffer> fb = m_current_fb.lock();
                        Ref<GLRenderbuffer> rb = this->ObjectGet<GLRenderbuffer>(renderbuffer);

                        GLFramebuffer::Attachment attach = fb->GetAttachment(attachment);
                        if (attach != GLFramebuffer::Attachment::None)
                        {
                            fb->SetAttachment(attach, rb);
                        }
                    }
                }
            }
        }

        void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
        {
            if (target == GL_FRAMEBUFFER)
            {
                if (!m_current_fb.expired())
                {
                    Ref<GLFramebuffer> fb = m_current_fb.lock();
                    
                    GLFramebuffer::Attachment attach = fb->GetAttachment(attachment);
                    if (attach != GLFramebuffer::Attachment::None)
                    {
                        fb->GetAttachmentParameteriv(attach, pname, params);
                    }
                }
            }
        }

        GLenum CheckFramebufferStatus(GLenum target)
        {
            if (target == GL_FRAMEBUFFER)
            {
                if (!m_current_fb.expired())
                {
                    Ref<GLFramebuffer> fb = m_current_fb.lock();
                    return fb->CheckStatus();
                }
            }

            return 0;
        }

        void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
        {
            unsigned char* color_buffer = m_default_color_buffer;
            int buffer_width = m_default_buffer_width;
            int buffer_height = m_default_buffer_height;

            switch (format)
            {
                case GL_RGBA:
                {
                    switch (type)
                    {
                        case GL_UNSIGNED_BYTE:
                            break;
                        case GL_UNSIGNED_SHORT_5_6_5:
                        case GL_UNSIGNED_SHORT_4_4_4_4:
                        case GL_UNSIGNED_SHORT_5_5_5_1:
                            return;
                    }
                    break;
                }
                case GL_ALPHA:
                case GL_RGB:
                    return;
            }

            if (!m_current_fb.expired())
            {
                color_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Color0, buffer_width, buffer_height);
            }

            int* dest = (int*) pixels;
            int* src = (int*) color_buffer;
            for (int i = 0; i < height; ++i)
            {
                Memory::Copy(&dest[i * width], &src[(buffer_height - 1 - y - i) * buffer_width + x], width * 4);
            }
        }

        void* GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment attachment, int& width, int& height)
        {
            void* buffer = nullptr;

            Ref<GLFramebuffer> fb = m_current_fb.lock();
            GLint attached_type;
            GLint attached_obj;
            fb->GetAttachmentParameteriv(attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attached_type);
            fb->GetAttachmentParameteriv(attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attached_obj);
            if (attached_type == GL_RENDERBUFFER)
            {
                Ref<GLRenderbuffer> rb = this->ObjectGet<GLRenderbuffer>(attached_obj);
                buffer = rb->GetBuffer();
                width = rb->GetWidth();
                height = rb->GetHeight();
            }
            else if (attached_type == GL_TEXTURE)
            {
                //TODO: render to texture
            }

            return buffer;
        }

        void GenRenderbuffers(GLsizei n, GLuint* renderbuffers)
        {
            this->GenObjects<GLRenderbuffer>(n, renderbuffers);
        }

        void DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
        {
            this->DeleteObjects<GLRenderbuffer>(n, renderbuffers, [this](const Ref<GLObject>& obj) {
                if (!m_current_rb.expired() && m_current_rb.lock() == obj)
                {
                    m_current_rb.reset();
                }
            });
        }

        GLboolean IsRenderbuffer(GLuint renderbuffer)
        {
            return this->ObjectIs<GLRenderbuffer>(renderbuffer);
        }

        void BindRenderbuffer(GLenum target, GLuint renderbuffer)
        {
            if (target == GL_RENDERBUFFER)
            {
                Ref<GLRenderbuffer> rb = this->ObjectGet<GLRenderbuffer>(renderbuffer);
                if (rb)
                {
                    m_current_rb = rb;
                }
                else
                {
                    m_current_rb.reset();
                }
            }
        }

        void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
        {
            if (target == GL_RENDERBUFFER)
            {
                if (!m_current_rb.expired())
                {
                    Ref<GLRenderbuffer> rb = m_current_rb.lock();
                    rb->Storage(internalformat, width, height);
                }
            }
        }

        void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
        {
            if (target == GL_RENDERBUFFER)
            {
                if (!m_current_rb.expired())
                {
                    Ref<GLRenderbuffer> rb = m_current_rb.lock();

                    switch (pname)
                    {
                        case GL_RENDERBUFFER_WIDTH:
                            *params = rb->GetWidth();
                            break;
                        case GL_RENDERBUFFER_HEIGHT:
                            *params = rb->GetHeight();
                            break;
                        case GL_RENDERBUFFER_INTERNAL_FORMAT:
                            *params = rb->GetInternalFormat();
                            break;
                        case GL_RENDERBUFFER_RED_SIZE:
                            *params = rb->GetRedComponentSize();
                            break;
                        case GL_RENDERBUFFER_GREEN_SIZE:
                            *params = rb->GetGreenComponentSize();
                            break;
                        case GL_RENDERBUFFER_BLUE_SIZE:
                            *params = rb->GetBlueComponentSize();
                            break;
                        case GL_RENDERBUFFER_ALPHA_SIZE:
                            *params = rb->GetAlphaComponentSize();
                            break;
                        case GL_RENDERBUFFER_DEPTH_SIZE:
                            *params = rb->GetDepthSize();
                            break;
                        case GL_RENDERBUFFER_STENCIL_SIZE:
                            *params = rb->GetStencilSize();
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

            unsigned char* color_buffer = m_default_color_buffer;
            float* depth_buffer = m_default_depth_buffer;
            unsigned char* stencil_buffer = m_default_stencil_buffer;
            int buffer_width = m_default_buffer_width;
            int buffer_height = m_default_buffer_height;

            if (!m_current_fb.expired())
            {
                color_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Color0, buffer_width, buffer_height);
                depth_buffer = (float*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Depth, buffer_width, buffer_height);
                stencil_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Stencil, buffer_width, buffer_height);
            }

            if (mask & GL_COLOR_BUFFER_BIT)
            {
                if (color_buffer)
                {
                    unsigned char r = FloatToColorByte(m_clear_color_red);
                    unsigned char g = FloatToColorByte(m_clear_color_green);
                    unsigned char b = FloatToColorByte(m_clear_color_blue);
                    unsigned char a = FloatToColorByte(m_clear_color_alpha);

                    for (int i = y; i < y + height && i < buffer_height; ++i)
                    {
                        for (int j = x; j < x + width && j < buffer_width; ++j)
                        {
                            color_buffer[i * buffer_width * 4 + j * 4 + 0] = r;
                            color_buffer[i * buffer_width * 4 + j * 4 + 1] = g;
                            color_buffer[i * buffer_width * 4 + j * 4 + 2] = b;
                            color_buffer[i * buffer_width * 4 + j * 4 + 3] = a;
                        }
                    }
                }
            }

            if (mask & GL_DEPTH_BUFFER_BIT)
            {
                if (m_depth_mask && depth_buffer)
                {
                    for (int i = y; i < y + height && i < buffer_height; ++i)
                    {
                        for (int j = x; j < x + width && j < buffer_width; ++j)
                        {
                            depth_buffer[i * buffer_width + j] = m_clear_depth;
                        }
                    }
                }
            }

            if (mask & GL_STENCIL_BUFFER_BIT)
            {
                if (stencil_buffer)
                {
                    for (int i = y; i < y + height && i < buffer_height; ++i)
                    {
                        for (int j = x; j < x + width && j < buffer_width; ++j)
                        {
                            stencil_buffer[i * buffer_width + j] = (unsigned char) m_clear_stencil;
                        }
                    }
                }
            }
        }

        GLuint CreateShader(GLenum type)
        {
            GLuint shader = 0;

            if (type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER)
            {
                this->GenObjects<GLShader>(1, &shader);
                Ref<GLShader> obj = this->ObjectGet<GLShader>(shader);
                obj->SetType(type);
            }

            return shader;
        }

        void DeleteShader(GLuint shader)
        {
            this->DeleteObjects<GLShader>(1, &shader);
        }

        GLboolean IsShader(GLuint shader)
        {
            return this->ObjectIs<GLShader>(shader);
        }

        void ShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)
        {
            Ref<GLShader> obj = this->ObjectGet<GLShader>(shader);
            if (obj)
            {
                obj->SetSource(count, string, length);
            }
        }

        void GetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source)
        {
            Ref<GLShader> obj = this->ObjectGet<GLShader>(shader);
            if (obj)
            {
                obj->GetSource(bufSize, length, source);
            }
        }

        void CompileShader(GLuint shader)
        {
            Ref<GLShader> obj = this->ObjectGet<GLShader>(shader);
            if (obj)
            {
                obj->Compile();
            }
        }

        GLuint CreateProgram()
        {
            GLuint program = 0;
            this->GenObjects<GLProgram>(1, &program);
            return program;
        }

        void DeleteProgram(GLuint program)
        {
            this->DeleteObjects<GLProgram>(1, &program);
        }

        GLboolean IsProgram(GLuint program)
        {
            return this->ObjectIs<GLProgram>(program);
        }

        void AttachShader(GLuint program, GLuint shader)
        {
            Ref<GLProgram> p = this->ObjectGet<GLProgram>(program);
            if (p)
            {
                Ref<GLShader> s = this->ObjectGet<GLShader>(shader);
                if (s)
                {
                    p->AttachShader(s);
                }
            }
        }

        void DetachShader(GLuint program, GLuint shader)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                obj->DetachShader(shader);
            }
        }

        void GetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                obj->GetAttachedShaders(maxCount, count, shaders);
            }
        }

        void BindAttribLocation(GLuint program, GLuint index, const GLchar* name)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                obj->BindAttribLocation(index, name);
            }
        }

        void LinkProgram(GLuint program)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                obj->Link();
            }
        }

        GLint GetAttribLocation(GLuint program, const GLchar* name)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                return obj->GetAttribLocation(name);
            }

            return -1;
        }

        GLint GetUniformLocation(GLuint program, const GLchar* name)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                return obj->GetUniformLocation(name);
            }

            return -1;
        }

        void UseProgram(GLuint program)
        {
            Ref<GLProgram> obj = this->ObjectGet<GLProgram>(program);
            if (obj)
            {
                m_using_program = obj;
                obj->Use();
            }
            else
            {
                m_using_program.reset();
            }
        }

        void Uniform4fv(GLint location, GLsizei count, const GLfloat* value)
        {
            if (!m_using_program.expired())
            {
                Ref<GLProgram> program = m_using_program.lock();
                program->Uniformv(location, count * sizeof(Vector4), value);
            }
        }

        void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
        {
            if (!m_using_program.expired())
            {
                Ref<GLProgram> program = m_using_program.lock();
                program->UniformMatrix4fv(location, count, transpose, value);
            }
        }
        
        void GenBuffers(GLsizei n, GLuint* buffers)
        {
            this->GenObjects<GLBuffer>(n, buffers);
        }

        void DeleteBuffers(GLsizei n, const GLuint* buffers)
        {
            this->DeleteObjects<GLBuffer>(n, buffers);
        }

        GLboolean IsBuffer(GLuint buffer)
        {
            return this->ObjectIs<GLBuffer>(buffer);
        }

        void BindBuffer(GLenum target, GLuint buffer)
        {
            Ref<GLBuffer> obj = this->ObjectGet<GLBuffer>(buffer);
            switch (target)
            {
                case GL_ARRAY_BUFFER:
                    if (obj)
                    {
                        m_current_vb = obj;
                    }
                    else
                    {
                        m_current_vb.reset();
                    }
                    break;
                case GL_ELEMENT_ARRAY_BUFFER:
                    if (obj)
                    {
                        m_current_ib = obj;
                    }
                    else
                    {
                        m_current_ib.reset();
                    }
                    break;
                default:
                    break;
            }
        }

        void BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
        {
            switch (target)
            {
                case GL_ARRAY_BUFFER:
                    if (!m_current_vb.expired())
                    {
                        m_current_vb.lock()->BufferData(size, data, usage);
                    }
                    break;
                case GL_ELEMENT_ARRAY_BUFFER:
                    if (!m_current_ib.expired())
                    {
                        m_current_ib.lock()->BufferData(size, data, usage);
                    }
                    break;
                default:
                    break;
            }
        }

        void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
        {
            switch (target)
            {
                case GL_ARRAY_BUFFER:
                    if (!m_current_vb.expired())
                    {
                        m_current_vb.lock()->BufferSubData(offset, size, data);
                    }
                    break;
                case GL_ELEMENT_ARRAY_BUFFER:
                    if (!m_current_ib.expired())
                    {
                        m_current_ib.lock()->BufferSubData(offset, size, data);
                    }
                    break;
                default:
                    break;
            }
        }

        void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)
        {
            VertexAttribArray va;
            va.enable = false;
            va.index = index;
            va.size = size;
            va.type = type;
            va.normalized = normalized;
            va.stride = stride;
            va.pointer = pointer;

            int exist_index = -1;
            for (int i = 0; i < m_vertex_attrib_arrays.Size(); ++i)
            {
                if (m_vertex_attrib_arrays[i].index == index)
                {
                    exist_index = i;
                    break;
                }
            }

            if (exist_index >= 0)
            {
                va.enable = m_vertex_attrib_arrays[exist_index].enable;
                m_vertex_attrib_arrays[exist_index] = va;
            }
            else
            {
                m_vertex_attrib_arrays.Add(va);
            }
        }

        void EnableVertexAttribArray(GLuint index)
        {
            int exist_index = -1;
            for (int i = 0; i < m_vertex_attrib_arrays.Size(); ++i)
            {
                if (m_vertex_attrib_arrays[i].index == index)
                {
                    exist_index = i;
                    break;
                }
            }

            if (exist_index >= 0)
            {
                m_vertex_attrib_arrays[exist_index].enable = true;
            }
            else
            {
                VertexAttribArray va;
                va.enable = true;
                va.index = index;
                va.size = 0;
                va.type = 0;
                va.normalized = 0;
                va.stride = 0;
                va.pointer = 0;

                m_vertex_attrib_arrays.Add(va);
            }
        }

        void DisableVertexAttribArray(GLuint index)
        {
            int exist_index = -1;
            for (int i = 0; i < m_vertex_attrib_arrays.Size(); ++i)
            {
                if (m_vertex_attrib_arrays[i].index == index)
                {
                    exist_index = i;
                    break;
                }
            }

            if (exist_index >= 0)
            {
                m_vertex_attrib_arrays[exist_index].enable = false;
            }
        }

        void DrawArrays(GLenum mode, GLint first, GLsizei count)
        {
            switch (mode)
            {
                case GL_TRIANGLES:
                    this->DrawArraysTriangles(first, count / 3);
                    break;
                default:
                    break;
            }
        }

        void DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
        {
            switch (mode)
            {
                case GL_TRIANGLES:
                    this->DrawElementsTriangles(count / 3, type, indices);
                    break;
                default:
                    break;
            }
        }

        void ApplyVertexAttribs(const Ref<GLProgram>& program, unsigned int index)
        {
            for (int k = 0; k < m_vertex_attrib_arrays.Size(); ++k) // attrib
            {
                const VertexAttribArray& va = m_vertex_attrib_arrays[k];
                if (va.enable)
                {
                    int size = 0;
                    switch (va.type)
                    {
                        case GL_BYTE:
                        case GL_UNSIGNED_BYTE:
                            size = va.size * 1;
                            break;
                        case GL_SHORT:
                        case GL_UNSIGNED_SHORT:
                        case GL_FIXED:
                            size = va.size * 2;
                            break;
                        case GL_FLOAT:
                            size = va.size * 4;
                            break;
                        default:
                            break;
                    }

                    if (!m_current_vb.expired())
                    {
                        Ref<GLBuffer> vb = m_current_vb.lock();
                        char* p = (char*) vb->GetData();
                        int offset = (int) (size_t) va.pointer;
                        program->SetVertexAttrib(va.index, &p[index * va.stride + offset], size);
                    }
                    else
                    {
                        char* p = (char*) va.pointer;
                        program->SetVertexAttrib(va.index, &p[index * va.stride], size);
                    }
                }
            }
        }

        void Rasterize(unsigned char* color_buffer, float* depth_buffer, int buffer_width, int buffer_height, const Ref<GLProgram>& program,
            const Vector4* positions, const Vector<GLProgram::Varying>* varyings)
        {
            float cross = (positions[1].x - positions[0].x) * (positions[2].y - positions[1].y)
                - (positions[2].x - positions[1].x) * (positions[1].y - positions[0].y);

            if (m_cull_face_enable == false || this->CullFaceTest(cross))
            {
                auto set_fragment = [=](const Vector2i& p, const Viry3D::Vector4& c, float depth) {
                    if (p.x >= 0 && p.x <= buffer_width - 1 &&
                        p.y >= 0 && p.y <= buffer_height - 1)
                    {
                        float old_depth = depth_buffer[p.y * buffer_width + p.x];
                        float mapped_depth = m_depth_range.x + (depth + 1) / 2 * (m_depth_range.y - m_depth_range.x);

                        if (m_depth_test_enable == false || DepthTest(mapped_depth, old_depth))
                        {
                            color_buffer[p.y * buffer_width * 4 + p.x * 4 + 0] = this->FloatToColorByte(c.x);
                            color_buffer[p.y * buffer_width * 4 + p.x * 4 + 1] = this->FloatToColorByte(c.y);
                            color_buffer[p.y * buffer_width * 4 + p.x * 4 + 2] = this->FloatToColorByte(c.z);
                            color_buffer[p.y * buffer_width * 4 + p.x * 4 + 3] = this->FloatToColorByte(c.w);

                            if (m_depth_mask)
                            {
                                depth_buffer[p.y * buffer_width + p.x] = mapped_depth;
                            }
                        }
                    }
                };

                GLRasterizer rasterizer(positions, varyings, program.get(), set_fragment, m_viewport_x, m_viewport_y, m_viewport_width, m_viewport_height, cross > 0);
                rasterizer.Run();
            }
        }

        void DrawArraysTriangles(GLint first, GLsizei count)
        {
            unsigned char* color_buffer = m_default_color_buffer;
            float* depth_buffer = m_default_depth_buffer;
            unsigned char* stencil_buffer = m_default_stencil_buffer;
            int buffer_width = m_default_buffer_width;
            int buffer_height = m_default_buffer_height;

            if (!m_current_fb.expired())
            {
                color_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Color0, buffer_width, buffer_height);
                depth_buffer = (float*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Depth, buffer_width, buffer_height);
                stencil_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Stencil, buffer_width, buffer_height);
            }

            Ref<GLProgram> program = m_using_program.lock();

            for (int i = 0; i < count; ++i) // triangle
            {
                Vector4 positions[3];
                Vector<GLProgram::Varying> varyings[3];

                for (int j = 0; j < 3; ++j) // vertex
                {
                    unsigned int index = first + i * 3 + j;

                    this->ApplyVertexAttribs(program, index);

                    positions[j] = *(Vector4*) program->CallVSMain();
                    varyings[j] = program->GetVSVaryings();
                }

                this->Rasterize(color_buffer, depth_buffer, buffer_width, buffer_height, program,
                    positions, varyings);
            }
        }

        void DrawElementsTriangles(GLsizei count, GLenum type, const void* indices)
        {
            unsigned char* color_buffer = m_default_color_buffer;
            float* depth_buffer = m_default_depth_buffer;
            unsigned char* stencil_buffer = m_default_stencil_buffer;
            int buffer_width = m_default_buffer_width;
            int buffer_height = m_default_buffer_height;

            if (!m_current_fb.expired())
            {
                color_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Color0, buffer_width, buffer_height);
                depth_buffer = (float*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Depth, buffer_width, buffer_height);
                stencil_buffer = (unsigned char*) this->GetFramebufferAttachmentBuffer(GLFramebuffer::Attachment::Stencil, buffer_width, buffer_height);
            }

            Ref<GLProgram> program = m_using_program.lock();

            int index_type_size = 0;
            switch (type)
            {
                case GL_UNSIGNED_BYTE:
                    index_type_size = 1;
                    break;
                case GL_UNSIGNED_SHORT:
                    index_type_size = 2;
                    break;
                case GL_UNSIGNED_INT:
                    index_type_size = 4;
                    break;
                default:
                    break;
            }

            for (int i = 0; i < count; ++i) // triangle
            {
                Vector4 positions[3];
                Vector<GLProgram::Varying> varyings[3];

                for (int j = 0; j < 3; ++j) // vertex
                {
                    unsigned int index = 0;
                    char* index_addr = nullptr;

                    if (!m_current_ib.expired())
                    {
                        Ref<GLBuffer> ib = m_current_ib.lock();
                        char* p = (char*) ib->GetData();
                        int offset = (int) (size_t) indices;
                        index_addr = (char*) &p[offset + (i * 3 + j) * index_type_size];
                    }
                    else
                    {
                        char* p = (char*) indices;
                        index_addr = (char*) &p[(i * 3 + j) * index_type_size];
                    }

                    switch (type)
                    {
                        case GL_UNSIGNED_BYTE:
                            index = *(unsigned char*) index_addr;
                            break;
                        case GL_UNSIGNED_SHORT:
                            index = *(unsigned short*) index_addr;
                            break;
                        case GL_UNSIGNED_INT:
                            index = *(unsigned int*) index_addr;
                            break;
                        default:
                            break;
                    }

                    this->ApplyVertexAttribs(program, index);

                    positions[j] = *(Vector4*) program->CallVSMain();
                    varyings[j] = program->GetVSVaryings();
                }

                this->Rasterize(color_buffer, depth_buffer, buffer_width, buffer_height, program,
                    positions, varyings);
            }
        }

        void Enable(GLenum cap)
        {
            switch (cap)
            {
                case GL_DEPTH_TEST:
                    m_depth_test_enable = true;
                    break;
                case GL_CULL_FACE:
                    m_cull_face_enable = true;
                    break;
                case GL_BLEND:
                    break;
                default:
                    break;
            }
        }

        void Disable(GLenum cap)
        {
            switch (cap)
            {
                case GL_DEPTH_TEST:
                    m_depth_test_enable = false;
                    break;
                case GL_CULL_FACE:
                    m_cull_face_enable = false;
                    break;
                case GL_BLEND:
                    break;
                default:
                    break;
            }
        }

        void DepthMask(GLboolean flag)
        {
            m_depth_mask = flag == GL_TRUE;
        }

        void DepthRangef(GLfloat n, GLfloat f)
        {
            m_depth_range = Vector2(n, f);
        }

        void DepthFunc(GLenum func)
        {
            m_depth_func = func;
        }

        void CullFace(GLenum mode)
        {
            m_cull_face = mode;
        }

        void FrontFace(GLenum mode)
        {
            m_front_face = mode;
        }

        bool DepthTest(float src, float dest)
        {
            switch (m_depth_func)
            {
                case GL_NEVER:
                    return false;
                case GL_LESS:
                    return src < dest;
                case GL_EQUAL:
                    return Mathf::FloatEqual(src, dest);
                case GL_LEQUAL:
                    return src <= dest;
                case GL_GREATER:
                    return src > dest;
                case GL_NOTEQUAL:
                    return !Mathf::FloatEqual(src, dest);
                case GL_GEQUAL:
                    return src >= dest;
                case GL_ALWAYS:
                    return true;
                default:
                    return false;
            }
        }

        bool CullFaceTest(float cross)
        {
            switch (m_cull_face)
            {
                case GL_BACK:
                    switch (m_front_face)
                    {
                        case GL_CCW:
                            return cross > 0;
                        case GL_CW:
                            return cross < 0;
                    }
                    break;
                case GL_FRONT:
                    switch (m_front_face)
                    {
                        case GL_CCW:
                            return cross < 0;
                        case GL_CW:
                            return cross > 0;
                    }
                    break;
                case GL_FRONT_AND_BACK:
                    return false;
            }

            return false;
        }

        GLContext():
            m_default_color_buffer(nullptr),
            m_default_depth_buffer(nullptr),
            m_default_stencil_buffer(nullptr),
            m_default_buffer_width(0),
            m_default_buffer_height(0),
            m_gen_id(0),
            m_viewport_x(-1),
            m_viewport_y(-1),
            m_viewport_width(-1),
            m_viewport_height(-1),
            m_clear_color_red(0.0f),
            m_clear_color_green(0.0f),
            m_clear_color_blue(0.0f),
            m_clear_color_alpha(1.0f),
            m_clear_depth(1.0f),
            m_clear_stencil(0),
            m_depth_test_enable(false),
            m_depth_mask(true),
            m_depth_range(0, 1),
            m_depth_func(GL_LESS),
            m_cull_face_enable(false),
            m_cull_face(GL_BACK),
            m_front_face(GL_CCW)
        {
        }

        ~GLContext()
        {
            assert(m_objects.Size() == 0);
        }

    private:
        unsigned char FloatToColorByte(float f)
        {
            return (unsigned char) (Mathf::Clamp01(f) * 255);
        }

    private:
        unsigned char* m_default_color_buffer;
        float* m_default_depth_buffer;
        unsigned char* m_default_stencil_buffer;
        int m_default_buffer_width;
        int m_default_buffer_height;
        Map<GLuint, Ref<GLObject>> m_objects;
        GLuint m_gen_id;
        WeakRef<GLFramebuffer> m_current_fb;
        WeakRef<GLRenderbuffer> m_current_rb;
        WeakRef<GLBuffer> m_current_vb;
        WeakRef<GLBuffer> m_current_ib;
        WeakRef<GLProgram> m_using_program;
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
        Vector<VertexAttribArray> m_vertex_attrib_arrays;
        bool m_depth_test_enable;
        bool m_depth_mask;
        Vector2 m_depth_range;
        GLenum m_depth_func;
        bool m_cull_face_enable;
        GLenum m_cull_face;
        GLenum m_front_face;
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

__declspec(dllexport) void set_gl_context_default_buffers(void* color_buffer, void* depth_buffer, void* stencil_buffer, int width, int height)
{
    gl->SetDefaultBuffers(color_buffer, depth_buffer, stencil_buffer, width, height);
}

#define NOT_IMPLEMENT_VOID_GL_FUNC(func) \
    void GL_APIENTRY gl##func { \
    }
#define NOT_IMPLEMENT_GL_FUNC(ret, func) \
    ret GL_APIENTRY gl##func { \
    }
#define IMPLEMENT_VOID_GL_FUNC_0(func) \
    void GL_APIENTRY gl##func() { \
        gl->func(); \
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
#define IMPLEMENT_VOID_GL_FUNC_5(func, t1, t2, t3, t4, t5) \
    void GL_APIENTRY gl##func(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) { \
        gl->func(p1, p2, p3, p4, p5); \
    }
#define IMPLEMENT_VOID_GL_FUNC_6(func, t1, t2, t3, t4, t5, t6) \
    void GL_APIENTRY gl##func(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6) { \
        gl->func(p1, p2, p3, p4, p5, p6); \
    }
#define IMPLEMENT_VOID_GL_FUNC_7(func, t1, t2, t3, t4, t5, t6, t7) \
    void GL_APIENTRY gl##func(t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) { \
        gl->func(p1, p2, p3, p4, p5, p6, p7); \
    }
#define IMPLEMENT_GL_FUNC_0(ret, func) \
    ret GL_APIENTRY gl##func() { \
        return gl->func(); \
    }
#define IMPLEMENT_GL_FUNC_1(ret, func, t1) \
    ret GL_APIENTRY gl##func(t1 p1) { \
        return gl->func(p1); \
    }
#define IMPLEMENT_GL_FUNC_2(ret, func, t1, t2) \
    ret GL_APIENTRY gl##func(t1 p1, t2 p2) { \
        return gl->func(p1, p2); \
    }

// Framebuffer
IMPLEMENT_VOID_GL_FUNC_2(GenFramebuffers, GLsizei, GLuint*)
IMPLEMENT_VOID_GL_FUNC_2(DeleteFramebuffers, GLsizei, const GLuint*)
IMPLEMENT_GL_FUNC_1(GLboolean, IsFramebuffer, GLuint)
IMPLEMENT_VOID_GL_FUNC_2(BindFramebuffer, GLenum, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(FramebufferRenderbuffer, GLenum, GLenum, GLenum, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(GetFramebufferAttachmentParameteriv, GLenum, GLenum, GLenum, GLint*)
IMPLEMENT_GL_FUNC_1(GLenum, CheckFramebufferStatus, GLenum)
IMPLEMENT_VOID_GL_FUNC_7(ReadPixels, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*)

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

// Shader
IMPLEMENT_GL_FUNC_1(GLuint, CreateShader, GLenum)
IMPLEMENT_VOID_GL_FUNC_1(DeleteShader, GLuint)
IMPLEMENT_GL_FUNC_1(GLboolean, IsShader, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(ShaderSource, GLuint, GLsizei, const GLchar* const*, const GLint*)
IMPLEMENT_VOID_GL_FUNC_4(GetShaderSource, GLuint, GLsizei, GLsizei*, GLchar*)
IMPLEMENT_VOID_GL_FUNC_1(CompileShader, GLuint)
NOT_IMPLEMENT_VOID_GL_FUNC(ShaderBinary(GLsizei, const GLuint*, GLenum binaryformat, const void*, GLsizei))
NOT_IMPLEMENT_VOID_GL_FUNC(ReleaseShaderCompiler())
NOT_IMPLEMENT_VOID_GL_FUNC(GetShaderPrecisionFormat(GLenum, GLenum, GLint*, GLint*))
NOT_IMPLEMENT_VOID_GL_FUNC(GetShaderiv(GLuint, GLenum, GLint*))
NOT_IMPLEMENT_VOID_GL_FUNC(GetShaderInfoLog(GLuint, GLsizei, GLsizei*, GLchar*))

//Program
IMPLEMENT_GL_FUNC_0(GLuint, CreateProgram)
IMPLEMENT_VOID_GL_FUNC_1(DeleteProgram, GLuint)
IMPLEMENT_GL_FUNC_1(GLboolean, IsProgram, GLuint)
IMPLEMENT_VOID_GL_FUNC_2(AttachShader, GLuint, GLuint)
IMPLEMENT_VOID_GL_FUNC_2(DetachShader, GLuint, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(GetAttachedShaders, GLuint, GLsizei, GLsizei*, GLuint*)
IMPLEMENT_VOID_GL_FUNC_3(BindAttribLocation, GLuint, GLuint, const GLchar*)
IMPLEMENT_VOID_GL_FUNC_1(LinkProgram, GLuint)
IMPLEMENT_GL_FUNC_2(GLint, GetAttribLocation, GLuint, const GLchar*)
IMPLEMENT_GL_FUNC_2(GLint, GetUniformLocation, GLuint, const GLchar*)
IMPLEMENT_VOID_GL_FUNC_1(UseProgram, GLuint)
IMPLEMENT_VOID_GL_FUNC_3(Uniform4fv, GLint, GLsizei, const GLfloat*)
IMPLEMENT_VOID_GL_FUNC_4(UniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat*)

// Buffer
IMPLEMENT_VOID_GL_FUNC_2(GenBuffers, GLsizei, GLuint*)
IMPLEMENT_VOID_GL_FUNC_2(DeleteBuffers, GLsizei, const GLuint*)
IMPLEMENT_GL_FUNC_1(GLboolean, IsBuffer, GLuint)
IMPLEMENT_VOID_GL_FUNC_2(BindBuffer, GLenum, GLuint)
IMPLEMENT_VOID_GL_FUNC_4(BufferData, GLenum, GLsizeiptr, const void*, GLenum)
IMPLEMENT_VOID_GL_FUNC_4(BufferSubData, GLenum, GLintptr, GLsizeiptr, const void*)

// Draw
IMPLEMENT_VOID_GL_FUNC_6(VertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)
IMPLEMENT_VOID_GL_FUNC_1(EnableVertexAttribArray, GLuint)
IMPLEMENT_VOID_GL_FUNC_1(DisableVertexAttribArray, GLuint)
IMPLEMENT_VOID_GL_FUNC_3(DrawArrays, GLenum, GLint, GLsizei)
IMPLEMENT_VOID_GL_FUNC_4(DrawElements, GLenum, GLsizei, GLenum, const void*)

// State
IMPLEMENT_VOID_GL_FUNC_1(Enable, GLenum)
IMPLEMENT_VOID_GL_FUNC_1(Disable, GLenum)
IMPLEMENT_VOID_GL_FUNC_1(DepthMask, GLboolean)
IMPLEMENT_VOID_GL_FUNC_2(DepthRangef, GLfloat, GLfloat)
IMPLEMENT_VOID_GL_FUNC_1(DepthFunc, GLenum)
IMPLEMENT_VOID_GL_FUNC_1(CullFace, GLenum)
IMPLEMENT_VOID_GL_FUNC_1(FrontFace, GLenum)

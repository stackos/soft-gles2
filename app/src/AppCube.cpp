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

#include <Windows.h>
#include "display/DisplayWindows.h"
#include "GLES2/gl2.h"
#include "Debug.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"

using namespace Viry3D;

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

struct Vertex
{
    Vector3 pos;
    Vector2 uv;
    Color color;
};

class Renderer
{
public:
    Renderer()
    {
        glViewport(0, 0, 1280, 720);
        glClearColor(0, 0, 0, 1);
        glClearDepthf(1);
        glClearStencil(0);

        // color rb
        glGenRenderbuffers(1, &m_rb_color);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rb_color);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, 1280, 720);
        
        // depth rb
        glGenRenderbuffers(1, &m_rb_depth);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rb_depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 1280, 720);

        // stencil rb
        glGenRenderbuffers(1, &m_rb_stencil);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rb_stencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 1280, 720);

        // fb
        glGenFramebuffers(1, &m_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fb);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rb_color);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rb_depth);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rb_stencil);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            Log("frame buffer status ok");
        }

        // shader
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        const char* vs_src = "\
attribute vec4 a_position;\n\
attribute vec4 a_color;\n\
varying vec4 v_color;\n\
void main()\n\
{\n\
    gl_Position = a_position;\n\
    v_color = a_color;\n\
}";
        glShaderSource(vs, 1, (const GLchar* const*) &vs_src, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        const char* ps_src = "\
precision highp float;\n\
varying vec4 v_color;\n\
void main()\n\
{\n\
    gl_FragColor = v_color;\n\
}";
        glShaderSource(fs, 1, (const GLchar* const*) &ps_src, nullptr);
        glCompileShader(fs);

        m_program = glCreateProgram();

        glAttachShader(m_program, vs);
        glAttachShader(m_program, fs);

        glBindAttribLocation(m_program, 0, "a_position");
        glBindAttribLocation(m_program, 1, "a_color");

        glLinkProgram(m_program);

        // vb
        glGenBuffers(1, &m_vb);
        glBindBuffer(GL_ARRAY_BUFFER, m_vb);

        Vertex vertices[3] = {
            { Vector3(-0.5f, 0.5f, 0), Vector2(0, 0), Color(1, 0, 0, 1) },
            { Vector3(-0.5f, -0.5f, 0), Vector2(0, 1), Color(0, 1, 0, 1) },
            { Vector3(0.5f, -0.5f, 0), Vector2(1, 1), Color(0, 0, 1, 1) },
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        // ib
        glGenBuffers(1, &m_ib);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib);

        unsigned short indices[6] = { 0, 1, 2, 0, 2, 3 };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

        // for test api
        {
            glIsRenderbuffer(m_rb_color);
            GLint width;
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);

            glIsFramebuffer(m_fb);
            GLint type;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
            GLint name;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

            glIsShader(vs);

            char buffer[1024];
            glGetShaderSource(vs, 1024, nullptr, buffer);
            glShaderBinary(0, nullptr, 0, nullptr, 0);
            glReleaseShaderCompiler();
            glGetShaderPrecisionFormat(0, 0, nullptr, nullptr);
            glGetShaderiv(0, 0, nullptr);
            glGetShaderInfoLog(0, 0, nullptr, nullptr);

            glIsProgram(m_program);

            int count;
            GLuint shaders[10];
            glGetAttachedShaders(m_program, 10, &count, shaders);

            glIsBuffer(m_vb);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), &vertices[0]);
        }

        //glFramebufferTexture2D

        //glGenTextures
        //glDeleteTextures
        //glIsTexture
        //glBindTexture
        //glActiveTexture
        //glCopyTexImage2D
        //glCopyTexSubImage2D
        //glGetTexParameterfv
        //glGetTexParameteriv
        //glTexImage2D
        //glTexParameterf
        //glTexParameterfv
        //glTexParameteri
        //glTexParameteriv
        //glTexSubImage2D
        //glCompressedTexImage2D
        //glCompressedTexSubImage2D

        glDeleteShader(vs);
        glDeleteShader(fs);

        // set to default frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    virtual ~Renderer()
    {
        glDeleteFramebuffers(1, &m_fb);
        glDeleteRenderbuffers(1, &m_rb_color);
        glDeleteRenderbuffers(1, &m_rb_depth);
        glDeleteRenderbuffers(1, &m_rb_stencil);
        glDeleteProgram(m_program);
        glDeleteBuffers(1, &m_vb);
        glDeleteBuffers(1, &m_ib);
    }

    void Draw()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        int loc_a_position = glGetAttribLocation(m_program, "a_position");
        int loc_a_color = glGetAttribLocation(m_program, "a_color");

        glBindBuffer(GL_ARRAY_BUFFER, m_vb);

        glVertexAttribPointer(loc_a_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) 0);
        glVertexAttribPointer(loc_a_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) (sizeof(Vector3) + sizeof(Vector2)));
        glEnableVertexAttribArray(loc_a_position);
        glEnableVertexAttribArray(loc_a_color);

        glUseProgram(m_program);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        //glDrawElements

        glDisableVertexAttribArray(loc_a_position);
        glDisableVertexAttribArray(loc_a_color);
    }

    GLuint m_fb;
    GLuint m_rb_color;
    GLuint m_rb_depth;
    GLuint m_rb_stencil;
    GLuint m_program;
    GLuint m_vb;
    GLuint m_ib;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    DisplayWindows* display = new DisplayWindows("soft-gles2", 1280, 720);
    Renderer* renderer = new Renderer();

    while(display->ProcessSystemEvents())
    {
        renderer->Draw();
        display->SwapBuffers();
    }

    delete renderer;
    delete display;

    return 0;
}

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

class Renderer
{
public:
    Renderer()
    {
        glViewport(100, 100, 200, 200);
        glClearColor(1, 0, 0, 1);
        glClearDepthf(1);
        glClearStencil(0);

        // color rbo
        glGenRenderbuffers(1, &m_rbo_color);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_color);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, 1280, 720);
        
        // depth rbo
        glGenRenderbuffers(1, &m_rbo_depth);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 1280, 720);

        // stencil rbo
        glGenRenderbuffers(1, &m_rbo_stencil);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_stencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 1280, 720);

        // fbo
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_color);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_stencil);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            // frame buffer status ok
        }

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        const char* vs_src = "\
uniform mat4 u_mvp;\n\
attribute vec4 a_position;\n\
attribute vec2 a_uv;\n\
attribute vec4 a_color;\n\
varying vec2 v_uv;\n\
varying vec4 v_color;\n\
void main()\n\
{\n\
    gl_Position = u_mvp * a_position;\n\
    v_uv = a_uv;\n\
    v_color = a_color;\n\
}";
        glShaderSource(vs, 1, (const GLchar* const*) &vs_src, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        const char* ps_src = "\
precision highp float;\n\
uniform sampler2D u_tex;\n\
varying vec2 v_uv;\n\
varying vec4 v_color;\n\
void main()\n\
{\n\
    gl_FragColor = texture2D(u_tex, v_uv) * v_color;\n\
}";
        glShaderSource(fs, 1, (const GLchar* const*) &ps_src, nullptr);
        glCompileShader(fs);

        // for test api
        {
            glIsRenderbuffer(m_rbo_color);
            GLint width;
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);

            glIsFramebuffer(m_fbo);
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
        }

        //glGetShaderiv
        //glGetShaderInfoLog
        
        //glCreateProgram
        //glDeleteProgram
        //glAttachShader
        //glDetachShader
        //glGetAttachedShaders

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
        glDeleteFramebuffers(1, &m_fbo);
        glDeleteRenderbuffers(1, &m_rbo_color);
        glDeleteRenderbuffers(1, &m_rbo_depth);
        glDeleteRenderbuffers(1, &m_rbo_stencil);
    }

    void Draw()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    GLuint m_fbo;
    GLuint m_rbo_color;
    GLuint m_rbo_depth;
    GLuint m_rbo_stencil;
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

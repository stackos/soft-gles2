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

        glGenFramebuffers(1, &m_fbo);
        glIsFramebuffer(m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        
        glGenRenderbuffers(1, &m_rbo);
        glIsRenderbuffer(m_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
        //glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, 1280, 720);
        //GLint width;
        //glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);

        //glFramebufferRenderbuffer
        //glFramebufferTexture2D
        //glGetFramebufferAttachmentParameteriv
        //glCheckFramebufferStatus
        
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
    }

    virtual ~Renderer()
    {
        glDeleteFramebuffers(1, &m_fbo);
        glDeleteRenderbuffers(1, &m_rbo);
    }

    void Draw()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    GLuint m_fbo;
    GLuint m_rbo;
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

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
#include "io/File.h"
#include "graphics/Image.h"

using namespace Viry3D;

struct Vertex
{
    Vector3 pos;
    Vector2 uv;
    Vector4 color;
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

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthRangef(0, 1);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);

        // shader
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
        const char* fs_src = "\
precision highp float;\n\
uniform sampler2D u_tex;\n\
uniform vec4 u_color;\n\
varying vec2 v_uv;\n\
varying vec4 v_color;\n\
void main()\n\
{\n\
    gl_FragColor = texture2D(u_tex, v_uv);// * v_color * u_color;\n\
}";
        glShaderSource(fs, 1, (const GLchar* const*) &fs_src, nullptr);
        glCompileShader(fs);

        m_program = glCreateProgram();

        glAttachShader(m_program, vs);
        glAttachShader(m_program, fs);

        glBindAttribLocation(m_program, 0, "a_position");
        glBindAttribLocation(m_program, 1, "a_uv");
        glBindAttribLocation(m_program, 2, "a_color");

        glLinkProgram(m_program);

        // vb
        glGenBuffers(1, &m_vb);
        glBindBuffer(GL_ARRAY_BUFFER, m_vb);

        Vertex vertices[] = {
            { Vector3(-0.5f, 0.5f, -0.5f), Vector2(0, 0), Vector4(0, 0, 0, 1) },
            { Vector3(-0.5f, -0.5f, -0.5f), Vector2(0, 1), Vector4(1, 0, 0, 1) },
            { Vector3(0.5f, -0.5f, -0.5f), Vector2(1, 1), Vector4(0, 1, 0, 1) },
            { Vector3(0.5f, 0.5f, -0.5f), Vector2(1, 0), Vector4(0, 0, 1, 1) },
            { Vector3(-0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector4(0, 1, 1, 1) },
            { Vector3(-0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector4(1, 0, 1, 1) },
            { Vector3(0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector4(1, 1, 0, 1) },
            { Vector3(0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector4(1, 1, 1, 1) },
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        // ib
        glGenBuffers(1, &m_ib);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib);

        unsigned short indices[] = {
            0, 1, 2, 0, 2, 3,
            3, 2, 6, 3, 6, 7,
            7, 6, 5, 7, 5, 4,
            4, 5, 1, 4, 1, 0,
            4, 0, 3, 4, 3, 7,
            1, 5, 6, 1, 6, 2,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

        // tex
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        int width;
        int height;
        int bpp;
        ByteBuffer image = Image::LoadPNG(File::ReadAllBytes("Assets/texture/girl.png"), width, height, bpp);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.Bytes());

        glDeleteShader(vs);
        glDeleteShader(fs);

        m_deg = 0;
    }

    virtual ~Renderer()
    {
        glDeleteProgram(m_program);
        glDeleteBuffers(1, &m_vb);
        glDeleteBuffers(1, &m_ib);
        glDeleteTextures(1, &m_tex);
    }

    void Draw()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(m_program);

        Matrix4x4 model = Matrix4x4::Rotation(Quaternion::Euler(0, m_deg, 0)) * Matrix4x4::Scaling(Vector3(1, 2, 1));
        Matrix4x4 view = Matrix4x4::LookTo(
            Vector3(0, 0, -4),
            Vector3(0, 0, 1),
            Vector3(0, 1, 0));
        Matrix4x4 proj = Matrix4x4::Perspective(60, 1280 / 720.0f, 0.3f, 1000);
        
        Matrix4x4 u_mvp = proj * view * model;
        Vector4 u_color(1, 1, 1, 1);

        int loc_u_mvp = glGetUniformLocation(m_program, "u_mvp");
        int loc_u_color = glGetUniformLocation(m_program, "u_color");
        int loc_u_tex = glGetUniformLocation(m_program, "u_tex");

        glUniformMatrix4fv(loc_u_mvp, 1, true, (const GLfloat*) &u_mvp);
        glUniform4fv(loc_u_color, 1, (const GLfloat*) &u_color);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glUniform1i(loc_u_tex, 0);

        int loc_a_position = glGetAttribLocation(m_program, "a_position");
        int loc_a_uv = glGetAttribLocation(m_program, "a_uv");
        int loc_a_color = glGetAttribLocation(m_program, "a_color");

        glBindBuffer(GL_ARRAY_BUFFER, m_vb);
        glVertexAttribPointer(loc_a_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) 0);
        glVertexAttribPointer(loc_a_uv, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) sizeof(Vector3));
        glVertexAttribPointer(loc_a_color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) (sizeof(Vector3) + sizeof(Vector2)));
        glEnableVertexAttribArray(loc_a_position);
        glEnableVertexAttribArray(loc_a_uv);
        glEnableVertexAttribArray(loc_a_color);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ib);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (const void*) 0);

        glDisableVertexAttribArray(loc_a_position);
        glDisableVertexAttribArray(loc_a_uv);
        glDisableVertexAttribArray(loc_a_color);

        m_deg += 1;
    }

    GLuint m_program;
    GLuint m_vb;
    GLuint m_ib;
    GLuint m_tex;
    float m_deg;
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

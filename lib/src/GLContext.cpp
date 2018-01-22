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

using namespace Viry3D;

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
        if (mask & GL_COLOR_BUFFER_BIT)
        {
            unsigned char r = FloatToColorByte(m_clear_color_red);
            unsigned char g = FloatToColorByte(m_clear_color_green);
            unsigned char b = FloatToColorByte(m_clear_color_blue);
            unsigned char a = FloatToColorByte(m_clear_color_alpha);

            for (int i = 0; i < m_default_buffer_height; ++i)
            {
                for (int j = 0; j < m_default_buffer_width; ++j)
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
            for (int i = 0; i < m_default_buffer_height; ++i)
            {
                for (int j = 0; j < m_default_buffer_width; ++j)
                {
                    m_default_depth_buffer[i * m_default_buffer_width + j] = m_clear_depth;
                }
            }
        }
    }

    GLContext():
        m_default_color_buffer(nullptr),
        m_default_depth_buffer(nullptr),
        m_default_buffer_width(0),
        m_default_buffer_height(0),
        m_clear_color_red(1),
        m_clear_color_green(1),
        m_clear_color_blue(1),
        m_clear_color_alpha(1),
        m_clear_depth(1)
    {
    }

    ~GLContext()
    {
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
}

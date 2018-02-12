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

#include "GLTexture2D.h"
#include "memory/Memory.h"
#include "math/Mathf.h"

using namespace Viry3D;

namespace sgl
{
    class GLTexture2DPrivate
    {
    public:
        GLTexture2DPrivate(GLTexture2D* p):
            m_p(p),
            m_data(nullptr)
        {
        }

        ~GLTexture2DPrivate()
        {
            Memory::SafeFree(m_data);
        }

        GLTexture2D* m_p;
        byte* m_data;
    };

    GLTexture2D::GLTexture2D(GLuint id):
        GLTexture(id),
        m_width(0),
        m_height(0),
        m_internalformat(0),
        m_format(0),
        m_type(0)
    {
        m_private = new GLTexture2DPrivate(this);
    }

    GLTexture2D::~GLTexture2D()
    {
        delete m_private;
    }

    void GLTexture2D::TexImage2D(GLint level, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
    {
        if (internalformat == GL_RGBA && format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        {
            m_width = width;
            m_height = height;
            m_internalformat = internalformat;
            m_format = format;
            m_type = type;

            int size = width * height * 4;
            m_private->m_data = Memory::Realloc(m_private->m_data, size);
            if (pixels)
            {
                Memory::Copy(m_private->m_data, pixels, size);
            }
        }
    }

    Vector4 GLTexture2D::Sample(const Vector2& uv) const
    {
        int x = (int) ((m_width - 1) * Mathf::Clamp01(uv.x));
        int y = (int) ((m_height - 1) * Mathf::Clamp01(uv.y));

        int r = m_private->m_data[y * m_width * 4 + x * 4 + 0];
        int g = m_private->m_data[y * m_width * 4 + x * 4 + 1];
        int b = m_private->m_data[y * m_width * 4 + x * 4 + 2];
        int a = m_private->m_data[y * m_width * 4 + x * 4 + 3];

        return Vector4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }
}

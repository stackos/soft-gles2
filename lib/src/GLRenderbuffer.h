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

#pragma once

#include "GLObject.h"

namespace sgl
{
    class GLRenderbuffer: public GLObject
    {
    public:
        GLRenderbuffer():
            m_width(0),
            m_height(0),
            m_internal_format(0),
            m_buffer_size(0),
            m_buffer(nullptr)
        {
        }

        virtual ~GLRenderbuffer()
        {
            SafeFree(m_buffer);
        }

        void Storage(GLenum internalformat, GLsizei width, GLsizei height)
        {
            int size = 0;
            GLenum format;

            switch (internalformat)
            {
                case GL_RGBA4:
                case GL_RGB565:
                case GL_RGB5_A1:
                    size = width * height * 4;
                    format = GL_RGBA;
                    break;
                case GL_DEPTH_COMPONENT16:
                    size = width * height * 4;
                    format = GL_DEPTH_COMPONENT32_OES;
                    break;
                case GL_STENCIL_INDEX8:
                    size = width * height;
                    format = GL_STENCIL_INDEX8;
                    break;
                default:
                    break;
            }

            if (m_width != width || m_height != height || m_buffer_size != size)
            {
                m_width = width;
                m_height = height;
                m_internal_format = format;
                m_buffer_size = size;
                m_buffer = (char*) realloc(m_buffer, size);
            }
        }

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        GLenum GetInternalFormat() const { return m_internal_format; }
        int GetRedComponentSize() const { return 8; }
        int GetGreenComponentSize() const { return 8; }
        int GetBlueComponentSize() const { return 8; }
        int GetAlphaComponentSize() const { return 8; }
        int GetDepthSize() const { return 32; }
        int GetStencilSize() const { return 8; }

    private:
        int m_width;
        int m_height;
        GLenum m_internal_format;
        int m_buffer_size;
        char* m_buffer;
    };
}

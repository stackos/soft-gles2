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

#include "GLTexture.h"
#include "math/Vector2.h"
#include "math/Vector4.h"

namespace sgl
{
    class GLTexture2DPrivate;
    class GLTexture2D: public GLTexture
    {
    public:
        GLTexture2D(GLuint id);
        virtual ~GLTexture2D();

        void TexImage2D(GLint level, GLint internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        Viry3D::Vector4 Sample(const Viry3D::Vector2& uv) const;

    private:
        friend class GLTexture2DPrivate;
        GLTexture2DPrivate* m_private;
        int m_width;
        int m_height;
        GLint m_internalformat;
        GLenum m_format;
        GLenum m_type;
    };
}

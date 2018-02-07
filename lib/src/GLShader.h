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
#include "string/String.h"
#include "memory/ByteBuffer.h"
#include "container/Map.h"

namespace sgl
{
    class GLShaderPrivate;
    class GLShader: public GLObject
    {
    public:
        GLShader(GLuint id);
        virtual ~GLShader();

        GLenum GetType() const { return m_type; }

        void SetType(GLenum type)
        {
            m_type = type;
        }

        void SetSource(GLsizei count, const GLchar* const* string, const GLint* length);
        void GetSource(GLsizei bufSize, GLsizei* length, GLchar* source) const;
        void Compile();
        Viry3D::ByteBuffer GetBinary() const;

    private:
        const Viry3D::Vector<Viry3D::String>& GetVertexAttribs() const;
        const Viry3D::Vector<Viry3D::String>& GetUniforms() const;
        Viry3D::Vector<Viry3D::String> GetVaryingNames() const;
        Viry3D::Vector<Viry3D::String> GetVaryingTypes() const;

    private:
        friend class GLShaderPrivate;
        friend class GLProgram;
        friend class GLProgramPrivate;
        GLShaderPrivate* m_private;
        GLenum m_type;
        Viry3D::String m_source;
    };
}

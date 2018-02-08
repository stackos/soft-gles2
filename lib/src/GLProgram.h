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
#include "memory/Ref.h"
#include "container/Vector.h"
#include "string/String.h"
#include "math/Vector4.h"

namespace sgl
{
    class GLShader;

    class GLProgramPrivate;
    class GLProgram: public GLObject
    {
    public:
        typedef void*(*VarGetter)();
        typedef void(*VarSetter)(void*, int);
        typedef void(*Main)();

        enum class VaryingType
        {
            None,

            Vec2,
            Vec3,
            Vec4,
        };

        struct Varying
        {
            Viry3D::String name;
            VaryingType type;
            int size;
            Viry3D::Vector4 value;
            VarGetter getter;
            VarSetter setter;
            
            Varying(const Viry3D::String& name):
                name(name),
                type(VaryingType::None),
                size(0),
                getter(nullptr),
                setter(nullptr)
            {
            }
        };

        GLProgram(GLuint id);
        virtual ~GLProgram();

        void AttachShader(const Ref<GLShader>& shader);
        void DetachShader(GLuint shader);
        void GetAttachedShaders(GLsizei maxCount, GLsizei* count, GLuint* shaders) const;
        void BindAttribLocation(GLuint index, const GLchar* name);
        void Link();
        GLint GetAttribLocation(const GLchar* name) const;
        GLint GetUniformLocation(const GLchar* name) const;
        void Use();
        void Uniformv(GLint location, int size, const void* value) const;
        void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) const;
        void SetVertexAttrib(GLuint index, const void* data, int size) const;
        void* CallVSMain() const;
        Viry3D::Vector<Varying> GetVSVaryings() const;
        void SetFSVarying(const Viry3D::String& name, const void* data, int size) const;
        void* CallFSMain() const;

    private:
        friend class GLProgramPrivate;
        GLProgramPrivate* m_private;
    };
}

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

namespace sgl
{
    class GLShader;

    class GLProgramPrivate;
    class GLProgram: public GLObject
    {
    public:
        GLProgram(GLuint id);
        virtual ~GLProgram();

        void AttachShader(const Ref<GLShader>& shader);
        void DetachShader(GLuint shader);
        void GetAttachedShaders(GLsizei maxCount, GLsizei* count, GLuint* shaders);
        void Link();

    private:
        friend class GLProgramPrivate;
        GLProgramPrivate* m_private;
    };
}

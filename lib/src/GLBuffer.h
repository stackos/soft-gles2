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
    class GLBufferPrivate;
    class GLBuffer: public GLObject
    {
    public:
        GLBuffer(GLuint id);
        virtual ~GLBuffer();

        void BufferData(GLsizeiptr size, const void* data, GLenum usage);
        void BufferSubData(GLintptr offset, GLsizeiptr size, const void* data);
        void* GetData() const;

    private:
        friend class GLBufferPrivate;
        GLBufferPrivate* m_private;
    };
}

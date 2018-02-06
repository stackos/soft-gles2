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

#include "GLBuffer.h"
#include "memory/Memory.h"

using namespace Viry3D;

namespace sgl
{
    class GLBufferPrivate
    {
    public:
        GLBufferPrivate(GLBuffer* p):
            m_p(p),
            m_data(nullptr),
            m_data_size(0)
        {
        }

        ~GLBufferPrivate()
        {
            Memory::SafeFree(m_data);
        }

        GLBuffer* m_p;
        GLbyte* m_data;
        int m_data_size;
    };

    GLBuffer::GLBuffer(GLuint id):
        GLObject(id)
    {
        m_private = new GLBufferPrivate(this);
    }

    GLBuffer::~GLBuffer()
    {
        delete m_private;
    }

    void GLBuffer::BufferData(GLsizeiptr size, const void* data, GLenum usage)
    {
        if (size < 0)
        {
            return;
        }

        if (m_private->m_data == nullptr || m_private->m_data_size != size)
        {
            m_private->m_data_size = size;

            if (size == 0)
            {
                Memory::SafeFree(m_private->m_data);
            }
            else
            {
                m_private->m_data = Memory::Realloc(m_private->m_data, size);
            }
        }

        if (data && size > 0)
        {
            Memory::Copy(m_private->m_data, data, size);
        }
    }

    void GLBuffer::BufferSubData(GLintptr offset, GLsizeiptr size, const void* data)
    {
        if (offset < 0 || size <= 0 || offset + size > m_private->m_data_size || m_private->m_data == nullptr || data == nullptr)
        {
            return;
        }

        Memory::Copy(&m_private->m_data[offset], data, size);
    }
}

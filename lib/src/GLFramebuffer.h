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
    class GLContext;

    class GLFramebuffer: public GLObject
    {
    public:
        enum class Attachment
        {
            None = -1,

            Color0,
            Depth,
            Stencil,

            Count
        };

        GLFramebuffer(GLuint id): GLObject(id) { }

        virtual ~GLFramebuffer() { }

        Attachment GetAttachment(GLenum attachment)
        {
            Attachment attach = Attachment::None;

            switch (attachment)
            {
                case GL_COLOR_ATTACHMENT0:
                    attach = Attachment::Color0;
                    break;
                case GL_DEPTH_ATTACHMENT:
                    attach = Attachment::Depth;
                    break;
                case GL_STENCIL_ATTACHMENT:
                    attach = Attachment::Stencil;
                    break;
                default:
                    break;
            }

            return attach;
        }

        void SetAttachment(Attachment attachment, const Ref<GLObject>& obj)
        {
            m_attachments[(int) attachment] = obj;
        }

        void GetAttachmentParameteriv(Attachment attachment, GLenum pname, GLint* params);
        GLenum CheckStatus();

    private:
        WeakRef<GLObject> m_attachments[(int) Attachment::Count];
    };
}

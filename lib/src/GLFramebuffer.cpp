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

#include "GLFramebuffer.h"
#include "GLRenderbuffer.h"
#include "GLTexture.h"

namespace sgl
{
    void GLFramebuffer::GetAttachmentParameteriv(Attachment attachment, GLenum pname, GLint* params)
    {
        Ref<GLObject> obj;

        if (m_attachments[(int) attachment].expired())
        {
            *params = GL_NONE;
            return;
        }
        else
        {
            obj = m_attachments[(int) attachment].lock();
        }

        switch (pname)
        {
            case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                if (RefCast<GLRenderbuffer>(obj))
                {
                    *params = GL_RENDERBUFFER;
                }
                else if (RefCast<GLTexture>(obj))
                {
                    *params = GL_TEXTURE;
                }
                else
                {
                    *params = GL_NONE;
                }
                break;
            case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                *params = (GLint) obj->GetId();
                break;
                //TODO: case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
                //TODO: case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
            default:
                break;
        }
    }

    GLenum GLFramebuffer::CheckStatus()
    {
        bool no_attached = true;
        bool all_attached = true;
        bool same_size = true;
        int w = -1;
        int h = -1;

        for (int i = 0; i < (int) Attachment::Count; ++i)
        {
            if (!m_attachments[i].expired())
            {
                no_attached = false;

                Ref<GLObject> obj = m_attachments[i].lock();
                if (RefCast<GLRenderbuffer>(obj))
                {
                    Ref<GLRenderbuffer> rbo = RefCast<GLRenderbuffer>(obj);
                    if (w != -1 && h != -1 && (w != rbo->GetWidth() || h != rbo->GetHeight()))
                    {
                        same_size = false;
                    }

                    w = rbo->GetWidth();
                    h = rbo->GetHeight();
                }
                else if (RefCast<GLTexture>(obj))
                {
                    Ref<GLTexture> tex = RefCast<GLTexture>(obj);
                    if (w != -1 && h != -1 && (w != tex->GetWidth() || h != tex->GetHeight()))
                    {
                        same_size = false;
                    }

                    w = tex->GetWidth();
                    h = tex->GetHeight();
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                all_attached = false;
            }
        }

        if (no_attached)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
        }
        else
        {
            if (all_attached)
            {
                if (same_size)
                {
                    if (w == 0 || h == 0)
                    {
                        return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                    }
                    else
                    {
                        return GL_FRAMEBUFFER_COMPLETE;
                    }
                }
                else
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
                }
            }
            else
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        return 0;
    }
}

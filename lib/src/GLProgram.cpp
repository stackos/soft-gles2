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

#include "GLProgram.h"
#include "GLShader.h"
#include "exec_cmd.h"
#include "io/File.h"
#include "string/String.h"
#include "container/Map.h"
#include "Debug.h"

using namespace Viry3D;

namespace sgl
{
    class GLProgramPrivate
    {
    public:
        struct Uniform
        {
            String name;
            int location;

            Uniform(const String& name):
                name(name),
                location(-1)
            {
            }
        };

        GLProgramPrivate(GLProgram* p):
            m_p(p)
        {
        }

        ~GLProgramPrivate()
        {
            String dll_name = this->GetTempDllName();

            File::Delete(dll_name + ".dll");
            File::Delete(dll_name + ".exp");
            File::Delete(dll_name + ".lib");
        }

        String GetTempDllName()
        {
            return String::Format("temp.p.%d", m_p->GetId());
        }

        void BindUniformLocations()
        {
            Vector<String> uniforms_vs = m_shaders[0]->GetUniforms();
            Vector<String> uniforms_fs = m_shaders[1]->GetUniforms();
            Vector<String> uniforms;
            if (uniforms_vs.Size() > 0)
            {
                uniforms.AddRange(&uniforms_vs[0], uniforms_vs.Size());
            }
            if (uniforms_fs.Size() > 0)
            {
                uniforms.AddRange(&uniforms_fs[0], uniforms_fs.Size());
            }

            m_uniforms.Clear();
            for (int i = 0; i < uniforms.Size(); ++i)
            {
                bool exist = false;
                for (int j = 0; j < m_uniforms.Size(); ++j)
                {
                    if (m_uniforms[j].name == uniforms[i])
                    {
                        exist = true;
                        break;
                    }
                }

                if (exist == false)
                {
                    GLProgramPrivate::Uniform u(uniforms[i]);
                    u.location = m_uniforms.Size();

                    m_uniforms.Add(u);
                }
            }
        }

        GLProgram* m_p;
        Ref<GLShader> m_shaders[2];
        Map<String, GLuint> m_bind_attribs;
        Vector<Uniform> m_uniforms;
    };

    GLProgram::GLProgram(GLuint id):
        GLObject(id)
    {
        m_private = new GLProgramPrivate(this);
    }

    GLProgram::~GLProgram()
    {
        delete m_private;
    }

    void GLProgram::AttachShader(const Ref<GLShader>& shader)
    {
        GLenum type = shader->GetType();

        if (type == GL_VERTEX_SHADER)
        {
            if (!m_private->m_shaders[0])
            {
                m_private->m_shaders[0] = shader;
            }
        }
        else if (type == GL_FRAGMENT_SHADER)
        {
            if (!m_private->m_shaders[1])
            {
                m_private->m_shaders[1] = shader;
            }
        }
    }

    void GLProgram::DetachShader(GLuint shader)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (m_private->m_shaders[i])
            {
                if (m_private->m_shaders[i]->GetId() == shader)
                {
                    m_private->m_shaders[i].reset();
                    break;
                }
            }
        }
    }

    void GLProgram::GetAttachedShaders(GLsizei maxCount, GLsizei* count, GLuint* shaders) const
    {
        int index = 0;

        for (int i = 0; i < 2; ++i)
        {
            if (m_private->m_shaders[i])
            {
                if (index < maxCount)
                {
                    shaders[index++] = m_private->m_shaders[i]->GetId();
                }
                else
                {
                    break;
                }
            }
        }

        *count = index;
    }

    void GLProgram::BindAttribLocation(GLuint index, const GLchar* name)
    {
        if (m_private->m_bind_attribs.Contains(name))
        {
            m_private->m_bind_attribs[name] = index;
        }
        else
        {
            m_private->m_bind_attribs.Add(name, index);
        }
    }

    void GLProgram::Link()
    {
        if (!m_private->m_shaders[0] || !m_private->m_shaders[1])
        {
            return;
        }

        const String vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community";
        //const String vs_path = "D:\\Program\\VS2017";
        const bool isX64 = sizeof(void*) == 8;
        const String host = "Hostx64"; // "Hostx86"
        String cl_dir;

        if (isX64)
        {
            cl_dir = vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\" + host + "\\x64";
        }
        else
        {
            cl_dir = vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\" + host + "\\x86";
        }

        String temp_vs_obj_name = "temp.vs.obj";
        String temp_fs_obj_name = "temp.fs.obj";
        String dll_name = m_private->GetTempDllName();
        String temp_out_name = dll_name + ".out.txt";

        ByteBuffer vs_bin = m_private->m_shaders[0]->GetBinary();
        ByteBuffer fs_bin = m_private->m_shaders[1]->GetBinary();

        if (vs_bin.Size() == 0 || fs_bin.Size() == 0)
        {
            return;
        }

        File::WriteAllBytes(temp_vs_obj_name, vs_bin);
        File::WriteAllBytes(temp_fs_obj_name, fs_bin);

        exec_cmd(cl_dir, "link.exe", "/dll " + temp_vs_obj_name + " " + temp_fs_obj_name + " /OUT:" + dll_name + ".dll "
            "/LIBPATH:\"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\lib\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\um\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\ucrt\\x64\"",
            temp_out_name);

        String out_text = File::ReadAllText(temp_out_name);

        File::Delete(temp_vs_obj_name);
        File::Delete(temp_fs_obj_name);
        File::Delete(temp_out_name);

        m_private->m_shaders[0]->BindAttribLocations(m_private->m_bind_attribs);
        m_private->BindUniformLocations();

        Log("Link info:\n%sgen dll:%s.dll", out_text.CString(), dll_name.CString());
    }

    GLint GLProgram::GetAttribLocation(const GLchar* name) const
    {
        if (m_private->m_shaders[0])
        {
            return m_private->m_shaders[0]->GetAttribLocation(name);
        }

        return -1;
    }

    GLint GLProgram::GetUniformLocation(const GLchar* name) const
    {
        for (int i = 0; i < m_private->m_uniforms.Size(); ++i)
        {
            const auto& u = m_private->m_uniforms[i];
            if (u.name == name)
            {
                return u.location;
            }
        }

        return -1;
    }
}

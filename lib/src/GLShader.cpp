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

#include "GLShader.h"
#include "exec_cmd.h"
#include "Debug.h"
#include "memory/Memory.h"
#include "io/File.h"

using namespace Viry3D;

namespace sgl
{
    class GLShaderPrivate
    {
    public:
        struct Varying
        {
            String name;
            String type;

            Varying(const String& name, const String& type):
                name(name),
                type(type)
            {
            }
        };

        GLShaderPrivate(GLShader* p):
            m_p(p)
        {
        }

        // https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.10.pdf
        void ParseSource(String& temp_file)
        {
            String src = m_p->m_source;
            src = src.Replace("\t", " ").Replace("\r", " ").Replace("\n", " ");
            Vector<String> sentences = src.Split(";", true);

            Vector<String> builtins;
            m_uniforms.Clear();
            m_attributes.Clear();
            m_varyings.Clear();

            for (int i = 0; i < sentences.Size(); ++i)
            {
                const String& s = sentences[i];
                Vector<String> words = s.Split(" ", true);

                if (words[0] == "uniform")
                {
                    m_uniforms.Add(words[2]);
                }
                else if (words[0] == "varying")
                {
                    m_varyings.Add(Varying(words[2], words[1]));
                }
                else
                {
                    if (m_p->m_type == GL_VERTEX_SHADER)
                    {
                        if (words[0] == "attribute")
                        {
                            m_attributes.Add(words[2]);
                        }
                        else if (s.Contains(" main("))
                        {
                            sentences[i] = "DLL_EXPORT " + s.Replace(" main(", " vs_main(");
                        }
                    }
                    else if (m_p->m_type == GL_FRAGMENT_SHADER)
                    {
                        if (s.Contains(" main("))
                        {
                            sentences[i] = "DLL_EXPORT " + s.Replace(" main(", " fs_main(");
                        }
                    }
                }
            }

            if (m_p->m_type == GL_VERTEX_SHADER)
            {
                builtins.Add("gl_Position");

                temp_file = "temp.vs";
                src = File::ReadAllText("Assets/shader/vs_include.txt") + "\n";
            }
            else if (m_p->m_type == GL_FRAGMENT_SHADER)
            {
                builtins.Add("gl_FragColor");

                temp_file = "temp.fs";
                src = File::ReadAllText("Assets/shader/fs_include.txt") + "\n";
            }

            for (int i = 0; i < sentences.Size(); ++i)
            {
                src += String::Format("%s;\n", sentences[i].CString());
            }
            src += "\n";

            for (int i = 0; i < m_uniforms.Size(); ++i)
            {
                src += String::Format("VAR_SETTER(%s)\n", m_uniforms[i].CString());
            }

            if (m_p->m_type == GL_VERTEX_SHADER)
            {
                for (int i = 0; i < m_attributes.Size(); ++i)
                {
                    src += String::Format("VAR_SETTER(%s)\n", m_attributes[i].CString());
                }

                for (int i = 0; i < m_varyings.Size(); ++i)
                {
                    src += String::Format("VAR_GETTER(%s)\n", m_varyings[i].name.CString());
                }
            }
            else if (m_p->m_type == GL_FRAGMENT_SHADER)
            {
                for (int i = 0; i < m_varyings.Size(); ++i)
                {
                    src += String::Format("VAR_SETTER(%s)\n", m_varyings[i].name.CString());
                }
            }

            for (int i = 0; i < builtins.Size(); ++i)
            {
                src += String::Format("VAR_GETTER(%s)\n", builtins[i].CString());
            }

            File::WriteAllText(temp_file + ".cpp", src);
        }

        GLShader* m_p;
        Vector<String> m_uniforms;
        Vector<String> m_attributes;
        Vector<Varying> m_varyings;
        ByteBuffer m_obj_bin;
    };

    GLShader::GLShader(GLuint id):
        GLObject(id)
    {
        m_private = new GLShaderPrivate(this);
    }

    GLShader::~GLShader()
    {
        delete m_private;
    }

    void GLShader::SetSource(GLsizei count, const GLchar* const* string, const GLint* length)
    {
        m_source = "";

        for (int i = 0; i < count; ++i)
        {
            if (length != nullptr && length[i] > 0)
            {
                m_source += String(string[i], length[i]);
            }
            else
            {
                m_source += String(string[i]);
            }
        }
    }

    void GLShader::GetSource(GLsizei bufSize, GLsizei* length, GLchar* source) const
    {
        if (bufSize > 0)
        {
            int size = m_source.Size();

            if (size > bufSize - 1)
            {
                size = bufSize - 1;
            }

            if (source != nullptr)
            {
                if (size > 0)
                {
                    Memory::Copy(source, m_source.CString(), size);
                }
                source[size] = 0;

                if (length != nullptr)
                {
                    *length = size;
                }
            }
        }
    }

    void GLShader::Compile()
    {
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

        String temp_src_name;
        m_private->ParseSource(temp_src_name);
        String temp_out_name = temp_src_name + ".out.txt";

        exec_cmd(cl_dir, "cl.exe", "/c " + temp_src_name + ".cpp "
            "/I \"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\include\" "
            "/I \"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.16299.0\\ucrt\"",
            temp_out_name);

        String out_text = File::ReadAllText(temp_out_name);

        m_private->m_obj_bin = File::ReadAllBytes(temp_src_name + ".obj");

        File::Delete(temp_src_name + ".cpp");
        File::Delete(temp_src_name + ".obj");
        File::Delete(temp_out_name);

        Log("Compile info:\n%sgen obj size:%d", out_text.CString(), m_private->m_obj_bin.Size());
    }

    ByteBuffer GLShader::GetBinary() const
    {
        return m_private->m_obj_bin;
    }

    const Vector<String>& GLShader::GetVertexAttribs() const
    {
        return m_private->m_attributes;
    }

    const Vector<String>& GLShader::GetUniforms() const
    {
        return m_private->m_uniforms;
    }

    Vector<String> GLShader::GetVaryingNames() const
    {
        Vector<String> names;
        for (const auto& i : m_private->m_varyings)
        {
            names.Add(i.name);
        }
        return names;
    }

    Vector<String> GLShader::GetVaryingTypes() const
    {
        Vector<String> types;
        for (const auto& i : m_private->m_varyings)
        {
            types.Add(i.type);
        }
        return types;
    }
}

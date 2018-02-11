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
#include "GLTexture2D.h"
#include "exec_cmd.h"
#include "io/File.h"
#include "string/String.h"
#include "container/Map.h"
#include "math/Mathf.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include "memory/Memory.h"
#include "Debug.h"
#include <Windows.h>

using namespace Viry3D;

extern const char* g_vs_path;

namespace sgl
{
    class GLProgramPrivate
    {
    public:
        struct Attribute
        {
            String name;
            int location;
            GLProgram::VarSetter setter;

            Attribute(const String& name):
                name(name),
                location(-1),
                setter(nullptr)
            {
            }
        };

        struct Uniform
        {
            String name;
            String type;
            int location;
            GLProgram::VarSetter setter;

            Uniform(const String& name):
                name(name),
                location(-1),
                setter(nullptr)
            {
            }
        };

        struct Sampler2D
        {
            typedef Vector4(*Sample)(GLTexture2D*, const Vector2*);
            GLTexture2D* texture;
            Sample sample_func = Sampler2D::SampleTexture;

            static Vector4 SampleTexture(GLTexture2D* tex, const Vector2* uv)
            {
                return tex->Sample(*uv);
            }
        };

        GLProgramPrivate(GLProgram* p):
            m_p(p),
            m_dll(nullptr),
            m_vs_main(nullptr),
            m_get_gl_Position(nullptr),
            m_set_gl_FragCoord(nullptr),
            m_fs_main(nullptr),
            m_get_gl_FragColor(nullptr)
        {
        }

        ~GLProgramPrivate()
        {
            if (m_dll)
            {
                FreeLibrary(m_dll);
                m_dll = nullptr;
            }

            String dll_name = this->GetTempDllName();
            File::Delete(dll_name + ".dll");
            File::Delete(dll_name + ".exp");
            File::Delete(dll_name + ".lib");
        }

        String GetTempDllName()
        {
            return String::Format("temp.p.%d", m_p->GetId());
        }

        void BindAttribLocations()
        {
            Vector<String> attribs = m_shaders[0]->GetVertexAttribs();
            m_attribs.Clear();
            for (const String& i : attribs)
            {
                m_attribs.Add(Attribute(i));
            }

            int max_index = -1;

            for (const auto& i : m_bind_attribs)
            {
                for (auto& j : m_attribs)
                {
                    if (i.first == j.name)
                    {
                        j.location = i.second;

                        max_index = Mathf::Max(max_index, j.location);
                        break;
                    }
                }
            }

            for (auto& i : m_attribs)
            {
                if (i.location < 0)
                {
                    i.location = ++max_index;
                }
            }
        }

        void BindUniformLocations()
        {
            Vector<String> uniforms_vs = m_shaders[0]->GetUniformNames();
            Vector<String> uniforms_fs = m_shaders[1]->GetUniformNames();
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

            Vector<String> uniforms_fs_types = m_shaders[1]->GetUniformTypes();
            for (int i = 0; i < uniforms_fs_types.Size(); ++i)
            {
                if (uniforms_fs_types[i] == "sampler2D")
                {
                    for (int j = 0; j < m_uniforms.Size(); ++j)
                    {
                        if (m_uniforms[j].name == uniforms_fs[i])
                        {
                            m_uniforms[j].type = uniforms_fs_types[i];
                            break;
                        }
                    }
                }
            }
        }

        GLProgram* m_p;
        Ref<GLShader> m_shaders[2];
        Map<String, GLuint> m_bind_attribs;
        Vector<Attribute> m_attribs;
        Vector<Uniform> m_uniforms;
        Vector<GLProgram::Varying> m_vs_varyings;
        Vector<GLProgram::Varying> m_fs_varyings;
        HMODULE m_dll;
        GLProgram::Main m_vs_main;
        GLProgram::VarGetter m_get_gl_Position;
        GLProgram::VarSetter m_set_gl_FragCoord;
        GLProgram::Main m_fs_main;
        GLProgram::VarGetter m_get_gl_FragColor;
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

        const bool isX64 = sizeof(void*) == 8;
        const String host = "Hostx64"; // "Hostx86"
        String cl_dir;

        if (isX64)
        {
            cl_dir = String(g_vs_path) + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\" + host + "\\x64";
        }
        else
        {
            cl_dir = String(g_vs_path) + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\" + host + "\\x86";
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
            "/LIBPATH:\"" + g_vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\lib\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\um\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\ucrt\\x64\"",
            temp_out_name);

        String out_text = File::ReadAllText(temp_out_name);

        File::Delete(temp_vs_obj_name);
        File::Delete(temp_fs_obj_name);
        File::Delete(temp_out_name);

        m_private->BindAttribLocations();
        m_private->BindUniformLocations();

        Log("Link info:\n%sgen dll:%s.dll", out_text.CString(), dll_name.CString());
    }

    GLint GLProgram::GetAttribLocation(const GLchar* name) const
    {
        for (auto& i : m_private->m_attribs)
        {
            if (i.name == name)
            {
                return i.location;
            }
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

    void GLProgram::Use()
    {
        if (m_private->m_dll == nullptr)
        {
            HMODULE dll = LoadLibrary((m_private->GetTempDllName() + ".dll").CString());
            m_private->m_dll = dll;

            assert(dll != nullptr);

            for (auto& i : m_private->m_attribs)
            {
                String func_name = "set_" + i.name;
                i.setter = (VarSetter) GetProcAddress(dll, func_name.CString());
            }

            for (auto& i : m_private->m_uniforms)
            {
                String func_name = "set_" + i.name;
                i.setter = (VarSetter) GetProcAddress(dll, func_name.CString());
            }

            m_private->m_vs_main = (Main) GetProcAddress(dll, "vs_main");
            m_private->m_get_gl_Position = (VarGetter) GetProcAddress(dll, "get_gl_Position");
            
            m_private->m_set_gl_FragCoord = (VarSetter) GetProcAddress(dll, "set_gl_FragCoord");
            m_private->m_fs_main = (Main) GetProcAddress(dll, "fs_main");
            m_private->m_get_gl_FragColor = (VarGetter) GetProcAddress(dll, "get_gl_FragColor");

            m_private->m_vs_varyings.Clear();
            Vector<String> varying_names = m_private->m_shaders[0]->GetVaryingNames();
            Vector<String> varying_types = m_private->m_shaders[0]->GetVaryingTypes();
            for (int i = 0; i < varying_names.Size(); ++i)
            {
                Varying v(varying_names[i]);
                if (varying_types[i] == "vec2")
                {
                    v.type = VaryingType::Vec2;
                    v.size = sizeof(float) * 2;
                }
                else if (varying_types[i] == "vec3")
                {
                    v.type = VaryingType::Vec3;
                    v.size = sizeof(float) * 3;
                }
                else if (varying_types[i] == "vec4")
                {
                    v.type = VaryingType::Vec4;
                    v.size = sizeof(float) * 4;
                }
                else
                {
                    assert(!"not implement varying type");
                }
                String func_name = "get_" + varying_names[i];
                v.getter = (VarGetter) GetProcAddress(dll, func_name.CString());
                m_private->m_vs_varyings.Add(v);
            }

            m_private->m_fs_varyings.Clear();
            varying_names = m_private->m_shaders[1]->GetVaryingNames();
            varying_types = m_private->m_shaders[1]->GetVaryingTypes();
            for (int i = 0; i < varying_names.Size(); ++i)
            {
                Varying v(varying_names[i]);
                if (varying_types[i] == "vec2")
                {
                    v.type = VaryingType::Vec2;
                    v.size = sizeof(float) * 2;
                }
                else if (varying_types[i] == "vec3")
                {
                    v.type = VaryingType::Vec3;
                    v.size = sizeof(float) * 3;
                }
                else if (varying_types[i] == "vec4")
                {
                    v.type = VaryingType::Vec4;
                    v.size = sizeof(float) * 4;
                }
                else
                {
                    assert(!"not implement varying type");
                }
                String func_name = "set_" + varying_names[i];
                v.setter = (VarSetter) GetProcAddress(dll, func_name.CString());
                m_private->m_fs_varyings.Add(v);
            }
        }
    }

    bool GLProgram::IsUniformSampler2D(GLint location) const
    {
        for (const auto& i : m_private->m_uniforms)
        {
            if (i.location == location)
            {
                if (i.type == "sampler2D")
                {
                    return true;
                }
            }
        }

        return false;
    }

    void GLProgram::UniformSampler2D(GLint location, const Ref<GLTexture2D>& texture) const
    {
        for (const auto& i : m_private->m_uniforms)
        {
            if (i.location == location)
            {
                GLProgramPrivate::Sampler2D sampler;
                sampler.texture = texture.get();
                i.setter((void*) &sampler, sizeof(GLProgramPrivate::Sampler2D));
                break;
            }
        }
    }

    void GLProgram::Uniformv(GLint location, int size, const void* value) const
    {
        for (const auto& i : m_private->m_uniforms)
        {
            if (i.location == location)
            {
                i.setter((void*) value, size);
                break;
            }
        }
    }

    void GLProgram::UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) const
    {
        for (const auto& i : m_private->m_uniforms)
        {
            if (i.location == location)
            {
                Matrix4x4* p = (Matrix4x4*) value;
                Vector<Matrix4x4> mats;
                for (int i = 0; i < count; ++i)
                {
                    Matrix4x4 m = p[i];
                    if (transpose == GL_TRUE)
                    {
                        m = m.Transpose();
                    }

                    mats.Add(m);
                }
                i.setter((void*) mats.Bytes(), mats.SizeInBytes());
                break;
            }
        }
    }

    void GLProgram::SetVertexAttrib(GLuint index, const void* data, int size) const
    {
        for (const auto& i : m_private->m_attribs)
        {
            if (i.location == index)
            {
                i.setter((void*) data, size);
                break;
            }
        }
    }

    void* GLProgram::CallVSMain() const
    {
        m_private->m_vs_main();
        return m_private->m_get_gl_Position();
    }

    Vector<GLProgram::Varying> GLProgram::GetVSVaryings() const
    {
        Vector<GLProgram::Varying> varyings;

        for (const auto& i : m_private->m_vs_varyings)
        {
            GLProgram::Varying v(i.name);
            v.size = i.size;
            v.type = i.type;
            Memory::Copy(&v.value, i.getter(), v.size);

            varyings.Add(v);
        }

        return varyings;
    }

    void GLProgram::SetFSVarying(const String& name, const void* data, int size) const
    {
        for (const auto& i : m_private->m_fs_varyings)
        {
            if (i.name == name)
            {
                i.setter((void*) data, size);
                break;
            }
        }
    }

    void* GLProgram::CallFSMain(const Vector4& frag_coord) const
    {
        m_private->m_set_gl_FragCoord((void*) &frag_coord, sizeof(Vector4));
        m_private->m_fs_main();
        return m_private->m_get_gl_FragColor();
    }
}

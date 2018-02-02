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
#include "Debug.h"
#include "memory/Memory.h"
#include "io/File.h"
#include <Windows.h>

using namespace Viry3D;

namespace sgl
{
    class GLShaderPrivate
    {
    public:
        GLShaderPrivate(GLShader* p):
            p(p)
        {
        }

        void ExecCmd(const String& path, const String& exe, const String& param, const String& output)
        {
            File::WriteAllText(output, "");

            SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
            HANDLE hOutput = CreateFile(output.CString(),
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            STARTUPINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.wShowWindow = SW_HIDE;
            si.hStdOutput = hOutput;

            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(pi));

            String cmd = "\"" + path + "\\" + exe + "\" " + param;

            if (CreateProcess(
                NULL,
                (LPSTR) cmd.CString(),    // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                TRUE,          // Set handle inheritance to FALSE
                0,              // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory 
                &si,            // Pointer to STARTUPINFO structure
                &pi))
            {
                // Wait until child process exits.
                WaitForSingleObject(pi.hProcess, INFINITE);

                // Close process and thread handles. 
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            CloseHandle(hOutput);
        }

        String GenTempSource()
        {
            String temp_file;
            String src = p->m_source;
            src = src.Replace("\t", " ").Replace("\r", " ").Replace("\n", " ");
            Vector<String> sentences = src.Split(";", true);
            Vector<String> gets;
            Vector<String> sets;

            for (int i = 0; i < sentences.Size(); ++i)
            {
                const String& s = sentences[i];
                Vector<String> words = s.Split(" ", true);

                if (words[0] == "uniform")
                {
                    sets.Add(words[2]);
                }
                else if (words[0] == "varying")
                {
                    if (p->m_type == GL_VERTEX_SHADER)
                    {
                        gets.Add(words[2]);
                    }
                    else if (p->m_type == GL_FRAGMENT_SHADER)
                    {
                        sets.Add(words[2]);
                    }
                }
                else
                {
                    if (p->m_type == GL_VERTEX_SHADER)
                    {
                        if (words[0] == "attribute")
                        {
                            sets.Add(words[2]);
                        }
                        else if (s.Contains(" main("))
                        {
                            sentences[i] = "DLL_EXPORT " + s.Replace(" main(", " vs_main(");
                        }
                    }
                    else if (p->m_type == GL_FRAGMENT_SHADER)
                    {
                        if (s.Contains(" main("))
                        {
                            sentences[i] = "DLL_EXPORT " + s.Replace(" main(", " fs_main(");
                        }
                    }
                }
            }

            if (p->m_type == GL_VERTEX_SHADER)
            {
                gets.Add("gl_Position");

                temp_file = "temp.vs.cpp";
                src = File::ReadAllText("Assets/shader/vs_include.txt");
            }
            else if (p->m_type == GL_FRAGMENT_SHADER)
            {
                gets.Add("gl_FragColor");

                temp_file = "temp.fs.cpp";
                src = File::ReadAllText("Assets/shader/fs_include.txt");
            }

            for (int i = 0; i < sentences.Size(); ++i)
            {
                src += String::Format("%s;\n", sentences[i].CString());
            }

            for (int i = 0; i < sets.Size(); ++i)
            {
                src += String::Format("VAR_SETTER(%s)\n", sets[i].CString());
            }

            for (int i = 0; i < gets.Size(); ++i)
            {
                src += String::Format("VAR_GETTER(%s)\n", gets[i].CString());
            }

            File::WriteAllText(temp_file, src);

            return temp_file;
        }

        GLShader* p;
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

    void GLShader::GetSource(GLsizei bufSize, GLsizei* length, GLchar* source)
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

        String temp_src_name = m_private->GenTempSource();
        String temp_out_name = temp_src_name + ".out.txt";

        m_private->ExecCmd(cl_dir, "cl.exe", "/c " + temp_src_name + " "
            "/I \"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\include\" "
            "/I \"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.16299.0\\ucrt\"",
            temp_out_name);

        String out_text = File::ReadAllText(temp_out_name);
        if (out_text.Size() > 0)
        {
            Log(String::Format("Compile info:\n%s", out_text.CString()).CString());
        }

        DeleteFile(temp_src_name.CString());
        DeleteFile(temp_out_name.CString());
    }
}

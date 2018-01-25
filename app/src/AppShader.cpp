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

#include <Windows.h>
#include "display/DisplayWindows.h"
#include "GLES2/gl2.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"
#include <string>

using namespace Viry3D;

void exec_cmd(const std::string& path, const std::string& exe, const std::string& param)
{
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "\"" + path + "\\" + exe + "\" " + param;

    if (CreateProcess(
        NULL,
        (LPSTR) cmd.c_str(),    // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
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
}

class Renderer
{
public:
    Renderer()
    {
        //std::string vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community";
        std::string vs_path = "D:\\Program\\VS2017";

        std::string cl_dir = vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\Hostx64\\x64";
        exec_cmd(cl_dir, "cl.exe", "/c test.vs.cpp test.ps.cpp "
            "/I \"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\include\" "
            "/I \"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.16299.0\\ucrt\"");
        exec_cmd(cl_dir, "link.exe", "/dll test.vs.obj test.ps.obj /OUT:test.dll "
            "/LIBPATH:\"" + vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\lib\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\um\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\ucrt\\x64\"");

        auto dll = LoadLibrary("test.dll");
        if (dll)
        {
            struct Vertex
            {
                Vector3 pos;
                Vector2 uv;
            };

            Matrix4x4 view = Matrix4x4::LookTo(
                Vector3(0, 1, -2),
                Vector3(0, 0, 1),
                Vector3(0, 1, 0));
            Matrix4x4 proj = Matrix4x4::Perspective(60, 1280 / 720.0f, 0.3f, 1000);
            Matrix4x4 vp = proj * view;

            Vertex vertices[3] = {
                { Vector3(-1, 0, 2), Vector2(0, 0) },
                { Vector3(-1, 0, 0), Vector2(0, 1) },
                { Vector3(1, 0, 0), Vector2(1, 1) },
            };
            unsigned short indices[3] = { 0, 1, 2 };

            Vector4 gl_position[3];
            Vector2 v_uv[3];

            typedef void*(*VarGetter)();
            typedef void(*VarSetter)(void*, int);
            typedef void(*Main)();

            VarSetter set_a_position = (VarSetter) GetProcAddress(dll, "set_a_position");
            VarSetter set_a_uv = (VarSetter) GetProcAddress(dll, "set_a_uv");
            Main vs_main = (Main) GetProcAddress(dll, "vs_main");
            VarGetter get_gl_Position = (VarGetter) GetProcAddress(dll, "get_gl_Position");
            VarGetter get_v_uv = (VarGetter) GetProcAddress(dll, "get_v_uv");

            if (vs_main)
            {
                for (int i = 0; i < 3; ++i)
                {
                    unsigned short index = indices[i];

                    Vector4 pos = vp * Vector4(vertices[index].pos, 1);

                    set_a_position(&pos, sizeof(Vector4));
                    set_a_uv(&vertices[index].uv, sizeof(Vector2));

                    vs_main();

                    gl_position[i] = *(Vector4*) get_gl_Position();
                    v_uv[i] = *(Vector2*) get_v_uv();
                }
            }

            VarSetter set_u_tex = (VarSetter) GetProcAddress(dll, "set_u_tex");
            VarSetter set_v_uv = (VarSetter) GetProcAddress(dll, "set_v_uv");
            Main ps_main = (Main) GetProcAddress(dll, "ps_main");
            VarGetter get_gl_FragColor = (VarGetter) GetProcAddress(dll, "get_gl_FragColor");

            if (ps_main)
            {

            }

            FreeLibrary(dll);
        }
        //
        //to do: glCreateShader
    }

    virtual ~Renderer()
    {
    }

    void Draw()
    {
    }
};

#if 1
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    DisplayWindows* display = new DisplayWindows("soft-gles2", 1280, 720);
    Renderer* renderer = new Renderer();

    while(display->ProcessSystemEvents())
    {
        renderer->Draw();
        display->SwapBuffers();
    }

    delete renderer;
    delete display;

    return 0;
}
#endif

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
#include <string>

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

struct vec4
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };

        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
    };

    vec4(float x = 0, float y = 0, float z = 0, float w = 0):
        x(x),
        y(y),
        z(z),
        w(w)
    {
    }
};

class Renderer
{
public:
    Renderer()
    {
        //std::string vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community";
        std::string vs_path = "D:\\Program\\VS2017";

        std::string cl_dir = vs_path + "\\VC\\Tools\\MSVC\\14.12.25827\\bin\\Hostx64\\x64";
        exec_cmd(cl_dir, "cl.exe", "/c test.cpp "
            "/I " + vs_path + "\"\\VC\\Tools\\MSVC\\14.12.25827\\include\" "
            "/I \"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.16299.0\\ucrt\"");
        exec_cmd(cl_dir, "link.exe", "/dll test.obj /OUT:test.dll "
            "/LIBPATH:" + vs_path + "\"\\VC\\Tools\\MSVC\\14.12.25827\\lib\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\um\\x64\" "
            "/LIBPATH:\"C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.16299.0\\ucrt\\x64\"");

        auto dll = LoadLibrary("test.dll");
        if (dll)
        {
            typedef void*(*VSVarGetter)();
            typedef void(*VSVarSetter)(void*);
            typedef void(*VSMain)();

            VSVarSetter set_uniform_color = (VSVarSetter) GetProcAddress(dll, "set_u_color");
            VSVarSetter set_attr_position = (VSVarSetter) GetProcAddress(dll, "set_a_position");
            VSVarSetter set_attr_color = (VSVarSetter) GetProcAddress(dll, "set_a_color");
            VSMain vs_main = (VSMain) GetProcAddress(dll, "main");
            VSVarGetter get_gl_position = (VSVarGetter) GetProcAddress(dll, "get_gl_Position");
            VSVarGetter get_varying_color = (VSVarGetter) GetProcAddress(dll, "get_v_color");

            vec4 gl_position;
            vec4 v_color;

            if (vs_main)
            {
                vec4 ucolor(2, 2, 2, 2);
                vec4 apos(0, 0, 0, 1);
                vec4 acolor(3, 3, 3, 3);

                set_uniform_color(&ucolor);
                set_attr_position(&apos);
                set_attr_color(&acolor);

                vs_main();

                gl_position = *(vec4*) get_gl_position();
                v_color = *(vec4*) get_varying_color();
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

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

#include "DisplayWindows.h"
#include "memory/Memory.h"
#include <windowsx.h>

using namespace Viry3D;

__declspec(dllimport) void create_gl_context();
__declspec(dllimport) void destroy_gl_context();
__declspec(dllimport) void set_gl_context_default_buffers(void* color_buffer, void* depth_buffer, int width, int height);

LRESULT CALLBACK win_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;

        default:
            break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DisplayWindows::DisplayWindows(const std::string& name, int width, int height):
    m_name(name),
    m_width(width),
    m_height(height),
    m_window(nullptr),
    m_hdc(nullptr),
    m_front_buffer(0),
    m_buffer_size(0),
    m_bmi_buffer(nullptr)
{
    Memory::Zero(m_color_buffers, sizeof(m_color_buffers));
    Memory::Zero(m_depth_buffers, sizeof(m_depth_buffers));

    this->CreateSystemWindow();
    this->CreateBuffers();

    create_gl_context();
    set_gl_context_default_buffers(m_color_buffers[m_front_buffer], m_depth_buffers[m_front_buffer], m_width, m_height);
}

DisplayWindows::~DisplayWindows()
{
    ReleaseDC(m_window, m_hdc);

    Memory::SafeFree(m_color_buffers[0]);
    Memory::SafeFree(m_color_buffers[1]);
    Memory::SafeFree(m_depth_buffers[0]);
    Memory::SafeFree(m_depth_buffers[1]);
    Memory::SafeFree(m_bmi_buffer);

    destroy_gl_context();
}

bool DisplayWindows::ProcessSystemEvents()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (WM_QUIT == msg.message)
        {
            return false;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return true;
}

void DisplayWindows::CreateSystemWindow()
{
    HINSTANCE inst = GetModuleHandle(NULL);

    WNDCLASSEX win_class;
    ZeroMemory(&win_class, sizeof(win_class));

    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = win_proc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = inst;
    win_class.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = m_name.c_str();
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hIcon = (HICON) LoadImage(NULL, "icon.ico", IMAGE_ICON, SM_CXICON, SM_CYICON, LR_LOADFROMFILE);
    win_class.hIconSm = win_class.hIcon;

    if (!RegisterClassEx(&win_class))
    {
        return;
    }

    DWORD style = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX;
    DWORD style_ex = 0;

    RECT wr = { 0, 0, m_width, m_height };
    AdjustWindowRect(&wr, style, FALSE);

    int x = (GetSystemMetrics(SM_CXSCREEN) - m_width) / 2 + wr.left;
    int y = (GetSystemMetrics(SM_CYSCREEN) - m_height) / 2 + wr.top;

    HWND hwnd = CreateWindowEx(
        style_ex,			// window ex style
        m_name.c_str(),		// class name
        m_name.c_str(),		// app name
        style,			    // window style
        x, y,				// x, y
        wr.right - wr.left, // width
        wr.bottom - wr.top, // height
        NULL,				// handle to parent
        NULL,               // handle to menu
        inst,				// hInstance
        NULL);              // no extra parameters
    if (!hwnd)
    {
        return;
    }

    ShowWindow(hwnd, SW_SHOW);

    m_window = hwnd;
}

void DisplayWindows::CreateBuffers()
{
    m_hdc = GetDC(m_window);

    m_buffer_size = m_width * m_height * 4;
    m_color_buffers[0] = Memory::Alloc<void>(m_buffer_size);
    m_color_buffers[1] = Memory::Alloc<void>(m_buffer_size);
    m_depth_buffers[0] = Memory::Alloc<void>(m_buffer_size);
    m_depth_buffers[1] = Memory::Alloc<void>(m_buffer_size);
    Memory::Zero(m_color_buffers[0], m_buffer_size);
    Memory::Zero(m_color_buffers[1], m_buffer_size);
    Memory::Zero(m_depth_buffers[0], m_buffer_size);
    Memory::Zero(m_depth_buffers[1], m_buffer_size);

    // setup bitmap info for blit
    int bmi_size = sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3;
    m_bmi_buffer = Memory::Alloc<void>(bmi_size);
    BITMAPINFO* bmi = (BITMAPINFO*) m_bmi_buffer;
    Memory::Zero(bmi, bmi_size);
    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth = m_width;
    bmi->bmiHeader.biHeight = m_height;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biCompression = BI_BITFIELDS;
    ((DWORD*) bmi->bmiColors)[0] = 0xff; // r mask
    ((DWORD*) bmi->bmiColors)[1] = 0xff00; // g mask
    ((DWORD*) bmi->bmiColors)[2] = 0xff0000; // b mask
}

void DisplayWindows::SwapBuffers()
{
    // blit front color buffer to window dc
    SetDIBitsToDevice(m_hdc,
        0, 0,
        m_width, m_height,
        0, 0,
        0, m_height,
        m_color_buffers[m_front_buffer],
        (BITMAPINFO*) m_bmi_buffer,
        DIB_RGB_COLORS);

    // swap front and back buffer
    m_front_buffer = (m_front_buffer + 1) % 2;

    set_gl_context_default_buffers(m_color_buffers[m_front_buffer], m_depth_buffers[m_front_buffer], m_width, m_height);
}

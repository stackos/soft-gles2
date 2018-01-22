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

#include <Windows.h>
#include <string>

class DisplayWindows
{
public:
    DisplayWindows(const std::string& name, int width, int height);
    virtual ~DisplayWindows();
    bool ProcessSystemEvents();
    void SwapBuffers();

private:
    void CreateSystemWindow();
    void CreateBuffers();

protected:
    std::string m_name;
    int m_width;
    int m_height;
    HWND m_window;
    HDC m_hdc;
    void* m_color_buffers[2];
    void* m_depth_buffers[2];
    int m_front_buffer;
    int m_buffer_size;
    void* m_bmi_buffer;
};

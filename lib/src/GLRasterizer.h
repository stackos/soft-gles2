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

#include "GLProgram.h"
#include "math/Vector4.h"
#include "math/Vector2i.h"
#include "container/Vector.h"
#include <functional>

namespace sgl
{
    typedef std::function<void(const Viry3D::Vector2i& p, const Viry3D::Vector4& c, float depth)> SetFragmentFunc;

    class GLRasterizer
    {
    public:
        GLRasterizer(
            const Viry3D::Vector4* positions,
            const Viry3D::Vector<GLProgram::Varying>* varyings,
            GLProgram* program,
            SetFragmentFunc set_fragment,
            int viewport_x,
            int viewport_y,
            int viewport_width,
            int viewport_height,
            bool ccw):
            m_positions(positions),
            m_varyings(varyings),
            m_program(program),
            m_set_fragment(set_fragment),
            m_viewport_x(viewport_x),
            m_viewport_y(viewport_y),
            m_viewport_width(viewport_width),
            m_viewport_height(viewport_height),
            m_ccw(ccw)
        {
        }
        void Run();

    private:
        float ProjToScreenX(float x);
        float ProjToScreenY(float y);
        void DrawHalfTriangle(
            const Viry3D::Vector<Viry3D::Vector2i>& e1, const Viry3D::Vector<Viry3D::Vector2i>& e2,
            int y_top, int y_bottom, bool include_bottom,
            const Viry3D::Vector2i& p0, const Viry3D::Vector2i& p1, const Viry3D::Vector2i& p2);
        void DrawScanLine(int y, int min_x, int max_x, const Viry3D::Vector2i& p0, const Viry3D::Vector2i& p1, const Viry3D::Vector2i& p2);
        void DrawScanLine2(int y0, int min_x0, int max_x0, int min_x1, int max_x1, bool draw_y1, const Viry3D::Vector2i& p0, const Viry3D::Vector2i& p1, const Viry3D::Vector2i& p2);

        const Viry3D::Vector4* m_positions;
        const Viry3D::Vector<GLProgram::Varying>* m_varyings;
        GLProgram* m_program;
        SetFragmentFunc m_set_fragment;
        int m_viewport_x;
        int m_viewport_y;
        int m_viewport_width;
        int m_viewport_height;
        bool m_ccw;
    };
}

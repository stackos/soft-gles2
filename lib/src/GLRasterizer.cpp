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

#include "GLRasterizer.h"
#include "math/Vector2.h"
#include "math/Vector4.h"

using namespace Viry3D;

namespace sgl
{
    static Vector<Vector2i> TriangleEdge(const Vector2i& p0, const Vector2i& p1)
    {
        Vector<Vector2i> line;

        if (p0.x == p1.x)
        {
            int y = p0.y;
            int step = p1.y > p0.y ? 1 : -1;
            while (y != p1.y)
            {
                line.Add({ p0.x, y });
                y += step;
            }
            line.Add({ p1.x, p1.y });
        }
        else
        {
            float k = fabs((p1.y - p0.y) / (float) (p1.x - p0.x));

            if (k < 1) // x step 1
            {
                int x = p0.x;
                int y = p0.y;
                int x_step = p1.x > p0.x ? 1 : -1;
                int y_step = p1.y > p0.y ? 1 : -1;
                float d_step = k;
                float d = 0;
                while (x != p1.x)
                {
                    if (d >= 0.5f)
                    {
                        line.Add({ x, y + y_step });
                    }
                    else
                    {
                        line.Add({ x, y });
                    }

                    x += x_step;
                    d += d_step;
                    if (d >= 1.0f)
                    {
                        d -= 1.0f;
                        y += y_step;
                    }
                }
                line.Add({ p1.x, p1.y });
            }
            else // y step 1
            {
                int x = p0.x;
                int y = p0.y;
                int x_step = p1.x > p0.x ? 1 : -1;
                int y_step = p1.y > p0.y ? 1 : -1;
                float d_step = 1.0f / k;
                float d = 0;
                while (y != p1.y)
                {
                    if (d >= 0.5f)
                    {
                        line.Add({ x + x_step, y });
                    }
                    else
                    {
                        line.Add({ x, y });
                    }

                    y += y_step;
                    d += d_step;
                    if (d >= 1.0f)
                    {
                        d -= 1.0f;
                        x += x_step;
                    }
                }
                line.Add({ p1.x, p1.y });
            }
        }

        return line;
    }

    static bool IsTopLeftEdge(const Vector2i& p0, const Vector2i& p1)
    {
        return ((p1.y > p0.y) || (p0.y == p1.y && p0.x > p1.x));
    }

    static int EdgeEquation(const Vector2i& p, const Vector2i& p0, const Vector2i& p1, bool ccw)
    {
        int q = (p1.x - p0.x) * (p.y - p0.y) - (p1.y - p0.y) * (p.x - p0.x);
        if (ccw == false)
        {
            q = -q;
        }
        return q + (IsTopLeftEdge(p0, p1) ? 0 : -1);
    }

    float GLRasterizer::ProjToScreenX(float x)
    {
        return m_viewport_x + (x * 0.5f + 0.5f) * m_viewport_width;
    }

    float GLRasterizer::ProjToScreenY(float y)
    {
        return m_viewport_y + (y * 0.5f + 0.5f) * m_viewport_height;
    }

    void GLRasterizer::DrawScanLine2(int y0, int min_x0, int max_x0, int min_x1, int max_x1, bool draw_y1, const Vector2i& p0, const Vector2i& p1, const Vector2i& p2)
    {
        // 重心坐标插值
        float one_div_area = 1.0f / fabs(Vector2i::Cross(p0 - p1, p2 - p1) / 2.0f);
        // 透视校正
        float one_div_ws[3] = {
            1.0f / m_positions[0].w,
            1.0f / m_positions[1].w,
            1.0f / m_positions[2].w,
        };
        float depths[3] = {
            m_positions[0].z * one_div_ws[0],
            m_positions[1].z * one_div_ws[1],
            m_positions[2].z * one_div_ws[2],
        };

        int y = y0;
        int min_x = Mathf::Min(min_x0, min_x1);
        int max_x = Mathf::Max(max_x0, max_x1) + 1;
        int mins[2] = { min_x0, min_x1 };
        int maxs[2] = { max_x0, max_x1 };

        for (int x = min_x; x <= max_x; x += 2)
        {
            struct Varying
            {
                Vector4 value;
                String name;
                int size;
            };

            struct Fragment
            {
                Vector2i p;
                bool draw;
                Vector4 frag_coord;
                float depth;
                Vector<Varying> varyings;
            };
            Fragment fragment[2][2];

            // 2 x 2 block
            for (int iy = 0; iy < 2; ++iy)
            {
                for (int ix = 0; ix < 2; ++ix)
                {
                    Fragment& f = fragment[iy][ix];

                    Vector2i p(x + ix, y - iy);
                    int w1 = EdgeEquation(p, p0, p1, m_ccw);
                    int w2 = EdgeEquation(p, p1, p2, m_ccw);
                    int w3 = EdgeEquation(p, p2, p0, m_ccw);

                    float a01 = fabs(Vector2i::Cross(p1 - p, p0 - p) / 2.0f) * one_div_area;
                    float a12 = fabs(Vector2i::Cross(p2 - p, p1 - p) / 2.0f) * one_div_area;
                    float a20 = 1.0f - a01 - a12;

                    float w = 1.0f / (a01 * one_div_ws[2] + a12 * one_div_ws[0] + a20 * one_div_ws[1]);

                    int varying_count = m_varyings[0].Size();
                    for (int i = 0; i < varying_count; ++i)
                    {
                        Vector4 varying = (m_varyings[2][i].value * a01 * one_div_ws[2] + m_varyings[0][i].value * a12 * one_div_ws[0] + m_varyings[1][i].value * a20 * one_div_ws[1]) * w;
                        
                        Varying v;
                        v.value = varying;
                        v.name = m_varyings[0][i].name;
                        v.size = m_varyings[0][i].size;

                        f.varyings.Add(v);
                    }

                    float depth = depths[2] * a01 + depths[0] * a12 + depths[1] * a20;
                    Vector4 frag_coord((float) p.x, (float) p.y, depth, 1.0f / w);

                    f.p = p;
                    f.draw = w1 >= 0 && w2 >= 0 && w3 >= 0;
                    f.frag_coord = frag_coord;
                    f.depth = depth;
                }
            }

            int lines = 0;
            if (draw_y1)
            {
                lines = 2;
            }
            else
            {
                lines = 1;
            }

            for (int iy = 0; iy < lines; ++iy)
            {
                for (int ix = 0; ix < 2; ++ix)
                {
                    const Fragment& f = fragment[iy][ix];

                    if (f.draw &&
                        f.p.x >= mins[iy] && f.p.x <= maxs[iy] &&
                        f.p.x >= m_viewport_x && f.p.x < m_viewport_x + m_viewport_width)
                    {
                        for (int i = 0; i < f.varyings.Size(); ++i)
                        {
                            const Varying& v = f.varyings[i];
                            m_program->SetFSVarying(v.name, &v.value, v.size);
                        }

                        Vector4 color = *(Vector4*) m_program->CallFSMain(f.frag_coord);

                        m_set_fragment(f.p, color, f.depth);
                    }
                }
            }
        }
    }

    void GLRasterizer::DrawScanLine(int y, int min_x, int max_x, const Vector2i& p0, const Vector2i& p1, const Vector2i& p2)
    {
        // 重心坐标插值
        float one_div_area = 1.0f / fabs(Vector2i::Cross(p0 - p1, p2 - p1) / 2.0f);
        // 透视校正
        float one_div_ws[3] = {
            1.0f / m_positions[0].w,
            1.0f / m_positions[1].w,
            1.0f / m_positions[2].w,
        };
        float depths[3] = {
            m_positions[0].z * one_div_ws[0],
            m_positions[1].z * one_div_ws[1],
            m_positions[2].z * one_div_ws[2],
        };

        for (int x = min_x; x <= max_x; ++x)
        {
            if (x >= m_viewport_x && x < m_viewport_x + m_viewport_width)
            {
                Vector2i p(x, y);
                int w1 = EdgeEquation(p, p0, p1, m_ccw);
                int w2 = EdgeEquation(p, p1, p2, m_ccw);
                int w3 = EdgeEquation(p, p2, p0, m_ccw);

                if (w1 >= 0 && w2 >= 0 && w3 >= 0)
                {
                    float a01 = fabs(Vector2i::Cross(p1 - p, p0 - p) / 2.0f) * one_div_area;
                    float a12 = fabs(Vector2i::Cross(p2 - p, p1 - p) / 2.0f) * one_div_area;
                    float a20 = 1.0f - a01 - a12;

                    float w = 1.0f / (a01 * one_div_ws[2] + a12 * one_div_ws[0] + a20 * one_div_ws[1]);

                    int varying_count = m_varyings[0].Size();
                    for (int i = 0; i < varying_count; ++i)
                    {
                        Vector4 varying = (m_varyings[2][i].value * a01 * one_div_ws[2] + m_varyings[0][i].value * a12 * one_div_ws[0] + m_varyings[1][i].value * a20 * one_div_ws[1]) * w;
                        m_program->SetFSVarying(m_varyings[0][i].name, &varying, m_varyings[0][i].size);
                    }

                    float depth = depths[2] * a01 + depths[0] * a12 + depths[1] * a20;
                    Vector4 frag_coord((float) p.x, (float) p.y, depth, 1.0f / w);

                    Vector4 color = *(Vector4*) m_program->CallFSMain(frag_coord);
                    
                    m_set_fragment(p, color, depth);
                }
            }
        }
    }

    void GLRasterizer::DrawHalfTriangle(
        const Vector<Vector2i>& e1, const Vector<Vector2i>& e2,
        int y_top, int y_bottom, bool include_bottom,
        const Vector2i& p0, const Vector2i& p1, const Vector2i& p2)
    {
        int i1 = 0;
        int i2 = 0;
        int y = y_top;

        int y0 = -1;
        int min_x0 = -1;
        int max_x0 = -1;
        int lines = 0;

        while (y >= y_bottom)
        {
            if (y == y_bottom && include_bottom == false)
            {
                break;
            }

            int min_x = 0x7fffffff;
            int max_x = -1;

            for (int i = i1; i < e1.Size(); ++i)
            {
                if (e1[i].y != y)
                {
                    i1 = i;
                    break;
                }
                else
                {
                    min_x = Mathf::Min(min_x, e1[i].x);
                    max_x = Mathf::Max(max_x, e1[i].x);
                }
            }

            for (int i = i2; i < e2.Size(); ++i)
            {
                if (e2[i].y != y)
                {
                    i2 = i;
                    break;
                }
                else
                {
                    min_x = Mathf::Min(min_x, e2[i].x);
                    max_x = Mathf::Max(max_x, e2[i].x);
                }
            }

            if (y >= m_viewport_y && y < m_viewport_y + m_viewport_height)
            {
                if (lines % 2 == 1)
                {
                    //DrawScanLine2(y0, min_x0, max_x0, min_x, max_x, true, p0, p1, p2);
                }

                y0 = y;
                min_x0 = min_x;
                max_x0 = max_x;

                DrawScanLine(y, min_x, max_x, p0, p1, p2);
            }

            --y;
            ++lines;
        }

        if (lines % 2 == 1)
        {
            //DrawScanLine2(y0, min_x0, max_x0, min_x0, max_x0, false, p0, p1, p2);
        }
    }

    void GLRasterizer::Run()
    {
        Vector2i p0;
        Vector2i p1;
        Vector2i p2;
        p0.x = (int) ProjToScreenX(m_positions[0].x / m_positions[0].w);
        p1.x = (int) ProjToScreenX(m_positions[1].x / m_positions[1].w);
        p2.x = (int) ProjToScreenX(m_positions[2].x / m_positions[2].w);
        p0.y = (int) ProjToScreenY(m_positions[0].y / m_positions[0].w);
        p1.y = (int) ProjToScreenY(m_positions[1].y / m_positions[1].w);
        p2.y = (int) ProjToScreenY(m_positions[2].y / m_positions[2].w);

        Vector4 a = m_positions[0];
        Vector4 b = m_positions[1];
        Vector4 c = m_positions[2];

        Vector4 pa = a / a.w;
        Vector4 pb = b / b.w;
        Vector4 pc = c / c.w;

        if (pa.y > pb.y)
        {
            std::swap(a, b);
            std::swap(pa, pb);
        }
        if (pb.y > pc.y)
        {
            std::swap(b, c);
            std::swap(pb, pc);
        }
        if (pa.y > pb.y)
        {
            std::swap(a, b);
            std::swap(pa, pb);
        }

        //             pc
        //
        //    pb     pd
        //
        //        pa

        {
            Vector2i sa, sb, sc, sd;
            sa.x = (int) ProjToScreenX(pa.x);
            sa.y = (int) ProjToScreenY(pa.y);
            sb.x = (int) ProjToScreenX(pb.x);
            sb.y = (int) ProjToScreenY(pb.y);
            sc.x = (int) ProjToScreenX(pc.x);
            sc.y = (int) ProjToScreenY(pc.y);

            // find pd
            {
                Vector<Vector2i> e = TriangleEdge(sc, sa);
                int min_x = 0x7fffffff;
                int max_x = -1;
                for (int i = 0; i < e.Size(); ++i)
                {
                    if (e[i].y == sb.y)
                    {
                        min_x = Mathf::Min(min_x, e[i].x);
                        max_x = Mathf::Max(max_x, e[i].x);
                    }
                }

                if (sb.x <= min_x)
                {
                    sd.x = max_x;
                }
                else
                {
                    sd.x = min_x;
                }
                sd.y = sb.y;
            }

            Vector<Vector2i> e1 = TriangleEdge(sc, sd);
            Vector<Vector2i> e2 = TriangleEdge(sc, sb);

            DrawHalfTriangle(e1, e2, sc.y, sd.y, false, p0, p1, p2);

            e1 = TriangleEdge(sd, sa);
            e2 = TriangleEdge(sb, sa);
            DrawHalfTriangle(e1, e2, sd.y, sa.y, true, p0, p1, p2);
        }
    }
}

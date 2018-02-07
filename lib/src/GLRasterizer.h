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
#include "container/Vector.h"

namespace sgl
{
    class GLRasterizer
    {
    public:
        GLRasterizer(const Viry3D::Vector4* positions, const Viry3D::Vector<GLProgram::Varying>* varyings):
            m_positions(positions),
            m_varyings(varyings)
        {
        }
        void Run();

    private:
        const Viry3D::Vector4* m_positions;
        const Viry3D::Vector<GLProgram::Varying>* m_varyings;
    };
}
 
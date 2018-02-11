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

#include "GLTexture2D.h"

using namespace Viry3D;

namespace sgl
{
    Vector4 GLTexture2D::Sample(const Vector2& uv) const
    {
        /*
        int x = (int) ((tex->width - 1) * Mathf::Clamp01(uv->x));
        int y = (int) ((tex->height - 1) * Mathf::Clamp01(uv->y));

        int r = tex->data[y * tex->width * 4 + x * 4 + 0];
        int g = tex->data[y * tex->width * 4 + x * 4 + 1];
        int b = tex->data[y * tex->width * 4 + x * 4 + 2];
        int a = tex->data[y * tex->width * 4 + x * 4 + 3];

        return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        */

        return Vector4(1, 1, 1, 1);
    }
}

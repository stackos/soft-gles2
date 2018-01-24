#include <memory.h>
#define DLL_EXPORT extern "C" _declspec(dllexport)
#define uniform
#define attribute
#define varying

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

    vec4(float x = 0, float y = 0, float z = 0, float w = 1):
        x(x),
        y(y),
        z(z),
        w(w)
    {
    }

    vec4 operator*(const vec4& right) const
    {
        return vec4(x * right.x, y * right.y, z * right.z, w * right.w);
    }
};

vec4 gl_Position;

//
// shader begin
//
uniform vec4 u_color;

attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_color;

DLL_EXPORT
void main()
{
    gl_Position = a_position;
    v_color = a_color * u_color;
}
//
// shader end
//

#define VS_VAR_SETTER(type, var) \
    DLL_EXPORT void set_##var(void* p, int size) \
    { \
        memcpy(&var, p, size); \
    }
#define VS_VAR_GETTER(type, var) \
    DLL_EXPORT void* get_##var() \
    { \
        return &var; \
    }
#define VS_UNIFORM_SETTER VS_VAR_SETTER
#define VS_ATTRIBUTE_SETTER VS_VAR_SETTER
#define VS_VARYING_GETTER VS_VAR_GETTER

VS_UNIFORM_SETTER(vec4, u_color)
VS_ATTRIBUTE_SETTER(vec4, a_position)
VS_ATTRIBUTE_SETTER(vec4, a_color)
VS_VAR_GETTER(vec4, gl_Position)
VS_VARYING_GETTER(vec4, v_color)

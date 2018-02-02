#include <memory.h>
#define DLL_EXPORT extern "C" _declspec(dllexport)
#define precision
#define highp
#define mediump
#define lowp
#define uniform static
#define attribute static
#define varying static
#define VAR_SETTER(var) \
    DLL_EXPORT void set_##var(void* p, int size) \
    { \
        memcpy(&var, p, size); \
    }
#define VAR_GETTER(var) \
    DLL_EXPORT void* get_##var() \
    { \
        return &var; \
    }

struct vec2
{
    struct
    {
        float x;
        float y;
    };

    vec2(float x = 0, float y = 0):
        x(x),
        y(y)
    {
    }
};

struct vec3
{
    float x;
    float y;
    float z;

    vec3(float x = 0, float y = 0, float z = 0):
        x(x),
        y(y),
        z(z)
    {
    }
};

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

static vec4 gl_Position;

//
// shader begin
//
attribute vec4 a_position;
attribute vec2 a_uv;
attribute vec4 a_color;

varying vec2 v_uv;
varying vec4 v_color;

DLL_EXPORT
void vs_main()
{
    gl_Position = a_position;
    v_uv = a_uv;
    v_color = a_color;
}
//
// shader end
//

VAR_SETTER(a_position)
VAR_SETTER(a_uv)
VAR_SETTER(a_color)
VAR_GETTER(gl_Position)
VAR_GETTER(v_uv)
VAR_GETTER(v_color)

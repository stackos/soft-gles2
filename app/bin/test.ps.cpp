#include <memory.h>
#define DLL_EXPORT extern "C" _declspec(dllexport)
#define uniform static
#define attribute static
#define varying static

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

class sampler2D
{
public:
    typedef vec4(*Sample)(void*, void*);
    void* texture;
    Sample sample_func;
};

static vec4 gl_FragColor;
static vec4 texture2D(const sampler2D& sampler, const vec2& uv)
{
    return sampler.sample_func(sampler.texture, (void*) &uv);
}

//
// shader begin
//
uniform sampler2D u_tex;

varying vec2 v_uv;
varying vec4 v_color;

DLL_EXPORT
void ps_main()
{
    //gl_FragColor = v_color;
    gl_FragColor = texture2D(u_tex, v_uv);
}
//
// shader end
//

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
#define UNIFORM_SETTER VAR_SETTER
#define VARYING_SETTER VAR_SETTER

UNIFORM_SETTER(u_tex)
VARYING_SETTER(v_uv)
VARYING_SETTER(v_color)
VAR_GETTER(gl_FragColor)

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

        float v[4];
    };

    vec4(float x = 0, float y = 0, float z = 0, float w = 1):
        x(x),
        y(y),
        z(z),
        w(w)
    {
    }

    vec4 operator*(const vec4& v) const
    {
        return vec4(x * v.x, y * v.y, z * v.z, w * v.w);
    }

    float& operator[](int index)
    {
        return v[index];
    }

    const float& operator[](int index) const
    {
        return v[index];
    }
};

struct mat4
{
    mat4()
    {
        memset(m_columns, 0, sizeof(m_columns));
    }

    mat4(float v)
    {
        memset(m_columns, 0, sizeof(m_columns));
        m_columns[0][0] = v;
        m_columns[1][1] = v;
        m_columns[2][2] = v;
        m_columns[3][3] = v;
    }

    mat4(const vec4& v0,
        const vec4& v1,
        const vec4& v2,
        const vec4& v3)
    {
        m_columns[0] = v0;
        m_columns[1] = v1;
        m_columns[2] = v2;
        m_columns[3] = v3;
    }

    mat4(float v0, float v1, float v2, float v3,
        float v4, float v5, float v6, float v7,
        float v8, float v9, float v10, float v11,
        float v12, float v13, float v14, float v15)
    {
        m_columns[0] = vec4(v0, v1, v2, v3);
        m_columns[1] = vec4(v4, v5, v6, v7);
        m_columns[2] = vec4(v8, v9, v10, v11);
        m_columns[3] = vec4(v12, v13, v14, v15);
    }

    vec4& operator[](int index)
    {
        return m_columns[index];
    }

    const vec4& operator[](int index) const
    {
        return m_columns[index];
    }

    vec4 operator*(const vec4& v) const
    {
        float x = m_columns[0][0] * v.x + m_columns[1][0] * v.y + m_columns[2][0] * v.z + m_columns[3][0] * v.w;
        float y = m_columns[0][1] * v.x + m_columns[1][1] * v.y + m_columns[2][1] * v.z + m_columns[3][1] * v.w;
        float z = m_columns[0][2] * v.x + m_columns[1][2] * v.y + m_columns[2][2] * v.z + m_columns[3][2] * v.w;
        float w = m_columns[0][3] * v.x + m_columns[1][3] * v.y + m_columns[2][3] * v.z + m_columns[3][3] * v.w;

        return vec4(x, y, z, w);
    }

private:
    vec4 m_columns[4];

    friend vec4 operator*(const vec4& v, const mat4& m);
};

static vec4 operator*(const vec4& v, const mat4& m)
{
    float x = v.x * m.m_columns[0][0] + v.y * m.m_columns[0][1] + v.z * m.m_columns[0][2] + v.w * m.m_columns[0][3];
    float y = v.x * m.m_columns[1][0] + v.y * m.m_columns[1][1] + v.z * m.m_columns[1][2] + v.w * m.m_columns[1][3];
    float z = v.x * m.m_columns[2][0] + v.y * m.m_columns[2][1] + v.z * m.m_columns[2][2] + v.w * m.m_columns[2][3];
    float w = v.x * m.m_columns[3][0] + v.y * m.m_columns[3][1] + v.z * m.m_columns[3][2] + v.w * m.m_columns[3][3];

    return vec4(x, y, z, w);
}

class sampler2D
{
public:
    typedef vec4(*Sample)(void*, void*);
    void* texture;
    Sample sample_func;
};

static vec4 texture2D(const sampler2D& sampler, const vec2& uv)
{
    return sampler.sample_func(sampler.texture, (void*) &uv);
}

static vec4 gl_FragColor;

//
// shader begin
//
precision highp float;

uniform sampler2D u_tex;

varying vec2 v_uv;
varying vec4 v_color;

DLL_EXPORT
void fs_main()
{
    gl_FragColor = texture2D(u_tex, v_uv) * v_color;
}
//
// shader end
//

VAR_SETTER(u_tex)
VAR_SETTER(v_uv)
VAR_SETTER(v_color)
VAR_GETTER(gl_FragColor)

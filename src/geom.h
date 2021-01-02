struct vec;
struct ivec;

//vector3: three dimensional vector object
//this object finds uses in nearly every part of the engine,
//including world geometry, mapmodels, particles, projectiles, actors, ai
struct vec
{
    union
    {
        struct { float x, y, z; };
        struct { float r, g, b; };
        float v[3];
    };

    vec() {}
    explicit vec(int a) : x(a), y(a), z(a) {}
    explicit vec(float a) : x(a), y(a), z(a) {}
    vec(float a, float b, float c) : x(a), y(b), z(c) {}
    explicit vec(int v[3]) : x(v[0]), y(v[1]), z(v[2]) {}
    explicit vec(const float *v) : x(v[0]), y(v[1]), z(v[2]) {}
    explicit vec(const ivec &v);

    vec(float yaw, float pitch) : x(-sinf(yaw)*cosf(pitch)), y(cosf(yaw)*cosf(pitch)), z(sinf(pitch)) {}
    vec &set(int i, float f) { v[i] = f; return *this; }

    //operator overloads
    float &operator[](int i)       { return v[i]; }
    float  operator[](int i) const { return v[i]; }
    bool operator==(const vec &o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const vec &o) const { return x != o.x || y != o.y || z != o.z; }

    //unary operators
    bool iszero() const { return x==0 && y==0 && z==0; }
    float squaredlen() const { return x*x + y*y + z*z; }
    vec &square()            { mul(*this); return *this; }
    vec &neg2()              { x = -x; y = -y; return *this; } //unused
    vec &neg()               { x = -x; y = -y; z = -z; return *this; }
    vec &abs() { x = fabs(x); y = fabs(y); z = fabs(z); return *this; }
    vec &recip()             { x = 1/x; y = 1/y; z = 1/z; return *this; } //used twice
    float magnitude2() const { return sqrtf(dot2(*this)); }
    float magnitude() const  { return sqrtf(squaredlen()); }
    vec &normalize()         { div(magnitude()); return *this; }
    vec &safenormalize()     { float m = magnitude(); if(m) div(m); return *this; }
    bool isnormalized() const { float m = squaredlen(); return (m>0.99f && m<1.01f); }

    //elementwise float operators
    vec &mul(float f)        { x *= f; y *= f; z *= f; return *this; }
    vec &mul2(float f)       { x *= f; y *= f; return *this; } //unused
    vec &div(float f)        { x /= f; y /= f; z /= f; return *this; }
    vec &div2(float f)       { x /= f; y /= f; return *this; } //unused
    vec &add(float f)        { x += f; y += f; z += f; return *this; }
    vec &add2(float f)       { x += f; y += f; return *this; } //used once
    vec &addz(float f)       { z += f; return *this; } //unused
    vec &sub(float f)        { x -= f; y -= f; z -= f; return *this; }
    vec &sub2(float f)       { x -= f; y -= f; return *this; } //unused
    vec &subz(float f)       { z -= f; return *this; } //unused
    vec &min(float f)        { x = ::std::min(x, f); y = ::std::min(y, f); z = ::std::min(z, f); return *this; }
    vec &max(float f)        { x = ::std::max(x, f); y = ::std::max(y, f); z = ::std::max(z, f); return *this; }
    vec &clamp(float l, float h) { x = ::clamp(x, l, h); y = ::clamp(y, l, h); z = ::clamp(z, l, h); return *this; }

    //elementwise vector operators
    vec &mul(const vec &o)   { x *= o.x; y *= o.y; z *= o.z; return *this; }
    vec &div(const vec &o)   { x /= o.x; y /= o.y; z /= o.z; return *this; }
    vec &add(const vec &o)   { x += o.x; y += o.y; z += o.z; return *this; }
    vec &sub(const vec &o)   { x -= o.x; y -= o.y; z -= o.z; return *this; }
    vec &min(const vec &o)   { x = ::std::min(x, o.x); y = ::std::min(y, o.y); z = ::std::min(z, o.z); return *this; }
    vec &max(const vec &o)   { x = ::std::max(x, o.x); y = ::std::max(y, o.y); z = ::std::max(z, o.z); return *this; }

    vec operator+(const vec &o) const {return vec(x + o.x, y + o.y, z + o.z);}
    vec operator-(const vec &o) const {return vec(x - o.x, y - o.y, z - o.z);}

    //dot products
    float dot2(const vec &o) const { return x*o.x + y*o.y; }
    float dot(const vec &o) const { return x*o.x + y*o.y + z*o.z; }

    //distances
    float squaredist(const vec &e) const { return vec(*this).sub(e).squaredlen(); }
    float dist(const vec &e) const { return sqrtf(squaredist(e)); }
    float dist(const vec &e, vec &t) const { t = *this; t.sub(e); return t.magnitude(); }
    float dist2(const vec &o) const { float dx = x-o.x, dy = y-o.y; return sqrtf(dx*dx + dy*dy); }

    vec &rescale(float k)
    {
        float mag = magnitude();
        if(mag > 1e-6f) mul(k / mag);
        return *this;
    }
};

inline uint hthash(const vec &k)
{
    union { uint i; float f; } x, y, z;
    x.f = k.x; y.f = k.y; z.f = k.z;
    uint v = x.i^y.i^z.i;
    return v + (v>>12);
}


struct ivec
{
    union
    {
        struct { int x, y, z; };
        struct { int r, g, b; };
        int v[3];
    };

    ivec() {}
    explicit ivec(const vec &v) : x(int(v.x)), y(int(v.y)), z(int(v.z)) {}
    ivec(int a, int b, int c) : x(a), y(b), z(c) {}
    ivec(int i, const ivec &co, int size) : x(co.x+((i&1)>>0)*size), y(co.y+((i&2)>>1)*size), z(co.z +((i&4)>>2)*size) {}

    int &operator[](int i)       { return v[i]; }
    int  operator[](int i) const { return v[i]; }

    //int idx(int i) { return v[i]; }
    bool operator==(const ivec &v) const { return x==v.x && y==v.y && z==v.z; }
    bool operator!=(const ivec &v) const { return x!=v.x || y!=v.y || z!=v.z; }
    bool iszero() const { return x==0 && y==0 && z==0; }
    ivec &shl(int n) { x<<= n; y<<= n; z<<= n; return *this; }
    ivec &shr(int n) { x>>= n; y>>= n; z>>= n; return *this; }
    ivec &mul(int n) { x *= n; y *= n; z *= n; return *this; }
    ivec &div(int n) { x /= n; y /= n; z /= n; return *this; }
    ivec &add(int n) { x += n; y += n; z += n; return *this; }
    ivec &sub(int n) { x -= n; y -= n; z -= n; return *this; }
    ivec &mul(const ivec &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    ivec &div(const ivec &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    ivec &add(const ivec &v) { x += v.x; y += v.y; z += v.z; return *this; }
    ivec &sub(const ivec &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    ivec &mask(int n) { x &= n; y &= n; z &= n; return *this; }
    ivec &neg() { x = -x; y = -y; z = -z; return *this; }
    ivec &min(const ivec &o) { x = ::std::min(x, o.x); y = ::std::min(y, o.y); z = ::std::min(z, o.z); return *this; }
    ivec &max(const ivec &o) { x = ::std::max(x, o.x); y = ::std::max(y, o.y); z = ::std::max(z, o.z); return *this; }
    ivec &min(int n) { x = ::std::min(x, n); y = ::std::min(y, n); z = ::std::min(z, n); return *this; }
    ivec &max(int n) { x = ::std::max(x, n); y = ::std::max(y, n); z = ::std::max(z, n); return *this; }
    ivec &abs() { x = ::abs(x); y = ::abs(y); z = ::abs(z); return *this; }
    ivec &clamp(int l, int h) { x = ::clamp(x, l, h); y = ::clamp(y, l, h); z = ::clamp(z, l, h); return *this; }
    ivec &cross(const ivec &a, const ivec &b) { x = a.y*b.z-a.z*b.y; y = a.z*b.x-a.x*b.z; z = a.x*b.y-a.y*b.x; return *this; }
    int dot(const ivec &o) const { return x*o.x + y*o.y + z*o.z; }

    static inline ivec floor(const vec &o) { return ivec(int(::floor(o.x)), int(::floor(o.y)), int(::floor(o.z))); }
    static inline ivec ceil(const vec &o) { return ivec(int(::ceil(o.x)), int(::ceil(o.y)), int(::ceil(o.z))); }
};

inline vec::vec(const ivec &v) : x(v.x), y(v.y), z(v.z) {}

inline bool htcmp(const ivec &x, const ivec &y)
{
    return x == y;
}

inline uint hthash(const ivec &k)
{
    return k.x^k.y^k.z;
}

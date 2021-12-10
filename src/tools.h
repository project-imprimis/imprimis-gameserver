// generic useful stuff for any C++ program

#ifndef _TOOLS_H
#define _TOOLS_H

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

template<class T, class U>
inline T clamp(T a, U b, U c)
{
    return std::max(T(b), std::min(a, T(c)));
}

inline int randomint(int x)
{
    return rand()%(x);
}

#define DELETEP(p) if(p) { delete   p; p = 0; }
#define DELETEA(p) if(p) { delete[] p; p = 0; }

#define PI (3.14159265358979f)
#define RAD (PI / 180.0f)

#ifdef WIN32

#ifndef __GNUC__
#pragma warning (3: 4189)       // local variable is initialized but not referenced
#pragma warning (disable: 4244) // conversion from 'int' to 'float', possible loss of data
#pragma warning (disable: 4267) // conversion from 'size_t' to 'int', possible loss of data
#pragma warning (disable: 4355) // 'this' : used in base member initializer list
#pragma warning (disable: 4996) // 'strncpy' was declared deprecated
#endif

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define PATHDIV '\\'

#else
#define __cdecl
#define _vsnprintf vsnprintf
#define PATHDIV '/'
#endif

#ifdef __GNUC__
#define PRINTFARGS(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define PRINTFARGS(fmt, args)
#endif

// easy safe strings

constexpr int MAXSTRLEN = 260;
typedef char string[MAXSTRLEN];

inline void formatstring(char *d, const char *fmt, va_list v) { _vsnprintf(d, MAXSTRLEN, fmt, v); d[MAXSTRLEN-1] = 0; }
struct s_sprintf_f
{
    char *d;
    s_sprintf_f(char *str): d(str) {}
    void operator()(const char* fmt, ...)
    {
        va_list v;
        va_start(v, fmt);
        formatstring(d, fmt, v);
        va_end(v);
    }
};
inline char *s_strncpy(char *d, const char *s, size_t m) { strncpy(d,s,m-1); d[m-1] = 0; return d; }
inline char *s_strcpy(char *d, const char *s) { return s_strncpy(d,s,MAXSTRLEN); }
#define s_sprintf(d) s_sprintf_f((char *)d)
#define s_sprintfd(d) string d; s_sprintf(d)

inline void vformatstring(char *d, const char *fmt, va_list v, int len) { _vsnprintf(d, len, fmt, v); d[len-1] = 0; }

template<size_t N>
inline void vformatstring(char (&d)[N], const char *fmt, va_list v) { vformatstring(d, fmt, v, N); }

inline char *copystring(char *d, const char *s, size_t len)
{
    size_t slen = std::min(strlen(s), len-1);
    memcpy(d, s, slen);
    d[slen] = 0;
    return d;
}
template<size_t N>
inline char *copystring(char (&d)[N], const char *s) { return copystring(d, s, N); }

inline char *concatstring(char *d, const char *s, size_t len) { size_t used = strlen(d); return used < len ? copystring(d+used, s, len-used) : d; }
template<size_t N>
inline char *concatstring(char (&d)[N], const char *s) { return concatstring(d, s, N); }

template<size_t N>
inline void formatstring(char (&d)[N], const char *fmt, ...) PRINTFARGS(2, 3);

template<size_t N>
inline void formatstring(char (&d)[N], const char *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    vformatstring(d, fmt, v, int(N));
    va_end(v);
}

extern char *tempformatstring(const char *fmt, ...) PRINTFARGS(1, 2);

#define DEF_FORMAT_STRING(d,...) string d; formatstring(d, __VA_ARGS__)
#define DEFV_FORMAT_STRING(d,last,fmt) string d; { va_list ap; va_start(ap, last); vformatstring(d, fmt, ap); va_end(ap); }

template<size_t N>
inline bool matchstring(const char *s, size_t len, const char (&d)[N])
{
    return len == N-1 && !memcmp(s, d, N-1);
}

inline char *newstring(size_t l)                { return new char[l+1]; }
inline char *newstring(const char *s, size_t l) { return copystring(newstring(l), s, l+1); }
inline char *newstring(const char *s)           { size_t l = strlen(s); char *d = newstring(l); memcpy(d, s, l+1); return d; }

template <class T>
struct databuf
{
    enum
    {
        OVERREAD  = 1<<0,
        OVERWROTE = 1<<1
    };

    T *buf;
    int len, maxlen;
    uchar flags;

    databuf() : buf(nullptr), len(0), maxlen(0), flags(0) {}

    template<class U>
    databuf(T *buf, U maxlen) : buf(buf), len(0), maxlen((int)maxlen), flags(0) {}

    void reset()
    {
        len = 0;
        flags = 0;
    }

    void reset(T *buf_, int maxlen_)
    {
        reset();
        buf = buf_;
        maxlen = maxlen_;
    }

    const T &get()
    {
        static const T overreadval = 0;
        if(len<maxlen) return buf[len++];
        flags |= OVERREAD;
        return overreadval;
    }

    databuf subbuf(int sz)
    {
        sz = clamp(sz, 0, maxlen-len);
        len += sz;
        return databuf(&buf[len-sz], sz);
    }

    T *pad(int numvals)
    {
        T *vals = &buf[len];
        len += std::min(numvals, maxlen-len);
        return vals;
    }

    void put(const T &val)
    {
        if(len<maxlen) buf[len++] = val;
        else flags |= OVERWROTE;
    }

    void put(const T *vals, int numvals)
    {
        if(maxlen - len < numvals)
        {
            numvals = maxlen - len;
            flags |= OVERWROTE;
        }
        memcpy(&buf[len], (const void *)vals, numvals*sizeof(T));
        len += numvals;
    }

    int get(T *vals, int numvals)
    {
        if(maxlen - len < numvals)
        {
            numvals = maxlen - len;
            flags |= OVERREAD;
        }
        memcpy(vals, (void *)&buf[len], numvals*sizeof(T));
        len += numvals;
        return numvals;
    }

    void offset(int n)
    {
        n = std::min(n, maxlen);
        buf += n;
        maxlen -= n;
        len = std::max(len-n, 0);
    }

    T *getbuf() const { return buf; }
    bool empty() const { return len==0; }
    int length() const { return len; }
    int remaining() const { return maxlen-len; }
    bool overread() const { return (flags&OVERREAD)!=0; }
    bool check(int n) { return remaining() >= n; }

    void forceoverread()
    {
        len = maxlen;
        flags |= OVERREAD;
    }
};

typedef databuf<char> charbuf;
typedef databuf<uchar> ucharbuf;

struct packetbuf : ucharbuf
{
    ENetPacket *packet;
    int growth;

    packetbuf(ENetPacket *packet) : ucharbuf(packet->data, packet->dataLength), packet(packet), growth(0) {}
    packetbuf(int growth, int pflags = 0) : growth(growth)
    {
        packet = enet_packet_create(nullptr, growth, pflags);
        buf = (uchar *)packet->data;
        maxlen = packet->dataLength;
    }
    ~packetbuf() { cleanup(); }

    void reliable() { packet->flags |= ENET_PACKET_FLAG_RELIABLE; }

    void resize(int n)
    {
        enet_packet_resize(packet, n);
        buf = (uchar *)packet->data;
        maxlen = packet->dataLength;
    }

    void checkspace(int n)
    {
        if(len + n > maxlen && packet && growth > 0) resize(std::max(len + n, maxlen + growth));
    }

    ucharbuf subbuf(int sz)
    {
        checkspace(sz);
        return ucharbuf::subbuf(sz);
    }

    void put(const uchar &val)
    {
        checkspace(1);
        ucharbuf::put(val);
    }

    void put(const uchar *vals, int numvals)
    {
        checkspace(numvals);
        ucharbuf::put(vals, numvals);
    }

    ENetPacket *finalize()
    {
        resize(len);
        return packet;
    }

    void cleanup()
    {
        if(growth > 0 && packet && !packet->referenceCount) { enet_packet_destroy(packet); packet = nullptr; buf = nullptr; len = maxlen = 0; }
    }
};

/* workaround for some C platforms that have these two functions as macros - not used anywhere */
#ifdef getchar
#undef getchar
#endif
#ifdef putchar
#undef putchar
#endif

struct stream
{
#ifdef WIN32
#if defined(__GNUC__) && !defined(__MINGW32__)
    typedef off64_t offset;
#else
    typedef __int64 offset;
#endif
#else
    typedef off_t offset;
#endif

    virtual ~stream() {}
    virtual void close() = 0;
    virtual bool end() = 0;
    virtual offset tell() { return -1; }
    virtual offset rawtell() { return tell(); }
    virtual bool seek(offset pos, int whence = SEEK_SET) { return false; }
    virtual offset size();
    virtual offset rawsize() { return size(); }
    virtual size_t read(void *buf, size_t len) { return 0; }
    virtual size_t write(const void *buf, size_t len) { return 0; }
    virtual bool flush() { return true; }
    virtual int getchar() { uchar c; return read(&c, 1) == 1 ? c : -1; }
    virtual bool putchar(int n) { uchar c = n; return write(&c, 1) == 1; }
    virtual bool getline(char *str, size_t len);
    virtual bool putstring(const char *str) { size_t len = strlen(str); return write(str, len) == len; }
    virtual bool putline(const char *str) { return putstring(str) && putchar('\n'); }
    virtual size_t printf(const char *fmt, ...) PRINTFARGS(2, 3);
    virtual uint getcrc() { return 0; }

    template<class T>
    size_t put(const T *v, size_t n) { return write(v, n*sizeof(T))/sizeof(T); }

    template<class T>
    bool put(T n) { return write(&n, sizeof(n)) == sizeof(n); }

    template<class T>
    size_t get(T *v, size_t n) { return read(v, n*sizeof(T))/sizeof(T); }

    template<class T>
    T get() { T n; return read(&n, sizeof(n)) == sizeof(n) ? n : 0; }
};

template<class T>
struct streambuf
{
    stream *s;

    streambuf(stream *s) : s(s) {}

    T get() { return s->get<T>(); }
    size_t get(T *vals, size_t numvals) { return s->get(vals, numvals); }
    void put(const T &val) { s->put(&val, 1); }
    void put(const T *vals, size_t numvals) { s->put(vals, numvals); }
    size_t length() { return s->size(); }
};

enum
{
    CT_PRINT   = 1<<0,
    CT_SPACE   = 1<<1,
    CT_DIGIT   = 1<<2,
    CT_ALPHA   = 1<<3,
    CT_LOWER   = 1<<4,
    CT_UPPER   = 1<<5,
    CT_UNICODE = 1<<6
};
extern const uchar cubectype[256];
inline int iscubeprint(uchar c) { return cubectype[c]&CT_PRINT; }
inline int iscubespace(uchar c) { return cubectype[c]&CT_SPACE; }
inline int iscubealnum(uchar c) { return cubectype[c]&(CT_ALPHA|CT_DIGIT); }

extern string homedir;

extern char *makerelpath(const char *dir, const char *file, const char *prefix = nullptr, const char *cmd = nullptr);
extern char *path(char *s);
extern const char *parentdir(const char *directory);
extern bool fileexists(const char *path, const char *mode);
extern bool createdir(const char *path);
extern size_t fixpackagedir(char *dir);
extern const char *sethomedir(const char *dir);
extern const char *findfile(const char *filename, const char *mode);
extern stream *openrawfile(const char *filename, const char *mode);
extern stream *openfile(const char *filename, const char *mode);
extern stream *opentempfile(const char *filename, const char *mode);
extern char *loadfile(const char *fn, size_t *size);

extern void putint(ucharbuf &p, int n);
extern void putint(packetbuf &p, int n);
extern void putint(std::vector<uchar> &p, int n);
extern int getint(ucharbuf &p);
extern void putuint(ucharbuf &p, int n);
extern int getuint(ucharbuf &p);
extern void putfloat(packetbuf &p, float f);
extern float getfloat(ucharbuf &p);
extern void sendstring(const char *t, ucharbuf &p);
extern void sendstring(const char *t, packetbuf &p);
extern void sendstring(const char *t, std::vector<uchar> &p);
extern void getstring(char *t, ucharbuf &p, size_t len);

template<size_t N>
inline void getstring(char (&t)[N], ucharbuf &p) { getstring(t, p, N); }

extern void filtertext(char *dst, const char *src, bool whitespace, bool forcespace, size_t len);

template<size_t N>
inline void filtertext(char (&dst)[N], const char *src, bool whitespace = true, bool forcespace = false) { filtertext(dst, src, whitespace, forcespace, N-1); }

struct ipmask
{
    enet_uint32 ip, mask;

    void parse(const char *name);
    int print(char *buf) const;
    bool check(enet_uint32 host) const { return (host & mask) == ip; }
};

#endif

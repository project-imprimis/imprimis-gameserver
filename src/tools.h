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

template<class T>
inline float heapscore(const T &n) { return n; }

struct sortless
{
    template<class T>
    bool operator()(const T &x, const T &y) const { return x < y; }

    bool operator()(char *x, char *y) const { return strcmp(x, y) < 0; }
    bool operator()(const char *x, const char *y) const { return strcmp(x, y) < 0; }
};

struct sortnameless
{
    template<class T>
    bool operator()(const T &x, const T &y) const { return sortless()(x.name, y.name); }

    template<class T>
    bool operator()(T *x, T *y) const { return sortless()(x->name, y->name); }

    template<class T>
    bool operator()(const T *x, const T *y) const { return sortless()(x->name, y->name); }
};

template<class T, class F>
inline void insertionsort(T *start, T *end, F fun)
{
    for(T *i = start+1; i < end; i++)
    {
        if(fun(*i, i[-1]))
        {
            T tmp = *i;
            *i = i[-1];
            T *j = i-1;
            for(; j > start && fun(tmp, j[-1]); --j)
                *j = j[-1];
            *j = tmp;
        }
    }

}

template<class T, class F>
inline void insertionsort(T *buf, int n, F fun)
{
    insertionsort(buf, buf+n, fun);
}

template<class T>
inline void insertionsort(T *buf, int n)
{
    insertionsort(buf, buf+n, sortless());
}

template<class T, class F>
inline void quicksort(T *start, T *end, F fun)
{
    while(end-start > 10)
    {
        T *mid = &start[(end-start)/2], *i = start+1, *j = end-2, pivot;
        if(fun(*start, *mid)) /* start < mid */
        {
            if(fun(end[-1], *start)) { pivot = *start; *start = end[-1]; end[-1] = *mid; } /* end < start < mid */
            else if(fun(end[-1], *mid)) { pivot = end[-1]; end[-1] = *mid; } /* start <= end < mid */
            else { pivot = *mid; } /* start < mid <= end */
        }
        else if(fun(*start, end[-1])) { pivot = *start; *start = *mid; } /*mid <= start < end */
        else if(fun(*mid, end[-1])) { pivot = end[-1]; end[-1] = *start; *start = *mid; } /* mid < end <= start */
        else { pivot = *mid; std::swap(*start, end[-1]); }  /* end <= mid <= start */
        *mid = end[-2];
        do
        {
            while(fun(*i, pivot)) if(++i >= j) goto partitioned;
            while(fun(pivot, *--j)) if(i >= j) goto partitioned;
            std::swap(*i, *j);
        }
        while(++i < j);
    partitioned:
        end[-2] = *i;
        *i = pivot;

        if(i-start < end-(i+1))
        {
            quicksort(start, i, fun);
            start = i+1;
        }
        else
        {
            quicksort(i+1, end, fun);
            end = i;
        }
    }

    insertionsort(start, end, fun);
}

template<class T, class F>
inline void quicksort(T *buf, int n, F fun)
{
    quicksort(buf, buf+n, fun);
}

template<class T>
inline void quicksort(T *buf, int n)
{
    quicksort(buf, buf+n, sortless());
}

template<class T> struct isclass
{
    template<class C>
    static char test(void (C::*)(void));

    template<class C>
    static int test(...);

    enum { yes = sizeof(test<T>(0)) == 1 ? 1 : 0, no = yes^1 };
};

inline uint hthash(const char *key)
{
    uint h = 5381;
    for(int i = 0, k; (k = key[i]); i++) h = ((h<<5)+h)^k;    // bernstein k=33 xor
    return h;
}

inline bool htcmp(const char *x, const char *y)
{
    return !strcmp(x, y);
}

template <class T>
struct vector
{
    static const int MINSIZE = 8;

    T *buf;
    int alen, ulen;

    vector() : buf(nullptr), alen(0), ulen(0)
    {
    }

    vector(const vector &v) : buf(nullptr), alen(0), ulen(0)
    {
        *this = v;
    }

    ~vector() { shrink(0); if(buf) delete[] (uchar *)buf; }

    vector<T> &operator=(const vector<T> &v)
    {
        shrink(0);
        if(v.length() > alen) growbuf(v.length());
        for(int i = 0; i < v.length(); i++) add(v[i]);
        return *this;
    }

    T &add(const T &x)
    {
        if(ulen==alen) growbuf(ulen+1);
        new (&buf[ulen]) T(x);
        return buf[ulen++];
    }

    T &add()
    {
        if(ulen==alen) growbuf(ulen+1);
        new (&buf[ulen]) T;
        return buf[ulen++];
    }

    bool inrange(size_t i) const { return i<size_t(ulen); }
    bool inrange(int i) const { return i>=0 && i<ulen; }

    T &pop() { return buf[--ulen]; }
    T &last() { return buf[ulen-1]; }
    void drop() { ulen--; buf[ulen].~T(); }
    bool empty() const { return ulen==0; }

    int capacity() const { return alen; }
    int length() const { return ulen; }
    T &operator[](int i) { return buf[i]; }
    const T &operator[](int i) const { return buf[i]; }

    T *disown() { T *r = buf; buf = nullptr; alen = ulen = 0; return r; }

    void shrink(int i) { if(isclass<T>::no) ulen = i; else while(ulen>i) drop(); }
    void setsize(int i) { ulen = i; }

    void deletecontents(int n = 0) { while(ulen > n) delete pop(); }
    void deletearrays(int n = 0) { while(ulen > n) delete[] pop(); }

    T *getbuf() { return buf; }
    const T *getbuf() const { return buf; }
    bool inbuf(const T *e) const { return e >= buf && e < &buf[ulen]; }

    template<class F>
    void sort(F fun, int i = 0, int n = -1)
    {
        quicksort(&buf[i], n < 0 ? ulen-i : n, fun);
    }

    void sort() { sort(sortless()); }
    void sortname() { sort(sortnameless()); }

    void growbuf(int sz)
    {
        int olen = alen;
        if(alen <= 0) alen = std::max(MINSIZE, sz);
        else while(alen < sz) alen += alen/2;
        if(alen <= olen) return;
        uchar *newbuf = new uchar[alen*sizeof(T)];
        if(olen > 0)
        {
            if(ulen > 0) memcpy(newbuf, (void *)buf, ulen*sizeof(T));
            delete[] (uchar *)buf;
        }
        buf = (T *)newbuf;
    }

    databuf<T> reserve(int sz)
    {
        if(alen-ulen < sz) growbuf(ulen+sz);
        return databuf<T>(&buf[ulen], sz);
    }

    void advance(int sz)
    {
        ulen += sz;
    }

    void addbuf(const databuf<T> &p)
    {
        advance(p.length());
    }

    void put(const T &v) { add(v); }

    void put(const T *v, int n)
    {
        databuf<T> buf = reserve(n);
        buf.put(v, n);
        addbuf(buf);
    }

    void remove(int i, int n)
    {
        for(int p = i+n; p<ulen; p++) buf[p-n] = buf[p];
        ulen -= n;
    }

    T remove(int i)
    {
        T e = buf[i];
        for(int p = i+1; p<ulen; p++) buf[p-1] = buf[p];
        ulen--;
        return e;
    }

    T removeunordered(int i)
    {
        T e = buf[i];
        ulen--;
        if(ulen>0) buf[i] = buf[ulen];
        return e;
    }

    template<class U>
    int find(const U &o)
    {
        for(int i = 0; i < ulen; ++i)
        {
            if(buf[i]==o)
            {
                return i;
            }
        }
        return -1;
    }

    void removeobj(const T &o)
    {
        for(int i = 0; i < int(ulen); ++i)
        {
            if(buf[i] == o)
            {
                int dst = i;
                for(int j = i+1; j < ulen; j++)
                {
                    if(!(buf[j] == o))
                    {
                        buf[dst++] = buf[j];
                    }
                }
                setsize(dst);
                break;
            }
        }
    }

    T &insert(int i, const T &e)
    {
        add(T());
        for(int p = ulen-1; p>i; p--) buf[p] = buf[p-1];
        buf[i] = e;
        return buf[i];
    }

    T *insert(int i, const T *e, int n)
    {
        if(alen-ulen < n) growbuf(ulen+n);
        for(int j = 0; j < n; ++j)
        {
            add(T());
        }
        for(int p = ulen-1; p>=i+n; p--)
        {
            buf[p] = buf[p-n];
        }
        for(int j = 0; j < n; ++j)
        {
            buf[i+j] = e[j];
        }
        return &buf[i];
    }

    template<class K>
    int htfind(const K &key)
    {
        for(int i = 0; i < int(ulen); ++i)
        {
            if(htcmp(key, buf[i]))
            {
                return i;
            }
        }
        return -1;
    }
};

template<class H, class E, class K, class T>
struct hashbase
{
    typedef E elemtype;
    typedef K keytype;
    typedef T datatype;

    enum { CHUNKSIZE = 64 };

    struct chain { E elem; chain *next; };
    struct chainchunk { chain chains[CHUNKSIZE]; chainchunk *next; };

    int size;
    int numelems;
    chain **chains;

    chainchunk *chunks;
    chain *unused;

    enum { DEFAULTSIZE = 1<<10 };

    hashbase(int size = DEFAULTSIZE)
      : size(size)
    {
        numelems = 0;
        chunks = nullptr;
        unused = nullptr;
        chains = new chain *[size];
        memset(chains, 0, size*sizeof(chain *));
    }

    ~hashbase()
    {
        DELETEA(chains);
        deletechunks();
    }

    chain *insert(uint h)
    {
        if(!unused)
        {
            chainchunk *chunk = new chainchunk;
            chunk->next = chunks;
            chunks = chunk;
            for(int i = 0; i < int(CHUNKSIZE-1); ++i)
            {
                chunk->chains[i].next = &chunk->chains[i+1];
            }
            chunk->chains[CHUNKSIZE-1].next = unused;
            unused = chunk->chains;
        }
        chain *c = unused;
        unused = unused->next;
        c->next = chains[h];
        chains[h] = c;
        numelems++;
        return c;
    }

    template<class U>
    T &insert(uint h, const U &key)
    {
        chain *c = insert(h);
        H::setkey(c->elem, key);
        return H::getdata(c->elem);
    }

    #define HTFIND(success, fail) \
        uint h = hthash(key)&(this->size-1); \
        for(chain *c = this->chains[h]; c; c = c->next) \
        { \
            if(htcmp(key, H::getkey(c->elem))) return success H::getdata(c->elem); \
        } \
        return (fail);

    template<class U>
    T *access(const U &key)
    {
        HTFIND(&, nullptr);
    }

    template<class U, class V>
    T &access(const U &key, const V &elem)
    {
        HTFIND( , insert(h, key) = elem);
    }

    template<class U>
    T &operator[](const U &key)
    {
        HTFIND( , insert(h, key));
    }

    template<class U>
    T &find(const U &key, T &notfound)
    {
        HTFIND( , notfound);
    }

    template<class U>
    const T &find(const U &key, const T &notfound)
    {
        HTFIND( , notfound);
    }

    template<class U>
    bool remove(const U &key)
    {
        uint h = hthash(key)&(size-1);
        for(chain **p = &chains[h], *c = chains[h]; c; p = &c->next, c = c->next)
        {
            if(htcmp(key, H::getkey(c->elem)))
            {
                *p = c->next;
                c->elem.~E();
                new (&c->elem) E;
                c->next = unused;
                unused = c;
                numelems--;
                return true;
            }
        }
        return false;
    }

    void deletechunks()
    {
        for(chainchunk *nextchunk; chunks; chunks = nextchunk)
        {
            nextchunk = chunks->next;
            delete chunks;
        }
    }

    void clear()
    {
        if(!numelems) return;
        memset(chains, 0, size*sizeof(chain *));
        numelems = 0;
        unused = nullptr;
        deletechunks();
    }

    static inline chain *enumnext(void *i) { return ((chain *)i)->next; }
    static inline K &enumkey(void *i) { return H::getkey(((chain *)i)->elem); }
    static inline T &enumdata(void *i) { return H::getdata(((chain *)i)->elem); }
};

template<class T>
inline void htrecycle(const T &) {}

template<class T>
struct hashset : hashbase<hashset<T>, T, T, T>
{
    typedef hashbase<hashset<T>, T, T, T> basetype;

    hashset(int size = basetype::DEFAULTSIZE) : basetype(size) {}

    static inline const T &getkey(const T &elem) { return elem; }
    static inline T &getdata(T &elem) { return elem; }
    template<class K> static inline void setkey(T &elem, const K &key) {}

    template<class V>
    T &add(const V &elem)
    {
        return basetype::access(elem, elem);
    }
};

template<class T>
struct hashnameset : hashbase<hashnameset<T>, T, const char *, T>
{
    typedef hashbase<hashnameset<T>, T, const char *, T> basetype;

    hashnameset(int size = basetype::DEFAULTSIZE) : basetype(size) {}

    template<class U>
    static inline const char *getkey(const U &elem) { return elem.name; }

    template<class U>
    static inline const char *getkey(U *elem) { return elem->name; }

    static inline T &getdata(T &elem) { return elem; }

    template<class K>
    static inline void setkey(T &elem, const K &key) {}

    template<class V>
    T &add(const V &elem)
    {
        return basetype::access(getkey(elem), elem);
    }
};

template<class K, class T>
struct hashtableentry
{
    K key;
    T data;
};

template<class K, class T>
inline void htrecycle(hashtableentry<K, T> &entry)
{
    htrecycle(entry.key);
    htrecycle(entry.data);
}

template<class K, class T>
struct hashtable : hashbase<hashtable<K, T>, hashtableentry<K, T>, K, T>
{
    typedef hashbase<hashtable<K, T>, hashtableentry<K, T>, K, T> basetype;
    typedef typename basetype::elemtype elemtype;

    hashtable(int size = basetype::DEFAULTSIZE) : basetype(size) {}

    static inline K &getkey(elemtype &elem) { return elem.key; }
    static inline T &getdata(elemtype &elem) { return elem.data; }
    template<class U> static inline void setkey(elemtype &elem, const U &key) { elem.key = key; }
};

#define ENUMERATE(ht,t,e,b)       for(int i = 0; i < int((ht).size); ++i) for(void *ec = (ht).chains[i]; ec;) { t &e = (ht).enumdata(ec); ec = (ht).enumnext(ec); b; }

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
inline int cube2uni(uchar c)
{
    extern const int cube2unichars[256];
    return cube2unichars[c];
}
inline uchar uni2cube(int c)
{
    extern const int uni2cubeoffsets[8];
    extern const uchar uni2cubechars[];
    return uint(c) <= 0x7FF ? uni2cubechars[uni2cubeoffsets[c>>8] + (c&0xFF)] : 0;
}

extern size_t decodeutf8(uchar *dst, size_t dstlen, const uchar *src, size_t srclen, size_t *carry = nullptr);
extern size_t encodeutf8(uchar *dstbuf, size_t dstlen, const uchar *srcbuf, size_t srclen, size_t *carry = nullptr);

extern string homedir;

extern char *makerelpath(const char *dir, const char *file, const char *prefix = nullptr, const char *cmd = nullptr);
extern char *path(char *s);
extern char *path(const char *s, bool copy);
extern const char *parentdir(const char *directory);
extern bool fileexists(const char *path, const char *mode);
extern bool createdir(const char *path);
extern size_t fixpackagedir(char *dir);
extern const char *sethomedir(const char *dir);
extern const char *findfile(const char *filename, const char *mode);
extern bool findzipfile(const char *filename);
extern stream *openrawfile(const char *filename, const char *mode);
extern stream *openzipfile(const char *filename, const char *mode);
extern stream *openfile(const char *filename, const char *mode);
extern stream *opentempfile(const char *filename, const char *mode);
extern stream *opengzfile(const char *filename, const char *mode, stream *file = nullptr, int level = Z_BEST_COMPRESSION);
extern stream *openutf8file(const char *filename, const char *mode, stream *file = nullptr);
extern char *loadfile(const char *fn, size_t *size, bool utf8 = true);
extern int listzipfiles(const char *dir, const char *ext, vector<char *> &files);

extern void putint(ucharbuf &p, int n);
extern void putint(packetbuf &p, int n);
extern void putint(vector<uchar> &p, int n);
extern int getint(ucharbuf &p);
extern void putuint(ucharbuf &p, int n);
extern void putuint(packetbuf &p, int n);
extern void putuint(vector<uchar> &p, int n);
extern int getuint(ucharbuf &p);
extern void putfloat(ucharbuf &p, float f);
extern void putfloat(packetbuf &p, float f);
extern void putfloat(vector<uchar> &p, float f);
extern float getfloat(ucharbuf &p);
extern void sendstring(const char *t, ucharbuf &p);
extern void sendstring(const char *t, packetbuf &p);
extern void sendstring(const char *t, vector<uchar> &p);
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

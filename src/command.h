
enum
{
    Id_Var,
    Id_FloatVar,
    Id_StringVar,
    ID_COMMAND,
    ID_CCOMMAND,
    ID_ALIAS
};

enum
{
    NO_OVERRIDE = INT_MAX,
    OVERRIDDEN = 0
};

enum
{
    IDF_PERSIST = 1<<0,
    IDF_OVERRIDE = 1<<1
};

struct identstack
{
    char *action;
    identstack *next;
};

union identval
{
    int i;      // Id_Var
    float f;    // Id_FloatVar
    char *s;    // Id_StringVar
};

union identvalptr
{
    int *i;   // Id_Var
    float *f; // Id_FloatVar
    char **s; // Id_StringVar
};

struct ident
{
    int type;           // one of ID_* above
    const char *name;
    union
    {
        int minval;    // Id_Var
        float minvalf; // Id_FloatVar
    };
    union
    {
        int maxval;    // Id_Var
        float maxvalf; // Id_FloatVar
    };
    int override;       // either NO_OVERRIDE, OVERRIDDEN, or value
    union
    {
        void (__cdecl *fun)(); // Id_Var, ID_COMMAND, ID_CCOMMAND
        identstack *stack;     // ID_ALIAS
    };
    union
    {
        const char *narg; // ID_COMMAND, ID_CCOMMAND
        char *action;     // ID_ALIAS
        identval val;     // Id_Var, Id_FloatVar, Id_StringVar
    };
    union
    {
        void *self;           // ID_COMMAND, ID_CCOMMAND
        char *isexecuting;    // ID_ALIAS
        identval overrideval; // Id_Var, Id_FloatVar, Id_StringVar
    };
    identvalptr storage; // Id_Var, Id_FloatVar, Id_StringVar
    int flags;

    ident() {}
    // Id_Var
    ident(int t, const char *n, int m, int c, int x, int *s, void *f = nullptr, int flags = 0)
        : type(t), name(n), minval(m), maxval(x), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.i = c; storage.i = s; }
    // Id_FloatVar
    ident(int t, const char *n, float m, float c, float x, float *s, void *f = nullptr, int flags = 0)
        : type(t), name(n), minvalf(m), maxvalf(x), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.f = c; storage.f = s; }
    // Id_StringVar
    ident(int t, const char *n, char *c, char **s, void *f = nullptr, int flags = 0)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.s = c; storage.s = s; }
    // ID_ALIAS
    ident(int t, const char *n, char *a, int flags)
        : type(t), name(n), override(NO_OVERRIDE), stack(nullptr), action(a), flags(flags) {}
    // ID_COMMAND, ID_CCOMMAND
    ident(int t, const char *n, const char *narg, void *f = nullptr, void *s = nullptr, int flags = 0)
        : type(t), name(n), fun((void (__cdecl *)(void))f), narg(narg), self(s), flags(flags) {}

    virtual ~ident() {}

    ident &operator=(const ident &o)
    {
        memcpy(this, &o, sizeof(ident));
        return *this;
    }        // force vtable copy, ugh

    virtual void changed() { if(fun) fun(); }
};
extern void addident(const char *name, ident *id);
extern void intret(int v);
extern const char *floatstr(float v);
extern void floatret(float v);
extern void result(const char *s);
void explodelist(const char *s, vector<char *> elems);

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#define COMMANDN(name, fun, nargs) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs)
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)

#define _VAR(name, global, min, cur, max, persist)  int global = variable(#name, min, cur, max, &global, nullptr, persist)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, 0)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, 0)
#define _VARF(name, global, min, cur, max, body, persist)  void var_##name(); int global = variable(#name, min, cur, max, &global, var_##name, persist); void var_##name() { body; }
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, 0)

#define _SVAR(name, global, cur, persist) char *global = svariable(#name, cur, &global, nullptr, persist)
#define SVAR(name, cur) _SVAR(name, name, cur, 0)
#define _SVARF(name, global, cur, body, persist) void var_##name(); char *global = svariable(#name, cur, &global, var_##name, persist); void var_##name() { body; }
#define SVARF(name, cur, body) _SVARF(name, name, cur, body, 0)

// new style macros, have the body inline, and allow binds to happen anywhere, even inside class constructors, and access the surrounding class
#define _COMMAND(idtype, tv, n, g, proto, b) \
    struct cmd_##n : ident \
    { \
        cmd_##n(void *self = nullptr) : ident(idtype, #n, g, (void *)run, self) \
        { \
            addident(name, this); \
        } \
        static void run proto { b; } \
    } icom_##n tv
#define ICOMMAND(n, g, proto, b) _COMMAND(ID_COMMAND, , n, g, proto, b)

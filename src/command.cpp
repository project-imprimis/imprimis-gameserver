// command.cpp: implements the parsing and execution of a tiny script language which
// is largely backwards compatible with the quake console language.

#include "engine.h"

#include <math.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <algorithm>

#include <enet/enet.h>
#include <zlib.h>

#include "tools.h"
#include "geom.h"
#include "command.h"

#include "iengine.h"

char *exchangestr(char *o, const char *n)
{
    delete[] o;
    return newstring(n);
}

typedef hashtable<const char *, ident> identtable;

identtable *idents = nullptr;        // contains ALL vars/commands/aliases

bool overrideidents = false,
     persistidents = true;

void clearstack(ident &id)
{
    identstack *stack = id.stack;
    while(stack)
    {
        delete[] stack->action;
        identstack *tmp = stack;
        stack = stack->next;
        delete tmp;
    }
    id.stack = nullptr;
}

void clear_command()
{
    ENUMERATE(*idents, ident, i, if(i.type==ID_ALIAS) { DELETEA(i.name); DELETEA(i.action); if(i.stack) clearstack(i); });
    if(idents) idents->clear();
}

void clearoverrides()
{
    ENUMERATE(*idents, ident, i,
        if(i.override!=NO_OVERRIDE)
        {
            switch(i.type)
            {
                case ID_ALIAS:
                    if(i.action[0])
                    {
                        if(i.action != i.isexecuting) delete[] i.action;
                        i.action = newstring("");
                    }
                    break;
                case Id_Var:
                    *i.storage.i = i.overrideval.i;
                    i.changed();
                    break;
                case Id_FloatVar:
                    *i.storage.f = i.overrideval.f;
                    i.changed();
                    break;
                case Id_StringVar:
                    delete[] *i.storage.s;
                    *i.storage.s = i.overrideval.s;
                    i.changed();
                    break;
            }
            i.override = NO_OVERRIDE;
        });
}

void pushident(ident &id, char *val)
{
    if(id.type != ID_ALIAS)
    {
        return;
    }
    identstack *stack = new identstack;
    stack->action = id.isexecuting==id.action ? newstring(id.action) : id.action;
    stack->next = id.stack;
    id.stack = stack;
    id.action = val;
}

void popident(ident &id)
{
    if(id.type != ID_ALIAS || !id.stack)
    {
        return;
    }
    if(id.action != id.isexecuting)
    {
        delete[] id.action;
    }
    identstack *stack = id.stack;
    id.action = stack->action;
    id.stack = stack->next;
    delete stack;
}

ident *newident(const char *name)
{
    ident *id = idents->access(name);
    if(!id)
    {
        ident init(ID_ALIAS, newstring(name), newstring(""), persistidents ? IDF_PERSIST : 0);
        id = &idents->access(init.name, init);
    }
    return id;
}

void pusha(const char *name, char *action)
{
    pushident(*newident(name), action);
}

void aliasa(const char *name, char *action)
{
    ident *b = idents->access(name);
    if(!b)
    {
        ident b(ID_ALIAS, newstring(name), action, persistidents ? IDF_PERSIST : 0);
        if(overrideidents)
        {
            b.override = OVERRIDDEN;
        }
        idents->access(b.name, b);
    }
    else if(b->type != ID_ALIAS)
    {
        conoutf(Console_Error, "cannot redefine builtin %s with an alias", name);
        delete[] action;
    }
    else
    {
        if(b->action != b->isexecuting)
        {
            delete[] b->action;
        }
        b->action = action;
        if(overrideidents) b->override = OVERRIDDEN;
        else
        {
            if(b->override != NO_OVERRIDE)
            {
                b->override = NO_OVERRIDE;
            }
            if(persistidents)
            {
                if(!(b->flags & IDF_PERSIST))
                {
                    b->flags |= IDF_PERSIST;
                }
            }
            else if(b->flags & IDF_PERSIST)
            {
                b->flags &= ~IDF_PERSIST;
            }
        }
    }
}

// variables and commands are registered through globals, see cube.h

int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), int flags)
{
    if(!idents)
    {
        idents = new identtable;
    }
    ident v(Id_Var, name, min, cur, max, storage, (void *)fun, flags);
    idents->access(name, v);
    return cur;
}

float fvariable(const char *name, float min, float cur, float max, float *storage, void (*fun)(), int flags)
{
    if(!idents)
    {
        idents = new identtable;
    }
    ident v(Id_FloatVar, name, min, cur, max, storage, (void *)fun, flags);
    idents->access(name, v);
    return cur;
}

char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), int flags)
{
    if(!idents)
    {
        idents = new identtable;
    }
    ident v(Id_StringVar, name, newstring(cur), storage, (void *)fun, flags);
    idents->access(name, v);
    return v.val.s;
}

#define _GETVAR(id, vartype, name, retval) \
    ident *id = idents->access(name); \
    if(!id || id->type!=vartype) \
    { \
        return retval; \
    }
#define GETVAR(id, name, retval) _GETVAR(id, Id_Var, name, retval)
void setvar(const char *name, int i, bool dofunc)
{
    GETVAR(id, name, );
    *id->storage.i = clamp(i, id->minval, id->maxval);
    if(dofunc)
    {
        id->changed();
    }
}
void setfvar(const char *name, float f, bool dofunc)
{
    _GETVAR(id, Id_FloatVar, name, );
    *id->storage.f = clamp(f, id->minvalf, id->maxvalf);
    if(dofunc)
    {
        id->changed();
    }
}
void setsvar(const char *name, const char *str, bool dofunc)
{
    _GETVAR(id, Id_StringVar, name, );
    *id->storage.s = exchangestr(*id->storage.s, str);
    if(dofunc)
    {
        id->changed();
    }
}
int getvar(const char *name)
{
    GETVAR(id, name, 0);
    return *id->storage.i;
}
int getvarmin(const char *name)
{
    GETVAR(id, name, 0);
    return id->minval;
}
int getvarmax(const char *name)
{
    GETVAR(id, name, 0);
    return id->maxval;
}
bool identexists(const char *name)
{
    return idents->access(name)!=nullptr;
}

ident *getident(const char *name)
{
    return idents->access(name);
}

void touchvar(const char *name)
{
    ident *id = idents->access(name);
    if(id) switch(id->type)
    {
        case Id_Var:
        case Id_FloatVar:
        case Id_StringVar:
        {
            id->changed();
            break;
        }
    }
}

const char *getalias(const char *name)
{
    ident *i = idents->access(name);
    return i && i->type==ID_ALIAS ? i->action : "";
}

bool addcommand(const char *name, void (*fun)(), const char *narg)
{
    if(!idents)
    {
        idents = new identtable;
    }
    ident c(ID_COMMAND, name, narg, (void *)fun);
    idents->access(name, c);
    return false;
}

void addident(const char *name, ident *id)
{
    if(!idents)
    {
        idents = new identtable;
    }
    idents->access(name, *id);
}

static vector<vector<char> *> wordbufs;
static int bufnest = 0;

char *parseexp(const char *&p, int right);

void parsemacro(const char *&p, int level, vector<char> &wordbuf)
{
    int escape = 1;
    while(*p=='@') p++, escape++;
    if(level > escape)
    {
        while(escape--)
        {
            wordbuf.add('@');
        }
        return;
    }
    if(*p=='(')
    {
        char *ret = parseexp(p, ')');
        if(ret)
        {
            for(char *sub = ret; *sub; )
            {
                wordbuf.add(*sub++);
            }
            delete[] ret;
        }
        return;
    }
    static vector<char> ident;
    while(isalnum(*p) || *p=='_')
    {
        ident.add(*p++);
    }
    ident.add(0);
    const char *alias = getalias(ident.getbuf());
    while(*alias)
    {
        wordbuf.add(*alias++);
    }
}

char *parseexp(const char *&p, int right)          // parse any nested set of () or []
{
    if(bufnest++>=wordbufs.length())
    {
        wordbufs.add(new vector<char>);
    }
    vector<char> &wordbuf = *wordbufs[bufnest-1];
    int left = *p++;
    for(int brak = 1; brak; )
    {
        int c = *p++;
        if(c=='\r')
        {
            continue;               // hack
        }
        if(left=='[' && c=='@')
        {
            parsemacro(p, brak, wordbuf);
            continue;
        }
        if(c=='\"')
        {
            wordbuf.add(c);
            const char *end = p+strcspn(p, "\"\r\n\0");
            while(p < end)
            {
                wordbuf.add(*p++);
            }
            if(*p=='\"')
            {
                wordbuf.add(*p++);
            }
            continue;
        }
        if(c=='/' && *p=='/')
        {
            p += strcspn(p, "\n\0");
            continue;
        }
        if(c==left)
        {
            brak++;
        }
        else if(c==right)
        {
            brak--;
        }
        else if(!c)
        {
            p--;
            conoutf(Console_Error, "missing \"%c\"", right);
            wordbuf.setsize(0);
            bufnest--;
            return nullptr;
        }
        wordbuf.add(c);
    }
    wordbuf.pop();
    char *s;
    if(left=='(')
    {
        wordbuf.add(0);
        char *ret = executeret(wordbuf.getbuf());                    // evaluate () exps directly, and substitute result
        wordbuf.pop();
        s = ret ? ret : newstring("");
    }
    else
    {
        s = newstring(wordbuf.getbuf(), wordbuf.length());
    }
    wordbuf.setsize(0);
    bufnest--;
    return s;
}

char *lookup(char *n)                           // find value of ident referenced with $ in exp
{
    ident *id = idents->access(n+1);
    if(id) switch(id->type)
    {
        case Id_Var:
        {
            s_sprintfd(t)("%d", *id->storage.i);
            return exchangestr(n, t);
        }
        case Id_FloatVar:
        {
            return exchangestr(n, floatstr(*id->storage.f));
        }
        case Id_StringVar:
        {
            return exchangestr(n, *id->storage.s);
        }
        case ID_ALIAS:
        {
            return exchangestr(n, id->action);
        }
    }
    conoutf(Console_Error, "unknown alias lookup: %s", n+1);
    return n;
}

char *parseword(const char *&p, int arg, int &infix)                       // parse single argument, including expressions
{
    for(;;)
    {
        p += strspn(p, " \t\r");
        if(p[0]!='/' || p[1]!='/')
        {
            break;
        }
        p += strcspn(p, "\n\0");
    }
    if(*p=='\"')
    {
        p++;
        const char *word = p;
        p += strcspn(p, "\"\r\n\0");
        char *s = newstring(word, p-word);
        if(*p=='\"')
        {
            p++;
        }
        return s;
    }
    if(*p=='(')
    {
        return parseexp(p, ')');
    }
    if(*p=='[')
    {
        return parseexp(p, ']');
    }
    const char *word = p;
    for(;;)
    {
        p += strcspn(p, "/; \t\r\n\0");
        if(p[0]!='/' || p[1]=='/')
        {
            break;
        }
        else if(p[1]=='\0')
        {
            p++;
            break;
        }
        p += 2;
    }
    if(p-word==0)
    {
        return nullptr;
    }
    if(arg==1 && p-word==1)
    {
        switch(*word)
        {
            case '=':
            {
                infix = *word;
                break;
            }
        }
    }
    char *s = newstring(word, p-word);
    if(*s=='$')
    {
        return lookup(s);               // substitute variables
    }
    return s;
}

char *conc(char **w, int n, bool space)
{
    int len = space ? std::max(n-1, 0) : 0;
    for(int j = 0; j < n; j++)
    {
        len += (int)strlen(w[j]);
    }
    char *r = newstring("", len);
    for(int i = 0; i < n; i++)
    {
        strcat(r, w[i]);  // make string-list out of all arguments
        if(i==n-1)
        {
            break;
        }
        if(space)
        {
            strcat(r, " ");
        }
    }
    return r;
}

VARN(numargs, _numargs, 0, 0, 25);

#define PARSEINT(s) strtol((s), nullptr, 0)

char *commandret = nullptr;

char *executeret(const char *p)               // all evaluation happens here, recursively
{
    const int MAXWORDS = 25;                    // limit, remove
    char *w[MAXWORDS];
    char *retval = nullptr;
    #define setretval(v) { char *rv = v; if(rv) retval = rv; }
    for(bool cont = true; cont;)                // for each ; seperated statement
    {
        int numargs = MAXWORDS, infix = 0;
        // collect all argument values
        for(int i = 0; i < MAXWORDS; i++)
        {
            w[i] = (char *)"";
            if(i>numargs)
            {
                continue;
            }
            char *s = parseword(p, i, infix);   // parse and evaluate exps
            if(s)
            {
                w[i] = s;
            }
            else
            {
                numargs = i;
            }
        }

        p += strcspn(p, ";\n\0");
        cont = *p++!=0;                         // more statements if this isn't the end of the string
        char *c = w[0];
        if(!*c)
        {
            continue;                       // empty statement
        }
        DELETEA(retval);
        if(infix)
        {
            switch(infix)
            {
                case '=':
                    aliasa(c, numargs>2 ? w[2] : newstring(""));
                    w[2] = nullptr;
                    break;
            }
        }
        else
        {
            ident *id = idents->access(c);
            if(!id)
            {
                if(!isdigit(*c) && ((*c!='+' && *c!='-' && *c!='.') || !isdigit(c[1])))
                {
                    conoutf(Console_Error, "unknown command: %s", c);
                }
                setretval(newstring(c));
            }
            else switch(id->type)
            {
                case ID_CCOMMAND:
                case ID_COMMAND:                     // game defined commands
                {
                    void *v[MAXWORDS];
                    union
                    {
                        int i;
                        float f;
                    } nstor[MAXWORDS];
                    int n = 0,
                        wn = 0;
                    char *cargs = nullptr;
                    if(id->type==ID_CCOMMAND) v[n++] = id->self;
                    for(const char *a = id->narg; *a; a++)
                    {
                        switch(*a)
                        {
                            case 's':                                 v[n] = w[++wn];     n++; break;
                            case 'i': nstor[n].i = PARSEINT(w[++wn]); v[n] = &nstor[n].i; n++; break;
                            case 'f': nstor[n].f = atof(w[++wn]);     v[n] = &nstor[n].f; n++; break;
                            case 'V': v[n++] = w+1; nstor[n].i = numargs-1; v[n] = &nstor[n].i; n++; break;
                            case 'C': if(!cargs) cargs = conc(w+1, numargs-1, true); v[n++] = cargs; break;
                            default: fatal("builtin declared with illegal type");
                        }
                    }
                    switch(n)
                    {
                        case 0: ((void (__cdecl *)()                                      )id->fun)();                             break;
                        case 1: ((void (__cdecl *)(void *)                                )id->fun)(v[0]);                         break;
                        case 2: ((void (__cdecl *)(void *, void *)                        )id->fun)(v[0], v[1]);                   break;
                        case 3: ((void (__cdecl *)(void *, void *, void *)                )id->fun)(v[0], v[1], v[2]);             break;
                        case 4: ((void (__cdecl *)(void *, void *, void *, void *)        )id->fun)(v[0], v[1], v[2], v[3]);       break;
                        case 5: ((void (__cdecl *)(void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4]); break;
                        case 6: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5]); break;
                        case 7: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6]); break;
                        case 8: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]); break;
                        default: fatal("builtin declared with too many args (use V?)");
                    }
                    if(cargs) delete[] cargs;
                    setretval(commandret);
                    commandret = nullptr;
                    break;
                }

                case Id_Var:                        // game defined variables
                    if(numargs <= 1) conoutf("%s = %d", c, *id->storage.i);      // var with no value just prints its current value
                    else if(id->minval>id->maxval) conoutf(Console_Error, "variable %s is read-only", id->name);
                    else
                    {
                        #define OVERRIDEVAR(saveval, resetval) \
                            if(overrideidents || id->flags&IDF_OVERRIDE) \
                            { \
                                if(id->flags&IDF_PERSIST) \
                                { \
                                    conoutf(Console_Error, "cannot override persistent variable %s", id->name); \
                                    break; \
                                } \
                                if(id->override==NO_OVERRIDE) { saveval; id->override = OVERRIDDEN; } \
                            } \
                            else if(id->override!=NO_OVERRIDE) { resetval; id->override = NO_OVERRIDE; }
                        OVERRIDEVAR(id->overrideval.i = *id->storage.i, )
                        int i1 = PARSEINT(w[1]);
                        if(i1<id->minval || i1>id->maxval)
                        {
                            i1 = i1<id->minval ? id->minval : id->maxval;                // clamp to valid range
                            conoutf(Console_Error, "valid range for %s is %d..%d", id->name, id->minval, id->maxval);
                        }
                        *id->storage.i = i1;
                        id->changed();                                             // call trigger function if available
                    }
                    break;

                case Id_FloatVar:
                    if(numargs <= 1)
                    {
                        conoutf("%s = %s", c, floatstr(*id->storage.f));
                    }
                    else if(id->minvalf>id->maxvalf)
                    {
                        conoutf(Console_Error, "variable %s is read-only", id->name);
                    }
                    else
                    {
                        OVERRIDEVAR(id->overrideval.f = *id->storage.f, );
                        float f1 = atof(w[1]);
                        if(f1<id->minvalf || f1>id->maxvalf)
                        {
                            f1 = f1<id->minvalf ? id->minvalf : id->maxvalf;                // clamp to valid range
                            conoutf(Console_Error, "valid range for %s is %s..%s", id->name, floatstr(id->minvalf), floatstr(id->maxvalf));
                        }
                        *id->storage.f = f1;
                        id->changed();
                    }
                    break;

                case Id_StringVar:
                    if(numargs <= 1)
                    {
                        conoutf(strchr(*id->storage.s, '"') ? "%s = [%s]" : "%s = \"%s\"", c, *id->storage.s);
                    }
                    else
                    {
                        OVERRIDEVAR(id->overrideval.s = *id->storage.s, delete[] id->overrideval.s);
                        *id->storage.s = newstring(w[1]);
                        id->changed();
                    }
                    break;

                case ID_ALIAS:                              // alias, also used as functions and (global) variables
                {
                    static vector<ident *> argids;
                    for(int i = 1; i<numargs; i++)
                    {
                        if(i > argids.length())
                        {
                            s_sprintfd(argname)("arg%d", i);
                            argids.add(newident(argname));
                        }
                        pushident(*argids[i-1], w[i]); // set any arguments as (global) arg values so functions can access them
                    }
                    _numargs = numargs-1;
                    bool wasoverriding = overrideidents;
                    if(id->override!=NO_OVERRIDE)
                    {
                        overrideidents = true;
                    }
                    char *wasexecuting = id->isexecuting;
                    id->isexecuting = id->action;
                    setretval(executeret(id->action));
                    if(id->isexecuting != id->action && id->isexecuting != wasexecuting)
                    {
                        delete[] id->isexecuting;
                    }
                    id->isexecuting = wasexecuting;
                    overrideidents = wasoverriding;
                    for(int i = 1; i<numargs; i++)
                    {
                        popident(*argids[i-1]);
                    }
                    continue;
                }
            }
        }
        for(int j = 0; j < numargs; j++) if(w[j]) delete[] w[j];
    }
    return retval;
}

int execute(const char *p)
{
    char *ret = executeret(p);
    int i = 0;
    if(ret)
    {
        i = PARSEINT(ret);
        delete[] ret;
    }
    return i;
}

bool execfile(const char *cfgfile)
{
    string s;
    s_strcpy(s, cfgfile);
    char *buf = loadfile(path(s), nullptr);
    if(!buf)
    {
        return false;
    }
    execute(buf);
    delete[] buf;
    return true;
}

void exec(const char *cfgfile)
{
    if(!execfile(cfgfile))
    {
        conoutf(Console_Error, "could not read \"%s\"", cfgfile);
    }
}

void intret(int v) { s_sprintfd(b)("%d", v); commandret = newstring(b); }

const char *floatstr(float v)
{
    static int n = 0;
    static string t[3];
    n = (n + 1)%3;
    s_sprintf(t[n])(v==int(v) ? "%.1f" : "%.7g", v);
    return t[n];
}

void floatret(float v)
{
    commandret = newstring(floatstr(v));
}

#define WHITESPACESKIP s += strspn(s, "\n\t ")
#define ELEMENTSKIP *s=='"' ? (++s, s += strcspn(s, "\"\n\0"), s += *s=='"') : s += strcspn(s, "\n\t \0")

void explodelist(const char *s, vector<char *> elems)
{
    WHITESPACESKIP;
    while(*s)
    {
        const char *elem = s;
        ELEMENTSKIP;
        elems.add(*elem=='"' ? newstring(elem+1, s-elem-(s[-1]=='"' ? 2 : 1)) : newstring(elem, s-elem));
        WHITESPACESKIP;
    }
}


COMMAND(exec, "s");
ICOMMAND(echo, "C", (char *s), conoutf(Console_Echo, "\f1%s", s));

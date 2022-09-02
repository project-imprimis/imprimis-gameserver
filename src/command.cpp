// command.cpp: implements the parsing and execution of a tiny script language which
// allows server values to be set by configurable script

#include "engine.h"

#include <cmath>

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

identtable *idents = nullptr;        // contains ALL vars/commands

bool overrideidents = false,
     persistidents = true;

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

char *lookup(char *n)                           // find value of ident referenced with $ in exp
{
    ident *id = idents->access(n+1);
    if(id) switch(id->type)
    {
        case Id_Var:
        {
            s_sprintfd(t)("%d\n", *id->storage.i);
            return exchangestr(n, t);
        }
        case Id_StringVar:
        {
            return exchangestr(n, *id->storage.s);
        }
    }
    printf("unknown alias lookup: %s\n", n+1);
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
        if(!infix)
        {
            ident *id = idents->access(c);
            if(!id)
            {
                if(!isdigit(*c) && ((*c!='+' && *c!='-' && *c!='.') || !isdigit(c[1])))
                {
                    printf("unknown command: %s\n", c);
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
                    if(numargs <= 1) printf("%s = %d\n", c, *id->storage.i);      // var with no value just prints its current value
                    else if(id->minval>id->maxval) printf("variable %s is read-only\n", id->name);
                    else
                    {
                        #define OVERRIDEVAR(saveval, resetval) \
                            if(overrideidents || id->flags&IDF_OVERRIDE) \
                            { \
                                if(id->flags&IDF_PERSIST) \
                                { \
                                    printf("cannot override persistent variable %s\n", id->name); \
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
                            printf("valid range for %s is %d..%d\n", id->name, id->minval, id->maxval);
                        }
                        *id->storage.i = i1;
                        id->changed();                                             // call trigger function if available
                    }
                    break;

                case Id_StringVar:
                    if(numargs <= 1)
                    {
                        printf(strchr(*id->storage.s, '"') ? "%s = [%s]\n" : "%s = \"%s\"\n", c, *id->storage.s);
                    }
                    else
                    {
                        OVERRIDEVAR(id->overrideval.s = *id->storage.s, delete[] id->overrideval.s);
                        *id->storage.s = newstring(w[1]);
                        id->changed();
                    }
                    break;
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
        printf("could not read \"%s\"\n", cfgfile);
    }
}

void intret(int v) { s_sprintfd(b)("%d", v); commandret = newstring(b); }

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

void echocmd(char *s)
{
    printf("\f1%s\n", s);
}
COMMANDN(echo, echocmd, "s");

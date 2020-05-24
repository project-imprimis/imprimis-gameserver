// worldio.cpp: loading & saving of maps and savegames

#include "engine.h"

void validmapname(char *dst, const char *src, const char *prefix = NULL, const char *alt = "untitled", size_t maxlen = 100)
{
    if(prefix) while(*prefix) *dst++ = *prefix++;
    const char *start = dst;
    if(src)
    {
        for(int i = 0; i < int(maxlen); ++i)
        {
            char c = *src++;
            if(iscubealnum(c) || c == '_' || c == '-' || c == '/' || c == '\\') *dst++ = c;
            else break;
        }
    }

    if(dst > start)
    {
        *dst = '\0';
    }
    else if(dst != alt)
    {
        copystring(dst, alt, maxlen);
    }
}

void fixmapname(char *name)
{
    validmapname(name, name, NULL, "");
}

static void fixent(entity &e, int version)
{
    if(version <= 0)
    {
        if(e.type >= EngineEnt_Decal) e.type++;
    }
}

static bool loadmapheader(stream *f, const char *ogzname, mapheader &hdr, octaheader &ohdr)
{
    if(f->read(&hdr, 3*sizeof(int)) != 3*sizeof(int)) { conoutf(Console_Error, "map %s has malformatted header", ogzname); return false; }
    LIL_ENDIAN_SWAP(&hdr.version, 2);

    if(!memcmp(hdr.magic, "TMAP", 4))
    {
        if(hdr.version>MAPVERSION) { conoutf(Console_Error, "map %s requires a newer version of Tesseract", ogzname); return false; }
        if(f->read(&hdr.worldsize, 6*sizeof(int)) != 6*sizeof(int)) { conoutf(Console_Error, "map %s has malformatted header", ogzname); return false; }
        LIL_ENDIAN_SWAP(&hdr.worldsize, 6);
        if(hdr.worldsize <= 0|| hdr.numents < 0) { conoutf(Console_Error, "map %s has malformatted header", ogzname); return false; }
    }
    else if(!memcmp(hdr.magic, "OCTA", 4))
    {
        if(hdr.version!=OCTAVERSION) { conoutf(Console_Error, "map %s uses an unsupported map format version", ogzname); return false; }
        if(f->read(&ohdr.worldsize, 7*sizeof(int)) != 7*sizeof(int)) { conoutf(Console_Error, "map %s has malformatted header", ogzname); return false; }
        LIL_ENDIAN_SWAP(&ohdr.worldsize, 7);
        if(ohdr.worldsize <= 0|| ohdr.numents < 0) { conoutf(Console_Error, "map %s has malformatted header", ogzname); return false; }
        memcpy(hdr.magic, "TMAP", 4);
        hdr.version = 0;
        hdr.headersize = sizeof(hdr);
        hdr.worldsize = ohdr.worldsize;
        hdr.numents = ohdr.numents;
        hdr.numvars = ohdr.numvars;
        hdr.numvslots = ohdr.numvslots;
    }
    else { conoutf(Console_Error, "map %s uses an unsupported map type", ogzname); return false; }

    return true;
}

bool loadents(const char *fname, vector<entity> &ents, uint *crc)
{
    string name;
    validmapname(name, fname);
    DEF_FORMAT_STRING(ogzname, "media/map/%s.ogz", name);
    path(ogzname);
    stream *f = opengzfile(ogzname, "rb");
    if(!f)
    {
        return false;
    }

    mapheader hdr;
    octaheader ohdr;
    if(!loadmapheader(f, ogzname, hdr, ohdr))
    {
        delete f;
        return false;
    }

    for(int i = 0; i < hdr.numvars; ++i)
    {
        int type = f->getchar(), ilen = f->getlil<ushort>();
        f->seek(ilen, SEEK_CUR);
        switch(type)
        {
            case Id_Var: f->getlil<int>(); break;
            case Id_FloatVar: f->getlil<float>(); break;
            case Id_StringVar: { int slen = f->getlil<ushort>(); f->seek(slen, SEEK_CUR); break; }
        }
    }

    string gametype;
    bool samegame = true;
    int len = f->getchar();
    if(len >= 0)
    {
        f->read(gametype, len+1);
    }
    gametype[max(len, 0)] = '\0';
    if(strcmp(gametype, game::gameident()))
    {
        samegame = false;
        conoutf(Console_Warn, "WARNING: loading map from %s game, ignoring entities except for lights/mapmodels", gametype);
    }
    int eif = f->getlil<ushort>();
    int extrasize = f->getlil<ushort>();
    f->seek(extrasize, SEEK_CUR);

    ushort nummru = f->getlil<ushort>();
    f->seek(nummru*sizeof(ushort), SEEK_CUR);

    for(int i = 0; i < min(hdr.numents, MAXENTS); ++i)
    {
        entity &e = ents.add();
        f->read(&e, sizeof(entity));
        LIL_ENDIAN_SWAP(&e.o.x, 3);
        LIL_ENDIAN_SWAP(&e.attr1, 5);
        fixent(e, hdr.version);
        if(eif > 0) f->seek(eif, SEEK_CUR);
        if(samegame)
        {
            entities::readent(e, NULL, hdr.version);
        }
        else if(e.type>=EngineEnt_GameSpecific)
        {
            ents.pop();
            continue;
        }
    }

    if(crc)
    {
        f->seek(0, SEEK_END);
        *crc = f->getcrc();
    }

    delete f;

    return true;
}

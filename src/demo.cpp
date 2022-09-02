/* demo.cpp: game recording via packet transcription
 *
 * demos are a form of packet log which can be used to recreate a game from
 * the recording client's point of view; in a demo every net packet is recorded
 * and transcribed to a file which can later be read in
 */
#include "engine.h"

#include <cmath>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <algorithm>

#include <enet/enet.h>
#include <zlib.h>

#include "tools.h"
#include "geom.h"
#include "command.h"

#include "iengine.h"
#include "igame.h"

#include "cserver.h"
#include "game.h"
#include "mapcontrol.h"

namespace server
{

    int nextplayback = 0,
        demomillis = 0;

    VAR(maxdemos, 0, 5, 25);
    VAR(maxdemosize, 0, 16, 31);
    VAR(restrictdemos, 0, 1, 1);

    struct demofile
    {
        string info;
        uchar *data;
        int len;
    };

    std::vector<demofile> demos;

    bool demonextmatch = false;
    stream *demotmp = nullptr,
           *demorecord = nullptr,
           *demoplayback = nullptr;

    void listdemos(int cn)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putint(p, NetMsg_SendDemoList);
        putint(p, demos.size());
        for(uint i = 0; i < demos.size(); i++)
        {
            sendstring(demos[i].info, p);
        }
        sendpacket(cn, 1, p.finalize());
    }

    void cleardemos(int n)
    {
        if(!n)
        {
            for(uint i = 0; i < demos.size(); i++)
            {
                delete[] demos[i].data;
            }
            demos.clear();
            sendservmsg("cleared all demos");
        }
        else if(demos.size() > n-1)
        {
            delete[] demos[n-1].data;
            demos.erase(demos.begin() + n-1);
            sendservmsgf("cleared demo %d", n);
        }
    }

    void freegetmap(ENetPacket *packet)
    {
        for(int i = 0; i < clients.length(); i++)
        {
            clientinfo *ci = clients[i];
            if(ci->getmap == packet)
            {
                ci->getmap = nullptr;
            }
        }
    }

    static void freegetdemo(ENetPacket *packet)
    {
        for(int i = 0; i < clients.length(); i++)
        {
            clientinfo *ci = clients[i];
            if(ci->getdemo == packet)
            {
                ci->getdemo = nullptr;
            }
        }
    }

    void senddemo(clientinfo *ci, int num)
    {
        if(ci->getdemo)
        {
            return;
        }
        if(!num)
        {
            num = demos.size();
        }
        if(!(demos.size() > num-1))
        {
            return;
        }
        demofile &d = demos[num-1];
        if((ci->getdemo = sendf(ci->clientnum, 2, "rim", NetMsg_SendDemo, d.len, d.data)))
        {
            ci->getdemo->freeCallback = freegetdemo;
        }
    }

    void enddemoplayback()
    {
        if(!demoplayback)
        {
            return;
        }
        DELETEP(demoplayback);

        for(int i = 0; i < clients.length(); i++)
        {
            sendf(clients[i]->clientnum, 1, "ri3", NetMsg_DemoPlayback, 0, clients[i]->clientnum);
        }

        sendservmsg("demo playback finished");

        for(int i = 0; i < clients.length(); i++)
        {
            sendwelcome(clients[i]);
        }
    }

    void setupdemoplayback()
    {
        if(demoplayback) return;
        demoheader hdr;
        string msg;
        msg[0] = '\0';
        DEF_FORMAT_STRING(file, "%s.dmo", smapname);
        demoplayback = openfile(file, "rb");
        if(!demoplayback) formatstring(msg, "could not read demo \"%s\"", file);
        else if(demoplayback->read(&hdr, sizeof(demoheader))!=sizeof(demoheader) || memcmp(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic)))
            formatstring(msg, "\"%s\" is not a demo file", file);
        else
        {
            if(hdr.version!=DEMO_VERSION) formatstring(msg, "demo \"%s\" requires an %s version of Tesseract", file, hdr.version<DEMO_VERSION ? "older" : "newer");
            else if(hdr.protocol!=PROTOCOL_VERSION) formatstring(msg, "demo \"%s\" requires an %s version of Tesseract", file, hdr.protocol<PROTOCOL_VERSION ? "older" : "newer");
        }
        if(msg[0])
        {
            DELETEP(demoplayback);
            sendservmsg(msg);
            return;
        }

        sendservmsgf("playing demo \"%s\"", file);

        demomillis = 0;
        sendf(-1, 1, "ri3", NetMsg_DemoPlayback, 1, -1);

        if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
        {
            enddemoplayback();
            return;
        }
    }

    void readdemo()
    {
        if(!demoplayback)
        {
            return;
        }
        demomillis += curtime;
        while(demomillis>=nextplayback)
        {
            int chan, len;
            if(demoplayback->read(&chan, sizeof(chan))!=sizeof(chan) ||
               demoplayback->read(&len, sizeof(len))!=sizeof(len))
            {
                enddemoplayback();
                return;
            }
            ENetPacket *packet = enet_packet_create(nullptr, len+1, 0);
            if(!packet || demoplayback->read(packet->data+1, len)!=size_t(len))
            {
                if(packet)
                {
                    enet_packet_destroy(packet);
                }
                enddemoplayback();
                return;
            }
            packet->data[0] = NetMsg_DemoPacket;
            sendpacket(-1, chan, packet);
            if(!packet->referenceCount)
            {
                enet_packet_destroy(packet);
            }
            if(!demoplayback)
            {
                break;
            }
            if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
            {
                enddemoplayback();
                return;
            }
        }
    }

    void prunedemos(int extra = 0)
    {
        int n = clamp(static_cast<int>(demos.size()) + extra - maxdemos, 0, static_cast<int>(demos.size()));
        if(n <= 0)
        {
            return;
        }
        for(int i = 0; i < n; ++i)
        {
            delete[] demos[i].data;
        }
        demos.erase(demos.begin(), demos.begin() + n);
    }

    void adddemo()
    {
        if(!demotmp)
        {
            return;
        }
        int len = static_cast<int>(std::min(demotmp->size(), stream::offset((maxdemosize<<20) + 0x10000)));
        demos.emplace_back();
        demofile &d = demos.back();
        time_t t = time(nullptr);
        char *timestr = ctime(&t),
             *trim = timestr + strlen(timestr);
        while(trim>timestr && iscubespace(*--trim))
        {
            *trim = '\0';
        }
        formatstring(d.info, "%s: %s, %s, %.2f%s", timestr, modeprettyname(gamemode), smapname, len > 1024*1024 ? len/(1024*1024.f) : len/1024.0f, len > 1024*1024 ? "MB" : "kB");
        sendservmsgf("demo \"%s\" recorded", d.info);
        d.data = new uchar[len];
        d.len = len;
        demotmp->seek(0, SEEK_SET);
        demotmp->read(d.data, len);
        DELETEP(demotmp);
    }

    void enddemorecord()
    {
        if(!demorecord)
        {
            return;
        }

        DELETEP(demorecord);

        if(!demotmp)
        {
            return;
        }
        if(!maxdemos || !maxdemosize)
        {
            DELETEP(demotmp);
            return;
        }

        prunedemos(1);
        adddemo();
    }


    void stopdemo()
    {
        if(modecheck(gamemode, Mode_Demo))
        {
            enddemoplayback();
        }
        else
        {
            enddemorecord();
        }
    }

    void writedemo(int chan, void *data, int len)
    {
        if(!demorecord)
        {
            return;
        }
        int stamp[3] = { gamemillis, chan, len };
        demorecord->write(stamp, sizeof(stamp));
        demorecord->write(data, len);
        if(demorecord->rawtell() >= (maxdemosize<<20))
        {
            enddemorecord();
        }
    }

    void setupdemorecord()
    {
        if(modecheck(gamemode, Mode_LocalOnly) || modecheck(gamemode, Mode_Edit))
        {
            return;
        }

        stream *f = openfile("demorecord", "wb");
        if(!f)
        {
            DELETEP(demotmp);
            return;
        }

        sendservmsg("recording demo");

        demorecord = f;

        demoheader hdr;
        memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
        hdr.version = DEMO_VERSION;
        hdr.protocol = PROTOCOL_VERSION;
        demorecord->write(&hdr, sizeof(demoheader));

        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        welcomepacket(p, nullptr);
        writedemo(1, p.buf, p.len);
    }
}

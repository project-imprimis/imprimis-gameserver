// server.cpp: little more than enhanced multicaster
// runs dedicated or as client coroutine

#include "engine.h"

#include <cmath>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <algorithm>
#include <queue>

#include <enet/enet.h>

#include <zlib.h>

#include "tools.h"
#include "geom.h"
#include "command.h"

#include "iengine.h"
#include "igame.h"
#include "game.h"
#include "mapcontrol.h"

constexpr int DEFAULTCLIENTS = 8;

enum
{
    ServerClient_Empty,
    ServerClient_Local,
    ServerClient_Remote
};

struct client                   // server side version of "dynent" type
{
    int type;
    int num;
    ENetPeer *peer;
    string hostname;
    void *info;
};

std::vector<client *> clients;

ENetHost *serverhost = nullptr;
ENetSocket lansock = ENET_SOCKET_NULL;

int localclients = 0,
    nonlocalclients = 0;

bool hasnonlocalclients()
{
    return nonlocalclients!=0;
}

client &addclient(int type)
{
    client *c = nullptr;
    for(uint i = 0; i < clients.size(); i++)
    {
        if(clients[i]->type==ServerClient_Empty)
        {
            c = clients[i];
            break;
        }
    }
    if(!c)
    {
        c = new client;
        c->num = static_cast<int>(clients.size());
        clients.push_back(c);
    }
    c->info = server::newclientinfo();
    c->type = type;
    switch(type)
    {
        case ServerClient_Remote:
        {
            nonlocalclients++;
            break;
        }
        case ServerClient_Local:
        {
            localclients++;
            break;
        }
    }
    return *c;
}

void delclient(client *c)
{
    if(!c)
    {
        return;
    }
    switch(c->type)
    {
        case ServerClient_Remote:
        {
            nonlocalclients--;
            if(c->peer)
            {
                c->peer->data = nullptr;
                break;
            }
        }
        case ServerClient_Local:
        {
            localclients--;
            break;
        }
        case ServerClient_Empty:
        {
            return;
        }
    }
    c->type = ServerClient_Empty;
    c->peer = nullptr;
    if(c->info)
    {
        server::deleteclientinfo(c->info);
        c->info = nullptr;
    }
}

void cleanupserver()
{
    enet_host_destroy(serverhost);
    serverhost = nullptr;
    if(lansock != ENET_SOCKET_NULL)
    {
        enet_socket_destroy(lansock);
    }
    lansock = ENET_SOCKET_NULL;
}

void fatal(const char *fmt, ...)
{
    cleanupserver();
    DEFV_FORMAT_STRING(msg,fmt,fmt);
    fprintf(stderr, "server error: %s\n", msg);
    exit(EXIT_FAILURE);
}

VARF(maxclients, 0, DEFAULTCLIENTS, MAXCLIENTS,
{
    if(!maxclients)
    {
        maxclients = DEFAULTCLIENTS;
    }
});

VARF(maxdupclients, 0, 0, MAXCLIENTS,
{
    serverhost->duplicatePeers = maxdupclients ? maxdupclients : MAXCLIENTS;

});

void process(ENetPacket *packet, int sender, int chan);
//void disconnect_client(int n, int reason);

int getservermtu()
{
    return serverhost->mtu;
}

void *getclientinfo(int i)
{
    return !clients.size() > i || clients[i]->type==ServerClient_Empty ? nullptr : clients[i]->info;
}

ENetPeer *getclientpeer(int i)
{
    return clients.size() > i && clients[i]->type==ServerClient_Remote ? clients[i]->peer : nullptr;
}

uint getclientip(int n)
{
    return clients.size() > n && clients[n]->type==ServerClient_Remote ? clients[n]->peer->address.host : 0;
}

void sendpacket(int n, int chan, ENetPacket *packet, int exclude)
{
    if(n<0)
    {
        server::recordpacket(chan, packet->data, packet->dataLength);
        for(uint i = 0; i < clients.size(); i++)
        {
            if(i!=exclude && server::allowbroadcast(i))
            {
                sendpacket(i, chan, packet);
            }
        }
        return;
    }
    switch(clients[n]->type)
    {
        case ServerClient_Remote:
        {
            enet_peer_send(clients[n]->peer, chan, packet);
            break;
        }
    }
}

ENetPacket *sendf(int cn, int chan, const char *format, ...)
{
    int exclude = -1;
    bool reliable = false;
    if(*format=='r')
    {
        reliable = true;
        ++format;
    }
    packetbuf p(MAXTRANS, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    va_list args;
    va_start(args, format);
    while(*format)
    {
        switch(*format++)
        {
            case 'x':
            {
                exclude = va_arg(args, int);
                break;
            }
            case 'v':
            {
                int n = va_arg(args, int);
                int *v = va_arg(args, int *);
                for(int i = 0; i < n; ++i)
                {
                    putint(p, v[i]);
                }
                break;
            }
            case 'i':
            {
                int n = isdigit(*format) ? *format++-'0' : 1;
                for(int i = 0; i < n; ++i)
                {
                    putint(p, va_arg(args, int));
                }
                break;
            }
            case 'f':
            {
                int n = isdigit(*format) ? *format++-'0' : 1;
                for(int i = 0; i < n; ++i)
                {
                    putfloat(p, static_cast<float>(va_arg(args, double)));
                }
                break;
            }
            case 's':
            {
                sendstring(va_arg(args, const char *), p);
                break;
            }
            case 'm':
            {
                int n = va_arg(args, int);
                p.put(va_arg(args, uchar *), n);
                break;
            }
        }
    }
    va_end(args);
    ENetPacket *packet = p.finalize();
    sendpacket(cn, chan, packet, exclude);
    return packet->referenceCount > 0 ? packet : nullptr;
}

ENetPacket *sendfile(int cn, int chan, stream *file, const char *format, ...)
{
    if(cn < 0)
    {
        return nullptr;
    }
    else if(!(clients.size() > cn))
    {
        return nullptr;
    }
    int len = static_cast<int>(std::min(file->size(), stream::offset(INT_MAX)));
    if(len <= 0 || len > 16<<20)
    {
        return nullptr;
    }
    packetbuf p(MAXTRANS+len, ENET_PACKET_FLAG_RELIABLE);
    va_list args;
    va_start(args, format);
    while(*format)
    {
        switch(*format++)
        {
            case 'i':
            {
                int n = isdigit(*format) ? *format++-'0' : 1;
                for(int i = 0; i < n; ++i)
                {
                    putint(p, va_arg(args, int));
                }
                break;
            }
            case 's':
            {
                sendstring(va_arg(args, const char *), p);
                break;
            }
            case 'l':
            {
                putint(p, len); break;
            }
        }
    }
    va_end(args);

    file->seek(0, SEEK_SET);
    file->read(p.subbuf(len).buf, len);

    ENetPacket *packet = p.finalize();
    if(cn >= 0)
    {
        sendpacket(cn, chan, packet, -1);
    }
    return packet->referenceCount > 0 ? packet : nullptr;
}

//takes an int representing a value from the Discon enum and returns a drop message
const char *disconnectreason(int reason)
{
    switch(reason)
    {
        case Discon_EndOfPacket:
        {
            return "end of packet";
        }
        case Discon_Local:
        {
            return "server is in local mode";
        }
        case Discon_Kick:
        {
            return "kicked/banned";
        }
        case Discon_MsgError:
        {
            return "message error";
        }
        case Discon_IPBan:
        {
            return "ip is banned";
        }
        case Discon_Private:
        {
            return "server is in private mode";
        }
        case Discon_MaxClients:
        {
            return "server FULL";
        }
        case Discon_Timeout:
        {
            return "connection timed out";
        }
        case Discon_Overflow:
        {
            return "overflow";
        }
        case Discon_Password:
        {
            return "invalid password";
        }
        default:
        {
            return nullptr;
        }
    }
}

void disconnect_client(int n, int reason)
{
    //don't drop local clients
    if(!(clients.size() >n) || clients[n]->type!=ServerClient_Remote)
    {
        return;
    }
    enet_peer_disconnect(clients[n]->peer, reason);
    server::clientdisconnect(n);
    delclient(clients[n]);
    const char *msg = disconnectreason(reason);
    string s;
    if(msg)
    {
        formatstring(s, "client (%s) disconnected because: %s", clients[n]->hostname, msg);
    }
    else
    {
        formatstring(s, "client (%s) disconnected", clients[n]->hostname);
    }
    printf("%s\n", s);
    server::sendservmsg(s);
}

void kicknonlocalclients(int reason)
{
    for(uint i = 0; i < clients.size(); i++)
    {
        if(clients[i]->type==ServerClient_Remote)
        {
            disconnect_client(i, reason);
        }
    }
}

void process(ENetPacket *packet, int sender, int chan)   // sender may be -1
{
    packetbuf p(packet);
    server::parsepacket(sender, chan, p);
    if(p.overread())
    {
        disconnect_client(sender, Discon_EndOfPacket);
        return;
    }
}

bool resolverwait(const char *name, ENetAddress *address)
{
    return enet_address_set_host(address, name) >= 0;
}

int connectwithtimeout(ENetSocket sock, const char *hostname, const ENetAddress &remoteaddress)
{
    return enet_socket_connect(sock, &remoteaddress);
}

ENetSocket mastersock = ENET_SOCKET_NULL;
ENetAddress masteraddress = { ENET_HOST_ANY, ENET_PORT_ANY },
            serveraddress = { ENET_HOST_ANY, ENET_PORT_ANY };
int lastupdatemaster = 0,
    lastconnectmaster = 0,
    masterconnecting = 0,
    masterconnected = 0;
std::vector<char> masterout, masterin;
int masteroutpos = 0,
    masterinpos = 0;
VARN(updatemaster, allowupdatemaster, 0, 1, 1);

void disconnectmaster()
{
    if(mastersock != ENET_SOCKET_NULL)
    {
        enet_socket_destroy(mastersock);
        mastersock = ENET_SOCKET_NULL;
    }
    masterout.clear();
    masterin.clear();
    masteroutpos = masterinpos = 0;

    masteraddress.host = ENET_HOST_ANY;
    masteraddress.port = ENET_PORT_ANY;

    lastupdatemaster = masterconnecting = masterconnected = 0;
}

SVARF(mastername, server::defaultmaster(), disconnectmaster());
VARF(masterport, 1, server::masterport(), 0xFFFF, disconnectmaster());

ENetSocket connectmaster(bool wait)
{
    if(!mastername[0]) //if no master to look up
    {
        return ENET_SOCKET_NULL;
    }
    if(masteraddress.host == ENET_HOST_ANY)
    {
        printf("looking up %s...\n", mastername);
        masteraddress.port = masterport;
        if(!resolverwait(mastername, &masteraddress))
        {
            return ENET_SOCKET_NULL;
        }
    }
    ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
    if(sock == ENET_SOCKET_NULL)
    {
        printf("could not open master server socket\n");
        return ENET_SOCKET_NULL;
    }
    if(wait || serveraddress.host == ENET_HOST_ANY || !enet_socket_bind(sock, &serveraddress))
    {
        enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);
        if(wait)
        {
            if(!connectwithtimeout(sock, mastername, masteraddress))
            {
                return sock;
            }
        }
        else if(!enet_socket_connect(sock, &masteraddress))
        {
            return sock;
        }
    }
    enet_socket_destroy(sock);
    printf("could not connect to master server\n");

    return ENET_SOCKET_NULL;
}

bool requestmaster(const char *req)
{
    if(mastersock == ENET_SOCKET_NULL)
    {
        mastersock = connectmaster(false);
        if(mastersock == ENET_SOCKET_NULL)
        {
            return false;
        }
        lastconnectmaster = masterconnecting = totalmillis ? totalmillis : 1;
    }
    if(masterout.size() >= 4096)
    {
        return false;
    }
    for(uint i = 0; i < strlen(req); ++i)
    {
        masterout.push_back(req[i]);
    }
    return true;
}

bool requestmasterf(const char *fmt, ...)
{
    DEFV_FORMAT_STRING(req, fmt, fmt);
    return requestmaster(req);
}

void processmasterinput()
{
    if(masterinpos >= static_cast<int>(masterin.size()))
    {
        return;
    }
    char *input = &masterin[masterinpos], *end = static_cast<char *>(memchr(input, '\n', masterin.size() - masterinpos));
    while(end)
    {
        *end = '\0';

        const char *args = input;
        while(args < end && !iscubespace(*args))
        {
            args++;
        }
        int cmdlen = args - input;
        while(args < end && iscubespace(*args))
        {
            args++;
        }
        if(matchstring(input, cmdlen, "failreg"))
        {
            printf("master server registration failed: %s\n", args);
        }
        else if(matchstring(input, cmdlen, "succreg"))
        {
            printf("master server registration succeeded\n");
        }
        end++;
        masterinpos = end - masterin.data();
        input = end;
        end = reinterpret_cast<char*>(memchr(input, '\n', masterin.size() - masterinpos));
    }

    if(masterinpos >= masterin.size())
    {
        masterin.clear();
        masterinpos = 0;
    }
}

void flushmasteroutput()
{
    if(masterconnecting && totalmillis - masterconnecting >= 60000)
    {
        printf("could not connect to master server\n");
        disconnectmaster();
    }
    if(masterout.empty() || !masterconnected)
    {
        return;
    }
    ENetBuffer buf;
    buf.data = &masterout[masteroutpos];
    buf.dataLength = masterout.size() - masteroutpos;
    int sent = enet_socket_send(mastersock, nullptr, &buf, 1);
    if(sent >= 0)
    {
        masteroutpos += sent;
        if(masteroutpos >= masterout.size())
        {
            masterout.clear();
            masteroutpos = 0;
        }
    }
    else
    {
        disconnectmaster();
    }
}

void flushmasterinput()
{
    if(masterin.size() >= masterin.capacity())
    {
        masterin.reserve(4096);
    }
    ENetBuffer buf;
    buf.data = masterin.data() + masterin.size();
    buf.dataLength = masterin.capacity() - masterin.size();
    int recv = enet_socket_receive(mastersock, nullptr, &buf, 1);
    if(recv > 0)
    {
        for(uint i = 0; i < recv; ++i)
        {
            masterin.emplace_back();
        }
        processmasterinput();
    }
    else
    {
        disconnectmaster();
    }
}

static ENetAddress serverinfoaddress;

void sendserverinforeply(ucharbuf &p)
{
    ENetBuffer buf;
    buf.data = p.buf;
    buf.dataLength = p.length();
    enet_socket_send(serverhost->socket, &serverinfoaddress, &buf, 1);
}

constexpr int MAXPINGDATA = 32;

void checkserversockets()        // reply all server info requests
{
    static ENetSocketSet readset, writeset;
    ENET_SOCKETSET_EMPTY(readset);
    ENET_SOCKETSET_EMPTY(writeset);
    ENetSocket maxsock = ENET_SOCKET_NULL;
    if(mastersock != ENET_SOCKET_NULL)
    {
        maxsock = maxsock == ENET_SOCKET_NULL ? mastersock : std::max(maxsock, mastersock);
        ENET_SOCKETSET_ADD(readset, mastersock);
        if(!masterconnected)
        {
            ENET_SOCKETSET_ADD(writeset, mastersock);
        }
    }
    if(lansock != ENET_SOCKET_NULL)
    {
        maxsock = maxsock == ENET_SOCKET_NULL ? lansock : std::max(maxsock, lansock);
        ENET_SOCKETSET_ADD(readset, lansock);
    }
    if(maxsock == ENET_SOCKET_NULL || enet_socketset_select(maxsock, &readset, &writeset, 0) <= 0)
    {
        return;
    }

    if(lansock != ENET_SOCKET_NULL && ENET_SOCKETSET_CHECK(readset, lansock))
    {
        ENetBuffer buf;
        uchar data[MAXTRANS];
        buf.data = data;
        buf.dataLength = sizeof(data);
        int len = enet_socket_receive(lansock, &serverinfoaddress, &buf, 1);
        if(len < 2 || data[0] != 0xFF || data[1] != 0xFF || len-2 > MAXPINGDATA)
        {
            return;
        }
        ucharbuf req(data+2, len-2), p(data+2, sizeof(data)-2);
        p.len += len-2;
        server::serverinforeply(req, p);
    }

    if(mastersock != ENET_SOCKET_NULL)
    {
        if(!masterconnected)
        {
            if(ENET_SOCKETSET_CHECK(readset, mastersock) || ENET_SOCKETSET_CHECK(writeset, mastersock))
            {
                int error = 0;
                if(enet_socket_get_option(mastersock, ENET_SOCKOPT_ERROR, &error) < 0 || error)
                {
                    printf("could not connect to master server\n");
                    disconnectmaster();
                }
                else
                {
                    masterconnecting = 0;
                    masterconnected = totalmillis ? totalmillis : 1;
                    server::masterconnected();
                }
            }
        }
        if(mastersock != ENET_SOCKET_NULL && ENET_SOCKETSET_CHECK(readset, mastersock))
        {
            flushmasterinput();
        }
    }
}

static int serverinfointercept(ENetHost *host, ENetEvent *event)
{
    if(host->receivedDataLength < 2 || host->receivedData[0] != 0xFF || host->receivedData[1] != 0xFF || host->receivedDataLength-2 > MAXPINGDATA)
    {
        return 0;
    }
    serverinfoaddress = host->receivedAddress;
    ucharbuf req(host->receivedData+2, host->receivedDataLength-2), p(host->receivedData+2, sizeof(host->packetData[0])-2);
    p.len += host->receivedDataLength-2;
    server::serverinforeply(req, p);
    return 1;
}

VAR(serveruprate, 0, 0, INT_MAX);
SVAR(serverip, "");
VARF(serverport, 0, server::serverport(), 0xFFFF,
{
    if(!serverport)
    {
        serverport = server::serverport();
    }
});

int curtime = 0,
    lastmillis = 0,
    elapsedtime = 0,
    totalmillis = 0;

void updatemasterserver()
{
    if(!masterconnected && lastconnectmaster && totalmillis-lastconnectmaster <= 5*60*1000)
    {
        return;
    }
    if(mastername[0] && allowupdatemaster)
    {
        requestmasterf("regserv %d\n", serverport);
    }
    lastupdatemaster = totalmillis ? totalmillis : 1;
}

uint totalsecs = 0;

void updatetime()
{
    static int lastsec = 0;
    if(totalmillis - lastsec >= 1000)
    {
        int cursecs = (totalmillis - lastsec) / 1000;
        totalsecs += cursecs;
        lastsec += cursecs * 1000;
    }
}

void serverslice(uint timeout)   // main server update, called from below in dedicated server
{
    static int laststatus = 0;
    static int lastcheckscore = -1;

    // below is network only
    int millis = static_cast<int>(enet_time_get());
    elapsedtime = millis - totalmillis;
    static int timeerr = 0;
    int scaledtime = server::scaletime(elapsedtime) + timeerr;
    curtime = scaledtime/100;
    timeerr = scaledtime%100;
    if(server::ispaused())
    {
        curtime = 0;
    }
    lastmillis += curtime;
    totalmillis = millis;
    updatetime();
    server::serverupdate(); //see game/server.cpp for meat of server update routine
    if(totalsecs-lastcheckscore > 0) //check scores 1/sec
    {
        lastcheckscore = totalsecs;
        updatescores(); //see game/mapcontrol.cpp for updating player scores
        sendscore(); //sends tallies of scores out to players
    }

    flushmasteroutput();
    checkserversockets();

    if(!lastupdatemaster || totalmillis-lastupdatemaster>60*60*1000)       // send alive signal to masterserver every hour of uptime
    {
        updatemasterserver();
    }

    if(totalmillis-laststatus>60*1000)   // display bandwidth stats, useful for server ops
    {
        laststatus = totalmillis;
        if(nonlocalclients || serverhost->totalSentData || serverhost->totalReceivedData)
        {
            printf("status: %d remote clients, %.1f send, %.1f rec (K/sec)\n", nonlocalclients, serverhost->totalSentData/60.0f/1024, serverhost->totalReceivedData/60.0f/1024);
        }
        serverhost->totalSentData = serverhost->totalReceivedData = 0;
    }

    ENetEvent event;
    bool serviced = false;
    while(!serviced)
    {
        if(enet_host_check_events(serverhost, &event) <= 0)
        {
            if(enet_host_service(serverhost, &event, timeout) <= 0)
            {
                break;
            }
            serviced = true;
        }
        switch(event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                client &c = addclient(ServerClient_Remote);
                c.peer = event.peer;
                c.peer->data = &c;
                string hn;
                copystring(c.hostname, (enet_address_get_host_ip(&c.peer->address, hn, sizeof(hn))==0) ? hn : "unknown");
                printf("client connected (%s)\n", c.hostname);
                int reason = server::clientconnect(c.num, c.peer->address.host);
                if(reason)
                {
                    disconnect_client(c.num, reason);
                }
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                client *c = static_cast<client *>(event.peer->data);
                if(c)
                {
                    process(event.packet, c->num, event.channelID);
                }
                if(event.packet->referenceCount==0)
                {
                    enet_packet_destroy(event.packet);
                }
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                client *c = static_cast<client *>(event.peer->data);
                if(!c)
                {
                    break;
                }
                printf("disconnected client (%s)\n", c->hostname);
                server::clientdisconnect(c->num);
                delclient(c);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    if(server::sendpackets())
    {
        enet_host_flush(serverhost);
    }
}

void flushserver(bool force)
{
    if(server::sendpackets(force) && serverhost)
    {
        enet_host_flush(serverhost);
    }
}

void rundedicatedserver()
{
    printf("dedicated server started, waiting for clients...\n");
    for(;;)
    {
        serverslice(5);
    }
}

bool servererror(const char *desc)
{
    fatal("%s", desc);
    return false;
}

bool setuplistenserver()
{
    ENetAddress address = { ENET_HOST_ANY, enet_uint16(serverport <= 0 ? server::serverport() : serverport) };
    if(*serverip)
    {
        if(enet_address_set_host(&address, serverip)<0)
        {
            printf("WARNING: server ip not resolved\n");
        }
        else
        {
            serveraddress.host = address.host;
        }
    }
    serverhost = enet_host_create(&address, std::min(maxclients + server::reserveclients(), MAXCLIENTS), server::numchannels(), 0, serveruprate);
    if(!serverhost)
    {
        return servererror("could not create server host");
    }
    serverhost->duplicatePeers = maxdupclients ? maxdupclients : MAXCLIENTS;
    serverhost->intercept = serverinfointercept;
    address.port = server::laninfoport();
    lansock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if(lansock != ENET_SOCKET_NULL && (enet_socket_set_option(lansock, ENET_SOCKOPT_REUSEADDR, 1) < 0 || enet_socket_bind(lansock, &address) < 0))
    {
        enet_socket_destroy(lansock);
        lansock = ENET_SOCKET_NULL;
    }
    if(lansock == ENET_SOCKET_NULL)
    {
        printf("WARNING: could not create LAN server info socket\n");
    }
    else
    {
        enet_socket_set_option(lansock, ENET_SOCKOPT_NONBLOCK, 1);
    }
    return true;
}

void initserver(bool listen)
{
    exec("../../config/server-init.cfg");
    if(listen)
    {
        setuplistenserver();
    }
    server::serverinit();
    if(listen)
    {
        updatemasterserver();
        rundedicatedserver(); // never returns
    }
}

int main(int argc, char **argv)
{
    if(enet_initialize()<0)
    {
        fatal("Unable to initialise network module");
    }
    atexit(enet_deinitialize);
    enet_time_set(0);

    initserver(true);
    return EXIT_SUCCESS;
}

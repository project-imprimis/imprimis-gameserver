// the interface the game uses to access the engine

extern int curtime;                     // current frame time
extern int lastmillis;                  // last time
extern int elapsedtime;                 // elapsed frame time
extern int totalmillis;                 // total elapsed time
extern uint totalsecs;
extern int gamespeed, paused;

// octaedit

enum
{
    Edit_Face = 0,
    Edit_Tex,
    Edit_Mat,
    Edit_Flip,
    Edit_Copy,
    Edit_Paste,
    Edit_Rotate,
    Edit_Replace,
    Edit_DelCube,
    Edit_CalcLight,
    Edit_Remip,
    Edit_VSlot,
    Edit_Undo,
    Edit_Redo
};

// command
extern int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), int flags);
extern char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), int flags);
extern bool addcommand(const char *name, void (*fun)(), const char *narg);
extern int execute(const char *p);
extern char *executeret(const char *p);
extern void exec(const char *cfgfile);
extern bool execfile(const char *cfgfile);

// console

// main
extern void fatal(const char *s, ...) PRINTFARGS(1, 2);

// server
constexpr int MAXCLIENTS = 128;                 // DO NOT set this any higher
constexpr int MAXTRANS   = 5000;                // max amount of data to swallow in 1 go

extern int maxclients;

enum
{
    Discon_None = 0,
    Discon_EndOfPacket,
    Discon_Local,
    Discon_Kick,
    Discon_MsgError,
    Discon_IPBan,
    Discon_Private,
    Discon_MaxClients,
    Discon_Timeout,
    Discon_Overflow,
    Discon_Password,
    Discon_NumDiscons
};

extern void *getclientinfo(int i);
extern ENetPeer *getclientpeer(int i);
extern ENetPacket *sendf(int cn, int chan, const char *format, ...);
extern ENetPacket *sendfile(int cn, int chan, stream *file, const char *format = "", ...);
extern void sendpacket(int cn, int chan, ENetPacket *packet, int exclude = -1);
extern void flushserver(bool force);
extern int getservermtu();
extern uint getclientip(int n);
extern const char *disconnectreason(int reason);
extern void disconnect_client(int n, int reason);
extern void kicknonlocalclients(int reason = Discon_None);
extern bool hasnonlocalclients();
extern void sendserverinforeply(ucharbuf &p);
extern bool requestmaster(const char *req);
extern bool requestmasterf(const char *fmt, ...) PRINTFARGS(1, 2);

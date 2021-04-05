
namespace server
{
    extern int nextplayback, demomillis;

    extern int maxdemos, maxdemosize, restrictdemos;

    extern bool demonextmatch;
    extern stream *demotmp,
                  *demorecord,
                  *demoplayback;

    extern void listdemos(int cn);
    extern void cleardemos(int n);
    extern void freegetmap(ENetPacket *packet);
    extern void senddemo(clientinfo *ci, int num);
    extern void enddemoplayback();
    extern void setupdemoplayback();
    extern void readdemo();
    extern void prunedemos(int extra = 0);
    extern void adddemo();
    extern void enddemorecord();
    extern void stopdemo();
    extern void writedemo(int chan, void *data, int len);
    extern void setupdemorecord();
}


struct entity                                   // persistent map entity
{
    vec o;                                      // position
    short attr1, attr2, attr3, attr4, attr5;    // attributes
    uchar type;                                 // type is one of the above
    uchar reserved;
};

constexpr int MAXENTS = 10000;

enum
{
    ClientState_Alive = 0,
    ClientState_Dead,
    ClientState_Spawning,
    ClientState_Lagged,
    ClientState_Editing,
    ClientState_Spectator,
};

enum
{
    Gun_Rail = 0,
    Gun_Pulse,
    Gun_Eng,
    Gun_Carbine,
    Gun_NumGuns
};

enum
{
    Attack_RailShot = 0,
    Attack_PulseShoot,
    Attack_EngShoot,
    Attack_CarbineShoot,
    Attack_NumAttacks
};

// this defines weapon properties
//                                   8            9       14     15    17
static const struct attackinfo { int attackdelay, damage, range, rays, exprad; } attacks[Attack_NumAttacks] =

//    8     9   14   15  17
{
    {  300,  5, 1200, 1,  0},
    {  700,  8, 1024, 1, 50},
    {  250,  0,  160, 1, 20},
    {   80,  2,  512, 1,  0},
};

#define VALID_GUN(n) ((n) >= 0 && (n) < Gun_NumGuns)
#define VALID_ATTACK(n) ((n) >= 0 && (n) < Attack_NumAttacks)
#define VALID_ITEM(n) false //no items in this game thus far

enum
{
    AI_None = 0,
    AI_Bot,
    AI_Max
};

//enum of gameplay mechanic flags; bitwise sum determines what a mode's attributes are
enum
{
    Mode_Team           = 1<<0,
    Mode_CTF            = 1<<1,
    Mode_AllowOvertime  = 1<<2,
    Mode_Edit           = 1<<3,
    Mode_Demo           = 1<<4,
    Mode_LocalOnly      = 1<<5,
    Mode_Lobby          = 1<<6,
    Mode_Rail           = 1<<7,
    Mode_Pulse          = 1<<8,
    Mode_Continuous     = 1<<9, //continuous scoring
    Mode_All            = 1<<10
};

enum
{
    Mode_Untimed         = Mode_Edit|Mode_LocalOnly|Mode_Demo,
    Mode_Bot             = Mode_LocalOnly|Mode_Demo
};

static struct gamemodeinfo
{
    const char *name, *prettyname;
    int flags;
} gamemodes[] =
//list of valid game modes with their name/prettyname/game flags/desc
{
    { "demo", "Demo", Mode_Demo | Mode_LocalOnly},
    { "edit", "Edit", Mode_Edit | Mode_All},
    { "tdm", "TDM",  Mode_Team | Mode_All | Mode_Continuous},
};

//these are the checks for particular mechanics in particular modes
//e.g. MODE_RAIL sees if the mode only have railguns
constexpr int STARTGAMEMODE = -1;
constexpr int  NUMGAMEMODES =((int)(sizeof(gamemodes)/sizeof(gamemodes[0])));

//check fxn
inline bool modecheck(int mode, int flag)
{
    if((mode) >= STARTGAMEMODE && (mode) < STARTGAMEMODE + NUMGAMEMODES) //make sure input is within valid range
    {
        if(gamemodes[(mode) - STARTGAMEMODE].flags&(flag))
        {
            return true;
        }
        return false;
    }
    return false;
}

// inherited by gameent and server clients
struct gamestate
{
    int health, maxhealth;
    int gunselect, gunwait;
    int combatclass;
    int ammo[Gun_NumGuns];
    int aitype, skill;

    gamestate() : maxhealth(1), aitype(AI_None), skill(0) {}

    bool canpickup(int type)
    {
        return VALID_ITEM(type);
    }

    void pickup(int type)
    {
    }

    void respawn()
    {
        health = maxhealth;
        gunselect = Gun_Rail;
        gunwait = 0;
        for(int i = 0; i < Gun_NumGuns; ++i)
        {
            ammo[i] = 0;
        }
    }

    void spawnstate(int gamemode)
    {
        if(modecheck(gamemode, Mode_All))
        {
            gunselect = Gun_Rail;
            for(int i = 0; i < Gun_NumGuns; ++i)
            {
                ammo[i] = 1;
            }
        }
        else if(modecheck(gamemode, Mode_Rail))
        {
            gunselect = Gun_Rail;
            ammo[Gun_Rail] = 1;
        }
        else if(modecheck(gamemode, Mode_Pulse))
        {
            gunselect = Gun_Pulse;
            ammo[Gun_Pulse] = 1;
        }
    }

    // just subtract damage here, can set death, etc. later in code calling this
    int dodamage(int damage)
    {
        health -= damage;
        return damage;
    }

    int hasammo(int gun, int exclude = -1)
    {
        return VALID_GUN(gun) && gun != exclude && ammo[gun] > 0;
    }
};

constexpr int MAXTEAMS = 2;

static const char * const teamnames[1+MAXTEAMS] = { "", "azul", "rojo" };
static const char * const teamtextcode[1+MAXTEAMS] = { "\f0", "\f1", "\f3" };
inline int teamnumber(const char *name)
{
    for(int i = 0; i < MAXTEAMS; ++i)
    {
        if(!strcmp(teamnames[1+i], name))
        {
            return 1+i;
        }
    }
    return 0;
}

#define VALID_TEAM(n) ((n) >= 1 && (n) <= MAXTEAMS)
#define TEAM_NAME(n) (teamnames[VALID_TEAM(n) ? (n) : 0])

struct teamscore
{
    int team, score;
    teamscore() {}
    teamscore(int team, int n) : team(team), score(n) {}

    bool operator==(const teamscore &y) const
    {
        return team == y.team;
    }

    bool operator<(const teamscore &y) const
    {
        if(score > y.score) return true;
        if(score < y.score) return false;
        return team < y.team;
    }
};
inline bool htcmp(int team, const teamscore &t) { return team == t.team; }

struct teaminfo
{
    int frags, score;

    teaminfo() { reset(); }

    void reset()
    {
        frags = 0;
        score = 0;
    }
};

namespace server
{
    struct gameevent;

    template <int N>
    struct projectilestate
    {
        int projs[N];
        int numprojs;

        projectilestate() : numprojs(0) {}

        void reset() { numprojs = 0; }

        void add(int val)
        {
            if(numprojs>=N) numprojs = 0;
            projs[numprojs++] = val;
        }

        bool remove(int val)
        {
            for(int i = 0; i < numprojs; ++i)
            {
                if(projs[i]==val)
                {
                    projs[i] = projs[--numprojs];
                    return true;
                }
            }
            return false;
        }
    };

    struct servstate : gamestate
    {
        vec o;
        int state, editstate;
        int lastdeath, deadflush, lastspawn, lifesequence;
        int lastshot;
        projectilestate<8> projs;
        int frags, score, deaths, teamkills, shotdamage, damage;
        int lasttimeplayed, timeplayed;
        float effectiveness;

        servstate() : state(ClientState_Dead), editstate(ClientState_Dead), lifesequence(0) {}

        bool isalive(int gamemillis);
        bool waitexpired(int gamemillis);
        void reset();
        void respawn();
        void reassign();
    };

    struct clientinfo
    {
        int clientnum, ownernum, connectmillis, sessionid, overflow;
        string name, mapvote;
        int team, playermodel, playercolor;
        int modevote;
        int privilege;
        bool connected, local, timesync;
        int gameoffset, lastevent, pushed, exceeded;
        servstate state;
        vector<gameevent *> events;
        vector<uchar> position, messages;
        uchar *wsdata;
        int wslen;
        vector<clientinfo *> bots;
        int ping, aireinit;
        string clientmap;
        int mapcrc;
        bool warned, gameclip;
        ENetPacket *getdemo, *getmap, *clipboard;
        int lastclipboard, needclipboard;
        int connectauth;
        uint authreq;
        string authname, authdesc;
        void *authchallenge;
        int authkickvictim;
        char *authkickreason;

        clientinfo() : getdemo(nullptr), getmap(nullptr), clipboard(nullptr), authchallenge(nullptr), authkickreason(nullptr) { reset(); }
        ~clientinfo() { events.deletecontents(); cleanclipboard();}

        enum
        {
            PUSHMILLIS = 3000
        };

        void addevent(gameevent *e);
        int calcpushrange();
        bool checkpushed(int millis, int range);
        void scheduleexceeded();
        void setexceeded();
        void setpushed();
        bool checkexceeded();
        void mapchange();
        void reassign();
        void cleanclipboard(bool fullclean = true);
        void reset();
        int geteventmillis(int servmillis, int clientmillis);
    };

    struct gameevent
    {
        virtual ~gameevent() {}

        virtual bool flush(clientinfo *ci, int fmillis);
        virtual void process(clientinfo *ci) {}

        virtual bool keepable() const { return false; }
    };

    extern void sendservmsgf(const char *fmt, ...);
    extern void sendwelcome(clientinfo *ci);
    extern int welcomepacket(packetbuf &p, clientinfo *ci);

    extern vector<clientinfo *> clients;
    extern int gamemillis;
    extern string smapname;
    extern teaminfo teaminfos[MAXTEAMS];

}

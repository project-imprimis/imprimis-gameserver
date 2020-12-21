#ifndef __GAME_H__
#define __GAME_H__

//defines game statics, like animation names, weapon variables, entity properties
//includes:
//animation names
//console message types
//weapon vars
//game state information
//game entity definition

// network quantization scale
#define DMF 16.0f                // for world locations
#define DNF 100.0f              // for normalized vectors
#define DVELF 1.0f              // for playerspeed based velocity vectors

enum
{
    EngineEnt_Empty=0,
    EngineEnt_Light,
    EngineEnt_Mapmodel,
    EngineEnt_Playerstart,
    EngineEnt_Particles,
    EngineEnt_Sound,
    EngineEnt_Spotlight,
    EngineEnt_Decal,
    EngineEnt_GameSpecific,
};

struct entity                                   // persistent map entity
{
    vec o;                                      // position
    short attr1, attr2, attr3, attr4, attr5;    // attributes
    uchar type;                                 // type is one of the above
    uchar reserved;
};

#define MAXENTS 10000

enum
{
    ClientState_Alive = 0,
    ClientState_Dead,
    ClientState_Spawning,
    ClientState_Lagged,
    ClientState_Editing,
    ClientState_Spectator,
};

//these are called "GamecodeEnt" to avoid name collision (as opposed to gameents or engine static ents)
enum                            // static entity types
{
    GamecodeEnt_NotUsed              = EngineEnt_Empty,        // entity slot not in use in map
    GamecodeEnt_Light                = EngineEnt_Light,        // lightsource, attr1 = radius, attr2 = red, attr3 = green, attr4 = blue, attr5 = flags
    GamecodeEnt_Mapmodel             = EngineEnt_Mapmodel,     // attr1 = index, attr2 = yaw, attr3 = pitch, attr4 = roll, attr5 = scale
    GamecodeEnt_Playerstart,                                   // attr1 = angle, attr2 = team
    GamecodeEnt_Particles            = EngineEnt_Particles,    // attr1 = index, attrs2-5 vary on particle index
    GamecodeEnt_MapSound             = EngineEnt_Sound,        // attr1 = index, attr2 = sound
    GamecodeEnt_Spotlight            = EngineEnt_Spotlight,    // attr1 = angle
    GamecodeEnt_Decal                = EngineEnt_Decal,        // attr1 = index, attr2 = yaw, attr3 = pitch, attr4 = roll, attr5 = scale
    GamecodeEnt_Teleport,                                      // attr1 = channel, attr2 = model, attr3 = tag
    GamecodeEnt_Teledest,                                      // attr1 = yaw, attr2 = channel
    GamecodeEnt_Jumppad,                                       // attr1 = zpush, attr2 = ypush, attr3 = xpush
    GamecodeEnt_Flag,                                          // attr1 = yaw, attr2 = team
    GamecodeEnt_MaxEntTypes,                                   // used for looping through full enum
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
    Attack_RailMelee,
    Attack_PulseShoot,
    Attack_PulseMelee,
    Attack_EngShoot,
    Attack_EngMelee,
    Attack_CarbineShoot,
    Attack_CarbineMelee,
    Attack_NumAttacks
};

#define VALID_GUN(n) ((n) >= 0 && (n) < Gun_NumGuns)
#define VALID_ATTACK(n) ((n) >= 0 && (n) < Attack_NumAttacks)

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
    Mode_All            = 1<<9
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
    const char *info;
} gamemodes[] =
//list of valid game modes with their name/prettyname/game flags/desc
{
    { "demo", "Demo", Mode_Demo | Mode_LocalOnly, NULL},
    { "edit", "Edit", Mode_Edit | Mode_All, "Cooperative Editing:\nEdit maps with multiple players simultaneously." },
    { "ctf", "CTF", Mode_CTF | Mode_Team | Mode_All, "Capture The Flag:\nCapture \fs\f3the enemy flag\fr and bring it back to \fs\f1your flag\fr to score points for \fs\f1your team\fr." },
};

//these are the checks for particular mechanics in particular modes
//e.g. MODE_RAIL sees if the mode only have railguns
#define STARTGAMEMODE (-1)
#define NUMGAMEMODES ((int)(sizeof(gamemodes)/sizeof(gamemodes[0])))

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

#define MODE_VALID(mode)          ((mode) >= STARTGAMEMODE && (mode) < STARTGAMEMODE + NUMGAMEMODES)

enum {
    MasterMode_Auth = -1,
    MasterMode_Open = 0,
    MasterMode_Veto,
    MasterMode_Locked,
    MasterMode_Private,
    MasterMode_Password,
    MasterMode_Start = MasterMode_Auth,
    MasterMode_Invalid = MasterMode_Start - 1
};

static const char * const mastermodenames[] =  { "auth",   "open",   "veto",       "locked",     "private",    "password" };
static const char * const mastermodecolors[] = { "",       "\f0",    "\f2",        "\f2",        "\f3",        "\f3" };
static const char * const mastermodeicons[] =  { "server", "server", "serverlock", "serverlock", "serverpriv", "serverpriv" };

// network messages codes, c2s, c2c, s2c

enum
{
    Priv_None = 0,
    Priv_Master,
    Priv_Auth,
    Priv_Admin
};

enum
{
    NetMsg_Connect = 0,
    NetMsg_ServerInfo,
    NetMsg_Welcome,
    NetMsg_InitClient,
    NetMsg_Pos,
    NetMsg_Text,
    NetMsg_Sound,
    NetMsg_ClientDiscon,
    NetMsg_Shoot,
    //game
    NetMsg_Explode,
    NetMsg_Suicide, //10
    NetMsg_Died,
    NetMsg_Damage,
    NetMsg_Hitpush,
    NetMsg_ShotFX,
    NetMsg_ExplodeFX,
    NetMsg_TrySpawn,
    NetMsg_SpawnState,
    NetMsg_Spawn,
    NetMsg_ForceDeath,
    NetMsg_GunSelect, //20
    NetMsg_Taunt,
    NetMsg_MapChange,
    NetMsg_MapVote,
    NetMsg_TeamInfo,
    NetMsg_ItemSpawn,
    NetMsg_ItemPickup,
    NetMsg_ItemAcceptance,
    NetMsg_Teleport,
    NetMsg_Jumppad,

    NetMsg_Ping, //30
    NetMsg_Pong,
    NetMsg_ClientPing,
    NetMsg_TimeUp,
    NetMsg_ForceIntermission,
    NetMsg_ServerMsg,
    NetMsg_ItemList,
    NetMsg_Resume,
    //edit
    NetMsg_EditMode,
    NetMsg_EditEnt,
    NetMsg_EditFace, //40
    NetMsg_EditTex,
    NetMsg_EditMat,
    NetMsg_EditFlip,
    NetMsg_Copy,
    NetMsg_Paste,
    NetMsg_Rotate,
    NetMsg_Replace,
    NetMsg_DelCube,
    NetMsg_AddCube,
    NetMsg_CalcLight,
    NetMsg_Remip, //50
    NetMsg_EditVSlot,
    NetMsg_Undo,
    NetMsg_Redo,
    NetMsg_Newmap,
    NetMsg_GetMap,
    NetMsg_SendMap,
    NetMsg_Clipboard,
    NetMsg_EditVar,
    //master
    NetMsg_MasterMode,
    NetMsg_Kick, //60
    NetMsg_ClearBans,
    NetMsg_CurrentMaster,
    NetMsg_Spectator,
    NetMsg_SetMasterMaster,
    NetMsg_SetTeam,
    //demo
    NetMsg_ListDemos,
    NetMsg_SendDemoList,
    NetMsg_GetDemo,
    NetMsg_SendDemo,
    NetMsg_DemoPlayback, //70
    NetMsg_RecordDemo,
    NetMsg_StopDemo,
    NetMsg_ClearDemos,
    //flag
    NetMsg_TakeFlag,
    NetMsg_ReturnFlag,
    NetMsg_ResetFlag,
    NetMsg_TryDropFlag,
    NetMsg_DropFlag,
    NetMsg_ScoreFlag,
    NetMsg_InitFlags, //80
    //misc
    NetMsg_SayTeam,
    NetMsg_Client,
    NetMsg_AuthTry,
    NetMsg_AuthKick,
    NetMsg_AuthChallenge,
    NetMsg_AuthAnswer,
    NetMsg_ReqAuth,
    NetMsg_PauseGame,
    NetMsg_GameSpeed,
    NetMsg_AddBot, //90
    NetMsg_DelBot,
    NetMsg_InitAI,
    NetMsg_FromAI,
    NetMsg_BotLimit,
    NetMsg_BotBalance,
    NetMsg_MapCRC,
    NetMsg_CheckMaps,
    NetMsg_SwitchName,
    NetMsg_SwitchModel,
    NetMsg_SwitchColor, //100
    NetMsg_SwitchTeam,
    NetMsg_ServerCommand,
    NetMsg_DemoPacket,
    NetMsg_NumMsgs //104
};

static const int msgsizes[] =               // size inclusive message token, 0 for variable or not-checked sizes
{
    NetMsg_Connect, 0,
    NetMsg_ServerInfo, 0,
    NetMsg_Welcome, 1,
    NetMsg_InitClient, 0,
    NetMsg_Pos, 0,
    NetMsg_Text, 0,
    NetMsg_Sound, 2,
    NetMsg_ClientDiscon, 2,

    NetMsg_Shoot, 0,
    NetMsg_Explode, 0,
    NetMsg_Suicide, 1,
    NetMsg_Died, 5,
    NetMsg_Damage, 5,
    NetMsg_Hitpush, 7,
    NetMsg_ShotFX, 10,
    NetMsg_ExplodeFX, 4,
    NetMsg_TrySpawn, 1,
    NetMsg_SpawnState, 8,
    NetMsg_Spawn, 3,
    NetMsg_ForceDeath, 2,
    NetMsg_GunSelect, 2,
    NetMsg_Taunt, 1,
    NetMsg_MapChange, 0,
    NetMsg_MapVote, 0,
    NetMsg_TeamInfo, 0,
    NetMsg_ItemSpawn, 2,
    NetMsg_ItemPickup, 2,
    NetMsg_ItemAcceptance, 3,

    NetMsg_Ping, 2,
    NetMsg_Pong, 2,
    NetMsg_ClientPing, 2,
    NetMsg_TimeUp, 2,
    NetMsg_ForceIntermission, 1,
    NetMsg_ServerMsg, 0,
    NetMsg_ItemList, 0,
    NetMsg_Resume, 0,

    NetMsg_EditMode, 2,
    NetMsg_EditEnt, 11,
    NetMsg_EditFace, 16,
    NetMsg_EditTex, 16,
    NetMsg_EditMat, 16,
    NetMsg_EditFlip, 14,
    NetMsg_Copy, 14,
    NetMsg_Paste, 14,
    NetMsg_Rotate, 15,
    NetMsg_Replace, 17,
    NetMsg_DelCube, 14,
    NetMsg_AddCube, 14,
    NetMsg_CalcLight, 1,
    NetMsg_Remip, 1,
    NetMsg_EditVSlot, 16,
    NetMsg_Undo, 0,
    NetMsg_Redo, 0,
    NetMsg_Newmap, 2,
    NetMsg_GetMap, 1,
    NetMsg_SendMap, 0,
    NetMsg_EditVar, 0,
    NetMsg_MasterMode, 2,
    NetMsg_Kick, 0,
    NetMsg_ClearBans, 1,
    NetMsg_CurrentMaster, 0,
    NetMsg_Spectator, 3,
    NetMsg_SetMasterMaster, 0,
    NetMsg_SetTeam, 0,

    NetMsg_ListDemos, 1,
    NetMsg_SendDemoList, 0,
    NetMsg_GetDemo, 2,
    NetMsg_SendDemo, 0,
    NetMsg_DemoPlayback, 3,
    NetMsg_RecordDemo, 2,
    NetMsg_StopDemo, 1,
    NetMsg_ClearDemos, 2,

    NetMsg_TakeFlag, 3,
    NetMsg_ReturnFlag, 4,
    NetMsg_ResetFlag, 3,
    NetMsg_TryDropFlag, 1,
    NetMsg_DropFlag, 7,
    NetMsg_ScoreFlag, 9,
    NetMsg_InitFlags, 0,

    NetMsg_SayTeam, 0,
    NetMsg_Client, 0,
    NetMsg_AuthTry, 0,
    NetMsg_AuthKick, 0,
    NetMsg_AuthChallenge, 0,
    NetMsg_AuthAnswer, 0,
    NetMsg_ReqAuth, 0,
    NetMsg_PauseGame, 0,
    NetMsg_GameSpeed, 0,
    NetMsg_AddBot, 2,
    NetMsg_DelBot, 1,
    NetMsg_InitAI, 0,
    NetMsg_FromAI, 2,
    NetMsg_BotLimit, 2,
    NetMsg_BotBalance, 2,
    NetMsg_MapCRC, 0,
    NetMsg_CheckMaps, 1,
    NetMsg_SwitchName, 0,
    NetMsg_SwitchModel, 2,
    NetMsg_SwitchColor, 2,
    NetMsg_SwitchTeam, 2,
    NetMsg_ServerCommand, 0,
    NetMsg_DemoPacket, 0,
    -1
};

#define TESSERACT_SERVER_PORT 42069
#define TESSERACT_LANINFO_PORT 42067
#define TESSERACT_MASTER_PORT 42068
#define PROTOCOL_VERSION 2              // bump when protocol changes
#define DEMO_VERSION 1                  // bump when demo format changes
#define DEMO_MAGIC "TESSERACT_DEMO\0\0"

struct demoheader
{
    char magic[16];
    int version, protocol;
};

#define MAXNAMELEN 15

enum
{
    HudIcon_RedFlag = 0,
    HudIcon_BlueFlag,

    HudIcon_Size    = 120,
};



#define VALID_ITEM(n) false //no items in this game thus far

#define MAXRAYS 1
#define EXP_SELFDAMDIV 2
#define EXP_SELFPUSH 2.5f
#define EXP_DISTSCALE 0.5f
// this defines weapon properties
//                                   8            9       14     15    17      19
static const struct attackinfo { int attackdelay, damage, range, rays, exprad, use; } attacks[Attack_NumAttacks] =

//    8     9   14   15  17 19
{
    {  250,  5, 2048, 1,  0, 0 },
    {  500, 10,   14, 1,  0, 0 },
    { 3000, 15, 1024, 1, 50, 0 },
    {  500, 10,   14, 1,  0, 0 },
    {  500,  0,  160, 1, 20, 0 },
    {  500, 10,   14, 1,  0, 0 },
    {   80,  2,  512, 1,  0, 0 },
    {  500,  7,   14, 1,  0, 0 }
};

enum
{
    AI_None = 0,
    AI_Bot,
    AI_Max
};

#define MAXBOTS 32

// inherited by gameent and server clients
struct gamestate
{
    int health, maxhealth;
    int gunselect, gunwait;
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

#define MAXTEAMS 2
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

    static bool compare(const teamscore &x, const teamscore &y)
    {
        if(x.score > y.score) return true;
        if(x.score < y.score) return false;
        return x.team < y.team;
    }
};

inline bool htcmp(int team, const teamscore &t) { return team == t.team; }

struct teaminfo
{
    int frags;

    teaminfo() { reset(); }

    void reset() { frags = 0; }
};

namespace server
{
    extern const char *modename(int n, const char *unknown = "unknown");
    extern const char *modeprettyname(int n, const char *unknown = "unknown");
    extern const char *mastermodename(int n, const char *unknown = "unknown");
    extern void startintermission();
    extern void stopdemo();
    extern void forcemap(const char *map, int mode);
    extern void forcepaused(bool paused);
    extern void forcegamespeed(int speed);
    extern int msgsizelookup(int msg);
    extern bool serveroption(const char *arg);
    extern bool delayspawn(int type);
}

#endif


// this file defines static map entities ("entity") and dynamic entities (players/monsters, "dynent")
// the gamecode extends these types to add game specific functionality

// Ent_*: the only static entity types dictated by the engine... rest are gamecode dependent

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

//extern vector<extentity *> ents;                // map entities

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
    Collide_None = 0,
    Collide_Ellipse,
    Collide_OrientedBoundingBox,
    Collide_TRI
};

enum
{
    Anim_Mapmodel = 0,
    Anim_GameSpecific
};

#define MAXANIMPARTS 3

struct occludequery;
struct ragdolldata;

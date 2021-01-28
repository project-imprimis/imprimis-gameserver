#include "engine.h"

#include <cmath>

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

#include "game.h"

//location for the spawns
vec spawn1 = vec(0,0,0),
    spawn2 = vec(0,0,0);

constexpr int maxgamescore = 500; //score margin at which to end the game

namespace server
{
    extern vector<clientinfo *> clients;
    extern int gamemillis;
    extern teaminfo teaminfos[MAXTEAMS];
}

bool mapcontrolintermission()
{
    if(std::abs(server::teaminfos[0].score - server::teaminfos[1].score) < maxgamescore)
    {
        return false; //keep playing
    }
    else
    {
        return true; //one team has enough points, end the game
    }
}

void clearspawns()
{
    spawn1 = spawn2 = vec(0,0,0);
    for(int i = 0; i < server::clients.length(); ++i)
    {
        server::clientinfo *ci = server::clients[i];
        ci->state.score = 0;
    }
}

void calcscores()
{
    for(int i = 0; i < server::clients.length(); ++i)
    {
        server::clientinfo *ci = server::clients[i];
        vec loc = ci->state.o;
        vec delta1 = spawn1-loc;
        vec delta2 = spawn2-loc;
        int fieldsize = (spawn2 - spawn1).magnitude();
        if(delta2.magnitude() > fieldsize || delta1.magnitude() > fieldsize)
        {
            ci->state.score += 0; //out of battlefield, award no points
        }
        else
        {
            if(ci->team == 1)
            {
                int score = 4*static_cast<int>(delta1.magnitude())/fieldsize;
                ci->state.score += score;
                server::teaminfos[0].score += score; //add score accrued to team the player belongs to
            }
            else
            {
                int score = 4*static_cast<int>(delta2.magnitude())/fieldsize;
                ci->state.score += score;
                server::teaminfos[1].score += score; //add score accrued to team the player belongs to
            }
        }
    }
}

//guess where spawn entities are on the map (server does not hold state of entities or world)
void calibratespawn()
{
    for(int i = 0; i < server::clients.length(); ++i)
    {
        server::clientinfo *ci = server::clients[i];
        if(server::gamemillis - ci->state.lastspawn < 2000) //recently spawned players
        {
            if(ci->team == 1)
            {
                spawn1 = ci->state.o;
            }
            else
            {
                spawn2 = ci->state.o;
            }
        }
    }
}


void sendscore()
{
    for(int i = 0; i < server::clients.length(); i++)
    {
        server::clientinfo *ci = server::clients[i];
        sendf(-1, 1, "iiiii", NetMsg_GetScore, ci->clientnum , ci->state.score, server::teaminfos[0].score, server::teaminfos[1].score);
    }
}
void updatescores()
{
    calibratespawn();
    calcscores();
}

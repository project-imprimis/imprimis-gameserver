#include "engine.h"

#include <cmath>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <algorithm>
#include <vector>

#include <enet/enet.h>
#include <zlib.h>

#include "tools.h"
#include "geom.h"
#include "iengine.h"
#include "igame.h"

#include "game.h"
#include "cserver.h"

//num: number of players to have on the server
//return true if #bots was changed
bool balancebots(int num)
{
    int curnum = server::clients.length();
    //remove from curnum any clients that are spectating
    for(int i = 0; i < server::clients.length(); ++i)
    {
        if(server::clients[i]->state.state == ClientState_Spectator)
        {
            curnum--;
        }
    }
    
    //printf("Number of Clients: %d\n", curnum);
    if(num < curnum)
    {
        for(int i = 0; i < curnum-num; ++i)
        {
            server::aiman::deleteai();
        }
        return true;
    }
    else if(num > curnum)
    {
        //printf("a\n");
        for(int i = 0; i < num-curnum; ++i)
        {
            printf("attempting to add: %d\n", i);
            server::aiman::addai(80, -1);
        }
        return true;
    }
    return false;
}

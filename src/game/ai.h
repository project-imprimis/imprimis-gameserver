struct gameent;

#define MAXBOTS 32

enum
{
    AI_None = 0,
    AI_Bot,
    AI_Max
};

namespace ai
{

    struct avoidset
    {
        struct obstacle
        {
            void *owner;
            int numwaypoints;
            float above;

            obstacle(void *owner, float above = -1) : owner(owner), numwaypoints(0), above(above) {}
        };

        vector<obstacle> obstacles;
        vector<int> waypoints;

        void clear()
        {
            obstacles.setsize(0);
            waypoints.setsize(0);
        }

        void add(void *owner, float above)
        {
            obstacles.add(obstacle(owner, above));
        }

        void add(void *owner, float above, int wp)
        {
            if(obstacles.empty() || owner != obstacles.last().owner)
            {
                add(owner, above);
            }
            obstacles.last().numwaypoints++;
            waypoints.add(wp);
        }

        void add(avoidset &avoid)
        {
            waypoints.put(avoid.waypoints.getbuf(), avoid.waypoints.length());
            for(int i = 0; i < avoid.obstacles.length(); i++)
            {
                obstacle &o = avoid.obstacles[i];
                if(obstacles.empty() || o.owner != obstacles.last().owner) add(o.owner, o.above);
                obstacles.last().numwaypoints += o.numwaypoints;
            }
        }

        void avoidnear(void *owner, float above, const vec &pos, float limit);

        #define LOOP_AVOID(v, d, body) \
            if(!(v).obstacles.empty()) \
            { \
                int cur = 0; \
                for(int i = 0; i < (v).obstacles.length(); i++) \
                { \
                    const ai::avoidset::obstacle &ob = (v).obstacles[i]; \
                    int next = cur + ob.numwaypoints; \
                    if(ob.owner != d) \
                    { \
                        for(; cur < next; cur++) \
                        { \
                            int wp = (v).waypoints[cur]; \
                            body; \
                        } \
                    } \
                    cur = next; \
                } \
            }

        bool find(int n, gameent *d) const
        {
            LOOP_AVOID(*this, d, { if(wp == n) return true; });
            return false;
        }

        int remap(gameent *d, int n, vec &pos, bool retry = false);
    };


    // ai state information for the owner client
    enum
    {
        AIState_Wait = 0,      // waiting for next command
        AIState_Defend,        // defend goal target
        AIState_Pursue,        // pursue goal target
        AIState_Interest,      // interest in goal entity
        AIState_Max,
    };

    enum
    { //renamed to Travel, but "T" could mean something else
        AITravel_Node,
        AITravel_Player,
        AITravel_Affinity,
        AITravel_Entity,
        AITravel_Max,
    };

    struct interest
    {
        int state, node, target, targtype;
        float score;
        interest() : state(-1), node(-1), target(-1), targtype(-1), score(0.f) {}
        ~interest() {}
    };

    struct aistate
    {
        int type, millis, targtype, target, idle;
        bool override;

        aistate(int m, int t, int r = -1, int v = -1) : type(t), millis(m), targtype(r), target(v)
        {
            reset();
        }
        ~aistate() {}

        void reset()
        {
            idle = 0;
            override = false;
        }
    };

    const int NUMPREVNODES = 6;

    struct aiinfo
    {
        vector<aistate> state;
        vector<int> route;
        vec target, spot;
        int enemy, enemyseen, enemymillis, weappref, prevnodes[NUMPREVNODES], targnode, targlast, targtime, targseq,
            lastrun, lasthunt, lastaction, lastcheck, jumpseed, jumprand, blocktime, huntseq, blockseq, lastaimrnd;
        float targyaw, targpitch, views[3], aimrnd[3];
        bool dontmove, becareful, tryreset, trywipe;

        aiinfo()
        {
            clearsetup();
            reset();
            for(int k = 0; k < 3; ++k)
            {
                views[k] = 0.f;
            }
        }
        ~aiinfo() {}

        void clearsetup()
        {
            weappref = Gun_Rail;
            spot = target = vec(0, 0, 0);
            lastaction = lasthunt = lastcheck = enemyseen = enemymillis = blocktime = huntseq = blockseq = targtime = targseq = lastaimrnd = 0;
            lastrun = jumpseed = lastmillis;
            jumprand = lastmillis+5000;
            targnode = targlast = enemy = -1;
        }

        void clear(bool prev = false)
        {
            if(prev) memset(prevnodes, -1, sizeof(prevnodes));
            route.setsize(0);
        }

        void wipe(bool prev = false)
        {
            clear(prev);
            state.setsize(0);
            addstate(AIState_Wait);
            trywipe = false;
        }

        void clean(bool tryit = false)
        {
            if(!tryit)
            {
                becareful = dontmove = false;
            }
            targyaw = randomint(360);
            targpitch = 0.f;
            tryreset = tryit;
        }

        void reset(bool tryit = false) { wipe(); clean(tryit); }

        bool hasprevnode(int n) const
        {
            for(int i = 0; i < NUMPREVNODES; ++i)
            {
                if(prevnodes[i] == n)
                {
                    return true;
                }
            }
            return false;
        }

        void addprevnode(int n)
        {
            if(prevnodes[0] != n)
            {
                memmove(&prevnodes[1], prevnodes, sizeof(prevnodes) - sizeof(prevnodes[0]));
                prevnodes[0] = n;
            }
        }

        aistate &addstate(int t, int r = -1, int v = -1)
        {
            return state.add(aistate(lastmillis, t, r, v));
        }

        void removestate(int index = -1)
        {
            if(index < 0) state.pop();
            else if(state.inrange(index)) state.remove(index);
            if(!state.length()) addstate(AIState_Wait);
        }

        aistate &getstate(int idx = -1)
        {
            if(state.inrange(idx))
            {
                return state[idx];
            }
            return state.last();
        }

        aistate &switchstate(aistate &b, int t, int r = -1, int v = -1)
        {
            if((b.type == t && b.targtype == r) || (b.type == AIState_Interest && b.targtype == AITravel_Node))
            {
                b.millis = lastmillis;
                b.target = v;
                b.reset();
                return b;
            }
            return addstate(t, r, v);
        }
    };

    extern avoidset obstacles;
    extern vec aitarget;
}



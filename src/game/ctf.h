#ifdef SERVERMODE
VAR(ctftkpenalty, 0, 1, 1);

struct ctfservermode : servermode
#else
struct ctfclientmode : clientmode
#endif
{
    static const int MAXFLAGS = 20;
    static const int FLAGRADIUS = 16;
    static const int FLAGLIMIT = 10;
    static const int RESPAWNSECS = 5;

    struct flag
    {
        int id, version;
        vec droploc, spawnloc;
        int team, droptime, owntime;
        int owner, dropcount, dropper;

        flag() : id(-1) { reset(); }

        void reset()
        {
            version = 0;
            droploc = spawnloc = vec(0, 0, 0);
            dropcount = 0;
            owner = dropper = -1;
            owntime = 0;
            team = 0;
            droptime = owntime = 0;
        }
    };

    vector<flag> flags;
    int scores[MAXTEAMS];

    void resetflags()
    {
        flags.shrink(0);
        for(int k = 0; k < MAXTEAMS; ++k)
        {
            scores[k] = 0;
        }
    }
    bool addflag(int i, const vec &o, int team)
    {
        if(i<0 || i>=MAXFLAGS) return false;
        while(flags.length()<=i) flags.add();
        flag &f = flags[i];
        f.id = i;
        f.reset();
        f.team = team;
        f.spawnloc = o;
        return true;
    }
    void ownflag(int i, int owner, int owntime)
    {
        flag &f = flags[i];
        f.owner = owner;
        f.owntime = owntime;
        if(owner == f.dropper) { if(f.dropcount < INT_MAX) f.dropcount++; }
        else f.dropcount = 0;
        f.dropper = -1;
    }

    void dropflag(int i, const vec &o, int droptime, int dropper = -1, bool penalty = false)
    {
        flag &f = flags[i];
        f.droploc = o;
        f.droptime = droptime;
        if(dropper < 0) f.dropcount = 0;
        else if(penalty) f.dropcount = INT_MAX;
        f.dropper = dropper;
        f.owner = -1;
    }

    void returnflag(int i)
    {
        flag &f = flags[i];
        f.droptime = 0;
        f.dropcount = 0;
        f.owner = f.dropper = -1;
    }

    int totalscore(int team)
    {
        return VALID_TEAM(team) ? scores[team-1] : 0;
    }

    int setscore(int team, int score)
    {
        if(VALID_TEAM(team)) return scores[team-1] = score;
        return 0;
    }

    int addscore(int team, int score)
    {
        if(VALID_TEAM(team)) return scores[team-1] += score;
        return 0;
    }

    bool hidefrags() { return true; }

    int getteamscore(int team)
    {
        return totalscore(team);
    }

    void getteamscores(vector<teamscore> &tscores)
    {
        for(int k = 0; k < MAXTEAMS; ++k)
        {
            if(scores[k])
            {
                tscores.add(teamscore(k+1, scores[k]));
            }
        }
    }
    static const int RESETFLAGTIME = 10000;

    bool notgotflags;

    ctfservermode() : notgotflags(false) {}

    void reset(bool empty)
    {
        resetflags();
        notgotflags = !empty;
    }

    void cleanup()
    {
        reset(false);
    }

    void setup()
    {
        reset(false);
        if(notgotitems || ments.empty()) return;
        for(int i = 0; i < ments.length(); i++)
        {
            entity &e = ments[i];
            if(e.type != GamecodeEnt_Flag || !VALID_TEAM(e.attr2)) continue;
            if(!addflag(flags.length(), e.o, e.attr2)) break;
        }
        notgotflags = false;
    }

    void newmap()
    {
        reset(true);
    }

    void dropflag(clientinfo *ci, clientinfo *dropper = NULL)
    {
        if(notgotflags) return;
        for(int i = 0; i < flags.length(); i++)
        {
            if(flags[i].owner==ci->clientnum)
            {
                flag &f = flags[i];
                ivec o(vec(ci->state.o).mul(DMF));
                sendf(-1, 1, "ri7", NetMsg_DropFlag, ci->clientnum, i, ++f.version, o.x, o.y, o.z);
                dropflag(i, vec(o).div(DMF), lastmillis, dropper ? dropper->clientnum : ci->clientnum, dropper && dropper!=ci);
            }
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        dropflag(ci);
        for(int i = 0; i < flags.length(); i++)
        {
            if(flags[i].dropper == ci->clientnum)
            {
                flags[i].dropper = -1;
                flags[i].dropcount = 0;
            }
        }
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        dropflag(ci, ctftkpenalty && actor && actor != ci && modecheck(gamemode, Mode_Team) && actor->team == ci->team ? actor : NULL);
        for(int i = 0; i < flags.length(); i++)
        {
            if(flags[i].dropper == ci->clientnum)
            {
                flags[i].dropper = -1;
                flags[i].dropcount = 0;
            }
        }
    }

    bool canspawn(clientinfo *ci, bool connecting)
    {
        return connecting || !ci->state.lastdeath || gamemillis+curtime-ci->state.lastdeath >= RESPAWNSECS*1000;
    }

    bool canchangeteam(clientinfo *ci, int oldteam, int newteam)
    {
        return true;
    }

    void changeteam(clientinfo *ci, int oldteam, int newteam)
    {
        dropflag(ci);
    }

    void scoreflag(clientinfo *ci, int goal, int relay = -1)
    {
        returnflag(relay >= 0 ? relay : goal);
        ci->state.flags++;
        int team = ci->team, score = addscore(team, 1);
        sendf(-1, 1, "ri9", NetMsg_ScoreFlag, ci->clientnum, relay, relay >= 0 ? ++flags[relay].version : -1, goal, ++flags[goal].version, team, score, ci->state.flags);
        if(score >= FLAGLIMIT) startintermission();
    }

    void takeflag(clientinfo *ci, int i, int version)
    {
        if(notgotflags || !flags.inrange(i) || ci->state.state!=ClientState_Alive || !ci->team) return;
        flag &f = flags[i];
        if(!VALID_TEAM(f.team) || f.owner>=0 || f.version != version || (f.droptime && f.dropper == ci->clientnum && f.dropcount >= 3)) return;
        if(f.team!=ci->team)
        {
            for(int j = 0; j < flags.length(); j++)
            {
                if(flags[j].owner==ci->clientnum)
                {
                    return;
                }
            }
            ownflag(i, ci->clientnum, lastmillis);
            sendf(-1, 1, "ri4", NetMsg_TakeFlag, ci->clientnum, i, ++f.version);
        }
        else if(f.droptime)
        {
            returnflag(i);
            sendf(-1, 1, "ri4", NetMsg_ReturnFlag, ci->clientnum, i, ++f.version);
        }
        else
        {
            for(int j = 0; j < flags.length(); j++)
            {
                if(flags[j].owner==ci->clientnum)
                {
                    scoreflag(ci, i, j);
                    break;
                }
            }
        }
    }

    void update()
    {
        if(gamemillis>=gamelimit || notgotflags) return;
        for(int i = 0; i < flags.length(); i++)
        {
            flag &f = flags[i];
            if(f.owner<0 && f.droptime && lastmillis - f.droptime >= RESETFLAGTIME)
            {
                returnflag(i);
                sendf(-1, 1, "ri3", NetMsg_ResetFlag, i, ++f.version);
            }
        }
    }

    void initclient(clientinfo *ci, packetbuf &p, bool connecting)
    {
        putint(p, NetMsg_InitFlags);
        for(int k = 0; k < 2; ++k)
        {
            putint(p, scores[k]);
        }
        putint(p, flags.length());
        for(int i = 0; i < flags.length(); i++)
        {
            flag &f = flags[i];
            putint(p, f.version);
            putint(p, f.owner);
            if(f.owner<0)
            {
                putint(p, f.droptime ? 1 : 0);
                if(f.droptime)
                {
                    putint(p, int(f.droploc.x*DMF));
                    putint(p, int(f.droploc.y*DMF));
                    putint(p, int(f.droploc.z*DMF));
                }
            }
        }
    }

    void parseflags(ucharbuf &p, bool commit)
    {
        int numflags = getint(p);
        for(int i = 0; i < numflags; ++i)
        {
            int team = getint(p);
            vec o;
            for(int k = 0; k < 3; ++k)
            {
                o[k] = max(getint(p)/DMF, 0.0f);
            }
            if(p.overread())
            {
                break;
            }
            if(commit && notgotflags)
            {
                addflag(i, o, team);
            }
        }
        if(commit && notgotflags)
        {
            notgotflags = false;
        }
    }
};

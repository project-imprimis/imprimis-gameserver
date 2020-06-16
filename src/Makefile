CXXFLAGS= -O3 -fomit-frame-pointer -ffast-math -std=c++17
override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti

INCLUDES= -Ienet/include -Ishared

all: imprimis_gameserver

imprimis_gameserver : engine/command.o engine/server.o engine/worldio.o game/cserver.o shared/crypto.o shared/stream.o shared/tools.o
		g++ $(CXXFLAGS) $(INCLUDES) -o imprimis_gameserver command.o server.o worldio.o cserver.o crypto.o stream.o tools.o -Lenet -lenet -lz

engine/command.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c engine/command.cpp

engine/server.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c engine/server.cpp

engine/worldio.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c engine/worldio.cpp

game/cserver.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c game/cserver.cpp

shared/crypto.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c shared/crypto.cpp

shared/stream.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c shared/stream.cpp

shared/tools.o : engine/engine.h engine/world.h shared/cube.h shared/tools.h shared/geom.h shared/ents.h shared/command.h shared/iengine.h shared/igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c shared/tools.cpp

clean:
		rm command.o server.o worldio.o cserver.o crypto.o stream.o tools.o imprimis_gameserver

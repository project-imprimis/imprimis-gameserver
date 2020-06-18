CXXFLAGS= -O3 -fomit-frame-pointer -ffast-math -std=c++17
override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti

INCLUDES= -Ienet/include -Ishared

all: imprimis_gameserver

imprimis_gameserver : command.o server.o worldio.o cserver.o crypto.o stream.o tools.o
		g++ $(CXXFLAGS) $(INCLUDES) -o imprimis_gameserver command.o server.o worldio.o cserver.o crypto.o stream.o tools.o -Lenet -lenet -lz

command.o : engine.h tools.h geom.h ents.h command.h
		g++ $(CXXFLAGS) $(INCLUDES) -c command.cpp

server.o : engine.h tools.h geom.h ents.h command.h iengine.h igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c server.cpp

worldio.o : engine.h world.h tools.h geom.h ents.h command.h iengine.h igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c worldio.cpp

cserver.o : engine.h game.h world.h tools.h geom.h ents.h command.h iengine.h igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c cserver.cpp

crypto.o : tools.h
		g++ $(CXXFLAGS) $(INCLUDES) -c crypto.cpp

stream.o : tools.h
		g++ $(CXXFLAGS) $(INCLUDES) -c stream.cpp

tools.o : engine.h tools.h geom.h command.h igame.h
		g++ $(CXXFLAGS) $(INCLUDES) -c tools.cpp

clean:
		rm -f command.o server.o worldio.o cserver.o crypto.o stream.o tools.o imprimis_gameserver

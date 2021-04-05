CXXFLAGS= -O3 -fomit-frame-pointer -ffast-math -std=c++17
override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti -ggdb3

INCLUDES= -Ienet/include -Ishared

all: imprimis_gameserver

imprimis_gameserver : command.o demo.o server.o cserver.o mapcontrol.o stream.o tools.o
		g++ $(CXXFLAGS) $(INCLUDES) -o imprimis_gameserver command.o server.o cserver.o mapcontrol.o stream.o tools.o demo.o -Lenet -lenet -lz

command.o :
		g++ $(CXXFLAGS) $(INCLUDES) -c command.cpp

demo.o :
		g++ $(CXXFLAGS) $(INCLUDES) -c demo.cpp

server.o :
		g++ $(CXXFLAGS) $(INCLUDES) -c server.cpp

cserver.o :
		g++ $(CXXFLAGS) $(INCLUDES) -c cserver.cpp

mapcontrol.o :
		g++ $(CXXFLAGS) $(INCLUDES) -c mapcontrol.cpp

crypto.o : tools.h
		g++ $(CXXFLAGS) $(INCLUDES) -c crypto.cpp

stream.o : tools.h
		g++ $(CXXFLAGS) $(INCLUDES) -c stream.cpp

tools.o :
		g++ $(CXXFLAGS) $(INCLUDES) -c tools.cpp

clean:
		rm -f command.o server.o demo.o worldio.o cserver.o crypto.o mapcontrol.o stream.o tools.o imprimis_gameserver

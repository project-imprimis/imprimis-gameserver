# Imprimis Dedicated Server

Written and © Alex "no-lex" Foster 2020, released CC0/WTFPL.

The Imprimis server is the only way to host multiplayer games using Imprimis. It
is a separate standalone from the game, unlike other Cube games, but otherwise
has similar functionality.

## Compilation Dependencies

* zlib1g-dev
* make
* g++

## Compilation Instructions

1. Clone the repository.
```bash
git clone https://github.com/project-imprimis/imprimis-gameserver.git --recurse-submodules
```
2. Go to the `enet` library folder and compile `enet`.
```bash 
cd imprimis-gameserver/src/enet/ && make
```
3. Go to `src` folder and compile the gameserver.
```bash 
cd .. && make
```
4. Execute the gameserver.
```bash 
./imprimis_gameserver
```

Note that the `enet` library has to be compiled first, and the gameserver
afterwards. The gameserver executable `imprimis_gameserver` can be run in
the command line to create a game server.


## Hardware Requirements

The hardware required to run Imprimis' server is rather trivial: modern single
board computers are easily adequate to run Imprimis' server with sane client
numbers. This is due to the clients, not the server, calculating the paths of
entities on the level.

It is highly preferable, though not strictly necessary, to run the server on a
stable broadband Ethernet connection. Wireless internet sources or wireless
local connections lead to latency inconsistencies which cannot be resolved by
server configuration, leading to inconsistent gameplay experiences for players.

## Overview

Imprimis' client-server model utilizes the game itself (the client) and the
server (this program). The Imprimis server is very lightweight and does not
actually modulate much of the gameplay, acting more as a packet multicaster
between clients which themselves largely control gameplay than as a centralized
source for information. While this topology is not the most secure possible,
it is a very responsive architecture that makes gameplay feel nearly local.

Imprimis' server uses the `enet` library to send packets over the UDP protocol,
which is, unlike the typical TCP that sends webpages and most other traffic, a
protocol that does not verify that packets make it to its destination. UDP,
however, loses the overhead associated with making sure packets arrive, lowering
its overhead and latency significantly.

The client-server model functions by having clients have domain over particular
parts of the dynamic game (e.g. their own weapons, player locations, etc.) and
being responsible for broadcasting updates for these entities. The server then
tells the other clients on the game about these changes, which then apply this
to their own clients.

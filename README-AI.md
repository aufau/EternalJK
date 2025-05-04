# OpenJK AI Integration

This OpenJK fork adds interfaces facilitating real-time integration
with AI agents.

## Compilation

Follow standard OpenJK compilation instructions. For AI training it is
handy to build with BuildPortableVersion=ON CMake option. With this
option all files will be read and written only in current directory
and its sub-directories.

## Game Architecture Primer

OpenJK is is extension of Id Tech 3 game engine and implements a
server-authoritative server-client game architecture.

See https://fabiensanglard.net/quake3/ for some details, but best
consult with experts because it doesn't give full explanation.

## Multiplayer Client Integration

AI agent integration is implemented in asynchronous mode, which means
that game client and AI agent run at independent frame rates.

Currently AI agent can read files updated by the game client to learn
about the game state and send commands to the client via UDP or TCP
socket.

### player_perspective.ndjson

*player_perspective.ndjson* file is created when game client connects
to a server and located in current fs_game directory (normally
*base/*). The file is rewritten whenever client receives new snapshot
from game server. On default server settings that is 40 times per
second, although various circumstances may affect it. It contains last
10 newline-delimited json objects with client snapshot data. First
line in the file contains latest received snapshot.

Client snapshots include all the data about *player state* and game
*entities* that is normally used to present them to the
player. Entities include other players, moving objects, some effects
and events like sparks, sounds etc. Player state contains more
detailed information about current player than what can be found in
his entity like health and armor.

Client snapshots don't include any information about map geometry.
Specific data about assets like 3d model vertices, sound waves etc. is
not included in each snapshot, only referenced and actual data is read
from assets in .pk3 files.

All keys in snapshot json object and embedded objects are required and
do not change. Size of "entities" array is encoded in "numEntities"
field. Maximum size is 256.

Snapshot object contains "aiMessageNumber" field. It contains latest AI
agent message number from AI TCP/UDP endpoint that affected this
snapshot (explained later).

### action_data.ndjson

*action_data.ndjson* file is appended each client frame with all new
input events. Input events include keyboard and mouse, there may be
multiple events per frame. It can be used to record human player or AI
agent input events.

### AI TCP/UDP Endpoint

Game client provides TCP and UDP endpoints listening at port 29010. If
port 29010 is occupied by another process, consecutive port numbers
will be used. Port is displayed in game window title and can be
examined with *net_ai_port* cvar. Currently it is recommended to use
the UDP endpoint to avoid TCP buffering.

These wait for messages in non-blocking manner. On UDP endpoints each
datagram constitutes a message, on TCP endpoint messages must be
delimited by a newline character. Message body must be in the following
format:

```
1;<message number>;<event type number>;<value 1>;<value 2>
```

Initial 1 is protocol version - currently always 1. \<message number\>
must start at 1 and be incremented by 1 with each message sent.

There are 2 event types supported:

```
1;<message number>;1;<key code>;<down>
```

This type sends a keyboard button press/release event. \<key code\> is
fakeAscii_t enum integer value (ASCII code for letters). \<down\> is 0
when button should be released and 1 when it should be pressed down.

```
1;<message number>;3;<dx>;<dy>
```

This type sends a mouse move event. \<dx\> is integer containing mouse
position delta in X axis. \<dy\> corresponds to Y axis.

### Cvars

There are following new cvars:

- *net_ai_enabled* - Enable AI endpoint to listen on TCP/UDP ports
- *net_ai_ip* - Network interface that should be used for AI endpoint
  (localhost or 0.0.0.0 mean listening on all interfaces, 127.0.0.1 -
  only on loopback)
- *net_ai_port* - Port that should be used for AI endpoint
- *ai_debugMsg* - AI messages Debug level. 1 will print protocol
  warning (dropped, reordered, duplicate messages). 2 will print out each
  received message in console

Additionally you may find these standard cvars useful:

- *r_backendDisable 1* - disable rendering scene relieving GPU load
* *in_mouse 0* - disable mouse input completely. AI events remain enabled.

## Recording game sessions

Game engine provides two mechanisms for recording and replaying game
sessions.

### Demos

Demos record a stream of snapshots but no input events. They can be
used to replay game session.

### Journaling

Journaling can record all game engine inputs, allowing to completely
replay and reproduce game engine state at any point. These include
mouse/keyboard events, files read from filesystem, network data.

To record launch the game with `+set journal 1`. To replay launch the
game with `+set journal 2`.

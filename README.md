# DiabloLodMapDll

Slim internal maphack for **Diablo II: Lord of Destruction 1.14d**,
offline / single-player only. A single module, loaded into the game with your
injector:

| Piece | What it does |
|---|---|
| `DiabloLodMapDll.dll` | one thread that (a) reveals the whole current area through the game's own automap code on every area change, (b) injects chest / object / exit marker cells into the automap, (c) draws monsters (immunity-colored crosses, boss names) and exit/area labels on the open automap via the game's own draw functions |

No external reader, no GDI overlay, no signature scanning, no offsets-file
handshake, no console. Everything is drawn by the game itself, so it works at
any zoom and needs no pixel alignment.

Modeled on gonzoj's **snoogans** maphack (`unseenstorm/D2linuxhack`, GPLv3):
same reveal flow (`init_automap_layer` -> per-room2
`add_room_data`/`reveal_automap_room`/`remove_room_data`, preset cells via
`new_automap_cell`/`add_automap_cell`, per-frame automap drawing), ported to a
native Windows DLL and trimmed to the maphack core.

## Build

Visual Studio 2022 (v145 toolset, C++20). Build **Release | Win32** (the game
is 32-bit). Output: `Release\DiabloLodMapDll.dll` (or `x64\...` if you only
build the DLL project without switching to Win32 — don't; x64 is empty).

## Use

1. Start Diablo II: LoD 1.14d and enter a **single-player** game
   (LaunchD2Windowed.bat starts it windowed, which is easiest to test).
2. Load the DLL with your injector (CreateRemoteThread + LoadLibrary style).
   `DLL_PROCESS_ATTACH` starts the module thread.
3. Open the automap (Tab): the whole area is revealed at once — walls,
   chests / objects / warps get the game's own icons, exits are labeled with
   their destination area name (the real Tal Rasha's tomb shows green), and
   monsters appear as crosses: white normal, colored by immunity (gold =
   physical, blue = cold, red = fire, yellow = lightning, green = poison,
   orange = magic), white cross + gold name for bosses.
4. Diagnostics: `%TEMP%\diablolodmap.log` — every reveal, the chosen data
   layout, and any faulted/disabled feature are logged there. Paste it when
   something misbehaves.

## How it stays safe

- All offsets are RVAs against the main module (`Game.exe` — 1.14 merged the
  game DLLs into it). Functions are byte-checked before first call, every
  game call is SEH-netted, and a feature that faults repeatedly disables
  itself. A wrong offset can never crash the game.
- Two 1.14d data-region sub-builds exist (globals shifted by -0x7F78); the
  module picks the layout whose player slot is live instead of scanning.
- The whole-act *force generation* trick (`init_level` on far levels) is
  deliberately **off**: earlier builds froze the game when this thread called
  it. Generated levels are revealed as the game makes them.

## Known weak spot

The per-frame automap drawing calls (`D2Gfx.draw_line`, `D2Win.draw_text`)
are the least-verified calls in the module — snoogans' tables, but not yet
confirmed live on a Windows 1.14d run. If the map opens but no monsters /
labels appear and the log shows repeated draw faults, the calling convention
of those two functions needs flipping (`call.cpp` + `automap.cpp`).

## Provenance / license

Offsets and struct layouts: snoogans (`unseenstorm/D2linuxhack`) + the
IDA-verified work of the earlier codebase in this repo. This module is a
close port of GPLv3 snoogans code, so the project should ship under the GPL
v3 (or later); no LICENSE file yet — decide before distributing.

Retired code stays on disk for reference and is **not** built:
`main.cpp`, `Core/` (external reader), `Drawing/` (GDI overlay + ASCII map),
`Reveal/` (old two-thread DLL — its `GameHook.*` files were deleted mid
refactor, which is why that tree no longer compiled), `reference/`
(D2linuxhack clone used for this rewrite).

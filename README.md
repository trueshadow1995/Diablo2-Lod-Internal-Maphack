# DiabloLodMapDll

Slim internal maphack for **Diablo II: Lord of Destruction 1.14d**


| Piece | What it does |
|---|---|
| `DiabloLodMapDll.dll` | one thread that (a) reveals the whole current area through the game's own automap code on every area change, (b) injects chest / object / exit marker cells into the automap, (c) draws monsters (immunity-colored crosses, boss names) and exit/area labels on the open automap via the game's own draw functions |

 Everything is drawn by the game itself, so it works at
any zoom and needs no pixel alignment.

Modeled on gonzoj's **snoogans** maphack (`unseenstorm/D2linuxhack`, GPLv3):
same reveal flow (`init_automap_layer` -> per-room2
`add_room_data`/`reveal_automap_room`/`remove_room_data`, preset cells via
`new_automap_cell`/`add_automap_cell`, per-frame automap drawing), ported to a
native Windows DLL and trimmed to the maphack core.


## Use

1. Start Diablo II: LoD 1.14d and enter a **single-player** game
   (LaunchD2Windowed.bat starts it windowed, which is easiest to test).
   
3. Load the DLL with your injector (CreateRemoteThread + LoadLibrary style).
   `DLL_PROCESS_ATTACH` starts the module thread.
   
4. Open the automap (Tab): the whole area is revealed at once — walls,
   chests / objects / warps get the game's own icons, exits are labeled with
   their destination area name (the real Tal Rasha's tomb shows green), and
   monsters appear as crosses: white normal, colored by immunity (gold =
   physical, blue = cold, red = fire, yellow = lightning, green = poison,
   orange = magic), white cross + gold name for bosses.
   
5. Diagnostics: `%TEMP%\diablolodmap.log` every reveal, the chosen data
   layout, and any faulted/disabled feature are logged there.



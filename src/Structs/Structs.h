/*
  mh.h -- shared facts + API for the slim maphack module.

  One injected DLL (DiabloLodMapDll.dll), modeled on gonzoj's "snoogans"
  maphack (unseenstorm/D2linuxhack, GPLv3) as a native Windows module for
  Diablo II: LoD 1.14d, offline/single-player only.

  1.14d merged the old game DLLs (D2Client/D2Common/D2Gfx/D2Win/D2Lang)
  into Game.exe, so every function/variable offset below is an RVA against
  the main module base and one base serves all five "modules".

  Function offsets: community-verified 1.14d set (snoogans d2pointers.c,
  confirmed against IDA and by live in-game runs of the previous code --
  RevealLevel 35/35 rooms etc.).

  Global-data offsets: two sub-builds of 1.14d exist in the wild.  The
  published data region (player slot 0x3A6A70, automap vars around 0x3A5xxx)
  is the common one; some installs shift the whole data region by -0x7F78
  (player slot 0x39EAF8).  The module picks the set that actually holds a
  live player unit.  Code (functions) is identical across both.

  All game calls are SEH-netted and every function pointer is byte-checked
  before first use, so a wrong offset disables itself instead of crashing
  the game (same contract as the old RevealCore).
*/
#pragma once
#include <cstdint>

#define MH_BUILD_TAG "s1"

/* struct member offsets (1.13c == 1.14d, byte-identical to snoogans) */
namespace off
{
    // unit_any
    constexpr uint32_t U_TYPE       = 0x00; // 0 = player, 1 = monster, 2 = object, 3 = missile
    constexpr uint32_t U_TXTFILE    = 0x04;
    constexpr uint32_t U_ID         = 0x0C;
    constexpr uint32_t U_MODE       = 0x10;
    constexpr uint32_t U_DATA       = 0x14; // monster_data / player_data ...
    constexpr uint32_t U_ACTNO      = 0x18;
    constexpr uint32_t U_ACT        = 0x1C;
    constexpr uint32_t U_PATH       = 0x2C;
    constexpr uint32_t U_LISTNEXT   = 0xE8; // unit.list_next (room unit list)

    // path
    constexpr uint32_t P_X          = 0x02; // u16 world x
    constexpr uint32_t P_Y          = 0x06; // u16 world y
    constexpr uint32_t P_ROOM1      = 0x1C;

    // monster_data flags byte (boss/champ/minion bitfield at byte 0x16)
    constexpr uint32_t MD_FLAGS     = 0x16;
    constexpr uint32_t MD_BOSS      = 0x08; // bit3
    constexpr uint32_t MD_CHAMP     = 0x04; // bit2

    // room1
    constexpr uint32_t R1_ROOM2     = 0x10;
    constexpr uint32_t R1_ACT       = 0x2C;
    constexpr uint32_t R1_UNITFIRST = 0x74;
    constexpr uint32_t R1_NEXT      = 0x7C;

    // room2
    constexpr uint32_t R2_NEXT      = 0x24;
    constexpr uint32_t R2_ROOM1     = 0x30;
    constexpr uint32_t R2_X         = 0x34; // room grid index (1 tile = 5 world)
    constexpr uint32_t R2_Y         = 0x38;
    constexpr uint32_t R2_ROOMTILES = 0x4C;
    constexpr uint32_t R2_LEVEL     = 0x58;
    constexpr uint32_t R2_PRESET    = 0x5C;

    // level
    constexpr uint32_t L_ROOM2FIRST = 0x10;
    constexpr uint32_t L_NEXT       = 0x1AC;
    constexpr uint32_t L_MISC       = 0x1B4;
    constexpr uint32_t L_LEVELNO    = 0x1D0;

    // act / act_misc
    constexpr uint32_t A_ROOM1      = 0x10;
    constexpr uint32_t A_MISC       = 0x48;
    constexpr uint32_t AM_STAFFTOMB = 0x94;
    constexpr uint32_t AM_ACT       = 0x46C;
    constexpr uint32_t AM_LVLFIRST  = 0x47C;

    // automap layer / cell
    constexpr uint32_t AL_LAYERNO   = 0x00;
    constexpr uint32_t AL_OBJECTS   = 0x10; // RB-tree root of object cells
    constexpr uint32_t AC_CELLNO    = 0x04; // u16
    constexpr uint32_t AC_PIXELX    = 0x06; // u16
    constexpr uint32_t AC_PIXELY    = 0x08; // u16
    constexpr uint32_t AC_LESS      = 0x0C;
    constexpr uint32_t AC_MORE      = 0x10;

    // preset_unit (room2.preset list)
    constexpr uint32_t PS_TXTFILE   = 0x04;
    constexpr uint32_t PS_X         = 0x08;
    constexpr uint32_t PS_NEXT      = 0x0C;
    constexpr uint32_t PS_TYPE      = 0x14; // 1 = monster, 2 = object, 5 = warp tile
    constexpr uint32_t PS_Y         = 0x18;

    // room_tile (room2.roomTiles list; exit pairing key is *numPtr == warp txt)
    constexpr uint32_t RT_ROOM2     = 0x00; // room2* in the DESTINATION level
    constexpr uint32_t RT_NEXT      = 0x04;
    constexpr uint32_t RT_NUMPTR    = 0x10; // DWORD* warp number

    // object_txt automap field (objects.txt record)
    constexpr uint32_t OT_AUTOMAP   = 0x1BC;

    // item_data (unit->pUnitData for type==4 items)
    constexpr uint32_t IT_QUALITY   = 0x00; // DWORD quality (1=low,2=normal,3=high,4=magic,5=set,6=rare,7=unique,8=crafted)
    constexpr uint32_t IT_FLAGS     = 0x18; // DWORD flags (0x400000 = ethereal)
}

/* resolved function set (absolute addresses once armed) */
struct Fn
{
    uint32_t initLayer      = 0; // 0x58D40  arg->EAX/ECX/EDI, ret automap_layer*
    uint32_t revealRoom     = 0; // 0x58F40  stdcall(room1*, clip=1, layer)
    uint32_t addRoom        = 0; // 0x21A070 stdcall(act, levelNo, x, y, anchorRoom1)
    uint32_t removeRoom     = 0; // 0x21A0C0 stdcall(same)
    uint32_t newCell        = 0; // 0x57C30  fastcall() -> automap_cell*
    uint32_t addCell        = 0; // 0x57B00  fastcall(cell, &treeRoot)
    uint32_t getObjectTxt   = 0; // 0x240E90 stdcall(objects.txt id) -> object_txt*
    uint32_t getUnitName    = 0; // 0x64A60  reg(EAX=unit) -> wchar*
    uint32_t getUnitStat    = 0; // 0x225480 stdcall(unit, stat, 0) -> dword
    uint32_t printGame      = 0; // 0x9E3A0  msg+color (stdcall, fallback fastcall)
    uint32_t drawLine       = 0; // 0xF6380  __stdcall(x1,y1,x2,y2,color,unk) D2GFX vtable
    uint32_t drawText       = 0; // 0x102320 __fastcall(ecx,edx,stackx3) D2WIN text
    uint32_t setTextSize    = 0; // 0x102EF0 fastcall(size)
    uint32_t getTextWidth   = 0; // 0x102520 fastcall(text,&width,&fileNo)
    uint32_t automapSize    = 0; // 0x5A710  stdcall() -> dword (0 = small map)
    uint32_t getLayer       = 0; // 0x21E470 fastcall(levelNo) -> automap_layer2*
    uint32_t revealCallback = 0; // 0x459150 stdcall(room1*) -- game's own reveal
};

/* data-region candidates (absolute once armed) */
struct Vars
{
    uint32_t player    = 0; // -> unit_any* current player
    uint32_t layer     = 0; // -> automap_layer* active layer
    uint32_t automapOn = 0; // -> dword  (0 = automap closed)
    uint32_t divisor   = 0; // -> int    (automap zoom divisor)
    uint32_t offset    = 0; // -> POINT  (automap pan)
    uint32_t scrollX   = 0; // -> int    (automap scroll X, per-frame)
    uint32_t scrollY   = 0; // -> int    (automap scroll Y, per-frame)
};

/* globals (defined in module.cpp) */
extern uint32_t g_base;     // main module base (Game.exe)
extern bool     g_armed;    // true once functions verified + data set chosen
extern Fn       g_fn;       // absolute function addresses
extern Vars     g_var;      // absolute data addresses (chosen layout)
extern int      g_dataSet;  // 0 = published layout, 1 = shifted (-0x7F78)

/* exit / warp info collected during reveal (labels drawn on the map) */
struct ExitInfo
{
    int      act;        // act that owns the warp
    uint32_t levelId;    // DESTINATION level no (label text = AreaName)
    int      x;          // world x of the warp (label anchor)
    int      y;          // world y of the warp
};

extern ExitInfo g_exits[96];
extern int      g_exitCount;

/* feature toggles (overlay UI + hotkeys) */
struct Settings
{
    bool showPanel   = false;  // INSERT toggles the settings panel
    bool itemFilter  = true;   // hide junk, show only good drops
    bool goldMarkers = true;   // highlight gold piles
    bool shrineMarkers = false; // show shrine locations (future)
    bool monsterHP   = false;  // monster health bars (future)
    bool questMarkers = false; // quest item markers (future)
};

extern Settings g_settings;

// Per-frame automap drawing (monsters, exit labels). Defined in automap.cpp.
void AmDrawTick();

// Settings overlay panel. Defined in overlay.cpp.
void OverlayTick();

// x86 call trampolines (call.cpp)
uint32_t CallFn0(uint32_t fn);
uint32_t CallFn1(uint32_t fn, uint32_t a);
uint32_t CallFn2(uint32_t fn, uint32_t a, uint32_t b);
uint32_t CallFn3(uint32_t fn, uint32_t a, uint32_t b, uint32_t c);
uint32_t CallFn5(uint32_t fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t CallFn6(uint32_t fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);
uint32_t CallFast1(uint32_t fn, uint32_t a);
uint32_t CallFast2(uint32_t fn, uint32_t a, uint32_t b);
uint32_t CallFast3(uint32_t fn, uint32_t a, uint32_t b, uint32_t c);
uint32_t CallFast4(uint32_t fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
uint32_t CallFast5(uint32_t fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
uint32_t CallEax1(uint32_t fn, uint32_t a);
uint32_t CallInitLayer(uint32_t fn, uint32_t levelNo);
uint32_t CallRevealRoom(uint32_t fn, uint32_t room1, uint32_t clip, uint32_t layer);

// Pattern scanner (scanner.cpp)
bool ScanAllPatterns(Fn& fn);
bool ScanDataPatterns(uint32_t base, const uint8_t* code, int codeLen, Vars& var);

// Map reveal (reveal.cpp)
void RevealAct(uint32_t player, bool fullAct);
void ResetDone();

// Debug console (Console.cpp)
void ConsoleStart();

// Logging to %TEMP%\diablolodmap.log (never touches game files).
void Dbg(const char* fmt, ...);

// player -> path -> room1 -> room2 -> level (0 when not in a game).
uint32_t GetPlayerLevelAddr(uint32_t player);

// player -> act number (0-4) or -1 on failure.
int PlayerAct(uint32_t player);

// Raw / netted 32-bit reads (game pointers live below 4 GB).
uint32_t Rd(uint32_t addr);
uint32_t SRead(uint32_t addr);          // SEH-netted read
bool     IsReadable(uint32_t addr);
bool     LooksCode(uint32_t addr);      // inside image, not padding

// Game message (gold). Never fatal: netted, self-disables on repeated faults.
void PrintGameMsg(const wchar_t* text);

// Module tick (module.cpp) -- called from ModuleThread
void ModuleTick();

// Thread (main.cpp)
unsigned long __stdcall ModuleThread(void*);
bool ModuleShouldRun();

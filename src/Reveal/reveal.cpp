#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>

#include "Structs.h"
#include "AreaNames.h"

extern Fn       g_fn;
extern ExitInfo g_exits[];
extern int      g_exitCount;
extern uint32_t g_base;

// preset cell injection + exit info
static void PresetRoom(uint32_t r2, int actNo, uint32_t layer)
{
    // object / monster preset cells
    for (uint32_t ps = SRead(r2 + off::R2_PRESET); ps;
         ps = SRead(ps + off::PS_NEXT))
    {
        int      cellNo = -1;
        uint32_t txt    = SRead(ps + off::PS_TXTFILE);
        uint32_t type   = SRead(ps + off::PS_TYPE);

        if (type == 1)
        {
            if (txt == 256)       cellNo = 300;   // special quest monster icons
            else if (txt == 745)  cellNo = 745;
        }
        else if (type == 2)
        {
            uint32_t lvlNo = 0;
            uint32_t lvl  = SRead(r2 + off::R2_LEVEL);
            if (lvl)
                lvlNo = SRead(lvl + off::L_LEVELNO);
            if (txt == 580 && lvlNo == 79)       cellNo = 318;  // LK superchests
            else if (txt == 371 && lvlNo == 25)  cellNo = 301;  // tower cellar 5
            else if (txt == 152)                 cellNo = 300;
            else if (txt == 460)                 cellNo = 1468; // chests
            else if (txt == 402 && lvlNo == 46)  cellNo = 0;    // canyon portal
            else if (txt == 267 && lvlNo != 75 && lvlNo != 103) cellNo = 0;
            else if (txt == 367 && lvlNo == 107) cellNo = 376;  // river of flame

            if (cellNo == -1)
            {
                if (txt > 572)
                {
                    cellNo = 0;
                }
                else
                {
                    uint32_t ot = 0;
                    __try { ot = CallFn1(g_fn.getObjectTxt, txt); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { ot = 0; }
                    if (ot && IsReadable(ot + off::OT_AUTOMAP))
                    {
                        uint32_t am = Rd(ot + off::OT_AUTOMAP);
                        if (am == 310)
                        {
                            uint8_t sub = *(volatile uint8_t*)(uintptr_t)(ot + 0x166);
                            cellNo = (sub & 1) ? 310 : 0;
                        }
                        else
                        {
                            cellNo = (int)am;
                        }
                    }
                }
            }
        }

        if (cellNo > 0 && layer)
        {
            int wx = (int)SRead(ps + off::PS_X) + (int)SRead(r2 + off::R2_X) * 5;
            int wy = (int)SRead(ps + off::PS_Y) + (int)SRead(r2 + off::R2_Y) * 5;
            uint32_t cell = 0;
            __try { cell = CallFast1(g_fn.newCell, 0); }
            __except (EXCEPTION_EXECUTE_HANDLER) { cell = 0; }
            if (cell && cell >= 0x10000 && cell < 0x80000000)
            {
                *(volatile uint16_t*)(uintptr_t)(cell + off::AC_CELLNO) = (uint16_t)cellNo;
                *(volatile uint16_t*)(uintptr_t)(cell + off::AC_PIXELX) = (uint16_t)((wx - wy) * 16 / 10 + 1);
                *(volatile uint16_t*)(uintptr_t)(cell + off::AC_PIXELY) = (uint16_t)((wx + wy) * 8 / 10 - 3);
                __try
                {
                    CallFast2(g_fn.addCell, cell, layer + off::AL_OBJECTS);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) { }
            }
        }
    }

    // warp tiles remember them for the exit labels drawn each frame
    if (g_exitCount < (int)(sizeof(g_exits) / sizeof(g_exits[0])))
    {
        int rx = (int)SRead(r2 + off::R2_X);
        int ry = (int)SRead(r2 + off::R2_Y);
        for (uint32_t ps = SRead(r2 + off::R2_PRESET); ps && g_exitCount < 96;
             ps = SRead(ps + off::PS_NEXT))
        {
            if (SRead(ps + off::PS_TYPE) != 5)
                continue;
            uint32_t txt = SRead(ps + off::PS_TXTFILE);
            for (uint32_t tile = SRead(r2 + off::R2_ROOMTILES); tile;
                 tile = SRead(tile + off::RT_NEXT))
            {
                uint32_t numPtr = SRead(tile + off::RT_NUMPTR);
                uint32_t num    = numPtr ? SRead(numPtr) : 0;
                if (num != txt)
                    continue;
                uint32_t dstR2  = SRead(tile + off::RT_ROOM2);
                uint32_t dstLvl = dstR2 ? SRead(dstR2 + off::R2_LEVEL) : 0;
                uint32_t dstNo  = dstLvl ? SRead(dstLvl + off::L_LEVELNO) : 0;
                if (!dstNo)
                    break;
                ExitInfo& e = g_exits[g_exitCount++];
                e.act    = actNo;
                e.levelId = dstNo;
                e.x = (int)SRead(ps + off::PS_X) + rx * 5 - 16;
                e.y = (int)SRead(ps + off::PS_Y) + ry * 5 - 10;
                break;
            }
        }
    }
}

// reveal one level snoogans reveal_level + draw_presets
static int  s_actSeen = -1;
static bool s_lvlDone[256];

void ResetDone()
{
    s_actSeen = -1;
    memset(s_lvlDone, 0, sizeof(s_lvlDone));
}

static void RevealLevel(uint32_t levelAddr, uint32_t player, int actNo)
{
    uint32_t levelNo = SRead(levelAddr + off::L_LEVELNO);
    if (!levelNo)
        return;
    if (levelNo < 256 && s_lvlDone[levelNo])
        return;

    if (!g_fn.revealCallback)
    {
        Dbg("RevealLevel %u: revealCallback not found", levelNo);
        return;
    }

    uint32_t act = 0;
    {
        uint32_t misc = SRead(levelAddr + off::L_MISC);
        act = misc ? SRead(misc + off::AM_ACT) : 0;
    }
    if (!act)
        act = SRead(player + off::U_ACT);

    uint32_t anchor = 0;
    {
        uint32_t path = player ? SRead(player + off::U_PATH) : 0;
        anchor = path ? SRead(path + off::P_ROOM1) : 0;
    }
    if (!anchor)
        return;

    uint32_t r2   = SRead(levelAddr + off::L_ROOM2FIRST);
    uint32_t n    = 0;
    uint32_t done = 0;
    uint32_t made = 0;

    while (r2 && n++ < 4096)
    {
        uint32_t next = SRead(r2 + off::R2_NEXT);
        uint32_t x    = SRead(r2 + off::R2_X);
        uint32_t y    = SRead(r2 + off::R2_Y);
        uint32_t room1 = SRead(r2 + off::R2_ROOM1);
        bool added = false;
        if (!room1 && g_fn.addRoom && act)
        {
            __try
            {
                CallFn5(g_fn.addRoom, act, levelNo, x, y, anchor);
                room1 = SRead(r2 + off::R2_ROOM1);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { room1 = 0; }
            if (room1)
            {
                added = true;
                made++;
            }
        }
        if (room1)
        {
            __try
            {
                CallFn1(g_fn.revealCallback, room1);
                done++;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { }
            PresetRoom(r2, actNo, SRead(0x7A5164));
        }
        if (added && g_fn.removeRoom)
        {
            __try { CallFn5(g_fn.removeRoom, act, levelNo, x, y, anchor); }
            __except (EXCEPTION_EXECUTE_HANDLER) { }
        }
        r2 = next;
    }

    Dbg("RevealLevel %u (%s): %u rooms revealed (%u materialized)",
        levelNo, AreaName(levelNo), done, made);
    if (levelNo < 256)
        s_lvlDone[levelNo] = true;
}

// First town level of each act
static const int kTownLevel[6] = { 1, 40, 75, 103, 109, 137 };

void RevealAct(uint32_t player, bool fullAct)
{
    int actNo = PlayerAct(player);
    if (actNo < 0)
        return;
    uint32_t cur = GetPlayerLevelAddr(player);
    if (!cur)
        return;

    RevealLevel(cur, player, actNo);

    if (fullAct)
    {
        uint32_t misc = SRead(SRead(player + off::U_ACT) + off::A_MISC);
        uint32_t lv   = misc ? SRead(misc + off::AM_LVLFIRST) : 0;
        int lo = kTownLevel[actNo] + 1;
        int hi = kTownLevel[actNo + 1];
        int walked = 0;
        while (lv && walked++ < 512)
        {
            uint32_t no   = SRead(lv + off::L_LEVELNO);
            uint32_t next = SRead(lv + off::L_NEXT);
            if (no >= (uint32_t)lo && no < (uint32_t)hi && no != 132 &&
                !s_lvlDone[no] && SRead(lv + off::L_ROOM2FIRST))
            {
                RevealLevel(lv, player, actNo);
            }
            if (!next)
                break;
            lv = next;
        }
    }
}

/*
  automap.cpp -- drawing on the game's own automap (snoogans style).

  While the in-game automap is open, every tick we draw through the game's
  own D2Gfx/D2Win functions:
    * monster crosses -- color coded by immunity, white + name for bosses
    * exit / area name labels over warp tiles (correct Tal Rasha's tomb
      green), from the exit info collected during reveal (module.cpp)

  Coordinates: the game projects automap px from world coords with the
  divisor/offset globals (snoogans' screen_to_automap).  The drawing calls
  are SEH-netted and the feature self-disables after repeated faults, so a
  wrong offset or convention can never crash the game -- it only turns the
  overlay off (reveal + preset cells keep working).

  Note: the draw-text/draw-line conventions on the merged 1.14d build are
  the least-verified calls in the module (they come from snoogans' tables
  without a live Windows run).  If the map opens but nothing extra shows
  and the log reports faults, the conventions need flipping -- that is the
  documented first thing to try.
*/
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>

#include <cstring>

#include "Structs.h"
#include "w2s.h"
#include "AreaNames.h"

// Cross glyph line list (snoogans draw_cross), offsets from the center.
static void AmCross(int px, int py, uint32_t color)
{
    static const int kOff[12][4] =
    {
        { 0,-2, 4,-4 }, { 4,-4, 8,-2 }, { 8,-2, 4, 0 },
        { 4, 0, 8, 2 }, { 8, 2, 4, 4 }, { 4, 4, 0, 2 },
        { 0, 2,-4, 4 }, {-4, 4,-8, 2 }, {-8, 2,-4, 0 },
        {-4, 0,-8,-2 }, {-8,-2,-4,-4 }, {-4,-4, 0,-2 },
    };
    __try
    {
        for (int i = 0; i < 12; i++)
            CallFn6(g_fn.drawLine,
                    (uint32_t)(px + kOff[i][0]), (uint32_t)(py + kOff[i][1]),
                    (uint32_t)(px + kOff[i][2]), (uint32_t)(py + kOff[i][3]),
                    color, 0xFFFFFFFF);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { }
}

// Game text draw (fastcall per snoogans; centered when requested).
static void AmText(const wchar_t* text, int px, int py,
                   int color, int font, bool center)
{
    __try
    {
        uint32_t prev = CallFast1(g_fn.setTextSize, (uint32_t)font);
        int x = px;
        if (center)
        {
            int w = 0;
            uint32_t fno = 0;
            CallFast3(g_fn.getTextWidth, (uint32_t)(uintptr_t)text,
                      (uint32_t)(uintptr_t)&w, (uint32_t)(uintptr_t)&fno);
            x -= w / 2;
        }
        CallFast5(g_fn.drawText, (uint32_t)(uintptr_t)text,
                  (uint32_t)x, (uint32_t)py, (uint32_t)color, 0xFFFFFFFF);
        CallFast1(g_fn.setTextSize, prev);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { }
}

// ASCII helper (area names from AreaNames.h) -> game wchar buffer.
static void AmTextAscii(const char* ascii, int px, int py,
                        int color, int font, bool center)
{
    wchar_t w[96];
    size_t n = strlen(ascii);
    if (n > 95)
        n = 95;
    for (size_t i = 0; i < n; i++)
        w[i] = (wchar_t)(unsigned char)ascii[i];
    w[n] = 0;
    AmText(w, px, py, color, font, center);
}

// monsters
static bool IsTownLevel(uint32_t levelNo)
{
    return levelNo == 1 || levelNo == 40 || levelNo == 75 ||
           levelNo == 103 || levelNo == 109;
}

static bool IsDummyMonster(uint32_t txt, uint32_t mode)
{
    if (mode == 0 || mode == 12)
        return true;
    if ((txt >= 110 && txt <= 113) || (txt == 608 && mode == 8))
        return true;
    if (txt == 68 && mode == 14)
        return true;
    if ((txt == 258 || txt == 261) && mode == 14)
        return true;
    // Bats, swarms, and other untargetable neutrals
    if ((txt >= 39 && txt <= 42) || txt == 118 || txt == 119)
        return true;
    // Chickens, goats, other town critters
    if (txt == 266 || txt == 408 || txt == 516 || txt == 517 ||
        txt == 518 || txt == 519 || txt == 522 || txt == 523 ||
        txt == 543 || txt == 545)
        return true;
    // Undead implicits (stygian dogs, dolls, etc that are really effects)
    if (txt == 326 || txt == 327 || txt == 328 || txt == 329 ||
        txt == 330 || txt == 410 || txt == 411 || txt == 412 ||
        txt == 413 || txt == 414 || txt == 415 || txt == 416 ||
        txt == 366 || txt == 406 || txt == 351 || txt == 352 ||
        txt == 353)
        return true;
    return false;
}

// Immunity color (snoogans: stat >= 100 = immune).  Returns 0 when the
// monster has no immunity (drawn white).
static uint32_t MonsterColor(uint32_t unit)
{
    struct Check { uint32_t stat; uint32_t color; };
    static const Check kChecks[] =
    {
        { 36, pal::GOLD },   // physical
        { 43, pal::BLUE },   // cold
        { 39, pal::RED },    // fire
        { 41, pal::YELLOW }, // lightning
        { 45, pal::GREEN },  // poison
        { 37, pal::ORANGE }, // magic
    };
    for (const Check& c : kChecks)
    {
        __try
        {
            if (CallFn3(g_fn.getUnitStat, unit, c.stat, 0) >= 100)
                return c.color;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }
    return 0;
}

static void DrawMonsters(uint32_t player)
{
    uint32_t act = SRead(player + off::U_ACT);
    if (!act)
        return;
    uint32_t curLvl = 0;
    {
        uint32_t lv = GetPlayerLevelAddr(player);
        curLvl = lv ? SRead(lv + off::L_LEVELNO) : 0;
    }
    if (IsTownLevel(curLvl))
        return;

    for (uint32_t r1 = SRead(act + off::A_ROOM1); r1;
         r1 = SRead(r1 + off::R1_NEXT))
    {
        for (uint32_t u = SRead(r1 + off::R1_UNITFIRST); u;
             u = SRead(u + off::U_LISTNEXT))
        {
            if (SRead(u + off::U_TYPE) != 1)
                continue;
            uint32_t mode = SRead(u + off::U_MODE);
            uint32_t txt  = SRead(u + off::U_TXTFILE);
            if (IsDummyMonster(txt, mode))
                continue;

            uint32_t path = SRead(u + off::U_PATH);
            if (!path)
                continue;
            int wx = *(volatile uint16_t*)(uintptr_t)(path + off::P_X);
            int wy = *(volatile uint16_t*)(uintptr_t)(path + off::P_Y);
            int px = 0, py = 0;
            if (!WorldToScreen(wx, wy, px, py))
                return;

            // Stat-172 == 2 marks non-monsters (snoogans), needs a game call.
            __try
            {
                if (CallFn3(g_fn.getUnitStat, u, 172, 0) == 2)
                    continue;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { }

            // Boss/unique flags live in monster_data.
            uint32_t md  = SRead(u + off::U_DATA);
            uint8_t  fl  = md ? *(volatile uint8_t*)(uintptr_t)(md + off::MD_FLAGS) : 0;
            bool boss    = (fl & off::MD_BOSS) != 0;
            bool champ   = (fl & off::MD_CHAMP) != 0;

            // Name lookup (also filters "an evil force"/"dummy" leftovers).
            wchar_t name[64];
            name[0] = 0;
            uint32_t wname = 0;
            __try { wname = CallEax1(g_fn.getUnitName, u); }
            __except (EXCEPTION_EXECUTE_HANDLER) { wname = 0; }
            if (wname >= 0x10000 && wname < 0x80000000)
            {
                __try
                {
                    int i = 0;
                    for (; i < 63; i++)
                    {
                        wchar_t ch = *(volatile wchar_t*)(uintptr_t)(wname + i * 2);
                        name[i] = ch;
                        if (!ch)
                            break;
                    }
                    name[i] = 0;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) { name[0] = 0; }
                char lo[64];
                int j = 0;
                for (; j < 63 && name[j]; j++)
                    lo[j] = (char)(name[j] & 0xFF);
                lo[j] = 0;
                if (!strcmp(lo, "an evil force") || !strcmp(lo, "dummy"))
                    continue;
            }

            // Color by monster type: unique=gold, champion=cyan, boss=red, else immunity
            uint32_t color;
            if (boss && !champ)
            {
                AmCross(px, py, pal::GOLD);
                if (name[0])
                    AmText(name, px, py - 10, pal::F_GOLD, pal::FONT_SMALL, true);
                continue;
            }
            else if (champ)
            {
                AmCross(px, py, pal::BLUE);
                if (name[0])
                    AmText(name, px, py - 10, pal::F_WHITE, pal::FONT_SMALL, true);
                continue;
            }
            else
            {
                color = MonsterColor(u);
                AmCross(px, py, color ? color : pal::WHITE);
            }
        }
    }
}

// objects (chests, shrines, etc.) and ground items
static bool IsChestObject(uint32_t txt)
{
    return txt == 397 || txt == 437 || txt == 580 ||
           txt == 152 || txt == 354 || txt == 407 || txt == 408 ||
           txt == 393 || txt == 394 || txt == 395 || txt == 396 ||
           txt == 244 || txt == 245 || txt == 246;
}

// Item quality -> automap color.
// quality: 1=low, 2=normal, 3=high, 4=magic, 5=set, 6=rare, 7=unique, 8=crafted
static uint32_t ItemQualityColor(uint32_t unit)
{
    uint32_t itemData = SRead(unit + off::U_DATA);
    if (!itemData || itemData < 0x10000 || itemData >= 0x80000000)
        return pal::WHITE;
    uint32_t quality = SRead(itemData + off::IT_QUALITY);
    switch (quality)
    {
        case 4:  return pal::BLUE;    // magic
        case 5:  return pal::GREEN;   // set
        case 6:  return pal::YELLOW;  // rare
        case 7:  return pal::GOLD;    // unique
        case 8:  return pal::ORANGE;  // crafted
        default: return pal::WHITE;   // normal/low/high
    }
}

static int ItemQualityLabel(uint32_t unit)
{
    uint32_t itemData = SRead(unit + off::U_DATA);
    if (!itemData || itemData < 0x10000 || itemData >= 0x80000000)
        return 0;
    return (int)SRead(itemData + off::IT_QUALITY);
}

// Gold pile txtfile ID
static const uint32_t GOLD_TXT = 529;

// Always-show item txtfile IDs (charms, runes, jewels, gems, potions, keys, quest items)
static bool IsWantedItemTxt(uint32_t txt)
{
    switch (txt)
    {
        // Charms
        case 603: case 604: case 605:  // small, large, grand charm
        // Runes (601-644 in 1.14d, but just catch the range)
        // Actually runes are txtfile 651+
        case 651: case 652: case 653: case 654: case 655: case 656:
        case 657: case 658: case 659: case 660: case 661: case 662:
        case 663: case 664: case 665: case 666: case 667: case 668:
        case 669: case 670: case 671: case 672: case 673:
        // Jewels
        case 645: case 646: case 647:
        // Gems (chipped through perfect)
        case 588: case 589: case 590: case 591: case 592:
        case 593: case 594: case 595: case 596: case 597: case 598:
        case 599: case 600:
        // Potions
        case 91: case 92: case 93: case 94: case 95: case 96:  // hp/mp/rv/stam/anti/thaw
        // Keys
        case 518:
        // Quest items (staff, amulet, hammer, etc.)
        case 17: case 18: case 19: case 20: case 21: case 37: case 38:
        case 39: case 181: case 182: case 183: case 184: case 185:
        case 186: case 187: case 188: case 189: case 190: case 191:
        // Gold
        case GOLD_TXT:
            return true;
        default:
            return false;
    }
}

static void DrawObjects(uint32_t player)
{
    uint32_t act = SRead(player + off::U_ACT);
    if (!act)
        return;

    for (uint32_t r1 = SRead(act + off::A_ROOM1); r1;
         r1 = SRead(r1 + off::R1_NEXT))
    {
        for (uint32_t u = SRead(r1 + off::R1_UNITFIRST); u;
             u = SRead(u + off::U_LISTNEXT))
        {
            uint32_t type = SRead(u + off::U_TYPE);
            if (type != 2 && type != 4)
                continue;

            uint32_t txt = SRead(u + off::U_TXTFILE);

            // Type 2 = objects (chests, shrines, etc.)
            uint32_t color = 0;
            if (type == 2)
            {
                if (!IsChestObject(txt))
                    continue;
                color = pal::GOLD;
            }
            else // type 4 = ground items
            {
                // Gold piles always show
                if (txt == GOLD_TXT)
                {
                    color = pal::YELLOW;
                }
                // Item filter: skip junk unless filter is off
                else if (g_settings.itemFilter)
                {
                    uint32_t itemData = SRead(u + off::U_DATA);
                    uint32_t quality = itemData ? SRead(itemData + off::IT_QUALITY) : 0;
                    bool wanted = quality >= 4 || IsWantedItemTxt(txt);
                    if (!wanted)
                        continue;
                    color = ItemQualityColor(u);
                    if (!color) color = pal::WHITE;
                }
                else
                {
                    color = ItemQualityColor(u);
                    if (!color) color = pal::WHITE;
                }
            }

            uint32_t path = SRead(u + off::U_PATH);
            if (!path)
                continue;

            int wx = 0, wy = 0;
            if (type == 4 || type == 2)
            {
                wx = (int)SRead(path + 0x0C);
                wy = (int)SRead(path + 0x10);
            }
            else
            {
                wx = *(volatile uint16_t*)(uintptr_t)(path + off::P_X);
                wy = *(volatile uint16_t*)(uintptr_t)(path + off::P_Y);
            }
            if (!wx && !wy)
                continue;

            int px = 0, py = 0;
            if (!WorldToScreen(wx, wy, px, py))
                return;

            // Draw marker
            __try
            {
                int sz = (type == 2) ? 3 : 2;
                CallFn6(g_fn.drawLine,
                        (uint32_t)(px - sz), (uint32_t)(py - sz),
                        (uint32_t)(px + sz), (uint32_t)(py + sz),
                        color, 0xFFFFFFFF);
                CallFn6(g_fn.drawLine,
                        (uint32_t)(px + sz), (uint32_t)(py - sz),
                        (uint32_t)(px - sz), (uint32_t)(py + sz),
                        color, 0xFFFFFFFF);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { }

            // Gold pile label
            if (type == 4 && txt == GOLD_TXT)
            {
                AmText(L"G", px, py + 8, pal::F_GOLD, pal::FONT_SMALL, true);
            }

            // Show item name label for magic+ quality items
            if (type == 4 && txt != GOLD_TXT)
            {
                int quality = ItemQualityLabel(u);
                if (quality >= 4)
                {
                    wchar_t wname[64];
                    wname[0] = 0;
                    uint32_t namePtr = 0;
                    __try { namePtr = CallEax1(g_fn.getUnitName, u); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { namePtr = 0; }
                    if (namePtr >= 0x10000 && namePtr < 0x80000000)
                    {
                        __try
                        {
                            int i = 0;
                            for (; i < 63; i++)
                            {
                                wchar_t ch = *(volatile wchar_t*)(uintptr_t)(namePtr + i * 2);
                                wname[i] = ch;
                                if (!ch) break;
                            }
                            wname[i] = 0;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER) { wname[0] = 0; }
                    }
                    if (wname[0])
                    {
                        int textColor = pal::F_WHITE;
                        if (quality == 5) textColor = pal::F_GREEN;
                        else if (quality == 7) textColor = pal::F_GOLD;
                        AmText(wname, px, py + 8, textColor, pal::FONT_SMALL, true);
                    }
                }
            }
        }
    }
}

// exit / area labels (level_info from snoogans draw_automap_text)
static void DrawExitLabels(uint32_t player)
{
    int actNo = (int)SRead(player + off::U_ACTNO);
    if (actNo < 0 || actNo > 4)
        return;

    // Green label for the real Tal Rasha's tomb / staff tomb of this act.
    uint32_t staffTomb = 0;
    {
        uint32_t act = SRead(player + off::U_ACT);
        uint32_t misc = act ? SRead(act + off::A_MISC) : 0;
        staffTomb = misc ? SRead(misc + off::AM_STAFFTOMB) : 0;
    }

    for (int i = 0; i < g_exitCount; i++)
    {
        const ExitInfo& e = g_exits[i];
        if (e.act != actNo)
            continue;
        int px = 0, py = 0;
        if (!WorldToScreen(e.x, e.y, px, py))
            continue;
        const char* name = AreaName(e.levelId);
        if (!name || !name[0])
            continue;
        int color = (e.levelId == staffTomb) ? pal::F_GREEN : pal::F_WHITE;
        AmTextAscii(name, px, py, color, pal::FONT_SMALL, true);
    }
}

// public tick
static int s_faults = 0;
static int s_disabled = 0;

void AmDrawTick()
{
    if (s_disabled)
        return;
    if (!g_armed || g_dataSet < 0 || !g_fn.drawLine || !g_fn.drawText)
        return;

    __try
    {
        uint32_t amOn = Rd(g_var.automapOn);
        if (!amOn)
            return;

        uint32_t player = SRead(g_var.player);
        if (player < 0x10000 || player >= 0x80000000)
            return;
        if (SRead(player + off::U_TYPE) != 0)
            return;

        int div = (int)Rd(g_var.divisor);
        if (div < 1 || div > 256)
        {
            s_disabled = 1;
            Dbg("AmDraw: bad divisor %d -- disabled", div);
            return;
        }

        DrawMonsters(player);
        DrawObjects(player);
        DrawExitLabels(player);
        OverlayTick();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (++s_faults > 4)
        {
            s_disabled = 1;
            Dbg("AmDraw: faulted %d times -- disabled", s_faults);
        }
    }
}

/*
  monsters.cpp -- monster overlay on the automap
  Color coded crosses + names for bosses/champions/uniques.
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
#include "automap.h"
#include "w2s.h"

namespace DrawFeatures {

static bool IsTownLevel(uint32_t levelNo) {
  return levelNo == 1 || levelNo == 40 || levelNo == 75 || levelNo == 103 ||
         levelNo == 109;
}

static bool IsDummyMonster(uint32_t txt, uint32_t mode) {
  if (mode == 0 || mode == 12) return true;
  if ((txt >= 110 && txt <= 113) || (txt == 608 && mode == 8)) return true;
  if (txt == 68 && mode == 14) return true;
  if ((txt == 258 || txt == 261) && mode == 14) return true;
  if ((txt >= 39 && txt <= 42) || txt == 118 || txt == 119) return true;
  if (txt == 266 || txt == 408 || txt == 516 || txt == 517 || txt == 518 ||
      txt == 519 || txt == 522 || txt == 523 || txt == 543 || txt == 545)
    return true;
  if (txt == 326 || txt == 327 || txt == 328 || txt == 329 || txt == 330 ||
      txt == 410 || txt == 411 || txt == 412 || txt == 413 || txt == 414 ||
      txt == 415 || txt == 416 || txt == 366 || txt == 406 || txt == 351 ||
      txt == 352 || txt == 353)
    return true;
  return false;
}

static uint32_t MonsterColor(uint32_t unit) {
  struct Check {
    uint32_t stat;
    uint32_t color;
  };
  static const Check kChecks[] = {
      {36, pal::GOLD},   {43, pal::BLUE},  {39, pal::RED},
      {41, pal::YELLOW}, {45, pal::GREEN}, {37, pal::ORANGE},
  };
  for (const Check& c : kChecks) {
    __try {
      if (CallFn3(g_fn.getUnitStat, unit, c.stat, 0) >= 100) return c.color;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
  return 0;
}

void DrawMonsters(uint32_t player) {
  if (!g_settings.showMonsters) return;

  uint32_t act = SRead(player + off::U_ACT);
  if (!act) return;
  uint32_t curLvl = 0;
  {
    uint32_t lv = GetPlayerLevelAddr(player);
    curLvl = lv ? SRead(lv + off::L_LEVELNO) : 0;
  }
  if (IsTownLevel(curLvl)) return;

  for (uint32_t r1 = SRead(act + off::A_ROOM1); r1;
       r1 = SRead(r1 + off::R1_NEXT)) {
    for (uint32_t u = SRead(r1 + off::R1_UNITFIRST); u;
         u = SRead(u + off::U_LISTNEXT)) {
      if (SRead(u + off::U_TYPE) != 1) continue;
      uint32_t mode = SRead(u + off::U_MODE);
      uint32_t txt = SRead(u + off::U_TXTFILE);
      if (IsDummyMonster(txt, mode)) continue;

      uint32_t path = SRead(u + off::U_PATH);
      if (!path) continue;
      int wx = *(volatile uint16_t*)(uintptr_t)(path + off::P_X);
      int wy = *(volatile uint16_t*)(uintptr_t)(path + off::P_Y);
      int px = 0, py = 0;
      if (!WorldToScreen(wx, wy, px, py)) return;

      __try {
        if (CallFn3(g_fn.getUnitStat, u, 172, 0) == 2) continue;
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }

      uint32_t md = SRead(u + off::U_DATA);
      uint8_t fl = md ? *(volatile uint8_t*)(uintptr_t)(md + off::MD_FLAGS) : 0;
      bool boss = (fl & off::MD_BOSS) != 0;
      bool champ = (fl & off::MD_CHAMP) != 0;

      wchar_t name[64];
      name[0] = 0;
      uint32_t wname = 0;
      __try {
        wname = CallEax1(g_fn.getUnitName, u);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        wname = 0;
      }
      if (wname >= 0x10000 && wname < 0x80000000) {
        __try {
          int i = 0;
          for (; i < 63; i++) {
            wchar_t ch = *(volatile wchar_t*)(uintptr_t)(wname + i * 2);
            name[i] = ch;
            if (!ch) break;
          }
          name[i] = 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
          name[0] = 0;
        }
        char lo[64];
        int j = 0;
        for (; j < 63 && name[j]; j++) lo[j] = (char)(name[j] & 0xFF);
        lo[j] = 0;
        if (!strcmp(lo, "an evil force") || !strcmp(lo, "dummy")) continue;
      }

      uint32_t color;
      if (boss && !champ) {
        AmCross(px, py, pal::GOLD);
        if (name[0])
          AmText(name, px, py - 10, pal::F_GOLD, pal::FONT_SMALL, true);
        continue;
      } else if (champ) {
        AmCross(px, py, pal::BLUE);
        if (name[0])
          AmText(name, px, py - 10, pal::F_WHITE, pal::FONT_SMALL, true);
        continue;
      } else {
        color = MonsterColor(u);
        AmCross(px, py, color ? color : pal::WHITE);
      }
    }
  }
}

}

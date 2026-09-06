/*
  items.cpp -- chests, gold piles, and ground items on the automap
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

static bool IsChestObject(uint32_t txt) {
  return txt == 397 || txt == 437 || txt == 580 || txt == 152 || txt == 354 ||
         txt == 407 || txt == 408 || txt == 393 || txt == 394 || txt == 395 ||
         txt == 396 || txt == 244 || txt == 245 || txt == 246;
}

static uint32_t ItemQualityColor(uint32_t unit) {
  uint32_t itemData = SRead(unit + off::U_DATA);
  if (!itemData || itemData < 0x10000 || itemData >= 0x80000000)
    return pal::WHITE;
  uint32_t quality = SRead(itemData + off::IT_QUALITY);
  switch (quality) {
    case 4:
      return pal::BLUE;
    case 5:
      return pal::GREEN;
    case 6:
      return pal::YELLOW;
    case 7:
      return pal::GOLD;
    case 8:
      return pal::ORANGE;
    default:
      return pal::WHITE;
  }
}

static int ItemQualityLabel(uint32_t unit) {
  uint32_t itemData = SRead(unit + off::U_DATA);
  if (!itemData || itemData < 0x10000 || itemData >= 0x80000000) return 0;
  return (int)SRead(itemData + off::IT_QUALITY);
}

static const uint32_t GOLD_TXT = 529;

static bool IsWantedItemTxt(uint32_t txt) {
  switch (txt) {
    case 603:
    case 604:
    case 605:
    case 651:
    case 652:
    case 653:
    case 654:
    case 655:
    case 656:
    case 657:
    case 658:
    case 659:
    case 660:
    case 661:
    case 662:
    case 663:
    case 664:
    case 665:
    case 666:
    case 667:
    case 668:
    case 669:
    case 670:
    case 671:
    case 672:
    case 673:
    case 645:
    case 646:
    case 647:
    case 588:
    case 589:
    case 590:
    case 591:
    case 592:
    case 593:
    case 594:
    case 595:
    case 596:
    case 597:
    case 598:
    case 599:
    case 600:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 518:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 37:
    case 38:
    case 39:
    case 181:
    case 182:
    case 183:
    case 184:
    case 185:
    case 186:
    case 187:
    case 188:
    case 189:
    case 190:
    case 191:
    case GOLD_TXT:
      return true;
    default:
      return false;
  }
}

void DrawObjects(uint32_t player) {
  uint32_t act = SRead(player + off::U_ACT);
  if (!act) return;

  for (uint32_t r1 = SRead(act + off::A_ROOM1); r1;
       r1 = SRead(r1 + off::R1_NEXT)) {
    for (uint32_t u = SRead(r1 + off::R1_UNITFIRST); u;
         u = SRead(u + off::U_LISTNEXT)) {
      uint32_t type = SRead(u + off::U_TYPE);
      if (type != 2 && type != 4) continue;

      uint32_t txt = SRead(u + off::U_TXTFILE);

      uint32_t color = 0;
      if (type == 2) {
        if (!g_settings.showChests) continue;
        if (!IsChestObject(txt)) continue;
        color = pal::GOLD;
      } else {
        if (txt == GOLD_TXT) {
          if (!g_settings.goldMarkers) continue;
          color = pal::YELLOW;
        } else if (g_settings.itemFilter) {
          uint32_t itemData = SRead(u + off::U_DATA);
          uint32_t quality = itemData ? SRead(itemData + off::IT_QUALITY) : 0;
          bool wanted = quality >= 4 || IsWantedItemTxt(txt);
          if (!wanted) continue;
          color = ItemQualityColor(u);
          if (!color) color = pal::WHITE;
        } else {
          color = ItemQualityColor(u);
          if (!color) color = pal::WHITE;
        }
      }

      uint32_t path = SRead(u + off::U_PATH);
      if (!path) continue;

      int wx = 0, wy = 0;
      if (type == 4 || type == 2) {
        wx = (int)SRead(path + off::P_GAMEX);
        wy = (int)SRead(path + off::P_GAMEY);
      } else {
        wx = *(volatile uint16_t*)(uintptr_t)(path + off::P_X);
        wy = *(volatile uint16_t*)(uintptr_t)(path + off::P_Y);
      }
      if (!wx && !wy) continue;

      int px = 0, py = 0;
      if (!WorldToScreen(wx, wy, px, py)) return;

      __try {
        int sz = (type == 2) ? 3 : 2;
        CallFn6(g_fn.drawLine, (uint32_t)(px - sz), (uint32_t)(py - sz),
                (uint32_t)(px + sz), (uint32_t)(py + sz), color, 0xFFFFFFFF);
        CallFn6(g_fn.drawLine, (uint32_t)(px + sz), (uint32_t)(py - sz),
                (uint32_t)(px - sz), (uint32_t)(py + sz), color, 0xFFFFFFFF);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }

      if (type == 4 && txt == GOLD_TXT) {
        AmText(L"G", px, py + 8, pal::F_GOLD, pal::FONT_SMALL, true);
      }

      if (type == 4 && txt != GOLD_TXT) {
        int quality = ItemQualityLabel(u);
        if (quality >= 4) {
          wchar_t wname[64];
          wname[0] = 0;
          uint32_t namePtr = 0;
          __try {
            namePtr = CallEax1(g_fn.getUnitName, u);
          } __except (EXCEPTION_EXECUTE_HANDLER) {
            namePtr = 0;
          }
          if (namePtr >= 0x10000 && namePtr < 0x80000000) {
            __try {
              int i = 0;
              for (; i < 63; i++) {
                wchar_t ch = *(volatile wchar_t*)(uintptr_t)(namePtr + i * 2);
                wname[i] = ch;
                if (!ch) break;
              }
              wname[i] = 0;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
              wname[0] = 0;
            }
          }
          if (wname[0]) {
            int textColor = pal::F_WHITE;
            if (quality == 5)
              textColor = pal::F_GREEN;
            else if (quality == 7)
              textColor = pal::F_GOLD;
            AmText(wname, px, py + 8, textColor, pal::FONT_SMALL, true);
          }
        }

      } /* namespace DrawFeatures */
    }
  }
}
}  // namespace DrawFeatures

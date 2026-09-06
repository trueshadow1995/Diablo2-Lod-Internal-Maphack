/*
  exits.cpp -- exit / area name labels on the automap
  Green for the real Tal Rasha's tomb, white for everything else.
*/
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>

#include "AreaNames.h"
#include "Structs.h"
#include "automap.h"
#include "w2s.h"

namespace DrawFeatures {

void DrawExitLabels(uint32_t player) {
  if (!g_settings.showExits) return;

  int actNo = (int)SRead(player + off::U_ACTNO);
  if (actNo < 0 || actNo > 4) return;

  uint32_t staffTomb = 0;
  {
    uint32_t act = SRead(player + off::U_ACT);
    uint32_t misc = act ? SRead(act + off::A_MISC) : 0;
    staffTomb = misc ? SRead(misc + off::AM_STAFFTOMB) : 0;
  }

  for (int i = 0; i < g_exitCount; i++) {
    const ExitInfo& e = g_exits[i];
    if (e.act != actNo) continue;
    int px = 0, py = 0;
    if (!WorldToScreen(e.x, e.y, px, py)) continue;
    const char* name = AreaName(e.levelId);
    if (!name || !name[0]) continue;
    int color = (e.levelId == staffTomb) ? pal::F_GREEN : pal::F_WHITE;
    AmTextAscii(name, px, py, color, pal::FONT_SMALL, true);
  }
}

}

/*
  automap.cpp -- orchestrator for automap overlay drawing
  Calls DrawMonsters, DrawObjects, DrawExitLabels, OverlayTick.
  Feature implementations live in their own .cpp files.
*/

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include "Structs.h"
#include "automap.h"

using namespace DrawFeatures;

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

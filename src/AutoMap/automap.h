#pragma once
/*
  automap.h --> all automap overlay drawing in one place.
  Shared primitives + feature functions live in DrawFeatures namespace.
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

namespace DrawFeatures
{

/* Cross glyph */
inline void AmCross(int px, int py, uint32_t color)
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

/* Game text draw */
inline void AmText(const wchar_t* text, int px, int py,
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

/* ASCII helper (area names) -> game wchar buffer. */
inline void AmTextAscii(const char* ascii, int px, int py,
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

/* Feature functions */
void DrawMonsters(uint32_t player);
void DrawObjects(uint32_t player);
void DrawExitLabels(uint32_t player);

} 

/* Master tick called from hook */
void AmDrawTick();

/*
 overlay.cpp -- settings panel rendered on the automap via D2GFX/D2WIN
 INSERT key toggles the panel.  Mouse clicks toggle individual features
 All drawing happens inside the automap render pass
 so it's SEH-safe and never crashes the game.
*/

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>

#include <cstdio>
#include <cstring>

#include "Structs.h"
#include "w2s.h"

// panel geometry
static const int PANEL_X = 20;   // screen pixels from left
static const int PANEL_Y = 80;   // screen pixels from top
static const int LINE_H = 14;    // line height per option
static const int PANEL_W = 220;  // panel width
static const int PANEL_PAD = 6;  // inner padding

// toggle entries
struct ToggleEntry {
  const char* label;
  bool* ptr;
};

static ToggleEntry s_toggles[] = {
    {"Item Filter", &g_settings.itemFilter},
    {"Gold Markers", &g_settings.goldMarkers},
    {"Shrine Markers", &g_settings.shrineMarkers},
    {"Monster HP Bars", &g_settings.monsterHP},
    {"Quest Markers", &g_settings.questMarkers},
};
static const int s_toggleCount = sizeof(s_toggles) / sizeof(s_toggles[0]);

// helpers
//  Draw a filled horizontal line (cheap rectangle fill)
static void FillHLine(int x1, int x2, int y, uint32_t color) {
  __try {
    CallFn6(g_fn.drawLine, (uint32_t)x1, (uint32_t)y, (uint32_t)x2, (uint32_t)y,
            color, 0xFFFFFFFF);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// Draw a filled rectangle (scanline fill with horizontal lines)
static void FillRect(int x, int y, int w, int h, uint32_t color) {
  for (int row = 0; row < h; row++) FillHLine(x, x + w, y + row, color);
}

// Draw text centered in the panel
static void PanelText(const char* ascii, int px, int py, int color, int font) {
  wchar_t w[96];
  size_t n = strlen(ascii);
  if (n > 95) n = 95;
  for (size_t i = 0; i < n; i++) w[i] = (wchar_t)(unsigned char)ascii[i];
  w[n] = 0;

  __try {
    uint32_t prev = CallFast1(g_fn.setTextSize, (uint32_t)font);
    CallFast5(g_fn.drawText, (uint32_t)(uintptr_t)w, (uint32_t)px, (uint32_t)py,
              (uint32_t)color, 0xFFFFFFFF);
    CallFast1(g_fn.setTextSize, prev);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// hotkey + input
static bool s_insertWasDown = false;
static bool s_lmbWasDown = false;
static int s_clickCooldown = 0;

static void HandleInput() {
  // INSERT toggles the panel.
  bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
  if (insertDown && !s_insertWasDown)
    g_settings.showPanel = !g_settings.showPanel;
  s_insertWasDown = insertDown;

  if (!g_settings.showPanel) return;

  if (s_clickCooldown > 0) {
    s_clickCooldown--;
    return;
  }

  bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
  bool lmbClicked = lmbDown && !s_lmbWasDown;
  s_lmbWasDown = lmbDown;

  if (!lmbClicked) return;

  POINT pt;
  if (!GetCursorPos(&pt)) return;

  HWND hwnd = GetForegroundWindow();
  if (!hwnd) return;
  ScreenToClient(hwnd, &pt);

  int rowX = PANEL_X;
  int rowW = PANEL_W;
  int rowY = PANEL_Y + LINE_H + PANEL_PAD;

  for (int i = 0; i < s_toggleCount; i++) {
    int ry = rowY + i * LINE_H - 2;  // 2px grace above
    int rh = LINE_H + 4;             // 4px grace below
    if (pt.x >= rowX && pt.x <= rowX + rowW && pt.y >= ry && pt.y < ry + rh) {
      *s_toggles[i].ptr = !(*s_toggles[i].ptr);
      s_clickCooldown = 8;
      break;
    }
  }
}

// render
static void RenderPanel() {
  int px = PANEL_X;
  int py = PANEL_Y;
  int pw = PANEL_W;
  int innerW = pw - PANEL_PAD * 2;

  // Panel background (dark semi-transparent via dark grey fill)
  FillRect(px, py, pw, LINE_H + s_toggleCount * LINE_H + PANEL_PAD * 2 + 8,
           0x01);
  // Border
  FillHLine(px, px + pw, py, pal::GOLD);
  FillHLine(px, px + pw,
            py + LINE_H + s_toggleCount * LINE_H + PANEL_PAD * 2 + 7,
            pal::GOLD);
  for (int row = 0; row < LINE_H + s_toggleCount * LINE_H + PANEL_PAD * 2 + 7;
       row++) {
    FillHLine(px, px + 1, py + row, pal::GOLD);
    FillHLine(px + pw - 1, px + pw, py + row, pal::GOLD);
  }

  // Title
  PanelText("DiabloLodMap", px + PANEL_PAD + 20, py + 4, pal::F_GOLD,
            pal::FONT_SMALL);

  // Toggle entries
  int contentX = px + PANEL_PAD + 4;
  int startY = py + LINE_H + PANEL_PAD + 4;

  for (int i = 0; i < s_toggleCount; i++) {
    bool on = *s_toggles[i].ptr;
    int textColor = on ? pal::F_GREEN : pal::F_RED;

    // Checkbox: [x] or [ ]
    char buf[96];
    snprintf(buf, sizeof(buf), "[%c] %s", on ? 'x' : ' ', s_toggles[i].label);
    PanelText(buf, contentX, startY + i * LINE_H, textColor, pal::FONT_SMALL);
  }

  // Footer hint
  PanelText("INSERT to close", px + PANEL_PAD + 30,
            startY + s_toggleCount * LINE_H + 4, pal::LTGREY, pal::FONT_SMALL);
}

// public API (called from module tick or automap draw
void OverlayTick() {
  if (!g_armed || g_dataSet < 0 || !g_fn.drawLine || !g_fn.drawText) return;

  HandleInput();

  if (g_settings.showPanel) {
    uint32_t amOn = Rd(g_var.automapOn);
    if (!amOn) return;

    __try {
      RenderPanel();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
  }
}

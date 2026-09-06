#pragma once
#include "Structs.h"
// D2GFX line colors
namespace pal {
constexpr uint32_t WHITE = 0xFF;
constexpr uint32_t RED = 0x0A;
constexpr uint32_t GREEN = 0x7D;
constexpr uint32_t BLUE = 0x91;
constexpr uint32_t GOLD = 0x5A;
constexpr uint32_t ORANGE = 0x60;
constexpr uint32_t YELLOW = 0x6F;
constexpr uint32_t LTGREY = 0x28;

// D2WIN text colors and font sizes
constexpr int F_WHITE = 0;
constexpr int F_RED = 1;
constexpr int F_GREEN = 2;
constexpr int F_BLUE = 3;
constexpr int F_GOLD = 4;
constexpr int FONT_SMALL = 6;
} 

// World coords -> automap screen pixels.
// Uses the game's own projection math (divisor + scroll globals)
inline bool WorldToScreen(int wx, int wy, int& px, int& py) {
  int div = (int)Rd(g_var.divisor);
  if (div < 1 || div > 256) return false;
  int sx = (int)Rd(g_var.scrollX);
  int sy = (int)Rd(g_var.scrollY);
  px = ((wx - wy) * 16 + 10) / div - sx;
  py = ((wx + wy) * 8 - 30) / div - sy;
  return true;
}

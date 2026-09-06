/*
  scanner.cpp --> runtime pattern scanner for Diablo II 1.14d.
  Scans Game.exe's .text section for byte patterns to find functions at load
  time instead of hardcoding RVAs.  Patterns use ?? wildcards for bytes that
  contain embedded absolute addresses (which shift between sub-builds).
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstring>

#include "Structs.h"
#include "patterns.h"

// pattern match helper returns true if pattern matches at the given address
static bool MatchAt(const uint8_t* base, int baseLen, const int8_t* pat,
                    int patLen, int offset) {
  if (offset < 0 || offset + patLen > baseLen) return false;
  for (int i = 0; i < patLen; i++) {
    if (pat[i] == -1)  // wildcard
      continue;
    if (base[offset + i] != (uint8_t)pat[i]) return false;
  }
  return true;
}

// scan for a pattern, return address of match or 0
static uint32_t ScanPattern(const uint8_t* code, int codeLen, const int8_t* pat,
                            int patLen) {
  // Skip first 0x100 bytes PE header / padding
  int start = 0x100;
  if (start + patLen > codeLen) return 0;
  for (int i = start; i <= codeLen - patLen; i++) {
    if (MatchAt(code, codeLen, pat, patLen, i))
      return (uint32_t)(uintptr_t)&code[i];
  }
  return 0;
}

// get the .text section of the main module
struct CodeSection {
  const uint8_t* base;
  int len;
};

static CodeSection GetCodeSection() {
  HMODULE mod = GetModuleHandleA(nullptr);
  if (!mod) return {nullptr, 0};
  IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)mod;
  IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((uint8_t*)mod + dos->e_lfanew);
  IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
  for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
    if (memcmp(sec[i].Name, ".text", 5) == 0) {
      return {(const uint8_t*)mod + sec[i].VirtualAddress,
              (int)sec[i].Misc.VirtualSize};
    }
  }
  // Fallback: treat the whole image as code
  return {(const uint8_t*)mod, (int)nt->OptionalHeader.SizeOfImage};
}

// scan all functions
extern uint32_t g_base;

bool ScanAllPatterns(Fn& fn) {
  CodeSection cs = GetCodeSection();
  if (!cs.base || cs.len < 0x1000) {
    Dbg("Scanner: failed to find .text section");
    return false;
  }

  Dbg("Scanner: .text at 0x%08X, %d bytes", (uint32_t)(uintptr_t)cs.base,
      cs.len);

  // Each entry: {name, pattern, pattern_length, output_field_offset}
  struct PatJob {
    const char* name;
    const int8_t* pat;
    int len;
    uint32_t* out;
  };

  PatJob jobs[] = {
      {"init_automap_layer", pat_initLayer, sizeof(pat_initLayer),
       &fn.initLayer},
      {"reveal_automap_room", pat_revealRoom, sizeof(pat_revealRoom),
       &fn.revealRoom},
      {"new_automap_cell", pat_newCell, sizeof(pat_newCell), &fn.newCell},
      {"add_automap_cell", pat_addCell, sizeof(pat_addCell), &fn.addCell},
      {"get_object_txt", pat_getObjTxt, sizeof(pat_getObjTxt),
       &fn.getObjectTxt},
      {"get_unit_name", pat_getUnitName, sizeof(pat_getUnitName),
       &fn.getUnitName},
      {"get_unit_stat", pat_getUnitStat, sizeof(pat_getUnitStat),
       &fn.getUnitStat},
      {"print_game_string", pat_printGame, sizeof(pat_printGame),
       &fn.printGame},
      {"draw_line", pat_drawLine, sizeof(pat_drawLine), &fn.drawLine},
      {"draw_text", pat_drawText, sizeof(pat_drawText), &fn.drawText},
      {"set_text_size", pat_setTextSize, sizeof(pat_setTextSize),
       &fn.setTextSize},
      {"get_text_width", pat_getTextWidth, sizeof(pat_getTextWidth),
       &fn.getTextWidth},
      {"get_automap_size", pat_automapSize, sizeof(pat_automapSize),
       &fn.automapSize},
      {"get_layer", pat_getLayer, sizeof(pat_getLayer), &fn.getLayer},
      {"add_room_data", pat_addRoomData, sizeof(pat_addRoomData), &fn.addRoom},
      {"remove_room_data", pat_removeRoomData, sizeof(pat_removeRoomData),
       &fn.removeRoom},
      {"reveal_callback", pat_revealCallback, sizeof(pat_revealCallback),
       &fn.revealCallback},
  };

  int found = 0;
  int total = sizeof(jobs) / sizeof(jobs[0]);

  for (int i = 0; i < total; i++) {
    uint32_t addr = ScanPattern(cs.base, cs.len, jobs[i].pat, jobs[i].len);
    if (addr) {
      *jobs[i].out = addr;
      found++;
      Dbg("Scanner: %-22s -> 0x%08X", jobs[i].name, addr);
    } else {
      *jobs[i].out = 0;
      Dbg("Scanner: %-22s -> NOT FOUND", jobs[i].name);
    }
  }

  Dbg("Scanner: %d/%d functions found", found, total);

  // Hardcoded fallbacks for D2GFX vtable wrappers that have generic prologues
  if (!fn.drawLine) {
    // D2GFX DrawLine: __stdcall(x1,y1,x2,y2,color,unk), retn 18h,
    // calls through dword_7C8CC0 + 0xC0.
    uint32_t candidates[] = {0x4F6380,
                             0x4F6380 + 0x7F78};  // published + shifted
    for (uint32_t ea : candidates) {
      __try {
        uint8_t* p = (uint8_t*)(uintptr_t)ea;
        //  push ebp; mov ebp,esp; ... retn 18h at end
        if (p[0] == 0x55 && p[1] == 0x8B && p[2] == 0xEC) {
          // Scan for retn 18h (C2 18 00) within 40 bytes
          for (int k = 16; k < 40; k++) {
            if (p[k] == 0xC2 && p[k + 1] == 0x18 && p[k + 2] == 0x00) {
              fn.drawLine = ea;
              Dbg("Scanner: draw_line (hardcoded) -> 0x%08X (retn at +%d)", ea,
                  k);
              break;
            }
          }
        }
      } __except (EXCEPTION_EXECUTE_HANDLER) {
      }
      if (fn.drawLine) break;
    }
    if (!fn.drawLine)
      Dbg("Scanner: draw_line -> NOT FOUND (all fallbacks failed)");
  }

  // Core functions that must be found
  bool coreOk = fn.initLayer && fn.revealRoom && fn.newCell && fn.addCell;
  if (!coreOk) {
    Dbg("Scanner: core functions missing (init=%d reveal=%d new=%d add=%d)",
        !!fn.initLayer, !!fn.revealRoom, !!fn.newCell, !!fn.addCell);
  }
  return coreOk;
}

struct DataPattern {
  const char* name;
  const int8_t* pat;  // pattern to match
  int patLen;
  int addrOffset;  // offset within pattern where the 4-byte address lives
  uint32_t* out;   // where to store the result
};

static uint32_t ExtractAddr(const uint8_t* matchBase, int offset) {
  return *(const uint32_t*)(matchBase + offset);
}

bool ScanDataPatterns(uint32_t base, const uint8_t* code, int codeLen,
                      Vars& var) {
  if (g_fn.initLayer) {
    const uint8_t* p = (const uint8_t*)(uintptr_t)g_fn.initLayer;
    if (p[0] == 0x56 && p[1] == 0x8B && p[2] == 0x35) {
      var.layer = *(const uint32_t*)(p + 3);
      Dbg("Scanner: automap_layer -> 0x%08X", var.layer);
    }
  }

  {
    struct DataLayout {
      uint32_t player;
      uint32_t layer;
      uint32_t automapOn;
      uint32_t divisor;
      uint32_t scrollX;
      uint32_t scrollY;
    };
    static const DataLayout kLayouts[] = {
        {0x7A6A70, 0x7A5160, 0x7A0634, 0x711254, 0x7A5198,
         0x7A519C},  // published
        {0x79EAF8, 0x79D1E8, 0x7986BC, 0x7092DC, 0x79D220,
         0x79D224},  // shifted (-0x7F78)
    };
    for (const auto& L : kLayouts) {
      uint32_t val = 0;
      __try {
        val = *(const uint32_t*)(uintptr_t)L.player;
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        val = 0;
      }
      if (val >= 0x10000 && val < 0x7FFFF000) {
        var.player = L.player;
        var.layer = L.layer;
        var.automapOn = L.automapOn;
        var.divisor = L.divisor;
        var.scrollX = L.scrollX;
        var.scrollY = L.scrollY;
        Dbg("Scanner: data layout matched (player=0x%08X div=0x%08X "
            "scrollX=0x%08X scrollY=0x%08X automapOn=0x%08X)",
            L.player, L.divisor, L.scrollX, L.scrollY, L.automapOn);
        break;
      }
    }
    if (!var.player) Dbg("Scanner: player_unit NOT FOUND in either sub-build");
  }

  return (var.player && var.layer && var.automapOn && var.divisor);
}

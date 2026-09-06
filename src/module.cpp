#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "Data/AreaNames.h"
#include "Structs/Structs.h"

uint32_t g_base = 0;
bool g_armed = false;
Fn g_fn;
Vars g_var;
int g_dataSet = -1;
Settings g_settings;

ExitInfo g_exits[96];
int g_exitCount = 0;

volatile LONG g_run = 1;

static bool InstallAutomapHook();

bool ModuleShouldRun() { return g_run != 0; }

// safe primitives
uint32_t Rd(uint32_t addr) { return *(volatile uint32_t*)(uintptr_t)addr; }

uint32_t SRead(uint32_t addr) {
  if (addr < 0x10000 || addr >= 0x80000000) return 0;
  __try {
    return Rd(addr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
}

bool IsReadable(uint32_t addr) {
  if (addr < 0x10000 || addr >= 0x80000000) return false;
  MEMORY_BASIC_INFORMATION mbi;
  if (!VirtualQuery((const void*)(uintptr_t)addr, &mbi, sizeof(mbi)))
    return false;
  return mbi.State == MEM_COMMIT &&
         !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
         (mbi.Protect &
          (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
           PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
}

bool LooksCode(uint32_t addr) {
  if (addr < 0x10000 || addr >= 0x80000000) return false;
  uint8_t b = 0;
  __try {
    b = *(volatile uint8_t*)(uintptr_t)addr;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  return b != 0x00 && b != 0xCC;
}

void Dbg(const char* fmt, ...) {
  wchar_t dir[MAX_PATH] = {0};
  GetTempPathW(MAX_PATH, dir);
  wchar_t path[MAX_PATH];
  swprintf_s(path, MAX_PATH, L"%sdiablolodmap.log", dir);
  FILE* f = nullptr;
  _wfopen_s(&f, path, L"a");
  if (!f) return;
  char buf[600];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
  va_end(ap);
  printf("%s\n", buf); 
  fflush(stdout);
  fwrite(buf, 1, strlen(buf), f);
  fwrite("\n", 1, 1, f);
  fclose(f);
}

// resolve + arm
static bool ArmFunctions() {
  if (!g_base) g_base = (uint32_t)(uintptr_t)GetModuleHandleA(nullptr);
  if (!g_base) return false;

  if (!ScanAllPatterns(g_fn)) return false;

  if (!(g_fn.addRoom && g_fn.removeRoom)) {
    g_fn.addRoom = 0;
    g_fn.removeRoom = 0;
    Dbg("Arm: add/remove room data not found -- materialization off");
  }
  g_armed = true;
  Dbg("Arm: OK base=0x%08X (print=0x%08X draw=0x%08X/0x%08X getLayer=0x%08X)",
      g_base, g_fn.printGame, g_fn.drawLine, g_fn.drawText, g_fn.getLayer);

  InstallAutomapHook();
  return true;
}


static bool PickDataLayout() {
  if (g_dataSet >= 0) return true;  

  HMODULE mod = GetModuleHandleA(nullptr);
  if (!mod) return false;

  // Get the .text section for scanning
  IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)mod;
  IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((uint8_t*)mod + dos->e_lfanew);
  IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
  const uint8_t* code = nullptr;
  int codeLen = 0;
  for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
    if (memcmp(sec[i].Name, ".text", 5) == 0) {
      code = (const uint8_t*)mod + sec[i].VirtualAddress;
      codeLen = (int)sec[i].Misc.VirtualSize;
      break;
    }
  }
  if (!code) {
    code = (const uint8_t*)mod;
    codeLen = (int)nt->OptionalHeader.SizeOfImage;
  }

  Vars v = {};
  if (!ScanDataPatterns(g_base, code, codeLen, v)) {
    Dbg("Data: scanner could not find all data globals");
    return false;
  }

  g_var = v;
  g_dataSet = 0;
  Dbg("Data: scanner armed (player=0x%08X layer=0x%08X automapOn=0x%08X "
      "divisor=0x%08X offset=0x%08X)",
      v.player, v.layer, v.automapOn, v.divisor, v.offset);
  return true;
}

// player / level helpers
static uint32_t Player() {
  if (!g_armed || g_dataSet < 0) return 0;
  uint32_t p = SRead(g_var.player);
  if (p < 0x10000 || p >= 0x80000000) return 0;
  return p;
}

// player -> path -> room1 -> room2 -> level 0 when not in a game
uint32_t GetPlayerLevelAddr(uint32_t player) {
  uint32_t path = player ? SRead(player + off::U_PATH) : 0;
  uint32_t r1 = path ? SRead(path + off::P_ROOM1) : 0;
  uint32_t r2 = r1 ? SRead(r1 + off::R1_ROOM2) : 0;
  return r2 ? SRead(r2 + off::R2_LEVEL) : 0;
}

int PlayerAct(uint32_t player) {
  uint32_t a = player ? SRead(player + off::U_ACT) : 0;
  if (!a) return -1;
  int actNo = (int)SRead(player + off::U_ACTNO);
  return (actNo >= 0 && actNo <= 4) ? actNo : -1;
}

static uint32_t s_hookCave = 0;
static uint32_t s_hookTarget = 0;
static bool s_hookInstalled = false;

// This is called from the code cave
// It must be __cdecl with no parameters
void __cdecl AmOverlayHook() { AmDrawTick(); }

static bool InstallAutomapHook() {
  if (s_hookInstalled) return true;
  if (!g_fn.initLayer) return false;

  uint32_t target =
      g_base + 0x59E90;  // sub_459E90 text renderer in automap draw


  uint8_t first2[2] = {};
  __try {
    memcpy(first2, (void*)(uintptr_t)target, 2);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  if (first2[0] != 0x55 || first2[1] != 0x8B) {
    Dbg("Hook: sub_459E90 prologue not 55 8B (got %02X %02X) -- skipping",
        first2[0], first2[1]);
    return false;
  }

  // Allocate a code cave (executable, R/W/X)
  void* cave = VirtualAlloc(nullptr, 128, MEM_COMMIT | MEM_RESERVE,
                            PAGE_EXECUTE_READWRITE);
  if (!cave) {
    Dbg("Hook: VirtualAlloc failed (%u)", GetLastError());
    return false;
  }

  /*
     Read the first 16 bytes of the target to find the instruction
     boundary.
     We need to overwrite enough bytes for a 5-byte jmp rel32 + padding,
     then replay those exact instructions in the trampoline.
     Standard 32-bit x86 walk instructions forward until we have >= 8
     bytes.
  */
  uint8_t origBytes[16] = {};
  __try {
    memcpy(origBytes, (void*)(uintptr_t)target, sizeof(origBytes));
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    VirtualFree(cave, 0, MEM_RELEASE);
    return false;
  }
  int overwriteLen = 0;
  {
    int pos = 0;
    while (pos < 12) {
      uint8_t b = origBytes[pos];
      int len = 1;  // default: 1-byte instruction

      if (b == 0x55)
        len = 1;  // push ebp
      else if (b == 0x8B)
        len = 2;  // mov r/m, r
      else if (b == 0x89)
        len = 2;  // mov r, r/m
      else if (b == 0x83)
        len = 3;  // alu r/m, imm8
      else if (b == 0x81)
        len = 6;  // alu r/m, imm32
      else if (b == 0xA1)
        len = 5;  // mov eax, [addr32]
      else if (b == 0x33)
        len = 2;  // xor reg, reg
      else if (b == 0x56)
        len = 1;  // push esi
      else if (b == 0x57)
        len = 1;  // push edi
      else if (b == 0x53)
        len = 1;  // push ebx
      else if (b == 0x50)
        len = 1;  // push eax
      else if (b == 0x51)
        len = 1;  // push ecx
      else if (b == 0x52)
        len = 1;  // push edx
      else if (b == 0x6A)
        len = 2;  // push imm8
      else if (b == 0x68)
        len = 5;  // push imm32
      else if (b == 0xB8 || b == 0xB9 || b == 0xBA || b == 0xBB || b == 0xBC ||
               b == 0xBD || b == 0xBE || b == 0xBF)
        len = 5;  // mov reg, imm32
      else if (b == 0xE8)
        len = 5;  // call rel32
      else if (b == 0xE9)
        len = 5;  // jmp rel32
      else if (b == 0xEB)
        len = 2;  // jmp rel8
      else if (b == 0xC3)
        len = 1;  // ret
      else if (b == 0xC2)
        len = 3;  // ret imm16
      else if (b == 0xFF) {
        uint8_t modrm = origBytes[pos + 1];
        uint8_t reg = (modrm >> 3) & 7;
        if (reg == 2)
          len = 2;  // call r/m
        else if (reg == 4)
          len = 2;  // jmp r/m
        else
          len = 2;  // generic FF xx
      } else if (b == 0x0F)
        len = 2;  // two-byte opcode
      else if (b == 0x84 || b == 0x85 || b == 0x8D)
        len = 6;  // jcc rel32, lea
      else
        len = 1;  // unknown: treat as 1-byte

      pos += len;
      if (pos >= 8) {
        overwriteLen = pos;
        break;
      }
    }
    if (overwriteLen < 8) overwriteLen = 8;  // fallback
  }

  Dbg("Hook: overwriting %d bytes of sub_459E90", overwriteLen);

  // Build trampoline in cave
  uint8_t* p = (uint8_t*)cave;

  // Replay the original overwritten instructions
  for (int i = 0; i < overwriteLen; i++) *p++ = origBytes[i];

  // pushad (save all regs)
  *p++ = 0x60;

  // mov eax, <AmOverlayHook>
  *p++ = 0xB8;
  *(uint32_t*)p = (uint32_t)(uintptr_t)AmOverlayHook;
  p += 4;

  // call eax
  *p++ = 0xFF;
  *p++ = 0xD0;

  // popad (restore all regs)
  *p++ = 0x61;

  // jmp rel32 (back to target + overwriteLen)
  *p++ = 0xE9;
  *(int32_t*)p =
      (int32_t)((target + overwriteLen) - ((uint32_t)(uintptr_t)p + 4));
  p += 4;

  s_hookCave = (uint32_t)(uintptr_t)cave;
  s_hookTarget = target;

  // Patch the original function overwrite first N bytes with jmp cave + nops
  DWORD oldProt = 0;
  VirtualProtect((void*)(uintptr_t)target, overwriteLen, PAGE_EXECUTE_READWRITE,
                 &oldProt);

  uint8_t* patch = (uint8_t*)(uintptr_t)target;
  patch[0] = 0xE9;  // jmp rel32
  *(int32_t*)(patch + 1) = (int32_t)((uint32_t)(uintptr_t)cave - (target + 5));
  for (int i = 5; i < overwriteLen; i++) patch[i] = 0x90;  // nop

  VirtualProtect((void*)(uintptr_t)target, overwriteLen, oldProt, &oldProt);

  s_hookInstalled = true;
  Dbg("Hook: installed on sub_459E90 (%08X) -> cave %08X", target, s_hookCave);
  return true;
}

// main loop
static int s_lastAct = -1;
static int s_lastLevel = -1;
static bool s_announced = false;

void ModuleTick() {
  static unsigned tick = 0;
  tick++;

  if (!g_armed) {
    if (!ArmFunctions()) {
      if (tick % 40 == 0)
        Dbg("tick: not armed yet (waiting for Game.exe base)");
      return;
    }
  }

  uint32_t player = Player();
  if (!player) {
    if (g_dataSet >= 0) {
      // Left the game (menu/lobby) forget reveal state.
      g_dataSet = -1;
      ResetDone();
      g_exitCount = 0;
      s_lastAct = -1;
      s_lastLevel = -1;
      s_announced = false;
      Dbg("tick: left game -- state reset");
    } else if (!PickDataLayout() && tick % 80 == 0) {
      Dbg("tick: scanner could not find data globals "
          "-- are you in a single-player game?");
    }
    return;
  }

  if (g_dataSet < 0 && !PickDataLayout())
    return; 

  int actNo = PlayerAct(player);
  uint32_t cur = GetPlayerLevelAddr(player);
  if (actNo < 0 || !cur) return;
  int levelNo = (int)SRead(cur + off::L_LEVELNO);

  if (actNo != s_lastAct || levelNo != s_lastLevel) {
    bool actChanged = (actNo != s_lastAct);
    s_lastAct = actNo;
    s_lastLevel = levelNo;
    if (actChanged) {
      s_announced = false;
      g_exitCount = 0;     
      ResetDone();          
    }

    // Only reveal the current level on level change (no full act walk).
    // Full act reveal only on act change.
    __try {
      RevealAct(player, actChanged);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      Dbg("tick: reveal faulted (swallowed)");
    }

    if (!s_announced) {
      s_announced = true;
      Dbg("revealed act %d (%s)", actNo + 1,
          AreaName((uint32_t)levelNo));
    } else {
      Dbg("revealed %s",
          AreaName((uint32_t)levelNo));
    }

    // Install the automap render-pass hook on first area enter
    if (!s_hookInstalled) InstallAutomapHook();
  }
}

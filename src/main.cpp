#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Structs.h"

extern volatile LONG g_run;

static DWORD WINAPI ModuleThread(LPVOID) {
  ConsoleStart();
  Dbg("# DiabloLodMapDll %s  thread started (pid %u)", MH_BUILD_TAG,
      GetCurrentProcessId());
  while (g_run) {
    __try {
      ModuleTick();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      Dbg("tick: unexpected fault swallowed");
    }
    Sleep(120);
  }
  Dbg("# module thread exiting");
  return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hInst);
    HANDLE t = CreateThread(nullptr, 0, ModuleThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
  } else if (reason == DLL_PROCESS_DETACH) {
    InterlockedExchange(&g_run, 0);
  }
  return TRUE;
}

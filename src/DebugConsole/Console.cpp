/*
  Console.cpp -- debug console window for the injected module.

  AllocConsole + freopen of stdout/stderr/stdin, with VT processing enabled
  so the ANSI codes the HUD uses also work here. printf with no console is
  a silent no-op, so calling this is purely additive: without it the module
  behaves exactly as before.
*/
#include "Structs.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>

void ConsoleStart()
{
    static bool s_on = false;
    if (s_on)
        return;
    s_on = true;

    if (!AllocConsole())
        return;

    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    SetConsoleTitleA("DiabloLodMap debug console");

    printf("[console] DiabloLodMap debug console on\n");
    fflush(stdout);
}

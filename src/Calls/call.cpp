/*
  call.cpp -- x86 trampoline bodies (Win32 only; Diablo II LoD is 32-bit).

  MSVC compiles functions containing __asm with a frame pointer, so locals
  stay at EBP-relative offsets and remain valid across the pushes below. We
  also load every local into a register before the first push, which makes
  the stubs safe even without relying on that.
*/
#ifdef _WIN64
#error DiabloLodMapDll must be built as Win32 (x86) -- Diablo II LoD is a 32-bit game.
#endif

#include "Structs.h"

uint32_t CallFn0(uint32_t fn)
{
    uint32_t r = 0;
    __asm
    {
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}

uint32_t CallFn1(uint32_t fn, uint32_t a)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [a]
        mov ecx, dword ptr [fn]
        push eax
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}

uint32_t CallFn2(uint32_t fn, uint32_t a, uint32_t b)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [b]
        push eax
        mov eax, dword ptr [a]
        push eax
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}

uint32_t CallFn3(uint32_t fn, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [c]
        push eax
        mov eax, dword ptr [b]
        push eax
        mov eax, dword ptr [a]
        push eax
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}

uint32_t CallFn5(uint32_t fn, uint32_t a, uint32_t b,
                 uint32_t c, uint32_t d, uint32_t e)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [e]
        push eax
        mov eax, dword ptr [d]
        push eax
        mov eax, dword ptr [c]
        push eax
        mov eax, dword ptr [b]
        push eax
        mov eax, dword ptr [a]
        push eax
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}

uint32_t CallFn6(uint32_t fn, uint32_t a, uint32_t b, uint32_t c,
                 uint32_t d, uint32_t e, uint32_t f)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [f]
        push eax
        mov eax, dword ptr [e]
        push eax
        mov eax, dword ptr [d]
        push eax
        mov eax, dword ptr [c]
        push eax
        mov eax, dword ptr [b]
        push eax
        mov eax, dword ptr [a]
        push eax
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}

// __fastcall, 1 arg: ECX.
uint32_t CallFast1(uint32_t fn, uint32_t a)
{
    uint32_t r = 0;
    __asm
    {
        push ebx
        push esi
        push edi
        mov ecx, dword ptr [a]
        mov eax, dword ptr [fn]
        call eax
        mov dword ptr [r], eax
        pop edi
        pop esi
        pop ebx
    }
    return r;
}

// __fastcall, 2 args: ECX, EDX.
uint32_t CallFast2(uint32_t fn, uint32_t a, uint32_t b)
{
    uint32_t r = 0;
    __asm
    {
        mov ecx, dword ptr [a]
        mov edx, dword ptr [b]
        mov eax, dword ptr [fn]
        call eax
        mov dword ptr [r], eax
    }
    return r;
}

// __fastcall, 3 args: ECX, EDX, stack.
uint32_t CallFast3(uint32_t fn, uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [c]
        push eax
        mov ecx, dword ptr [a]
        mov edx, dword ptr [b]
        mov eax, dword ptr [fn]
        call eax
        mov dword ptr [r], eax
    }
    return r;
}

// __fastcall, 4 args: ECX, EDX, stack (d then c).
uint32_t CallFast4(uint32_t fn, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [d]
        push eax
        mov eax, dword ptr [c]
        push eax
        mov ecx, dword ptr [a]
        mov edx, dword ptr [b]
        mov eax, dword ptr [fn]
        call eax
        mov dword ptr [r], eax
    }
    return r;
}

// __fastcall, 5 args: ECX, EDX, stack (e, d, c).
uint32_t CallFast5(uint32_t fn, uint32_t a, uint32_t b, uint32_t c,
                   uint32_t d, uint32_t e)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [e]
        push eax
        mov eax, dword ptr [d]
        push eax
        mov eax, dword ptr [c]
        push eax
        mov ecx, dword ptr [a]
        mov edx, dword ptr [b]
        mov eax, dword ptr [fn]
        call eax
        mov dword ptr [r], eax
    }
    return r;
}

// Register call, single argument in EAX (get_unit_name).
uint32_t CallEax1(uint32_t fn, uint32_t a)
{
    uint32_t r = 0;
    __asm
    {
        push ebx
        push esi
        push edi
        mov eax, dword ptr [a]
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
        pop edi
        pop esi
        pop ebx
    }
    return r;
}

// reveal_automap_room: __stdcall (3 stack args: room1, clip, layer).
uint32_t CallRevealRoom(uint32_t fn, uint32_t room1, uint32_t clip, uint32_t layer)
{
    uint32_t r = 0;
    __asm
    {
        mov eax, dword ptr [layer]
        push eax
        mov eax, dword ptr [clip]
        push eax
        mov eax, dword ptr [room1]
        push eax
        mov ecx, dword ptr [fn]
        call ecx
        mov dword ptr [r], eax
    }
    return r;
}
// (__usercall). Older builds used ECX (__fastcall) or EAX (register). Setting
// all three satisfies every variant safely; the function reads the one it
// wants.
uint32_t CallInitLayer(uint32_t fn, uint32_t levelNo)
{
    uint32_t r = 0;
    __asm
    {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [levelNo]
        mov ecx, dword ptr [levelNo]
        mov eax, dword ptr [levelNo]
        mov edx, dword ptr [fn]
        call edx
        mov dword ptr [r], eax
        pop edi
        pop esi
        pop ebx
    }
    return r;
}

// Part of CRC-32C library: https://crc32c.machinezoo.com/
// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>

void __crc32_init();

BOOL DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
	__crc32_init();
    return TRUE;
}


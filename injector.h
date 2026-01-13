#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <Windows.h>

int Injecter(DWORD pid, char* FullDLLPath) {

    HANDLE mainHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (mainHandle == NULL) {
        MessageBoxA(NULL, "Injecter : Error OpenProcess", "debug", MB_OK);
        return 1;
    }

    LPVOID allocMem = VirtualAllocEx(mainHandle, NULL, lstrlenA(FullDLLPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (allocMem == NULL) {
        MessageBoxA(NULL, "Injecter : Error VirtalAlloc", "debug", MB_OK);
        CloseHandle(mainHandle);
        return 1;
    }
    BOOL WriteMem = WriteProcessMemory(mainHandle, allocMem, FullDLLPath, lstrlenA(FullDLLPath) + 1, NULL);
    if (WriteMem == 0) {
        MessageBoxA(NULL, "Injecter : Error WriteProcessMemory", "debug", MB_OK);
        VirtualFree(allocMem, lstrlenA(FullDLLPath) + 1, MEM_RELEASE);
        CloseHandle(mainHandle);
        return 1;
    }
    HMODULE hModule = GetModuleHandleA("kernel32.dll");
    if (hModule == NULL) {
        MessageBoxA(NULL, "Injecter : Error GetModuleHandle", "debug", MB_OK);
        VirtualFree(allocMem, lstrlenA(FullDLLPath) + 1, MEM_RELEASE);
        CloseHandle(mainHandle);
        return 1;
    }
    FARPROC LoadLibAddr = GetProcAddress(hModule, "LoadLibraryA");
    if (LoadLibAddr == NULL) {
        MessageBoxA(NULL, "Injecter : Error GetProcAddress", "debug", MB_OK);
        VirtualFree(allocMem, lstrlenA(FullDLLPath) + 1, MEM_RELEASE);
        CloseHandle(mainHandle);
        return 1;
    }
    HANDLE hSeconde = CreateRemoteThread(mainHandle, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibAddr, allocMem, 0, NULL);
    if (hSeconde == NULL) {
        MessageBoxA(NULL, "Injecter : Error CreateRemoteThread", "debug", MB_OK);
        VirtualFree(allocMem, lstrlenA(FullDLLPath) + 1, MEM_RELEASE);
        CloseHandle(mainHandle);
        return 1;
    }

    MessageBoxA(NULL, "Succes", "debug", MB_OK);
    CloseHandle(mainHandle);
    return 0;
}

#include <stdint.h>
#include <stdio.h>

uintptr_t ArgsCmdLineOverride_OrignalFunc = 0;
uintptr_t ArgsCmdLineOverride_Return = 0;
int bWantGPUBenchmark = 0;
wchar_t cmd[260] {};

static const wchar_t* readCMDLine()
{
    _snwprintf_s(cmd, __crt_countof(cmd), L""
        "-ao BENCHMARK_SCENE_INDEX %d "
        "-ao AUTO_QUIT_ENGINE 120 -ao START_BENCHMARK 1 "
        "ConsoleCmd UI_ShowProfileData 1 "
        "ConsoleCmd UI_ShowProfileStats 1", bWantGPUBenchmark ? 1 : 2);
    wprintf_s(L"formmated cmd: %s\n", cmd);
    return cmd;
}

// Intelisense jail
void __attribute__((naked)) ArgsCmdLineOverrideAsm()
{
    __asm
    {
        call qword ptr[rip + ArgsCmdLineOverride_OrignalFunc];
        call readCMDLine;
        mov r12, rax;
        jmp qword ptr[rip + ArgsCmdLineOverride_Return];
    }
}

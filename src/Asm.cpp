#include <stdint.h>

uintptr_t ArgsCmdLineOverride_OrignalFunc = 0;
uintptr_t ArgsCmdLineOverride_Return = 0;

static const wchar_t* readCMDLine()
{
    return L"-ao BENCHMARK_SCENE_INDEX 2 -ao AUTO_QUIT_ENGINE 120 -ao START_BENCHMARK 1 ConsoleCmd UI_ShowProfileData 1 ConsoleCmd UI_ShowProfileStats 1";
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

#pragma once

#include <stdint.h>

extern uintptr_t ArgsCmdLineOverride_OrignalFunc;
extern uintptr_t ArgsCmdLineOverride_Return;

// Intelisense jail
void __attribute__((naked)) ArgsCmdLineOverrideAsm();

#include "BenchmarkStarter.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

// CSGOSimple's pattern scan
// https://github.com/OneshotGH/CSGOSimple-master/blob/master/CSGOSimple/helpers/utils.cpp
std::uint8_t* PatternScan(void* module, const char* signature)
{
    static auto pattern_to_byte = [](const char* pattern)
        {
            auto bytes = std::vector<int>{};
            auto start = const_cast<char*>(pattern);
            auto end = const_cast<char*>(pattern) + strlen(pattern);

            for (auto current = start; current < end; ++current)
            {
                if (*current == '?')
                {
                    ++current;
                    if (*current == '?')
                        ++current;
                    bytes.push_back(-1);
                }
                else
                {
                    bytes.push_back(strtoul(current, &current, 16));
                }
            }
            return bytes;
        };

    auto dosHeader = (PIMAGE_DOS_HEADER)module;
    auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

    auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    auto patternBytes = pattern_to_byte(signature);
    auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

    auto s = patternBytes.size();
    auto d = patternBytes.data();

    for (auto i = 0ul; i < sizeOfImage - s; ++i)
    {
        bool found = true;
        for (auto j = 0ul; j < s; ++j)
        {
            if (scanBytes[i + j] != d[j] && d[j] != -1)
            {
                found = false;
                break;
            }
        }
        if (found)
        {
            return &scanBytes[i];
        }
    }
    return nullptr;
}

bool DetourFunction32(void* src, void* dst, int len)
{
    if (len < 5)
    {
        return false;
    }
    DWORD curProtection;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

    memset(src, 0x90, len);

    uintptr_t relativeAddress = ((uintptr_t)dst - (uintptr_t)src) - 5;

    *(BYTE*)src = 0xE9;
    *(uint32_t*)((uintptr_t)src + 1) = relativeAddress;

    DWORD temp;
    VirtualProtect(src, len, curProtection, &temp);

    return true;
}

void* DetourFunction64(void* pSource, void* pDestination, DWORD dwLen)
{
    constexpr DWORD MinLen = 14;

    if (dwLen < MinLen)
    {
        return nullptr;
    }

    BYTE stub[] =
    {
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp qword ptr [$+6]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // ptr
    };

    DWORD dwOld = 0;
    VirtualProtect(pSource, dwLen, PAGE_EXECUTE_READWRITE, &dwOld);

    // orig
    memcpy(stub + 6, &pDestination, 8);
    memcpy(pSource, stub, sizeof(stub));

    for (DWORD i = MinLen; i < dwLen; i++)
    {
        *(BYTE*)((DWORD_PTR)pSource + i) = 0x90;
    }

    VirtualProtect(pSource, dwLen, dwOld, &dwOld);
    return pSource;
}

uintptr_t ArgsCmdLineOverride_Return = 0;
uintptr_t ArgsCmdLineOverride_OrignalFunc = 0;

static const wchar_t* readCMDLine()
{
    return L"-ao BENCHMARK_SCENE_INDEX 2 -ao AUTO_QUIT_ENGINE 120 -ao START_BENCHMARK 1 ConsoleCmd UI_ShowProfileData 1 ConsoleCmd UI_ShowProfileStats 1";
}

static void __attribute__((naked)) ArgsCmdLineOverrideAsm()
{
    __asm
    {
        call qword ptr[rip + ArgsCmdLineOverride_OrignalFunc];
        call readCMDLine;
        mov r12, rax;
        jmp qword ptr[rip + ArgsCmdLineOverride_Return];
    }
}

uintptr_t ReadLEA32(uintptr_t Address, size_t offset, size_t lea_size, size_t lea_opcode_size)
{
    uintptr_t Address_Result = Address;
    uintptr_t Patch_Address = 0;
    int32_t lea_offset = 0;
    uintptr_t New_Offset = 0;
    if (Address_Result)
    {
        if (offset)
        {
            Patch_Address = offset + Address_Result;
            lea_offset = *(int32_t*)(lea_size + Address_Result);
            New_Offset = Patch_Address + lea_offset + lea_opcode_size;
        }
        else
        {
            Patch_Address = Address_Result;
            lea_offset = *(int32_t*)(lea_size + Address_Result);
            New_Offset = Patch_Address + lea_offset + lea_opcode_size;
        }
        return New_Offset;
    }
    return 0;
}

void BenchmarkStarter::Init()
{
    // this gets executed more than once for some reason
    if (m_ModuleStarted)
    {
        return;
    }
    // Find `ZMicroprofileUIController`
    m_gameBase = GetModuleHandle(NULL);
    if (m_gameBase)
    {
        uintptr_t ZMicroprofileUIControllerAddr = (uintptr_t)PatternScan(m_gameBase, "48 8b 0d ? ? ? ? 48 85 c9 74 ?? f3 41 0f 10 0c 24 e8 ? ? ? ? 49 8b 8e");
        if (ZMicroprofileUIControllerAddr)
        {
            m_pUIController = (ZMicroprofileUIController**)ReadLEA32(ZMicroprofileUIControllerAddr, 0, 3, 7);
            printf_s("Pointer for ZMicroprofileUIController: 0x%p\n", m_pUIController);
        }

        wchar_t* ProgramCMD = GetCommandLineW();
        if (!wcsstr(ProgramCMD, L"BENCHMARK_SCENE_INDEX") || !wcsstr(ProgramCMD, L"START_BENCHMARK 1"))
        {
            uintptr_t overrideCMDline = (uintptr_t)PatternScan(m_gameBase, "e8 ? ? ? ? 4c 8d 05 ? ? ? ? ba 01 00 00 00 33 c9");
            if (overrideCMDline)
            {
                ArgsCmdLineOverride_OrignalFunc = ReadLEA32(overrideCMDline, 0, 1, 5);
                if (ArgsCmdLineOverride_OrignalFunc)
                {
                    ArgsCmdLineOverride_Return = overrideCMDline + 5;
                    void* jumpPattern = (void*)PatternScan(m_gameBase, "CC CC CC CC CC CC CC CC CC CC CC CC CC CC");
                    // Call this then override the R12 with our commandline.txt data
                    DetourFunction32((void*)overrideCMDline, jumpPattern, 5);
                    DetourFunction64(jumpPattern, (void*)ArgsCmdLineOverrideAsm, 14);
                    wprintf_s(L"installed override cmdline parser.\n");
                }
                m_WantedBenchmark = true;
            }
        }
        else
        {
            wprintf_s(L"Found benchmark parameters, could be launching from launcher.\n");
            m_WantedBenchmark = true;
        }
        wprintf_s(L"GetCommandLineW: %s\n", ProgramCMD);
    }
    m_ModuleStarted = true;
}

void BenchmarkStarter::OnEngineInitialized()
{
    Logger::Info("BenchmarkStarter has been initialized!");

    // Register a function to be called on every game frame while the game is in play mode.
    const ZMemberDelegate<BenchmarkStarter, void(const SGameUpdateEvent&)> s_Delegate(this, &BenchmarkStarter::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

BenchmarkStarter::~BenchmarkStarter()
{
    // Unregister our frame update function when the mod unloads.
    const ZMemberDelegate<BenchmarkStarter, void(const SGameUpdateEvent&)> s_Delegate(this, &BenchmarkStarter::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void BenchmarkStarter::OnDrawMenu()
{
}

void BenchmarkStarter::OnDrawUI(bool p_HasFocus)
{
}

void SendInputWrapper(WORD inputKey)
{
    INPUT inputs[2]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = inputKey;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = inputKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput((sizeof(inputs) / sizeof(inputs[0])), inputs, sizeof(INPUT));
}

void BenchmarkStarter::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    if (m_BenchmarkCompleted)
    {
        return;
    }
    // This function is called every frame while the game is in play mode.
    if (m_WantedBenchmark && m_pUIController[0])
    {
        if (m_pUIController[0]->benchStarted && m_pUIController[0]->benchStarted2)
        {
            if (!m_BenchAlreadyStarted)
            {
                SendInputWrapper(VK_F11);
                m_BenchAlreadyStarted = true;
            }
            if (m_pUIController[0]->BenchCompletedFlag != 0x40000000 && !m_BenchmarkCompleted)
            {
                SendInputWrapper(VK_F11);
                m_BenchmarkCompleted = true;
            }
        }
    }
}

DECLARE_ZHM_PLUGIN(BenchmarkStarter);

// Cmake `set` isn't working for some reason
#define _UNICODE
#define UNICODE

#include <yaml-cpp/yaml.h>

#include "BenchmarkStarter.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

#include "Asm.h"

#include <shlobj.h>
#include <strsafe.h>
#include <io.h>
#include <filesystem>

#define PROJECT_NAME "BenchmarkStarter"
#define BENCHMARK_PATH_DEFAULT "C:\\benchmarking\\Hitman3"
// helper macros
#define GET_KEY_FMT(key, section, as_type, default_, fmt) key = config[section][#key].as<as_type>(default_); printf_s("" #key ": " fmt "\n", key)
#define GET_KEY_BOOL(key, section ,as_type ,default_) key = config[section][#key].as<as_type>(default_); printf_s("" #key ": %s\n", key ? "true" : "false")
#define ADD_YAML_KEY(emit, key, val) (void)key; emit << YAML::Key << #key << YAML::Value << val; printf_s("Adding `" #key "` with value `" #val "`\n")

bool bBenchmarkingOnly = true;
extern int bWantGPUBenchmark;
int iRestartTimer {}, iSendInputAPIDelayMs {}, iBenchmarkKey {};
bool createdConfigQuit {};

// Benchmark config index
size_t iBenchmarkIndex {};
size_t iBenchmarkSize {};

std::filesystem::path config_path = PROJECT_NAME ".yaml";
std::filesystem::path index_path = PROJECT_NAME ".index.yaml";

std::filesystem::path currentDir {};
std::string currentConfigPath {};
std::string currentConfigPathFileName {};
std::filesystem::path fs_currentConfigPath {};
const char* pCurrentConfigPath {};

static void saveIndex(size_t index, const std::string& indexFile)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "BenchmarkIndex" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "index" << YAML::Value << index;
    out << YAML::EndMap;
    out << YAML::EndMap;
    std::ofstream fout(indexFile);
    fout << out.c_str();
    fout << "\n";
    fout.close();
}

static void RestartAppSteamTimer(int appid, int timer)
{
#define CMD_FILE "restart.bat"
    FILE* batch = NULL;
    errno_t err = _wfopen_s(&batch, TEXT(CMD_FILE), L"w");
    if (!err && batch)
    {
        fwprintf_s(batch, L"timeout /t %d /nobreak\n", timer);
        fwprintf_s(batch, L"start steam://run/%d\n", appid);
        fclose(batch);
        ShellExecute(NULL, NULL, TEXT(CMD_FILE), NULL, NULL, SW_SHOWNORMAL);
        ExitProcess(0);
    }
#undef CMD_FILE
}

static void RestartApp()
{
#if 1
    RestartAppSteamTimer(1659040, iRestartTimer);
#else
    STARTUPINFO startinfo {};
    PROCESS_INFORMATION processinfo {};
    std::wstring path = currentDir.native();
    if (!CreateProcess(
        L"HITMAN3.exe",
        GetCommandLine(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        path.c_str(),
        &startinfo,
        &processinfo))
    {
        wprintf_s(L"CreateProcess(%s) failed %d\n", GetCommandLine(), GetLastError());
        MessageBox(0, L"Failed to create process. Application may not restart.", L"" PROJECT_NAME, MB_ICONERROR | MB_OK);
    }
    else
    {
        ExitProcess(0);
    }
#endif
}

static void SaveIndexInc()
{
    if (std::ifstream(index_path))
    {
        YAML::Node indexNode = YAML::LoadFile(index_path.string());
        iBenchmarkIndex++;
        indexNode["BenchmarkIndex"]["index"] = iBenchmarkIndex;
        saveIndex(iBenchmarkIndex, index_path.string());
    }
    else
    {
        MessageBox(0, L"File not found", L"", 0);
    }
}

static void SaveIndexRestart()
{
    SaveIndexInc();
    RestartApp();
}

#define CHECK_FILE "C:\\benchmarking\\check.txt"

static void CheckScriptFile()
{
    BOOL check = GetFileAttributes(TEXT(CHECK_FILE)) != INVALID_FILE_ATTRIBUTES;
    if (!check)
    {
        HANDLE fd = CreateFile(TEXT(CHECK_FILE), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (fd == INVALID_HANDLE_VALUE)
        {
            return;
        }
        else
        {
            const wchar_t* default_ini = L"Example file created by patch\n";
            DWORD dwBytesWritten = 0;
            WriteFile(fd,
                default_ini,
                (wcslen(default_ini) * sizeof(wchar_t)),
                &dwBytesWritten,
                0);
            CloseHandle(fd);
        }
    }
}

static void ReadConfig()
{
    currentDir = std::filesystem::current_path();
    {
        std::filesystem::path cfgPath = currentDir / config_path;
        index_path = currentDir / index_path;
        std::ifstream yamlFile(cfgPath);
        if (!yamlFile)
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "Settings" << YAML::Value << YAML::BeginMap;
            /*
            out << YAML::Key << "iRestartTimer" << YAML::Value << 30;
            out << YAML::Key << "bWantGPUBenchmark" << YAML::Value << false;
            */
            ADD_YAML_KEY(out, iRestartTimer, 30);
            ADD_YAML_KEY(out, iBenchmarkKey, VK_F11); 
            out << YAML::Comment(
                "See VK codes https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes for valid keys codes\n"
                "122 is F11 by default");
            ADD_YAML_KEY(out, iSendInputAPIDelayMs, 16);
            out << YAML::Comment("Delay for between Pressing and Releasing Keys, try higher values if data provider doesn't detect key");
            ADD_YAML_KEY(out, bWantGPUBenchmark, false);
            out << YAML::EndMap;
            out << YAML::Newline;
            out << YAML::Comment("Input path must be absolute path and no quotation marks around it.");
            out << YAML::Key << "ConfigPath" << YAML::Value << YAML::BeginSeq;            out << YAML::Comment(
                "Path to save file must be absolute path and no quotation marks.\n"
                "(i.e " BENCHMARK_PATH_DEFAULT "\\Hitman3_720p.reg = good)\n"
                "(But \"" BENCHMARK_PATH_DEFAULT "\\Hitman3_720p.reg\" = bad)"
            );
            out << BENCHMARK_PATH_DEFAULT "\\Hitman3_720p.reg";
            out << BENCHMARK_PATH_DEFAULT "\\Hitman3_1080p.reg";
            out << BENCHMARK_PATH_DEFAULT "\\Hitman3_1440p.cfg";
            out << BENCHMARK_PATH_DEFAULT "\\Hitman3_2160p.cfg";
            out << YAML::EndSeq;
            out << YAML::EndMap;
            std::ofstream yamlFileOut(config_path);
            yamlFileOut << out.c_str();
            yamlFileOut << "\n";
            {
                wchar_t msg[512] {};
                _snwprintf_s(msg, _countof(msg), L"Created patch config file at \"%s\". Application will now quit for user to adjust file.", cfgPath.u16string().c_str());
                MessageBox(0, msg, L"" PROJECT_NAME, MB_ICONINFORMATION | MB_OK);
                createdConfigQuit = true;
            }
            return;
        }
    }
    wprintf_s(L"Current directory: %s\n", currentDir.c_str());
    YAML::Node config = YAML::LoadFile(config_path.string());
    YAML::Node indexNode;
    if (std::ifstream(index_path))
    {
        indexNode = YAML::LoadFile(index_path.string());
    }
    else
    {
        indexNode["BenchmarkIndex"]["index"] = 0;
        saveIndex(0, index_path.string());
    }
    {
        if (config["Settings"])
        {
            GET_KEY_FMT(iRestartTimer, "Settings", int, 30, "%d");
            GET_KEY_FMT(iBenchmarkKey, "Settings", int, VK_F11, "%d");
            GET_KEY_FMT(iSendInputAPIDelayMs, "Settings", int, 16, "%d");
            GET_KEY_BOOL(bWantGPUBenchmark, "Settings", bool, false);
        }
        if (config["ConfigPath"])
        {
            iBenchmarkSize = config["ConfigPath"].size();
            iBenchmarkIndex = indexNode["BenchmarkIndex"]["index"].as<size_t>();
            if (iBenchmarkIndex >= iBenchmarkSize)
            {
                saveIndex(0, index_path.string());
                CheckScriptFile();
                iBenchmarkIndex = 0;
                int retID = MessageBox(0,
                    L"Benchmark Completed.\n"
                    "Do you want to start game or quit?\n"
                    "Starting game will not load into benchmark but config file will remain the same.",
                    L"" PROJECT_NAME, MB_YESNO | MB_ICONWARNING);
                switch (retID)
                {
                    case IDYES:
                    {
                        bBenchmarkingOnly = false;
                        break;
                    }
                    case IDNO:
                    {
                        ExitProcess(0);
                        break;
                    }
                }
            }
            currentConfigPath = config["ConfigPath"][iBenchmarkIndex].as<std::string>();
            fs_currentConfigPath = currentConfigPath;
            currentConfigPathFileName = fs_currentConfigPath.stem().string();
            pCurrentConfigPath = currentConfigPath.c_str();

            if (!fs_currentConfigPath.empty())
            {
                errno_t cfg_ret = _access_s(currentConfigPath.c_str(), 0);
                if (cfg_ret)
                {
                    wchar_t msg[512] {};
                    _snwprintf_s(msg, _countof(msg), L"One or more requested files does not exist.\n"
                        "Config file: %hs (%s)",
                        currentConfigPath.c_str(),
                        cfg_ret == ENOENT ? L"not found" : L"exists");
                    MessageBox(0, msg, L"" PROJECT_NAME " Startup Error", MB_OK | MB_ICONERROR);
                    createdConfigQuit = true;
                    return;
                }
                else
                {
                    wchar_t cmd[320] {};
                    _snwprintf_s(cmd, _countof(cmd), L"reg import \"%hs\"", currentConfigPath.c_str());
                    wprintf_s(L"Running cmd: %s\n", cmd);
                    STARTUPINFO startinfo {};
                    PROCESS_INFORMATION processinfo {};
                    if (!CreateProcess(
                        NULL,
                        cmd,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &startinfo,
                        &processinfo))
                    {
                        wchar_t msg[128] {};
                        _snwprintf_s(msg, _countof(msg), L"CreateProcess failed %d\n", GetLastError());
                        MessageBox(0, msg, L"" PROJECT_NAME " Startup Error", MB_ICONERROR | MB_OK);
                    }
                    wchar_t docuementPath[_MAX_PATH] {};
                    // I find this funny
                    // #define CSIDL_MYDOCUMENTS CSIDL_PERSONAL //  Personal was just a silly name for My Documents
                    HRESULT result = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docuementPath);
                    if (SUCCEEDED(result))
                    {
                        wchar_t CX_Path[_MAX_PATH] {};
                        StringCchCat(CX_Path, _countof(CX_Path), docuementPath);
                        StringCchCat(CX_Path, _countof(CX_Path), L"\\CapFrameX\\Profile.txt");
                        HANDLE fd = CreateFile(CX_Path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                        if (fd == INVALID_HANDLE_VALUE)
                        {
                            wchar_t msg[512] {};
                            _snwprintf_s(msg, _countof(msg), L"Unable to create file %s because handle was invalid. (0x%08x)", CX_Path, fd);
                            MessageBox(0, msg, L"" PROJECT_NAME " Startup Error", MB_ICONERROR | MB_OK);
                        }
                        else
                        {
                            DWORD dwBytesWritten = 0;
                            std::string fileData = (bWantGPUBenchmark ? "GPU_" : "CPU_") + currentConfigPathFileName + "\n";
                            if (!WriteFile(fd, fileData.c_str(), fileData.length(), &dwBytesWritten, 0))
                            {
                                MessageBox(0, L"Failed to write profile name.\nFile name might be wrong after captures.", L"" PROJECT_NAME " Startup Error", MB_ICONERROR | MB_OK);
                            }
                            else
                            {
                                CloseHandle(fd);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            MessageBox(0, L"ConfigPath not found in YAML file", L"" PROJECT_NAME " Startup Error", MB_OK | MB_ICONERROR);
            ExitProcess(1);
        }
    }
}

// CSGOSimple's pattern scan
// https://github.com/OneshotGH/CSGOSimple-master/blob/master/CSGOSimple/helpers/utils.cpp
std::uint8_t* PatternScan(void* module, const char* signature)
{
    static auto pattern_to_byte = [](const char* pattern)
        {
            auto bytes = std::vector<int> {};
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
    ReadConfig();
    if (createdConfigQuit)
    {
        ExitProcess(0);
        return;
    }
    if (!bBenchmarkingOnly)
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
                    DetourFunction64(jumpPattern, (void*)&ArgsCmdLineOverrideAsm, 14);
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
    INPUT inputs[1] {};
    const LPARAM Extras = GetMessageExtraInfo();
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = inputKey;
    inputs[0].ki.dwExtraInfo = Extras;
    /*
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = inputKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput((sizeof(inputs) / sizeof(inputs[0])), inputs, sizeof(INPUT));
    */
    SendInput(_countof(inputs), inputs, sizeof(INPUT));
    Sleep(iSendInputAPIDelayMs); // huh? this worksaround frameview not picking up our input
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(_countof(inputs), inputs, sizeof(INPUT));
}

void BenchmarkStarter::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    if (m_BenchmarkCompleted)
    {
        m_TimeToQuit += p_UpdateEvent.m_GameTimeDelta.ToSeconds();
    }
    // want to restart game ourselves
    if (m_TimeToQuit > 4.0)
    {
        SaveIndexRestart();
        return;
    }
    // This function is called every frame while the game is in play mode.
    if (m_WantedBenchmark && m_pUIController[0])
    {
        if (m_pUIController[0]->benchStarted && m_pUIController[0]->benchStarted2)
        {
            if (!m_BenchAlreadyStarted)
            {
                SendInputWrapper(iBenchmarkKey);
                m_BenchAlreadyStarted = true;
            }
            if (m_pUIController[0]->BenchCompletedFlag != 0x40000000 && !m_BenchmarkCompleted)
            {
                SendInputWrapper(iBenchmarkKey);
                m_BenchmarkCompleted = true;
            }
        }
    }
}

DECLARE_ZHM_PLUGIN(BenchmarkStarter);

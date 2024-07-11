#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

struct ZMicroprofileUIController
{
    void* ptr1;
    void* ptr2;
    int unk1;
    int unk2;
    int unk3;
    int unk4;
    void* ptr3;
    int unk5;
    int unk6;
    void* ptr4;
    float fUnk1;
    int unk7;
    float fUnk2;
    bool benchStarted;
    bool benchStarted2;
    void* ptr5;
    void* ptr6;
    int unk8;
    int unk9;
    int unk10;
    int unk11;
    int maybe_ptr;
    int BenchCompletedFlag;
};

class BenchmarkStarter : public IPluginInterface
{
public:
    void Init() override;
    void OnEngineInitialized() override;
    ~BenchmarkStarter() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

private:
    HMODULE m_gameBase = 0;
    ZMicroprofileUIController** m_pUIController = 0;
    bool m_BenchAlreadyStarted = false;
    bool m_WantedBenchmark = false;
    bool m_ModuleStarted = false;
    bool m_BenchmarkCompleted = false;
    double m_TimeToQuit = 0;
};

DEFINE_ZHM_PLUGIN(BenchmarkStarter)

#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

class BenchmarkStarter : public IPluginInterface
{
public:
    void OnEngineInitialized() override;
    ~BenchmarkStarter() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    DECLARE_PLUGIN_DETOUR(BenchmarkStarter, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData);

private:
    bool m_ShowMessage = false;
};

DEFINE_ZHM_PLUGIN(BenchmarkStarter)

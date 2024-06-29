#include "BenchmarkStarter.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

void BenchmarkStarter::OnEngineInitialized()
{
    Logger::Info("BenchmarkStarter has been initialized!");

    // Register a function to be called on every game frame while the game is in play mode.
    const ZMemberDelegate<BenchmarkStarter, void(const SGameUpdateEvent&)> s_Delegate(this, &BenchmarkStarter::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Install a hook to print the name of the scene every time the game loads a new one.
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &BenchmarkStarter::OnLoadScene);
}

BenchmarkStarter::~BenchmarkStarter()
{
    // Unregister our frame update function when the mod unloads.
    const ZMemberDelegate<BenchmarkStarter, void(const SGameUpdateEvent&)> s_Delegate(this, &BenchmarkStarter::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void BenchmarkStarter::OnDrawMenu()
{
    // Toggle our message when the user presses our button.
    if (ImGui::Button(ICON_MD_LOCAL_FIRE_DEPARTMENT " BenchmarkStarter"))
    {
        m_ShowMessage = !m_ShowMessage;
    }
}

void BenchmarkStarter::OnDrawUI(bool p_HasFocus)
{
    if (m_ShowMessage)
    {
        // Show a window for our mod.
        if (ImGui::Begin("BenchmarkStarter", &m_ShowMessage))
        {
            // Only show these when the window is expanded.
            ImGui::Text("Hello from BenchmarkStarter!");
        }
        ImGui::End();
    }
}

void BenchmarkStarter::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
    // This function is called every frame while the game is in play mode.
}

DEFINE_PLUGIN_DETOUR(BenchmarkStarter, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData)
{
    Logger::Debug("Loading scene: {}", p_SceneData.m_sceneName);
    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(BenchmarkStarter);

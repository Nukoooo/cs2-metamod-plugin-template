#include "gamesystem.hpp"
#include "gameconfig.hpp"

CBaseGameSystemFactory** CBaseGameSystemFactory::sm_pFirst = nullptr;

CGameSystem g_GameSystem;
IGameSystemFactory* CGameSystem::sm_Factory = nullptr;

// This mess is needed to get the pointer to sm_pFirst so we can insert game systems
bool InitGameSystems()
{
    // This signature directly points to the instruction referencing sm_pFirst, and the opcode is 3 bytes so we skip those
    Address ptr = g_pGameConfig->ResolveSignature("IGameSystem_InitAllSystems_pFirst");

    if (!ptr.is_valid())
        return false;

    // Now grab our pointer
    CBaseGameSystemFactory::sm_pFirst = ptr.rel(3).cast<CBaseGameSystemFactory**>();

    // And insert the game system(s)
    CGameSystem::sm_Factory = new CGameSystemStaticFactory<CGameSystem>("CS2Fixes_GameSystem", &g_GameSystem);

    return true;
}

GS_EVENT_MEMBER(CGameSystem, BuildGameSessionManifest)
{
    spdlog::info("CGameSystem::BuildGameSessionManifest");

    IEntityResourceManifest* pResourceManifest = msg->m_pResourceManifest;

    // This takes any resource type, model or not
    // Any resource adding MUST be done here, the resource manifest is not long-lived
    // pResourceManifest->AddResource("characters/models/my_character_model.vmdl");
}

// Called every frame before entities think
GS_EVENT_MEMBER(CGameSystem, ServerPreEntityThink)
{
}

// Called every frame after entities think
GS_EVENT_MEMBER(CGameSystem, ServerPostEntityThink)
{
}

#pragma once

#include "cbaseplayercontroller.h"
#include "services.h"
#include "../../interfaces.hpp"

class CCSPlayerController : public CBasePlayerController
{
  public:
    DECLARE_SCHEMA_CLASS(CCSPlayerController);

    SCHEMA_FIELD(CCSPlayerController_InGameMoneyServices*, m_pInGameMoneyServices)
    SCHEMA_FIELD(CCSPlayerController_ActionTrackingServices*, m_pActionTrackingServices)
    SCHEMA_FIELD(bool, m_bPawnIsAlive);
    SCHEMA_FIELD(CHandle<CCSPlayerPawn>, m_hPlayerPawn);
    SCHEMA_FIELD(CHandle<CCSPlayerController>, m_hOriginalControllerOfCurrentPawn);

    static CCSPlayerController* FromPawn(CCSPlayerPawn* pawn)
    {
        return (CCSPlayerController*)pawn->m_hController().Get();
    }

    static CCSPlayerController* FromSlot(CPlayerSlot slot)
    {
        return (CCSPlayerController*)g_pEntitySystem->GetEntityInstance(CEntityIndex(slot.Get() + 1));
    }

    // Returns the actual player pawn
    CCSPlayerPawn *GetPlayerPawn()
    {
        return m_hPlayerPawn().Get();
    }

    /*ZEPlayer* GetZEPlayer()
    {
        return g_playerManager->GetPlayer(GetPlayerSlot());
    }

    void ChangeTeam(int iTeam)
    {
        static int offset = g_GameConfig->GetOffset("CCSPlayerController_ChangeTeam");
        CALL_VIRTUAL(void, offset, this, iTeam);
    }

    void SwitchTeam(int iTeam)
    {
        if (!IsController())
            return;

        if (iTeam == CS_TEAM_SPECTATOR)
        {
            ChangeTeam(iTeam);
        }
        else
        {
            addresses::CCSPlayerController_SwitchTeam(this, iTeam);
        }
    }

    void Respawn()
    {
        CCSPlayerPawn *pPawn = GetPlayerPawn();
        if (!pPawn || pPawn->IsAlive())
            return;

        SetPawn(pPawn);
        static int offset = g_GameConfig->GetOffset("CCSPlayerController_Respawn");
        CALL_VIRTUAL(void, offset, this);
    }*/

    CSPlayerState GetPawnState()
    {
        // All CS2 pawns are derived from this
        CCSPlayerPawnBase *pPawn = (CCSPlayerPawnBase*)GetPawn();

        // The player is still joining so their pawn doesn't exist yet, and STATE_WELCOME is what they start with
        if (!pPawn)
            return STATE_WELCOME;

        return pPawn->m_iPlayerState();
    }

    CSPlayerState GetPlayerPawnState()
    {
        CCSPlayerPawn *pPawn = GetPlayerPawn();

        // The player is still joining so their pawn doesn't exist yet, and STATE_WELCOME is what they start with
        if (!pPawn)
            return STATE_WELCOME;

        return pPawn->m_iPlayerState();
    }

    Z_CBaseEntity *GetObserverTarget()
    {
        auto pPawn = GetPawn();

        if (!pPawn)
            return nullptr;

        return pPawn->m_pObserverServices->m_hObserverTarget().Get();
    }
};
#pragma once
#include <cstdint>

class CEntityInstance;
class CEntityIdentity;
class CBasePlayerController;
class CCSPlayerController;
class CCSPlayerPawn;
class CBaseModelEntity;
class Z_CBaseEntity;
class CGameConfig;
class CEntitySystem;
class IEntityFindFilter;
class CGameRules;
class CEntityKeyValues;
class IRecipientFilter;
class CTakeDamageInfo;

struct EmitSound_t;
struct SndOpEventGuid_t;

#if defined(_WIN32)
#define FASTCALL __fastcall
#define THISCALL __thiscall
#else
#define FASTCALL
#define THISCALL
#endif

namespace addresses
{
    bool Initialize();

    inline void(FASTCALL* NetworkStateChanged)(int64_t chainEntity, int64_t offset, int64_t a3);
    inline void(FASTCALL* StateChanged)(void* networkTransmitComponent, CEntityInstance* ent, int64_t offset, int16_t a4, int16_t a5);
    inline void(FASTCALL* CBaseModelEntity_SetModel)(CBaseModelEntity* pModel, const char* szModel);
}// namespace addresses
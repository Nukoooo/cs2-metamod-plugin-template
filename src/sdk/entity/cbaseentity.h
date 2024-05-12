#pragma once

class CBaseEntity;

#include "../schema.hpp"
#include "ehandle.h"
#include "entityinstance.h"
#include "globaltypes.h"
#include "mathlib/vector.h"
#include "../../gameconfig.hpp"

class CGameSceneNode
{
  public:
    DECLARE_SCHEMA_CLASS(CGameSceneNode)

    SCHEMA_FIELD(CEntityInstance*, m_pOwner)

    SCHEMA_FIELD(CGameSceneNode*, m_pParent)

    SCHEMA_FIELD(CGameSceneNode*, m_pChild)

    SCHEMA_FIELD(CNetworkOriginCellCoordQuantizedVector, m_vecOrigin)

    SCHEMA_FIELD(QAngle, m_angRotation)

    SCHEMA_FIELD(float, m_flScale)

    SCHEMA_FIELD(float, m_flAbsScale)

    SCHEMA_FIELD(Vector, m_vecAbsOrigin)

    SCHEMA_FIELD(QAngle, m_angAbsRotation)

    SCHEMA_FIELD(Vector, m_vRenderOrigin)

    matrix3x4_t EntityToWorldTransform()
    {
        matrix3x4_t mat;

        // issues with this and im tired so hardcoded it
        //AngleMatrix(this->m_angAbsRotation(), this->m_vecAbsOrigin(), mat);

        QAngle angles = this->m_angAbsRotation();
        float sr, sp, sy, cr, cp, cy;
        SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
        SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
        SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);
        mat[0][0] = cp * cy;
        mat[1][0] = cp * sy;
        mat[2][0] = -sp;

        float crcy = cr * cy;
        float crsy = cr * sy;
        float srcy = sr * cy;
        float srsy = sr * sy;
        mat[0][1] = sp * srcy - crsy;
        mat[1][1] = sp * srsy + crcy;
        mat[2][1] = sr * cp;

        mat[0][2] = (sp * crcy + srsy);
        mat[1][2] = (sp * crsy - srcy);
        mat[2][2] = cr * cp;

        Vector pos = this->m_vecAbsOrigin();
        mat[0][3] = pos.x;
        mat[1][3] = pos.y;
        mat[2][3] = pos.z;

        return mat;
    }
};

class CBodyComponent
{
  public:
    DECLARE_SCHEMA_CLASS(CBodyComponent)

    SCHEMA_FIELD(CGameSceneNode*, m_pSceneNode)
};

class CEntitySubclassVDataBase
{
  public:
    DECLARE_SCHEMA_CLASS(CEntitySubclassVDataBase)
};

struct VPhysicsCollisionAttribute_t {
    DECLARE_SCHEMA_CLASS_INLINE(VPhysicsCollisionAttribute_t)

    SCHEMA_FIELD(uint8, m_nCollisionGroup)

    SCHEMA_FIELD(uint64_t, m_nInteractsAs)

    SCHEMA_FIELD(uint64_t, m_nInteractsWith)

    SCHEMA_FIELD(uint64_t, m_nInteractsExclude)
};

class CCollisionProperty
{
  public:
    DECLARE_SCHEMA_CLASS_INLINE(CCollisionProperty)

    SCHEMA_FIELD(VPhysicsCollisionAttribute_t, m_collisionAttribute)

    SCHEMA_FIELD(SolidType_t, m_nSolidType)

    SCHEMA_FIELD(uint8, m_usSolidFlags)

    SCHEMA_FIELD(uint8, m_CollisionGroup)
};

class Z_CBaseEntity : public CEntityInstance
{
  public:
    typedef Z_CBaseEntity ThisClass;
    static constexpr const char* ThisClassName = "CBaseEntity";
    static constexpr bool IsStruct = false;

    SCHEMA_FIELD(CBodyComponent*, m_CBodyComponent)

    SCHEMA_FIELD(CBitVec<64>, m_isSteadyState)

    SCHEMA_FIELD(float, m_lastNetworkChange)

    SCHEMA_FIELD_POINTER(CNetworkTransmitComponent, m_NetworkTransmitComponent)

    SCHEMA_FIELD(int, m_iHealth)

    SCHEMA_FIELD(int, m_iMaxHealth)

    SCHEMA_FIELD(int, m_iTeamNum)

    SCHEMA_FIELD(bool, m_bLagCompensate)

    SCHEMA_FIELD(Vector, m_vecAbsVelocity)

    SCHEMA_FIELD(Vector, m_vecBaseVelocity)

    SCHEMA_FIELD(CCollisionProperty*, m_pCollision)

    SCHEMA_FIELD(MoveCollide_t, m_MoveCollide)

    SCHEMA_FIELD(MoveType_t, m_MoveType)

    SCHEMA_FIELD(MoveType_t, m_nActualMoveType)

    SCHEMA_FIELD(CHandle<Z_CBaseEntity>, m_hEffectEntity)

    SCHEMA_FIELD(uint32, m_spawnflags)

    SCHEMA_FIELD(uint32, m_fFlags)

    SCHEMA_FIELD(LifeState_t, m_lifeState)

    SCHEMA_FIELD(float, m_flDamageAccumulator)

    SCHEMA_FIELD(bool, m_bTakesDamage)

    SCHEMA_FIELD_POINTER(CUtlStringToken, m_nSubclassID)

    SCHEMA_FIELD(float, m_flFriction)

    SCHEMA_FIELD(float, m_flGravityScale)

    SCHEMA_FIELD(float, m_flTimeScale)

    SCHEMA_FIELD(float, m_flSpeed);

    SCHEMA_FIELD(CUtlString, m_sUniqueHammerID);

    SCHEMA_FIELD(CUtlSymbolLarge, m_target);

    int entindex()
    {
        return m_pEntity->m_EHandle.GetEntryIndex();
    }

    Vector GetAbsOrigin()
    {
        return m_CBodyComponent->m_pSceneNode->m_vecAbsOrigin;
    }

    QAngle GetAbsRotation()
    {
        return m_CBodyComponent->m_pSceneNode->m_angAbsRotation;
    }

    void SetAbsOrigin(Vector vecOrigin)
    {
        m_CBodyComponent->m_pSceneNode->m_vecAbsOrigin = vecOrigin;
    }

    void SetAbsRotation(QAngle angAbsRotation)
    {
        m_CBodyComponent->m_pSceneNode->m_angAbsRotation = angAbsRotation;
    }

    void SetAbsVelocity(Vector vecVelocity)
    {
        m_vecAbsVelocity = vecVelocity;
    }

    void SetBaseVelocity(Vector vecVelocity)
    {
        m_vecBaseVelocity = vecVelocity;
    }

    bool IsPawn()
    {
        static int offset = g_pGameConfig->GetOffset("IsEntityPawn");
        return Memory::VirtualCall<bool>(this, offset);
    }

    bool IsController()
    {
        static int offset = g_pGameConfig->GetOffset("IsEntityController");
        return Memory::VirtualCall<bool>(this, offset);
    }

    bool IsAlive()
    {
        return m_lifeState == LifeState_t::LIFE_ALIVE;
    }

    CHandle<CBaseEntity> GetHandle() { return m_pEntity->m_EHandle; }

    // A double pointer to entity VData is available 4 bytes past m_nSubclassID, if applicable
    CEntitySubclassVDataBase* GetVData() { return *(CEntitySubclassVDataBase**)((uint8*)(m_nSubclassID()) + 4); }

    const char* GetName() const { return m_pEntity->m_name.String(); }
};

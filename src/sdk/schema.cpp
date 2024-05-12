#include "schema.hpp"
#include "../interfaces.hpp"
#include "../modules.hpp"
#include "entity/cbaseentity.h"
#include "spdlog/spdlog.h"
#include <schemasystem.h>

// #defien DUMP_SCHEMA

#ifdef DUMP_SCHEMA
#include <fstream>
#endif

using SchemaKeyValueMap_t = CUtlMap<uint32_t, SchemaKey>;
using SchemaTableMap_t = CUtlMap<uint32_t, SchemaKeyValueMap_t*>;

// std::unordered_map<std::uint64_t, std::int32_t> schema_offsets;

static bool IsFieldNetworked(SchemaClassFieldData_t& field)
{
    for (int i = 0; i < field.m_nStaticMetadataCount; i++)
    {
        if ("MNetworkEnable"_fnv1a32 == fnv1a32::hash(field.m_pStaticMetadata[i].m_pszName))
            return true;
    }

    return false;
}

static bool InitSchemaFieldsForClass(SchemaTableMap_t* tableMap, const char* className, uint32_t classKey)
{
    CSchemaSystemTypeScope* pType = g_pSchemaSystem->FindTypeScopeForModule(mods::server.ModuleName().data());

    if (!pType)
        return false;

    SchemaClassInfoData_t* pClassInfo = pType->FindDeclaredClass(className).Get();

    if (!pClassInfo)
    {
        auto* map = new SchemaKeyValueMap_t(0, 0, DefLessFunc(uint32_t));
        tableMap->Insert(classKey, map);

        Warning("InitSchemaFieldsForClass(): '%s' was not found!\n", className);
        return false;
    }

    short fieldsSize = pClassInfo->m_nFieldCount;
    SchemaClassFieldData_t* pFields = pClassInfo->m_pFields;

    SchemaKeyValueMap_t* keyValueMap = new SchemaKeyValueMap_t(0, 0, DefLessFunc(uint32_t));
    keyValueMap->EnsureCapacity(fieldsSize);
    tableMap->Insert(classKey, keyValueMap);

    for (int i = 0; i < fieldsSize; ++i)
    {
        SchemaClassFieldData_t& field = pFields[i];

#ifdef _DEBUG
        spdlog::info("{}::{} found at -> {:#x} - {:#x}", className, field.m_pszName, field.m_nSingleInheritanceOffset, (uintptr_t) (&field));
#endif

        keyValueMap->Insert(fnv1a32::hash(field.m_pszName), {field.m_nSingleInheritanceOffset, IsFieldNetworked(field)});
    }

    return true;
}

/*void schema::Dump()
{
    auto type_scope = interfaces::schemaSystem->FindTypeScopeForModule(mods::server.ModuleName().data());
    if (!type_scope)
        return;

    auto table_size = type_scope->m_DeclaredClasses.m_Map.Count();

#ifdef DUMP_SCHEMA
    std::ofstream dumped_schema("schema_server.txt");
#endif
    std::vector<UtlTSHashHandle_t> elements{ };
    elements.resize(table_size + 1);
    for (auto i = 0; i < table_size; i++)
    {
        auto element = type_scope->m_DeclaredClasses.m_Map.Element(i);
        if (element == nullptr)
            continue;

        auto field_count = element->m_pClassInfo->m_nFieldCount;
        auto name = element->m_pClassInfo->m_pszName;

        if (field_count == 0 || name == nullptr)
            continue;

#ifdef DUMP_SCHEMA
        dumped_schema << std::format("class {}:\n", name);
#endif

        for (auto j = 0; j < field_count; j++)
        {
            auto field = element->m_pClassInfo->m_pFields[j];
#ifdef DUMP_SCHEMA
            dumped_schema << std::format("    [{}] {} -> {:#x}\n", field.m_pType->m_sTypeName.Get(), field.m_pszName, field.m_nSingleInheritanceOffset);
#endif

            auto schema_name_hashed = fnv1a64::hash(std::format("{}->{}", name, name));
            schema_offsets[schema_name_hashed] = field.m_nSingleInheritanceOffset;
        }

#ifdef DUMP_SCHEMA
        dumped_schema << std::endl;
#endif
    }
}*/

SchemaKey schema::GetOffset(const char* className, uint32_t classKey, const char* memberName, uint32_t memberKey)
{
    static SchemaTableMap_t schemaTableMap(0, 0, DefLessFunc(uint32_t));
    int16_t tableMapIndex = schemaTableMap.Find(classKey);
    if (!schemaTableMap.IsValidIndex(tableMapIndex))
    {
        if (InitSchemaFieldsForClass(&schemaTableMap, className, classKey))
            return GetOffset(className, classKey, memberName, memberKey);

        return {0, false};
    }

    SchemaKeyValueMap_t* tableMap = schemaTableMap[tableMapIndex];
    int16_t memberIndex = tableMap->Find(memberKey);
    if (!tableMap->IsValidIndex(memberIndex))
    {
        Warning("schema::GetOffset(): '%s' was not found in '%s'!\n", memberName, className);
        return {0, false};
    }

    return tableMap->Element(memberIndex);
}

int16_t schema::FindChainOffset(const char* className)
{
    CSchemaSystemTypeScope* pType = g_pSchemaSystem->FindTypeScopeForModule(mods::server.ModuleName().data());

    if (!pType)
        return false;

    SchemaClassInfoData_t* pClassInfo = pType->FindDeclaredClass(className).Get();

    do
    {
        SchemaClassFieldData_t* pFields = pClassInfo->m_pFields;
        short fieldsSize = pClassInfo->m_nFieldCount;
        for (int i = 0; i < fieldsSize; ++i)
        {
            SchemaClassFieldData_t& field = pFields[i];

            if (V_strcmp(field.m_pszName, "__m_pChainEntity") == 0)
            {
                return field.m_nSingleInheritanceOffset;
            }
        }
    } while ((pClassInfo = pClassInfo->m_pBaseClasses ? pClassInfo->m_pBaseClasses->m_pClass : nullptr) != nullptr);

    return 0;
}

void SetStateChanged(Z_CBaseEntity* pEntity, int offset)
{
    addresses::StateChanged(pEntity->m_NetworkTransmitComponent(), pEntity, offset, -1, -1);

    pEntity->m_lastNetworkChange = gpGlobals->curtime;
    pEntity->m_isSteadyState().ClearAll();
}

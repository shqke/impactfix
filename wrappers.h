#ifndef _INCLUDE_WRAPPERS_H_
#define _INCLUDE_WRAPPERS_H_

#include "smsdk_ext.h"
#include <extensions/IBinTools.h>
#include <vphysics_interface.h>
#include <const.h>
#include <IEngineTrace.h>

extern IServerGameEnts* gameents;

class CDetour;

typedef bool (*ShouldHitFunc_t)(IHandleEntity* pHandleEntity, int contentsMask);

class CTraceFilterSimple :
	public CTraceFilter
{
public:
	static void* pfn_ctor;

	static ICallWrapper* call_ctor;

public:
	CTraceFilterSimple(const IHandleEntity* passedict, int collisionGroup, ShouldHitFunc_t pExtraShouldHitFunc = NULL)
	{
		m_pPassEnt = passedict;
		m_collisionGroup = collisionGroup;
		m_pExtraShouldHitCheckFunction = pExtraShouldHitFunc;

		// Call contrustor to replace vtable on this instance
		struct {
			const CTraceFilterSimple* pFilter;
			const IHandleEntity* passedict;
			int collisionGroup;
			ShouldHitFunc_t pExtraShouldHitFunc;
		} stack{ this, passedict, collisionGroup, pExtraShouldHitFunc };

		call_ctor->Execute(&stack, NULL);
	}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override
	{
		return false;
	}

private:
	const IHandleEntity* m_pPassEnt;
	int m_collisionGroup;
	ShouldHitFunc_t m_pExtraShouldHitCheckFunction;
};

class CBaseEntity :
	public IServerEntity
{
public:
	static int sendprop_movetype;

	static int dataprop_m_pPhysicsObject;
	static int dataprop_m_lifeState;

	static int vtblindex_WorldSpaceCenter;
	static int vtblindex_GetVectors;

	static ICallWrapper* vcall_WorldSpaceCenter;
	static ICallWrapper* vcall_GetVectors;

	edict_t* edict()
	{
		return gameents->BaseEntityToEdict(this);
	}

	int entindex()
	{
		return gamehelpers->EntityToBCompatRef(this);
	}

	const Vector& WorldSpaceCenter() const
	{
		struct {
			const CBaseEntity* pEntity;
		} stack{ this };

		Vector* ret;
		vcall_WorldSpaceCenter->Execute(&stack, &ret);

		return *ret;
	}

	void GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
	{
		struct {
			const CBaseEntity* pEntity;
			Vector* pForward;
			Vector* pRight;
			Vector* pUp;
		} stack{ this, pForward, pRight, pUp };

		vcall_GetVectors->Execute(&stack, NULL);
	}

	IPhysicsObject* VPhysicsGetObject()
	{
		if (CBaseEntity::dataprop_m_pPhysicsObject == 0) {
			datamap_t* pDataMap = gamehelpers->GetDataMap(const_cast<CBaseEntity*>(this));
			if (pDataMap == NULL) {
				return NULL;
			}

			sm_datatable_info_t info;
			if (!gamehelpers->FindDataMapInfo(pDataMap, "m_pPhysicsObject", &info)) {
				return NULL;
			}

			CBaseEntity::dataprop_m_pPhysicsObject = info.actual_offset;
		}

		return *(IPhysicsObject**)((byte*)(this) + CBaseEntity::dataprop_m_pPhysicsObject);
	}

	MoveType_t GetMoveType()
	{
		return (MoveType_t)*(char*)((byte*)(this) + CBaseEntity::sendprop_movetype);
	}

	const char* GetClassname()
	{
		return gamehelpers->GetEntityClassname(this);
	}

	char& m_lifeState()
	{
		static char cMissing = LIFE_DEAD;

		if (CBaseEntity::dataprop_m_lifeState == 0) {
			datamap_t* pDataMap = gamehelpers->GetDataMap(const_cast<CBaseEntity*>(this));
			if (pDataMap == NULL) {
				return cMissing;
			}

			sm_datatable_info_t info;
			if (!gamehelpers->FindDataMapInfo(pDataMap, "m_lifeState", &info)) {
				return cMissing;
			}

			CBaseEntity::dataprop_m_lifeState = info.actual_offset;
		}

		return *(char*)((byte*)(this) + CBaseEntity::dataprop_m_lifeState);
	}

	bool IsAlive()
	{
		return m_lifeState() == LIFE_ALIVE;
	}
};

class CBasePlayer :
	public CBaseEntity
{
public:
	static int sendprop_m_fFlags;

	static int vtblindex_PlayerSolidMask;

	static ICallWrapper* vcall_PlayerSolidMask;

	int GetFlags()
	{
		return *(int*)((byte*)(this) + CBasePlayer::sendprop_m_fFlags);
	}

	unsigned int PlayerSolidMask(bool brushOnly = false)
	{
		struct {
			const CBasePlayer* pPlayer;
			int brushOnly;
		} stack{ this, brushOnly };

		unsigned int ret;
		vcall_PlayerSolidMask->Execute(&stack, &ret);

		return ret;
	}
};

class CTerrorPlayer :
	public CBasePlayer
{
public:
	static int sendprop_m_carryVictim;

	CTerrorPlayer* GetCarryVictim()
	{
		edict_t* pEdict = gamehelpers->GetHandleEntity(*(CBaseHandle*)((byte*)(this) + CTerrorPlayer::sendprop_m_carryVictim));
		if (pEdict == NULL) {
			return NULL;
		}

		// Make sure it's a player
		if (engine->GetPlayerUserId(pEdict) == -1) {
			return NULL;
		}

		return (CTerrorPlayer*)gameents->EdictToBaseEntity(pEdict);
	}
};

class CCharge
{
public:
	static int sendprop_m_owner;

	static void* pfn_DoImpactProbe;

	static CDetour* detour_DoImpactProbe;

	CBasePlayer* GetOwner()
	{
		edict_t* pEdict = gamehelpers->GetHandleEntity(*(CBaseHandle*)((byte*)(this) + CCharge::sendprop_m_owner));
		if (pEdict == NULL) {
			return NULL;
		}

		// Make sure it's a player
		if (engine->GetPlayerUserId(pEdict) == -1) {
			return NULL;
		}

		return (CBasePlayer*)gameents->EdictToBaseEntity(pEdict);
	}
};

#endif // _INCLUDE_WRAPPERS_H_

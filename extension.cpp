#include "extension.h"
#include <CDetour/detours.h>

CImpactFix g_ImpactFix;
SMEXT_LINK(&g_ImpactFix);

SH_DECL_HOOK0(IPhysicsObject, GetMass, const, 0, float);

IBinTools* bintools = NULL;
IServerGameEnts* gameents = NULL;
IEngineTrace* enginetrace = NULL;

void* CTraceFilterSimple::pfn_ctor = NULL;
ICallWrapper* CTraceFilterSimple::call_ctor = NULL;
int CBaseEntity::vtblindex_WorldSpaceCenter = 0;
ICallWrapper* CBaseEntity::vcall_WorldSpaceCenter = NULL;
int CBaseEntity::vtblindex_GetVectors = 0;
ICallWrapper* CBaseEntity::vcall_GetVectors = NULL;
int CBaseEntity::dataprop_m_pPhysicsObject = 0;
int CBaseEntity::dataprop_m_lifeState = 0;
int CBaseEntity::sendprop_movetype = 0;
void* CCharge::pfn_DoImpactProbe = NULL;
CDetour* CCharge::detour_DoImpactProbe = NULL;
int CCharge::sendprop_m_owner = 0;
int CBasePlayer::sendprop_m_fFlags = 0;
int CTerrorPlayer::sendprop_m_carryVictim = 0;
int CBasePlayer::vtblindex_PlayerSolidMask = 0;
ICallWrapper* CBasePlayer::vcall_PlayerSolidMask = NULL;

void UTIL_TraceHull(const Vector& vecAbsStart, const Vector& vecAbsEnd, const Vector& hullMin, const Vector& hullMax, unsigned int mask, const IHandleEntity* ignore, int collisionGroup, trace_t* ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd, hullMin, hullMax);
	CTraceFilterSimple traceFilter(ignore, collisionGroup);

	enginetrace->TraceRay(ray, mask, &traceFilter, ptr);
}

/*
Original game source code would look something like this:
void CCharge::DoImpactProbe()
{
	CTerrorPlayer *pCharger = m_owner.Get();
	if ( !pCharger )
	{
		return;
	}

	if ( !( pCharger->GetFlags() & FL_ONGROUND ) )
	{
		return;
	}

	const Vector &vecStart = pCharger->WorldSpaceCenter();

	Vector vecForward;
	pCharger->GetVectors( &vecForward, NULL, NULL );

	float flProbeDistance = z_charger_probe_alone.GetFloat();
	unsigned int mask = pCharger->PlayerSolidMask();

	if ( pCharger->IsCarryingSomeone() )
	{
		mask |= pCharger->m_carryVictim.Get()->PlayerSolidMask();
		flProbeDistance = z_charger_probe_attack.GetFloat();
	}

	trace_t	tr;
	UTIL_TraceHull( vecStart, vecStart + vecForward * flProbeDistance, Vector( -15, -15, -12 ), Vector( 15, 15, 24 ), mask, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
	if ( tr.m_pEnt )
	{
		IPhysicsObject *pObject = tr.m_pEnt->VPhysicsGetObject();
		if ( pObject && pCharger->IsCarryingSomeone() && FClassnameIs(tr.m_pEnt, "prop_car_alarm") )
		{
			// CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, ... );
			tr.m_pEnt->TakeDamage( CTakeDamageInfo( pCharger->m_carryVictim.Get(), pCharger->m_carryVictim.Get(), pCharger->GetAbsVelocity(), tr.m_pEnt->WorldSpaceCenter(), 1.0, DMG_CLUB ) );

			return;
		}

		if ( pObject && pObject->GetMass() < 250.0 )
		{
			// CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, ... );
			tr.m_pEnt->TakeDamage( CTakeDamageInfo( pCharger, pCharger, pCharger->GetAbsVelocity(), tr.m_pEnt->WorldSpaceCenter(), tr.m_pEnt->GetHealth() + 1, DMG_CLUB ) );

			return;
		}
	}

	if ( tr.fraction < 1.0 && !( tr.m_pEnt && tr.m_pEnt->IsPlayer() ) )
	{
		if ( pCharger->IsCarryingSomeone() )
		{
			pCharger->OnSlammedSurvivor( pCharger->m_carryVictim.Get(), true, m_vecCarryStartPos.z - pCharger->GetAbsOrigin().z > 360.0 );

			return;
		}

		if ( fabs( tr.plane.normal.Dot( forward ) ) <= z_charge_impact_angle.GetFloat() )
		{
			return;
		}

		if ( !DidLastChargeHitAnySurvivors() )
		{
			ImpactStagger();
		}

		pCharger->EmitSound( "ChargerZombie.ImpactHard" );
		DispatchParticleEffect( "charger_wall_impact", pCharger->WorldSpaceCenter(), vec3_angle );

		EndCharge();
		ImpactRumble();
	}
}
*/
DETOUR_DECL_MEMBER0(Handler_CCharge_DoImpactProbe, void)
{
	int hookId = 0;
	CCharge* _this = (CCharge*)this;

	CTerrorPlayer* pCharger = (CTerrorPlayer*)_this->GetOwner();
	if (pCharger == NULL) {
		return;
	}

	if (~pCharger->GetFlags() & FL_ONGROUND) {
		return;
	}

	const Vector& vecStart = pCharger->WorldSpaceCenter();

	Vector vecForward;
	pCharger->GetVectors(&vecForward, NULL, NULL);

	static ConVarRef z_charger_probe_alone("z_charger_probe_alone");
	float flProbeDistance = z_charger_probe_alone.GetFloat();
	unsigned int mask = pCharger->PlayerSolidMask();

	CBasePlayer* pCarryVictim = pCharger->GetCarryVictim();
	if (pCarryVictim != NULL) {
		static ConVarRef z_charger_probe_attack("z_charger_probe_attack");
		flProbeDistance = z_charger_probe_attack.GetFloat();

		mask |= pCarryVictim->PlayerSolidMask();
	}

	/*
	c2m2 custom immovable prop dynamic
	Hit: 1706 (prop_dynamic)
        Movetype: 7
        Moveable: false
        Model: models/props_misc/fairground_tent_closed.mdl

	c1m1 death charge glass
	Hit: 233 (prop_physics)
		Movetype: 6
		Moveable: false
		Model: models/props_windows/hotel_window_glass001.mdl

	c10m4 breakable window
	Hit: 375 (prop_physics)
		Movetype: 6
		Moveable: false
		Model: models/props_windows/window_industrial.mdl

	c10m4 breakable wooden door near bus
	Hit: 283 (func_breakable)
		Movetype: 7
		Moveable: false
		Model: *75

	c1m4 unbreakable window by charger
	Hit: 160 (func_breakable)
		Movetype: 7
		Moveable: false
		Model: *92

	c2m2 hittables
	Hit: 840 (prop_physics)
		Movetype: 6
		Moveable: true
		Model: models/props_street/trashbin01.mdl

	c10m5 moveable physics prop but not affected by damage
	Hit: 261 (prop_physics)
		Movetype: 6
		Alive: true
		Moveable: true
		Motion Enabled: true
		Asleep: true
		Model: models/props_interiors/table_picnic.mdl
	*/

	trace_t	tr;
	UTIL_TraceHull(vecStart, vecStart + vecForward * flProbeDistance, Vector(-15, -15, -12), Vector(15, 15, 24), mask, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);
	if (tr.m_pEnt != NULL) {
		IPhysicsObject* pIPhysicsObject = tr.m_pEnt->VPhysicsGetObject();
		if (pIPhysicsObject != NULL) {
			bool bDoDamageToACarAlarmCondition = pCarryVictim != NULL && strcmp(tr.m_pEnt->GetClassname(), "prop_car_alarm") == 0;
			bool bDoDamageToALowMassPropCondition = !bDoDamageToACarAlarmCondition && pIPhysicsObject->GetMass() < 250.0;
			bool bIsNotMoveable = tr.m_pEnt->GetMoveType() != MOVETYPE_VPHYSICS || !pIPhysicsObject->IsMoveable();

			if (bDoDamageToALowMassPropCondition) {
				// Call original second time - it will damage a prop (low mass prop condition is going to be hit and return early)
				DETOUR_MEMBER_CALL(Handler_CCharge_DoImpactProbe)();

				// https://github.com/ValveSoftware/source-sdk-2013/blob/f56bb35301836e56582a575a75864392a0177875/mp/src/game/server/physics.cpp#L2181
				if (tr.m_pEnt->IsAlive() && (bIsNotMoveable || pIPhysicsObject->IsAsleep())) {
					// If prop still exists - do impact (break condition by overriding mass on its physics object >= 250.0)
					// Prop must be immovable or still asleep after receiving damage
					hookId = SH_ADD_HOOK(IPhysicsObject, GetMass, pIPhysicsObject, SH_MEMBER(&g_ImpactFix, &CImpactFix::Handler_IPhysicsObject_GetMass), false);
				}

				//DevMsg
				//(
				//	"Hit: %d (%s)\n"
				//	"	Movetype: %d\n"
				//	"	Moveable: %s\n"
				//	"	Asleep: %s\n"
				//	"	IsAlive: %s\n"
				//	"	Model: %s\n",
				//	tr.m_pEnt->entindex(),
				//	tr.m_pEnt->GetClassname(),
				//	tr.m_pEnt->GetMoveType(),
				//	pIPhysicsObject->IsMoveable() ? "true" : "false",
				//	pIPhysicsObject->IsAsleep() ? "true" : "false",
				//	tr.m_pEnt->IsAlive() ? "true" : "false",
				//	tr.m_pEnt->GetModelName()
				//);
			}
		}
	}

	DETOUR_MEMBER_CALL(Handler_CCharge_DoImpactProbe)();
	
	SH_REMOVE_HOOK_ID(hookId);
}

float CImpactFix::Handler_IPhysicsObject_GetMass() const
{
	RETURN_META_VALUE(MRES_SUPERCEDE, 250.0);
}

bool CImpactFix::SetupFromGameConfig(IGameConfig* gc, char* error, int maxlength)
{
	static const struct {
		const char* key;
		void*& address;
	} s_sigs[] = {
		{ "CCharge::DoImpactProbe", CCharge::pfn_DoImpactProbe },
		{ "CTraceFilterSimple::CTraceFilterSimple", CTraceFilterSimple::pfn_ctor },
	};

	for (auto&& el : s_sigs) {
		if (!gc->GetMemSig(el.key, &el.address)) {
			ke::SafeSprintf(error, maxlength, "Unable to find signature for \"%s\" from game config (file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}

		if (el.address == NULL) {
			ke::SafeSprintf(error, maxlength, "Sigscan for \"%s\" failed (game config file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}
	}

	static const struct {
		const char* key;
		int& offset;
	} s_offsets[] = {
		{ "CBaseEntity::WorldSpaceCenter", CBaseEntity::vtblindex_WorldSpaceCenter },
		{ "CBaseEntity::GetVectors", CBaseEntity::vtblindex_GetVectors },
		{ "CBasePlayer::PlayerSolidMask", CBasePlayer::vtblindex_PlayerSolidMask },
	};

	for (auto&& el : s_offsets) {
		if (!gc->GetOffset(el.key, &el.offset)) {
			ke::SafeSprintf(error, maxlength, "Unable to get offset for \"%s\" from game config (file: \"" GAMEDATA_FILE ".txt\")", el.key);

			return false;
		}
	}

	return true;
}

bool CImpactFix::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	sm_sendprop_info_t info;
	if (!gamehelpers->FindSendPropInfo("CBaseEntity", "movetype", &info)) {
		ke::SafeStrcpy(error, maxlength, "Unable to find SendProp \"CBaseEntity::movetype\"");

		return false;
	}

	CBaseEntity::sendprop_movetype = info.actual_offset;

	if (!gamehelpers->FindSendPropInfo("CCharge", "m_owner", &info)) {
		ke::SafeStrcpy(error, maxlength, "Unable to find SendProp \"CCharge::m_owner\"");

		return false;
	}

	CCharge::sendprop_m_owner = info.actual_offset;

	if (!gamehelpers->FindSendPropInfo("CBasePlayer", "m_fFlags", &info)) {
		ke::SafeStrcpy(error, maxlength, "Unable to find SendProp \"CBasePlayer::m_fFlags\"");

		return false;
	}

	CBasePlayer::sendprop_m_fFlags = info.actual_offset;

	if (!gamehelpers->FindSendPropInfo("CTerrorPlayer", "m_carryVictim", &info)) {
		ke::SafeStrcpy(error, maxlength, "Unable to find SendProp \"CTerrorPlayer::m_carryVictim\"");

		return false;
	}

	CTerrorPlayer::sendprop_m_carryVictim = info.actual_offset;

	IGameConfig* gc = NULL;
	if (!gameconfs->LoadGameConfigFile(GAMEDATA_FILE, &gc, error, maxlength)) {
		return false;
	}

	if (!SetupFromGameConfig(gc, error, maxlength)) {
		gameconfs->CloseGameConfigFile(gc);

		return false;
	}

	gameconfs->CloseGameConfigFile(gc);

	// Game config is never used by detour class to handle errors ourselves
	CDetourManager::Init(smutils->GetScriptingEngine(), NULL);

	CCharge::detour_DoImpactProbe = DETOUR_CREATE_MEMBER(Handler_CCharge_DoImpactProbe, CCharge::pfn_DoImpactProbe);
	if (CCharge::detour_DoImpactProbe == NULL) {
		ke::SafeStrcpy(error, maxlength, "Unable to create a detour for \"CCharge::DoImpactProbe\"");

		return false;
	}

	sharesys->AddDependency(myself, "bintools.ext", true, true);

	return true;
}

void CImpactFix::SDK_OnUnload()
{
	if (CTraceFilterSimple::call_ctor != NULL) {
		CTraceFilterSimple::call_ctor->Destroy();
		CTraceFilterSimple::call_ctor = NULL;
	}

	if (CBaseEntity::vcall_WorldSpaceCenter != NULL) {
		CBaseEntity::vcall_WorldSpaceCenter->Destroy();
		CBaseEntity::vcall_WorldSpaceCenter = NULL;
	}

	if (CBaseEntity::vcall_GetVectors != NULL) {
		CBaseEntity::vcall_GetVectors->Destroy();
		CBaseEntity::vcall_GetVectors = NULL;
	}

	if (CBasePlayer::vcall_PlayerSolidMask != NULL) {
		CBasePlayer::vcall_PlayerSolidMask->Destroy();
		CBasePlayer::vcall_PlayerSolidMask = NULL;
	}

	if (CCharge::detour_DoImpactProbe != NULL) {
		CCharge::detour_DoImpactProbe->Destroy();
		CCharge::detour_DoImpactProbe = NULL;
	}
}

void CImpactFix::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(BINTOOLS, bintools);
	if (bintools == NULL) {
		return;
	}

	SourceMod::PassInfo params[] = {
#if SMINTERFACE_BINTOOLS_VERSION == 4
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(IHandleEntity*), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(int), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(ShouldHitFunc_t), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYREF, sizeof(Vector*), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(Vector*), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(Vector*), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(Vector*), NULL, 0 },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(int), NULL, 0 },
#else
		// sm1.9- support
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(IHandleEntity*) },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(int) },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(ShouldHitFunc_t) },
		{ PassType_Basic, PASSFLAG_BYREF, sizeof(Vector*) },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(Vector*) },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(Vector*) },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(Vector*) },
		{ PassType_Basic, PASSFLAG_BYVAL, sizeof(int) },
#endif
	};

	CTraceFilterSimple::call_ctor = bintools->CreateCall(CTraceFilterSimple::pfn_ctor, CallConv_ThisCall, NULL, &params[0], 3);
	if (CTraceFilterSimple::call_ctor == NULL) {
		smutils->LogError(myself, "Unable to create ICallWrapper for \"CTraceFilterSimple::CTraceFilterSimple\"!");

		return;
	}

	CBaseEntity::vcall_WorldSpaceCenter = bintools->CreateVCall(CBaseEntity::vtblindex_WorldSpaceCenter, 0, 0, &params[3], NULL, 0);
	if (CBaseEntity::vcall_WorldSpaceCenter == NULL) {
		smutils->LogError(myself, "Unable to create ICallWrapper for \"CBaseEntity::WorldSpaceCenter\"!");

		return;
	}

	CBaseEntity::vcall_GetVectors = bintools->CreateVCall(CBaseEntity::vtblindex_GetVectors, 0, 0, NULL, &params[4], 3);
	if (CBaseEntity::vcall_GetVectors == NULL) {
		smutils->LogError(myself, "Unable to create ICallWrapper for \"CBaseEntity::GetVectors\"!");

		return;
	}

	CBasePlayer::vcall_PlayerSolidMask = bintools->CreateVCall(CBasePlayer::vtblindex_PlayerSolidMask, 0, 0, &params[7], &params[7], 1);
	if (CBasePlayer::vcall_PlayerSolidMask == NULL) {
		smutils->LogError(myself, "Unable to create ICallWrapper for \"CBasePlayer::PlayerSolidMask\"!");

		return;
	}

	CCharge::detour_DoImpactProbe->EnableDetour();
}

bool CImpactFix::QueryInterfaceDrop(SMInterface* pInterface)
{
	return pInterface != bintools;
}

void CImpactFix::NotifyInterfaceDrop(SMInterface* pInterface)
{
	SDK_OnUnload();
}

bool CImpactFix::QueryRunning(char* error, size_t maxlength)
{
	SM_CHECK_IFACE(BINTOOLS, bintools);

	return true;
}

bool CImpactFix::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetServerFactory, gameents, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);

	// For ConVarRef
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	return true;
}

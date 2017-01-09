#include "bot_common.h"

// Face the entity and "use" it
// NOTE: This state assumes we are standing in range of the entity to be used, with no obstructions.

void UseEntityState::OnEnter(CHLBot *me)
{
	;
}

void UseEntityState::OnUpdate(CHLBot *me)
{
	// in the very rare situation where two or more bots "used" a hostage at the same time,
	// one bot will fail and needs to time out of this state
	const float useTimeout = 5.0f;
	if (me->GetStateTimestamp() - gpGlobals->time > useTimeout)
	{
		me->Idle();
		return;
	}

	// look at the entity
	Vector pos = m_entity->pev->origin + Vector(0, 0, HumanHeight * 0.5f);
	me->SetLookAt("Use entity", &pos, PRIORITY_HIGH);

	// if we are looking at the entity, "use" it and exit
/// todo
//	if (me->IsLookingAtPosition(&pos))
	{
		me->UseEnvironment();
		me->Idle();
	}
}

void UseEntityState::OnExit(CHLBot *me)
{
	me->ClearLookAt();
	me->ResetStuckMonitor();
}

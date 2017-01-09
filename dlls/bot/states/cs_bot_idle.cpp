#include "bot_common.h"

// range for snipers to select a hiding spot
const float sniperHideRange = 2000.0f;

// The Idle state.
// We never stay in the Idle state - it is a "home base" for the state machine that
// does various checks to determine what we should do next.

void IdleState::OnEnter(CCSBot *me)
{
	me->DestroyPath();
	me->SetEnemy(NULL);

	// lurking death
	if (me->IsUsingKnife() && me->IsWellPastSafe() && !me->IsHurrying())
		me->Walk();

	// Since Idle assigns tasks, we assume that coming back to Idle means our task is complete
	me->SetTask(CCSBot::SEEK_AND_DESTROY);
	me->SetDisposition(CCSBot::ENGAGE_AND_INVESTIGATE);
}

void IdleState::OnUpdate(CCSBot *me)
{
	// all other states assume GetLastKnownArea() is valid, ensure that it is
	if (me->GetLastKnownArea() == NULL && me->StayOnNavMesh() == false)
		return;

	// zombies never leave the Idle state
	if (cv_bot_zombie.value > 0.0f)
	{
		me->ResetStuckMonitor();
		return;
	}

	// if we are in the early "safe" time, grab a knife or grenade
	if (me->IsSafe())
	{
		// if we have a grenade, use it
		if (!me->EquipGrenade())
		{
			// high-skill bots run with the knife
			if (me->GetProfile()->GetSkill() > 0.33f)
			{
				me->EquipKnife();
			}
		}
	}

	CCSBotManager *ctrl = TheCSBots();

	// if round is over, hunt
	if (me->GetGameState()->IsRoundOver())
	{
		me->Hunt();
		return;
	}

	const float defenseSniperCampChance = 75.0f;
	const float offenseSniperCampChance = 10.0f;

	// if we were following someone, continue following them
	if (me->IsFollowing())
	{
		me->ContinueFollowing();
		return;
	}
	// deathmatch
	// sniping check
	if (me->GetFriendsRemaining() && me->IsSniper() && RANDOM_FLOAT(0, 100.0f) < offenseSniperCampChance)
	{
		me->SetTask(CCSBot::MOVE_TO_SNIPER_SPOT);
		me->Hide(me->GetLastKnownArea(), RANDOM_FLOAT(10.0f, 30.0f), sniperHideRange);
		me->SetDisposition(CCSBot::OPPORTUNITY_FIRE);
		me->PrintIfWatched("Sniping!\n");
		return;
	}

	// if we have nothing special to do, go hunting for enemies
	me->Hunt();
}

#include "bot_common.h"

// Move to a potentially far away position.

void MoveToState::OnEnter(CHLBot *me)
{
	if (me->IsUsingKnife() && me->IsWellPastSafe() && !me->IsHurrying())
	{
		me->Walk();
	}
	else
	{
		me->Run();
	}

	// if we need to find the bomb, get there as quick as we can
	RouteType route;
	switch (me->GetTask())
	{
	// maybe also fastest way to healthkits??
	case CHLBot::MOVE_TO_LAST_KNOWN_ENEMY_POSITION:
			route = FASTEST_ROUTE;
			break;

		default:
			route = SAFEST_ROUTE;
			break;
	}

	// build path to, or nearly to, goal position
	me->ComputePath(TheNavAreaGrid.GetNavArea(&m_goalPosition), &m_goalPosition, route);

	m_radioedPlan = false;
	m_askedForCover = false;
}

// Move to a potentially far away position.

void MoveToState::OnUpdate(CHLBot *me)
{
	CHLBotManager *ctrl = TheCSBots();

	// assume that we are paying attention and close enough to know our enemy died
	if (me->GetTask() == CHLBot::MOVE_TO_LAST_KNOWN_ENEMY_POSITION)
	{
		// TODO: Account for reaction time so we take some time to realized the enemy is dead
		CBasePlayer *victim = static_cast<CBasePlayer *>(me->GetTaskEntity());
		if (victim == NULL || !victim->IsAlive())
		{
			me->PrintIfWatched("The enemy I was chasing was killed - giving up.\n");
			me->Idle();
			return;
		}
	}

	// look around
	me->UpdateLookAround();

	if (me->UpdatePathMovement() != CHLBot::PROGRESSING)
	{
		// reached destination
		switch (me->GetTask())
		{
			case CHLBot::MOVE_TO_LAST_KNOWN_ENEMY_POSITION:
			{
				CBasePlayer *victim = static_cast<CBasePlayer *>(me->GetTaskEntity());
				if (victim != NULL && victim->IsAlive())
				{
					// if we got here and haven't re-acquired the enemy, we lost him
					me->GetChatter()->Say("LostEnemy");
				}
				break;
			}
		}

		// default behavior when destination is reached
		me->Idle();
		return;
	}
}

void MoveToState::OnExit(CHLBot *me)
{
	// reset to run in case we were walking near our goal position
	me->Run();
	me->SetDisposition(CHLBot::ENGAGE_AND_INVESTIGATE);
	//me->StopAiming();
}

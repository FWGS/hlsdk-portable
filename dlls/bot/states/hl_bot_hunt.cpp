#include "bot_common.h"

// Begin the hunt

void HuntState::OnEnter(CHLBot *me)
{
	// lurking death
	if (me->IsUsingKnife() && me->IsWellPastSafe() && !me->IsHurrying())
		me->Walk();
	else
		me->Run();

	me->StandUp();
	me->SetDisposition(CHLBot::ENGAGE_AND_INVESTIGATE);
	me->SetTask(CHLBot::SEEK_AND_DESTROY);
	me->DestroyPath();
}

// Hunt down our enemies

void HuntState::OnUpdate(CHLBot *me)
{
	// listen for enemy noises
	if (me->ShouldInvestigateNoise())
	{
		me->InvestigateNoise();
		return;
	}

	// look around
	me->UpdateLookAround();

	// if we have reached our destination area, pick a new one
	// if our path fails, pick a new one
	if (me->GetLastKnownArea() == m_huntArea || me->UpdatePathMovement() != CHLBot::PROGRESSING)
	{
		m_huntArea = NULL;
		float oldest = 0.0f;

		int areaCount = 0;
		const float minSize = 150.0f;

		FOR_EACH_LL (TheNavAreaList, it)
		{
			CNavArea *area = TheNavAreaList[it];

			++areaCount;

			// skip the small areas
			const Extent *extent = area->GetExtent();
			if (extent->hi.x - extent->lo.x < minSize || extent->hi.y - extent->lo.y < minSize)
				continue;

			// keep track of the least recently cleared area
			float age = gpGlobals->time - area->GetClearedTimestamp(me->m_iTeam - 1);
			if (age > oldest)
			{
				oldest = age;
				m_huntArea = area;
			}
		}

		// if all the areas were too small, pick one at random
		int which = RANDOM_LONG(0, areaCount - 1);

		areaCount = 0;
		FOR_EACH_LL (TheNavAreaList, it2)
		{
			m_huntArea = TheNavAreaList[it2];

			if (which == areaCount)
				break;

			--which;
		}

		if (m_huntArea != NULL)
		{
			// create a new path to a far away area of the map
			me->ComputePath(m_huntArea, NULL, SAFEST_ROUTE);
		}
	}
}

// Done hunting

void HuntState::OnExit(CHLBot *me)
{
	;
}

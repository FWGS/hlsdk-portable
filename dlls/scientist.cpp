/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// human scientist (passive lab worker)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"weapons.h"

#define NUM_SCIENTIST_HEADS		10 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3, HEAD_LUKA = 4, HEAD_VORT = 5, HEAD_GLASSES2 = 6, HEAD_EINSTEIN2 = 7, HEAD_LUTHER2 = 8, HEAD_SLICK2 = 9 };

#define VENDOR_TYPE_WEP	0
#define VENDOR_TYPE_CLOTHES 1
#define VENDOR_TYPE_FOOD 2
#define VENDOR_TYPE_TRINKET 4
#define VENDOR_TYPE_WFOOD 5
#define MAX_VENDOR_WEPS 7
#define MAX_VENDOR_CLOTHES 3
#define MAX_VENDOR_FOOD 3
#define MAX_VENDOR_TRINKETS 6

//WEAPONS
const char* VendorItems[MAX_VENDOR_WEPS] = {
"IKEA .357 - Price: 50$", 
"Clawgun - Price: 35$", 
"Sawn-off Shotgun - Price: 50$",
"Chainsaw Launcher - Price: 100$",
"Railgun - Price: 50$",
"Magic Wand - Price : 80$",
"Rune Scimitar - Price : 100$"
};

char* VendorItemNames[MAX_VENDOR_WEPS] = {
	"weapon_357",
	"weapon_clawgun",
	"weapon_sawnoff",
	"weapon_rpg",
	"weapon_gauss",
	"weapon_hornetgun",
	"weapon_scimitar"
};

int VendorPrices[MAX_VENDOR_WEPS] =
{
	50,35,50,100,50,80,100
};

//CLOTHES
const char* VendorClothes[MAX_VENDOR_CLOTHES] = {
	"Battery - Price: 2$",
	"Full HEV Charge - Price: 10$",
	"Scorpion Jacket - Price: 15$"
};

char* VendorClothesNames[MAX_VENDOR_CLOTHES] = {
	"item_battery",
	"item_fullbattery",
	"item_scorpionjacket"
};

int VendorClothesPrices[MAX_VENDOR_CLOTHES] =
{
	2, 10, 15
};

//FOOD
const char* VendorFood[MAX_VENDOR_FOOD] = {
	"Pizza - Price: 3$",
	"Pizza 2: Electric Boogaloo - Price: 5$",
	"Pizza 3: The Reckoning - Price: 9$"
};

char* VendorFoodNames[MAX_VENDOR_FOOD] = {
	"item_pizza",
	"item_midpizza",
	"item_bigpizza"
};

int VendorFoodsPrices[MAX_VENDOR_FOOD] =
{
	3, 5, 9
};

//TRINKETS
const char* VendorTrinkets[MAX_VENDOR_TRINKETS] = {
	"Bepis - Price: 4$",
	"Tripmine - Price: 2$",
	"DO YOU SUCK DICKS? - Price: 3$",
	"UPS Mail - Price: 2$",
	"Banana - Price: 2$",
	"Wrench - Price: 60$"
};

char* VendorTrinketNames[MAX_VENDOR_TRINKETS] = {
	"weapon_handgrenade",
	"weapon_tripmine",
	"weapon_snark",
	"weapon_satchel",
	"weapon_banana",
	"weapon_wrench"
};

int VendorTrinketPrices[MAX_VENDOR_TRINKETS] =
{
	4, 2, 3, 2, 2, 60
};

//WAGNER FOOD
const char* VendorWFood[MAX_VENDOR_FOOD] = {
	"Pizza - Price: 1$",
	"Pizza 2: Electric Boogaloo - Price: 3$",
	"Pizza 3: The Reckoning - Price: 5$"
};

char* VendorWFoodNames[MAX_VENDOR_FOOD] = {
	"item_pizza",
	"item_midpizza",
	"item_bigpizza"
};

int VendorWFoodsPrices[MAX_VENDOR_FOOD] =
{
	1, 3, 5
};

static cvar_t *g_psv_override_scientist_mdl;

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL	( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )
#define		SCIENTIST_SUDOKU		( 420 )
#define		SCIENTIST_DESPACITO		( 69 )
#define		SCIENTIST_SHOCKED		( 74 )
#define		SCIENTIST_EXPLODE		( 75 )
#define		SCIENTIST_RAGE			( 101 )
#define		SCIENTIST_RAGEHIT		( 102 )
#define		SCIENTIST_STOPRAGE		( 103 )
#define		SCIENTIST_GRAB			( 53  )
#define		SCIENIST_DEATHSHOT		( 54  )
#define		FATSCIENTIST_WATERCANON		( 105 )
#define		FATSCIENTIST_WATERCANOFF		( 106 )
#define		FATSCIENTIST_CUPON		( 107 )
#define		FATSCIENTIST_CUPOFF		( 108 )

//=======================================================
// Scientist
//=======================================================
class CScientist : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void SetYawSpeed( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int ObjectCaps( void ) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int FriendNumber( int arrayNumber );
	void SetActivity( Activity newActivity );
	Activity GetStoppedActivity( void );
	int ISoundMask( void );
	void DeclineFollowing( void );
	void SwitchVendorItem( int ItemType );
	int FIdleSpeak(void);
	void EXPORT CommitSudoku(void);
	void Sudoku(void);
	bool Suicidal = false;
	bool BeingShocked;

	int GetPaid(int amount);
	void KeyValue(KeyValueData *pkvd);
	int m_vendorType;

	float CoverRadius( void ) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || ( gpGlobals->time - m_fearTime ) > 15; }

	BOOL CanHeal( void );
	void Heal( void );
	void Grab(void);
	void Scream( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type );
	Schedule_t *GetSchedule( void );
	MONSTERSTATE GetIdealState( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES

private:	
	const char *GetScientistModel( void );

	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS( monster_scientist, CScientist )

TYPEDESCRIPTION	CScientist::m_SaveData[] =
{
	DEFINE_FIELD( CScientist, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_fearTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_vendorType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE( CScientist, CTalkMonster )

void CScientist::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "vendortype"))
	{
		m_vendorType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}


int CScientist::FIdleSpeak(void)
{
	// try to start a conversation, or make statement
	int pitch;

	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	//Lowered it so that they would be constantly shittalking
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.8, 2.2);

	pitch = GetVoicePitch();

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
	CBaseEntity *pentFriend = FindNearestFriend(FALSE);

	//if (pentFriend && RANDOM_LONG(0,1))
	if (pentFriend)
	{
		/*CTalkMonster *pTalkMonster = GetClassPtr((CTalkMonster *)pentFriend->pev);
		pTalkMonster->SetAnswerQuestion(this);

		IdleHeadTurn(pentFriend->pev->origin);
		SENTENCEG_PlayRndSz(ENT(pev), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch);
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.8, 2.2);*/
		return TRUE;
	}

	// otherwise, play an idle statement
	//if (RANDOM_LONG(0,1))
	//{
	SENTENCEG_PlayRndSz(ENT(pev), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch);
	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.8, 2.2);
	return TRUE;
	//}//What if we allowed him to constantly chatter?

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE, 128.0f },	// Move within 128 of target ent (client)
	//{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slFollow[] =
{
	{
		tlFollow,
		ARRAYSIZE( tlFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t tlFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, 128.0f },	// Move within 128 of target ent (client)
	//{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t slFollowScared[] =
{
	{
		tlFollowScared,
		ARRAYSIZE( tlFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t tlFaceTargetScared[] =
{
	{ TASK_FACE_TARGET, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t slFaceTargetScared[] =
{
	{
		tlFaceTargetScared,
		ARRAYSIZE( tlFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t tlStopFollowing[] =
{
	{ TASK_CANT_FOLLOW, 0.0f },
};

Schedule_t slStopFollowing[] =
{
	{
		tlStopFollowing,
		ARRAYSIZE( tlStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};

Task_t tlHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, 70.0f },	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL, 0.0f },
	{ TASK_SAY_HEAL, 0.0f },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_ARM },			// Whip out the needle
	{ TASK_HEAL, 0.0f },	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_DISARM },			// Put away the needle
};

Schedule_t slHeal[] =
{
	{
		tlHeal,
		ARRAYSIZE( tlHeal ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"Heal"
	},
};

Task_t tlFaceTarget[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_TARGET, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slFaceTarget[] =
{
	{
		tlFaceTarget,
		ARRAYSIZE( tlFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t tlSciPanic[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_SCREAM, 0.0f },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_EXCITED },	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slSciPanic[] =
{
	{
		tlSciPanic,
		ARRAYSIZE( tlSciPanic ),
		0,
		0,
		"SciPanic"
	},
};

Task_t tlIdleSciStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2.0f }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t slIdleSciStand[] =
{
	{
		tlIdleSciStand,
		ARRAYSIZE( tlIdleSciStand ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_CLIENT_PUSH |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags
		//bits_SOUND_PLAYER |
		//bits_SOUND_WORLD |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleSciStand"

	},
};

Task_t tlScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0.0f },
	{ TASK_RUN_PATH_SCARED, 0.0f },
	{ TASK_TURN_LEFT, 179.0f },
	{ TASK_SET_SCHEDULE, (float)SCHED_HIDE },
};

Schedule_t slScientistCover[] =
{
	{
		tlScientistCover,
		ARRAYSIZE( tlScientistCover ),
		bits_COND_NEW_ENEMY,
		0,
		"ScientistCover"
	},
};

Task_t tlScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM, 10.0f },
};

Schedule_t slScientistHide[] =
{
	{
		tlScientistHide,
		ARRAYSIZE( tlScientistHide ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ScientistHide"
	},
};

Task_t tlScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM, 0.3f },				// Scream 30% of the time
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, 0.1f },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 1.0f },
};

Schedule_t slScientistStartle[] =
{
	{
		tlScientistStartle,
		ARRAYSIZE( tlScientistStartle ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"ScientistStartle"
	},
};

Task_t tlFear[] =
{
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_SAY_FEAR, 0.0f },
	//{ TASK_PLAY_SEQUENCE, (float)ACT_FEAR_DISPLAY },
};

Schedule_t slFear[] =
{
	{
		tlFear,
		ARRAYSIZE( tlFear ),
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};

DEFINE_CUSTOM_SCHEDULES( CScientist )
{
	slFollow,
	slFaceTarget,
	slIdleSciStand,
	slFear,
	slScientistCover,
	slScientistHide,
	slScientistStartle,
	slHeal,
	slStopFollowing,
	slSciPanic,
	slFollowScared,
	slFaceTargetScared,
};

IMPLEMENT_CUSTOM_SCHEDULES( CScientist, CTalkMonster )

int CScientist::GetPaid(int amount)
{
	if (!FBitSet(pev->spawnflags, SF_MONSTER_VENDOR))
		return 0;

	switch (m_vendorType)
	{
	case VENDOR_TYPE_WEP:
	{
		if (amount < VendorPrices[SelectedItem])
		{
			UTIL_CenterPrintAll("Not enough money!");
			PlaySentence("SC_VENDDECLINE", 2, VOL_NORM, ATTN_NORM);
			return 0;
		}
		else
		{
			UTIL_CenterPrintAll("Item purchased!");
			PlaySentence("SC_VENDBUY", 2, VOL_NORM, ATTN_NORM);

			UTIL_MakeVectors(pev->angles);

			CBaseEntity *pGun = DropItem(VendorItemNames[SelectedItem], pev->origin + gpGlobals->v_forward * 6 + gpGlobals->v_up * 64, pev->angles);
			pGun->pev->velocity = pGun->pev->velocity + gpGlobals->v_forward * 128;

			return VendorPrices[SelectedItem];
		}

		break;
	}
	case VENDOR_TYPE_CLOTHES:
	{
		if (amount < VendorClothesPrices[SelectedItem])
		{
			UTIL_CenterPrintAll("Not enough money!");
			PlaySentence("SC_VENDDECLINE", 2, VOL_NORM, ATTN_NORM);
			return 0;
		}
		else
		{
			UTIL_CenterPrintAll("Item purchased!");
			PlaySentence("SC_VENDBUY", 2, VOL_NORM, ATTN_NORM);

			UTIL_MakeVectors(pev->angles);

			CBaseEntity *pGun = DropItem(VendorClothesNames[SelectedItem], pev->origin + gpGlobals->v_forward * 42 + gpGlobals->v_up * 64, pev->angles);
			pGun->pev->velocity = pGun->pev->velocity + gpGlobals->v_forward * 128;

			return VendorClothesPrices[SelectedItem];
		}
		break;
	}
	case VENDOR_TYPE_FOOD:
	{
		if (amount < VendorFoodsPrices[SelectedItem])
		{
			UTIL_CenterPrintAll("Not enough money!");
			PlaySentence("SC_VENDDECLINE", 2, VOL_NORM, ATTN_NORM);
			return 0;
		}
		else
		{
			UTIL_CenterPrintAll("Item purchased!");
			PlaySentence("SC_VENDBUY", 2, VOL_NORM, ATTN_NORM);

			UTIL_MakeVectors(pev->angles);

			CBaseEntity *pGun = DropItem(VendorFoodNames[SelectedItem], pev->origin + gpGlobals->v_forward * 42 + gpGlobals->v_up * 64, pev->angles);
			pGun->pev->velocity = pGun->pev->velocity + gpGlobals->v_forward * 128;

			return VendorFoodsPrices[SelectedItem];
		}
		break;
	}
	case VENDOR_TYPE_TRINKET:
	{
		if (amount < VendorTrinketPrices[SelectedItem])
		{
			UTIL_CenterPrintAll("Not enough money!");
			PlaySentence("SC_VENDDECLINE", 2, VOL_NORM, ATTN_NORM);
			return 0;
		}
		else
		{
			UTIL_CenterPrintAll("Item purchased!");
			PlaySentence("SC_VENDBUY", 2, VOL_NORM, ATTN_NORM);

			UTIL_MakeVectors(pev->angles);

			CBaseEntity *pGun = DropItem(VendorTrinketNames[SelectedItem], pev->origin + gpGlobals->v_forward * 42 + gpGlobals->v_up * 64, pev->angles);
			pGun->pev->velocity = pGun->pev->velocity + gpGlobals->v_forward * 128;

			return VendorTrinketPrices[SelectedItem];
		}
		break;
	}
	case VENDOR_TYPE_WFOOD:
	{
		if (amount < VendorWFoodsPrices[SelectedItem])
		{
			UTIL_CenterPrintAll("Not enough money!");
			PlaySentence("SC_VENDDECLINE", 2, VOL_NORM, ATTN_NORM);
			return 0;
		}
		else
		{
			UTIL_CenterPrintAll("Item purchased!");
			PlaySentence("SC_VENDBUY", 2, VOL_NORM, ATTN_NORM);

			UTIL_MakeVectors(pev->angles);

			CBaseEntity *pGun = DropItem(VendorWFoodNames[SelectedItem], pev->origin + gpGlobals->v_forward * 42 + gpGlobals->v_up * 64, pev->angles);
			pGun->pev->velocity = pGun->pev->velocity + gpGlobals->v_forward * 128;

			return VendorWFoodsPrices[SelectedItem];
		}
		break;
	}
	default:
		return 0; break;
	}
}

void CScientist::DeclineFollowing( void )
{
	if (FBitSet(pev->spawnflags, SF_MONSTER_VENDOR))
	{
		SwitchVendorItem(m_vendorType);
		return;
	}

	if (BeingShocked)
		return;
	if (Suicidal)
	{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 2;
		ResetSequenceInfo();
		CommitSudoku();
	}
	else
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		if (pev->body == HEAD_LUKA)
			PlaySentence( "SC_POKL", 2, VOL_NORM, ATTN_NORM );
		else
			PlaySentence( "SC_POK", 2, VOL_NORM, ATTN_NORM );
	}
}

void CScientist::SwitchVendorItem(int ItemType)
{
	switch (ItemType)
	{
	case VENDOR_TYPE_WEP:
	{
		if (SelectedItem >= MAX_VENDOR_WEPS - 1)
			SelectedItem = 0;
		else
			SelectedItem += 1;
		UTIL_CenterPrintAll(VendorItems[SelectedItem]);
			  break;
	}
	case VENDOR_TYPE_CLOTHES:
	{
		if (SelectedItem >= MAX_VENDOR_CLOTHES -1)
			SelectedItem = 0;
		else
			SelectedItem += 1;
		UTIL_CenterPrintAll(VendorClothes[SelectedItem]);
		break;
	}
	case VENDOR_TYPE_FOOD:
	{
		if (SelectedItem >= MAX_VENDOR_FOOD - 1)
			SelectedItem = 0;
		else
			SelectedItem += 1;
		UTIL_CenterPrintAll(VendorFood[SelectedItem]);
		break;
	}
	case VENDOR_TYPE_TRINKET:
	{
		if (SelectedItem >= MAX_VENDOR_TRINKETS - 1)
			SelectedItem = 0;
		else
			SelectedItem += 1;
		UTIL_CenterPrintAll(VendorTrinkets[SelectedItem]);
		break;
	}
	case VENDOR_TYPE_WFOOD:
	{
		if (SelectedItem >= MAX_VENDOR_FOOD - 1)
			SelectedItem = 0;
		else
			SelectedItem += 1;
		UTIL_CenterPrintAll(VendorWFood[SelectedItem]);
		break;
	}

	default:
		break;
	}
	
	PlaySentence("SC_VEND", 2, VOL_NORM, ATTN_NORM);
}


const char *CScientist::GetScientistModel( void )
{
	if( !g_psv_override_scientist_mdl )
		g_psv_override_scientist_mdl = CVAR_GET_POINTER( "_sv_override_scientist_mdl" );

	if( !( g_psv_override_scientist_mdl && g_psv_override_scientist_mdl->string ))
		return "models/scientist.mdl";

	if( strlen( g_psv_override_scientist_mdl->string ) < sizeof( "01.mdl" ) - 1 )
		return "models/scientist.mdl";

	return g_psv_override_scientist_mdl->string;
}

void CScientist::Scream( void )
{
	if( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT( 3.0f, 6.0f ), VOL_NORM, ATTN_NORM );
	}
}

Activity CScientist::GetStoppedActivity( void )
{ 
	if( m_hEnemy != 0 ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}

void CScientist::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
		//if( FOkToSpeak() )
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "SC_HEAL", 2, VOL_NORM, ATTN_IDLE );
		TaskComplete();
		break;
	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;
	case TASK_RANDOM_SCREAM:
		if( RANDOM_FLOAT( 0.0f, 1.0f ) < pTask->flData )
			Scream();
		TaskComplete();
		break;
	case TASK_SAY_FEAR:
		if( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;

			//The enemy can be null here. - Solokiller
			//Discovered while testing the barnacle grapple on headcrabs with scientists in view.
			if( m_hEnemy != 0 && m_hEnemy->IsPlayer() )
				PlaySentence( "SC_PLFEAR", 5, VOL_NORM, ATTN_NORM );
			else
				PlaySentence( "SC_FEAR", 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_BITE;
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if( ( m_hTargetEnt->pev->origin - pev->origin ).Length() < 1.0f )
			{
				TaskComplete();
			}
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
					TaskFail();
			}
		}
		break;
	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CScientist::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if( MovementIsComplete() )
			TaskComplete();
		if( RANDOM_LONG( 0, 31 ) < 8 )
			Scream();
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if( RANDOM_LONG( 0, 63 ) < 8 )
				Scream();

			if( m_hEnemy == 0 )
			{
				TaskFail();
			}
			else
			{
				float distance;

				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if( ( distance < pTask->flData ) || ( m_vecMoveGoal - m_hTargetEnt->pev->origin ).Length() > pTask->flData * 0.5f )
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if( distance < pTask->flData )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
					m_movementActivity = ACT_WALK_SCARED;
				else if( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
					m_movementActivity = ACT_RUN_SCARED;
			}
		}
		break;
	case TASK_HEAL:
		if( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			if( TargetDistance() > 90 )
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CScientist::Classify( void )
{
	return CLASS_HUMAN_PASSIVE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CScientist::SetYawSpeed( void )
{
	int ys;

	ys = 90;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	default:
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CScientist::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{	
	case SCIENIST_DEATHSHOT:
		{
			Vector target = m_hTargetEnt->pev->origin - pev->origin;
			if (target.Length() > 150)
				return;

			if (m_hTargetEnt->IsPlayer())
			{
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
				m_hTargetEnt->TakeDamage(pev, pev, 300, DMG_PARALYZE);
			}
		}
	case SCIENTIST_GRAB:
		{
		Grab();
		}	
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
		//Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
		{
			int oldBody = pev->body;
			pev->body = ( oldBody % NUM_SCIENTIST_HEADS ) + NUM_SCIENTIST_HEADS * 1;
		}
		break;
	case SCIENTIST_AE_NEEDLEOFF:
		{
			int oldBody = pev->body;
			pev->body = ( oldBody % NUM_SCIENTIST_HEADS ) + NUM_SCIENTIST_HEADS * 0;
		}
		break;
	case FATSCIENTIST_WATERCANON:
		{
		int oldBody = pev->body;
		pev->body = 1;
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "newsounds/watercan.wav", 1, ATTN_NORM, 0, 100);
		}
		break;
	case FATSCIENTIST_WATERCANOFF:
		{
		int oldBody = pev->body;
		pev->body = 0;
		}
		break;
		case FATSCIENTIST_CUPON:
		{
		int oldBody = pev->body;
		pev->body = 2;
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/slurp.wav", 1, ATTN_NORM, 0, 100);
		}
		break;
		case FATSCIENTIST_CUPOFF:
		{
		int oldBody = pev->body;
		pev->body = 0;
		}
		break;
	case SCIENTIST_DESPACITO:
		{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 2;
		ResetSequenceInfo();
		CommitSudoku();
		}
		break;
	case SCIENTIST_SUDOKU:
		{
		Sudoku();
		}
		break;
	case SCIENTIST_SHOCKED:
		{
		BeingShocked = true;
		SENTENCEG_PlayRndSz(ENT(pev), "SC_SHOCKED", 1.0, ATTN_NORM, 0, m_voicePitch);
		pev->nextthink = gpGlobals->time + 6.9;
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
		WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(25); // scale * 10
		WRITE_BYTE(10); // framerate
		MESSAGE_END();
		CBeam *m_pBeam[3];
		for (int i = 0; i < 3; i++)
		{
			m_pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
			if (!m_pBeam)
				return;

			m_pBeam[i]->PointsInit(pev->origin + gpGlobals->v_up * RANDOM_LONG(32, 64) + gpGlobals->v_right * RANDOM_LONG(-20, 20) - gpGlobals->v_forward * 15, pev->origin + gpGlobals->v_forward * 52 + gpGlobals->v_up * 40 + gpGlobals->v_right * RANDOM_LONG(-10, 10));
			m_pBeam[i]->SetColor(20, 20, 255);
			m_pBeam[i]->SetBrightness(255);
			m_pBeam[i]->LiveForTime(7);
			m_pBeam[i]->SetNoise(95);
		}
		}
		break;
	case SCIENTIST_EXPLODE:
		{
		CGrenade::ShootContact(pev, pev->origin, gpGlobals->v_up);
		}
		break;
	case SCIENTIST_RAGE:
		{
		SENTENCEG_PlayRndSz(ENT(pev), "SC_RAGE", 1.0, ATTN_NORM, 0, m_voicePitch);
		BeingShocked = true;
		}
		break;
	case SCIENTIST_RAGEHIT:
		{
		UTIL_Sparks(pev->origin + gpGlobals->v_forward * 14 + gpGlobals->v_up * RANDOM_LONG(50, 72) + gpGlobals->v_right * RANDOM_LONG(-30, 30));
		}
		break;
	case SCIENTIST_STOPRAGE:
		{
		BeingShocked = false;
		}
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

void CScientist::CommitSudoku(void)
{
	
	SENTENCEG_PlayRndSz(ENT(pev), "SC_DESPACITO", 1.0, ATTN_NORM, 0, m_voicePitch);
	pev->sequence = LookupSequence("sudoku");
	pev->frame = 0;
	pev->framerate = 1;
	ResetSequenceInfo();
	pev->nextthink = gpGlobals->time + 2.3;
}

void CScientist::Sudoku(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "weapons/357_shot1.wav", 1, ATTN_NORM, 0, 100);
	pev->effects = EF_MUZZLEFLASH;

	TakeDamage(pev, pev, pev->health, DMG_BULLET);
	SpawnBlood(pev->origin + gpGlobals->v_up * 64, BloodColor(), 100);// a little surface blood.
	m_LastHitGroup = HITGROUP_HEAD;

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
}

//=========================================================
// Spawn
//=========================================================
void CScientist::Spawn( void )
{
	// We need to set it before precache so the right voice will be chosen
	if( pev->body == -1 )
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );// pick a head, any head
	}

	Precache();

	SET_MODEL( ENT( pev ), GetScientistModel());
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//m_flDistTooFar = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	// Luther is black, make his hands black
	if( pev->body == HEAD_LUTHER || pev->body == HEAD_LUTHER2 )
		pev->skin = 1;

		SelectedItem = 0;
	if (m_iTriggerCondition == AITRIGGER_NONE)
	{
		if (FStringNull(pev->targetname) && (RANDOM_LONG(0, 4) == 1))
			Suicidal = true;
		else
			Suicidal = false;
	}

	BeingShocked = false;

	//AddShockEffect(255, 155, 155, 16, 15);

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CScientist::Precache( void )
{
	PRECACHE_MODEL( GetScientistModel());
	PRECACHE_SOUND( "scientist/sci_pain1.wav" );
	PRECACHE_SOUND("common/bodydrop1.wav");
	PRECACHE_SOUND( "scientist/sci_pain2.wav" );
	PRECACHE_SOUND( "scientist/sci_pain3.wav" );
	PRECACHE_SOUND( "scientist/sci_pain4.wav" );
	PRECACHE_SOUND( "scientist/sci_pain5.wav" );

	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");
	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");
	PRECACHE_SOUND("weapons/slurp.wav");
	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("scientist/shocked.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CScientist::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "SC_ANSWER";
	m_szGrp[TLK_QUESTION] = "SC_QUESTION";
	m_szGrp[TLK_IDLE] = "SC_IDLE";
	m_szGrp[TLK_STARE] = "SC_STARE";
	m_szGrp[TLK_USE] = "SC_OK";
	m_szGrp[TLK_UNUSE] = "SC_WAIT";
	m_szGrp[TLK_STOP] = "SC_STOP";
	m_szGrp[TLK_NOSHOOT] = "SC_SCARED";
	m_szGrp[TLK_HELLO] = "SC_HELLO";

	m_szGrp[TLK_PLHURT1] = "!SC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!SC_CUREB"; 
	m_szGrp[TLK_PLHURT3] = "!SC_CUREC";

	m_szGrp[TLK_PHELLO] = "SC_PHELLO";
	m_szGrp[TLK_PIDLE] = "SC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
	m_szGrp[TLK_SMELL] = "SC_SMELL";

	m_szGrp[TLK_WOUND] = "SC_WOUND";
	m_szGrp[TLK_MORTAL] = "SC_MORTAL";

	// get voice for head
	switch( pev->body % NUM_SCIENTIST_HEADS )
	{
	default:
	case HEAD_GLASSES:
		m_voicePitch = 105;
		break;	//glasses
	case HEAD_EINSTEIN:
		m_voicePitch = 100;
		break;	//einstein
	case HEAD_LUTHER:
		m_voicePitch = 95;
		break;	//luther
	case HEAD_SLICK:
		m_voicePitch = 100;
		break;	//slick
	case HEAD_LUKA:		
		m_voicePitch = 90;  
		break;  //Lukashenko
	case HEAD_VORT:		
		m_voicePitch = 70; 
		break;  //Vortigaunt
	}
}

int CScientist::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}

	if ((FClassnameIs(pevInflictor, "monster_madscientist") || FClassnameIs(pevInflictor, "monster_forklift")) || (FClassnameIs(pevInflictor, "monster_camerasci") && (RANDOM_LONG(0, 1) == 0)))
	{
		int pitch = GetVoicePitch();
		SetThink(&CBaseEntity::SUB_Remove);
		CBaseEntity *pMad = CBaseEntity::Create("monster_madscientist", pev->origin, pev->angles);
		pMad->pev->targetname = pev->targetname;
		pMad->pev->modelindex = pev->modelindex;
		pMad->pev->frame = 0;
		pMad->pev->body = pev->body;
		pMad->pev->skin = pev->skin;
		int oldBody = pMad->pev->body;
		pMad->pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 1;
		SENTENCEG_PlayRndSz(ENT(pMad->pev), "SC_FUCKIT", 1.0, ATTN_IDLE, 0, pitch);
		pMad->pev->sequence = LookupSequence("pull_needle");
	}

	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CScientist::ISoundMask( void )
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

//=========================================================
// PainSound
//=========================================================
void CScientist::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	//SPECIAL HAPPENING
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	{
		switch (RANDOM_LONG(0, 10))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain1.wav", 1, ATTN_NORM, 0, 100); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain2.wav", 1, ATTN_NORM, 0, 100); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain3.wav", 1, ATTN_NORM, 0, 100); break;
		case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain4.wav", 1, ATTN_NORM, 0, 100); break;
		case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain5.wav", 1, ATTN_NORM, 0, 100); break;
		case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain6.wav", 1, ATTN_NORM, 0, 100); break;
		case 6: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain7.wav", 1, ATTN_NORM, 0, 100); break;
		case 7: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain8.wav", 1, ATTN_NORM, 0, 100); break;
		case 8: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain9.wav", 1, ATTN_NORM, 0, 100); break;
		case 9: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain10.wav", 1, ATTN_NORM, 0, 100); break;
		case 10: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain11.wav", 1, ATTN_NORM, 0, 100); break;
		}
		return;
	}
	//SPECIAL HAPPENING

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 0.75f );

	switch( RANDOM_LONG( 0, 4 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 3:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 4:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CScientist::DeathSound( void )
{
	PainSound();
}

void CScientist::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CScientist::SetActivity( Activity newActivity )
{
	int iSequence;

	iSequence = LookupActivity( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}

Schedule_t *CScientist::GetScheduleOfType( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
			return slFaceTarget;	// override this for different target face behavior
		else
			return psched;
	case SCHED_TARGET_CHASE:
		return slFollow;
	case SCHED_CANT_FOLLOW:
		return slStopFollowing;
	case SCHED_PANIC:
		return slSciPanic;
	case SCHED_TARGET_CHASE_SCARED:
		return slFollowScared;
	case SCHED_TARGET_FACE_SCARED:
		return slFaceTargetScared;
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
			return slIdleSciStand;
		else
			return psched;
	case SCHED_HIDE:
		return slScientistHide;
	case SCHED_STARTLE:
		return slScientistStartle;
	case SCHED_FEAR:
		return slFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CScientist::GetSchedule( void )
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( pEnemy )
		{
			if( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = 0;
				pEnemy = 0;
			}
		}

		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Cower when you hear something scary
		if( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT( pSound != NULL );
			if( pSound )
			{
				if( pSound->m_iType & ( bits_SOUND_DANGER | bits_SOUND_COMBAT ) )
				{
					if( gpGlobals->time - m_fearTime > 3 )	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if( IsFollowing() )
		{
			if( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if( pEnemy != NULL )
				relationship = IRelationship( pEnemy );

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if( relationship != R_DL && relationship != R_HT )
			{
				// If I'm already close enough to my target
				if( TargetDistance() <= 128 )
				{
					if( CanHeal() )	// Heal opportunistically
						return slHeal;
					if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
					return GetScheduleOfType( SCHED_FEAR );					// React to something scary
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
			}
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if( HasConditions( bits_COND_NEW_ENEMY ) )
			return slFear;					// Point and scream!
		if( HasConditions( bits_COND_SEE_ENEMY ) )
			return slScientistCover;		// Take Cover

		if( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slScientistCover;			// Run & Cower
		break;
	default:
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CScientist::GetIdealState( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if( IsFollowing() )
			{
				int relationship = IRelationship( m_hEnemy );
				if( relationship != R_FR || ( relationship != R_HT && !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) ) )
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
				StopFollowing( TRUE );
			}
		}
		else if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// Stop following if you take damage
			if( IsFollowing() )
				StopFollowing( TRUE );
		}
		break;
	case MONSTERSTATE_COMBAT:
		{
			CBaseEntity *pEnemy = m_hEnemy;
			if( pEnemy != NULL )
			{
				if( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
				{
					// Strip enemy when going to alert
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					m_hEnemy = 0;
					return m_IdealMonsterState;
				}

				// Follow if only scared a little
				if( m_hTargetEnt != 0 )
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}

				if( HasConditions( bits_COND_SEE_ENEMY ) )
				{
					m_fearTime = gpGlobals->time;
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return m_IdealMonsterState;
				}
			}
		}
		break;
	default:
		break;
	}

	return CTalkMonster::GetIdealState();
}

BOOL CScientist::CanHeal( void )
{ 
	if (Suicidal)
	{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 2;
		ResetSequenceInfo();
		CommitSudoku();
		return FALSE;
	}

	if( ( m_healTime > gpGlobals->time ) || ( m_hTargetEnt == 0 ) || ( m_hTargetEnt->pev->health > ( m_hTargetEnt->pev->max_health * 0.5f ) ) )
		return FALSE;

	return TRUE;
}

void CScientist::Heal( void )
{
	if( !CanHeal() )
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if( target.Length() > 150.0f )
		return;

	m_hTargetEnt->TakeHealth( gSkillData.scientistHeal, DMG_GENERIC );
}

void CScientist::Grab(void)
{
	if (!CanHeal())
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if( target.Length() > 100.0f )
		return;

	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;

	if (m_hTargetEnt->IsPlayer())
	{
		m_hTargetEnt->TakeDamage(pev, pev, 1, DMG_PARALYZE);
		m_hTalkTarget->pev->origin = pev->origin + gpGlobals->v_up * 44 + gpGlobals->v_forward * 28;
		m_hTalkTarget->pev->angles = -pev->angles;
		m_hTalkTarget->FBecomeProne();
		m_hTalkTarget->pev->weapons = 0;
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/bodydrop1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		SENTENCEG_PlayRndSz(ENT(pev), "SC_DEATHSHOT", 1.0, ATTN_IDLE, 0, GetVoicePitch());
		CBaseEntity *pEntity = Create("player_weaponstrip", m_hTalkTarget->pev->origin, m_hTalkTarget->pev->angles);
		pEntity->Use(m_hTalkTarget, m_hTalkTarget, USE_SET, 1);

		//m_hTargetEnt->TakeDamage(pev, pev, m_hTargetEnt->pev->health, DMG_PARALYZE);
	}
}


int CScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if( arrayNumber < 3 )
		return array[arrayNumber];
	return arrayNumber;
}


//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CBaseMonster
{
public:
	void Spawn( void );
	int Classify( void )
	{
		return CLASS_HUMAN_PASSIVE;
	}

	void KeyValue( KeyValueData *pkvd );
	int m_iPose;// which sequence to display
	static const char *m_szPoses[7];

private:
	const char *GetScientistModel( void );
};

const char *CDeadScientist::m_szPoses[] =
{
	"lying_on_back",
	"lying_on_stomach",
	"dead_sitting",
	"dead_hang",
	"dead_table1",
	"dead_table2",
	"dead_table3"
};

void CDeadScientist::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_scientist_dead, CDeadScientist )

//
// ********** DeadScientist SPAWN **********
//
void CDeadScientist::Spawn()
{
	const char *pszModel = GetScientistModel();

	PRECACHE_MODEL( pszModel );
	SET_MODEL( ENT( pev ), pszModel );

	pev->effects = 0;
	pev->sequence = 0;

	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	if( pev->body == -1 )
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );// pick a head, any head
	}

	// Luther is black, make his hands black
	if( pev->body == HEAD_LUTHER )
		pev->skin = 1;
	else
		pev->skin = 0;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if( pev->sequence == -1 )
	{
		ALERT ( at_console, "Dead scientist with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}

const char *CDeadScientist::GetScientistModel( void )
{
	if( !g_psv_override_scientist_mdl )
		g_psv_override_scientist_mdl = CVAR_GET_POINTER( "_sv_override_scientist_mdl" );

	if( !( g_psv_override_scientist_mdl && g_psv_override_scientist_mdl->string ))
		return "models/scientist.mdl";

	if( strlen( g_psv_override_scientist_mdl->string ) < sizeof( "01.mdl" ) - 1 )
		return "models/scientist.mdl";

	return g_psv_override_scientist_mdl->string;
}

//=========================================================
// Sitting Scientist PROP
//=========================================================
class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn( void );
	void Precache( void );

	void EXPORT SittingThink( void );
	int Classify( void );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual void SetAnswerQuestion( CTalkMonster *pSpeaker );
	int FriendNumber( int arrayNumber );

	int FIdleSpeak( void );
	int m_baseSequence;	
	int m_headTurn;
	float m_flResponseDelay;
};

LINK_ENTITY_TO_CLASS( monster_sitting_scientist, CSittingScientist )

TYPEDESCRIPTION	CSittingScientist::m_SaveData[] =
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingScientist, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingScientist, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingScientist, CScientist )

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;

//
// ********** Scientist SPAWN **********
//
void CSittingScientist::Spawn()
{
	PRECACHE_MODEL( "models/scientist.mdl" );
	SET_MODEL( ENT( pev ), "models/scientist.mdl" );
	Precache();
	InitBoneControllers();

	UTIL_SetSize( pev, Vector( -14, -14, 0 ), Vector( 14, 14, 36 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = 50;
	
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability= bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits( pev->spawnflags, SF_MONSTER_PREDISASTER ); // predisaster only!

	if( pev->body == -1 )
	{
		// -1 chooses a random head
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );// pick a head, any head
	}

	// Luther is black, make his hands black
	if( pev->body == HEAD_LUTHER )
		pev->skin = 1;

	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG( 0, 4 );
	ResetSequenceInfo();

	SetThink( &CSittingScientist::SittingThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	DROP_TO_FLOOR( ENT( pev ) );
}

void CSittingScientist::Precache( void )
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int CSittingScientist::Classify( void )
{
	return CLASS_HUMAN_PASSIVE;
}

int CSittingScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if( arrayNumber < 3 )
		return array[arrayNumber];
	return arrayNumber;
}

//=========================================================
// sit, do stuff
//=========================================================
void CSittingScientist::SittingThink( void )
{
	CBaseEntity *pent;

	StudioFrameAdvance();

	// try to greet player
	if( FIdleHello() )
	{
		pent = FindNearestFriend( TRUE );
		if( pent )
		{
			float yaw = VecToYaw( pent->pev->origin - pev->origin ) - pev->angles.y;

			if( yaw > 180 )
				yaw -= 360;
			if( yaw < -180 )
				yaw += 360;

			if( yaw > 0 )
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

		ResetSequenceInfo();
		pev->frame = 0;
		SetBoneController( 0, 0 );
		}
	}
	else if( m_fSequenceFinished )
	{
		int i = RANDOM_LONG( 0, 99 );
		m_headTurn = 0;
		
		if( m_flResponseDelay && gpGlobals->time > m_flResponseDelay )
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if( i < 30 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	

			// turn towards player or nearest friend and speak

			if( !FBitSet( m_bitsSaid, bit_saidHelloPlayer ) )
				pent = FindNearestFriend( TRUE );
			else
				pent = FindNearestFriend( FALSE );

			if( !FIdleSpeak() || !pent )
			{
				m_headTurn = RANDOM_LONG( 0, 8 ) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				// only turn head if we spoke
				float yaw = VecToYaw( pent->pev->origin - pev->origin ) - pev->angles.y;

				if( yaw > 180 )
					yaw -= 360;
				if( yaw < -180 )
					yaw += 360;

				if( yaw > 0 )
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

				//ALERT( at_console, "sitting speak\n" );
			}
		}
		else if( i < 60 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	
			m_headTurn = RANDOM_LONG( 0, 8 ) * 10 - 40;
			if( RANDOM_LONG( 0, 99 ) < 5 )
			{
				//ALERT( at_console, "sitting speak2\n" );
				FIdleSpeak();
			}
		}
		else if( i < 80 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		}
		else if( i < 100 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		}

		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, m_headTurn );
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

// prepare sitting scientist to answer a question
void CSittingScientist::SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT( 3.0f, 4.0f );
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}

//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingScientist::FIdleSpeak( void )
{ 
	// try to start a conversation, or make statement
	int pitch;

	if( !FOkToSpeak() )
		return FALSE;

	// set global min delay for next conversation
	//Lowered it so that they would be constantly shittalking
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.8, 2.2 );

	pitch = GetVoicePitch();

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
	CBaseEntity *pentFriend = FindNearestFriend( FALSE );

	//if( pentFriend && RANDOM_LONG( 0, 1 ) )
	if (pentFriend)
	{
		CTalkMonster *pTalkMonster = GetClassPtr( (CTalkMonster *)pentFriend->pev );
		pTalkMonster->SetAnswerQuestion( this );

		IdleHeadTurn( pentFriend->pev->origin );
		SENTENCEG_PlayRndSz( ENT( pev ), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.8, 2.2 );
		return TRUE;
	}

	// otherwise, play an idle statement
	//if( RANDOM_LONG( 0, 1 ) )
	//{
		SENTENCEG_PlayRndSz( ENT( pev ), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.8, 2.2 );
		return TRUE;
	//}//What if we allowed him to constantly chatter?

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}

class CFatScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn(void);
	void SetYawSpeed(void);
};

LINK_ENTITY_TO_CLASS(monster_fatscientist, CFatScientist);

//
// ********** Scientist SPAWN **********
//
void CFatScientist::Spawn()
{
	PRECACHE_MODEL("models/fatscientist.mdl");
	PRECACHE_SOUND("newsounds/watercan.wav");
	PRECACHE_SOUND("weapons/slurp.wav");
	TalkInit();
	SET_MODEL(ENT(pev), "models/fatscientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.6; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS - 1);// pick a head, any head
	}

	// Luther is black, make his hands black
	if (pev->body == HEAD_LUTHER || pev->body == HEAD_LUTHER2)
		pev->skin = 1;


	SelectedItem = 0;

	if (FStringNull(pev->targetname) && (RANDOM_LONG(0, 4) == 1))
		Suicidal = true;
	else
		Suicidal = false;

	BeingShocked = false;

	//AddShockEffect(255, 155, 155, 16, 15);

	MonsterInit();
	SetUse(&CTalkMonster::FollowerUse);
}

void CFatScientist::SetYawSpeed(void)
{
	pev->yaw_speed = 360;
}

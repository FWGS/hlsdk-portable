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
// barnacle - stationary ceiling mounted 'fishing' monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"

#define BARNACLE_KILL_VICTIM_DELAY	3 // how many seconds after pulling prey in to gib them. 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BARNACLE_AE_PUKEGIB	2

class CBarnacle : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	CBaseEntity *TongueTouchEnt ( float *pflLength );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void EXPORT BarnacleThink ( void );
	void EXPORT WaitTillDead ( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flAltitude;
	float m_flKillVictimTime;
	int	  m_cGibs;// barnacle loads up on gibs each time it kills something.
	BOOL  m_fTongueExtended;
	BOOL  m_fLiftingPrey;
	Vector m_teleportorigin;
	Vector m_speedvec;
	float m_flTongueAdj;
};
LINK_ENTITY_TO_CLASS( monster_barnacle, CBarnacle );
LINK_ENTITY_TO_CLASS( monster_barnacle_fantasy, CBarnacle );
LINK_ENTITY_TO_CLASS( monster_barnacle_fantasy_r, CBarnacle );

TYPEDESCRIPTION	CBarnacle::m_SaveData[] = 
{
	DEFINE_FIELD( CBarnacle, m_flAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( CBarnacle, m_flKillVictimTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacle, m_cGibs, FIELD_INTEGER ),// barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD( CBarnacle, m_fTongueExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle, m_fLiftingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarnacle, m_flTongueAdj, FIELD_FLOAT ),
	DEFINE_FIELD( CBarnacle, m_teleportorigin, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CBarnacle, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBarnacle :: Classify ( void )
{
	if ( FClassnameIs(pev, "monster_barnacle_fantasy_r")){
	return	CLASS_HUMAN_ASS;
	}
	else if ( FClassnameIs(pev, "monster_barnacle_fantasy")){
	return	CLASS_ALIEN_MONSTER;
	}
	else{
	return	CLASS_ALIEN_PREY;
	}
}

void CBarnacle :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNACLE_AE_PUKEGIB:
		CGib::SpawnRandomGibs( pev, 1, 1,m_diefadeout );	
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarnacle :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/barnacle.mdl");
	UTIL_SetSize( pev, Vector(-16, -16, -32), Vector(16, 16, 0) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->takedamage		= DAMAGE_AIM;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= EF_INVLIGHT; // take light from the ceiling 

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 40;
	}
	else{
	pev->health			= 24;
	}

	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flKillVictimTime	= 0;
	m_cGibs				= 0;
	m_fLiftingPrey		= FALSE;
	m_flTongueAdj		= -100;

	InitBoneControllers();

	SetActivity ( ACT_IDLE );

	pev->flags			|= FL_MONSTER;//�����ʶ
	pev->flags			|= FL_NOTARGET;
	m_selfmode	= TRUE;
	m_duckseq = 1;

	SetThink ( &CBarnacle::BarnacleThink );
	pev->nextthink = gpGlobals->time + 0.5;

	UTIL_SetOrigin ( pev, pev->origin );
	m_killed_exp = 20;
	m_rpgms_level = 20;

	pev->netname = MAKE_STRING( "Barnacle" );

	if ( FClassnameIs(pev, "monster_barnacle_fantasy") || FClassnameIs(pev, "monster_barnacle_fantasy_r")){
	SET_MODEL(ENT(pev), "models/barnacle_fantasy.mdl");
	UTIL_SetSize( pev, Vector(-32, -32, -64), Vector(32, 32, 64) );

	if ( FClassnameIs(pev, "monster_barnacle_fantasy_r")){
	m_killed_exp = 40;
	m_rpgms_level = 60;
	if (g_iSkillLevel == SKILL_HARD){
	pev->health	 = 300;
	}
	else{
	pev->health	 = 240;
	}
	pev->netname = MAKE_STRING( "Barnacle.Fantasy.R" );
	pev->skin = 1;
	m_bloodColor		= BLOOD_COLOR_RED;
	}
	else{
	m_killed_exp = 30;
	m_rpgms_level = 50;
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 240;
	}
	else{
	pev->health	 = 180;
	}
	pev->netname = MAKE_STRING( "Barnacle.Fantasy" );
	pev->skin = 0;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	}
	pev->flags &= ~FL_NOTARGET;

	m_duckseq = 0;
	pev->movetype = MOVETYPE_BOUNCEMISSILE;

	pev->flags	 |= FL_FLY;
	pev->gravity = 0.5;
	m_flFieldOfView = -1;
	m_teleportorigin = pev->origin;

	m_flDistLook		= 4096.0;

	m_lookignoremod		= 1;

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
		WRITE_BYTE(3);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z);
		WRITE_SHORT(g_sModelIndexCteleport);
		WRITE_BYTE(15);
		WRITE_BYTE(15);
		WRITE_BYTE(4);
		MESSAGE_END();
						
		EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "debris/beamstart2old.wav", 1, ATTN_NORM, 0, 100 );

	}
}

int CBarnacle::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & DMG_CLUB )
	flDamage *= 4.0;//�����˺�!

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
//=========================================================
void CBarnacle :: BarnacleThink ( void )
{
	CBaseEntity *pTouchEnt;
	CBaseMonster *pVictim;
	float flLength;

	int bbh = 36;
	int bps = 4;

	if ( FClassnameIs(pev, "monster_barnacle")){

		pev->nextthink = gpGlobals->time + 0.05;

		if (FBitSet( pev->flags, FL_NOTARGET )){
			if ( m_hTargetEnt != NULL ){
			pev->flags &= ~FL_NOTARGET;
			}
		}
		else if (!FBitSet( pev->flags, FL_NOTARGET )){
			if ( m_hTargetEnt == NULL ){
			pev->flags	|= FL_NOTARGET;
			}
		}
	}
	else{
		bbh = 72;
		bps = 8;

		pev->nextthink = gpGlobals->time + 0.05;

			if(pev->movetype == MOVETYPE_BOUNCEMISSILE){
					if(m_hTargetEnt != NULL || m_fLiftingPrey != NULL){
						pev->velocity = g_vecZero;
						if ( !m_fLiftingPrey && !m_hTargetEnt->IsAlive() )
						{
							m_fLiftingPrey = FALSE;// indicate that we're not lifting prey.
							m_hTargetEnt = NULL;
						}
					}
					else{
						Look( m_flDistLook );
						m_hEnemy = BestVisibleEnemy();

						if (m_teleportorigin.z - pev->origin.z > 30){
							pev->velocity.z = 30;
						}
						else if (m_teleportorigin.z - pev->origin.z < -30){
							pev->velocity.z = -30;
						}

						if(m_hEnemy != NULL){
							if(( pev->origin - m_hEnemy->pev->origin).Length2D() >= 300){
								m_speedvec = (m_hEnemy->pev->origin - pev->origin).Normalize() * 300;
								pev->velocity.x = m_speedvec.x;
								pev->velocity.y = m_speedvec.y;
							}
							else if(pev->velocity.Length() < 150){
								pev->velocity.x += RANDOM_LONG(-300,300);
								pev->velocity.y += RANDOM_LONG(-300,300);
							}
						}
					}
			}
	}

	if ( m_hTargetEnt != NULL )
	{
// barnacle has prey.

		if ( !m_fLiftingPrey && !m_hTargetEnt->IsAlive() )
		{
			if ( fabs( pev->origin.z - ( m_hTargetEnt->pev->origin.z + m_hTargetEnt->pev->view_ofs.z - 8 ) ) >= bbh )
			{
			m_hTargetEnt->pev->velocity = g_vecZero;
			UTIL_SetOrigin ( m_hTargetEnt->pev, m_hTargetEnt->pev->origin + Vector(0,0,1) );
			}
		}

		pVictim = m_hTargetEnt->MyMonsterPointer();
		if ( pVictim ){
			if( (m_hTargetEnt->pev->flags & FL_MONSTER) && m_hTargetEnt->IsAlive() ){
				if ( pVictim->m_MonsterState != MONSTERSTATE_PRONE 
				&& pVictim->m_IdealMonsterState != MONSTERSTATE_PRONE
				&& pVictim->m_barnacleres_time > 0){
				TakeDamage ( pev, m_hTargetEnt->pev, 300, DMG_SLASH );
				m_fLiftingPrey = FALSE;
				m_hTargetEnt = NULL;	
				return;
				}
			}
		}

				if(m_hTargetEnt->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hTargetEnt->pev);
						if ( !m_fLiftingPrey && player->m_barnacle_Level == 1){
						player->m_barnacle_Level = 3;//�ȼ�����
						}
						if(player->m_barnacle_RTP_bar >= 255){

						if ( fabs( pev->origin.z - ( m_hTargetEnt->pev->origin.z + m_hTargetEnt->pev->view_ofs.z - 8 ) ) < bbh ){
							TakeDamage ( pev, player->pev, 300, DMG_SLASH );	
						}
						else{
								if ( m_hTargetEnt != NULL )
								{
									if ( pVictim )
									{
										pVictim->BarnacleVictimReleased();
									}

									CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hTargetEnt->pev);
									if(player->m_barnacle_RTP != 0){
										player->m_barnacle_RTP_relase = 1;
										player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
										player->m_barnacle_RTP = 0;
										player->m_barnacle_RTP_bar = 0;
										player->m_barnacle_Level = 0;
										player->m_barnacle_catchme = NULL;
									}

									m_fLiftingPrey = FALSE;
									m_hTargetEnt = NULL;
								}
								//pev->solid = SOLID_NOT;
								pev->nextthink = gpGlobals->time + 0.5;
						}

						return;
						}
				}

		if ( m_fLiftingPrey )
		{
			if ( m_hTargetEnt != NULL && m_hTargetEnt->pev->deadflag != DEAD_NO )
			{
				// crap, someone killed the prey on the way up.
				m_hTargetEnt = NULL;
				m_fLiftingPrey = FALSE;
				return;
			}

	// still pulling prey.
			Vector vecNewEnemyOrigin = m_hTargetEnt->pev->origin;
			vecNewEnemyOrigin.x = pev->origin.x;
			vecNewEnemyOrigin.y = pev->origin.y;

			// guess as to where their neck is
			vecNewEnemyOrigin.x -= 6 * cos(m_hTargetEnt->pev->angles.y * M_PI/180.0);	
			vecNewEnemyOrigin.y -= 6 * sin(m_hTargetEnt->pev->angles.y * M_PI/180.0);
			
			m_flAltitude -= bps;
			vecNewEnemyOrigin.z += bps;
			
				Vector vecArmPos,vecArmDir;
				GetBonePosition( 0, vecArmPos, vecArmDir );
				vecNewEnemyOrigin.x = vecArmPos.x;
				vecNewEnemyOrigin.y = vecArmPos.y;

			if ( fabs( pev->origin.z - ( vecNewEnemyOrigin.z + m_hTargetEnt->pev->view_ofs.z - 8 ) ) < bbh
			|| (pev->origin.z < ( vecNewEnemyOrigin.z + m_hTargetEnt->pev->view_ofs.z + 8)) )
			{
		// prey has just been lifted into position ( if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body )
				m_fLiftingPrey = FALSE;

				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_bite3.wav", 1, ATTN_NORM );	

				m_flKillVictimTime = gpGlobals->time + 5;

				if (FClassnameIs(pev, "monster_barnacle_fantasy_r")){
				pVictim->TakeDamage ( pev, pev, 300, DMG_SLASH | DMG_NEVERGIB );
				}
				else if ( FClassnameIs(pev, "monster_barnacle_fantasy")){
				pVictim->TakeDamage ( pev, pev, 200, DMG_SLASH | DMG_NEVERGIB );
				}
				else{
				pVictim->TakeDamage ( pev, pev, 100, DMG_SLASH | DMG_NEVERGIB );
				}

				int blood = m_hTargetEnt->BloodColor();
				SpawnBlood(m_hTargetEnt->pev->origin + m_hTargetEnt->pev->view_ofs, blood, 150);
				if (blood == BLOOD_COLOR_RED){
				UTIL_BloodStream( m_hTargetEnt->pev->origin + m_hTargetEnt->pev->view_ofs,gpGlobals->v_up * -5,70, 100 );
				FX_Explosion( m_hTargetEnt->pev->origin + m_hTargetEnt->pev->view_ofs, 234 );
				}
				else if (blood == BLOOD_COLOR_YELLOW || blood == BLOOD_COLOR_GREEN){
				UTIL_BloodStream( m_hTargetEnt->pev->origin + m_hTargetEnt->pev->view_ofs,gpGlobals->v_up * -5,58, 100 );
				FX_Explosion( m_hTargetEnt->pev->origin + m_hTargetEnt->pev->view_ofs, 235 );
				}

				if ( pVictim )
				{
					pVictim->BarnacleVictimBitten( pev );
					SetActivity ( ACT_EAT );
				}
			}

			UTIL_SetOrigin ( m_hTargetEnt->pev, vecNewEnemyOrigin );
		}
		else
		{
	// prey is lifted fully into feeding position and is dangling there.
			if ( m_flKillVictimTime != -1 && gpGlobals->time > m_flKillVictimTime )
			{
				// kill!
				if ( pVictim )
				{
					if(pVictim->pev->deadflag != DEAD_NO && (pVictim->pev->flags & FL_CLIENT) ){
					pVictim->BarnacleVictimReleased();//��ҳԲ���
					m_fLiftingPrey = FALSE;
					m_hTargetEnt = NULL;
					}
					else if(pVictim->pev->takedamage == 0){
					pVictim->BarnacleVictimReleased();//ҧ����
					m_fLiftingPrey = FALSE;
					m_hTargetEnt = NULL;
					}
					else{
					pVictim->TakeDamage ( pev, pev, pVictim->pev->health, DMG_SLASH | DMG_ALWAYSGIB );
					m_cGibs = 3;
					}
				}

				return;
			}

			// bite prey every once in a while
			if ( pVictim )
			{
				pVictim->BarnacleVictimBitten( pev );
			}

		}
	}
	else
	{
// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.5,0.75);	// Stagger a bit to keep barnacles from thinking on the same frame

		if ( m_fSequenceFinished )
		{// this is done so barnacle will fidget.
			SetActivity ( ACT_IDLE );
			m_flTongueAdj = -100;
		}

		pTouchEnt = TongueTouchEnt( &flLength );

		if ( pTouchEnt != NULL && m_fTongueExtended )
		{
			if(pTouchEnt->pev->takedamage == DAMAGE_NO){
			goto failed;
			}

			if(pTouchEnt->pev->flags & FL_MONSTER ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pTouchEnt->MyMonsterPointer();
				if(pEnemyMonster->m_canbarnacle_mode == 0 || 
				pEnemyMonster->m_MonsterState == MONSTERSTATE_PRONE){

				if ( FClassnameIs(pev, "monster_barnacle_fantasy") 
				|| FClassnameIs(pev, "monster_barnacle_fantasy_r")){
				pTouchEnt->TakeDamage ( pev, pev, 1, DMG_SLASH | DMG_NEVERGIB );
				}

				goto failed;
				}
			}
			// tongue is fully extended, and is touching someone.
				if(pTouchEnt->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)pTouchEnt->pev);
					if(player->m_barnacle_RTP == 0 && player->m_barnacle_god_time <= gpGlobals->time){
						player->m_barnacle_RTP = 1;
						player->m_barnacle_Level = 1;
						player->m_MonsterCatchTime = gpGlobals->time;
						player->m_barnacle_catchme = this;
						player->pev->punchangle.x += RANDOM_FLOAT(-25, 25);
						player->pev->punchangle.y += RANDOM_FLOAT(-25, 25);
						player->pev->punchangle.z += RANDOM_FLOAT(-25, 25);
					}
					else{
						if ( FClassnameIs(pev, "monster_barnacle_fantasy")
						|| FClassnameIs(pev, "monster_barnacle_fantasy_r")){
						pTouchEnt->TakeDamage ( pev, pev, 1, DMG_SLASH | DMG_NEVERGIB );
						}

						goto failed;
					}
				}
				if ( pTouchEnt->FBecomeProne() )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_alert2.wav", 1, ATTN_NORM );	

					SetSequenceByName ( "attack1" );
					m_flTongueAdj = -20;

					m_hTargetEnt = pTouchEnt;
					if(pTouchEnt->pev->flags & FL_MONSTER ){
					pTouchEnt->pev->movetype = MOVETYPE_FLY;
					}
					pTouchEnt->pev->velocity = g_vecZero;
					pTouchEnt->pev->basevelocity = g_vecZero;
					pTouchEnt->pev->origin.x = pev->origin.x;
					pTouchEnt->pev->origin.y = pev->origin.y;

					//pTouchEnt->TakeDamage ( pev, pev, 10, DMG_SLASH | DMG_NEVERGIB );

					m_fLiftingPrey = TRUE;// indicate that we should be lifting prey.
					m_flKillVictimTime = -1;// set this to a bogus time while the victim is lifted.

					m_flAltitude = (pev->origin.z - pTouchEnt->EyePosition().z);
				}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it. 
			if ( m_flAltitude < flLength )
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += bps;
				m_fTongueExtended = FALSE;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = TRUE;
			}

		}
		failed:;
	}

	// ALERT( at_console, "tounge %f\n", m_flAltitude + m_flTongueAdj );
	SetBoneController( 0, -(m_flAltitude + m_flTongueAdj) );
	StudioFrameAdvance( 0.1 );
}

//=========================================================
// Killed.
//=========================================================
void CBarnacle :: Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die > 1)
	return;

	if(m_die == 0){
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
		if ( (pevAttacker->flags & FL_CLIENT) )//��һ�ɱ
		{
			if (pevAttacker){
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pevAttacker);
				if(pPlayer){
				pPlayer->TeamMate_expadd(NULL,this);
					if(m_killed_exp >= 1){
					g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
					}
				}
			}
		}
		else if ( (pevAttacker->flags & FL_MONSTER) && pEntity->Classify() == CLASS_PLAYER_ALLY )//�����ɱ������Ѿ�
		{
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			if(pEnemyMonster){
				if(pEnemyMonster->m_hPlayer != NULL && pEnemyMonster->m_lovehate > 0){

					if(pEnemyMonster->m_rpgms_type == 1 && 
					(pEnemyMonster->m_rpgms_inteam == 0 || pEnemyMonster->m_rpgms_inteam == 5) ){
					pEnemyMonster->m_rpgms_exp += m_killed_exp;
					pEnemyMonster->m_rpgms_maxexp += m_killed_exp;
							if(pEnemyMonster->m_rpgms_inteam == 5 && m_killed_exp >= 1){
							g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
							}
					}
					else if(pEnemyMonster->m_rpgms_inteam > 0){
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEnemyMonster->m_hPlayer->pev);
						if(pPlayer){
						pPlayer->TeamMate_expadd(pEnemyMonster,this);
							if(m_killed_exp >= 1){
							g_pGameRules->DeathNotice( this, pevAttacker, NULL,m_killedbydmg );
							}
						}
					}	
				}
			}
		}

		CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
		if ( pOwner )
		{
			pOwner->DeathNotice( pev );
		}
	}

	CBaseMonster *pVictim;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;
	m_die += 1;
	pev->flags = FL_NOTARGET;

	if(pev->movetype == MOVETYPE_BOUNCEMISSILE){
	pev->movetype = MOVETYPE_TOSS;
	}

	if ( m_hTargetEnt != NULL )
	{
		pVictim = m_hTargetEnt->MyMonsterPointer();

			if ( pVictim )
			{
				if ( pVictim->pev->deadflag != DEAD_NO ){
				pVictim->m_die = 2;
				}
				pVictim->BarnacleVictimReleased();
			}

				if(m_hTargetEnt->pev->flags & FL_CLIENT ){
					CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hTargetEnt->pev);
					if(player->m_barnacle_RTP != 0){
						player->m_barnacle_RTP_relase = 1;
						player->m_barnacle_draw_time = gpGlobals->time + 0.5;//0.5����ǹ���
						player->m_barnacle_RTP = 0;
						player->m_barnacle_RTP_bar = 0;
						player->m_barnacle_Level = 0;
						player->m_barnacle_catchme = NULL;
					}
				}
	}

	switch ( RANDOM_LONG ( 0, 1 ) )
	{
	case 0:	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_die1.wav", 1, ATTN_NORM );	break;
	case 1:	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barnacle/bcl_die3.wav", 1, ATTN_NORM );	break;
	}
	
	SetActivity ( ACT_DIESIMPLE );
	SetBoneController( 0, 0 );

	StudioFrameAdvance( 0.1 );

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink ( &CBarnacle::WaitTillDead );
}

//=========================================================
//=========================================================
void CBarnacle :: WaitTillDead ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	float flInterval = StudioFrameAdvance( 0.1 );
	DispatchAnimEvents ( flInterval );

	if ( m_fSequenceFinished )
	{
		// death anim finished. 
		StopAnimation();
		SetThink ( NULL );
		SUB_StartFadeOut();
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarnacle :: Precache()
{
	PRECACHE_MODEL("models/barnacle.mdl");
	PRECACHE_MODEL("models/barnacle_fantasy.mdl");

	PRECACHE_SOUND("barnacle/bcl_alert2.wav");//happy, lifting food up
	PRECACHE_SOUND("barnacle/bcl_bite3.wav");//just got food to mouth
	PRECACHE_SOUND("barnacle/bcl_die1.wav" );
	PRECACHE_SOUND("barnacle/bcl_die3.wav" );
}	

//=========================================================
// TongueTouchEnt - does a trace along the barnacle's tongue
// to see if any entity is touching it. Also stores the length
// of the trace in the int pointer provided.
//=========================================================

CBaseEntity *CBarnacle :: TongueTouchEnt ( float *pflLength )
{
	if(m_freezetime > 0){
	return NULL;//����
	}

	TraceResult	tr;
	float		length;

	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine ( pev->origin, pev->origin - Vector ( 0 , 0 , 2048 ), ignore_monsters, ENT(pev), &tr );
	length = fabs( pev->origin.z - tr.vecEndPos.z );
	if ( pflLength )
	{
		*pflLength = length;
	}

	Vector vecArmPos,vecArmDir;
    //GetAttachment( 1, vecArmPos, vecArmDir );
	//vecArmPos.z = pev->origin.z;
	vecArmPos = pev->origin;

	int check_spac = 15;
	if ( FClassnameIs(pev, "monster_barnacle_fantasy")
	|| FClassnameIs(pev, "monster_barnacle_fantasy_r")){
	   check_spac = 90;
	}

	Vector delta = Vector( check_spac, check_spac, 0 );
	Vector mins = vecArmPos - delta;
	Vector maxs = vecArmPos + delta;

	maxs.z = pev->origin.z;
	mins.z -= length;

	CBaseEntity *pList[10];
	int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, (FL_CLIENT|FL_MONSTER) );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			// only clients and monsters
			if ( pList[i] != this && IRelationship( pList[i] ) > R_NO 
			&& pList[ i ]->pev->movetype != MOVETYPE_FLY && pList[ i ]->pev->deadflag == DEAD_NO
			&& pList[ i ]->pev->movetype != MOVETYPE_NOCLIP )	// this ent is one of our enemies. Barnacle tries to eat it.
			{
				if(pList[i]->Classify() != CLASS_MACHINE && pList[i]->Classify() != CLASS_MACHINE_ASS
				&& pList[i]->Classify() != CLASS_MACHINE_BLACK){
				return pList[i];
				}
			}
		}
	}

	return NULL;
}

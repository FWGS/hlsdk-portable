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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"player.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CKadoma : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int  Classify ( void );

	void BarneyFirePistol( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	virtual int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	void EXPORT		FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


	int m_iTrail;

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_cleaner, CKadoma );
LINK_ENTITY_TO_CLASS( monster_cleaner2, CKadoma );
LINK_ENTITY_TO_CLASS( monster_kadoma, CKadoma );
LINK_ENTITY_TO_CLASS( monster_kadoma2, CKadoma );
LINK_ENTITY_TO_CLASS( monster_kadoma_arm, CKadoma );
LINK_ENTITY_TO_CLASS( monster_kadoma_arm2, CKadoma );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CKadoma :: Classify ( void )
{
	return	CLASS_PLAYER;
}


void CKadoma :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsAlive() && pCaller != NULL && pCaller->IsPlayer())
	{
		if(pev->team == 0 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) == 1999){
		pev->team = 1;

		pev->sequence = LookupSequence( "i_need_more_power" );
		ResetSequenceInfo( );
		pev->frame = 0;
		}
	}
}

void CKadoma :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->frags == 1){
	m_flGroundSpeed = 300;
	}
	if(pev->weapons == 2){
		if(pev->sequence != LookupActivity ( ACT_RUN_SCARED )){
		SetActivity( ACT_RUN_SCARED );
		}
	}
	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 45;
	}
	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 255;
	}
	if(pev->frags == 1){
	m_flGroundSpeed = 300;
	}

	if(pev->sequence == LookupSequence( "sword_jump_hitfly" )){
		TraceResult tr;
		UTIL_MakeVectors(pev->angles);
		Vector vecSrc	= Center();
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * -24;
		UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT( pev ), &tr );
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if ( pEntity && tr.flFraction < 1.0 ){
			if ( pEntity->pev->solid == SOLID_BSP){
			pev->velocity = Vector(0,0,-10);
			FX_Explosion( Center(), EXPLOSION_SPARKSHOWER );
			FX_Explosion( Center(), 238 );
			pev->sequence = LookupSequence( "sword_jump_hitfly_down" );
			ResetSequenceInfo( );
			pev->frame = 0;
			EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/bustconcrete2.wav", 1, 0.5); 
			}
		}
	}

	/*
			if(GetBodygroup( 2 ) == 4){//ս���ֵ�Ͳ
				if(!FBitSet(pev->effects, EF_DIMLIGHT)){
				SetBits(pev->effects, EF_DIMLIGHT);
				}
			}
			else{
				if(FBitSet(pev->effects, EF_DIMLIGHT)){
				ClearBits(pev->effects, EF_DIMLIGHT);
				}
			}
	*/
}

void CKadoma :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		// grunt is either shooting standing or shooting crouched
		if (pev->weapons == 4)
		{
			iSequence = LookupSequence( "run_flash" );
		}
		else if (pev->weapons == 2)
		{
			iSequence = LookupActivity ( ACT_RUN_SCARED );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		// grunt is either shooting standing or shooting crouched
		if (pev->weapons == 4)
		{
			iSequence = LookupSequence( "walk_flash" );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		// grunt is either shooting standing or shooting crouched
		if (pev->weapons == 1)
		{
			iSequence = LookupSequence( "dying_friendidle" );
		}
		else if (pev->weapons == 3)
		{
			iSequence = LookupActivity ( ACT_COMBAT_IDLE );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	m_IdealActivity = m_Activity;

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CKadoma :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CKadoma :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
			if ( GetBodygroup( 2 ) == 0 ){
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMFOLLOW );
				WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
				WRITE_SHORT(m_iTrail );	// model
				WRITE_BYTE( 3 ); // life
				WRITE_BYTE( 3 );  // width
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 255 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 188 );	// brightness
				MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)


				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity )//��Gman���İ�
				{
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					SetBodygroup( 2, 1 );
					pEnemyMonster->SetBodygroup( 2, 2 );
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_SMALL_FLINCH );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;

					Vector vecGunPos;
					Vector vecGunAngles;
					GetAttachment( 0, vecGunPos, vecGunAngles );

					UTIL_Sparks( vecGunPos );
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/struggle_hit.wav", 1, ATTN_NORM);
				}
			}
		}
		break;

	case 2:
		{//ժ�¿���
			if ( GetBodygroup( 1 ) == 1 ){
				SetBodygroup( 1, 0 );
			}
		}
		break;

	case 3:
		{//�ķ�ֹͣ
			if ( pev->frags == 3  ){
				pev->framerate = 0;
			}
		}
		break;

	case 4:
		{//ˮ�в��Ƽ���
		TraceResult tr;
		UTIL_TraceLine(Center(), pev->origin, ignore_monsters, ENT(pev), &tr);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, Center(), 0, 114, 0 );
		}
		break;
	case 5:
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss" );
				if ( pEntity )//�Ա����ع�
				{
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_DIE_HEADSHOT );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetBodygroup( 0, 1 );
						SpawnBlood(pEnemyMonster->EyePosition(), BLOOD_COLOR_RED, 200);
						FX_Explosion( pEnemyMonster->EyePosition(), 238 );
						pev->velocity.y = -100;
						pev->velocity.z = 50;
						pEntity->pev->velocity.y = 100;
						pEntity->pev->velocity.z = 50;
						EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/dbarrel1.wav", 1, ATTN_NORM );
				}
		}
		break;
	case 6:
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * 520;
			pev->velocity.z += 260;
		break;
	case 7:
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_human_grunt" );
				if ( pEntity )//��Hecu�ع�
				{
					
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();

					if(pEntity->pev->health == pEntity->pev->max_health){
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_SMALL_FLINCH );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_boltpoison = 100;
						pEntity->pev->yaw_speed = 0;
						pEnemyMonster->m_killed_exp = 0;
						pEnemyMonster->m_undropgun		= TRUE;
					}

					SpawnBlood(pEnemyMonster->EyePosition(), BLOOD_COLOR_RED, 30);
					FX_Explosion( pEnemyMonster->EyePosition(), 232 );

					pEnemyMonster->TakeDamage ( pev, pev, pEntity->pev->max_health * 0.35, DMG_GENERIC );

					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM );
				}
		}
		break;

	case 8:
		BarneyFirePistol();
		break;

	case 9://�γ�ʥ��
		{
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "valvesword_cycler" );
			if ( pEntity ){
				FX_Explosion( pEntity->pev->origin, EXPLOSION_LIGHTSABER );
				SetBodygroup( 2, 7 );
				UTIL_Remove( pEntity );
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/valvesword_hit2.wav", 1, ATTN_NORM); 
			}
		}
		break;

	case 10://ʥ����
		{
			SetBits(pev->effects, EF_DIMLIGHT);
			FX_Explosion( Center(), 42);

				CBaseEntity *pEntity = Create( "monster_kadoma2", pev->origin, pev->angles, NULL );
				SET_MODEL(ENT(pEntity->pev), "models/kadoma_sword.mdl");
				pEntity->pev->movetype = MOVETYPE_FOLLOW;
				pEntity->pev->aiment = edict();
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEntity->pev->renderfx = kRenderFxGlowShell;
				pEntity->pev->rendercolor.x = 255;
				pEntity->pev->rendercolor.y = 192;
				pEntity->pev->rendermode = kRenderNormal;
				pEntity->pev->renderamt = 48;
		}
		break;

	case 11://�ӽ���β
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
			WRITE_SHORT(m_iTrail );	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 1 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 188 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

	case 12://�ӽ�����
		{
				pev->angles.y = 270;

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity )//��Ұ���ͱ��ع�
				{
					UTIL_SetOrigin( pev, pev->origin + Vector(0,120,0) );

					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();

					if(pEntity->pev->health == pEntity->pev->max_health){
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_DIESIMPLE );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					}

					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/valvesword_hit2.wav", 1, ATTN_NORM); 
					FX_Explosion( pEnemyMonster->Center(), EXPLOSION_LIGHTSABER );

				}
		}
		break;

	case 13://��doma���ɣ��Ⱥ���
		{
				FX_Explosion(Center(), 47 );
				SpawnBlood(Center(), BloodColor(), 250);
				SetBodygroup( 0, 2 );
				SetBodygroup( 1, 3 );
				pev->velocity.x = 300;
				pev->velocity.y = 300;
				pev->velocity.z = 60;
				EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
		}
		break;

	case 14://kadoma����
		{
			pev->movetype = MOVETYPE_TOSS;
		}
		break;

	case 15://��������
		{
			FX_Explosion( Center(), EXPLOSION_LIGHTSABER );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/valvesword_hit2.wav", 1, ATTN_NORM); 
		}
		break;

	case 16://�ͳ�������
		{
			SetBodygroup( 2, 8 );
		}
		break;

	case 17://ʹ�ô�����
		{
			FX_Explosion(Center(), EXPLOSION_DISPTELEPORT );

			CBasePlayer *pPlayer;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity ){
			pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
			}

			if(pPlayer){
				pPlayer->MenuItem_remove(22);
				if (pPlayer->m_team_npc1 != NULL){
				pPlayer->m_team_npc1->pev->effects |= EF_NODRAW;
				}
				if (pPlayer->m_team_npc2 != NULL){
				pPlayer->m_team_npc2->pev->effects |= EF_NODRAW;
				}
				if (pPlayer->m_team_npc3 != NULL){
				pPlayer->m_team_npc3->pev->effects |= EF_NODRAW;
				}
				if (pPlayer->m_team_npc4 != NULL){
				pPlayer->m_team_npc4->pev->effects |= EF_NODRAW;
				}
				UTIL_Remove( this );
			}

		}
		break;

	case 18://����
		{
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= pev->origin + Vector(0,0,256);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 60;

			CBaseEntity *pEntity = Create( "wrongdoor_kadoma_moon", vecEnd, pev->angles, edict() );
			if(pev->impulse != 99){
			SetBodygroup( 1, 5 );
			}
			else{
			SetBodygroup( 1, 2 );
			}
			//SetBits(pev->effects, EF_BRIGHTLIGHT);
		}
		break;

	case 19://����
		{
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= pev->origin + Vector(0,0,66);
			if(pev->impulse == 99){//���ͱ��Ƕȸ���
			vecSrc	= pev->origin + Vector(0,0,70);
			}
			Vector vecEnd	= vecSrc + gpGlobals->v_right * -2 + gpGlobals->v_forward * 0.5;

			CBaseEntity *pEntity2 = Create( "wrongdoor_kadoma_camera", vecEnd, pev->angles, edict() );
			pEntity2->pev->angles.x = -90;

			CBasePlayer *pPlayer;
			CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity3 ){
			pPlayer = GetClassPtr((CBasePlayer *)pEntity3->pev);
			SET_VIEW( pPlayer->edict(), pEntity2->edict() );
			pPlayer->m_player_camera = pEntity2;
			pPlayer->EnableControl(FALSE);
			pPlayer->pev->v_angle = pEntity2->pev->angles;
			pPlayer->pev->angles = pEntity2->pev->angles;
			pPlayer->pev->fixangle = TRUE;
			m_rpgms_level = pPlayer->m_kadoma_level;
			}
		}
		break;

	case 20:
		{
			if(pev->impulse != 99){
			char text[256];
			sprintf( text, "- I need more power\n");
			UTIL_SayTextAll( text,this );
			}
		}
		break;

	case 21://����
		{
			if(pev->impulse >= 99){
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "wrongdoor_kadoma_moon" );
				if ( pEntity3 ){
				FX_Explosion( pEntity3->Center(), EXPLOSION_CHRONOCLIP );
				EMIT_SOUND_DYN( ENT(pEntity3->pev), CHAN_STREAM, "weapons/chronoclip_explode.wav", 1, 0.4, 0, 100);
				UTIL_Remove( pEntity3 );
				}
			}
		}
		break;

	case 22://�ָ�
		{
			if(pev->impulse < 99){
				CBasePlayer *pPlayer;
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "player" );
				if ( pEntity3 ){
				pPlayer = GetClassPtr((CBasePlayer *)pEntity3->pev);
				pPlayer->EnableControl(TRUE);
				pPlayer->Clear_SayText();
				pPlayer->m_trainning = 0;
				pPlayer->pev->origin = pev->origin + Vector(0,0,36);
				}
				UTIL_Remove( this );
			}
		}
		break;

	case 23://�ӽ�����2
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_sicker_zz" );
			if ( pEntity )//߱��
			{
				FX_Explosion( pEntity->Center(), 137 );
				UTIL_Remove( pEntity );
			}
		}
		break;

	case 24://�ӽ�����3
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity )//��Gman�ع�
				{	
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_DIESIMPLE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );

					SpawnBlood(pEntity->pev->origin + Vector(0,0,8), BLOOD_COLOR_RED, 200);
					FX_Explosion( pEntity->pev->origin + Vector(0,0,16), EXPLOSION_LIGHTSABER );
				}
		}
		break;

	case 25://����
		{
			FX_Trail(pev->origin, entindex(), 151);
		}
		break;

	case 26://��β��ȭ
		{
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMFOLLOW );
				WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
				WRITE_SHORT(m_iTrail );	// model
				WRITE_BYTE( 8 ); // life
				WRITE_BYTE( 4 );  // width
				WRITE_BYTE( 188 );	// R
				WRITE_BYTE( 188 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 188 );	// brightness
				MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

	case 27://��ɱDoma
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity )//��Doma�ع�
			{	
				FX_Explosion( pEntity->Center(), 107 );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->GibMonster();
			}
		}
		break;
	case 28:
		{//�ϱ�! or ��Ѫ!
			if(FClassnameIs( pev, "monster_kadoma_arm") || FClassnameIs( pev, "monster_kadoma_arm2") ){
			SetBodygroup( 0, 1 );
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion( vecGunPos, 47 );
			SpawnBlood(vecGunPos, BloodColor(), 200);
			FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
			EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
			}
			else{
			FX_Explosion(Center(), 236 );
			}
		}
		break;
	case 29:
		{//�ϱ�����!
			FX_Trail(pev->origin, entindex(), 151);
			EMIT_SOUND(ENT(pev), CHAN_BODY, "willam/danmu_start.wav", 1, 0.6);
		}
		break;
	case 30:
		{//�ϱ�����!
			FX_Trail( pev->origin, entindex(), PROJ_REMOVE );

			SetBits(pev->effects, EF_DIMLIGHT);

			EMIT_SOUND(ENT(pev), CHAN_BODY, "willam/superarmor.wav", 1, 0.6);	

			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion( vecGunPos, 52);

			CBaseEntity *pEntity = Create( "monster_kadoma2", pev->origin, pev->angles, NULL );
			SET_MODEL(ENT(pEntity->pev), "models/kadoma_arm.mdl");
			pEntity->pev->movetype = MOVETYPE_FOLLOW;
			pEntity->pev->aiment = edict();
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->SetBodygroup( 0, 2 );
			pEnemyMonster->SetBodygroup( 1, 2 );
			pEntity->pev->renderfx = kRenderFxGlowShell;
			pEntity->pev->rendercolor.x = 192;
			pEntity->pev->rendercolor.y = 128;
			pEntity->pev->rendercolor.z = 255;
			pEntity->pev->rendermode = kRenderNormal;
			pEntity->pev->renderamt = 2;
		}
		break;
	case 31://��ɱZ����
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_zdeadeye" );
				if ( pEntity )//�������ع�
				{	
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/gluongun_fire.wav", 1, ATTN_NORM);

					TraceResult tr;

					Vector vecGunPos,vecGunAngles;
					GetAttachment( 0, vecGunPos, vecGunAngles );
					UTIL_TraceLine(vecGunPos, pEntity->Center(), dont_ignore_monsters, edict(), &tr);
					FireBeam(vecGunPos, pEntity->Center(), 24, 623, pev);

					FX_Explosion( pEntity->Center(), 107 );
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEntity->SUB_StartFadeOut3();
					pEntity->pev->movetype = MOVETYPE_TOSS;
				}
		}
		break;

	case 32:
		{
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion(vecGunPos, 46 );
		}
		break;

	case 33:
		{//����֮��
			FX_Trail(pev->origin, entindex(), PROJ_SUNOFGOD2);
		}
		break;

	case 34:
		{//����֮�⡤����
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion(vecGunPos, 45 );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_LARGEFUNNEL );
			WRITE_COORD( vecGunPos.x );
			WRITE_COORD( vecGunPos.y );
			WRITE_COORD( vecGunPos.z );
			WRITE_SHORT( g_sModelIndexFlareGlow );
			WRITE_SHORT( 1 );
			MESSAGE_END();

			FX_Trail(pev->origin, entindex(), PROJ_REMOVE);
		}
		break;

	case 35://S��ȭ����
		{
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Explosion( vecGunPos, 107 );
		}
		break;

	case 36://�Ƶ�Misaliya
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity )
				{	
					FX_Explosion( pEntity->Center(), 133 );
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/fist_explode.wav", 1, ATTN_NORM);

					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEntity->pev->flags &= ~FL_ONGROUND;
					pEntity->pev->movetype = MOVETYPE_FLY;
					pEntity->pev->velocity.x = 600;

					//ñ�Ӥε���
					CBaseEntity *pEntity2 = Create( "monster_generic_item2", pEntity->pev->origin + Vector(0,0,72), Vector(0,0,0), NULL );
					SET_MODEL(ENT(pEntity2->pev), "models/props_all.mdl");
					pEntity2->pev->body = 9;
					pEntity2->pev->velocity.x = 30;
					pEntity2->pev->velocity.z = 50;
					pEnemyMonster->SetBodygroup( 8, 1 );

					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "s_ending_seq2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
				}
		}
		break;

	case 37://��β����
		{
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMFOLLOW );
				WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
				WRITE_SHORT(m_iTrail );	// model
				WRITE_BYTE( 8 ); // life
				WRITE_BYTE( 4 );  // width
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 255 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 188 );	// brightness
				MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
		}
		break;

	case 38:
		{//�ӵ��⻷
			FX_Explosion( Center(), 127);
			SetBodygroup( 1, 3 );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM, "newadd/exp2_frost.wav", 1.0, 0.6, 0, 100);

			//�⻷ʵ��ε���
			CBaseEntity *pEntity = Create( "monster_generic_item2", pev->origin + Vector(0,0,72), Vector(0,0,0), NULL );
			SET_MODEL(ENT(pEntity->pev), "models/props_all.mdl");
			pEntity->pev->body = 10;
			pEntity->pev->velocity.y = 150;
			pEntity->pev->velocity.z = 30;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CKadoma :: BarneyFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);

	vecShootOrigin = pev->origin + Vector( 0, 0, 65 );
	
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	pev->effects = EF_MUZZLEFLASH;

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	

	int iBulletType;
	iBulletType = BULLET_12MM;

	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.0,0.0,0.0), 2048, iBulletType,1);

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CKadoma :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CKadoma :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/kadoma.mdl");

	if(FClassnameIs( pev, "monster_kadoma_arm") || FClassnameIs( pev, "monster_kadoma_arm2") ){
	SET_MODEL(ENT(pev), "models/kadoma_arm.mdl");
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 100;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	SetUse( &CKadoma::FollowerUse2 );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CKadoma :: Precache()
{
	PRECACHE_MODEL("models/kadoma.mdl");
	PRECACHE_MODEL("models/kadoma_sword.mdl");
	PRECACHE_MODEL("models/kadoma_arm.mdl");
	PRECACHE_MODEL("models/kadoma_ending.mdl");
	
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
	PRECACHE_MODEL("sprites/moon.spr");

	PRECACHE_SOUND("willam/danmu_start.wav");
	PRECACHE_SOUND("willam/superarmor.wav");
	PRECACHE_SOUND("newadd/exp2_frost.wav");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

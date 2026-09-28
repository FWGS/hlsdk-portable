#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"
#include "game.h"
#include "player.h"
#include "shake.h"
#include "soundent.h"

extern DLL_GLOBAL int		g_restore_fix;
extern int gmsgTbutton;
extern int gmsgItemPickup;
extern int gmsgGunScope;
extern int game_boss_battle;

extern DLL_GLOBAL int			g_fGameJumpCG;
extern DLL_GLOBAL int			g_fGameSkipCG;
//=========================================================
// ��CG�¼���ͳ��2
//=========================================================
class CMain_Event2 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
};

LINK_ENTITY_TO_CLASS( main_cg_event_new2, CMain_Event2 );//����2

void CMain_Event2::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event2::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��2=================================================//
void CMain_Event2::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

		if(pev->armortype == 15){//�¼�15 ����
			if(pev->frags == 0){
						pPlayer->m_trainning = 1;
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->pev->effects = 0;

						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 10, 10, 255, FFADE_IN );

						CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "player_tel_jail" );
						if ( pEntity2 ){
						pPlayer->pev->origin = pEntity2->pev->origin;
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						UTIL_Remove( pEntity2 );
						}
			}
			if(pev->frags == 20){
					pev->frags = 80;
			}
			if(pev->frags == 100){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->movetype = MOVETYPE_TOSS;
						pSpot->pev->angles.x = -60;
					}
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pSpot->pev->origin + Vector(-30,0,0), Vector(0,180,0), NULL );
					pCleaner1->pev->movetype = MOVETYPE_TOSS;
					UTIL_SetSize(pCleaner1->pev, Vector( 0, 0, 0), Vector(0, 0, 0));
			}
			if(pev->frags == 105){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;//������������ֹͣ��˼��
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "cprbarney" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->pev->framerate = 0;
				pEnemyMonster->SetBodygroup( 1, 5 );
				pEnemyMonster->SetThink( NULL );
				}
			}
			if(pev->frags == 110){
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();

						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->Clear_SayText();
						}
			}
			if(pev->frags == 130){
			SERVER_COMMAND("mp3 play media/music9.mp3\n");
			}
			if(pev->frags == 160){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->movetype = MOVETYPE_NOCLIP;
						pSpot->pev->avelocity.x = 30;
						pSpot->pev->velocity.z = 8;
						pSpot->pev->armortype = 6;
					}
			}
			if(pev->frags == 200){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						UTIL_SetSize(pev, Vector( 0, 0, -4), Vector(0, 0, 4));
						pSpot->pev->avelocity.x = -100;
						pSpot->pev->armortype = 0;
						pSpot->pev->gravity = 0.1;
						pSpot->pev->movetype = MOVETYPE_TOSS;
					}
			}
			if(pev->frags == 220){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->avelocity.x = 0;
					}
			}
			if(pev->frags == 240){
					FireTargets( "player_bed", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 300){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick" );
				if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP33", VOL_NORM, 0.5, 0, PITCH_NORM );
					char text[256];
			
					sprintf( text, "???: You look awful.\n");
				
					UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 340){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pSpot->pev->movetype = MOVETYPE_NOCLIP;
							pSpot->pev->avelocity.x = 20;
							pSpot->pev->avelocity.z = 40;
							pSpot->pev->avelocity.y = -40;
							pSpot->pev->velocity.z = 2;
							pPlayer->Clear_SayText();
						}
			}
			if(pev->frags == 360){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->avelocity.x = 0;
						pSpot->pev->avelocity.z = 0;
						pSpot->pev->avelocity.y = 0;
						pSpot->pev->velocity.z = 0;
					}
			}
			if(pev->frags == 375){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity ){
					pEntity->pev->effects = EF_NODRAW;
					}
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->angles = Vector(0,90,0);
						pSpot->pev->origin = pSpot->pev->origin + Vector(-32,-24,16);
					}
			}
			if(pev->frags == 380){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP1", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
					
						sprintf( text, "Scientist: You have no idea what's going on, do you?\n");
					
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 450){
						pPlayer->Clear_SayText();
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4, 6, 255, FFADE_IN );//��Ϲ���
						pPlayer->m_iClient_Gameover = 2;
						pPlayer->m_fGameOverTime = gpGlobals->time + 5;
			}
			if(pev->frags == 480){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->m_duckseq = 2;
				pEnemyMonster->m_ignoredamage = 1;
				pEnemyMonster->pev->health = 80000;
				}
			}
			if(pev->frags == 490){
						pPlayer->EnableControl(FALSE);

						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = Vector(0,0,0);
							pSpot->pev->angles.x = -30;
							pSpot->pev->avelocity.x = 6;
							pSpot->pev->velocity.y = -47;
							pSpot->pev->velocity.z = -5;
						}
			}
			if(pev->frags == 590){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->frags = 1;
						pSpot->pev->armortype = 6;
					}
			}
			if(pev->frags == 660){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
					if ( pEntity )
					{
						pEntity->pev->flags |= FL_NOTARGET;
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pSpot->pev->origin = pEntity->pev->origin + Vector(0,-72,18);
							pSpot->pev->angles.x = 0;
							pSpot->pev->angles.y = 90;
							pSpot->pev->avelocity = g_vecZero;
							pSpot->pev->velocity = g_vecZero;
						}
					}
			}
			if(pev->frags == 675){
					FireTargets( "hecu_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 680){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_human_grunt" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->skin = 0;
				pEnemyMonster->pev->body = 0;
				pEnemyMonster->m_cClipSize = 50;
				pEnemyMonster->m_cAmmoLoaded = pEnemyMonster->m_cClipSize;
				}
			}
			if(pev->frags == 740){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 2, 3 );
				}
			}
			if(pev->frags == 785){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
					if ( pEntity )
					{
						pEntity->pev->flags &= ~FL_NOTARGET;
					}
			}
			if(pev->frags == 790){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_human_grunt" );
					if ( pEntity )
					{
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pSpot->pev->origin = pEntity->pev->origin + Vector(-120,0,40);
							pSpot->pev->angles.y = 0;
							pSpot->pev->velocity.y = -30;
							pSpot->pev->frags = 0;
							pSpot->pev->armortype = 0;
						}
					}
			}
			if(pev->frags == 810){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->frags = 1;
						pSpot->pev->armortype = 6;
					}
			}
			if(pev->frags == 860){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_4" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;
				pEnemyMonster->SetThink( NULL );
				}
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->frags = 0;
					pSpot->pev->armortype = 0;
					pSpot->pev->angles.y = 270;
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->avelocity.x = 4;
					pSpot->pev->velocity.z = 4;
					pSpot->pev->origin = pEntity->pev->origin + Vector(0,36,18);
				}
				CBaseEntity *pSpot2 = UTIL_FindEntityByTargetname( NULL, "gman_kabang");
				if ( pSpot2 ){
					pSpot2->pev->origin.y -= 16;
				}
			}
			if(pev->frags == 890){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->avelocity.x = 0;
						pSpot->pev->velocity.z = 0;

						pSpot->pev->angles.x = 90;
						pSpot->pev->velocity.y = -10;
						pSpot->pev->origin = pEntity->pev->origin + Vector(0,48,64);
					}
				}
			}
			if(pev->frags == 960){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->frags = 1;
					pSpot->pev->armortype = 6;
				}
			}
			if(pev->frags == 990){
				CBaseEntity *pSpot2 = UTIL_FindEntityByTargetname( NULL, "gman_kabang");
				if ( pSpot2 ){
				pSpot2->pev->body = 1;
				EMIT_SOUND_DYN( ENT(pSpot2->pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
				}
			}
			if(pev->frags == 1020){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "gman_kabang");
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pEntity ){
					if ( pSpot ){
						pSpot->pev->frags = 0;
						pSpot->pev->armortype = 0;
						pSpot->pev->avelocity = g_vecZero;
						pSpot->pev->velocity = g_vecZero;
						pSpot->pev->angles.x = 0;
						pSpot->pev->angles.y = 90;

						pSpot->pev->origin = pEntity->pev->origin + Vector(0,0,8);
					}
				}
			}
			if(pev->frags == 1040){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_5" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 1060){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "gman_kabang");
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pEntity ){
					if ( pSpot ){
						pSpot->pev->frags = 0;
						pSpot->pev->armortype = 0;
						pSpot->pev->avelocity = g_vecZero;
						pSpot->pev->velocity = g_vecZero;
						pSpot->pev->angles.x = 60;
						pSpot->pev->angles.y = 270;

						pSpot->pev->origin = pEntity->pev->origin + Vector(0,6,18);
					}
				}
			}
			if(pev->frags == 1070){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_6" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			
			if(pev->frags == 1095){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}
			}
			if(pev->frags == 1100){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->movetype = MOVETYPE_NOCLIP;
						pSpot->pev->velocity.y = 400;
						SetBits(pSpot->pev->effects, EF_BRIGHTLIGHT);
					}
			}
			if(pev->frags == 1105){
					FireTargets( "cg_door1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1116){
					FireTargets( "cg_door2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1129){
					FireTargets( "cg_door3", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1135){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "cower_stand" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 1, 2 );
				pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;//ֹͣ��˼��
				pEnemyMonster->SetThink( NULL );
				SetBits( pEntity->pev->effects, EF_DIMLIGHT);

					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "tel_kadoma_cg");
					if ( pSpot ){
						UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin );
						pEntity->pev->angles.y = 270;
					}
				}
			}
			if(pev->frags == 1140){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->frags = 1;
						pSpot->pev->armortype = 6;
						pSpot->pev->velocity.z = -10;
					}
			}
			if(pev->frags == 1142){
					FireTargets( "cg_door4", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1180){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner2" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "headache" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 1195){
				pPlayer->Clear_SayText();
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );//��Ϲ���
			}

			if(pev->frags == 1220){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;//������������ֹͣ��˼��
				pEnemyMonster->SetThink( NULL );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "console_sitting" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 1, 0 );
				pEntity->pev->effects = 0;

					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "kadoma_sit_point");
					if ( pSpot ){
						UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin );
						pEntity->pev->angles.y = 90;
					}
				}
			}
			if(pev->frags == 1225){
						pPlayer->Clear_SayText();
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 1230){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pSpot->pev->angles.x = 5;
						}
			}
			if(pev->frags == 1255){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_IDLE6", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
					
						sprintf( text, "Scientist: What's with all this mess?\n");
					
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 1275){
			FireTargets( "gman_walkto1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1295){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_OK1", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
				
						sprintf( text, "Scientist: I really hope you know what you're doing.\n");
						
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 1330){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
					if ( pEntity )
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->SetBodygroup( 1, 1 );
						pEnemyMonster->SetBodygroup( 2, 2 );
					}
			}
			if(pev->frags == 1345){
					pPlayer->Clear_SayText();
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick" );
					if ( pEntity ){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
							pSpot->pev->angles.x = 0;
							pSpot->pev->angles.y = 0;
							pSpot->pev->origin.x = pEntity->pev->origin.x + 36;
							pSpot->pev->origin.y = pEntity->pev->origin.y + 72;
							pEntity->pev->origin.x -= 40;
							pEntity->pev->origin.y += 20;
							pEntity->pev->angles.y = 280;
						}
					}
			}
			if(pev->frags == 1430){
			FireTargets( "gman_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1450){
			FireTargets( "gman_walkto2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1525){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity ){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
							pSpot->pev->angles.x = 20;
							pSpot->pev->angles.y = 90;
							pSpot->pev->origin.x = pEntity->pev->origin.x;
							pSpot->pev->origin.y = pEntity->pev->origin.y - 40;
							pSpot->pev->origin.z = pEntity->pev->origin.z + 80;
							pSpot->pev->velocity.y = -2;
							pSpot->pev->velocity.z = 2;
						}
					}
			}
			if(pev->frags == 1555){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
					if ( pEntity )
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "teleport_use" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 1570){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 2.0, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 1585){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 3.0, 255, FFADE_IN );//��Ϲ���
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->pev->angles = Vector(0,270,0);
						pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 1595){
							CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "player_tel_jail" );
							if ( pEntity2 ){
							pPlayer->pev->origin = pEntity2->pev->origin;
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							UTIL_Remove( pEntity2 );
							}
			}
			if(pev->frags == 1610){
					FireTargets( "bar_walktalk1", this, this, USE_TOGGLE, 0 );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_level_up_switch = FALSE;
						pPlayer->pev->armorvalue = 0;
						pPlayer->m_skill_maxarmor = 0;
						pPlayer->m_iClientBattery = -1;
						pPlayer->m_fDeadRespawn = FALSE;
						pPlayer->m_hasflashlight = FALSE;
						pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
						pPlayer->m_flVelocityModifier = -2;
						pPlayer->m_trainning = 0;
						pPlayer->m_fPlayerHideMode = TRUE;
						pPlayer->pev->flags |= FL_NOTARGET;
						pPlayer->m_needleheal2 += pPlayer->pev->max_health;
						pPlayer->m_game_rate = 25;//��Ϸ����25%
			}
			if(pev->frags == 1640){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1660){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 1670){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "first_bar" );
					if ( pEntity ){
						pEntity->pev->spawnflags = SF_MONSTER_GAG;
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP1", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
					
						sprintf( text, "Guard: I'm sorry. The boss says you can't leave.\n");
					
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 1711){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 1713){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "first_bar" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP2", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
						
						sprintf( text, "Guard: The army will handle things here on out.\n");
					
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 1763){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 1800){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "first_bar" );
				if ( pEntity ){
				pEntity->pev->health = 80000;//��ʱ�޵�
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_boltpoison = 40;
				pEnemyMonster->m_alert = 100;
				pEnemyMonster->m_hPlayer = pPlayer;
				pEnemyMonster->m_rpgms_inteam = 5;
				UTIL_ScreenShake( pEntity->pev->origin, 12.0, 100.0, 2.0, 1000 );
				}
			}
			if(pev->frags == 1830){
			FireTargets( "hound1_spawn", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1900){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "first_bar" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP3", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
				
						sprintf( text, "Guard: That was weird.\n");
					
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 1940){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 1950){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "first_bar" );
					if ( pEntity ){
					pEntity->pev->health = 20;//biss
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					//pEnemyMonster->m_boltpoison = 25;
					pEnemyMonster->m_boltpoison = 50;//��ɱͷз Bug Fix 1.0
					pEntity->pev->takedamage = DAMAGE_YES;//��֤�BUG
					pEnemyMonster->m_godmode = FALSE;
					}
			}
			if(pev->frags == 1960){
			FireTargets( "crab_spawn1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 2060){
			FireTargets( "zombar_walk", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 2070){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zombie_break_door" );
					if ( pEntity ){
					pEntity->pev->takedamage = DAMAGE_YES;
					}
			}
			if(pev->frags == 2080){
					pPlayer->m_fPlayerHideMode = FALSE;
					pPlayer->pev->flags &= ~FL_NOTARGET;
			}
			if(pev->frags == 2100){
					pPlayer->GiveNamedItem( "weapon_fist" );
			}
			if(pev->frags == 2120){
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 16){//�¼�16 ˹����2���
			if(pev->frags == 0){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick2" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP3", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
						
						sprintf( text, "Scientist: I'm saved! Thank God you're here.\n");
						
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 20){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick2" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();

					pEnemyMonster->m_groundElev2 = TRUE;//������ҵĸ����Ż�����
					pEnemyMonster->m_lovehate += 20;//����Һøж�����
					pEnemyMonster->m_MoveFail_SimpleRoad = TRUE;

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "player" );
					if ( pEntity2 ){
					pEntity->pev->health = 50;
					pEntity->pev->max_health = 100;
					pEnemyMonster->m_rpgms_level = 4;
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->m_hTargetEnt = pEntity2;
					}
					pEnemyMonster->ClearSchedule();
					}
			}
			if(pev->frags == 40){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick2" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP4", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
						
						sprintf( text, "Scientist: You'll need me to operate the retinal scanner.\n");
						
						UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 80){
					pPlayer->Clear_SayText();
					pPlayer->m_game_rate = 26;//��Ϸ����26%
			}
			if(pev->frags == 90){
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 17){//�¼�17 ��ʬ����ս
		if(pev->frags == 0){
		FireTargets( "zombie_spawner1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1){
		SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 5){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_slick2" );
				if ( pEntity ){//˹����2��ʧ
					UTIL_Remove( pEntity );
				}
		}
		if(pev->frags == 10){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zbarney" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_alert = 100;
				pEnemyMonster->m_guard_mode = TRUE;
				pEnemyMonster->m_MoveFail_SimpleRoad = TRUE;
				pEnemyMonster->m_no_pov_limit = 1;
				pEnemyMonster->pev->health = 100;//Ѫ��������΢���
				pEnemyMonster->pev->max_health = 100;
				}
		}
		if(pev->frags == 25){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zbarney" );
				if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP4", VOL_NORM, 0.5, 0, PITCH_NORM );
					char text[256];
					
					sprintf( text, "Guard: What the hell is going on?\n");
				
					UTIL_SayTextAll( text,this );
				}
		}
		if(pev->frags == 45){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zbarney" );
				if ( pEntity ){
					char text[256];
				
					sprintf( text, "Guard: Why are they all wearing science uniforms?\n");
				
					UTIL_SayTextAll( text,this );
				}
		}
		if(pev->frags == 60){
		FireTargets( "crab_ceil1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 75){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zbarney" );
				if ( pEntity ){
				pEntity->pev->spawnflags = 0;
				}
		}
		if(pev->frags == 85){
				pPlayer->Clear_SayText();
				pPlayer->m_game_rate = 27;//��Ϸ����27%
		}
		if(pev->frags == 100){
				UTIL_Remove( this );
				return;
		}
	}
	else if(pev->armortype == 18){//�¼�18 û�Ű�������
			if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
				FireTargets( "bullsquid_maker1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 2){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zbarney" );
				if ( pEntity ){//��ʬ������ʧ
					UTIL_Remove( pEntity );
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "hurt_sit_sci" );
				if ( pEntity2 ){//���˿�ѧ����ʧ
					UTIL_Remove( pEntity2 );
				}
			}
			if(pev->frags == 3){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP5", VOL_NORM, 0.5, 0, PITCH_NORM );
					char text[256];
					
					sprintf( text, "Guard: Hey, you need to go up!\n");
				
					UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 20){
				FireTargets( "islave_maker7", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 27){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
					if ( pEntity ){
					char text[256];
				
					sprintf( text, "Guard: This way is completely blocked.\n");
					
					UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 30){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();

					pEntity->pev->spawnflags = 0;
					pEnemyMonster->m_selfmode = FALSE;
					pEnemyMonster->m_longming = 0;
					pEnemyMonster->m_EyeMod = 1;

					pEnemyMonster->m_godmode = FALSE;
					pEnemyMonster->m_iTriggerCondition = 4;
					pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("game_over_event");

					pEntity->pev->health = 120;
					pEntity->pev->max_health = 120;

					pEnemyMonster->m_MoveFail_SimpleRoad = TRUE;

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "player" );
					if ( pEntity2 ){
					pEnemyMonster->m_hTargetEnt = pEntity2;
					pEnemyMonster->m_rpgms_level = 6;
					pPlayer->TeamMate_add(pEnemyMonster);
					}
					pEnemyMonster->ClearSchedule();
					}
			}
			if(pev->frags == 65){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 70){
				pPlayer->m_game_rate = 28;//��Ϸ����28%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 19){//�¼�19 �����������
			if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 2){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
					if ( pEntity ){
					pEntity->pev->solid = SOLID_SLIDEBOX;
					UTIL_SetSize(pEntity->pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
					}
			}
			if(pev->frags == 3){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_pain3.wav", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
			}
			if(pev->frags == 9){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_pain1.wav", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
			}
			if(pev->frags == 15){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_pain1.wav", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
			}
			if(pev->frags == 21){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_pain2.wav", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
			}
			if(pev->frags == 27){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_pain2.wav", VOL_NORM, 0.5, 0, PITCH_NORM + 10 );
					}
			}
			if(pev->frags == 33){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_pain2.wav", VOL_NORM, 0.5, 0, PITCH_NORM + 10 );
					}
			}
			if(pev->frags == 39){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "pain_dying_bar" );
					if ( pEntity ){
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "barney/ba_die1.wav", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
			}
			if(pev->frags == 63){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP6", VOL_NORM, 0.6, 0, PITCH_NORM );
						char text[256];
						
						sprintf( text, "Guard: Wow it stinks here.\n");
					
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 103){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 114){
					pPlayer->m_game_rate = 30;//��Ϸ����30%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 20){//�¼�20 ���Ӱ�������
			if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 5){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
					if ( pEntity ){//û�Ű�����ʧ
					UTIL_Remove( pEntity );
					}
					//�������
					pPlayer->TeamMate_Nagamatagi_Allclear(0);
			}
			if(pev->frags == 10){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "ladder_barney" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						pEntity->pev->spawnflags = SF_MONSTER_GAG;
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP8", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
					
						sprintf( text, "Guard: The rescue may be underway, but I don't want to wait.\n");
						
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 40){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "ladder_barney" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						char text[256];
					
						sprintf( text, "Guard: I need a way up there.\n");
					
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 50){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "ladder_barney" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_walkaround = TRUE;
					pEntity->pev->max_health = 160;
					pEntity->pev->health = pEntity->pev->max_health;
					pEnemyMonster->m_rpgms_level = 12;//�ȼ��ϸߵľ������׵�
					if(pPlayer->HasTeamMate_CanAdd(pEnemyMonster)){
					pEnemyMonster->m_rpgms_skill1_learn = 44;//ӵ�м��ܤ����⾯��
					pPlayer->TeamMate_add(pEnemyMonster);
					}
					pEnemyMonster->ClearSchedule();
					}
			}
			if(pev->frags == 90){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 100){
					pPlayer->m_game_rate = 32;//��Ϸ����32%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 21){//�¼�21 ʵ���ҽ�ʬϮ��
			if(pev->frags == 0){
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						pPlayer->m_trainning = 1;
						pPlayer->pev->effects = 0;
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );
			}
			if(pev->frags == 2){
					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "ladder_barney");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pSpot->pev->flags |= FL_FROZEN;
					pEnemyMonster->m_notarget_hide = 200;
					pEnemyMonster->m_boltpoison = 195;
					}
			}
			if(pev->frags == 5){//����һ��һ��һ��һ��������������ʵ�壡
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) ){
								if(pEntity->pev->origin.z + 128 < pev->origin.z){//����²��ȫ������
								UTIL_Remove( pEntity );
								}
							}
						}
						SERVER_COMMAND("mp3 play media/music10.mp3\n");
			}
			if(pev->frags == 10){
					CBaseEntity *pSci1 = Create( "monster_scientist", pev->origin + Vector(-128,-160,-40), Vector(0,180,0), NULL );
					pSci1->pev->weapons = 1;
					CBaseMonster *pEnemyMonster1;
					pEnemyMonster1 = pSci1->MyMonsterPointer();
					pEnemyMonster1->SetBodygroup( 1, 1 );
					pEnemyMonster1->m_boltpoison = 10;
					pEnemyMonster1->m_no_pov_limit = 1;

					CBaseEntity *pSci2 = Create( "monster_scientist", pev->origin + Vector(-256,-160,-40), Vector(0,0,0), NULL );
					pSci2->pev->weapons = 1;
					CBaseMonster *pEnemyMonster2;
					pEnemyMonster2 = pSci2->MyMonsterPointer();
					pEnemyMonster2->SetBodygroup( 1, 3 );
					pEnemyMonster2->m_boltpoison = 15;			
					pEnemyMonster2->m_no_pov_limit = 1;

					CBaseEntity *pSci3 = Create( "monster_scientist", pev->origin + Vector(-128,-20,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster3;
					pEnemyMonster3 = pSci3->MyMonsterPointer();
					pEnemyMonster3->SetBodygroup( 1, 8 );
					pEnemyMonster3->m_no_cover_mode = 1;
					pEnemyMonster3->pev->health = 1;
					pEnemyMonster3->pev->targetname = MAKE_STRING("rbmadsci");
					pEnemyMonster3->m_no_pov_limit = 1;

					CBaseEntity *pZombie = Create( "monster_zombie", pev->origin + Vector(-192,-160,-40), Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster4;
					pEnemyMonster4 = pZombie->MyMonsterPointer();
					pEnemyMonster4->m_boltpoison = 20;
					pEnemyMonster4->pev->health = 100;
					pEnemyMonster4->m_alert = 100;
					pEnemyMonster4->m_no_pov_limit = 1;
			}
			if(pev->frags == 15){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
							pSpot->pev->armortype = 5;
							pSpot->pev->velocity.y = 40.0;
						}
			}
			if(pev->frags == 20){
				FireTargets( "togwall1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 65){
				FireTargets( "sciuserbbtn", this, this, USE_TOGGLE, 0 );
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 40.0;
				}
			}
			if(pev->frags == 90){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->velocity.y = -20.0;
					pSpot->pev->armortype = 7;
					CBaseEntity *pCrab = Create( "monster_headcrab", pSpot->pev->origin + Vector(0,0,-50), Vector(0,30,0), NULL );
				}
			}
			if(pev->frags == 105){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					CBaseEntity *pZombie1 = Create( "monster_zombie", pSpot->pev->origin + Vector(20,480,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster1;
					pEnemyMonster1 = pZombie1->MyMonsterPointer();
					pEnemyMonster1->m_walkaround = TRUE;
					pEnemyMonster1->m_no_pov_limit = 1;

					CBaseEntity *pZombie2 = Create( "monster_zombie_barney", pSpot->pev->origin + Vector(80,500,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster2;
					pEnemyMonster2 = pZombie2->MyMonsterPointer();
					pEnemyMonster2->m_walkaround = TRUE;
					pEnemyMonster2->m_no_pov_limit = 1;

					CBaseEntity *pZombie3 = Create( "monster_zombie", pSpot->pev->origin + Vector(-40,500,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster3;
					pEnemyMonster3 = pZombie3->MyMonsterPointer();
					pEnemyMonster3->m_walkaround = TRUE;
					pEnemyMonster3->m_no_pov_limit = 1;

					CBaseEntity *pZombie4 = Create( "monster_zombie", pSpot->pev->origin + Vector(20,560,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster4;
					pEnemyMonster4 = pZombie4->MyMonsterPointer();
					pEnemyMonster4->m_walkaround = TRUE;
					pEnemyMonster4->m_no_pov_limit = 1;

					CBaseEntity *pZombie5 = Create( "monster_zombie_barney", pSpot->pev->origin + Vector(80,560,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster5;
					pEnemyMonster5 = pZombie5->MyMonsterPointer();
					pEnemyMonster5->m_walkaround = TRUE;
					pEnemyMonster5->m_no_pov_limit = 1;

					CBaseEntity *pZombie6 = Create( "monster_zombie", pSpot->pev->origin + Vector(-40,560,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster6;
					pEnemyMonster6 = pZombie6->MyMonsterPointer();
					pEnemyMonster6->m_walkaround = TRUE;
					pEnemyMonster6->m_no_pov_limit = 1;
				}
			}
			if(pev->frags == 107){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					CBaseEntity *pZombie7 = Create( "monster_zombie", pSpot->pev->origin + Vector(20,620,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster7;
					pEnemyMonster7 = pZombie7->MyMonsterPointer();
					pEnemyMonster7->m_no_pov_limit = 1;

					CBaseEntity *pZombie8 = Create( "monster_zombie_barney", pSpot->pev->origin + Vector(80,620,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster8;
					pEnemyMonster8 = pZombie8->MyMonsterPointer();
					pEnemyMonster8->m_no_pov_limit = 1;

					CBaseEntity *pZombie9 = Create( "monster_zombie", pSpot->pev->origin + Vector(-40,620,-40), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster9;
					pEnemyMonster9 = pZombie9->MyMonsterPointer();
					pEnemyMonster9->m_no_pov_limit = 1;

				}
			}
			if(pev->frags == 110){
				FireTargets( "zbbrdoor", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 120){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 30.0;
					pSpot->pev->velocity.y = -10.0;
				}
			}
			if(pev->frags == 125){
				FireTargets( "rb_btn1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 140){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_else_rabbit");
				if ( pSpot ){
					UTIL_SetOrigin( pSpot->pev, pSpot->pev->origin + Vector(80,0,0) );
				}
			}
			if(pev->frags == 195){
						//pPlayer->pev->origin = pev->origin - Vector(-120,80,40);
						//pPlayer->m_stuck_origin = pPlayer->pev->origin;
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
			}
			if(pev->frags == 200){
				FireTargets( "eye_doors2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 205){
					pPlayer->m_game_rate = 33;//��Ϸ����33%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 22){//�¼�22 �����г�
			if(pev->frags == 0){
					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "shield_barney");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
						if(pEnemyMonster->m_cleardally_enemy_long > 0){//��δ��������
						pev->frags = -10;
						}
					}
			}
			if(pev->frags == 1){
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 2){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );
						pPlayer->pev->origin = pev->origin + Vector(-64,-16,36);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 5){
					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "ladder_barney");
					if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						if(pSpot->pev->deadflag == DEAD_NO){
							pEnemyMonster->m_iTriggerCondition = 0;
							pEnemyMonster->ClearSchedule();
							pEnemyMonster->RouteClear();
							pEnemyMonster->m_selfmode = TRUE;
							pEnemyMonster->m_longming = 1;
							pEnemyMonster->m_hTargetEnt = NULL;
							pEnemyMonster->m_hEnemy = NULL;
							pEnemyMonster->m_boltpoison = 100;
							pEnemyMonster->m_flDistLook = 1024;
							pEnemyMonster->m_hTargetEnt = NULL;
							UTIL_SetOrigin( pSpot->pev, pev->origin + Vector(0,120,0) );
							pEnemyMonster->m_groundElev2 = TRUE;
							pSpot->pev->angles = Vector(0,90,0);
							pEnemyMonster->pev->gravity= 1.6;
							if(pPlayer->HasTeamMate_CanAdd(pEnemyMonster)){
							pPlayer->TeamMate_add(pEnemyMonster);
							}
						}
						else{
							pPlayer->TeamMate_remove(pEnemyMonster);
						}
					}
			}
			if(pev->frags == 6){
						if(pPlayer->pev->health < pPlayer->pev->max_health * 0.75){//��ѧ������ҽ��
							CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "train_sci" );
							if ( pEntity2 ){
								if(pEntity2->pev->deadflag == DEAD_NO){
								EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!SC_HEAL5", VOL_NORM, 0.5, 0, PITCH_NORM );
								char text[256];
				
								sprintf( text, "Scientist: Here, let me heal you.\n");
							
								UTIL_SayTextAll( text,this );
								}
							}
						}
			}
			if(pev->frags == 7){
					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "shield_barney");
					if ( pSpot ){
						pSpot->pev->takedamage = DAMAGE_NO;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_iTriggerCondition = 0;
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->RouteClear();
						pEnemyMonster->m_selfmode = TRUE;
						pEnemyMonster->m_longming = 1;
						pEnemyMonster->m_hTargetEnt = NULL;
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_boltpoison = 100;
						pEnemyMonster->m_flDistLook = 1024;
						UTIL_SetOrigin( pSpot->pev, pev->origin + Vector(-60,120,0) );
						pEnemyMonster->m_groundElev2 = TRUE;
						pSpot->pev->angles = Vector(0,90,0);
						pEnemyMonster->pev->gravity= 1.6;
						if(pPlayer->HasTeamMate_CanAdd(pEnemyMonster)){
						pPlayer->TeamMate_add(pEnemyMonster);
						}
					}
			}
			if(pev->frags == 9){
					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "train_sci");
					if ( pSpot ){
						pSpot->pev->takedamage = DAMAGE_NO;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_iTriggerCondition = 0;
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->RouteClear();
						pEnemyMonster->m_selfmode = TRUE;
						pEnemyMonster->m_longming = 1;
						pEnemyMonster->m_hTargetEnt = NULL;
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_boltpoison = 100;
						pEnemyMonster->m_flDistLook = 1024;
						UTIL_SetOrigin( pSpot->pev, pev->origin );
						pEnemyMonster->m_groundElev2 = TRUE;
						pSpot->pev->angles = Vector(0,0,0);
						pEnemyMonster->pev->gravity= 1.6;
						pEnemyMonster->m_lovehate = 10;
						if(pPlayer->HasTeamMate_CanAdd(pEnemyMonster)){
						pPlayer->TeamMate_add(pEnemyMonster);
						}
					}
			}
			if(pev->frags == 15){
					if(pPlayer->pev->health <= pPlayer->pev->max_health * 0.75){//��ѧ������ҽ��
					pPlayer->m_needleheal2 += (int)pPlayer->pev->max_health * 0.25;
					//��ֹ����ʱѪ������ը��
					}
			}
			if(pev->frags == 46){
					pPlayer->Clear_SayText();
					pPlayer->m_game_rate = 35;//��Ϸ����35%
			}
			if(pev->frags == 60){
				FireTargets( "breakable_train", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 90){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "train_sci" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						pEntity->pev->takedamage = DAMAGE_YES;
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_QUESTION3", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
				
						sprintf( text, "Scientist: Will we ever make it out?\n");
					
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 120){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "shield_barney" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						pEntity->pev->takedamage = DAMAGE_YES;
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_IDLE4", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
			
						sprintf( text, "Guard: Our luck will run out eventually.\n");
					
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 160){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 250){
					pPlayer->Clear_SayText();
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "breakable_train" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						pEntity->TakeDamage ( pev, pev, pEntity->pev->health, DMG_GENERIC );//����
						}
					}
			}
			if(pev->frags == 260){
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 23){//�¼�23 Biss��ѧ�Ҵ���
			if(pev->frags == 0){
					SERVER_COMMAND( "autosave\n" );//�Զ�����
			}
			if(pev->frags == 5){
					CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "biss_sci");
					if ( pSpot ){
						if(pSpot->pev->deadflag == DEAD_NO){//��˹��ѧ�һ����ţ�
						UTIL_SetOrigin( pSpot->pev, pev->origin );//����
						pSpot->pev->angles.y = 90;//���ýǶ�
						pSpot->Killed( pev, GIB_NEVER );//����
						FireTargets( "biss_locked_door", this, this, USE_TOGGLE, 0 );//�ű���
						pPlayer->m_ending_frags += 5;//���ȿ�ѧ�ң���Ʒֵ+5%!
						}
					}
			}
			if(pev->frags == 10){
					Create( "monster_alien_slave", pev->origin + Vector(256,0,0), Vector(0,180,0), NULL );//һ��ͨ�����ظ�
			}
			if(pev->frags == 20){
					pPlayer->m_game_rate = 36;//��Ϸ����36%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 24){//�¼�24 ��ʬ���ݹ���ս
			if(pev->frags == 0){
					pPlayer->m_flVelocityModifier = 0;
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 5){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.x = -10;
						}
			}
			if(pev->frags == 10){//����һ��һ��һ��һ��������������ʵ�壡
				//������ʵ����ർ����ˢ����NPC AIû�з�Ӧ! Ӧ��������һЩ! By 2023.1.16 ���Է���
						//int clear_num = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  FClassnameIs ( pEntity->pev, "monstermaker" ) ||
							(pEntity->pev->flags & FL_MONSTER) 
							|| pEntity->pev->deadflag != DEAD_NO){
								if(pEntity->pev->origin.z + 128 < pev->origin.z){
									//����²��ȫ������ & ʬ��
									UTIL_Remove( pEntity );
									//clear_num += 1;
								}
								else if (  FClassnameIs ( pEntity->pev, "monster_zombie" )
									|| FClassnameIs ( pEntity->pev, "monster_zombie_barney" )
									|| FClassnameIs ( pEntity->pev, "monster_zombie_soldier" )
									|| FClassnameIs ( pEntity->pev, "monster_gonome" )
									|| FClassnameIs ( pEntity->pev, "monster_headcrab" )){
									//����ִ��ȫ��ͷз�ͽ�ʬ
									UTIL_Remove( pEntity );
									//clear_num += 1;
								}
							}
						}
					//	char text[256];
						//sprintf( text, "ClearEnt: %d\n",clear_num);
					//	UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 15){
				FireTargets( "zombie_break_mtdoor", this, this, USE_TOGGLE, 0 );//�ű���
			}
			if(pev->frags == 20){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP5", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
					
						sprintf( text, "Scientist: Things are too dangerous.\n");
						
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 25){
				FireTargets( "fscimove1", this, this, USE_TOGGLE, 0 );//��ѧ��1��·
			}
			if(pev->frags == 40){
				FireTargets( "fscimove2", this, this, USE_TOGGLE, 0 );//��ѧ��2��·
			}
			if(pev->frags == 50){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev,pEntity->pev->origin - Vector(0,96,0) );
					}
			}
			if(pev->frags == 55){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");//����ͷ����
					if ( pSpot ){
						pSpot->pev->armortype = 4;
					}
			}
			if(pev->frags == 60){
					FireTargets( "fscimove3", this, this, USE_TOGGLE, 0 );//��ѧ��3��·
			}
			if(pev->frags == 70){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 75){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev,pEntity->pev->origin - Vector(0,192,0) );
					}
			}
			if(pev->frags == 85){
				FireTargets( "fscimove4", this, this, USE_TOGGLE, 0 );//��ѧ��4��·
			}
			if(pev->frags == 90){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfbar" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev,pEntity->pev->origin - Vector(0,256,0) );
					}
			}
			if(pev->frags == 100){
				FireTargets( "fbarmove", this, this, USE_TOGGLE, 0 );//������·
			}
			if(pev->frags == 120){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfbar2" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev,pEntity->pev->origin - Vector(0,384,0) );
					}
			}
			if(pev->frags == 125){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");//����ͷ����
					if ( pSpot ){
						pSpot->pev->armortype = 8;
					}
			}
			if(pev->frags == 130){
				FireTargets( "fbarmove2", this, this, USE_TOGGLE, 0 );//������·
			}
			if(pev->frags == 165){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_no_cover_mode = 1;//���ֲ���������
						pEnemyMonster->m_guard_mode = TRUE;//����ģʽ
						pEnemyMonster->m_chase_mode = -1;
						pEnemyMonster->m_lovehate = 100;
						pEnemyMonster->m_rpgms_inteam = 5;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_no_cover_mode = 1;//���ֲ���������
						pEnemyMonster->m_guard_mode = TRUE;//����ģʽ
						pEnemyMonster->m_chase_mode = -1;
						pEnemyMonster->m_lovehate = 100;
						pEnemyMonster->m_rpgms_inteam = 5;
						}
					}
			}
			if(pev->frags == 175){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_no_cover_mode = 1;//���ֲ���������
						pEnemyMonster->m_guard_mode = TRUE;//����ģʽ
						pEnemyMonster->m_chase_mode = -1;
						pEnemyMonster->m_lovehate = 100;
						pEnemyMonster->m_rpgms_inteam = 5;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_no_cover_mode = 1;//���ֲ���������
						pEnemyMonster->m_guard_mode = TRUE;//����ģʽ
						pEnemyMonster->m_chase_mode = -1;
						pEnemyMonster->m_lovehate = 100;
						pEnemyMonster->m_rpgms_inteam = 5;
						}
					}
			}
			if(pev->frags == 180){//��������
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfbar" );
				if ( pEntity ){
					if(pEntity->pev->deadflag == DEAD_NO){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_lovehate = 100;
					pEnemyMonster->m_ignoredamage = 1;
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->m_rpgms_level = 12;//�ȼ��ϸߵľ���
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "elevfbar2" );
				if ( pEntity ){
					if(pEntity->pev->deadflag == DEAD_NO){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_lovehate = 100;
					pEnemyMonster->m_ignoredamage = 1;
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->m_rpgms_level = 12;//�ȼ��ϸߵľ���
					}
				}
			}
			if(pev->frags == 185){
					pPlayer->pev->origin = pev->origin + Vector(0,0,-36);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					pPlayer->EnableControl(TRUE);
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
					FireTargets( "zbelev_pan", this, this, USE_TOGGLE, 0 );//���ݤ�����
			}
			if(pev->frags == 190){
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 222){
					char text[256];
					
					sprintf( text, "- At least one scientist must survive.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 237){
					FireTargets( "elevzb_maker", this, this, USE_TOGGLE, 0 );
					FireTargets( "elevflag_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 245){
					UTIL_ScreenShake( pev->origin, 24.0, 200.0, 4.0, 1000 );
					FireTargets( "fongroad_rock", this, this, USE_TOGGLE, 0 );
					FireTargets( "rock_light", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 265){
					FireTargets( "zbelev_woodbar", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 270){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zombie_break_mtdoor" );
				if ( pEntity ){
				pEntity->TakeDamage ( pev, pev, pEntity->pev->health, DMG_FALL );
				}
			}
			if(pev->frags == 275){
				SERVER_COMMAND("mp3 play media/music11.mp3\n");
			}
			if(pev->frags == 282){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 680){
					FireTargets( "zbelev_g1_maker", this, this, USE_TOGGLE, 0 );//��Ӣ�ֳ��֣�������ʼ
			}
			if(pev->frags == 780){
					FireTargets( "zbelev_g2_maker", this, this, USE_TOGGLE, 0 );//��Ӣ�ֳ���2
			}
			if(pev->frags == 835){
					FireTargets( "zbelev_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 860){
					if(pPlayer->pev->deadflag != DEAD_NO){
					return;//������
					}
					pPlayer->TeamMate_Nagamatagi_Allclear(0);//��ն���
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
					pPlayer->pev->origin = pev->origin - Vector(380,0,0);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
			}
			if(pev->frags == 861){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
					if ( pEntity ){//��ǿ��ѧ��2
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_no_cover_mode = 0;//���ֲ���������
						pEnemyMonster->m_guard_mode = FALSE;//����ģʽ
						pEnemyMonster->m_chase_mode = 0;
						pEnemyMonster->m_groundElev2 = TRUE;//����ģʽ
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->m_selfmode = FALSE;
						pEnemyMonster->m_no_cover_mode = 0;
						pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("sci_all_die_over");
						pEnemyMonster->m_lovehate = 120;
						pEnemyMonster->m_rpgms_level = 8;//�ȼ��ϸߵĿ�ѧ��
						pEntity->pev->health = 120;
						pEntity->pev->max_health = 120;
						pEntity->pev->spawnflags = 0;
						UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(330,70,0) );
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
					if ( pEntity ){//��ǿ��ѧ��3
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_no_cover_mode = 0;//���ֲ���������
						pEnemyMonster->m_guard_mode = FALSE;//����ģʽ
						pEnemyMonster->m_chase_mode = 0;
						pEnemyMonster->m_groundElev2 = TRUE;//����ģʽ
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->m_selfmode = FALSE;
						pEnemyMonster->m_no_cover_mode = 0;
						pEnemyMonster->m_rpgms_level = 8;//�ȼ��ϸߵĿ�ѧ��
						pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("sci_all_die_over");
						pEnemyMonster->m_lovehate = 120;
						pEntity->pev->health = 120;
						pEntity->pev->max_health = 120;
						pEntity->pev->spawnflags = 0;
						UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(430,70,0) );
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->m_groundElev2 = TRUE;//����ģʽ
						pEnemyMonster->m_selfmode = FALSE;
						pEnemyMonster->m_rpgms_level = 5;//�ȼ��ϸߵĿ�ѧ��
						pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("sci_all_die_over");
						pEnemyMonster->m_lovehate = 120;
						pEntity->pev->health = 80;
						pEntity->pev->max_health = 80;
						pEntity->pev->spawnflags = 0;
						UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(430,-70,0) );
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->m_groundElev2 = TRUE;//����ģʽ
						pEnemyMonster->m_selfmode = FALSE;
						pEnemyMonster->m_rpgms_level = 5;//�ȼ��ϸߵĿ�ѧ��
						pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("sci_all_die_over");
						pEnemyMonster->m_lovehate = 120;
						pEntity->pev->health = 80;
						pEntity->pev->max_health = 80;
						pEntity->pev->spawnflags = 0;
						UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(330,-70,0) );
						}
					}
			}
			if(pev->frags == 885){
					FireTargets( "zbelev_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 900){//��������
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "zbelev_pan" );
				if ( pEntity ){
					pEntity->pev->armorvalue = 5;//������ʬģʽ
					pEntity->pev->speed = 50;//����
				}
			}
			if(pev->frags == 910){
					FireTargets( "zbelev_pan", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 980){//����ǿ������
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfbar" );
				if ( pEntity ){
					if(pEntity->pev->deadflag == DEAD_NO){
					pEntity->Killed( pev, GIB_ALWAYS );//����
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "elevfbar2" );
				if ( pEntity ){
					if(pEntity->pev->deadflag == DEAD_NO){
					pEntity->Killed( pev, GIB_ALWAYS );//����
					}
				}
			}
			if(pev->frags == 1140){//����һ��һ��һ��һ��������������ʵ�壡
						//int clear_num = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  FClassnameIs ( pEntity->pev, "monstermaker" ) || (pEntity->pev->flags & FL_MONSTER) ){
								if(pEntity->pev->origin.z < pev->origin.z){
								//����²��ȫ���������﹤��
								UTIL_Remove( pEntity );
								//clear_num += 1;
								}
							}
						}
						//char text[256];
						//sprintf( text, "ClearEnt: %d\n",clear_num);
						//UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1150){
					pPlayer->m_game_rate = 37;//��Ϸ����37%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 25){//�¼�25 ��ѧ�������ɽ���ȴ׳��ǳ�
			if(pev->frags == 0){
					pPlayer->m_flVelocityModifier = 0;
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
					pPlayer->EnableControl(FALSE);
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 3){
					FireTargets( "ospery_sound", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 5){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.y = -10;
						}
			}
			if(pev->frags == 7){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin, Vector(0,180,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 2, 3 );
					//pEnemyMonster->SetBodygroup( 3, 1 );
			}
			if(pev->frags == 8){
						CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
						if ( pEntity ){//��ѧ��1
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;
							pEnemyMonster->pev->nextthink = gpGlobals->time + 7.2;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 270;
							UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(96,32,0) );
							}
						}
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
						if ( pEntity ){//��ѧ��2
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;
							pEnemyMonster->pev->nextthink = gpGlobals->time + 7.2;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 270;
							UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(96,-64,0) );
							}
						}
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
						if ( pEntity ){//��ѧ��3
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;
							pEnemyMonster->pev->nextthink = gpGlobals->time + 7.2;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 270;
							UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(160,-64,0) );
							}
						}
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
						if ( pEntity ){//��ѧ��4
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;
							pEnemyMonster->pev->nextthink = gpGlobals->time + 7.2;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 270;
							UTIL_SetOrigin( pEntity->pev,pev->origin - Vector(160,32,0) );
							}
						}				
			}
			if(pev->frags == 25){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						goto scispeak;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						goto scispeak;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						goto scispeak;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						goto scispeak;
						}
					}
					return;

					scispeak:
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP7", VOL_NORM, 0.5, 0, PITCH_NORM );
					char text[256];
					
					sprintf( text, "Scientist: It's the army! We're saved!\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 30){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
					if ( pEntity ){//��ѧ��1
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "run" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->pev->velocity.y = -250;
						pEnemyMonster->pev->movetype = MOVETYPE_FLY;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
					if ( pEntity ){//��ѧ��2
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "run" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->pev->velocity.y = -250;
						pEnemyMonster->pev->movetype = MOVETYPE_FLY;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
					if ( pEntity ){//��ѧ��3
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "run" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->pev->velocity.y = -250;
						pEnemyMonster->pev->movetype = MOVETYPE_FLY;
						}
					}
					pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
					if ( pEntity ){//��ѧ��4
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "run" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->pev->velocity.y = -250;
						pEnemyMonster->pev->movetype = MOVETYPE_FLY;
						}
					}
			}
			if(pev->frags == 32){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pSpot->pev->velocity.x = 20;
							pSpot->pev->velocity.y = -250;
							pSpot->pev->avelocity.y = -50;
							pSpot->pev->armortype = 8;
						}
			}
			if(pev->frags == 35){
					CBaseEntity *Flyer = UTIL_FindEntityByTargetname( NULL, "osprey" );
					if ( Flyer ){//�����
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = Flyer->MyMonsterPointer();
							pEnemyMonster->pev->sequence = 1;
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->SetThink( NULL );
					}
			}
			if(pev->frags == 60){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity ){
					UTIL_Remove( pEntity );
					}
			}
			if(pev->frags == 65){
					FireTargets( "hecuout_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 70){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 80){
						pPlayer->pev->origin = pev->origin + Vector(0,0,36);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->pev->angles = Vector(0,180,0);
						pPlayer->pev->v_angle = Vector(0,180,0);
						pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 85){
					CBaseEntity *Flyer = UTIL_FindEntityByTargetname( NULL, "osprey" );
					if ( Flyer ){//�����
						Flyer->pev->sequence = 1;
						CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
						if ( pEntity ){//��ѧ��1
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 90;
							pEntity->pev->takedamage = DAMAGE_NO;
							pEntity->pev->solid = SOLID_NOT;
							UTIL_SetOrigin( pEntity->pev,Flyer->pev->origin + Vector(0,32,40) );
							}
						}
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
						if ( pEntity ){//��ѧ��2
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 90;
							pEntity->pev->takedamage = DAMAGE_NO;
							pEntity->pev->solid = SOLID_NOT;
							UTIL_SetOrigin( pEntity->pev,Flyer->pev->origin + Vector(16,-32,40) );
							}
						}
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
						if ( pEntity ){//��ѧ��3
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 90;
							pEntity->pev->takedamage = DAMAGE_NO;
							pEntity->pev->solid = SOLID_NOT;
							UTIL_SetOrigin( pEntity->pev,Flyer->pev->origin + Vector(64,-32,40) );
							}
						}
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
						if ( pEntity ){//��ѧ��4
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->pev->velocity = g_vecZero;
							pEnemyMonster->pev->yaw_speed = 0;
							pEnemyMonster->pev->angles.y = 90;
							pEntity->pev->takedamage = DAMAGE_NO;
							pEntity->pev->solid = SOLID_NOT;
							UTIL_SetOrigin( pEntity->pev,Flyer->pev->origin + Vector(64,32,40) );
							}
						}
					}
					
			}
			if(pev->frags == 110){
					CBaseEntity *Flyer = UTIL_FindEntityByTargetname( NULL, "osprey" );
					if ( Flyer ){//�����
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = Flyer->MyMonsterPointer();
							CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
							if ( pEntity )//���
							{
									float flDist = ( Flyer->pev->origin - pEntity->pev->origin).Length();
									if(flDist < 700){
										pEnemyMonster->pev->sequence = 2;
										pEnemyMonster->ResetSequenceInfo( );
										pEnemyMonster->pev->frame = 0;

										pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci1" );
										if ( pEntity ){//��ѧ��1
										UTIL_Remove( pEntity );
										}
										pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci2" );
										if ( pEntity ){//��ѧ��2
										UTIL_Remove( pEntity );
										}
										pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci3" );
										if ( pEntity ){//��ѧ��3
										UTIL_Remove( pEntity );
										}
										pEntity = UTIL_FindEntityByTargetname( NULL, "elevfsci4" );
										if ( pEntity ){//��ѧ��4
										UTIL_Remove( pEntity );
										}
									}
									else{
										pev->frags = 105;
									}
							}
					}
			}
			if(pev->frags == 125){
					CBaseEntity *FlyerSound = UTIL_FindEntityByTargetname( NULL, "ospery_sound" );
					if ( FlyerSound ){//�������Ч����
					SET_MODEL(ENT(FlyerSound->pev), "models/camera_rocket.mdl");
					FlyerSound->pev->movetype = MOVETYPE_FLY;
					FlyerSound->pev->velocity.x = -60;
					FlyerSound->pev->velocity.z = 120;
					//FlyerSound->pev->effects |= EF_LIGHT;
					}
			}
			if(pev->frags == 150){
					CBaseEntity *FlyerSound = UTIL_FindEntityByTargetname( NULL, "ospery_sound" );
					if ( FlyerSound ){
					FlyerSound->pev->velocity.x = -200;
					FlyerSound->pev->velocity.z = 200;
					}
			}
			if(pev->frags == 180){
					CBaseEntity *FlyerSound = UTIL_FindEntityByTargetname( NULL, "ospery_sound" );
					if ( FlyerSound ){
					FlyerSound->pev->velocity.x = -300;
					FlyerSound->pev->velocity.z = 150;
					}
			}
			if(pev->frags == 209){
					FireTargets( "ospery_sound", this, this, USE_TOGGLE, 0 );
					CBaseEntity *Flyer = UTIL_FindEntityByTargetname( NULL, "osprey" );
					if ( Flyer ){//�����
					UTIL_Remove( Flyer );
					}
			}
			if(pev->frags == 220){
					pPlayer->m_game_rate = 38;//��Ϸ����38%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 26){//�¼�26 ���ѵİ���
			if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 2){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "talkbani" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP9", VOL_NORM, 0.5, 0, PITCH_NORM );
						char text[256];
					
						sprintf( text, "Guard: Okay. If I see Dengor or Kadoma...\n");
						
						UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 35){
					char text[256];
					
					sprintf( text, "Guard: I'll shoot!\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 65){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "talkbani" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_walkaround = TRUE;
							if(pEnemyMonster->m_hEnemy == NULL){
							EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_OK2", VOL_NORM, 0.5, 0, PITCH_NORM );
							char text[256];
						
							sprintf( text, "Guard: Let's get going!\n");
							
							UTIL_SayTextAll( text,this );
							FireTargets( "baniopen_door", this, this, USE_TOGGLE, 0 );
							}
						}
					}
			}
			if(pev->frags == 100){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 110){
					pPlayer->m_game_rate = 40;//��Ϸ����40%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 27){//�¼�27 �����Ǹ�
			if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "aimdengor_ba" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEntity->pev->spawnflags = 0;
						pEnemyMonster->m_no_pov_limit = 1;
					}
			}
			if(pev->frags == 2){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEntity->pev->spawnflags = 0;
						pEnemyMonster->m_no_pov_limit = 1;
						pEnemyMonster->m_boltpoison = 20;
						pEnemyMonster->m_lovehate += 30;
						pEnemyMonster->m_rpgms_actor = 8;
						pEntity->pev->netname = MAKE_STRING( "Dengor" );
						pEntity->pev->health = 300;
						pEntity->pev->max_health = pEntity->pev->health;
						//Bug Fix 3.0 Ѫ�غ�ĳ�̬Dengor!
					}
			}
			if(pev->frags == 6){
					FireTargets( "dg_use_turt", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 15){
					FireTargets( "dengor_turdr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 50){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "aimdengor_ba" );
					if ( pEntity ){
						if(pEntity->pev->deadflag != DEAD_NO){
						FireTargets( "dengor_turdr", this, this, USE_TOGGLE, 0 );
						}
						else{
						pev->frags = 40;
						}
					}
			}
			if(pev->frags == 60){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_hEnemy = NULL;
						if (pEnemyMonster->m_pCine)
						{
							pEnemyMonster->CineCleanup( );
						}
						pEnemyMonster->ClearSchedule();
							if(pEntity->pev->deadflag == DEAD_NO){
							EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP8", VOL_NORM, 0.5, 0, 95 );
							char text[256];
							
							sprintf( text, "Dengor: Did you hear? The soldiers, guards, and scientists...\n");
							
							UTIL_SayTextAll( text,this );
							}
					}
			}
			if(pev->frags == 90){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
							char text[256];
						
							sprintf( text, "Dengor: Want to kill us.\n");
						
							UTIL_SayTextAll( text,this );
							}
					}
			}
			if(pev->frags == 115){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 120){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();

							if(pEntity->pev->deadflag == DEAD_NO){
							pEnemyMonster->m_selfmode = FALSE;
							EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_OK3", VOL_NORM, 0.5, 0, 95 );
							char text[256];
							
							sprintf( text, "Dengor: Let's go.\n");
							
							UTIL_SayTextAll( text,this );
							}

							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "player" );
							if ( pEntity2 ){
							pEnemyMonster->m_hTargetEnt = pEntity2;
							pEnemyMonster->m_rpgms_level = 18;
							pEnemyMonster->m_lovehate = 100;
							pPlayer->TeamMate_add(pEnemyMonster);
							}
							pEnemyMonster->ClearSchedule();
					}
			}
			if(pev->frags == 150){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 160){
					pPlayer->m_game_rate = 41;//��Ϸ����41%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 28){//�¼�28 ������ѧ�ҽܸ�
			if(pev->frags == 10){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci" );
					if ( pEntity ){
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "player" );
							if ( pEntity2 )//���
							{
									float flDist = ( pEntity->pev->origin - pEntity2->pev->origin).Length();
									if(flDist < 300){
										if(pEntity->pev->deadflag == DEAD_NO){
											EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP10", VOL_NORM, 0.5, 0, 95 );
											char text[256];
											
											sprintf( text, "Scientist: Don't hurt me! I'm a manager!\n");
											
											UTIL_SayTextAll( text,this );
											CBaseMonster *pEnemyMonster;
											pEnemyMonster = pEntity->MyMonsterPointer();
											pEntity->pev->team = 0;
											pEntity->pev->flags |= FL_NOTARGET;
										}
									}
									else{
										pev->frags = 0;
									}
							}
					}
			}
			if(pev->frags == 40){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci" );
					if ( pEntity ){
										if(pEntity->pev->deadflag == DEAD_NO){
											EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP4", VOL_NORM, 0.5, 0, 95 );
											char text[256];
											
											sprintf( text, "Scientist: You'll need me to operate the retinal scanner.\n");
										
											UTIL_SayTextAll( text,this );
											CBaseMonster *pEnemyMonster;
											pEnemyMonster = pEntity->MyMonsterPointer();
											pEnemyMonster->m_selfmode = FALSE;
											pEnemyMonster->m_lovehate = 40;
											pEnemyMonster->m_hEnemy = NULL;
												CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "player" );
												if ( pEntity2 ){
												pEnemyMonster->m_rpgms_level = 4;
												pPlayer->TeamMate_add(pEnemyMonster);//�ܸ��ѧ��
												}
												pEnemyMonster->Hunt_Stand_Set(2);
										}
					}
			}
			if(pev->frags == 50){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->m_hEnemy = NULL;
							}
					}
			}
			if(pev->frags == 70){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 80){
					pPlayer->m_game_rate = 42;//��Ϸ����42%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 29){//�¼�29 �Ǹ괩HEV
			if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->m_trainning = 1;
			}
			if(pev->frags == 2){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci" );
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
							UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-144,-16,64) );	
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEntity->pev->angles.y = 0;
							pEntity->pev->yaw_speed = 0;
							pEnemyMonster->m_hEnemy = NULL;
							pEnemyMonster->m_hTargetEnt = NULL;
							pEnemyMonster->ClearSchedule();
							pEnemyMonster->SetState( MONSTERSTATE_HUNT );
							pEnemyMonster->RouteClear();
							pEnemyMonster->SetActivity( ACT_IDLE );
							pEntity->pev->flags |= FL_NOTARGET;
							pEnemyMonster->m_noidleseq = TRUE;
							}
					}
			}
			if(pev->frags == 4){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
							UTIL_SetOrigin( pEntity->pev, pev->origin);	
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEntity->pev->angles.y = 0;
							pEntity->pev->yaw_speed = 0;
							pEnemyMonster->m_hEnemy = NULL;
							pEnemyMonster->m_hTargetEnt = NULL;
							pEnemyMonster->ClearSchedule();
							}
					}
			}
			if(pev->frags == 6){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
							if ( pSpot ){
								SET_VIEW( pPlayer->edict(), pSpot->edict() );
								pPlayer->m_player_camera = pSpot;
								pSpot->pev->velocity.x = 15;
								pSpot->pev->avelocity.x = 2;
								pPlayer->pev->angles = Vector(0,90,0);
							}
			}
			if(pev->frags == 50){
					FireTargets( "hevpushbtn", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 80){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 81){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dengor_sci" );
					if ( pEntity ){
							UTIL_Remove( pEntity );
							CBaseEntity *pHevsuit = UTIL_FindEntityByTargetname( NULL, "hevsuit_f" );
							if ( pHevsuit ){
							UTIL_Remove( pHevsuit );
							}
					}
			}
			if(pev->frags == 82){
					FireTargets( "dengor_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 84){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
							if ( pSpot ){
								pSpot->pev->origin = pev->origin + Vector(100,0,60);
								pSpot->pev->velocity.x = 0;
								pSpot->pev->velocity.z = 8;
								pSpot->pev->angles.x = 0;
								EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/hevsuit_pickup.wav", 1, ATTN_NORM, 0, 150 );
							}
			}
			if(pev->frags == 86){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_godmode = TRUE;
					pEntity->pev->angles.y = 180;
					pEntity->pev->yaw_speed = 0;
					pEntity->pev->body = 1;
					pEnemyMonster->m_walkaround = FALSE;
					pEnemyMonster->m_walkaroundFail = FALSE;
					}
			}
			if(pev->frags == 100){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
								EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP9", VOL_NORM, 0.5, 0, 95 );
								char text[256];
								
								sprintf( text, "Dengor: This new HEV suit should prove useful.\n");
								
								UTIL_SayTextAll( text,this );
							}
					}
			}
			if(pev->frags == 150){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 155){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci" );
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
							pEntity->pev->angles.y = 270;
							pEntity->pev->yaw_speed = 0;
							UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin - Vector(0,20,0) );	
							}
					}
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin + Vector(-145,-80,60), Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 0, 1 );
					//pEnemyMonster->SetBodygroup( 3, 1 );
			}
			if(pev->frags == 160){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
							if ( pSpot ){
								pSpot->pev->avelocity.y = 95;
								pSpot->pev->velocity.x = -95;
								pSpot->pev->velocity.y = -30;
								pSpot->pev->velocity.z = 8;
							}
			}
			if(pev->frags == 180){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
							if ( pSpot ){
								pSpot->pev->avelocity.y = 0;
								pSpot->pev->avelocity.x = 0;
								pSpot->pev->velocity.x = 0;
								pSpot->pev->velocity.y = 0;
								pSpot->pev->velocity.z = 0;
							}
			}
			if(pev->frags == 190){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci");
					if ( pEntity ){
								EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_PQUEST15", VOL_NORM, 0.5, 0, 100 );
								char text[256];
								
								sprintf( text, "Scientist: Is anyone hungry?\n");
								
								UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 220){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci");
					if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "give_donut" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP11", VOL_NORM, 0.5, 0, 100 );
							char text[256];
							
							sprintf( text, "Scientist: I've got a donut right here.\n");
							
							UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 240){
						SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 250){
						pPlayer->m_fSelectMode = TRUE;
						pPlayer->m_fSelectNumber = 0;
						pPlayer->ShowVGUIMenu(33);
						pPlayer->m_load_check = 1;
			}
			if(pev->frags == 270){
						if(pPlayer->m_fSelectNumber == 0){
									if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
										pPlayer->ShowVGUIMenu(33);
										pPlayer->m_load_check = 1;
									}
						pev->frags = 260;
						}
			}
			if(pev->frags == 275){
						pev->team = pPlayer->m_fSelectNumber;
						pPlayer->m_fSelectNumber = 0;
						pPlayer->Clear_SayText();
			}
			if(pev->team == 1){//��ҳ�������Ȧ������
				if(pev->frags == 280){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
						if ( pEntity ){
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								pEnemyMonster->m_noidleseq = TRUE;
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "eat_do" );
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								pEnemyMonster->SetThink( NULL );
						}
				}
				if(pev->frags == 283){
						CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci");
						if ( pEntity ){
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
								pEnemyMonster->m_noidleseq = FALSE;
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								pEnemyMonster->SetBodygroup( 2, 0 );
						}

						pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
						if ( pEntity ){
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								pEnemyMonster->SetBodygroup( 2, 6 );
						}
				}
				if(pev->frags == 330){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
						if ( pEntity )//��Ҥ�ѡ��
						{
						pEntity->Killed( pev, GIB_NEVER );
						return;
						}
				}
			}
			else if(pev->team == 2){//����
				if(pev->frags == 290){
						CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "jgsci");
						if ( pEntity ){
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
								pEnemyMonster->m_noidleseq = FALSE;
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								pEnemyMonster->m_iTriggerCondition = 0;
								pPlayer->TeamMate_remove(pEnemyMonster);
								pEntity->pev->team = 1;
								pEntity->pev->spawnflags |= SF_MONSTER_PRISONER;
								pEntity->pev->health = 80;

								EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_STARE0", VOL_NORM, 0.5, 0, 100 );
								char text[256];
								
								sprintf( text, "Scientist: Really?\n");
								
								UTIL_SayTextAll( text,this );
						}
				}
				if(pev->frags == 320){
						pPlayer->Clear_SayText();
				}
				if(pev->frags == 330){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_selfmode = TRUE;
					pEnemyMonster->m_longming = 1;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-144,0,59) );	
					pEntity->pev->angles.y = 270;
					pEntity->pev->yaw_speed = 0;
					pEntity->pev->body = 1;
					pEntity->pev->gravity = 1.1;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetActivity( ACT_MELEE_ATTACK1 );
					}
				}
				if(pev->frags == 345){
								CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
								if ( pEntity2 ){
									pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
									pPlayer->m_stuck_origin = pPlayer->pev->origin;
									SET_VIEW( pPlayer->edict(), pPlayer->edict() );
									pPlayer->EnableControl(TRUE);
									pPlayer->pev->angles = Vector(0,90,0);
									pPlayer->pev->v_angle = Vector(0,90,0);
									pPlayer->pev->fixangle = TRUE;
									pPlayer->m_trainning = 0;
									pPlayer->m_fSelectMode = FALSE;
									UTIL_Remove( pEntity2 );
								}
				}
				if(pev->frags == 355){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
							if(pEntity->pev->deadflag == DEAD_NO){
								EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP12", VOL_NORM, 0.5, 0, 95 );
								char text[256];
								
								sprintf( text, "Dengor: Don't trust the scientists.\n");
								
								UTIL_SayTextAll( text,this );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pEntity->MyMonsterPointer();
								pEnemyMonster->SetState( MONSTERSTATE_IDLE );
							}
					}
				}
				if(pev->frags == 400){
						pPlayer->Clear_SayText();
				}
				if(pev->frags == 410){
					FireTargets( "dengor_usechange", this, this, USE_TOGGLE, 0 );
					UTIL_Remove( this );
					return;
				}
			}

	}
	else if(pev->armortype == 30){//�¼�30 �Ǹ��ٵǳ�
			if(pev->frags == 0){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){//Bug Fix 3.0 ����Ƿ�bug�����ϸ�dengor��
					UTIL_Remove( pEntity );
					}

					FireTargets( "dengor_remaker", this, this, USE_TOGGLE, 0 );
					FireTargets( "fongroad_dgores", this, this, USE_TOGGLE, 0 );//��ҷ�·
			}
			if(pev->frags == 3){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEntity->pev->takedamage = DAMAGE_NO;
					pEntity->pev->angles.y = 270;
					pEntity->pev->body = 1;
					pEnemyMonster->m_boltpoison = 80;
					pEnemyMonster->m_crouchmode = 1;
					pEnemyMonster->m_walkaround = TRUE;
					pEnemyMonster->m_walkaroundFail = TRUE;
					UTIL_SetSize(pEntity->pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_DUCK);//�Ǹ����
					}
			}
			if(pev->frags == 5){
					FireTargets( "dengor_res_brkv", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 50){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
								ClearBits( pEntity->pev->flags, FL_ONGROUND );
								UTIL_SetOrigin (pEntity->pev, pEntity->pev->origin + Vector ( 0,0,1) );
								pEntity->pev->velocity.y -= 400;
					}
			}
			if(pev->frags == 60){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEntity->pev->body = 2;
					pEnemyMonster->m_crouchmode = 0;
					UTIL_SetSize(pEntity->pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);//�Ǹ�����
					pEnemyMonster->SetActivity( ACT_IDLE );
					pEnemyMonster->RouteClear();
					pEnemyMonster->m_iTriggerCondition = 4;
					pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("game_over_event");
					pEntity->pev->takedamage = DAMAGE_YES;

					pEnemyMonster->m_walkaround = FALSE;
					pEnemyMonster->m_walkaroundFail = FALSE;

					pEnemyMonster->ClearSchedule();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->m_hTargetEnt = pPlayer;

					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP13", VOL_NORM, 0.5, 0, 95 );
					char text[256];
					
					sprintf( text, "Dengor: Thought I was dead?\n");
					
					UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 110){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 115){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 120){//����һ��һ��һ��һ��������������ʵ�壡
						//int clear_num = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) ){
								if(pEntity->pev->origin.y > pev->origin.y){//���Y���Ϸ���ȫ������
								UTIL_Remove( pEntity );
						//		clear_num++;
								}
							}
						}
					//	char text[256];
					//	sprintf( text, "ClearEnt: %d\n",clear_num);
					//	UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 140){//�ȴ׸���
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) ){
								if(FClassnameIs ( pEntity->pev, "monster_human_grunt" )){
								CBaseMonster *pEnemyMonster;
									pEnemyMonster = pEntity->MyMonsterPointer();
									if(pEnemyMonster->m_FTSmod == 8){
									pEnemyMonster->m_no_pov_limit = 1;
									}
								}
							}
						}
			}
			if(pev->frags == 150){
					pPlayer->m_game_rate = 43;//��Ϸ����43%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 31){//�¼�31 �����߳�¯
			if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->m_trainning = 1;

						//�������
						pPlayer->TeamMate_Nagamatagi_Allclear(0);
			}
			if(pev->frags == 2){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pSpot->pev->velocity.y = -5;
						}
			}
			if(game_boss_battle == 1 && pev->frags == 3){
			pev->frags = 19;//��������һС��
			}
			if(pev->frags == 20){
					FireTargets( "activate_boss", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 50){
									SET_VIEW( pPlayer->edict(), pPlayer->edict() );
									UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
									pPlayer->EnableControl(TRUE);
									pPlayer->m_trainning = 0;
									pPlayer->m_flVelocityModifier = 0;
									pPlayer->m_fMask = FALSE;
									game_boss_battle = 1;
			}
			if(pev->frags == 55){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_crasher_boss");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 20;
					pSpot->pev->spawnflags = 0;
					pEnemyMonster->m_godmode = FALSE;
					pEnemyMonster->m_walkaround = TRUE;
					pEnemyMonster->m_walkaroundFail = TRUE;
					pEnemyMonster->m_chase_mode = 3;
					pEnemyMonster->m_chase_failed_max = 2;
					pEnemyMonster->m_EyeMod = 2;
					pEnemyMonster->m_MoveFail_FuckRoad = TRUE;
					pEnemyMonster->m_diefadeout = 1;
					UTIL_SetOrigin( pSpot->pev, pev->origin );
					}
			}
			if(pev->frags == 60){
					pPlayer->BOSS_Find();
			}
			if(pev->frags == 65){
					pPlayer->m_music_save = 6;
					//SERVER_COMMAND("mp3 loop media/boss2.mp3\n");
					CLIENT_COMMAND(pPlayer->edict(), "cd loop 17\n");
			}
			if(pev->frags == 70){
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 32){//�¼�32 �����߻�ɱ��Gman����Xen
			if(pev->frags == 10){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 30){
				SERVER_COMMAND("mp3 stop\n");
			}
			if(pev->frags == 31){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->m_trainning = 1;
						game_boss_battle = 0;
						pPlayer->m_music_save = 0;
			}
			if(pev->frags == 34){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
						}
			}
			if(pev->frags == 37){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_gman");
					if ( pGman ){
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->angles.y = 0;
					pSpot->pev->origin = pGman->pev->origin + Vector(-24,0,64);
					}
				}
			}
			if(pev->frags == 39){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", Vector(2382,-1125,-796), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 2, 3 );
					//pEnemyMonster->SetBodygroup( 3, 1 );
			}
			if(pev->frags == 50){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "pistol_aim2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetThink( NULL );
				}
			}
			if(pev->frags == 70){
				CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pGman ){
					EMIT_SOUND_DYN( pGman->edict(), CHAN_VOICE, "!GM_ZP1", VOL_NORM, 0.5, 0, 100 );
					char text[256];
					
					sprintf( text, "Gman: I suppose we won't be working together.\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 110){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 120){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
					if ( pEntity )
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "teleport_rec" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetBodygroup( 2, 2 );
					}
			}
			if(pev->frags == 150){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->velocity.y = 4;
				}
				CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pGman ){
					EMIT_SOUND_DYN( pGman->edict(), CHAN_VOICE, "!GM_ZP2", VOL_NORM, 0.5, 0, 100 );
					char text[256];
					
					sprintf( text, "Gman: I have relieved you of your weapons.\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 180){
				char text[256];
				
				sprintf( text, "Gman: And now, a fight you can't win...\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 210){
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;
					pPlayer->pev->origin = pev->origin + Vector(-16,256,200);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->RemoveAllItems( FALSE );
					pPlayer->pev->armorvalue = 0;
					pPlayer->m_skill_maxarmor	= 0;
					pPlayer->Clear_SayText();
					pPlayer->m_game_rate = 45;//��Ϸ����45%
			}
			if(pev->frags == 215){
					UTIL_Remove( this );
					return;
			}
	}

	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
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
// ��CG�¼���ͳ��4
//=========================================================
class CMain_Event4 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
	char text[256];
};

LINK_ENTITY_TO_CLASS( main_cg_event_new4, CMain_Event4 );//����4

void CMain_Event4::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event4::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��4=================================================//
void CMain_Event4::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}
	if(pev->armortype == 48){//�¼�48 ׼����������ľ֮ɭ
			if(pev->frags == 3){
					pPlayer->Clear_SayText();
					SERVER_COMMAND( "=cammousemove\n");//��ȫ��������
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
						pSpot->pev->velocity.x = -5;
					}
			}
			if(pev->frags == 5){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(0,32,0) );
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity2->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity2->pev, pev->origin - Vector(0,32,0) );
				}
			}
			if(pev->frags == 10){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "main_cg_event_new3");
					if ( pSpot ){
						pev->team = pSpot->pev->team;
						UTIL_Remove( pSpot );
					}
			}
			if(pev->frags == 20){
					if(pPlayer->m_fMoveItem != NULL){//Bug Fix 3.0 С����������ţ����Ƴ���
					UTIL_Remove( pPlayer->m_fMoveItem );
					}
			}
			if(pev->frags == 65){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						pSpot->pev->velocity.x = 1;
						pSpot->pev->origin = pev->origin + Vector(-180,4,64);
						pSpot->pev->velocity.z = 4;
					}
			}
			if(pev->frags == 90){
				sprintf( text, "Misaliya: ......\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 140){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 165){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						pSpot->pev->velocity.x = 0;
						pSpot->pev->origin = pev->origin + Vector(128,4,72);
						pSpot->pev->velocity.z = 0;
					}
			}
			if(pev->frags == 170){
			
				sprintf( text, "???: Hey, wait!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 200){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 208){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->angles.y = 180;
				pEntity->pev->movetype = MOVETYPE_STEP;
				pEntity->pev->frags = 0;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(384,0,0) );
				}
			}
			if(pev->frags == 211){
				FireTargets( "wiseb_intodivt", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 215){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					pEntity->pev->angles.y = 340;
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					pEntity2->pev->angles.y = 20;
				}
			}
			if(pev->frags == 240){
				if(pev->team == 1){
					
					sprintf( text, "Wisebeast: You really wanted to kill me?\n");
					
				}
				else if(pev->team == 2){
				
					sprintf( text, "Wisebeast: You two left without me!\n");
					
				}
				else if(pev->team == 3){
					
					sprintf( text, "Wisebeast: No hard feelings, Misaliya?\n");
					
				}
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 280){
				
				sprintf( text, "Misaliya: You recovered your magic?\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 310){
				if(pev->team == 2){
				
					sprintf( text, "Misaliya: I guess you did...\n");
				
				}
				else{
				
					sprintf( text, "Misaliya: You found another way.\n");
				
				}
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 350){
				
				sprintf( text, "Wisebeast: Yes, now we can fight together!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 390){
				
				sprintf( text, "Misaliya: Yes, let's.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 450){
				pPlayer->Clear_SayText();
				UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 472){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles.y = 180;
						pSpot->pev->velocity.x = -5;
					}
			}
			if(pev->frags == 474){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
				if ( pSpot ){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity ){
						pEntity->pev->angles.y = 180;
						UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin + Vector(192,-48,-64) );
					}
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity2 ){
						pEntity2->pev->angles.y = 180;
						UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(192,48,-64) );
					}
					CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
					if ( pEntity3 ){
						pEntity3->pev->angles.y = 180;
						UTIL_SetOrigin( pEntity3->pev, pSpot->pev->origin + Vector(160,0,-64) );
					}
				}
			}
			if(pev->frags == 475){
				UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 505){
				
				sprintf( text, "Wisebeast: I'll show you my true power!\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 540){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
					pPlayer->m_stuck_origin = pPlayer->pev->origin + Vector(0,0,36);
					UTIL_Remove( pEntity2 );
					pPlayer->Clear_SayText();
					pPlayer->m_trainning = 0;
					pPlayer->EnableControl(TRUE);
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
				}
			}
			if(pev->frags == 542){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_enemyfollower = 1;
				SetBits( pEntity->pev->effects, EF_DIMLIGHT);//��������ģʽ
				pEntity->pev->skin = 0;//Bug Fix 3.0 ���鸴λһ��
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_enemyfollower = 1;
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->m_chase_mode = 1;
				pEntity2->pev->takedamage = DAMAGE_YES;
				ClearBits(pEntity2->pev->flags, FL_NOTARGET);
				pEntity2->pev->spawnflags = 0;
				pEntity2->pev->frags = 0;
				SetBits( pEntity2->pev->effects, EF_DIMLIGHT);//��������ģʽ
				}
			}
			if(pev->frags == 600){
				pPlayer->m_game_rate = 69;//��Ϸ����69%
				UTIL_Remove( this );
				return;
			}
	}

	if(pev->armortype == 49){//�¼�49 ������BOSSս
			if(pev->frags == 0){//Bug Fix 3.0 ���ָĳɵ��˴��ر�
				pPlayer->m_music_save = 0;
				SERVER_COMMAND("mp3 stop\n");
			}
			if(pev->frags == 21){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 23){
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pev->origin.z = pSpot->pev->origin.z;
						CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_gman");
						if ( pGman ){
							SetBits( pGman->pev->effects, EF_DIMLIGHT);//��������ģʽ
							pSpot->pev->origin = pGman->pev->origin + Vector(32,4,64);
							pSpot->pev->angles.y = 220;
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							pPlayer->m_trainning = 1;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->m_wdoor_mynpc = NULL;
							pPlayer->m_guard_mynpc = 0;
							pPlayer->pev->origin = pev->origin + Vector(-512,0,0);
							pPlayer->m_stuck_origin = pev->origin + Vector(-512,0,0);
						}
					}
			}
			if(pev->frags == 25){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(640,480,0));
					pEntity->pev->angles.y = 60;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 2, 7 );
				}
				else{
					pEntity = Create( "monster_kadoma", pev->origin + Vector(640,480,0), Vector(0,60,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 2, 7 );
				}
			}
			if(pev->frags == 26){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(700,440,0) );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity->pev->angles.y = 65;
				pEnemyMonster->m_enemyfollower = 0;
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity->pev->movetype = MOVETYPE_STEP;
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEnemyMonster->pev->velocity = g_vecZero;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->RouteClear();
				pEnemyMonster->m_hEnemy = NULL;
				pEnemyMonster->m_hOldEnemy[0] = NULL;
				pEnemyMonster->m_hOldEnemy[1] = NULL;
				pEnemyMonster->m_hOldEnemy[2] = NULL;
				pEnemyMonster->m_hOldEnemy[3] = NULL;
				pEnemyMonster->m_playerguardian_mode = 0;
				pEnemyMonster->pev->flags &= ~FL_NOTARGET;
				pEntity->pev->owner = NULL;
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(750,400,0) );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity2->pev->angles.y = 70;
				pEnemyMonster->m_enemyfollower = 0;
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity2->pev->movetype = MOVETYPE_STEP;
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEnemyMonster->pev->velocity = g_vecZero;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->RouteClear();
				pEnemyMonster->m_hEnemy = NULL;
				pEnemyMonster->m_hOldEnemy[0] = NULL;
				pEnemyMonster->m_hOldEnemy[1] = NULL;
				pEnemyMonster->m_hOldEnemy[2] = NULL;
				pEnemyMonster->m_hOldEnemy[3] = NULL;
				pEnemyMonster->m_playerguardian_mode = 0;
				pEnemyMonster->pev->flags &= ~FL_NOTARGET;
				pEntity2->pev->owner = NULL;
				}
			}
			if(pev->frags == 30){
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss" );
				if ( pEntity3 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity3->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle3" );//��֪Ϊ����Ч��
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
			}
			if(pev->frags == 35){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "stand" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
			}
			if(pev->frags == 60){
				char text[256];
				
				sprintf( text, "Gman: Hydra's Seal was broken.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 100){
				char text[256];
				
				sprintf( text, "Gman: I'll leave you to it.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 160){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 163){
				char text[256];
				
				sprintf( text, "Wisebeast: Hey, stop!\n");
				
				UTIL_SayTextAll( text,this );
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "teleport_self" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 203){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 205){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss");
						if ( pGman ){
							pSpot->pev->origin = pGman->pev->origin + Vector(-256,-256,64);
							pSpot->pev->angles.y = 45;
							pSpot->pev->velocity.z = 25;
						}
					}
			}
			if(pev->frags == 233){
				char text[256];
				
				sprintf( text, "Wisebeast: It's coming!\n");
				
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 273){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 275){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss");
						if ( pGman ){
							SetBits( pGman->pev->effects, EF_DIMLIGHT);
							pSpot->pev->velocity.z = 0;
						}
					}
			}
			if(pev->frags == 277){
				FireTargets( "into_boss_gate", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 280){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					UTIL_SetOrigin( pEntity2->pev, pEntity2->pev->origin + Vector(-64,-64,0) );
					pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
					pPlayer->m_stuck_origin = pPlayer->pev->origin + Vector(0,0,36);
					UTIL_Remove( pEntity2 );
					pPlayer->Clear_SayText();
					pPlayer->m_trainning = 0;
					pPlayer->EnableControl(TRUE);
					pPlayer->m_flVelocityModifier = 0;
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
				}
			}
			if(pev->frags == 285){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->m_boltpoison = 20;
				pEnemyMonster->m_enemyfollower = 1;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(-192,64,0) );
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->m_boltpoison = 20;
				pEnemyMonster->m_enemyfollower = 1;
				}
			}
			if(pev->frags == 287){
				CBaseEntity *pHydra = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss");
				if ( pHydra ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pHydra->MyMonsterPointer();
					pEnemyMonster->m_walkaround = TRUE;
					pEnemyMonster->m_walkaroundFail = TRUE;
					pEnemyMonster->m_godmode = FALSE;
					pEnemyMonster->m_boltpoison = 25;
					ClearBits(pHydra->pev->flags, FL_NOTARGET);
					ClearBits(pHydra->pev->spawnflags, SF_MONSTER_PRISONER);
					pEnemyMonster->SetActivity( ACT_IDLE );
					pEnemyMonster->ClearSchedule();
					pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				}
				CBaseEntity *pHydra_Spore = UTIL_FindEntityByClassname( NULL, "hydra_spore_fun");
				if ( pHydra_Spore ){
					ClearBits(pHydra_Spore->pev->spawnflags, SF_MONSTER_PRISONER);
				}
			}
			if(pev->frags == 290){
				pPlayer->BOSS_Find();
				game_boss_battle = 1;
			}
			if(pev->frags == 292){
				pPlayer->m_music_save = 4;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 20\n");
				//SERVER_COMMAND("mp3 loop media/boss4.mp3\n");
			}
			if(pev->frags == 295){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 300){
				UTIL_Remove( this );
				return;
			}
	}
	
	if(pev->armortype == 50){//�¼�50 ��������ӡ
			if(pev->frags == 60){
			pPlayer->m_music_save = 0;
			SERVER_COMMAND( "mp3 stop\n" );
			}
			if(pev->frags == 61){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 63){
					game_boss_battle = 0;
					pPlayer->m_music_save = 0;
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
							pSpot->pev->origin = pev->origin + Vector(-384,-384,64);
							pSpot->pev->angles.y = 45;
							pSpot->pev->angles.x = 10;
							pSpot->pev->velocity.x = -4;
							pSpot->pev->velocity.y = -4;
							pSpot->pev->velocity.z = -2;
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							pPlayer->m_trainning = 1;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->m_wdoor_mynpc = NULL;
							pPlayer->m_guard_mynpc = 0;
							pPlayer->pev->origin = pev->origin + Vector(-2048,0,0);
							pPlayer->m_stuck_origin = pev->origin + Vector(-2048,0,0);
					}
			}
			if(pev->frags == 65){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-400,-360,0));
					pEntity->pev->angles.y = 30;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 2, 7 );
				}
				else{
					pEntity = Create( "monster_kadoma", pev->origin + Vector(-400,-360,0), Vector(0,30,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 2, 7 );
				}
			}
			if(pev->frags == 66){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();

				//Bug Fix 3.0 ����Ч��Ӱ������ݳ�
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEnemyMonster->m_playerguardian_mode = 0;
				pEnemyMonster->pev->flags &= ~FL_NOTARGET;
				pEntity->pev->owner = NULL;

				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_hEnemy = NULL;
				pEnemyMonster->m_hOldEnemy[0] = NULL;
				pEnemyMonster->m_hOldEnemy[1] = NULL;
				pEnemyMonster->m_hOldEnemy[2] = NULL;
				pEnemyMonster->m_hOldEnemy[3] = NULL;
				pEnemyMonster->SetActivity( ACT_IDLE );
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-360,-420,-93) );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity->pev->angles.y = 70;
				pEnemyMonster->m_enemyfollower = 0;
				pEnemyMonster->pev->yaw_speed = 0;
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();

				//Bug Fix 3.0 ����Ч��Ӱ������ݳ�
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEnemyMonster->m_playerguardian_mode = 0;
				pEnemyMonster->pev->flags &= ~FL_NOTARGET;
				pEntity2->pev->owner = NULL;

				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetActivity( ACT_IDLE );
				UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(-300,-300,-93) );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity2->pev->angles.y = 45;
				pEnemyMonster->m_enemyfollower = 0;
				pEnemyMonster->pev->yaw_speed = 0;
				}
			}
			if(pev->frags == 70){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity->pev->renderfx = 0;
				pEntity->pev->solid = SOLID_NOT;
				pEntity->pev->effects = 0;
				pEntity->pev->frame = 255;
				pEntity->pev->framerate = 0;
				UTIL_SetOrigin( pEntity->pev, pev->origin - Vector(0,0,96));
				pEntity->pev->angles.y = 225;
				}
			}
			if(pev->frags == 100){
				char text[256];
				sprintf( text, "Wisebeast: yataze!\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 140){
				char text[256];
				
				sprintf( text, "Wisebeast: Resealing Hydra!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 200){
					CBaseEntity *pEntity = Create( "monster_gman", pev->origin + Vector(0,0,-64), Vector(0,225,0), NULL );
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "teleport_use" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					SetBits( pEntity->pev->effects, EF_DIMLIGHT);//��������ģʽ
			}
			if(pev->frags == 205){
				char text[256];
				sprintf( text, "Wisebeast: fa!?\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 210){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->velocity = g_vecZero;
					}
			}
			if(pev->frags == 250){
				pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->velocity.x = 150;
						pSpot->pev->velocity.y = 150;
						pSpot->pev->velocity.z = -20;
					}
			}
			if(pev->frags == 275){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->velocity.x = 0;
						pSpot->pev->velocity.y = 0;
						pSpot->pev->velocity.z = 0;
					}
			}
			if(pev->frags == 290){
					CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_gman");
					if ( pGman ){
					CBaseMonster *pEnemyMonster = pGman->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idlebrush" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
				char text[256];
				
				sprintf( text, "Gman: A good experiment. Well done.\n");
		
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 340){
				char text[256];
				
				sprintf( text, "Gman: Goodbye.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 380){
				char text[256];
				
				sprintf( text, "Wisebeast: You won't escape!\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 430){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 435){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->velocity.x = -25;
						pSpot->pev->velocity.y = -25;
					}

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEntity2->pev->movetype = MOVETYPE_FLY;
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "swim" );
					pEntity2->pev->velocity.x = 320;
					pEntity2->pev->velocity.y = 320;
					pEntity2->pev->velocity.z = 20;
					}

					CBaseEntity *pGman = UTIL_FindEntityByClassname( NULL, "monster_gman");
					if ( pGman ){
					CBaseMonster *pEnemyMonster = pGman->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "teleport_self" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 445){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
					if ( pEntity2 ){
						EMIT_SOUND(ENT(pEntity2->pev), CHAN_WEAPON, "debris/beamstart2.wav", 1, 0.7);	
						int teleb = PRECACHE_MODEL("sprites/b-tele1.spr");
						MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pEntity2->pev->origin);
						WRITE_BYTE(3);
						WRITE_COORD( pEntity2->pev->origin.x);
						WRITE_COORD( pEntity2->pev->origin.y);
						WRITE_COORD( pEntity2->pev->origin.z + 32);
						WRITE_SHORT(teleb);
						WRITE_BYTE(15);
						WRITE_BYTE(15);
						WRITE_BYTE(4);
						MESSAGE_END();
						UTIL_Remove( pEntity2 );
					}
			}
			if(pev->frags == 480){
				
				sprintf( text, "Misaliya: They teleported away - will he be OK?\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 530){
				
				sprintf( text, "Misaliya: We should get to New Nippori.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 590){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 600){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->v_angle = pSpot->pev->angles;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 602){
				FireTargets( "doma_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 606){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_shoot1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->angles.y = 50;
				}
			}
			if(pev->frags == 607){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "cover" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEntity->pev->velocity.x = -90;
					pEntity->pev->velocity.y = 90;
					pEntity->pev->movetype = MOVETYPE_FLY;
					pEntity->pev->solid = SOLID_NOT;
					UTIL_SetSize(pEntity->pev, Vector( 0, 0, 0), Vector(0, 0, 0));
				}
			}
			if(pev->frags == 612){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					pEntity->pev->velocity.x = 0;
					pEntity->pev->velocity.y = 0;
					pEntity->pev->movetype = MOVETYPE_STEP;
					UTIL_SetSize(pEntity->pev, Vector( -16, -16, 0), Vector(16, 16, 72));
				}
			}
			if(pev->frags == 623){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->armortype = 10;
					}
			}
			if(pev->frags == 665){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->angles.y = 225;
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							pSpot->pev->origin.x = pEntity->pev->origin.x + 64;
							pSpot->pev->origin.y = pEntity->pev->origin.y + 64;
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.y = 0;
							pSpot->pev->velocity.z = -4;
						}
					}
			}
			if(pev->frags == 710){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_disarm" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							SetBits( pEntity->pev->effects, EF_DIMLIGHT);//��������ģʽ
						}
			}
			if(pev->frags == 740){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->armortype = 9;
						pSpot->pev->velocity.x = -1;
						pSpot->pev->velocity.y = -1;
					}
			}
			if(pev->frags == 750){
				
				sprintf( text, "Doma: I am Doma...\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 775){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->armortype = 10;
					}
			}
			if(pev->frags == 780){
				
				sprintf( text, "Doma: I've come to kill you, Kadoma.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 795){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles.y = 50;
						pPlayer->pev->v_angle.y = 50;
						pPlayer->pev->fixangle = TRUE;

					}
			}
			if(pev->frags == 820){
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-192,-192,-93) );
				pEntity->pev->angles.y = 225;
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_aim" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(-224,-160,-93) );
					pEntity2->pev->angles.y = 240;
					pEnemyMonster->SetBodygroup( 0, 0 );
					pEnemyMonster->SetBodygroup( 1, 3 );
					pEnemyMonster->SetBodygroup( 2, 7 );
				}
			}
			if(pev->frags == 830){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 840){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->angles.y = 45;
						pSpot->pev->velocity.x = 4;
						pSpot->pev->velocity.y = 4;
						pSpot->pev->velocity.z = -2;
						UTIL_SetOrigin( pSpot->pev, pev->origin + Vector(-256,-256,-16) );
					}
			}
			if(pev->frags == 850){
				sprintf( text, "Misaliya: (Kadoma?...)\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 890){
				
				sprintf( text, "Misaliya: Why do this?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 905){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 920){
				pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->avelocity.y = 10;
						pSpot->pev->velocity.x = -20;
						pSpot->pev->velocity.y = -20;
					}
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity2 ){
						FX_Explosion( pEntity2->Center(), EXPLOSION_SPARKSHOWER );
						pEntity2->pev->effects |= EF_NODRAW;
						EMIT_SOUND(ENT(pEntity2->pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
					}
			}
			if(pev->frags == 930){//kadoma A����ȥ
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
					if ( pEntity ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_jump" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(128,128,8) );
					pEntity2->pev->angles.y = 230;
					pEntity2->pev->effects &= ~EF_NODRAW;
					pEntity2->pev->movetype = MOVETYPE_FLY;
					pEntity2->pev->velocity.x = -16;
					pEntity2->pev->velocity.y = -16;
					pEntity2->pev->avelocity.y = 1;
					pEnemyMonster->SetBodygroup( 1, 0 );
					}
				}
			}
			if(pev->frags == 932){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity2 ){
						pEntity2->pev->flags |= FL_NOTARGET;
						pSpot->pev->origin = pEntity2->pev->origin + Vector(160,96,64);
						pSpot->pev->avelocity.y = -20;
						pSpot->pev->velocity.x = -50;
						pSpot->pev->velocity.y = -60;
						pSpot->pev->velocity.z = 4;
						pSpot->pev->angles.y = 180;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
						}
					}
			}
			if(pev->frags == 967){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity2 ){
						pEntity2->pev->velocity = g_vecZero;
						pEntity2->pev->avelocity = g_vecZero;
						pSpot->pev->origin = pEntity2->pev->origin + Vector(-24,-24,68);
						pSpot->pev->avelocity = g_vecZero;
						pSpot->pev->velocity.x = 4;
						pSpot->pev->velocity.y = 4;
						pSpot->pev->velocity.z = 0;
						pSpot->pev->angles.y = 50;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->v_angle = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
						}
					}
			}
			if(pev->frags == 980){//kadoma��������
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_jump_hitfly" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 990){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
						if ( pEntity2 ){
						pEntity2->pev->velocity = g_vecZero;
						pSpot->pev->origin = pEntity2->pev->origin + Vector(96,-128,64);
						pSpot->pev->avelocity.y = -25;
						pSpot->pev->avelocity.x = -5;
						pSpot->pev->angles.y = 140;
						pSpot->pev->angles.x = -5;
						pSpot->pev->velocity.x = 40;
						pSpot->pev->velocity.y = 80;
						pSpot->pev->velocity.z = 20;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
						}
					}
			}
			if(pev->frags == 1010){
				sprintf( text, "Misaliya: Kadoma!?\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1015){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->armortype = 12;
					}
			}
			if(pev->frags == 1030){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->velocity.x = 10;
						pSpot->pev->velocity.y = 15;
						pSpot->pev->velocity.z = -4;
						pSpot->pev->avelocity.y = -4;
						pSpot->pev->avelocity.x = 4;
						pSpot->pev->armortype = 4;
					}
			}
			if(pev->frags == 1050){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1060){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->armortype = 12;
						pSpot->pev->avelocity.x = 0;
						pSpot->pev->avelocity.y = 0;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 1061){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_alert = 100;
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_enemyfollower = 0;
				pEnemyMonster->m_enemyfollower_combat = 0;
				pEnemyMonster->m_no_pov_limit = 1;
				pEnemyMonster->m_aimflag_dist = 80.0;
				pEnemyMonster->SetYawSpeed();
				pEnemyMonster->m_user_aimflag = FALSE;
				pEntity->pev->angles.y = 45;
				}
			}
			if(pev->frags == 1070){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
				CBaseEntity *pEntity = Create( "npc_aim_flag", pEntity2->pev->origin + Vector(-16,-16,8), Vector(0,0,0), NULL );
				pEntity->pev->health = 50;
				pEntity->pev->frags = 6;
				}
			}
			if(pev->frags == 1075){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->armortype = 12;
						pSpot->pev->velocity.x = -15;
						pSpot->pev->velocity.z = -15;
					}
			}
			if(pev->frags == 1087){
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "heal_duck2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 45;
				pEnemyMonster->pev->yaw_speed = 0;
				}
			}
			if(pev->frags == 1100){
				
				sprintf( text, "Misaliya: You're hurt bad.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1135){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 1145){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1150){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity2 ){
						pSpot->pev->origin = pEntity2->pev->origin + Vector(128,128,72);
						pSpot->pev->angles.y = 225;
						pSpot->pev->armortype = 10;
						pSpot->pev->velocity.x = -20;
						pSpot->pev->velocity.y = -20;
						pSpot->pev->velocity.z = -4;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
						}
					}
			}
			if(pev->frags == 1160){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_draw" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
						}
			}
			if(pev->frags == 1175){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_aim" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
						}
			}
			if(pev->frags == 1180){
				
				sprintf( text, "Doma: I won't say. Die!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1205){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_shoot2" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 1220){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1229){
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "protect_kadoma" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 225;
				pEnemyMonster->pev->yaw_speed = 0;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(36,36,-93) );
				}
			}
			if(pev->frags == 1230){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
						if ( pEntity2 ){
						pSpot->pev->origin = pEntity2->pev->origin + Vector(-96,-96,55);
						pSpot->pev->angles.y = 45;
						pSpot->pev->armortype = 10;
						pSpot->pev->velocity.x = 24;
						pSpot->pev->velocity.y = 24;
						pSpot->pev->velocity.z = 0;
						pEntity2->pev->skin = 2;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
						}
					}
			}
			if(pev->frags == 1232){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_aim" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
						}
			}
			if(pev->frags == 1265){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_shoot2" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
							if ( pEntity2 ){
							pEnemyMonster->m_hEnemy = pEntity2;
							pEnemyMonster->m_vecEnemyLKP = pEntity2->pev->origin;
							pEntity2->pev->spawnflags |= SF_MONSTER_GAG;
							pEntity2->pev->health = 999;
							}
						}
			}
			if(pev->frags == 1270){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(120,-600,16);
						pSpot->pev->angles.y = 150;
						pSpot->pev->armortype = 10;
						pSpot->pev->velocity.x = 20;
						pSpot->pev->velocity.y = 20;
						pSpot->pev->avelocity.y = -5;
						pSpot->pev->velocity.z = 0;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 1330){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->angles.y = 225;
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							pSpot->pev->origin.x = pEntity->pev->origin.x + 64;
							pSpot->pev->origin.y = pEntity->pev->origin.y + 64;
							pSpot->pev->origin.z = pEntity->pev->origin.z + 48;
							pSpot->pev->velocity = g_vecZero;
							pSpot->pev->avelocity = g_vecZero;
						}
					}
			}
			if(pev->frags == 1340){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "gun_disarm" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
						}
						sprintf( text, "Doma: ......\n");
						UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1370){
				
				sprintf( text, "Doma: A useless gesture.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1390){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 1400){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity2 ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_stand" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEntity2->pev->angles.y = 225;
						UTIL_SetOrigin( pEntity2->pev, pEntity2->pev->origin + Vector(-180,-180,0) );
						}
					}
			}
			if(pev->frags == 1410){
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity2 ){
						pSpot->pev->origin = pEntity2->pev->origin + Vector(-128,-128,96);
						pSpot->pev->angles.y = 45;
						pSpot->pev->angles.x = 15;
						pSpot->pev->armortype = 10;
						pSpot->pev->velocity.x = 20;
						pSpot->pev->velocity.y = 20;
						pSpot->pev->velocity.z = -10;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;

						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->SetBodygroup( 0, 2 );
						pEnemyMonster->SetBodygroup( 1, 4 );
						pEnemyMonster->SetBodygroup( 2, 7 );
						}
					}
			}
			if(pev->frags == 1445){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity2 ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "holysword_skill" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						}
					}
			}
			if(pev->frags == 1453){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->velocity.z += 30;
					}
			}
			if(pev->frags == 1470){
					CBaseEntity::Create( "holy_valve_sword", pev->origin + Vector(0,0,768), g_vecZero, edict() );
			}
			if(pev->frags == 1475){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->angles.y = 225;
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
						if ( pEntity ){
							pSpot->pev->origin.x = pEntity->pev->origin.x + 96;
							pSpot->pev->origin.y = pEntity->pev->origin.y + 96;
							pSpot->pev->origin.z = pEntity->pev->origin.z + 64;
							pSpot->pev->velocity = Vector(20,20,20);
							pSpot->pev->avelocity = Vector(-2,0,0);
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
							pPlayer->m_sword_aim_origin = pEntity->pev->origin + Vector(0,0,768);
							pEntity->pev->health = 1000;
							pEntity->pev->spawnflags |= SF_MONSTER_GAG;
							UTIL_SetSize(pEntity->pev, Vector( -16, -16, 0), Vector(16, 16, 48));
						}
					}
			}
			if(pev->frags == 1495){
				
				sprintf( text, "Doma: What the!?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1515){
				
				sprintf( text, "Doma: Impossible! Where did you get that power!?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1535){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
						if ( pEntity2 ){
						pEntity2->pev->renderfx = 0;
						pEntity2->pev->effects = EF_NODRAW;
						}
			}
			if(pev->frags == 1550){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "kadoma_bao_idle" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;

						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity2 ){
						pEntity->pev->angles.y = 225;
						pEntity2->pev->angles.y = 225;
						UTIL_SetOrigin( pEntity->pev, pEntity2->pev->origin + Vector(-4,-4,48) );

						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "bao_idle" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetBodygroup( 2, 0 );
						}
					}
			}
			if(pev->frags == 1555){
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity2 ){
						pSpot->pev->origin = pEntity2->pev->origin + Vector(-96,-96,48);
						pSpot->pev->angles.y = 45;
						pSpot->pev->angles.x = 0;
						pSpot->pev->armortype = 10;
						pSpot->pev->velocity.x = 10;
						pSpot->pev->velocity.y = 10;
						pSpot->pev->velocity.z = 10;
						pSpot->pev->avelocity.x = 1;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
						}
					}
			}
			if(pev->frags == 1595){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity ){
						pEntity->pev->effects = EF_NODRAW;
					}
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity2 ){
						FX_Explosion( pEntity2->Center(), EXPLOSION_SPARKSHOWER );
						pEntity2->pev->effects = EF_NODRAW;
						EMIT_SOUND(ENT(pEntity2->pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
					}
			}
			if(pev->frags == 1600){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 1630){
				CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "next_map_target" );
				if ( pTel ){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity ){
					pEntity->pev->takedamage = DAMAGE_NO;
					UTIL_SetOrigin( pEntity->pev, pTel->pev->origin );
					}

					pPlayer->pev->origin = pTel->pev->origin - Vector(0,0,36);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->m_trainning = 0;
					pPlayer->EnableControl(TRUE);
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );

					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				}
				UTIL_Remove( this );
				return;
			}
	}
	if(pev->armortype == 51){//�¼�51 ����ĺ�������
			if(pev->frags == 1){
					pPlayer->pev->velocity = g_vecZero;
					pPlayer->pev->health = pPlayer->pev->max_health;

					pPlayer->EnableControl(FALSE);
					pPlayer->Clear_SayText();
					pPlayer->m_trainning = 1;
					pPlayer->m_wdoor_mynpc = NULL;
					pPlayer->m_guard_mynpc = 0;

					//�������
					pPlayer->TeamMate_Nagamatagi_Allclear(0);
					
					pPlayer->m_hasflashlight = FALSE;

					pPlayer->RemoveAllItems( FALSE );
			}
			if(pev->frags == 30){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 35){
					game_boss_battle = 0;
					pPlayer->m_music_save = 0;
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
							pSpot->pev->velocity.x = -10;
							pSpot->pev->avelocity.x = 1;
							pSpot->pev->velocity.z = -1;

							pPlayer->EnableControl(FALSE);
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							
							pPlayer->pev->angles = pSpot->pev->angles;
					}
			}
			if(pev->frags == 45){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lying_shinpori1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
			}
			if(pev->frags == 60){
					SERVER_COMMAND("mp3 play media/music9.mp3\n");
			}
			if(pev->frags == 100){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
							pSpot->pev->velocity.x = -40;
							pSpot->pev->avelocity.x = 5;
							pSpot->pev->velocity.z = -5;
					}
			}
			if(pev->frags == 150){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.z = 0;
							pSpot->pev->avelocity.x = 0;
					}
			}
			if(pev->frags == 160){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lying_shinpori2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
			}
			if(pev->frags == 190){
				
				sprintf( text, "???: Are you up?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 220){
				
				sprintf( text, "???: You slept a long time; I thought you'd never wake up.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 270){
				pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
							pSpot->pev->velocity.x = 10;
							pSpot->pev->velocity.y = -5;
							pSpot->pev->velocity.z = -4;
							pSpot->pev->avelocity.x = -2;
							pSpot->pev->origin.x -= 64;
							pSpot->pev->angles.y = 340;
					}
			}
			if(pev->frags == 300){
				
				sprintf( text, "Vanlve: I am Vanlve. We've met before. Do you remember?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 350){
				
				sprintf( text, "Vanlve: I run this place, as Dungeon Master.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 360){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.y = 0;
							pSpot->pev->velocity.z = 0;
							pSpot->pev->avelocity.x = 0;
					}
			}
			if(pev->frags == 390){
				
				sprintf( text, "Vanlve: I know why you came here.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 430){
				
				sprintf( text, "Vanlve: You can ask me some questions.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 470){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
							pSpot->pev->origin.x -= 64;
							pSpot->pev->origin.y += 32;
							pSpot->pev->origin.z -= 16;
							pSpot->pev->angles.y = 180;
					}
				pPlayer->m_fSelectMode = TRUE;
				pPlayer->m_fSelectNumber = 0;
				pPlayer->ShowVGUIMenu(36);
				pPlayer->m_load_check = 1;
			}
			if(pev->frags == 472){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 477){
				if(pPlayer->m_fSelectNumber == 0){
					if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
						pPlayer->Clear_SayText();
						pPlayer->EnableControl(FALSE);
						pPlayer->ShowVGUIMenu(36);
						pPlayer->m_load_check = 1;
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						}
					}
				pev->frags = 474;
				pev->team = -1;
				}
			}
		if(pev->frags == 485){
				pev->team = pPlayer->m_fSelectNumber;
				pPlayer->m_fSelectNumber = 0;
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 490){
			if(pev->team == 1){//kadoma ��
				
				sprintf( text, "Vanlve: I had to heal you, but you kept moving in your sleep.\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->team == 2){//kadoma ��
				
				sprintf( text, "Vanlve: You're New Nippori's new hero. You must save the world!\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->team == 3){//kadoma ��
				
				sprintf( text, "Vanlve: The girl?\n");
			
				UTIL_SayTextAll( text,this );
			}
			else if(pev->team == 4){//kadoma ��Ĭ
				sprintf( text, "Vanlve: ?\n");
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 540){
			if(pev->team == 1){//kadoma ��
				pPlayer->Clear_SayText();
				pev->frags = 474;
				pPlayer->m_load_check = 0;
				pPlayer->m_fSelectMode = TRUE;
			}
			else if(pev->team == 2){//kadoma ��
				
				sprintf( text, "Vanlve: The world is twisted, distorted. It has mixed with elements that didn't belong here.\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->team == 3){//kadoma ��
				
				sprintf( text, "Vanlve: The girl's dead. Sorry.\n");
				
				UTIL_SayTextAll( text,this );
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->angles.y = 330;
						pSpot->pev->origin.x += 32;
						pSpot->pev->origin.y -= 16;
					}
			}
			else if(pev->team == 4){//kadoma ��Ĭ
				pPlayer->Clear_SayText();
				pev->frags = 474;
				pPlayer->m_load_check = 0;
				pPlayer->m_fSelectMode = TRUE;
			}
		}
		if(pev->frags == 600){
			if(pev->team == 2){//kadoma ��
				pPlayer->Clear_SayText();
				pev->frags = 474;
				pPlayer->m_load_check = 0;
				pPlayer->m_fSelectMode = TRUE;
			}
			else if(pev->team == 3){//kadoma ��
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->angles.y = 180;
						pSpot->pev->origin.x += 96;
						pSpot->pev->origin.y += 64;
						pSpot->pev->angles.x = 0;
					}
			}
		}
		if(pev->frags == 610){//Kadoma ��������
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lying_shinpori3" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;

					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
				}
		}
		if(pev->frags == 640){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->angles.y = 270;
					pSpot->pev->origin.x += 32;
					pSpot->pev->origin.y += 8;

					pPlayer->pev->angles = Vector(0,270,0);
					pPlayer->pev->fixangle = TRUE;
				}
		}
		if(pev->frags == 642){//Kadoma ��������
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(80,0,-20) );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 1, 3 );
				}
		}
		if(pev->frags == 650){
				FireTargets( "kadoma_move_1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 670){
				FireTargets( "vanlve_move_1", this, this, USE_TOGGLE, 0 );
				
				sprintf( text, "Vanlve: Where are you going? I'm not done yet.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 710){
				pPlayer->Clear_SayText();

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
				pSpot->pev->velocity.x = 250;
				}

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "van_takeboy1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );

				FX_Explosion( pEntity2->Center(), EXPLOSION_SPARKSHOWER );
				EMIT_SOUND(ENT(pEntity2->pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);

				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(-10,-4,0) );
				pEntity2->pev->angles.y = 15;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "anime2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 725){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
			pSpot->pev->velocity.x = 0;
			pSpot->pev->velocity.y = -40;
			pSpot->pev->avelocity.y = -45;
			}
		}
		if(pev->frags == 745){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
			pSpot->pev->velocity.x = 0;
			pSpot->pev->velocity.y = 0;
			pSpot->pev->avelocity.y = 0;
			}
			
			sprintf( text, "Vanlve: Do you like that huh?\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 785){
			pPlayer->Clear_SayText();
			pPlayer->m_trainning = 0;
			pPlayer->EnableControl(TRUE);
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			pPlayer->pev->v_angle = Vector(0,0,0);
			pPlayer->pev->angles = Vector(0,0,0);
			pPlayer->pev->fixangle = TRUE;

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pSpot ){
				pPlayer->pev->origin = pSpot->pev->origin + Vector(0,0,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin + Vector(0,0,36);
				pSpot->pev->effects |= EF_NODRAW;
				pSpot->pev->solid = SOLID_NOT;
				}
			pPlayer->m_barnacle_RTP = 1;
			pPlayer->m_barnacle_Level = 0;

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
			if ( pEntity2 ){
				pPlayer->m_barnacle_catchme = pEntity2;
				pEntity2->pev->takedamage = DAMAGE_NO;
				pEntity2->pev->solid = SOLID_NOT;
			}
			
		}
		if(pev->frags >= 790 && pPlayer->m_barnacle_RTP == 1){
			pev->frags = 789;
			if(pPlayer->m_barnacle_RTP_bar >= 255){
					pev->frags = 791;
					pPlayer->m_barnacle_RTP_relase = 1;
					pPlayer->m_barnacle_draw_time = gpGlobals->time + 0.5;
					pPlayer->m_barnacle_RTP = 0;
					pPlayer->m_barnacle_RTP_bar = 0;
					pPlayer->m_barnacle_Level = 0;
					pPlayer->m_barnacle_catchme = NULL;

					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pPlayer->pev->origin.x += 96;
						pPlayer->EnableControl(FALSE);
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "van_takeboy2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->effects = 0;
					}
			}
		}
		if(pev->frags == 800){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "van_takedamage" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );

					pEntity2->pev->movetype = MOVETYPE_FLY;
					pEntity2->pev->solid = SOLID_NOT;
					pEntity2->pev->flags &= ~FL_ONGROUND;
					pEntity2->pev->velocity.x = -30;
					pEntity2->pev->velocity.z = 0;
					}
		}
		if(pev->frags == 820){
					SERVER_COMMAND( "mp3 stop\n" );

					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;
					pPlayer->pev->origin = pPlayer->pev->origin + Vector(384,0,36);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );

					pPlayer->m_game_rate = 71;//��Ϸ����71%

					UTIL_Remove( this );
					return;
		}
	}
	if(pev->armortype == 52){//�¼�52 Misaliya��ʬ��
			if(pev->frags == 1){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 5){
					game_boss_battle = 0;
					pPlayer->m_music_save = 0;
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							pSpot->pev->velocity.z = 2;
							
							pPlayer->pev->origin = pPlayer->pev->origin + Vector(0,256,0);
							pPlayer->m_stuck_origin = pPlayer->pev->origin + Vector(0,256,0);

							pPlayer->EnableControl(FALSE);
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							
							pPlayer->pev->angles = pSpot->pev->angles;
					}
			}
		    if(pev->frags == 10){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity ){
						UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin + Vector(0,160,-44) );
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle2" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEntity->pev->angles.y = 270;
						}
					}
			}
			if(pev->frags == 40){
				SERVER_COMMAND("mp3 play media/music16.mp3\n");
			}
			if(pev->frags == 100){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
							if ( mi ){
							pSpot->pev->origin = mi->pev->origin + Vector(16,0,24);
							pSpot->pev->velocity.x = -4;
							pSpot->pev->velocity.z = 0;
							pSpot->pev->angles.x = 90;
							pSpot->pev->angles.y = 180;
							}
					}
			}
			if(pev->frags == 215){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->velocity.x = 0;
					}
			}
			 if(pev->frags == 230){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
						if ( pEntity2 ){
						UTIL_SetOrigin( pEntity2->pev,  pEntity->pev->origin + Vector(96,96,0) );
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fa_stand" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEntity2->pev->angles.y = 270;
						pEntity2->pev->velocity = g_vecZero;
						pEntity2->pev->solid = SOLID_SLIDEBOX;
						}
					}
			}
			if(pev->frags == 240){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *ka = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
							if ( ka ){
							pSpot->pev->origin = ka->pev->origin + Vector(0,-128,72);
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.z = 0;
							pSpot->pev->angles.x = 0;
							pSpot->pev->angles.y = 90;
							}
					}
			}
			if(pev->frags == 260){
				
				sprintf( text, "Vanlve: She isn't as strong as you.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 320){
				
				sprintf( text, "Vanlve: There's no healing her.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 390){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 430){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							pSpot->pev->origin = pSpot->pev->origin + Vector(64,128,0);
							pSpot->pev->angles.y = 60;
					}
			}
			if(pev->frags == 440){
				
				sprintf( text, "Vanlve: But it can't end here, right?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 490){
				
				sprintf( text, "Vanlve: Can't end here.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 550){
				pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *ka = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
							if ( ka ){
							pSpot->pev->origin = ka->pev->origin + Vector(0,1500,72);
							pSpot->pev->velocity.y = 10;
							pSpot->pev->angles.y = 90;
							}
					}
			}
			if(pev->frags == 575){
					FireTargets( "wrong_door", this, this, USE_TOGGLE, 0 );
					
					sprintf( text, "Vanlve: The Wrong Door is open. God is inviting you in.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 625){
				
				sprintf( text, "Vanlve: Pass the test, and there may be a miracle.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 675){
				  pPlayer->Clear_SayText();
			}
			if(pev->frags == 685){
					
					sprintf( text, "Vanlve: Go on, Boy Next Door!\n");
					
				    UTIL_SayTextAll( text,this );

					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
						if ( pEntity ){
						UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(0,128,0) );
						pEntity->pev->angles.y = 90;
						pSpot->pev->origin = pEntity->pev->origin + Vector(0,-128,72);
						pSpot->pev->velocity.y = 0;
						}
					}
			}
			if(pev->frags == 690){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!VAN_1", VOL_NORM, 0.1, 0, PITCH_NORM );
					}
			}
			if(pev->frags == 735){
				  pPlayer->Clear_SayText();
				  FireTargets( "freeze_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 760){
					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );

					FireTargets( "freeze_door", this, this, USE_TOGGLE, 0 );

					pPlayer->GiveNamedItem( "weapon_fist" );//��ʣȭͷ��

					pPlayer->pev->origin = pPlayer->pev->origin + Vector(0,384,0);
					pPlayer->m_stuck_origin = pPlayer->pev->origin + Vector(0,384,0);
					pPlayer->pev->v_angle = Vector(0,90,0);
					pPlayer->pev->angles = Vector(0,90,0);
					pPlayer->pev->fixangle = TRUE;

					pPlayer->m_game_rate = 72;//��Ϸ����72%
			}
			if(pev->frags == 770){
				SERVER_COMMAND( "autosave\n" );//Bug Fix 3.0 �Զ��浵����Wrong Roadɱ
			}
			if(pev->frags == 780){
				UTIL_Remove( this );
				return;
			}
	}
	if(pev->armortype == 53){//�¼�53 WrongDoor������1
			if(pev->frags == 1){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.0, 4.0, 255, FFADE_OUT );
			}
			if(pev->frags == 20){
			pPlayer->pev->origin = pev->origin + Vector(0,96,0);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			}
			if(pev->frags == 40){
			sprintf( text, "???: Kadoma......\n");
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 80){
				
				sprintf( text, "???: You're lost in darkness, without light or hope...\n");
				
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 120){
				
				sprintf( text, "???: You can't go on without her, can you?\n");
				
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 170){
				
				sprintf( text, "???: Behold the light I've lit for you. Go there.\n");
				
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 180){
			FireTargets( "dark_tg_wall_1", this, this, USE_TOGGLE, 0 );
			pPlayer->pev->v_angle = Vector(0,90,0);
			pPlayer->pev->angles = Vector(0,90,0);
			pPlayer->pev->fixangle = TRUE;
			pPlayer->EnableControl(TRUE);
			}
			if(pev->frags == 240){
			pPlayer->Clear_SayText();
			UTIL_Remove( this );
			return;
			}
	}
	if(pev->armortype == 54){//�¼�54 WrongDoor������2
			if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 10){
				
				sprintf( text, "???: Do you hear the shrieks? It's them.\n");
				
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 50){
				
				sprintf( text, "???: They come. Fight for what's important to you!\n");
				
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 60){
			FireTargets( "making_dark_ms", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 80){
			pPlayer->m_music_save = 8;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 14\n");
			//SERVER_COMMAND("mp3 loop media/music17.mp3\n");
			}
			if(pev->frags == 100){
				
				sprintf( text, "???: Kill kill kill kill kill!\n");
				
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 160){
			pPlayer->Clear_SayText();
			UTIL_Remove( this );
			return;
			}
	}
	if(pev->armortype == 55){//�¼�55 WrongDoor������3
			if(pev->frags == 30){
				if(pPlayer->m_fValve){
					
					sprintf( text, "???: Place the valve, and restore the holy sword's power!\n");
					
				UTIL_SayTextAll( text,this );
				FireTargets( "light_spot_center_e", this, this, USE_TOGGLE, 0 );
				FireTargets( "dark_tg_wall_2", this, this, USE_TOGGLE, 0 );
				UTIL_Remove( this );
				return;
				}
				else{//���Ÿ������� Bug Fix 1.0
					if(pev->impulse == 2){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_eatkey_valve");
						if ( pSpot ){
						pSpot->pev->effects |= EF_LIGHT;
						}
						pev->impulse = 3;
					}
					else{
						pev->impulse += 1;
					}
					pev->frags = 10;
				}
			}
	}
	if(pev->armortype == 56){//�¼�56 WrongDoor������4
			if(pev->frags == 20){
				
				sprintf( text, "???: Feel holy sword's true power!\n");
			
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 50){
					pPlayer->m_skill_valvesword = 5;
					
					sprintf( text, "- Kadoma Learned a Skill (Holy Swordplay)\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 80){
					
					sprintf( text, "- Reload when holding the holy sword to launch a special attack.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 130){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						UTIL_SetOrigin( pPlayer->pev, pSpot->pev->origin);
					}
			}
			if(pev->frags == 150){
			pPlayer->m_music_save = 0;
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 170){
				
				sprintf( text, "Valve Holy Sword: Destroy the enemy with your new power!\n");
			
			UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 220){
			SERVER_COMMAND("mp3 stop\n");

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			FireTargets( "chainsaw_boss", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 250){
			pPlayer->BOSS_Find();
			pPlayer->Clear_SayText();
			pPlayer->m_music_save = 9;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 23\n");
			//SERVER_COMMAND("mp3 loop media/boss5.mp3\n");
			UTIL_Remove( this );
			return;
			}
	}
	if(pev->armortype == 57){//�¼�57 WrongDoor������5
			if(pev->frags == 0){
				pPlayer->m_music_save = 0;
				SERVER_COMMAND("mp3 play media/music32.mp3\n");
			}
			if(pev->frags == 60){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 80){
				FireTargets( "water_on", this, this, USE_TOGGLE, 0 );
				UTIL_Remove( this );
				return;
			}
	}
	if(pev->armortype == 58){//�¼�58 WrongDoor������6 ���� ����
			if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 10){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 4.0, 255, FFADE_OUT );
			}
			if(pev->frags == 50){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 4, 255, FFADE_IN );//��Ϲ���
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
					if ( pSpot ){
						UTIL_SetOrigin( pPlayer->pev, pSpot->pev->origin);
					}
					pPlayer->EnableControl(FALSE);
			}
			if(pev->frags == 60){
					game_boss_battle = 0;
					pPlayer->m_music_save = 0;
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
							pSpot->pev->velocity.z = 4;
							pSpot->pev->velocity.x = 4;
							pSpot->pev->avelocity.x = 1;

							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							
							pPlayer->pev->angles = pSpot->pev->angles;

							CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
							if ( pEntity ){
							UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin + Vector(-128,0,-62) );
							pEntity->pev->angles.y = 0;
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_aim" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->SetState( MONSTERSTATE_HUNT );
							pEnemyMonster->SetBodygroup( 2, 7 );
							}
					}
			}
			if(pev->frags == 90){
				SERVER_COMMAND("mp3 play media/music15.mp3\n");
			}
			if(pev->frags == 120){
						CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
						if ( pTel ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pTel->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_1" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pTel->pev->effects = EF_BRIGHTLIGHT | EF_BRIGHTFIELD;
						}
			}
			if(pev->frags == 150){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
							pSpot->pev->velocity.z = 30;
							pSpot->pev->velocity.x = 70;
							pSpot->pev->avelocity.x = 2;
							pSpot->pev->angles.x = 0;
							pSpot->pev->angles.y = 0;

							pPlayer->pev->v_angle = pSpot->pev->angles;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 195){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						pSpot->pev->velocity.z = 0;
					}
			}
			if(pev->frags == 230){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						pSpot->pev->velocity.x = 0;
					}
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
					if ( pTel ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pTel->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_2" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					}
			}
			if(pev->frags == 250){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
							pSpot->pev->avelocity.x = 0;
					}
			}
			if(pev->frags == 260){
						CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
						if ( pTel ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pTel->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_3" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						}
					
					sprintf( text, "???: Congratulations on passing the Wrong Door test.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 300){
				
					sprintf( text, "???: You're the second one who came here. I've been watching you, Kadoma.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 340){
						CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
						if ( pTel ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pTel->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_4" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						}
					
					sprintf( text, "Z.Z: I am Z.Z - God! The world's creator!\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 350){
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
					if ( pTel ){
						FX_Explosion( pTel->Center(), 42);
					}

					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(512,0,89) );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					}
			}
			if(pev->frags == 380){
						CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
						if ( pTel ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pTel->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_5" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						}
					
					sprintf( text, "Z.Z: My power is not what it was, but I can still grant your wish!\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 450){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						pSpot->pev->angles.x = 20;
						pSpot->pev->angles.y = 180;
						pPlayer->Clear_SayText();
					}
			}
			if(pev->frags == 480){
						CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
						if ( pTel ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pTel->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_3" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						}
			}
			if(pev->frags == 500){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot ){
						pSpot->pev->angles.x = 0;
						pSpot->pev->angles.y = 0;
					}
					
					sprintf( text, "Z.Z: You want the dead to return?\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 540){
					
					sprintf( text, "Z.Z: This will affect the world greatly. Even so.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 600){
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
					if ( pTel ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pTel->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_6" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );

						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
						pSpot->pev->velocity.x = -10;
						}
					}

					
					sprintf( text, "Z.Z: You shall have your wish!\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 610){
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
					if ( pTel ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pTel->MyMonsterPointer();

					Vector vecStart, angleGun;
					pEnemyMonster->GetAttachment( 0, vecStart, angleGun );
					FX_Trail(pEnemyMonster->pev->origin, pEnemyMonster->entindex(), PROJ_SUNOFGOD2);
					}
			}
			if(pev->frags == 620){
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 630){
					UTIL_ScreenFade( pPlayer, Vector(255,255,255), 4.0, 4.0, 255, FFADE_OUT );
			}
			if(pev->frags == 675){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
							if ( mi ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 4, 255, FFADE_IN );//��Ϲ���
							pSpot->pev->origin = mi->pev->origin + Vector(-32,0,24);
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.y = 0;
							pSpot->pev->velocity.z = 0;
							pSpot->pev->angles.x = 90;
							pSpot->pev->angles.y = 180;
							pPlayer->Clear_SayText();
							}
					}
			}
			if(pev->frags == 680){
					SERVER_COMMAND( "mp3 stop\n" );
			}
			if(pev->frags == 740){
						CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
						if ( mi ){
						mi->pev->skin = 6;
						}
			}
			if(pev->frags == 770){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
							if ( mi ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = mi->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sleep2" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->SetState( MONSTERSTATE_HUNT );
							}
					}
			}
			if(pev->frags == 785){
						CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
						if ( mi ){
						mi->pev->skin = 7;
						}
			}
			if(pev->frags == 788){
						CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
						if ( mi ){
						mi->pev->skin = 0;
						}
			}
			if(pev->frags == 810){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
							if ( mi ){
							pSpot->pev->origin = mi->pev->origin + Vector(0,-96,48);
							pSpot->pev->angles.x = 0;
							pSpot->pev->angles.y = 90;
							}
					}

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					pEntity2->pev->effects = EF_NODRAW;
					pEntity2->pev->solid = SOLID_NOT;
					pEntity2->pev->flags |= FL_NOTARGET;
					pEntity2->pev->takedamage = DAMAGE_NO;
					}

					FireTargets( "misaliya_glass", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 820){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
							if ( mi ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = mi->MyMonsterPointer();
							pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dead_sit" );
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->SetBodygroup( 2, 1 );
							pEnemyMonster->SetBodygroup( 3, 1 );
							pEnemyMonster->SetBodygroup( 7, 1 );
							}
					}
			}
			if(pev->frags == 860){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin + Vector(32,256,0) );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;

					pEntity->pev->velocity.z = -10;
					pEntity->pev->angles.y = 270;

						EMIT_SOUND(ENT(pEntity->pev), CHAN_WEAPON, "debris/beamstart2.wav", 1, 0.7);	
						int teleb = PRECACHE_MODEL("sprites/b-tele1.spr");
						MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pEntity->pev->origin);
						WRITE_BYTE(3);
						WRITE_COORD( pEntity->pev->origin.x);
						WRITE_COORD( pEntity->pev->origin.y);
						WRITE_COORD( pEntity->pev->origin.z + 32);
						WRITE_SHORT(teleb);
						WRITE_BYTE(15);
						WRITE_BYTE(15);
						WRITE_BYTE(4);
						MESSAGE_END();
					}
				}
			}
			if(pev->frags == 900){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_Remove( pEntity );
					}
					CBaseEntity *mi = UTIL_FindEntityByTargetname( NULL, "misaliya_dead" );
					if ( mi ){
					UTIL_Remove( mi );
					}
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 905){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pPlayer->edict() );
							
							pPlayer->pev->health = pPlayer->pev->max_health;

							pPlayer->pev->origin = pSpot->pev->origin + Vector(32,256,0);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							pPlayer->pev->v_angle = Vector(0,270,0);
							pPlayer->pev->angles = Vector(0,270,0);
							pPlayer->pev->fixangle = TRUE;
							pPlayer->Clear_SayText();
							pPlayer->m_trainning = 0;
							pPlayer->EnableControl(TRUE);

							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
							if ( pEntity2 ){
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pEntity2->MyMonsterPointer();
									pEnemyMonster->SetState( MONSTERSTATE_HUNT );
									pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "hurt_idle" );
									pEnemyMonster->ResetSequenceInfo( );
									pEnemyMonster->pev->frame = 0;
									pEnemyMonster->m_playerguardian_mode = 0;
									pEnemyMonster->pev->flags &= ~FL_NOTARGET;
									pEnemyMonster->pev->effects &= ~EF_NODRAW;
									pEnemyMonster->m_hEnemy = NULL;
									pEnemyMonster->m_hOldEnemy[0] = NULL;
									pEnemyMonster->m_hOldEnemy[1] = NULL;
									pEnemyMonster->m_hOldEnemy[2] = NULL;
									pEnemyMonster->m_hOldEnemy[3] = NULL;
									pEntity2->pev->owner = NULL;
									pEntity2->pev->angles.y = 90;
									pEntity2->pev->deadflag = DEAD_NO;
									pEntity2->pev->spawnflags = 0;
									pEnemyMonster->m_die = 0;
									pEnemyMonster->m_dieseq	= 0;
									pEntity2->pev->health = pEntity2->pev->max_health;//��Ѫ����
									pEnemyMonster->m_enemyfollower = 0;
									pEnemyMonster->pev->yaw_speed = 0;
									pEntity2->pev->takedamage = DAMAGE_NO;
									pEntity2->pev->velocity.z = -10;
									UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(0,150,0) );
									pEntity2->pev->skin = 1;
							}
					}
			}
			if(pev->frags == 940){
				SERVER_COMMAND("mp3 play media/music18.mp3\n");
			}
			if(pev->frags == 945){
				
				sprintf( text, "Misaliya: Kadoma, thanks for the help.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 990){
				
				sprintf( text, "Misaliya: I saw it all. Sorry for being a bother.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1050){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity2 ){
					pEntity2->pev->skin = 7;
				}
				
				sprintf( text, "Misaliya: I'm okay. I'll do better next time.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1120){
				pPlayer->Clear_SayText();
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
							if ( pEntity2 ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity2->MyMonsterPointer();
							pEnemyMonster->SetActivity( ACT_IDLE );
							pEnemyMonster->SetState( MONSTERSTATE_IDLE );
							pEnemyMonster->m_enemyfollower = 1;
							pEnemyMonster->SetYawSpeed();
							pPlayer->TeamMate_add(pEnemyMonster);//�������
							pEntity2->pev->takedamage = DAMAGE_YES;
							pEnemyMonster->ClearSchedule();
							pEntity2->pev->skin = 0;
							}
			}
			if(pev->frags == 1200){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 4.0, 255, FFADE_OUT );
			}
			if(pev->frags == 1260){
			pPlayer->m_game_rate = 73;//��Ϸ����73%
			FireTargets( "get_the_shinopori", this, this, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
			}
	}
	if(pev->armortype == 59){//�¼�59 ��������ĺ��
			if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 10){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 4, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 15){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
						UTIL_SetOrigin( pPlayer->pev, pSpot->pev->origin + Vector(0,512,0));
						pSpot->pev->angles.y = 90;
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}

					pPlayer->m_wdoor_mynpc = NULL;
					pPlayer->m_guard_mynpc = 0;

					pPlayer->EnableControl(FALSE);
			}
			if(pev->frags == 20){
					game_boss_battle = 0;
					pPlayer->m_music_save = 0;
					pPlayer->Clear_SayText();

						CBaseEntity *pEntity = Create( "monster_kadoma2", pev->origin, Vector(0,270,0), NULL );

						if ( pEntity ){
						UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-16,0,0) );
						pEntity->pev->angles.y = 270;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->SetBodygroup( 1, 0 );
						pEnemyMonster->SetBodygroup( 2, 0 );
						}
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
						if ( pEntity2 ){
						UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(16,0,0) );
						pEntity2->pev->angles.y = 270;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->SetBodygroup( 1, 0 );
						pEnemyMonster->SetBodygroup( 2, 0 );
						pEntity2->pev->skin = 0;

						pEnemyMonster->pev->effects &= ~EF_NODRAW;
						pEnemyMonster->pev->velocity = g_vecZero;
						pEnemyMonster->pev->yaw_speed = 0;
						pEnemyMonster->RouteClear();
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->m_playerguardian_mode = 0;
						pEnemyMonster->pev->flags &= ~FL_NOTARGET;
						pEntity2->pev->owner = NULL;
						}
			}
			if(pev->frags == 70){
				
					sprintf( text, "Misaliya: This is New Nippori?\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 110){
				
					sprintf( text, "Misaliya: This is my first time too.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 170){
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
					pSpot->pev->origin.y += 192;
					pSpot->pev->angles.y = 270;
					pSpot->pev->velocity.z = 20;
					}
			}
			if(pev->frags == 250){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 3.0, 255, FFADE_OUT );
			}
			if(pev->frags == 280){
					CBaseEntity *pEntity = NULL;
					CBaseMonster *pMonster;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if( FStrEq(STRING(pEntity->pev->targetname), "surive_npcs")){
							pMonster = pEntity->MyMonsterPointer( );
							if(pMonster){
							pMonster->m_rpgms_inteam = 5;
							pMonster->m_longming = 1;
							pMonster->m_selfmode = TRUE;
							pMonster->m_nevergibmode = TRUE;
							}
						}
					}
			}
			if(pev->frags == 290){
							pPlayer->Clear_SayText();
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
							if ( pEntity2 ){
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity2->MyMonsterPointer();
							pEnemyMonster->SetActivity( ACT_IDLE );
							pEnemyMonster->SetState( MONSTERSTATE_IDLE );
							pEnemyMonster->m_enemyfollower = 1;
							pEnemyMonster->SetYawSpeed();
							pEnemyMonster->ClearSchedule();
							}
							CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
							if ( pEntity ){
							UTIL_Remove( pEntity );
							}
							CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
							if ( pSpot ){
							pSpot->pev->velocity = g_vecZero;
							}
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
							pPlayer->pev->origin = pev->origin + Vector(-16,0,36);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							pPlayer->m_trainning = 0;
							pPlayer->EnableControl(TRUE);
							SET_VIEW( pPlayer->edict(), pPlayer->edict() );

							pPlayer->GiveNamedItem( "weapon_egon" );
							pPlayer->m_hasflashlight = TRUE;
							pPlayer->MenuItem_add(1);//�ֵ�ͲGet
							pPlayer->pev->health = pPlayer->pev->max_health;
			}
			if(pev->frags == 300){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					pEntity2->pev->movetype = MOVETYPE_NONE;
					pEntity2->pev->solid	= SOLID_NOT;
					pEntity2->pev->effects &= ~EF_NODRAW;
					pEntity2->pev->velocity = g_vecZero;

					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_ddf3" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );

					pEnemyMonster->pev->yaw_speed = 0;
					pEntity2->pev->angles.y = 90;

						CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "vanlve_sit_point" );
						if ( pTel ){
						pTel->pev->effects |= EF_LIGHT;
						UTIL_SetOrigin( pEntity2->pev, pTel->pev->origin );
						}
					}
			}
			if(pev->frags == 310){

				//Bug Fix 3.0 ����ǽ��С����
				CBaseEntity *pAirWall = Create( "wrongdoor_airwall", Vector(-3840,968,2328), Vector(0,0,0), NULL );
				SET_MODEL( ENT(pAirWall->pev), "models/props_all.mdl" );
				pAirWall->pev->body = 7;
				UTIL_SetSize ( pAirWall->pev, Vector(-32,-32,-64), Vector(32,32,128));
				
						FireTargets( "aniki_ally_door", this, this, USE_TOGGLE, 0 );
						pPlayer->m_music_save = 11;
						SERVER_COMMAND("mp3 loop media/music19.mp3\n");
			}
			if(pev->frags == 320){
						SERVER_COMMAND( "autosave\n" );
						UTIL_Remove( this );
						return;
			}
	}
	if(pev->armortype == 60){//�¼�60 Misaliya��Van����
			if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 10){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 15){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->origin.y += 32;
						UTIL_SetOrigin( pPlayer->pev, pSpot->pev->origin);
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}

					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "vanlve_sit_point" );
					if ( pTel ){
					pTel->pev->effects = 0;
					}

					pPlayer->m_wdoor_mynpc = NULL;
					pPlayer->m_guard_mynpc = 0;

					pPlayer->EnableControl(FALSE);
			}
			if(pev->frags == 20){
					pPlayer->Clear_SayText();

						CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin, Vector(0,90,0), NULL );

						if ( pEntity ){
						UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-32,0,0) );
						pEntity->pev->angles.y = 90;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->SetBodygroup( 1, 0 );
						pEnemyMonster->SetBodygroup( 2, 0 );
						}
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
						if ( pEntity2 ){
						UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(32,0,0) );
						pEntity2->pev->angles.y = 90;
						pEntity2->pev->health = pEntity2->pev->max_health;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->SetBodygroup( 1, 0 );
						pEnemyMonster->SetBodygroup( 2, 0 );
						pEntity2->pev->skin = 0;
						pEnemyMonster->ClearSchedule();

						pEnemyMonster->pev->effects &= ~EF_NODRAW;
						pEnemyMonster->pev->velocity = g_vecZero;
						pEnemyMonster->pev->yaw_speed = 0;
						pEnemyMonster->RouteClear();
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->m_playerguardian_mode = 0;
						pEnemyMonster->pev->flags &= ~FL_NOTARGET;
						pEntity2->pev->owner = NULL;
						}
			}
			if(pev->frags == 40){//���һЩʵ�壨������
				CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "god_cshl623" );
				if ( pTel ){
				UTIL_Remove( pTel );
				}
				pTel = UTIL_FindEntityByClassname( NULL, "monster_chainsaw_boss" );
				if ( pTel ){
				UTIL_Remove( pTel );
				}		
			}
			if(pev->frags == 70){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->origin.y += 512;
						pSpot->pev->origin.z -= 32;
						pSpot->pev->angles.y = 270;
					}
					
					sprintf( text, "Vanlve: What do you think of the place?\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 120){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 125){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->origin.x += 128;
						pSpot->pev->origin.y -= 192;
						pSpot->pev->angles.y = 180;
					}
					
					sprintf( text, "Misaliya: Alright?\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 170){
		
					sprintf( text, "Misaliya: It feels familiar.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 220){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 225){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->origin.x -= 256;
						pSpot->pev->origin.y += 192;
						pSpot->pev->angles.y = 0;
					}
					
					sprintf( text, "Vanlve: Indeed.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 255){
				
					sprintf( text, "Vanlve: There's something I'd like to request.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 295){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
						pSpot->pev->angles.y = 90;
						pSpot->pev->angles.x = 10;
						pSpot->pev->velocity.y = 10;
						pSpot->pev->avelocity.x = 1;
						pSpot->pev->origin.y -= 384;
						pSpot->pev->origin.z += 128;
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 300){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fa_stand" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					}
			}
			if(pev->frags == 310){
					FireTargets( "big_blackmesa_build", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 360){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
						pSpot->pev->avelocity.x = 0;
					}
			}
			if(pev->frags == 400){
				FireTargets( "blackmesa_build_tg", this, this, USE_TOGGLE, 0 );
				FireTargets( "big_hecu_gate_door", this, this, USE_TOGGLE, 0 );

				pPlayer->m_music_save = 0;
				SERVER_COMMAND("mp3 stop\n");

				CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "big_blackmesa_build" );
				if ( pTel ){
				UTIL_Remove( pTel );
				}
			}
			if(pev->frags == 450){
				FireTargets( "hecu_blackmesa_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 480){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
						pSpot->pev->origin.y += 512;
						pSpot->pev->angles.x = 20;
						pSpot->pev->angles.y = 270;
						pSpot->pev->velocity.y = -10;

						pPlayer->pev->v_angle = pSpot->pev->angles;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}

					FireTargets( "break_gate_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 600){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->origin.x += 128;
						pSpot->pev->angles.y = 270;
						UTIL_SetOrigin( pPlayer->pev, pSpot->pev->origin);
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!VAN_2", VOL_NORM, 0.1, 0, PITCH_NORM );

					
					sprintf( text, "Vanlve: Shit! It's those damn intruders again!\n");
					
					UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 650){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 660){
					pPlayer->Clear_SayText();

					pPlayer->m_music_save = 12;
					CLIENT_COMMAND(pPlayer->edict(), "cd loop 3\n");
					//SERVER_COMMAND("mp3 loop media/music20.mp3\n");

					CBaseMonster *pEnemyMonster;
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity2 ){
						UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(150,900,-588) );
						pEntity2->pev->angles.y = 90;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						pEnemyMonster->Hunt_Stand_Set(2);//Bug Fix 3.0 Misaliya����ƺ���Щ��̫������
						pEntity2->pev->frags = 1;
					}

					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
					UTIL_SetOrigin( pPlayer->pev, pev->origin + Vector(100,900,-550));
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					pPlayer->EnableControl(TRUE);
					pPlayer->GiveNamedItem( "weapon_medkit" );
					pPlayer->GiveNamedItem( "weapon_deagle" );
					pPlayer->GiveNamedItem( "weapon_ak47" );
					pPlayer->GiveNamedItem( "weapon_rpg" );
					pPlayer->GiveNamedItem( "weapon_sniperrifle" );
					pPlayer->GiveNamedItem( "weapon_satchel" );

					pPlayer->pev->health = pPlayer->pev->max_health;

					pPlayer->GiveNamedItem( "item_armor2" );
					pPlayer->GiveNamedItem( "item_armor2" );

					pPlayer->m_flVelocityModifier = -3;

					FireTargets( "door_stuck_wall", this, this, USE_TOGGLE, 0 );

					pPlayer->m_game_rate = 74;//��Ϸ����74%
			}
			if(pev->frags == 680){
					pPlayer->GiveAmmo( 2, "kmedkit", MEDKIT_MAX_CARRY );
					pPlayer->GiveAmmo( 35, "357", _357_MAX_CARRY );
					pPlayer->GiveAmmo( 180, "762nato", AK_MAX_CARRY );
					pPlayer->GiveAmmo( 5, "rockets", ROCKET_MAX_CARRY );
					pPlayer->GiveAmmo( 30, "338mag", SNIPER_MAX_CARRY );
					pPlayer->GiveAmmo( 4, "Satchel Charge", SATCHEL_MAX_CARRY );
					//pPlayer->MenuItem_equip(1);//װ�������û���	
					//Bug Fix 3.0 �ƺ�û��Ҫ���װ���ˣ�
			}
			if(pev->frags == 690){
					FireTargets( "aniki_ally_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 700){
					FireTargets( "aniki_grunt_ally_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 720){
				
					sprintf( text, "Vanlve: Hold out and defend the residents of the city!\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 780){
				
					sprintf( text, "Vanlve: Stay alive, and pull back if you must. There are medkits and cover back here.\n");
			
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 860){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 1300){
					FireTargets( "blackmesa_build_tg_duck", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1320){
					FireTargets( "hecu_maker_door", this, this, USE_TOGGLE, 0 );
					FireTargets( "hecu_blackmesa_maker2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1920){
					FireTargets( "apache_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 2680){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4, 6, 255, FFADE_IN );//��Ϲ���
					pPlayer->m_wdoor_mynpc = NULL;
					pPlayer->m_guard_mynpc = 0;
					pPlayer->m_trainning = 1;
					pPlayer->m_music_save = 0;

					pPlayer->EnableControl(FALSE);

					SERVER_COMMAND("mp3 stop\n");

					pPlayer->m_wdoor_mynpc = NULL;
					pPlayer->m_guard_mynpc = 0;

					FireTargets( "hecu_blackmesa_maker", this, this, USE_TOGGLE, 0 );
					FireTargets( "hecu_blackmesa_maker2", this, this, USE_TOGGLE, 0 );
					FireTargets( "aniki_grunt_ally_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 2685){
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if (  (pEntity->pev->flags & FL_MONSTER) ){
								if(FClassnameIs ( pEntity->pev, "monster_human_grunt" )
								|| FClassnameIs ( pEntity->pev, "monster_human_grunt_medic" )
								|| FClassnameIs ( pEntity->pev, "monster_human_grunt_torch" )
								|| FClassnameIs ( pEntity->pev, "monster_human_assault" )
								|| FClassnameIs ( pEntity->pev, "monster_human_grunt_ally" )
								|| FClassnameIs ( pEntity->pev, "monster_apache" )){

								if(FClassnameIs ( pEntity->pev, "monster_apache" )){
								STOP_SOUND( ENT(pEntity->pev), CHAN_STATIC, "apache/ap_rotor2.wav" );
								}

								UTIL_Remove( pEntity );
								}
						}
					}
			}
			if(pev->frags == 2692){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_ddf3" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					}

					CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
						if ( pEntity3 ){
						UTIL_SetOrigin( pEntity3->pev, pev->origin + Vector(32,0,0) );
						pEntity3->pev->angles.y = 90;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity3->MyMonsterPointer();

						if(pEntity3->pev->deadflag == DEAD_NO){
						pPlayer->m_ending_frags += 5;//Misaliya����Ʒֵ+5!
						}

						pEnemyMonster->m_die = 0;
						pEnemyMonster->m_dieseq	= 0;
						pEntity3->pev->health = pEntity3->pev->max_health;
						pEntity3->pev->deadflag = DEAD_NO;
						pEntity3->pev->rendermode = 0;
						pEntity3->pev->movetype = MOVETYPE_STEP;
						pEntity3->pev->takedamage = DAMAGE_YES;
						pEntity3->pev->solid = SOLID_BBOX;
						pEnemyMonster->ClearSchedule();

						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "hurt_idle" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEntity3->pev->skin = 0;

						pEnemyMonster->pev->effects &= ~EF_NODRAW;
						pEnemyMonster->pev->velocity = g_vecZero;
						pEnemyMonster->pev->yaw_speed = 0;
						pEnemyMonster->RouteClear();
						pEnemyMonster->m_hEnemy = NULL;
						pEnemyMonster->m_hOldEnemy[0] = NULL;
						pEnemyMonster->m_hOldEnemy[1] = NULL;
						pEnemyMonster->m_hOldEnemy[2] = NULL;
						pEnemyMonster->m_hOldEnemy[3] = NULL;
						pEnemyMonster->m_playerguardian_mode = 0;
						pEnemyMonster->pev->flags &= ~FL_NOTARGET;
						pEntity3->pev->owner = NULL;
					}
			}
			if(pev->frags == 2695){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
					if ( pSpot ){
						pSpot->pev->angles.y = 270;
						UTIL_SetOrigin( pPlayer->pev, pSpot->pev->origin);
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}
			}
			if(pev->frags == 2700){
					Vector dead_org;
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "dead_corpse_sitpoint" );
					if ( pTel ){
						dead_org = pTel->pev->origin;
					}
					CBaseEntity *pEntity = NULL;
					CBaseMonster *pMonster;
					int surive_num = 0;
					int dead_num = 0;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if( FStrEq(STRING(pEntity->pev->targetname), "surive_npcs")){
							pMonster = pEntity->MyMonsterPointer( );
							if(pMonster){
								pEntity->pev->health = pEntity->pev->max_health;

								if(pEntity->pev->deadflag == DEAD_NO){
								surive_num++;
								}
								else{
									pMonster->m_nevergibmode = FALSE;

									pEntity->pev->angles.y = 0;
									UTIL_SetOrigin( pEntity->pev, dead_org);
									dead_org.y += 96;
									dead_num++;

									if(dead_num == 6 || dead_num == 12){
									dead_org.x += 256;
									dead_org.y -= 576;
									}
								}
							}
						}
					}
				
					sprintf( text, "- Survived: %d Died: %d\n",surive_num,dead_num);
					

					if(surive_num >= 4){
					pPlayer->m_ending_frags += 5;//��������϶�!��Ʒֵ+5%
					}

					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 2762){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 2802){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_ddf4" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!VAN_3", VOL_NORM, 0.1, 0, PITCH_NORM );
					sprintf( text, "Vanlve: The Deep Dark Fantasy.\n");
					UTIL_SayTextAll( text,this );
					}
			}
			if(pev->frags == 2832){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
					if ( pSpot ){
						pSpot->pev->angles.y = 90;
						pSpot->pev->velocity.y = 15;
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->v_angle = pSpot->pev->angles;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
			if(pev->frags == 2840){
				pPlayer->Clear_SayText();
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
				if ( pSpot ){
					CGrenade::ShootTimed_darkhole( pPlayer->pev, pSpot->pev->origin - Vector(0,-10,0), Vector(0,750,0), 4.0 );
					CGrenade::ShootTimed_darkhole( pPlayer->pev, pSpot->pev->origin - Vector(-20,-10,0), Vector(250,750,0), 4.0 );
					CGrenade::ShootTimed_darkhole( pPlayer->pev, pSpot->pev->origin - Vector(20,-10,0), Vector(-250,750,0), 4.0 );
				}
			}
			if(pev->frags == 2940){
					FireTargets( "blackmesa_build_tg", this, this, USE_TOGGLE, 0 );
					FireTargets( "blackmesa_build_tg_duck", this, this, USE_TOGGLE, 0 );
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 3010){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
					if ( pSpot ){
						pPlayer->pev->origin = pSpot->pev->origin + Vector(480,-1550,0);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->m_trainning = 0;
						pPlayer->m_flVelocityModifier = -2;
						pPlayer->pev->health = pPlayer->pev->max_health;
						pPlayer->EnableControl(TRUE);
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->m_game_rate = 75;//��Ϸ����75%
					}
			}
			if(pev->frags == 3030){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fa_stand" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
						if ( pSpot ){
						UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin - Vector(192,0,83) );
						pEntity2->pev->angles.y = 180;
						}
					}
			}
			if(pev->frags == 3040){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 3050){
				UTIL_Remove( this );
				return;
			}
	}
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
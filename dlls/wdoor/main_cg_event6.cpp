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
// ��CG�¼���ͳ��6
//=========================================================
class CMain_Event6 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
	char text[256];
};

LINK_ENTITY_TO_CLASS( main_cg_event_new6, CMain_Event6 );//����6

void CMain_Event6::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event6::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��5=================================================//
void CMain_Event6::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}
	if(pev->armortype == 74){//�¼�74 Doraemon��BOSSս!
		if(pev->frags == 0){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin.x += 128;
					pSpot->pev->angles.y = 180;
					pSpot->pev->velocity.x = 10;
				}
				pPlayer->pev->origin = pev->origin + Vector(0,-128,0);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
		}
		if(pev->frags == 10){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_doraemon_boss");
			if ( pSpot ){
				CBaseEntity *pEntity = Create( "monster_kadoma", pSpot->pev->origin + Vector(-1200,0,0), Vector(0,0,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_nobita");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(128,0,0) );
				pEntity2->pev->angles.y = 0;
				}

				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_mario");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(64,64,0) );
				pEntity2->pev->angles.y = 0;
				}

				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(64,-64,0) );
				pEntity2->pev->angles.y = 0;
				}
			}
		}
		if(pev->frags == 20){
				//int clear_num = 0;
				CBaseEntity *pEntity = NULL;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
				{
						if (  (pEntity->pev->flags & FL_MONSTER) 
						|| pEntity->pev->deadflag != DEAD_NO
						|| pEntity->pev->movetype == MOVETYPE_TOSS){
							if(pEntity->pev->origin.x + 1024 < pev->origin.x
							&& pEntity->pev->origin.y < pev->origin.y
							&& fabs( pEntity->pev->origin.z - pev->origin.z ) <= 384
							&& pEntity->Classify() != CLASS_PLAYER_ALLY ){
							//������޶������ʵ��!
								if( (pEntity->pev->flags & FL_MONSTER)
								&& pEntity->pev->deadflag == DEAD_NO
								&& pEntity->Classify() == CLASS_ALIEN_MILITARY){
									//Bug Fix 3.0 �ص������ͷ����ĺ��
									pEntity->Killed( pev, GIB_ALWAYS );
								}
								else{
									UTIL_Remove( pEntity );
								}
							//clear_num++;
							}
						}
				}
				//sprintf( text, "ClearEnt: %d\n",clear_num);
				//UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 50){
			sprintf( text, "Nobita: ......Doraemon!\n");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 100){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->avelocity.y = 45;
				pSpot->pev->velocity.x = 160;
			}
		}
		if(pev->frags == 120){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->armortype = 8;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 150){
			sprintf( text, "Doraemon: Nobita......\n");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 190){
			
			sprintf( text, "Doraemon: You've grown fast under extreme circumstances.\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 240){
			
			sprintf( text, "Doraemon: My secret stash hasn't made you stronger, but your battles with monsters.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 310){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->angles.y = 180;
				pSpot->pev->origin.x -= 768;
			}
		}
		if(pev->frags == 320){
			
			sprintf( text, "Nobita: You can't be serious! You make me sick!\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 360){
			
			sprintf( text, "Nobita: Why do that? Is it 'for my own good'?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 400){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_nobita");
			CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
			if ( pEntity2 ){
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "reload" );
			pEnemyMonster->pev->framerate = 0;
			pEnemyMonster->pev->frame = 0;
			}
			
			sprintf( text, "Nobita: They're all dead because of you! I'll get you, and I'll bring everything back to normal!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 475){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 480){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->angles.y = 0;
				pSpot->pev->origin.x += 768;
			}
		
			sprintf( text, "Doraemon: I will carry out G's will!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 520){
			
			sprintf( text, "Doraemon: Stop me then!\n");
			
			UTIL_SayTextAll( text,this );
			FireTargets( "boss_combat_clipwall", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 560){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(128,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,0,0);
			pPlayer->pev->angles = Vector(0,0,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			}
			pPlayer->TeamMate_Nagamatagi_Teleport(1);
		}
		if(pev->frags == 580){
			FireTargets( "boss_combat_labdr", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 585){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 590){
			CBaseMonster *pEnemyMonster;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doraemon_boss");
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_godmode = FALSE;
				pEntity->pev->spawnflags = 0;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_freeman");
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_godmode = FALSE;
				pEntity->pev->spawnflags = 0;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_luigi");
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_godmode = FALSE;
				pEntity->pev->spawnflags = 0;
			}
		}
		if(pev->frags == 600){
			pPlayer->BOSS_Find();
			game_boss_battle = 1;
			pPlayer->m_music_save = 17;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 9\n");
			//SERVER_COMMAND("mp3 loop media/boss8.mp3\n");
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 75){//�¼�75 Doraemon��ɱ!
		if(pev->frags == 0){
			game_boss_battle = 0;
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 10){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 60){//��û��?
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_freeman");
			if ( pEntity ){
				pEntity->pev->nextthink = gpGlobals->time + 0.1;
				pEntity->SetThink ( &CMain_Event6::SUB_Remove );//����A�� Bug Fix 2.0
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_luigi");
			if ( pEntity ){
				pEntity->pev->nextthink = gpGlobals->time + 0.1;
				pEntity->SetThink ( &CMain_Event6::SUB_Remove );//����A�� Bug Fix 2.0
			}
		}
		if(pev->frags == 80){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin.x = pev->origin.x - 512;
					pSpot->pev->origin.y = pev->origin.y - 192;
					pSpot->pev->origin.z = pev->origin.z + 72;
					pSpot->pev->frags = 0;
					pSpot->pev->angles.y = 90;
					pSpot->pev->velocity.x = -15;
				}
				pPlayer->pev->origin = pev->origin + Vector(-4096,0,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
		}
		if(pev->frags == 85){//ս����������Ѫ+����
			pPlayer->pev->health = pPlayer->pev->max_health;
			pPlayer->TeamMate_Nagamatagi_RespawnStone(1);
		}
		if(pev->frags == 90){//ȫԱ����
			pPlayer->TeamMate_Nagamatagi_Teleport(2);
		}
		if(pev->frags == 95){//��������
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(-512,0,0), Vector(0,0,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_nobita");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(-64,64,0) );
				pEntity2->pev->angles.y = 0;
				}

				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_lelite");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(-128,0,0) );
				pEntity2->pev->angles.y = 180;
				}

				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(-64,-64,0) );
				pEntity2->pev->angles.y = 0;
				}
		}
		if(pev->frags == 130){
			
			sprintf( text, "Nobita: He blew himself up. What for?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 150){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 180){
			pPlayer->Clear_SayText();
			FireTargets( "boss_combat_clipwall", this, this, USE_TOGGLE, 0 );
			FireTargets( "boss_combat_labdr", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 210){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
			pSpot->pev->origin.x = pev->origin.x - 700;
			pSpot->pev->origin.y = pev->origin.y;
			pSpot->pev->angles.y = 0;
			}
		}
		if(pev->frags == 220){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				sprintf( text, "Lelite: ......\n");
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 250){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				
				sprintf( text, "Lelite: Trouble's here.\n");
			
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!LELITE_4", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 280){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				
				sprintf( text, "Lelite: I'll handle them.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!LELITE_5", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 320){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				
				sprintf( text, "Lelite: Retreat! I'll cover you!\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!LELITE_6", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 370){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_dengor");
			if ( pSpot ){
				
				sprintf( text, "Dengor: We're the load, huh?\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_WAIT0", VOL_NORM, 0.5, 0, 95 );
				pSpot->pev->angles.y = 180;
			}
		}
		if(pev->frags == 420){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				
				sprintf( text, "Lelite: Sorry!\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!LELITE_7", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 450){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->angles.y = 180;
			}
			FireTargets( "unlocknobita_dr", this, this, USE_TOGGLE, 0 );
			FireTargets( "doraemon_boss_door", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 470){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
			if ( pEntity2 ){
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			}
		}
		if(pev->frags == 530){//lelite���ͽ���ķ��
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_hPlayer = pPlayer;//�Է���һ
				pEnemyMonster->Hunt_Stand_Set(2);//�����ж����������!
				pEnemyMonster->pev->health = pEnemyMonster->pev->max_health * 0.5;//��Ѫ!
				pEnemyMonster->m_iTriggerCondition = 4;
				pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("lambda_sci_dievent");//��Ϊ�ؼ�NPC!
				pEntity->pev->angles.y = 90;
				CBaseEntity *pTelpinter = UTIL_FindEntityByTargetname( NULL, "lelite_in_here" );
				if(pTelpinter){
				UTIL_SetOrigin( pEntity->pev, pTelpinter->pev->origin);
				pEntity->pev->velocity = g_vecZero;
				pPlayer->TeamMate_remove(pEnemyMonster);
				}
				pEnemyMonster->m_rpgms_inteam = 5;
			}
		}
		if(pev->frags == 540){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->angles.y = 0;
			}
		}
		if(pev->frags == 560){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_dengor");
			if ( pSpot ){
				
				sprintf( text, "Dengor: Huh.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_PIDLE5", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 600){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = 45;
				pSpot->pev->velocity.z = 20;
				pSpot->pev->avelocity.x = 2;
				pSpot->pev->frags = 0;
				FireTargets( "misaliyadoor", this, this, USE_TOGGLE, 0 );
			}
		}
		if(pev->frags == 615){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
			CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
			if ( pEntity2 ){
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
		}
		if(pev->frags == 650){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
			CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
			if ( pEntity2 ){
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEnemyMonster->m_rpgms_level = 75;//Bug Fix 3.0 Misaliya�ȼ�����������
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			}
		
			sprintf( text, "Misaliya: Kadoma, is everyone okay?");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 660){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 690){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->angles.y = 90;
				pSpot->pev->origin.x += 192;
				pSpot->pev->origin.y -= 256;
				pSpot->pev->velocity.x = 30;
				pSpot->pev->armortype = 0;
			}
			
			sprintf( text, "Misaliya: We're done fighting, and found some new friends!");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 740){
			CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
			if ( pEntity3 ){
			pEntity3->pev->angles.y = 0;
			FireTargets( "misaliyadoor", this, this, USE_TOGGLE, 0 );
			}
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dragon");
			if ( pEntity2 ){
			UTIL_SetOrigin( pEntity2->pev, pEntity3->pev->origin + Vector(1024,0,0) );
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hime");
			if ( pEntity2 ){
			UTIL_SetOrigin( pEntity2->pev, pEntity3->pev->origin + Vector(1094,0,0) );
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_willam");
			if ( pEntity2 ){
			UTIL_SetOrigin( pEntity2->pev, pEntity3->pev->origin + Vector(1164,0,0) );
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
			if ( pEntity2 ){
			UTIL_SetOrigin( pEntity2->pev, pEntity3->pev->origin + Vector(1234,0,0) );
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_blues");
			if ( pEntity2 ){
			UTIL_SetOrigin( pEntity2->pev, pEntity3->pev->origin + Vector(1304,0,0) );
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast");
			if ( pEntity2 ){
			UTIL_SetOrigin( pEntity2->pev, pEntity3->pev->origin + Vector(1374,0,0) );
			}
			
			sprintf( text, "Misaliya: Everyone has gotten a lot stronger!");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 750){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hime");
			CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
			if ( pEntity2 ){
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dragon");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_willam");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_WALK );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_blues");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_RUN );
			pEntity2->pev->movetype = MOVETYPE_NOCLIP;
			pEntity2->pev->velocity.x = -300;
			pEntity2->pev->angles.y = 180;
			}
		}
		if(pev->frags == 782){
			pPlayer->Clear_SayText();

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hime");
			CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
			if ( pEntity2 ){
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			pEnemyMonster->m_rpgms_level = 60;
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dragon");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			pEnemyMonster->m_rpgms_level = 72;
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			pEnemyMonster->m_rpgms_level = 80;
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_willam");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			pEnemyMonster->m_rpgms_level = 80;
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_blues");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
			if ( pEntity2 ){
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEntity2->pev->movetype = MOVETYPE_STEP;
			pEntity2->pev->velocity.x = 0;
			pPlayer->TeamMate_add(pEnemyMonster);//���!
			}
		}
		if(pev->frags == 785){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 8;
			}
		}
		if(pev->frags == 820){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 825){
			CBaseEntity *pTelpinter = UTIL_FindEntityByTargetname( NULL, "center_bnboss_telpo" );
			if(pTelpinter){
				Vector org = pTelpinter->pev->origin + Vector(-256,36,0);

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dragon");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_willam");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_giant");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_mario");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
			}
		}
		if(pev->frags == 830){
			CBaseEntity *pTelpinter = UTIL_FindEntityByTargetname( NULL, "center_bnboss_telpo" );
			if(pTelpinter){
				Vector org = pTelpinter->pev->origin + Vector(-256,-36,0);

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_blues");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hime");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_saintna");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_nobita");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity2->pev, org);
				org.x += 96;
				}
				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 90;
				org.x -= 288;
				org.y -= 512;
				UTIL_SetOrigin( pEntity2->pev, org);
				}
			}
		}
		if(pev->frags == 835){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->origin.z -= 16;
				pSpot->pev->origin.y -= 128;
				pSpot->pev->origin.x -= 512;
				pSpot->pev->avelocity.x = 1;
				pSpot->pev->velocity.x = 0;
				pSpot->pev->velocity.y = 5;
				pSpot->pev->velocity.z = 5;
				pSpot->pev->armortype = 0;
			}
		}
		if(pev->frags == 900){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_dengor");
			if ( pSpot ){
				
				sprintf( text, "Dengor: We'll be a great team!\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_ZP14", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 960){//ȫԱ����
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			pPlayer->TeamMate_Nagamatagi_Teleport(2);
			pPlayer->m_game_rate = 88;//��Ϸ����88%

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,90,0);
			pPlayer->pev->angles = Vector(0,90,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			}
		}
		if(pev->frags == 970){
			SERVER_COMMAND( "autosave\n" );
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 76){//�¼�76 ������������ս��!
		if(pev->frags == 0){
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 5){
			pPlayer->TeamMate_Nagamatagi_Teleport(3);
			pPlayer->m_game_rate = 89;//��Ϸ����89%
		}
		if(pev->frags == 10){//����һ��һ��һ��һ��������������ʵ�壡
				//int clear_num = 0;
				CBaseEntity *pEntity2 = NULL;
				while ((pEntity2 = UTIL_FindEntityInSphere( pEntity2, pev->origin, 16384 )) != NULL)
				{
					if ( (pEntity2->pev->flags & FL_MONSTER) ){
							if(pEntity2->pev->origin.z > (pev->origin.z + 810)){
								if(FClassnameIs( pEntity2->pev, "monster_scientist" )
								|| FClassnameIs( pEntity2->pev, "monster_scientist_mad" )
								|| FClassnameIs( pEntity2->pev, "monster_sitting_scientis" )
								|| FClassnameIs( pEntity2->pev, "monster_barney" )
								|| FClassnameIs( pEntity2->pev, "monster_barney_shield" )
								|| FClassnameIs( pEntity2->pev, "monster_barney_hevshield" )
								|| FClassnameIs( pEntity2->pev, "monster_barney_tr1" )){
								UTIL_Remove( pEntity2 );//�������ʵ��!
								//clear_num++;
								}
								else if(!FBitSet( pEntity2->pev->flags, FL_NOTARGET )){
								UTIL_Remove( pEntity2 );//�������ʵ��!
								//clear_num++;
								}
							}
					}
				}
				//sprintf( text, "ClearEnt: %d\n",clear_num);
				//UTIL_SayTextAll( text,this );	
		}
		if(pev->frags == 20){
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 77){//�¼�77 ս��������������ķ������!
		if(pev->frags == 10){
			pPlayer->m_game_rate = 90;//��Ϸ����90%
		}
		if(pev->frags == 30){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 50){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 90){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 100){//���ͽ���ķ��
			CBaseEntity *pTelpinter = UTIL_FindEntityByTargetname( NULL, "player_tel_in_lambda" );
			if(pTelpinter){
				pPlayer->pev->origin = pTelpinter->pev->origin + Vector(0,0,36);
				pPlayer->TeamMate_Nagamatagi_Teleport(5);
				pPlayer->m_flVelocityModifier = -3;
			}
		}
		if(pev->frags == 120){
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 78){//�¼�78 Lelite���ը��
		if(pev->frags == 0){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				
				sprintf( text, "Lelite: I'll defuse these...\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!LELITE_8", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 30){//��˽�ο�ѧ�ҡ��̶�
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_sci1");
			CBaseMonster *pEnemyMonster;
			if ( pSpot ){
			pEnemyMonster = pSpot->MyMonsterPointer();
			pEnemyMonster->m_selfmode = TRUE;
			pEnemyMonster->m_rpgms_inteam = 5;
			}

			pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_sci2");
			if ( pSpot ){
			pEnemyMonster = pSpot->MyMonsterPointer();
			pEnemyMonster->m_selfmode = TRUE;
			pEnemyMonster->m_rpgms_inteam = 5;
			}
		}
		if(pev->frags == 50){
		pPlayer->Clear_SayText();
		}
		if(pev->frags == 60){//��˽�ο�ѧ�һ��
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_sci1");
			if ( pSpot ){
			EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_ZP3", VOL_NORM, 0.5, 0, PITCH_NORM );
			char text[256];
			
			sprintf( text, "Scientist: I'm saved! Thank God you're here.\n");
			
			UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 120){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 140){
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_game_rate = 91;//��Ϸ����91%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 10.0, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 161){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 210){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "barnacledemon_corpse");
			CBaseMonster *pEnemyMonster;
			if ( pSpot ){
			pEnemyMonster = pSpot->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "dead_getup" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 235){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 10.0, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 256){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 285){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 305){
			FireTargets( "crasher_glass_unbrk", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 325){
			FireTargets( "crasher_generic_maker", this, this, USE_TOGGLE, 0 );

			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "crasher_generic");
			if ( pSpot ){
			UTIL_Remove( pSpot );
			}

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
			if ( pEntity ){
			UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-80,160,0));
			pEntity->pev->angles.y = 180;
			pEntity->Killed( pev, GIB_NEVER );
			}

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pev->origin + Vector(32,160,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,180,0);
			pPlayer->pev->angles = Vector(0,180,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 79){//�¼�79 ���߾�Ԯ
		if(pev->frags == 20){
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_game_rate = 92;//��Ϸ����92%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 10.0, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 45){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = -30;
				pSpot->pev->frags = 10;
				pSpot->pev->origin + Vector(128,0,0);
				pPlayer->pev->origin = pev->origin + Vector(256,96,36);
			}
		}
		if(pev->frags == 50){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
			if ( pEntity ){
			UTIL_Remove( pEntity );
			}
			//�������
			pPlayer->TeamMate_Nagamatagi_Allclear(0);
		}
		if(pev->frags == 55){
			//ȫԱ���!
			pPlayer->TeamMate_Nagamatagi_Allclear(10);
		}
		if(pev->frags == 57){
			//ȫԱ�ָ�
			pPlayer->TeamMate_Nagamatagi_RespawnStone(1);
		}
		if(pev->frags == 60){
			pPlayer->TeamMate_Nagamatagi_Teleport(6); //Bug Fix 3.0//����ս����ͬ��+�油����
		}
		if(pev->frags == 64){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_last_barney");
			if ( pSpot ){
			UTIL_SetOrigin( pSpot->pev, pev->origin + Vector(-160,72,0) );
			pSpot->pev->angles.y = 0;
			}
		}
		if(pev->frags == 100){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				pSpot->pev->armortype = 10;
				pSpot->pev->frags = 10;
			}
			FireTargets( "barnacledemonboss_maker", this, this, USE_TOGGLE, 0 );
			FireTargets( "boss_combat_clipwall", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 125){
				pPlayer->pev->health = pPlayer->pev->max_health;//��һ�Ѫ
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "monster_barnacle_boss");
					if ( pSpot2 ){
						pSpot->pev->origin = pSpot2->pev->origin + Vector(-256,64,64);
						pSpot->pev->velocity = g_vecZero;
						pSpot->pev->avelocity = g_vecZero;
						pSpot->pev->angles.x = 0;
						pSpot->pev->angles.y = 0;
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
					}
				}
		}
		if(pev->frags == 150){
			
				sprintf( text, "- Select teammates to kill it!\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 180){
				pPlayer->m_fSelectMode = TRUE;
				pPlayer->m_fSelectNumber = 0;
				pPlayer->ShowVGUIMenu(37);
				pPlayer->m_load_check = 1;
		}
		if(pev->frags == 190){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 200){
				if(pPlayer->m_fSelectNumber == 0){
					if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
						pPlayer->ShowVGUIMenu(37);
						pPlayer->m_load_check = 1;
					}
					pev->frags = 195;
					pev->team = -1;
				}
		}
		if(pev->frags == 205){
			pev->team = pPlayer->m_fSelectNumber;
			pPlayer->m_fSelectNumber = 0;
			pPlayer->Clear_SayText();
		}
		if(pev->team == 1){//ս��
			if(pev->frags == 210){//ͬ��˵�����
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin.y -= 32;
					pSpot->pev->velocity.x = 30;
					pPlayer->pev->origin = pev->origin + Vector(256,96,36);
				}

				pPlayer->TeamMate_GetNagamatagi();
				pPlayer->m_rpg_menu_on = 7;
				pPlayer->m_rpg_menu_select = 1;
				pPlayer->m_rpg_menu_skill_chater = 0;
				
				sprintf( text, "- They'll go fight it!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 230){
				if(pPlayer->m_rpg_menu_on > 0){
				pev->frags = 220;
				}
			}
			if(pev->frags == 235){
				FireTargets( "barlambdateldoor", this, this, USE_TOGGLE, 0 );
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 10.0, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 250){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_barnacle_boss");
				if ( pSpot ){
					pPlayer->m_boss_find = pSpot;
					pPlayer->m_boss_on = 1;
					game_boss_battle = 1;
					CBaseMonster *pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 50;
					pEnemyMonster->pev->flags |= FL_FROZEN;
					pEnemyMonster->m_notarget_hide = 50;
				}
			}
			if(pev->frags == 255){
				CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "center_bnboss_telpo");
				if ( pSpot ){
				pPlayer->EnableControl(TRUE);
				pPlayer->m_trainning = 0;
				pPlayer->pev->origin = pSpot->pev->origin + Vector(-256,0,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->pev->velocity = g_vecZero;
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );
				pPlayer->pev->v_angle = Vector(0,0,0);
				pPlayer->pev->angles = Vector(0,0,0);
				pPlayer->pev->fixangle = TRUE;

				pPlayer->m_mode_origin = pSpot->pev->origin;

				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->TeamMate_Nagamatagi_Teleport(7);

				pPlayer->m_team_npc1->pev->angles.y = 0;
				pPlayer->m_team_npc2->pev->angles.y = 0;
				pPlayer->m_team_npc3->pev->angles.y = 0;
				pPlayer->m_team_npc4->pev->angles.y = 0;

				pPlayer->m_godposion = 0;
				pPlayer->m_rpg_menu_actor1 = 0;
				pPlayer->m_iClientHealth  = -1;
				pPlayer->m_iClient_mynpc  = -1;
				pPlayer->m_iClientBattery = -1;
				pPlayer->pev->movetype = MOVETYPE_NOCLIP;//kadoma�ι۲���ģʽ
				pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS;
				pPlayer->m_hasflashlight = FALSE;
				pPlayer->pev->flags |= FL_NOTARGET;
				pPlayer->pev->solid = SOLID_NOT;

				pPlayer->Clear_SayText();
				}
			}
			if(pev->frags == 280){
				pPlayer->TeamMate_Nagamatagi_RespawnStone(2);//�ж�
				pPlayer->m_music_save = 14;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 5\n");
				//SERVER_COMMAND("mp3 loop media/boss6.mp3\n");
				
				sprintf( text, "- Observer Mode\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 360){
				if(pPlayer->m_team_npc1->pev->deadflag == DEAD_DEAD
				&& pPlayer->m_team_npc2->pev->deadflag == DEAD_DEAD
				&& pPlayer->m_team_npc3->pev->deadflag == DEAD_DEAD
				&& pPlayer->m_team_npc4->pev->deadflag == DEAD_DEAD){//С��ȫ�𡢾�Ԯʧ��
					pPlayer->m_music_save = 0;
					SERVER_COMMAND("mp3 stop\n");
				}
				else if (FNullEnt(pPlayer->m_boss_find)){//BOSS��ɱ��
					pPlayer->m_music_save = 0;
					SERVER_COMMAND("mp3 stop\n");
					pev->impulse = 1;
				}
				else if(pev->team == 1){
					pev->frags = 340;//ѭ��
				}
			}
			if(pev->frags == 390){
				if(pev->impulse == 1){//�ɹ�!
					pPlayer->m_ending_frags += 15;//��Ԯ�ɹ�!��Ʒֵ+15%
					pPlayer->EnableControl(FALSE);
					pPlayer->m_trainning = 1;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 10.0, 255, FFADE_OUT );//��Ϲ���
					pPlayer->Clear_SayText();
				}
				else{//ʧ��!
					FireTargets( "lambda_sci_dievent", this, this, USE_TOGGLE, 0 );
					return;//GAME OVER
				}
			}
		}
		else if(pev->team == 2){//�ر�
			if(pev->frags == 220){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->Clear_SayText();
				}
			}
			if(pev->frags == 240){
				CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_last_barney");
				if ( pSpot ){
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!BA_SHOT4", VOL_NORM, 0.5, 0, PITCH_NORM );

				
				sprintf( text, "Guard: Oh, with friends like you...!\n");
				
				UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 300){
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 10.0, 255, FFADE_OUT );//��Ϲ���
				pPlayer->Clear_SayText();
				pPlayer->m_ending_frags -= 15;//������Ԯ!��Ʒֵ-15%
				pev->frags = 400;
			}
		}
		if(pev->frags == 420){//�ҷ�ȫ����ȫ�ָ�
			pPlayer->TeamMate_Nagamatagi_RespawnStone(1);
		}
		if(pev->frags == 425){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

					pPlayer->m_godposion = 0;
					pPlayer->m_rpg_menu_actor1 = 1;
					pPlayer->m_iClientHealth  = -1;
					pPlayer->m_iClient_mynpc  = -1;
					pPlayer->m_iClientBattery = -1;
					pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
					pPlayer->m_hasflashlight = TRUE;
					pPlayer->pev->flags &= ~FL_NOTARGET;
					pPlayer->pev->solid			= SOLID_SLIDEBOX;
					pPlayer->pev->movetype		= MOVETYPE_WALK;

					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;
					pPlayer->pev->origin = pSpot->pev->origin + Vector(128,64,0);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->pev->velocity = g_vecZero;
					pPlayer->pev->v_angle = Vector(0,90,0);
					pPlayer->pev->angles = Vector(0,90,0);
					pPlayer->pev->fixangle = TRUE;
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );

					CBaseEntity *pScipot = UTIL_FindEntityByTargetname( NULL, "lambda_sci1");
					CBaseMonster *pEnemyMonster;
					if ( pScipot ){
					pEnemyMonster = pScipot->MyMonsterPointer();
					pEnemyMonster->SetActivity( ACT_IDLE );
					UTIL_SetOrigin( pScipot->pev, pSpot->pev->origin + Vector(192,64,-64) );
					pScipot->pev->angles.y = 90;
					pScipot->pev->gravity = 1.6;//����
					pScipot->pev->takedamage = DAMAGE_NO;
					}

					pScipot = UTIL_FindEntityByTargetname( NULL, "lambda_sci2");
					if ( pScipot ){
					pEnemyMonster = pScipot->MyMonsterPointer();
					pEnemyMonster->SetActivity( ACT_IDLE );
					UTIL_SetOrigin( pScipot->pev, pSpot->pev->origin + Vector(256,-64,-64) );
					pScipot->pev->angles.y = 90;
					pScipot->pev->gravity = 1.6;//����
					pScipot->pev->takedamage = DAMAGE_NO;
					}
			}
		}
		if(pev->frags == 470 && pev->impulse == 1){
				
				sprintf( text, "- Kadoma's team manages to rescue some survivors.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 500){
				FireTargets( "lambda_sciuse_retian", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 520){
				SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 550 && pev->impulse == 1){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				CBaseEntity *pEntity = Create( "monster_scientist", pSpot->pev->origin + Vector(480,10,-64), Vector(0,180,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_selfmode = TRUE;
				pEnemyMonster->m_fightmode = TRUE;
				pEnemyMonster->m_makerspawn_call = 1;
				pEnemyMonster->m_rpgms_inteam = 5;
				pEntity->pev->body = -1;
				pEntity->pev->takedamage = DAMAGE_NO;//�޵�

				pEntity = Create( "monster_scientist", pSpot->pev->origin + Vector(460,-40,-64), Vector(0,180,0), NULL );
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_selfmode = TRUE;
				pEnemyMonster->m_fightmode = TRUE;
				pEnemyMonster->m_makerspawn_call = 1;
				pEnemyMonster->m_rpgms_inteam = 5;
				pEntity->pev->body = -1;
				pEntity->pev->takedamage = DAMAGE_NO;//�޵�

				pEntity = Create( "monster_scientist", pSpot->pev->origin + Vector(480,-90,-64), Vector(0,180,0), NULL );
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_selfmode = TRUE;
				pEnemyMonster->m_fightmode = TRUE;
				pEnemyMonster->m_makerspawn_call = 1;
				pEnemyMonster->m_rpgms_inteam = 5;
				pEntity->pev->body = -1;
				pEntity->pev->takedamage = DAMAGE_NO;//�޵�
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 580){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_sci1");
			if ( pSpot ){
			EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_ZP31", VOL_NORM, 0.5, 0, PITCH_NORM );
			char text[256];
			
			sprintf( text, "Scientist: What's wrong with our equipment?\n");
			
			UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 630){
			pPlayer->Clear_SayText();
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 80){//�¼�80 ��ɫ�滭��������
		if(pev->frags == 0){
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_game_rate = 94;//��Ϸ����94%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
		}
		//Bug Fix 3.0 ǿ������������������⣬�ǾͲ������ˣ�����������
		/*
		if(pev->frags == 5){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_ally_turret");
			if ( pSpot ){
			EMIT_SOUND_DYN(ENT(pSpot->pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);
			UTIL_Remove( pSpot );
			}
		}
		if(pev->frags == 8){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_ally_turret");
			if ( pSpot ){
			EMIT_SOUND_DYN(ENT(pSpot->pev), CHAN_STATIC, "turret/tu_active2.wav", 0, 0, SND_STOP, 100);
			UTIL_Remove( pSpot );
			}
		}*/
		if(pev->frags == 10){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "nobita_sit_bluedraw_p");
			if ( pSpot ){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_nobita");
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					if ( pEntity2 ){
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin);
					pEntity2->pev->angles.y = 270;
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sit_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			pSpot = UTIL_FindEntityByTargetname( NULL, "saintna_sit_bluedraw_p");
			if ( pSpot ){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					if ( pEntity2 ){
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin);
					pEntity2->pev->angles.y = 90;
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sit_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
		}
		if(pev->frags == 12){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "ally_bluedraw_p_org");
			if ( pSpot ){
					CBaseEntity *pEntity2 = Create( "monster_kadoma", pSpot->pev->origin + Vector(-60,60,0), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 0, 3 );
					pEnemyMonster->pev->flags |= FL_NOTARGET;

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(-60,-60,0) );
					pEntity2->pev->angles.y = 90;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hime");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(192,128,0) );
					pEntity2->pev->angles.y = 90;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dragon");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(256,128,0) );
					pEntity2->pev->angles.y = 90;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(320,128,0) );
					pEntity2->pev->angles.y = 90;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_willam");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(192,-128,0) );
					pEntity2->pev->angles.y = 270;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_giant");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(256,-128,0) );
					pEntity2->pev->angles.y = 270;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(320,-128,0) );
					pEntity2->pev->angles.y = 270;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_mario");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					pEntity2->pev->weapons = 3;//�ָ�3����
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(620,80,0) );
					pEntity2->pev->angles.y = 0;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(620,16,0) );
					pEntity2->pev->angles.y = 0;
					pEntity2->pev->frags = 0;
					}

					pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_blues");
					if ( pEntity2 ){
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(0);//��ֹ
					UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin + Vector(620,-48,0) );
					pEntity2->pev->angles.y = 0;
					}
			}
		}
		if(pev->frags == 15){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 30;
				pPlayer->pev->origin = pSpot->pev->origin + Vector(0,-2048,0);
			}
		}
		if(pev->frags == 65){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pSpot->pev->armortype = 10;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 95){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = -30;
				pSpot->pev->armortype = 10;
				pSpot->pev->frags = 1;
				pSpot->pev->angles.y = 180;
			}
		}
		if(pev->frags == 115){
			
			sprintf( text, "Misaliya: The final battle has come at last.");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 160){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pSpot->pev->velocity.x = 0;

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					if ( pEntity2 ){
					UTIL_SetOrigin( pSpot->pev, pEntity2->pev->origin + Vector(0,60,60) );
					pSpot->pev->angles.y = 270;
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "deep_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEntity2->pev->skin = 2;
					}

					
					sprintf( text, "Misaliya: Kadoma, make sure to make it to the end!");
					
					UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 210){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
					pSpot->pev->angles.y = 90;
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					if ( pEntity2 ){
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "stoptalk" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
					pPlayer->Clear_SayText();
			}
		}
		if(pev->frags == 250){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pSpot->pev->angles.y = 270;

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					if ( pEntity2 ){
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "hurt_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEntity2->pev->skin = 6;
					}

					sprintf( text, "Misaliya: I haven't said it yet...");
					
					UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 290){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEntity2->pev->skin = 1;
				}
				
				sprintf( text, "Misaliya: Though to be fair, it would probably set a Death Flag.");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 340){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEntity2->pev->skin = 7;
				}
				
				sprintf( text, "Misaliya: Let's wait until this is done to say it.");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 400){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pPlayer->Clear_SayText();
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			}
		}
		if(pev->frags == 405){
			CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot2 ){
				CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_sci1");
				if ( pSpot ){
				UTIL_SetOrigin( pSpot->pev, pSpot2->pev->origin + Vector(96,-240,-50));
				pSpot2->pev->origin = pSpot2->pev->origin + Vector(96,-128,0);
				pSpot->pev->angles.y = 90;
				pSpot->pev->velocity.z = -10;
				}
			}
		}
		if(pev->frags == 430){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
			if ( pEntity2 ){
			pEntity2->pev->skin = 0;
			}

			FireTargets( "bluedrawroom_door", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 450){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "lambda_sci1");
			if ( pSpot ){
			pSpot->pev->takedamage = DAMAGE_NO;
			EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_ZP29", VOL_NORM, 0.5, 0, PITCH_NORM );
			char text[256];
			
			sprintf( text, "Scientist: The transporter is ready.\n");
			
			UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 500){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			
			pPlayer->Clear_SayText();
			pPlayer->TeamMate_Nagamatagi_Teleport(2);
			pPlayer->m_game_rate = 95;//��Ϸ����95%

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,270,0);
			pPlayer->pev->angles = Vector(0,270,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			}
		}
		if(pev->frags == 510){//���β���
			FireTargets( "lambda_last_teltest_rn1", this, this, USE_TOGGLE, 0 );

			pPlayer->pev->health = pPlayer->pev->max_health;
				pPlayer->GiveAmmo( 64, "buckshot", BUCKSHOT_MAX_CARRY );
				pPlayer->GiveAmmo( 4, "rockets", ROCKET_MAX_CARRY );
				pPlayer->GiveAmmo( 120, "762nato", AK_MAX_CARRY );
				pPlayer->GiveAmmo( 120, "556nato", M16_MAX_CARRY );
				pPlayer->GiveAmmo( 30, "bolts", BOLT_MAX_CARRY );
				pPlayer->GiveAmmo( 4, "ARgrenades", M203_GRENADE_MAX_CARRY );
				pPlayer->GiveAmmo( 18, "357", _357_MAX_CARRY );
				pPlayer->GiveAmmo( 15, "338mag", SNIPER_MAX_CARRY );
				pPlayer->GiveAmmo( 180, "45acp", MAC_MAX_CARRY );
				pPlayer->GiveAmmo( 180, "9mm", _9MM_MAX_CARRY );
				pPlayer->GiveAmmo( 150, "762natobox", MINIGUN_MAX_CARRY );
				pPlayer->GiveAmmo( 40, "uranium", URANIUM_MAX_CARRY );
				pPlayer->GiveAmmo( 2, "nuke", 5 );
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 81){//�¼�81 ���վ�ս�ε������Ԫ�ռ䡤����
		if(pev->frags == 10){
			pPlayer->pev->origin = pev->origin + Vector(0,0,64);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->m_teleprort_in_xen = 1;
			pPlayer->m_flVelocityModifier = -4;
			pPlayer->TeamMate_Nagamatagi_RespawnStone(2);//�ж�
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 82){//�¼�82 Gman���ն�ս
		if(pev->frags == 0){
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_game_rate = 96;//��Ϸ����96%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

			CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin, Vector(0,180,0), NULL );
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle1_angry" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 0, 3 );
			pEnemyMonster->SetBodygroup( 2, 7 );

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 10){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(32,-48,0) );
				pEntity2->pev->angles.y = 180;
				}

				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(32,48,0) );
				pEntity2->pev->angles.y = 180;
				pEntity2->pev->skin = 2;
				}

				pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle_finaly1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity2->pev->angles.y = 0;
				}
		}
		if(pev->frags == 40){
			pPlayer->m_music_save = 20;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 21\n");
			//SERVER_COMMAND("mp3 loop media/music25.mp3\n");
		}
		if(pev->frags == 70){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->origin.x -= 512;
				pSpot->pev->origin.z -= 16;
			}
		}
		if(pev->frags == 80){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: The one in the HEV suit is...\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP3", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 120){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->angles.y = 0;
				pSpot->pev->origin.x += 256;
			}

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				sprintf( text, "Gman: Gordon? Dengor?\n");
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP4", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 170){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 180){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->angles.y = 180;
				pSpot->pev->origin.x -= 256;
			}
			
			sprintf( text, "Gman: Everything is going to plan. The world will return to normal.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 240){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: It's why I'm here.\n");
			
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP5", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 300){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: The existence of mages is inherently wrong. They hold no value.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 370){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: The world must be rebooted, restarted.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP7", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 420){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				FireTargets( "gordon_draw_pict", this, this, USE_TOGGLE, 0 );
				pSpot->pev->origin.x += 64;
				pSpot->pev->origin.z -= 32;
			}
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: Dengor can turn back into Gordon.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 470){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: I trust you understand.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP6", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 540){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 550){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->angles.y = 0;
				pSpot->pev->origin.x += 320;
				pSpot->pev->origin.y -= 48;
				pSpot->pev->origin.z += 16;
			}
		}
		if(pev->frags == 560){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
			if ( pEntity2 ){
				
				sprintf( text, "Dengor: You're dead wrong.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!SC_ANSWER21", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 600){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 610){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->angles.y = 0;
				pSpot->pev->origin.y += 96;
			}
		}
		if(pev->frags == 620){
			
			sprintf( text, "Misaliya: We might not know what the world was like...\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 650){
			
			sprintf( text, "Misaliya: But your actions are wrong!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 680){
			
			sprintf( text, "Misaliya: Our existence holds value!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 720){
			
			sprintf( text, "Misaliya: You say you want to save the world, but you've hurt many innocent people.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 760){
			
			sprintf( text, "Misaliya: The world isn't delicate enough to need you to save it!!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 830){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 840){
			FireTargets( "gordon_draw_pict", this, this, USE_TOGGLE, 0 );
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->origin.y -= 48;
			}
		}
		if(pev->frags == 870){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: It seems we won't be working together.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP1", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 930){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: The world was twisted and broken by the creator - Z.Z. He has gone mad, becoming an evil god.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP7", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 1000){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: If you feel that you're in the right, and wish to stop me, then fight me!\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1090){
			pPlayer->Clear_SayText();

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
			if ( pEntity2 ){
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "use_needle" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetBodygroup( 2, 1 );

			pPlayer->pev->v_angle = Vector(0,270,0);
			pPlayer->pev->angles = Vector(0,270,0);
			pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 1140){
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.5, 1.5, 255, FFADE_OUT );
		}
		if(pev->frags == 1160){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->angles.y = 180;
				pSpot->pev->velocity.x = -90;
				pSpot->pev->armortype = 8;
				pSpot->pev->frags = 1;
			}

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
			if ( pEntity2 ){
			CBaseEntity *pEntity = Create( "monster_gman_boss", pEntity2->pev->origin, pEntity2->pev->angles, NULL );
			pEntity->pev->spawnflags |= SF_MONSTER_PRISONER;
			pEntity->pev->takedamage = DAMAGE_NO;
			UTIL_Remove( pEntity2 );
			}
		}
		if(pev->frags == 1170){
			//ȫԱ����׼��
			pPlayer->TeamMate_Nagamatagi_Teleport(2);
		}
		if(pev->frags == 1180){
			
			sprintf( text, "Gman: Only Power is Truth.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1230){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(128,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,180,0);
			pPlayer->pev->angles = Vector(0,180,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			pPlayer->m_flVelocityModifier = -2;
			}
			pPlayer->TeamMate_Nagamatagi_Teleport(1);
		}
		if(pev->frags == 1270){
			pPlayer->BOSS_Find();
			game_boss_battle = 1;
			pPlayer->m_music_save = 16;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 18\n");
			//SERVER_COMMAND("mp3 loop media/boss7.mp3\n");

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
			pEntity2->pev->spawnflags = 0;
			pEntity2->pev->takedamage = DAMAGE_AIM;
			}
		}
		if(pev->frags == 1280){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 1320){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
				if(pEntity2->pev->deadflag == DEAD_NO || pPlayer->pev->deadflag != DEAD_NO){
					pev->frags = 1300;
				}
			}
		}
		if(pev->frags == 1340){//BOSSս����
			game_boss_battle = 0;
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 1350){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 1360){//Bug Fix 2.0 Misaliya���鸴λ
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
			if ( pEntity2 ){
				if ( pEntity2->pev->deadflag == DEAD_NO ){
				pEntity2->pev->skin = 0;
				}
			}
		}
		if(pev->frags == 1380){
			pPlayer->pev->origin = pev->origin + Vector(1024,0,64);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_game_rate = 97;//��Ϸ����97%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->origin.y -= 128;
				pSpot->pev->origin.z -= 16;
				pSpot->pev->angles.x = 10;
			}
		}
		if(pev->frags == 1385){//ȫԱ����
			pPlayer->TeamMate_Nagamatagi_Teleport(2);
		}
		if(pev->frags == 1390){
			CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(-220,32,0), Vector(0,180,0), NULL );
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle5" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 0, 3 );
			pEnemyMonster->SetBodygroup( 2, 7 );

			pEntity = Create( "monster_gman", pev->origin + Vector(-270,32,0), Vector(0,0,0), NULL );
			pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle_finaly2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 1, 1 );
			pEnemyMonster->SetBodygroup( 2, 2 );
		}
		if(pev->frags == 1395){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
			UTIL_Remove( pEntity2 );
			}
		}
		if(pev->frags == 1420){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: Well done...\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1460){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: You may be up to the task.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1510){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: Doma...the other Kadoma awaits you.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1570){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: You may kill me now.\n");
			
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1600){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: Time to choose.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_CHOOSE1", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 1640){
			pPlayer->m_fSelectMode = TRUE;
			pPlayer->m_fSelectNumber = 0;
			pPlayer->ShowVGUIMenu(37);
			pPlayer->m_load_check = 1;
		}
		if(pev->frags == 1650){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 1670){
			if(pPlayer->m_fSelectNumber == 0){
				if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
					pPlayer->ShowVGUIMenu(37);
					pPlayer->m_load_check = 1;
				}
				pev->frags = 1655;
				pev->team = -1;
			}
		}
		if(pev->frags == 1680){
			pev->team = pPlayer->m_fSelectNumber;
			pPlayer->m_fSelectNumber = 0;
			pPlayer->Clear_SayText();
		}

		if(pev->team == 1){//ɱ��Gman
			if(pev->frags == 1700){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle5_attack" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;

				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
				}
				pPlayer->m_ending_frags -= 30;//��ɱGman!��Ʒֵ-30%
			}
			if(pev->frags == 1740){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 1775){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){
				UTIL_Remove( pEntity2 );
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "gman_center_downfloor" );
				if(pEntity2){
				UTIL_Remove( pEntity2 );
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "center_timer_lgt" );
				if(pEntity2){
				UTIL_Remove( pEntity2 );
				}
				FX_Explosion( pev->origin, 254 );//��������Ѫ������!
			}
			if(pev->frags == 1780){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
				pPlayer->EnableControl(TRUE);
				pPlayer->m_trainning = 0;
				pPlayer->pev->origin = pEntity2->pev->origin + Vector(-512,0,128);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->TeamMate_Nagamatagi_Teleport(1);//Bug Fix 3.0 Gmanս����Ѵ����һָ��ж�
				pPlayer->pev->velocity = g_vecZero;
				pPlayer->pev->v_angle = Vector(0,0,0);
				pPlayer->pev->angles = Vector(0,0,0);
				pPlayer->pev->fixangle = TRUE;
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );
				UTIL_Remove( pEntity2 );
				pPlayer->m_flVelocityModifier = -2;
				}
				UTIL_Remove( this );
				return;
			}
		}
		else if(pev->team == 2){//�Ź�Gman
			if(pev->frags == 1700){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 1, 3 );

				CBaseEntity *pEntity = Create( "monster_sicker_zz", pEntity2->pev->origin + Vector(32,40,0), Vector(0,200,0), NULL );
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 1740){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity2 ){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->origin = pEntity2->pev->origin + Vector(-64,0,64);
						pSpot->pev->angles.y = 0;
					}	
				}
				sprintf( text, "Z.Z: Kadoma?\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1770){
				
				sprintf( text, "Z.Z: Why do you hesitate? Do it!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1820){
				pPlayer->Clear_SayText();

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle5_stop" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;

				pPlayer->pev->v_angle = Vector(0,0,0);
				pPlayer->pev->angles = Vector(0,0,0);
				pPlayer->pev->fixangle = TRUE;
				}
				pPlayer->m_ending_frags += 30;//�Ź�Gman!��Ʒֵ+30%
			}
			if(pev->frags == 1870){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->origin = pSpot->pev->origin + Vector(32,0,-16);
					pSpot->pev->angles.x = 45;
					pSpot->pev->angles.y = 180;
				}	
				sprintf( text, "Gman: ......\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1900){
				
				sprintf( text, "Gman: Looks like you realized that you are being manipulated.");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1950){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: Very clever.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP11", VOL_NORM, 0.5, 0, 100 );
				}
			}
			if(pev->frags == 1980){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: See you next time.\n");
					
				UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP10", VOL_NORM, 0.5, 0, 100 );
				}
			}
			if(pev->frags == 2025){
				pPlayer->Clear_SayText();
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 2.0, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 2030){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){//Gman���ص�����ʧ
				pEntity2->pev->effects = EF_NODRAW;
				pEntity2->pev->solid   = SOLID_NOT;
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "gman_center_downfloor" );
				if(pEntity2){
				UTIL_Remove( pEntity2 );
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "center_timer_lgt" );
				if(pEntity2){
				UTIL_Remove( pEntity2 );
				}
				FX_Explosion( pev->origin, 254 );//��������Ѫ������!
			}
			if(pev->frags == 2035){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
				pPlayer->EnableControl(TRUE);
				pPlayer->m_trainning = 0;
				pPlayer->pev->origin = pEntity2->pev->origin + Vector(-512,0,128);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->TeamMate_Nagamatagi_Teleport(1);//Bug Fix 3.0 Gmanս����Ѵ����һָ��ж�
				pPlayer->pev->velocity = g_vecZero;
				pPlayer->pev->v_angle = Vector(0,0,0);
				pPlayer->pev->angles = Vector(0,0,0);
				pPlayer->pev->fixangle = TRUE;
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );
				UTIL_Remove( pEntity2 );
				pPlayer->m_flVelocityModifier = -2;
				}
				UTIL_Remove( this );
				return;
			}
		}
	}
	if(pev->armortype == 83){//�¼�83 Doma���ն�ս
		if(pev->frags == 0){
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_game_rate = 98;//��Ϸ����98%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

			CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin, Vector(0,90,0), NULL );
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle1_angry" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 0, 3 );
			pEnemyMonster->SetBodygroup( 2, 7 );
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->avelocity.x = 1;
				pSpot->pev->velocity.y = 60;
				pSpot->pev->velocity.z = -20;
			}
		}
		if(pev->frags == 5){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 10){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
				if ( pEntity ){	
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					SetBits( pEntity->pev->effects, EF_DIMLIGHT);
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 70){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity ){	
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.x = 0;
					pSpot->pev->velocity.y = 0;
					pSpot->pev->velocity.z = 0;
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 90;
					pSpot->pev->origin = pEntity->pev->origin + Vector(0,-64,64);
				}
			}
			
			sprintf( text, "Doma: Ah...Kadoma.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 110){
			
			sprintf( text, "Doma: I am you from another world, here to kill you.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 160){
			
			sprintf( text, "Doma: After gaining the power of a dragon, my team betrayed me. I killed them all, and came to this world.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 230){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity ){	
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.y = 180;
					pSpot->pev->origin = pEntity->pev->origin + Vector(512,-256,64);
				}
			}
			
			sprintf( text, "Doma: Weird, huh? I don't know why, but all I want to do is fight.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 300){
			
			sprintf( text, "Doma: Come! Let's end this nonsense!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 370){
			pPlayer->Clear_SayText();
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity ){	
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "hen_sing" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;

				pEntity->pev->spawnflags |= SF_MONSTER_PRISONER;
				//Bug Fix 3.0 Doma����ǰ���¶��ѳ�޹���ƭȡ���ܤο��ܣ�

				pPlayer->pev->v_angle = Vector(0,90,0);
				pPlayer->pev->angles = Vector(0,90,0);
				pPlayer->pev->fixangle = TRUE;

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.x = 0;
					pSpot->pev->velocity.y = 0;
					pSpot->pev->velocity.z = 0;
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 90;
					pSpot->pev->origin = pEntity->pev->origin + Vector(0,-96,64);
				}
			}
		}
		if(pev->frags == 400){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,-256,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,90,0);
			pPlayer->pev->angles = Vector(0,90,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			pPlayer->m_flVelocityModifier = -2;
			}
			pPlayer->TeamMate_Nagamatagi_Teleport(8); //����������ͬ�� Bug Fix 1.0
		}
		if(pev->frags == 410){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity2 ){
			CBaseEntity *pEntity = Create( "monster_doma_boss", pEntity2->pev->origin + Vector(0,-256,8), pEntity2->pev->angles, NULL );
			pEntity->pev->spawnflags |= SF_MONSTER_PRISONER;
			pEntity->pev->takedamage = DAMAGE_NO;
			pEntity->pev->velocity = g_vecZero;
			UTIL_Remove( pEntity2 );
			}
		}
		if(pev->frags == 430){
			pPlayer->BOSS_Find();
			game_boss_battle = 1;
			pPlayer->m_music_save = 21;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 6\n");
			//SERVER_COMMAND("mp3 loop media/boss9.mp3\n");
		}
		if(pev->frags == 440){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma_boss" );
			if ( pEntity2 ){
			pEntity2->pev->spawnflags = 0;
			pEntity2->pev->takedamage = DAMAGE_AIM;
			}
		}
		if(pev->frags == 450){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 500){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma_boss" );
			if ( pEntity2 ){
				if(pEntity2->pev->deadflag == DEAD_NO || pPlayer->pev->deadflag != DEAD_NO){
					pev->frags = 480;
				}
			}
		}
		if(pev->frags == 540){//BOSSս����
			game_boss_battle = 0;
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 600){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 620){
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

			CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin, Vector(0,90,0), NULL );
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle6" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 0, 3 );
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);
			pEntity->pev->takedamage = DAMAGE_NO;
			pEnemyMonster->pev->flags |= FL_NOTARGET;

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity = g_vecZero;
				pSpot->pev->angles = g_vecZero;
				pSpot->pev->avelocity = g_vecZero;
				pSpot->pev->frags = 0;
				pSpot->pev->origin = pEntity->pev->origin + Vector(-128,32,64);
			}

			pPlayer->pev->origin = pev->origin + Vector(0,-768,0);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
		}
		if(pev->frags == 625){
			pPlayer->TeamMate_Nagamatagi_Teleport(2);
		}
		if(pev->frags == 630){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma_boss" );
			if ( pEntity2 ){
				UTIL_Remove( pEntity2 );
			}
			CBaseEntity *pEntity = Create( "monster_doma", pev->origin + Vector(0,64,0), Vector(0,270,0), NULL );
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "dead_duck" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEntity->pev->body = 0;
			pEntity->pev->takedamage = DAMAGE_NO;
			pEnemyMonster->SetBodygroup( 3, 2 );
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);
		}
		if(pev->frags == 700){
			pPlayer->m_fSelectMode = TRUE;
			pPlayer->m_fSelectNumber = 0;
			pPlayer->ShowVGUIMenu(37);
			pPlayer->m_load_check = 1;
		}
		if(pev->frags == 705){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 720){
			if(pPlayer->m_fSelectNumber == 0){
				if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
					pPlayer->ShowVGUIMenu(37);
					pPlayer->m_load_check = 1;
				}
				pev->frags = 710;
				pev->team = -1;
			}
		}
		if(pev->frags == 730){
			pev->team = pPlayer->m_fSelectNumber;
			pPlayer->m_fSelectNumber = 0;
			g_fGameJumpCG = 0;//��δ���
			pPlayer->Clear_SayText();
		}
		if(pev->team == 1){//ɱ��Doma
			if(pev->frags == 740){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "kadoma_last_atk1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;

				pPlayer->pev->v_angle = Vector(0,0,0);
				pPlayer->pev->angles = Vector(0,0,0);
				pPlayer->pev->fixangle = TRUE;
				}
			}
			if(pev->frags == 780){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity2 ){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->angles.y = 90;
						pSpot->pev->origin = pEntity2->pev->origin + Vector(0,-96,64);

						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->fixangle = TRUE;
					}
				}
			}
			if(pev->frags == 840){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity2 ){
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
				}
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.y = 270;
					pSpot->pev->origin = pEntity2->pev->origin + Vector(0,96,64);
				}
			}
			if(pPlayer->m_ending_frags > 20 && pPlayer->m_ending_frags < 80){//RP����
				if(pev->frags == 870){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "headache" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;

					CBaseEntity *pEntity = Create( "monster_sicker_zz", pEntity2->pev->origin + Vector(32,-40,0), Vector(0,110,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
				}
				if(pev->frags == 880){
					pPlayer->pev->fov = pPlayer->m_iFOV = -60;
				}
				if(pev->frags == 910){
					pPlayer->Game_Save_SecondData();
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 10, 10, 255, FFADE_IN );//��Ϲ���
				}
				if(pev->frags == 920){
					if(pPlayer->m_player_diamonds == 18){//ȫ�ռ�!
					g_fGameJumpCG = 14;
					}
					else{
					g_fGameJumpCG = 4;//���D����
					}
					SERVER_COMMAND( "map wdoor_ending_d\n" );//������D����ɱ��Scuide��
					return;
				}
			}
			else{//��ս����
				if(pev->frags == 890){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->angles.x = 0;
						pSpot->pev->angles.y = 270;
						pSpot->pev->velocity.y = -10;
						pSpot->pev->origin = pev->origin + Vector(0,-2048,256);
					}
				}
				if(pev->frags == 910){
					FireTargets( "god_hell_open_dr", this, this, USE_TOGGLE, 0 );
					
					sprintf( text, "- The Brave Kadoma and his team defeated Demon King Gman and the Evil Dragon Doma.\n");
					
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 950){
					
					sprintf( text, "- But the battle is not over. The door to the unknown opens again.\n");
				
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 1000){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 10, 10, 255, FFADE_IN );//��Ϲ���
					
					sprintf( text, "- The Final Battle\n");
					
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 1050){
					pev->frags = 1100;
					pPlayer->Clear_SayText();
				}
			}
		}
		else if(pev->team == 2){//�Ź�Doma
			if(pev->frags == 740){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity2 ){
				pEntity2->pev->angles.y = 270;
				pPlayer->pev->v_angle = Vector(0,90,0);
				pPlayer->pev->angles = Vector(0,90,0);
				pPlayer->pev->fixangle = TRUE;
				}
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.y = 90;
					pSpot->pev->origin = pEntity2->pev->origin + Vector(0,-96,64);
				}
			}
			if(pev->frags == 770){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_doma");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
					if(pPlayer->m_ending_frags >= 80){//Bug Fix 3.0 ���A·���Ѻöȹ��ߣ�doma�뿪
					FX_Explosion(pEntity2->Center(), EXPLOSION_DISPTELEPORT );
					UTIL_Remove( pEntity2 );
					pev->frags = 870;//������
					}
					else{
					UTIL_SetOrigin( pEntity2->pev, pEntity2->pev->origin + Vector(0,-16,0) );
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "doma_deadattack" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
				}
			}
			if(pev->frags == 810){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				if ( pEntity2 ){
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "kadoma_last_atk2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 890){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->velocity.y = -10;
				}
			}
			if(pPlayer->m_ending_frags > 20 && pPlayer->m_ending_frags < 80){//RP����
				if(pev->frags == 930){
					pPlayer->Game_Save_SecondData();
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 10, 10, 255, FFADE_IN );//��Ϲ���
				}
				if(pev->frags == 940){
					if(pPlayer->m_player_diamonds == 18){//ȫ�ռ�!
					g_fGameJumpCG = 15;
					}
					else{
					g_fGameJumpCG = 5;//���E����
					}
					SERVER_COMMAND( "map wdoor_ending_e\n" );//������E����ɱ��Victim��
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 10, 10, 255, FFADE_IN );//��Ϲ���
					return;
				}
			}
			else{//��ս����
				if(pev->frags == 930){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->angles.x = 0;
						pSpot->pev->angles.y = 270;
						pSpot->pev->velocity.y = -10;
						pSpot->pev->origin = pev->origin + Vector(0,-2048,256);
					}
				}
				if(pev->frags == 950){
					FireTargets( "god_hell_open_dr", this, this, USE_TOGGLE, 0 );
					
					sprintf( text, "- The Brave Kadoma and his team defeated Demon King Gman and the Evil Dragon Doma.\n");
					
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 990){
					
					sprintf( text, "- But the battle is not over. The door to the unknown opens again.\n");
					
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 1040){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 10, 10, 255, FFADE_IN );//��Ϲ���
					
					sprintf( text, "- The Final Battle\n");
					
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 1090){
					pev->frags = 1100;
					pPlayer->Clear_SayText();
				}
			}
		}
		if(pev->frags == 1120){
			FireTargets( "godzzhell_combat", this, this, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
		}
	}
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
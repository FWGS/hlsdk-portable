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
// ��CG�¼���ͳ��3
//=========================================================
class CMain_Event3 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
	Vector org_floor;
	char text[256];
};

LINK_ENTITY_TO_CLASS( main_cg_event_new3, CMain_Event3 );//����3

void CMain_Event3::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event3::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��3=================================================//
void CMain_Event3::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	if(pev->armortype == 33){//�¼�33 Xen��½
			if(pev->frags == 0){
				pPlayer->EnableControl(FALSE);
				pPlayer->Clear_SayText();
			}

			if(pev->frags == 20){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 25){
						pPlayer->EnableControl(TRUE);
						UTIL_ScreenFade( pPlayer, Vector(0,255,0), 1, 1, 255, FFADE_IN );
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->m_fPlayerHideMode = TRUE;
						pPlayer->pev->flags |= FL_NOTARGET;
						pPlayer->m_teleprort_in_xen = 1;

						CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "player_tel_xenstart" );
						if ( pEntity2 ){
							pPlayer->pev->origin = pEntity2->pev->origin;
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							pPlayer->pev->angles = Vector(0,270,0);
							pPlayer->pev->v_angle = Vector(0,270,0);
							pPlayer->pev->fixangle = TRUE;
							EMIT_SOUND(ENT(pPlayer->pev), CHAN_WEAPON, "debris/beamstart2old.wav", 1, 0.7);	
							UTIL_Remove( pEntity2 );
						}
			}
			if(pev->frags == 35){
						pPlayer->GiveNamedItem( "weapon_fist" );
						pPlayer->m_hasflashlight = TRUE;
						pPlayer->MenuItem_add(1);//�ֵ�ͲGet
						pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
			}
			if(pev->frags == 45){
						pPlayer->m_fPlayerHideMode = FALSE;
						pPlayer->pev->flags &= ~FL_NOTARGET;
			}
			if(pev->frags == 50){
					pPlayer->m_game_rate = 46;//��Ϸ����46%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 34){//�¼�34 Xen�ص���
			if(pev->frags == 0){
					pPlayer->EnableControl(FALSE);
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 30){
					pPlayer->EnableControl(TRUE);
					pPlayer->m_teleprort_in_xen = 0;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

					CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "player_tel_blackmesa" );
					if ( pEntity2 ){
						pPlayer->pev->origin = pEntity2->pev->origin;
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->pev->angles = Vector(0,0,0);
						pPlayer->pev->v_angle = Vector(0,0,0);
						pPlayer->pev->fixangle = TRUE;
						UTIL_Remove( pEntity2 );
					}
			}
			if(pev->frags == 80){
				FireTargets( "use_hgally_btn", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 100){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 110){
						pPlayer->EnableControl(FALSE);
						pPlayer->m_trainning = 1;
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 112){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pSpot->pev->origin + Vector(-120,140,-32), Vector(0,320,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
				}
			}
			if(pev->frags == 115){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					CBaseEntity *pHGally2 = UTIL_FindEntityByTargetname( NULL, "gally2" );
					if ( pHGally2 ){
					UTIL_SetOrigin( pHGally2->pev, pSpot->pev->origin + Vector(16,96,-32) );
					pHGally2->pev->angles.y = 180;
					}
					CBaseEntity *pHGally1 = UTIL_FindEntityByTargetname( NULL, "gally1" );
					if ( pHGally1 ){
					UTIL_SetOrigin( pHGally1->pev, pSpot->pev->origin + Vector(-32,64,-32) );
					pHGally1->pev->angles.y = 150;
					}
				}
			}
			if(pev->frags == 120){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pev->origin = pSpot->pev->origin;
						pSpot->pev->velocity.x -= 6;
						pSpot->pev->avelocity.y = -1;
					}
			}
			if(pev->frags == 140){
		
				sprintf( text, "- These two odd soldiers have been waiting here for awhile.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 180){
			
				sprintf( text, "- Their boss wants to see Kadoma after he escapes Black Mesa.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 220){
			
				sprintf( text, "- Things should be simple upon escape.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 280){
						pPlayer->EnableControl(TRUE);
						pPlayer->m_level_up_switch = FALSE;
						pPlayer->m_flVelocityModifier = -1;
						pPlayer->m_trainning = 0;
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->Clear_SayText();
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
			}
			if(pev->frags == 285){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
					if ( pSpot ){
						UTIL_Remove( pSpot );
					}
			}
			if(pev->frags == 290){
					CBaseEntity *pHGally2 = UTIL_FindEntityByTargetname( NULL, "gally2" );
					CBaseMonster *pEnemyMonster;
					if ( pHGally2 ){
					pEnemyMonster = pHGally2->MyMonsterPointer();
					pEnemyMonster->m_enemyfollower = 1;
					pEnemyMonster->m_walkaround = TRUE;
					pEnemyMonster->m_walkaroundFail = TRUE;
					pEnemyMonster->m_groundElev2 = TRUE;
					pPlayer->TeamMate_add(pEnemyMonster);
					}
					CBaseEntity *pHGally1 = UTIL_FindEntityByTargetname( NULL, "gally1" );
					if ( pHGally1 ){
					pEnemyMonster = pHGally1->MyMonsterPointer();
					pEnemyMonster->m_enemyfollower = 1;
					pEnemyMonster->m_walkaround = TRUE;
					pEnemyMonster->m_walkaroundFail = TRUE;
					pEnemyMonster->m_groundElev2 = TRUE;
					pPlayer->TeamMate_add(pEnemyMonster);
					}
			}
			if(pev->frags == 300){
					pPlayer->m_game_rate = 50;//��Ϸ����50%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 35){//�¼�35 ��ѧ��ͷBOSS
			if(pev->frags == 0){
					pPlayer->EnableControl(FALSE);
					pPlayer->Clear_SayText();
					pPlayer->m_trainning = 1;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 1){
					pPlayer->pev->origin = pev->origin + Vector(-64,0,0);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 3){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
					}
			}
			if(pev->frags == 5){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin, Vector(0,180,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 2, 5 );
					//pEnemyMonster->SetBodygroup( 3, 1 );
					pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;//�����ꡤֹͣ��˼��
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "m16_idle_angry" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetThink( NULL );
			}
			if(pev->frags == 7){
					CBaseEntity *pHGally2 = UTIL_FindEntityByTargetname( NULL, "gally2" );
					CBaseMonster *pEnemyMonster;
					if ( pHGally2 && pHGally2->pev->deadflag == DEAD_NO ){
					pEnemyMonster = pHGally2->MyMonsterPointer();
					pEnemyMonster->m_enemyfollower = 0;
					pEnemyMonster->m_walkaround = FALSE;
					pEnemyMonster->m_walkaroundFail = FALSE;
					pEnemyMonster->ClearSchedule();
					pEnemyMonster->m_lovehate += 30;//����Һøж�����?
					pEnemyMonster->m_hEnemy = NULL;
					pEnemyMonster->m_hOldEnemy[0] = NULL;
					pEnemyMonster->m_hOldEnemy[1] = NULL;
					pEnemyMonster->m_hOldEnemy[2] = NULL;
					pEnemyMonster->m_hOldEnemy[3] = NULL;
					pEnemyMonster->m_cAmmoLoaded  = pEnemyMonster->m_cClipSize;
					pEnemyMonster->SetActivity( ACT_IDLE );
					pEnemyMonster->RouteClear();
					pEnemyMonster->pev->angles.y = 180;
					UTIL_SetOrigin( pHGally2->pev, pev->origin + Vector(-16,64,-32) );
					}
					CBaseEntity *pHGally1 = UTIL_FindEntityByTargetname( NULL, "gally1" );
					if ( pHGally1 && pHGally1->pev->deadflag == DEAD_NO ){
					pEnemyMonster = pHGally1->MyMonsterPointer();
					pEnemyMonster->m_enemyfollower = 0;
					pEnemyMonster->m_walkaround = FALSE;
					pEnemyMonster->m_walkaroundFail = FALSE;
					pEnemyMonster->ClearSchedule();
					pEnemyMonster->m_lovehate += 30;//����Һøж�����?
					pEnemyMonster->m_hEnemy = NULL;
					pEnemyMonster->m_hOldEnemy[0] = NULL;
					pEnemyMonster->m_hOldEnemy[1] = NULL;
					pEnemyMonster->m_hOldEnemy[2] = NULL;
					pEnemyMonster->m_hOldEnemy[3] = NULL;
					pEnemyMonster->m_cAmmoLoaded  = pEnemyMonster->m_cClipSize;
					pEnemyMonster->SetActivity( ACT_IDLE );
					pEnemyMonster->RouteClear();
					pEnemyMonster->pev->angles.y = 180;
					UTIL_SetOrigin( pHGally1->pev, pev->origin + Vector(-16,-64,-32) );
					}
			}
			if(pev->frags == 20){
				FireTargets( "locked_bossdrb", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 30){//����ǽx2
				CBaseEntity *pOrgPoint = UTIL_FindEntityByTargetname( NULL, "boss_feild_wall1_m" );
				if ( pOrgPoint){
				CBaseEntity *pAirWall = Create( "wrongdoor_airwall", pOrgPoint->pev->origin+Vector(-20,20,-20), Vector(0,0,0), NULL );
				UTIL_SetSize ( pAirWall->pev, Vector(-16,-64,0), Vector(16,64,128));
				}

				pOrgPoint = UTIL_FindEntityByTargetname( NULL, "boss_feild_wall2_m" );
				if ( pOrgPoint){
				CBaseEntity *pAirWall = Create( "wrongdoor_airwall", pOrgPoint->pev->origin+Vector(-20,20,-20), Vector(0,0,0), NULL );
				UTIL_SetSize ( pAirWall->pev, Vector(-16,-64,0), Vector(16,64,128));
				}
			}
			if(pev->frags == 45){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 50){
						pPlayer->EnableControl(TRUE);
						pPlayer->m_level_up_switch = FALSE;
						pPlayer->m_flVelocityModifier = -2;
						pPlayer->m_trainning = 0;
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->Clear_SayText();
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
						game_boss_battle = 1;
			}
			if(pev->frags == 51){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
					if ( pSpot ){
						UTIL_Remove( pSpot );
					}
			}
			if(pev->frags == 55){
					pPlayer->BOSS_Find();
			}
			if(pev->frags == 60){
			pPlayer->m_music_save = 7;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 12\n");
			//SERVER_COMMAND("mp3 loop media/boss3.mp3\n");
			}
			if(pev->frags == 65){//����һ��һ��һ��һ��������������ʵ�壡
				int clear_num = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) || pEntity->pev->deadflag == DEAD_DEAD ){
								if(pEntity->pev->origin.x > pev->origin.x + 200){//���X�Ჿ�ֹ���
								UTIL_Remove( pEntity );
								clear_num++;
								}
							}
						}
					//	sprintf( text, "ClearEnt: %d\n",clear_num);
					//	UTIL_SayTextAll( text,this );		
			}
			if(pev->frags == 75){
				FireTargets( "boss_rbmaker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 80){
				FireTargets( "boss_rbgate_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 120){
					CBaseEntity *pBoss = UTIL_FindEntityByClassname( NULL, "monster_scihead_boss");
					if ( pBoss ){
						if(pBoss->pev->deadflag == DEAD_NO){
						pev->frags = 110;//BOSS����ж�
						}
						if(pBoss->pev->weapons == 1){
						pBoss->pev->weapons = 2;
						FireTargets( "boss_tur", this, this, USE_TOGGLE, 0 );//BOSS��������
						}
					}
			}
			if(pev->frags == 140){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
					game_boss_battle = 0;
					pPlayer->m_music_save = 0;
					SERVER_COMMAND("mp3 stop\n");
			}
			if(pev->frags == 141){
					//����һ��һ��һ��һ����������ɱ��ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if (  (pEntity->pev->flags & FL_MONSTER) ){
							if(FClassnameIs ( pEntity->pev, "monster_else_rabbit" )
							|| FClassnameIs ( pEntity->pev, "monster_miniturret" )
							|| FClassnameIs ( pEntity->pev, "monster_turret" )){
							pEntity->Killed( pEntity->pev, GIB_NEVER );//����
							}
							if(FClassnameIs ( pEntity->pev, "monster_scihead_boss" )){
							UTIL_Remove( pEntity );
							}
						}
					}
	
				FireTargets( "boss_rbmaker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 147){
					CBaseEntity *pHGally2 = UTIL_FindEntityByTargetname( NULL, "gally2" );
					if ( pHGally2 && pHGally2->pev->deadflag == DEAD_NO ){
					pHGally2->Killed( pev, GIB_NEVER );//ǿ������
					}
					CBaseEntity *pHGally1 = UTIL_FindEntityByTargetname( NULL, "gally1" );
					if ( pHGally1 && pHGally1->pev->deadflag == DEAD_NO ){
					pHGally1->Killed( pev, GIB_NEVER );//ǿ������
					}
			}
			if(pev->frags == 170){
					//�������
					pPlayer->TeamMate_Nagamatagi_Allclear(0);
			}
			if(pev->frags == 180){
				FireTargets( "bosskill_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 200){
					pPlayer->m_game_rate = 52;//��Ϸ����52%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 36){//�¼�36 �ж����ڻң��ǳ��¼�
			if(pev->frags == 0){
					pPlayer->EnableControl(FALSE);
					pPlayer->Clear_SayText();
					pPlayer->m_trainning = 1;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 1){
					pPlayer->pev->origin = pev->origin + Vector(0,0,38);
					pPlayer->pev->velocity = g_vecZero;
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 10){
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 1024 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) && pEntity->pev->deadflag == DEAD_NO ){
								if(FClassnameIs ( pEntity->pev, "monster_human_grunt_ally")){
								pEnemyMonster = pEntity->MyMonsterPointer();
								pEnemyMonster->m_longming = 0;
								pEnemyMonster->m_no_pov_limit = 1;
								pEnemyMonster->m_enemyfollower = 1;
								pPlayer->TeamMate_add(pEnemyMonster);
								}
								else if(FClassnameIs ( pEntity->pev, "monster_lelite")){
								pEnemyMonster = pEntity->MyMonsterPointer();
								pEnemyMonster->m_longming = 0;
								pEnemyMonster->m_no_pov_limit = 1;
								pEnemyMonster->m_iTriggerCondition = 0;//����ҪNPC���޴ȱ���
								pEnemyMonster->m_hTargetEnt = pPlayer;
								pPlayer->TeamMate_add(pEnemyMonster);
								}
							}
						}
						
			}
			if(pev->frags == 20){
				CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
			}
			if(pev->frags == 30){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
							EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!LELITE_1", VOL_NORM, 0.5, 0, PITCH_NORM );
							
							sprintf( text, "Lelite: I am Lelite, the commander of this squad.\n");
							
							UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 70){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!LELITE_2", VOL_NORM, 0.5, 0, PITCH_NORM );
							
							sprintf( text, "Lelite: The enemy is here in force. Good luck.\n");
							
							UTIL_SayTextAll( text,this );
						}
					}
			}
			if(pev->frags == 120){
						pPlayer->EnableControl(TRUE);
						pPlayer->m_level_up_switch = FALSE;
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->m_trainning = 0;
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 125){//����һ��һ��һ��һ��������������ʵ�壡
			//	int clear_num = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) 
							|| pEntity->pev->deadflag == DEAD_DEAD
							|| pEntity->pev->movetype == MOVETYPE_TOSS){
								if(pEntity->pev->origin.x > pev->origin.x && 
								!FClassnameIs ( pEntity->pev, "monster_human_grunt") ){
								//���X�Ჿ�ֹ�����������ߣ���������hecu
								UTIL_Remove( pEntity );
								//clear_num++;
								}
							}
						}	
			}
			if(pev->frags == 130){
				FireTargets( "hecu_sniper_maker", this, this, USE_TOGGLE, 0 );
				//�ӹ�!
				Create( "monster_human_assault", Vector(-2583,191,-476), Vector(0,270,0), NULL );
				Create( "monster_human_grunt_medic", Vector(-2773,-583,-468), Vector(0,0,0), NULL );
				Create( "monster_human_grunt_medic", Vector(-1930,-323,-332), Vector(0,270,0), NULL );
				//Bug Fix 3.0 ��׷��һֻҽ�Ʊ����ѶȲ�֪���ӻ����½���
			}
			if(pev->frags == 135){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 140){
				pPlayer->m_game_rate = 54;//��Ϸ����54%
				UTIL_Remove( this );
				return;
			}
	}
	else if(pev->armortype == 37){//�¼�37 �ж�˲��
			if(pev->frags == 0){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 2){
				CBaseMonster *pEnemyMonster;
				CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) && pEntity->pev->deadflag == DEAD_NO ){
								if(FClassnameIs ( pEntity->pev, "monster_lelite") || FClassnameIs ( pEntity->pev, "monster_human_grunt_ally")){
								pEnemyMonster = pEntity->MyMonsterPointer();
								pEnemyMonster->m_hEnemy = NULL;
								pEnemyMonster->m_hOldEnemy[0] = NULL;
								pEnemyMonster->m_hOldEnemy[1] = NULL;
								pEnemyMonster->m_hOldEnemy[2] = NULL;
								pEnemyMonster->m_hOldEnemy[3] = NULL;
								pEnemyMonster->m_lovehate += 30;//����Һøж�����?
								//pEnemyMonster->TakeHealth(30, DMG_GENERIC);//�ָ�����������ֵ
								pEnemyMonster->m_cAmmoLoaded  = pEnemyMonster->m_cClipSize;
								pEnemyMonster->SetActivity( ACT_IDLE );
								pEnemyMonster->RouteClear();
								pEnemyMonster->pev->angles.y = 90;
								UTIL_SetOrigin( pEntity->pev, pev->origin);
								pev->origin.x += 64;
								}
							}
						}
						
			}
			if(pev->frags == 40){//����һ��һ��һ��һ��������������ʵ�壡
				//int clear_num = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) 
							|| pEntity->pev->deadflag == DEAD_DEAD
							|| pEntity->pev->movetype == MOVETYPE_TOSS){
								if(pEntity->pev->origin.y + 16 < pev->origin.y ){
								//����������ں���Ĺ���!
								UTIL_Remove( pEntity );
								//clear_num++;
								}
							}
						}
						//sprintf( text, "ClearEnt: %d\n",clear_num);
						//UTIL_SayTextAll( text,this );
						
			}
			if(pev->frags == 50){
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 38){//�¼�38 ��ɽ������
			if(pev->frags == 0){
				FireTargets( "monster_test_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 20){
				FireTargets( "fight_gate_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 30){
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 40){
				CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy6");
				if ( pSpot ){
					CBaseEntity *pMS1 = Create( "monster_headcrab", pSpot->pev->origin, Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pMS1->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy7");
				if ( pSpot ){
					CBaseEntity *pMS1 = Create( "monster_headcrab", pSpot->pev->origin, Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pMS1->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy8");
				if ( pSpot ){
					CBaseEntity *pMS1 = Create( "monster_headcrab", pSpot->pev->origin, Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pMS1->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy9");
				if ( pSpot ){
					CBaseEntity *pMS1 = Create( "monster_headcrab", pSpot->pev->origin, Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pMS1->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy10");
				if ( pSpot ){
					CBaseEntity *pMS1 = Create( "monster_headcrab", pSpot->pev->origin, Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pMS1->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
			}
			if(pev->frags == 55){
						pPlayer->m_mode_int1 = 1;//�ؿ�
						pPlayer->m_mode_int2 = -1;//ѡ��ĵ�λ
						pPlayer->m_mode_int3 = 0;
						pPlayer->m_music_save = 2;
						SERVER_COMMAND("mp3 loop media/music12.mp3\n");
			}
select_npc:
			if(pev->frags == 57){
					FireTargets( "red_btn_gai", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 64){//56-64,ѡ��λ
						if(pPlayer->m_mode_int2 != -1){
							if(pPlayer->m_mode_int2 == 1){//�ٻ�����ѧ��
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally1");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_scientist", pSpot->pev->origin, Vector(0,210,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->m_walkaround = TRUE;
									pMS1->pev->weapons = -1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally2");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_scientist", pSpot->pev->origin, Vector(0,240,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->m_walkaround = TRUE;
									pMS1->pev->weapons = -1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally3");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_scientist", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->m_walkaround = TRUE;
									pMS1->pev->weapons = -1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally4");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_scientist", pSpot->pev->origin, Vector(0,300,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->m_walkaround = TRUE;
									pMS1->pev->weapons = -1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally5");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_scientist", pSpot->pev->origin, Vector(0,330,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->m_walkaround = TRUE;
									pMS1->pev->weapons = -1;
								}
							}
							else if(pPlayer->m_mode_int2 == 2){//�ٻ�������
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally1");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_else_rabbit", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->pev->health = 180;//��Ѫ
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally2");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_else_rabbit", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->pev->health = 180;//��Ѫ
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally3");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_else_rabbit", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->pev->health = 180;//��Ѫ
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally4");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_else_rabbit", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->pev->health = 180;//��Ѫ
								}
							}
							else if(pPlayer->m_mode_int2 == 3){//�ٻ�������
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally2");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_barney", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally3");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_barney", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pMS1->pev->skin = 1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally4");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_barney", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pMS1->pev->skin = 2;
								}
							}
							else if(pPlayer->m_mode_int2 == 4){//�ٻ�����ɫ����
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally2");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_barney_shield", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally4");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_barney_shield", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
								}
							}
							else if(pPlayer->m_mode_int2 == 5){//�ٻ����ֻ�
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally3");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_giant", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pMS1->pev->health = 900;//��Ѫ
								}
							}
							else if(pPlayer->m_mode_int2 == 6){//�ٻ����Ǹ�
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally3");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_dengor", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pEnemyMonster->m_fightmode = 1;
									pEnemyMonster->m_walkaround = FALSE;
									pMS1->pev->health = 900;//��Ѫ
								}
							}
							else if(pPlayer->m_mode_int2 == 7){//�ٻ����ȴ�
								CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally2");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_human_grunt", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pMS1->pev->health = 120;//��Ѫ
									pEnemyMonster->m_fightmode = 1;
								}
								pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_ally4");
								if ( pSpot ){
									CBaseEntity *pMS1 = Create( "monster_human_grunt", pSpot->pev->origin, Vector(0,270,0), NULL );
									CBaseMonster *pEnemyMonster;
									pEnemyMonster = pMS1->MyMonsterPointer();
									pMS1->pev->health = 120;//��Ѫ
									pEnemyMonster->m_fightmode = 1;
								}
							}
						}
						else{
							pev->frags = 58;
						}//ѡ��ĵ�λ
			}
combat_fight:
			if(pev->frags == 80){//��ս��
					FireTargets( "fight_white_wall", this, this, USE_TOGGLE, 0 );//��ǽ�ر�
					if(pev->impulse == 0 || pev->impulse == 2){
					FireTargets( "red_btn_gai", this, this, USE_TOGGLE, 0 );
					}
			}
			if(pev->frags == 83){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 100){//����һ��һ��һ��һ�������������ʵ�壡
				int alive_ally = 0,alive_enemy = 0;
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 2048 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) && pEntity->pev->deadflag == DEAD_NO){
								if(pEntity->pev->origin.x > pev->origin.x - 192 ){
									if(pEntity->Classify() == CLASS_PLAYER_ALLY 
									|| pEntity->Classify() == CLASS_HUMAN_ASS){
										alive_ally++;
									}
									else{
										alive_enemy++;
									}
								}
							}
						}

						if(alive_ally != 0 && alive_enemy != 0){
							pev->frags = 85;
						}
						//sprintf( text, "ClearEnt: %d\n",clear_num);
						//UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 120){//�غϽ������峡
					FireTargets( "fight_gate_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 130){//���︴λ
				int alive_ally = 0,alive_enemy = 0;
				Vector new_origin;
				pPlayer->Clear_SayText();

						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 2048 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER)){
								if(pEntity->pev->origin.x > pev->origin.x - 192 ){

									if(pEntity->pev->deadflag != DEAD_NO){
									UTIL_Remove( pEntity );
									continue;
									}

									if(pEntity->Classify() == CLASS_PLAYER_ALLY 
									|| pEntity->Classify() == CLASS_HUMAN_ASS){
										alive_ally++;
											if(alive_ally == 1){
											new_origin.x = pev->origin.x - 128;
											}
											else if(alive_ally == 2){
											new_origin.x = pev->origin.x - 64;
											}
											else if(alive_ally == 3){
											new_origin.x = pev->origin.x;
											}
											else if(alive_ally == 4){
											new_origin.x = pev->origin.x + 64;
											}
											else if(alive_ally == 5){
											new_origin.x = pev->origin.x + 128;
											}

											if ( FClassnameIs(pEntity->pev,"monster_dengor") || FClassnameIs(pEntity->pev,"monster_giant")){
											new_origin.x = pev->origin.x;//�Ǹ�/�ֻ� ��λ
											}

											new_origin.y = pev->origin.y + 350;
											new_origin.z = pev->origin.z - 50;
											UTIL_SetOrigin( pEntity->pev, new_origin);
										pEntity->pev->angles.y = 270;
									}
									else{
										alive_enemy++;
												if(pPlayer->m_mode_int1 == 10){
												new_origin.x = pev->origin.x;
												new_origin.y = pev->origin.y - 290;
												}
												else if(alive_enemy == 1){
												new_origin.x = pev->origin.x - 128;
												new_origin.y = pev->origin.y - 290;
												}
												else if(alive_enemy == 2){
												new_origin.x = pev->origin.x - 64;
												new_origin.y = pev->origin.y - 290;
												}
												else if(alive_enemy == 3){
												new_origin.x = pev->origin.x;
												new_origin.y = pev->origin.y - 290;
												}
												else if(alive_enemy == 4){
												new_origin.x = pev->origin.x + 64;
												new_origin.y = pev->origin.y - 290;
												}
												else if(alive_enemy == 5){
												new_origin.x = pev->origin.x + 128;
												new_origin.y = pev->origin.y - 290;
												}
												else if(alive_enemy == 6){
												new_origin.x = pev->origin.x - 128;
												new_origin.y = pev->origin.y - 350;
												}
												else if(alive_enemy == 7){
												new_origin.x = pev->origin.x - 64;
												new_origin.y = pev->origin.y - 350;
												}
												else if(alive_enemy == 8){
												new_origin.x = pev->origin.x;
												new_origin.y = pev->origin.y - 350;
												}
												else if(alive_enemy == 9){
												new_origin.x = pev->origin.x + 64;
												new_origin.y = pev->origin.y - 350;
												}
												else if(alive_enemy == 10){
												new_origin.x = pev->origin.x + 128;
												new_origin.y = pev->origin.y - 350;
												}

												if(pPlayer->m_mode_int1 == 7){
												new_origin.z = pev->origin.z - 30;
												}
												else{
												new_origin.z = pev->origin.z - 50;
												}

												UTIL_SetOrigin( pEntity->pev, new_origin);
											pEntity->pev->angles.y = 90;
										}
								}
							}
				}

									if(alive_enemy == 0){
										pPlayer->m_mode_int1 += 1;//����ȫ�壬��ת��һ��
										
										if(alive_ally == 0){
										pPlayer->m_mode_int3 += 1;//�ҷ�����
										pev->impulse = 2;//ͬ���ھ����������ɵ���+ѡ���ҷ���λ
										}
										else{
										pev->impulse = 1;//�������ɵ���,�ҷ����
										}
									}
									else{			
										pPlayer->m_mode_int3 += 1;//�ҷ�����
										pev->impulse = 0;//����ѡ���ҷ���λ
									}

									if(pPlayer->m_mode_int3 >= 7){//��Ϸ����
									pev->impulse = 3;
									pev->frags = 200;
									}
									else if(pPlayer->m_mode_int1 >= 11){//��Ϸ����
									pev->impulse = 4;
									pev->frags = 200;
									}

						FireTargets( "fight_white_wall", this, this, USE_TOGGLE, 0 );//��ǽ����
						//sprintf( text, "ClearEnt: %d\n",clear_num);
						//UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 150){//�峡����
					FireTargets( "fight_gate_dr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 170){//��������
						if(pPlayer->m_mode_int1 == 2 && pev->impulse >= 1){//�ڶ���
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy6");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_zombie", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy7");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_zombie", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy8");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_zombie", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy9");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_zombie", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy10");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_zombie", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 3 && pev->impulse >= 1){//������
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy6");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_houndeye", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy7");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_houndeye", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy8");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_houndeye", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy9");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_houndeye", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy10");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_houndeye", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 4 && pev->impulse >= 1){//���Ĺ�
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy7");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_bullchicken", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy9");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_bullchicken", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 5 && pev->impulse >= 1){//�����
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy6");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_slave", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy7");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_slave", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy9");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_slave", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy10");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_slave", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 6 && pev->impulse >= 1){//������
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy7");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_grunt", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy9");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_grunt", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 7 && pev->impulse >= 1){//���߹�
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy6");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_controller", pSpot->pev->origin + Vector(0,0,16), Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy8");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_controller", pSpot->pev->origin + Vector(0,0,16), Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy10");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_alien_controller", pSpot->pev->origin + Vector(0,0,16), Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 8 && pev->impulse >= 1){//�ڰ˹�
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy2");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_vortigaunt", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy4");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_vortigaunt", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 9 && pev->impulse >= 1){//�ھŹ�
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy6");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_gonome", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
							pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy10");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_gonome", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
							}
						}
						else if(pPlayer->m_mode_int1 == 10 && pev->impulse >= 1){//��ʮ��
							CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "msfight_enemy8");
							if ( pSpot ){
								CBaseEntity *pMS1 = Create( "monster_bigmomma", pSpot->pev->origin, Vector(0,90,0), NULL );
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pMS1->MyMonsterPointer();
								pEnemyMonster->m_fightmode = 1;
								pEnemyMonster->pev->health = 900;//Ѫ������
							}
						}
			}

			if(pev->impulse == 0 || pev->impulse == 2){//�ҷ�ȫ��
				if(pev->frags == 180){
						pPlayer->m_mode_int2 = -1;//ѡ��ĵ�λ
						pev->frags = 55;
						goto select_npc;
				}
			}
			else{
				if(pev->frags == 180){
						pev->frags = 70;//ֱ�ӽ�����һ��ս��
						goto combat_fight;
				}
			}

			if(pev->frags >= 210){//��Ϸ����
							pPlayer->m_mode_int1 = 0;
							pPlayer->m_mode_int2 = 0;
							pPlayer->m_mode_int3 = 0;
							pPlayer->m_music_save = 0;
							SERVER_COMMAND("mp3 stop\n");
						if(pev->impulse == 4){
						FireTargets( "fight_win_bounce", this, this, USE_TOGGLE, 0 );//ʤ����Ʒ
						}
						FireTargets( "fight_over_door", this, this, USE_TOGGLE, 0 );//���ڿ���

						UTIL_Remove( this );
						return;
			}

	}
	else if(pev->armortype == 39){//�¼�39 GHԱ��
			if(pev->frags == 0){
					//�������
					pPlayer->TeamMate_Nagamatagi_Allclear(0);
			}
			if(pev->frags == 20){
					CBaseEntity *pGH = UTIL_FindEntityByTargetname( NULL, "ghworker" );
					if ( pGH ){
					pGH->pev->weapons = 167;
					}
			}
			if(pev->frags == 25){
					FireTargets( "ghworker_walk1", this, this, USE_TOGGLE, 0 );//���ڿ���
			}
			if(pev->frags == 30){
					pPlayer->m_game_rate = 57;//��Ϸ����57%
					pPlayer->Clear_SayText();
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 40){//�¼�40 ��ͷս��
			if(pev->frags == 10){
					FireTargets( "out_exit_elevdr", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 20){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 30){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
					if ( pEntity ){
						if(pEntity->pev->deadflag == DEAD_NO){
							pPlayer->m_ending_frags += 5;//�����ش���Ʒֵ+5!
						}
					}
					
					sprintf( text, "???: Drive the tram to New Nippori.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 110){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 130){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.5, 5.0, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 170){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
					CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "exit_blackmesa" );
					if ( pEntity2 ){
						pPlayer->pev->origin = pEntity2->pev->origin;
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->pev->velocity = pev->velocity;
					}
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 41){//�¼�41 ���ٹ�·
			if(pev->frags == 10){
					CBaseEntity *Flyer = UTIL_FindEntityByTargetname( NULL, "osprey" );
					if ( Flyer ){//�����
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = Flyer->MyMonsterPointer();
							pEnemyMonster->pev->sequence = 1;
							pEnemyMonster->ResetSequenceInfo( );
							pEnemyMonster->pev->frame = 0;
							pEnemyMonster->pev->framerate = 0;
							pEnemyMonster->SetThink( NULL );
					}
			}
			if(pev->frags == 20){
					CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "rape_car" );
					if ( pEntity2 ){
					UTIL_SetOrigin(pEntity2->pev, pEntity2->pev->origin + Vector(0,1536,0));
					}
			}
			
			if(pev->frags == 30){
			pPlayer->m_music_save = 10;
			SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 40){
					CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "drive_car_tel" );
						if ( pEntity2 ){
							pPlayer->pev->origin = pEntity2->pev->origin;
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							pPlayer->pev->velocity = pev->velocity;
							pPlayer->pev->angles = Vector(0,90,0);
							pPlayer->pev->v_angle = Vector(0,90,0);
							pPlayer->pev->fixangle = TRUE;
						}
			}
			if(pev->frags == 45){
					CLIENT_COMMAND(pPlayer->edict(), "cd loop 13\n");
					//SERVER_COMMAND("mp3 play media/music13.mp3\n");
			}
			if(pev->frags == 70){
					FireTargets( "bullck_maker1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 75){
					FireTargets( "bullck_maker2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 80){
					FireTargets( "bullck_maker3", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 85){
					FireTargets( "bullck_maker4", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 90){
					FireTargets( "bullck_maker5", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 95){
					FireTargets( "bullck_maker6", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 100){
					FireTargets( "bullck_maker7", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 105){
					FireTargets( "bullck_maker8", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 110){
					FireTargets( "bullck_maker9", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 115){
					FireTargets( "bullck_maker10", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 120){
					pPlayer->m_game_rate = 60;//��Ϸ����60%
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 42){//����֮ɭ
		if(pev->frags == 0){
				pPlayer->RemoveAllItems( FALSE );
				pPlayer->pev->armorvalue = 0;
				pPlayer->m_skill_maxarmor = 0;
				pPlayer->m_iClientBattery = -1;
		}
		if(pev->frags == 25){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_misaliya_1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 40){
				pPlayer->EnableControl(FALSE);
				pPlayer->Clear_SayText();
				pPlayer->m_trainning = 1;
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
					pSpot->pev->velocity.x += 60;
					pSpot->pev->avelocity.x = 1;
				}
		}
		if(pev->frags == 90){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
				pSpot->pev->armortype = 10;
				}
		}
		if(pev->frags == 150){
				FireTargets( "misaliya_1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 160){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 180){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
				pSpot->pev->velocity.x = 0;
				pSpot->pev->avelocity.x = 0;
				pSpot->pev->armortype = 0;
				}
		}
		if(pev->frags == 185){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "leg_catch" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
		}
		if(pev->frags == 190){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 200){
				pPlayer->m_fSelectMode = TRUE;
				pPlayer->m_fSelectNumber = 0;
				pPlayer->ShowVGUIMenu(34);
				pPlayer->m_load_check = 1;
		}
		if(pev->frags == 207){
				if(pPlayer->m_fSelectNumber == 0){
					if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
						pPlayer->ShowVGUIMenu(34);
						pPlayer->m_load_check = 1;
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						}
					}
				pev->frags = 203;
				pev->team = -1;
				}
		}
		if(pev->frags == 220){
				pev->team = pPlayer->m_fSelectNumber;
				pPlayer->m_fSelectNumber = 0;
				pPlayer->pev->angles = Vector(0,90,0);
				pPlayer->pev->v_angle = Vector(0,90,0);
				pPlayer->pev->fixangle = TRUE;
		}
		if(pev->frags == 225){
			if(pev->team == 1){//kadoma ץס��δ�������ţ�
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_misaliya_2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			else if(pev->team == 2){//kadoma ���
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_misaliya_3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			else if(pev->team == 3){//kadoma װ��
			//		FireTargets( "misaliya_2", this, this, USE_TOGGLE, 0 );
			}
			else if(pev->team == 4){//kadoma ����
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_misaliya_4" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
		}
		if(pev->frags == 229){
			if(pev->team == 1){//kadoma ץס��δ�������ţ�
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){		
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "jump_atk" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
		}
		if(pev->frags == 250){
			if(pev->team == 2 || pev->team == 3){//��kadoma����
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){		
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "heal_duck" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
					if(pev->team == 2){//�øж���΢����
					pEnemyMonster->m_lovehate += 10;
					}
				}
			}

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				pEntity->pev->framerate = 0;
				}
		}
		if(pev->frags == 245){
			if(pev->team == 1){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 6.0, 255, FFADE_OUT );
					pPlayer->m_iClient_Gameover = -1;
					pPlayer->pev->health = 0;//��ֹ�浵!
					pPlayer->m_fGameOverTime = gpGlobals->time + 4;
					return;
			}
			else if(pev->team == 4){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){		
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
		}
		if(pev->frags == 260){
			if(pev->team == 4){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 6.0, 255, FFADE_OUT );
					pPlayer->m_iClient_Gameover = -1;
					pPlayer->pev->health = 0;//��ֹ�浵!
					pPlayer->m_fGameOverTime = gpGlobals->time + 4;
					return;
			}
		}
		if(pev->frags == 300){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 2.0, 255, FFADE_IN );
		}
		if(pev->frags == 302){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pSpot ){
					CBaseEntity *pAimFlag = Create( "npc_attack_flag", pSpot->pev->origin + Vector(350,600,64), Vector(0,0,0), NULL );
					pAimFlag->pev->health = 180;
					pAimFlag->pev->frags = 6;
					pAimFlag->pev->team = 1;//�����ڿ��ƻ���
				}
		}
		if(pev->frags == 305){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					pPlayer->pev->origin = pEntity->pev->origin;
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->pev->angles = Vector(0,0,0);
					pPlayer->pev->v_angle = Vector(0,0,0);
					pPlayer->pev->fixangle = TRUE;
					pPlayer->m_flVelocityModifier = -5;
					pPlayer->m_trainning = 0;
					pPlayer->m_hasflashlight = FALSE;
					pPlayer->pev->health = pPlayer->pev->max_health;
					pPlayer->EnableControl(TRUE);
					UTIL_Remove( pEntity );
				}
		}
		if(pev->frags == 315){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_godmode = TRUE;
				}
		}
		if(pev->frags == 320){
				pPlayer->m_flVelocityModifier = -5;
				pPlayer->GiveNamedItem( "weapon_fist" );
				pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
				pPlayer->MenuItem_add(1);//��ʼ�ֵ�Ͳ Bug Fix 1.0
				pPlayer->m_hasflashlight = TRUE;
		}
		if(pev->frags == 350){
				pPlayer->m_flVelocityModifier = -5;
				
				sprintf( text, "???: Who are you, and why're you...?\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 400){
				
				sprintf( text, "???: Oh, you want to go to New Nippori?\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 450){
				
				sprintf( text, "???: You're in Old Nippori - I'm searching for something here.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 500){
				
				sprintf( text, "Misaliya: I'm Misaliya - a mage.\n");
				
				UTIL_SayTextAll( text,this );

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_enemyfollower = 1;
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->m_godmode = FALSE;
				}
		}
		if(pev->frags == 550){
				
				sprintf( text, "Misaliya: We should continue together.\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 600){
			
				sprintf( text, "Misaliya: If you get hurt, I can heal you again.\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 670){
				pPlayer->m_game_rate = 63;//��Ϸ����63%
				pPlayer->m_load_check = 0;
				pPlayer->Clear_SayText();
				UTIL_Remove( this );
				return;
		}
	}
	else if(pev->armortype == 43){//����
		if(pev->frags == 0){
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 10){
			
				sprintf( text, "Misaliya: I can't go through here; it's too dangerous.\n");
		
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 50){
			
				sprintf( text, "Misaliya: Huh? You'll carry me through?\n");
		
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 90){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 100){
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 110){
				pPlayer->m_skill_defguard = 2;
		
				sprintf( text, "- Kadoma Learning New Skill! (Defensive array)\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- Aimed friendly units press T (Use / Cancel) to form a defensive formation.\n");
				UTIL_SayTextAll( text,this );
			
		}
		if(pev->frags == 125){
					if(pPlayer->m_guard_mynpc == 1){
					FireTargets( "npcguard_wall", this, this, USE_TOGGLE, 0 );
					}
					else{
					pev->frags = 115;
					}
		}
		if(pev->frags == 135){
			
				sprintf( text, "Misaliya: This is a little...well, don't fall.\n");
		
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 195){
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 200){
				pPlayer->m_game_rate = 64;//��Ϸ����64%
				UTIL_Remove( this );
				return;
		}
	}
	else if(pev->armortype == 44){//ʥ�����������ɣ����Ž���
		if(pev->frags == 0){
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->m_trainning = 1;
						pPlayer->pev->angles = Vector(0,0,0);
						pPlayer->pev->v_angle = Vector(0,0,0);
						pPlayer->pev->fixangle = TRUE;
						pPlayer->pev->origin = pev->origin + Vector(-384,0,64);
						pPlayer->m_stuck_origin = pev->origin + Vector(-384,0,64);
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 2.0, 255, FFADE_IN );
						pPlayer->m_wdoor_mynpc = NULL;
						pPlayer->m_guard_mynpc = 0;

						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.x += 2;
							pSpot->pev->avelocity.x = -1;
						}
		}
		if(pev->frags == 5){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_enemyfollower = 0;
				pEntity->pev->angles.y = 0;
				pEnemyMonster->pev->velocity = g_vecZero;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->RouteClear();
				pEnemyMonster->m_lovehate += 10;
				pEnemyMonster->m_hEnemy = NULL;
				pEnemyMonster->m_hOldEnemy[0] = NULL;
				pEnemyMonster->m_hOldEnemy[1] = NULL;
				pEnemyMonster->m_hOldEnemy[2] = NULL;
				pEnemyMonster->m_hOldEnemy[3] = NULL;
				pEnemyMonster->m_playerguardian_mode = 0;
				pEnemyMonster->pev->flags &= ~FL_NOTARGET;
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEntity->pev->movetype = MOVETYPE_STEP;
				pEntity->pev->owner = NULL;
				UTIL_SetOrigin( pEntity->pev, pev->origin - Vector(80,20,-30) );
				}
		}
		if(pev->frags == 10){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev, pev->origin);
					pEntity->pev->angles.y = 180;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
				else{
					pEntity = Create( "monster_kadoma", pev->origin, Vector(0,180,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 30){
			
				sprintf( text, "Misaliya: The dark holy sword of legend...\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 70){
		
			sprintf( text, "Misaliya: Only the brave can use it. Want to give it a try?\n");
			
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 115){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->velocity.x = 0;
					pSpot->pev->avelocity.x = 0;
				}
		}
		if(pev->frags == 125){
				pPlayer->Clear_SayText();
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "holysword_get" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
		}
		if(pev->frags == 170){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_Remove( pEntity );
					}

					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					pPlayer->pev->origin = pev->origin + Vector(0,0,36);
					pPlayer->m_stuck_origin = pev->origin + Vector(0,0,36);
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
					pPlayer->m_flVelocityModifier = -2;
					pPlayer->m_trainning = 0;
					pPlayer->EnableControl(TRUE);

					pPlayer->GiveNamedItem( "weapon_valvesword" );
		}
		if(pev->frags == 210){
			
				sprintf( text, "Misaliya: You got it?\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 220){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_enemyfollower = 1;
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				}
		}
		if(pev->frags == 270){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					if ( pEntity->pev->deadflag == DEAD_NO ){
					pEntity->pev->skin = 5;//��
					
					sprintf( text, "Misaliya: You must be real brave!\n");
					
					UTIL_SayTextAll( text,this );
					}
				}
		}
		if(pev->frags == 340){
			
			sprintf( text, "Misaliya: Things'll be easy!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 400){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					if ( pEntity->pev->deadflag == DEAD_NO ){
					pEntity->pev->skin = 0;
					}
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 420){
			
			sprintf( text, "- You can charge up for a charged attack.\n");
			
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 500){
				pPlayer->m_game_rate = 65;//��Ϸ����65%
				pPlayer->Clear_SayText();
				UTIL_Remove( this );
				return;
		}
	}
	else if(pev->armortype == 45){//�ִ�����֮ɭ
		if(pev->frags == 0){
				pPlayer->m_flVelocityModifier = -4;
				pPlayer->Clear_SayText();
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
				pPlayer->pev->origin = pev->origin + Vector(0,15,20);
				pPlayer->m_stuck_origin = pev->origin + Vector(0,15,20);
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;
		}
		if(pev->frags == 4){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();

				if ( pEnemyMonster->m_MonsterState == MONSTERSTATE_SCRIPT 
				|| pEnemyMonster->m_IdealMonsterState == MONSTERSTATE_SCRIPT )
				{//Bug Fix 2.0 Misaliya�ű�ִ���ӳ� 
				pev->frags -= 1;
				pev->nextthink = gpGlobals->time + 1.0;
				return;
				}

				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEntity->pev->angles.y = 90;
				pEnemyMonster->pev->velocity = g_vecZero;
				pEnemyMonster->RouteClear();
				pEnemyMonster->m_lovehate += 10;
				pEnemyMonster->m_hEnemy = NULL;
				pEnemyMonster->m_hOldEnemy[0] = NULL;
				pEnemyMonster->m_hOldEnemy[1] = NULL;
				pEnemyMonster->m_hOldEnemy[2] = NULL;
				pEnemyMonster->m_hOldEnemy[3] = NULL;
				pEnemyMonster->m_playerguardian_mode = 0;
				pEnemyMonster->pev->flags &= ~FL_NOTARGET;
				pEnemyMonster->m_alert = 100;
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEntity->pev->movetype = MOVETYPE_STEP;
				pEntity->pev->owner = NULL;
				UTIL_SetOrigin( pEntity->pev, pev->origin - Vector(0,48,12) );
				SetBits( pEntity->pev->effects, EF_DIMLIGHT);//��������ģʽ
				}
		}
		if(pev->frags == 10){
			pPlayer->m_music_save = 3;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 11\n");
			//SERVER_COMMAND("mp3 loop media/music14.mp3\n");
		}
		if(pev->frags == 20){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 30){
			FireTargets( "bloodfollower_maker", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 35){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				if ( pEntity->pev->deadflag == DEAD_NO ){
					
					sprintf( text, "Misaliya: This is Wiseter/Sage Forest. A wise sage lives here. \n");
					
				UTIL_SayTextAll( text,this );
				}
			}
		}
		if(pev->frags == 75){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				if ( pEntity->pev->deadflag == DEAD_NO ){
					
					sprintf( text, "Misaliya: He guards the Seal of the Holy Tree.\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
		}
		if(pev->frags == 125){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				if ( pEntity->pev->deadflag == DEAD_NO ){
					
					sprintf( text, "Misaliya: The Seal was weakened recently. Many monsters appeared.\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
		}
		if(pev->frags == 175){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				if ( pEntity->pev->deadflag == DEAD_NO ){
				
					sprintf( text, "Misaliya: I don't know why it happened, but we should find the sage.\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
		}
		if(pev->frags == 245){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 270){
			pPlayer->m_game_rate = 66;//��Ϸ����66%
			UTIL_Remove( this );
			return;
		}
	}
	else if(pev->armortype == 46){//Ұ���ͱ�
		if(pev->frags == 0){
				pPlayer->EnableControl(FALSE);
				pPlayer->Clear_SayText();
				pPlayer->m_trainning = 1;
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
				pPlayer->pev->origin = pev->origin + Vector(-256,0,0);
				pPlayer->m_stuck_origin = pev->origin + Vector(-256,0,0);
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 2.0, 255, FFADE_IN );
		}
		if(pev->frags == 2){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_enemyfollower = 0;
				pEntity->pev->angles.y = 90;
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
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEntity->pev->movetype = MOVETYPE_STEP;
				pEntity->pev->owner = NULL;
				pEntity->pev->solid = SOLID_NOT;
				ClearBits(pEnemyMonster->pev->effects, EF_DIMLIGHT);
				UTIL_SetOrigin( pEntity->pev, pev->origin - Vector(0,32,0) );
				}
		}
		if(pev->frags == 4){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					UTIL_Remove( pEntity );
				}

				pEntity = Create( "monster_kadoma", pev->origin + Vector(0,32,0), Vector(0,0,0), NULL );
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 2, 7 );
		}
		if(pev->frags == 6){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
						pSpot->pev->velocity.x += 40;
					}
		}
		if(pev->frags == 10){//�޵Ф�Ұ���ͱ�
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				pEntity->pev->takedamage = DAMAGE_NO;
				}
		}
		if(pev->frags == 15){
				FireTargets( "yj_misaliya_move1", this, this, USE_TOGGLE, 0 );
				FireTargets( "yj_kadoma_move1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 70){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.y = 800;
							pSpot->pev->velocity.z = 40;
				}
		}
		if(pev->frags == 94){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.y = 0;
							pSpot->pev->velocity.z = 0;
				}
		}
		if(pev->frags == 100){
				sprintf( text, "Misaliya: !?\n");
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 150){
				FireTargets( "yj_misaliya_move2", this, this, USE_TOGGLE, 0 );

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					UTIL_Remove( pEntity );

						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->m_trainning = 0;
						pPlayer->EnableControl(TRUE);
						pPlayer->Clear_SayText();

						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->fixangle = TRUE;

						pPlayer->pev->origin = pEntity->pev->origin + Vector(0,0,36);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;

				}
		}
		if(pev->frags == 200){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_enemyfollower = 1;
				pEnemyMonster->m_igonre_npc = 40;
				pEntity->pev->solid = SOLID_SLIDEBOX;
				}
		}
		if(pev->frags == 210){
				SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 225){
				FireTargets( "yjmajo_maker1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 230){
					pPlayer->Clear_SayText();
		}
		if(pev->frags == 275){
				FireTargets( "yjmajo_maker2", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 280){
			pPlayer->m_game_rate = 67;//��Ϸ����67%
			UTIL_Remove( this );
			return;
		}
	}
	else if(pev->armortype == 47){//Ұ���ͱ���м
		if(pev->frags == 5){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 8){
			pev->frags = 9;
			pev->nextthink = gpGlobals->time + 1.0;
			return;//�ȴ�!
		}
		if(pev->frags == 10){
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->m_trainning = 1;

						pPlayer->pev->angles = Vector(0,270,0);
						pPlayer->pev->v_angle = Vector(0,270,0);
						pPlayer->pev->fixangle = TRUE;

						pPlayer->pev->origin = pev->origin + Vector(0,-512,0);
						pPlayer->m_stuck_origin = pev->origin + Vector(0,-512,0);

						pPlayer->m_wdoor_mynpc = NULL;
						pPlayer->m_guard_mynpc = 0;

						//pPlayer->m_music_save = 0;
						//SERVER_COMMAND("mp3 stop\n");
						//Bug Fix 3.0 ��������BGMЧ������ã�

						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.0, 1.0, 255, FFADE_IN );
		}
		if(pev->frags == 12){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_IDLE );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->m_enemyfollower = 0;
				pEntity->pev->angles.y = 110;
				pEntity->pev->health = pEntity->pev->max_health;
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
				pEnemyMonster->pev->effects &= ~EF_NODRAW;
				pEntity->pev->movetype = MOVETYPE_STEP;
				pEntity->pev->owner = NULL;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(100,-80,0) );
				}
		}
		if(pev->frags == 14){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					UTIL_Remove( pEntity );
				}

				pEntity = Create( "monster_kadoma", pev->origin + Vector(40,-80,0), Vector(0,70,0), NULL );
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 2, 7 );
		}
		if(pev->frags == 16){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
						SET_VIEW( pPlayer->edict(), pSpot->edict() );
						pPlayer->m_player_camera = pSpot;
						pPlayer->pev->angles = pSpot->pev->angles;
						pSpot->pev->origin = pev->origin + Vector(72,-160,40);
						pSpot->pev->velocity.z = -6;
					}
		}
		if(pev->frags == 18){//����һ��һ��һ��һ��������������ʵ�壡
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
								if(pEntity->pev->origin.z < pev->origin.z){//����²����Լ����﹤����ʵ��
									if (  (pEntity->pev->flags & FL_MONSTER) ){
										if (  !FClassnameIs ( pEntity->pev, "monster_misaliya")
										&& !FClassnameIs ( pEntity->pev, "monster_wisebeast")
										&& !FClassnameIs ( pEntity->pev, "monster_kadoma")){
										UTIL_Remove( pEntity );
										}
									}
									if (  FClassnameIs ( pEntity->pev, "monstermaker") ){
									UTIL_Remove( pEntity );
									}
								}
						}
		}
		if(pev->frags == 40){
				FireTargets( "yj_shizi_feng", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 41){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				pEntity->pev->velocity.z = 1;
				pEntity->pev->frags = 0;
				}
		}
		if(pev->frags == 45){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_IDLE );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 66){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->velocity.z = 0;
				}
		}
		if(pev->frags == 76){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.y += 110;
				}
				
				sprintf( text, "Wisebeast: I'm saved.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 120){
				
				sprintf( text, "Wisebeast: Thank you, black haired boy and white haired girl.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 175){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.y = 270;
					pSpot->pev->origin.y += 10;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 180){
			
			sprintf( text, "Misaliya: My name is Misaliya, he is Kadoma\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 230){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x += 30;
					pSpot->pev->origin.z -= 5;
					pSpot->pev->origin.y -= 10;
					pSpot->pev->angles.y = 270;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 235){
			
				sprintf( text, "Misaliya: Aren't you a great sage? What happened?\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 280){
			
				sprintf( text, "Misaliya: A sage wouldn't let himself get captured so easily.\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 345){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x -= 30;
					pSpot->pev->origin.z += 5;
					pSpot->pev->origin.y += 10;
					pSpot->pev->angles.y = 90;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 350){
			
				sprintf( text, "Wisebeast: The witches poisoned me; I can't use magic.\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 390){
			
				sprintf( text, "Wisebeast: I must go into sage state to recover...\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 450){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEntity->pev->skin = 6;
				}
		}
		if(pev->frags == 455){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x += 30;
					pSpot->pev->origin.z -= 5;
					pSpot->pev->origin.y -= 10;
					pSpot->pev->angles.y = 270;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 457){
				sprintf( text, "Misaliya: ???\n");
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 500){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x -= 30;
					pSpot->pev->origin.z += 5;
					pSpot->pev->origin.y += 10;
					pSpot->pev->angles.y = 90;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 505){
			
				sprintf( text, "Wisebeast: I must connect to restore my magic.\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 545){
		
				sprintf( text, "Wisebeast: Help me, Misaliya!\n");
			
			
				UTIL_SayTextAll( text,this );

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "slmas" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
		}
		if(pev->frags == 599){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEntity->pev->skin = 2;
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dame" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
		}
		if(pev->frags == 600){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x += 30;
					pSpot->pev->origin.z -= 5;
					pSpot->pev->origin.y -= 10;
					pSpot->pev->angles.y = 270;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 602){
			
				sprintf( text, "Misaliya: No way!\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 635){
			
				sprintf( text, "Misaliya: Fix it yourself!\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 675){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
		}
		if(pev->frags == 680){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x -= 30;
					pSpot->pev->origin.z += 5;
					pSpot->pev->origin.y += 10;
					pSpot->pev->angles.y = 90;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 682){
			
				sprintf( text, "Wisebeast: I can't. I must connect with another!\n");
			
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 742){
			
			sprintf( text, "Wisebeast: If I don't recover, the world will end!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 805){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEntity->pev->skin = 1;
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_GUARD );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
		}
		if(pev->frags == 808){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x += 30;
					pSpot->pev->origin.z -= 10;
					pSpot->pev->origin.y -= 10;
					pSpot->pev->angles.y = 270;
				}
				pPlayer->Clear_SayText();
		}
		if(pev->frags == 810){
				sprintf( text, "Misaliya: ......\n");
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 855){
				SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 860){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin.x -= 60;
					pSpot->pev->origin.z += 5;
					pSpot->pev->angles.y = 270;
				}
					pPlayer->Clear_SayText();
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					SERVER_COMMAND( "=cammousemove\n");//��ȫ��������
		}
		if(pev->frags == 862){
			
				sprintf( text, "Wisebeast: Is this okay, Kadoma?\n");
		
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 890){
				pPlayer->m_fSelectMode = TRUE;
				pPlayer->m_fSelectNumber = 0;
				pPlayer->m_load_check = 1;
				pPlayer->ShowVGUIMenu(35);
		}
		if(pev->frags == 897){
				if(pPlayer->m_fSelectNumber == 0){
					if(pPlayer->m_load_check == 0){//Bug Fix 3.0 ��Ҵ������BUG��
					pPlayer->ShowVGUIMenu(35);
					pPlayer->m_load_check = 1;
					}
					pev->frags = 892;
					pev->team = -1;
				}
		}
		if(pev->frags == 900){
					pev->team = pPlayer->m_fSelectNumber;
					pPlayer->m_fSelectNumber = 0;
		}
		if(pev->team == 1){//kadoma �ν�ɱ���ͱ�
			if(pev->frags == 905){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_aim" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
				pPlayer->Clear_SayText();
			}
			else if(pev->frags == 925){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 90;
					pSpot->pev->velocity.x += 20;
				}
			}
			else if(pev->frags == 945){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 0;
					pSpot->pev->velocity.y += 3;
					pSpot->pev->velocity.x = 0;
				}
			}
			else if(pev->frags == 960){
				
				sprintf( text, "Wisebeast: What did you say?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 990){
				
				sprintf( text, "Wisebeast: Are you nuts?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1000){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->armortype = 5;
				}
			}
			else if(pev->frags == 1020){
				
				sprintf( text, "Wisebeast: Don't do this!\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1040){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin - Vector(40,40,0) );
				pEntity->pev->angles.y = 80;
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(80,-70,-54) );
				pEntity->pev->angles.y = 90;
				}
			}
			else if(pev->frags == 1060){
					pPlayer->Clear_SayText();
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->velocity.y += 48;
						pSpot->pev->origin = pev->origin + Vector(160,-64,0);
						pSpot->pev->angles.y = 180;
					}
			}
			else if(pev->frags == 1065){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_swing" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			else if(pev->frags == 1090){
					pPlayer->m_ending_frags -= 5;//̫�����ˣ���Ʒֵ-5%
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
			}
			else if(pev->frags == 1110){
				
				sprintf( text, "Misaliya: Kadoma, I appreciate your concern, but...\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1150){
				
				sprintf( text, "Misaliya: That was a bit much.\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1190){
				
				sprintf( text, "Misaliya: Okay?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1250){
				pPlayer->Clear_SayText();
			}
			else if(pev->frags == 1260){
				FireTargets( "intodvtre_event", this, this, USE_TOGGLE, 0 );
			}
		}
		else if(pev->team == 2){//kadoma �ν���ֹ�ͱ�
			if(pev->frags == 910){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					pEntity->pev->origin.x = pev->origin.x + 85;
					pEntity->pev->origin.y = pev->origin.y - 130;
					UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin);
					pEntity->pev->angles.y = 90;
				}

				pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_aim2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );

				UTIL_SetOrigin( pEntity->pev, pev->origin - Vector(-100,100,54) );
				pEntity->pev->angles.y = 90;
				}
				pPlayer->Clear_SayText();
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(90,-60,0);
					pSpot->pev->angles.y = 270;
					pSpot->pev->velocity.z += 1;
				}
			}
			else if(pev->frags == 930){
				sprintf( text, "Misaliya: Kadoma...!\n");
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 960){
				pPlayer->Clear_SayText();
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = -90;
					pSpot->pev->velocity.z = 0;
				}
			}
			else if(pev->frags == 980){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 0;
				}
			}
			else if(pev->frags == 990){
				
				sprintf( text, "Wisebeast: What other way is there?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1030){
				
				sprintf( text, "Wisebeast: If I don't recover...\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1060){
				FireTargets( "yjmajo_corpse_maker", this, this, USE_TOGGLE, 0 );//ħŮ��ʬ
			}
			else if(pev->frags == 1070){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.y = 270;
					pSpot->pev->velocity.x = 20;
				}
				pPlayer->Clear_SayText();
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sword_aim3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
			}
			else if(pev->frags == 1100){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 90;
					pSpot->pev->velocity.x = 35;
					pSpot->pev->velocity.y = -7;
					pSpot->pev->avelocity.x = 7;
				}
			}
			else if(pev->frags == 1110){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 0;
				}
			}
			else if(pev->frags == 1140){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->armortype = 6;
				}
			}
			else if(pev->frags == 1150){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				pEntity->pev->angles.y = 330;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin - Vector(-36,36,0));
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				pEntity->pev->angles.y = 0;
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEntity->pev->angles.y = 0;
				}
			}
			else if(pev->frags == 1160){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->angles.x = 0;
						pSpot->pev->angles.y = 150;
						pSpot->pev->origin = pEntity->pev->origin + Vector(110,-110,60);
					}
				}
			}
			else if(pev->frags == 1170){
				
				sprintf( text, "Wisebeast: You mean her?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1200){
				
				sprintf( text, "Wisebeast: I guess that can work...\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1240){
					pPlayer->Clear_SayText();
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
			}
			else if(pev->frags == 1270){
				
				sprintf( text, "Misaliya: Thanks for the help, Kadoma.\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1310){
					pPlayer->Clear_SayText();
			}
			else if(pev->frags == 1320){
				FireTargets( "intodvtre_event", this, this, USE_TOGGLE, 0 );
			}
		}
		else if(pev->team == 3){//kadoma ʲôҲ����
			if(pev->frags == 910){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 1, 3 );
				}

				pPlayer->Clear_SayText();
			}
			if(pev->frags == 920){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
					pEntity->pev->angles.y = 230;
					UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin - Vector(-114,110,0));

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity2 ){
					pEntity2->pev->skin = 4;
					pEntity2->pev->angles.y = 40;
					UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin - Vector(16,25,0));
					}
				}
			}
			if(pev->frags == 930){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "hand_put" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity3 ){
				pEntity3->pev->angles.y = 0;
				UTIL_SetOrigin( pEntity3->pev, pEntity3->pev->origin - Vector(72,24,0));
				}
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 180;
					pSpot->pev->velocity.x = -3;
					pSpot->pev->velocity.z = -1;
					pSpot->pev->origin = pev->origin + Vector(256,-128,16);
				}
			}
			else if(pev->frags == 935){
				
				sprintf( text, "Wisebeast: Come!\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 990){
				
				sprintf( text, "Misaliya: Get away from me!\n");
				
				UTIL_SayTextAll( text,this );

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 180;
					pSpot->pev->velocity.x = 0;
					pSpot->pev->velocity.z = 0;
					pSpot->pev->origin = pev->origin + Vector(64,-96,0);
				}
			}
			else if(pev->frags == 1000){
				
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
				CBaseEntity *ShockBall = Create( "shock_ball", pSpot->pev->origin + Vector(8,0,72), Vector(0,0,0), NULL );
				ShockBall->pev->velocity.z = -600;
				}
				pPlayer->pev->angles = Vector(0,180,0);
				pPlayer->pev->v_angle = Vector(0,180,0);
				pPlayer->pev->fixangle = TRUE;
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "die_backwards2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->solid = SOLID_NOT;
				pEntity->pev->flags &= ~FL_ONGROUND;
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = -150;
				pEntity->pev->velocity.y = 30;
				pEntity->pev->velocity.z = 0;
				pEntity->pev->angles.y = 330;
				}
			}
			else if(pev->frags == 1015){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity ){
				pEntity->pev->velocity.x = 0;
				pEntity->pev->velocity.y = 0;
				pEntity->pev->velocity.z = 0;
				pEntity->pev->solid = SOLID_SLIDEBOX;
				}
			}
			else if(pev->frags == 1020){
				pPlayer->Clear_SayText();
			}
			else if(pev->frags == 1030){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 90;
				}
			}
			else if(pev->frags == 1040){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
				pEntity->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(-20,20,0));
				}
			}
			else if(pev->frags == 1050){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 0;
				}
			}
			else if(pev->frags == 1060){
				
				sprintf( text, "Misaliya: Kadoma, why did you do nothing?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1090){
				
				sprintf( text, "Misaliya: Do you not care about me?\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1140){
				
				sprintf( text, "Misaliya: ...Actually, never mind.\n");
				
				UTIL_SayTextAll( text,this );
			}
			else if(pev->frags == 1180){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					pEntity->pev->skin = 0;
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_IDLE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
				}	
				pPlayer->Clear_SayText();
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->velocity.x = -2;
				}
			}
			else if(pev->frags == 1200){
				pPlayer->m_ending_frags -= 3;//Į�Ӳ�������Ʒֵ-3%
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
			}
			else if(pev->frags == 1240){
				FireTargets( "intodvtre_event", this, this, USE_TOGGLE, 0 );
			}
		}
		else if(pev->team == 4){//kadoma Ҳ��Ҫ����
			if(pev->frags == 910){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				}

				pPlayer->Clear_SayText();
			}
			if(pev->frags == 920){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity2 ){
				pEntity2->pev->skin = 4;
				}
			}
			if(pev->frags == 940){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 90;
					pSpot->pev->origin = pev->origin + Vector(80,-64,8);
				}
			}
			if(pev->frags == 945){
				
				sprintf( text, "Wisebeast: You want to join in? Go right ahead!\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 975){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->velocity.x = 100;
				}
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 995){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->velocity.x = 0;
				}
			}
			if(pev->frags == 997){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				
				if ( pEntity ){
				org_floor = pev->origin + Vector(75,-90,0);
				org_floor.z = pEntity->pev->origin.z;
				pEntity->pev->angles.y = 35;
				UTIL_SetOrigin( pEntity->pev, org_floor);
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fear" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 1, 3 );
				pEnemyMonster->SetBodygroup( 2, 0 );
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity2 ){
				org_floor = pev->origin + Vector(70,-48,0);
				org_floor.z = pEntity2->pev->origin.z;
				pEntity2->pev->angles.y = 330;
				UTIL_SetOrigin( pEntity2->pev, org_floor);
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "hand_put" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity3 ){
				org_floor = pev->origin + Vector(100,-64,0);
				org_floor.z = pEntity3->pev->origin.z;
				pEntity3->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity3->pev, org_floor);
				}
			}
			if(pev->frags == 1000){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 90;
				}
			}
			if(pev->frags == 1010){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.y = 0;
					pSpot->pev->velocity.x = -25;
				}
			}
			if(pev->frags == 1030){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->armortype = 5;
				}
			}
			if(pev->frags == 1060){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->velocity.x = 0;
					pSpot->pev->angles.y = 0;
					pSpot->pev->angles.x = 10;
					pSpot->pev->origin.x -= 110;
				}
				
				sprintf( text, "Misaliya: You too?\n");
				
				UTIL_SayTextAll( text,this );
				SERVER_COMMAND("mp3 stop\n");
			}
			if(pev->frags == 1100){
				pPlayer->pev->angles = Vector(0,0,0);
				pPlayer->pev->v_angle = Vector(0,0,0);
				pPlayer->pev->fixangle = TRUE;
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1102){
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity3 ){
				pEntity3->pev->skin = 8;
				FX_Explosion( pEntity3->Center(), EXPLOSION_HEVCHARGER);
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast" );
				if ( pEntity2 ){
				UTIL_SetOrigin( pEntity2->pev, pev->origin - Vector(20,0,0));
				}
				
				sprintf( text, "Misaliya: Then die!!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1122){//���֮��
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );
				pPlayer->m_player_camera = NULL;
				pPlayer->m_deadtakedmgkill = DMG_ENERGYBLAST;//Bug Fix 3.0 ��ɱ��ʾ�����ϸ��
				pPlayer->Killed( pev, GIB_ALWAYS );
			}
		}
	}
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
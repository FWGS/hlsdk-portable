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
// ��CG�¼���ͳ��5
//=========================================================
class CMain_Event5 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
	char text[256];
};

LINK_ENTITY_TO_CLASS( main_cg_event_new5, CMain_Event5 );//����5

void CMain_Event5::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event5::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��5=================================================//
void CMain_Event5::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}
	if(pev->armortype == 61){//�¼�61 �ٻ�����������
			if(pev->frags == 0){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;
			}
			if(pev->frags == 5){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
				}
			}
			if(pev->frags == 10){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(384,-32,-16) );
				}
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity2->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(384,32,-16) );
				}
				CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
				if ( pEntity3 ){
				pEnemyMonster = pEntity3->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity3->pev->angles.y = 180;
				pEntity3->pev->effects = 0;
				UTIL_SetOrigin( pEntity3->pev, pev->origin + Vector(256,0,-16) );
				}
			}
			if(pev->frags == 40){
					
					sprintf( text, "Vanlve: Thanks for the help.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 70){
					
					sprintf( text, "Vanlve: You fought well. Quite well.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 120){
				
					sprintf( text, "Vanlve: But it's not enough!\n");
			
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 190){
					pPlayer->Clear_SayText();
					CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity3 ){
					pEntity3->pev->angles.y = 0;
					}
			}
			if(pev->frags == 220){
					FireTargets( "teleport_anotherworld_yoja", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 260){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(128,-64,48);
				}
			}
			if(pev->frags == 290){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(128,64,64);
				}
			}
			if(pev->frags == 320){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(16,-64,72);
				}
			}
			if(pev->frags == 350){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(16,64,96);
				}
			}
			if(pev->frags == 380){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(450,0,60);
				}
				
				sprintf( text, "Vanlve: These heroes are from another world, chosen by God!\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 420){
				
				sprintf( text, "Vanlve: We need more fighting power\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 470){
				
				sprintf( text, "Vanlve: To kill Gman and Doma, you must work together.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 540){
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
					if ( pSpot ){
						pSpot->pev->origin = pSpot->pev->origin + Vector(-128,96,-16);
						pSpot->pev->angles.y = 300;
					}
			}
			if(pev->frags == 550){
				
					sprintf( text, "Misaliya: (They look weird)\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 600){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 610){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(320,-96,48);
						pSpot->pev->angles.y = 120;
					}
			}
			if(pev->frags == 620){
				
				sprintf( text, "Vanlve: Kadoma, acquaint yourself with your new pals, and then come find me.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 670){
				
				sprintf( text, "- Only 4 fighters can be fielded, but they can be replaced by those in reserve.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 720){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity2 ){
					pEntity2->pev->frame = 0;
					pEntity2->pev->framerate = 0;
					}
			}
			if(pev->frags == 740){
					pPlayer->Clear_SayText();
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(360,32,52);
						pSpot->pev->angles.y = 0;
					}
			}
			if(pev->frags == 760){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "agree" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 790){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 20, 255, FFADE_OUT );
					FireTargets( "door_stuck_wall", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 820){
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 830 || pev->frags == 835){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){//kadoma remove
					UTIL_Remove( pEntity );
					}
			}
			if(pev->frags == 840){
					pPlayer->Clear_SayText();
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );
					pPlayer->pev->origin = pev->origin + Vector(1330,384,420);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					CBaseMonster *pEnemyMonster;
					
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
					if ( pEntity ){//Misaliya
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_remove(pEnemyMonster);
					pEnemyMonster->m_enemyfollower = 0;
					pEnemyMonster->m_enemyfollower_combat = 0;
					pEntity->pev->angles.y = 270;
					pEnemyMonster->m_rpgms_inteam = 5;

					pEntity = Create( "monster_lelite", pEntity->pev->origin + Vector(256,0,32), pev->angles, NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_rpgms_inteam = 5;
					//Lelite��ͻ�γ���
					}
					CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity3 ){
					pEnemyMonster = pEntity3->MyMonsterPointer();
					pEntity3->pev->angles.y = 270;
					pEntity3->pev->takedamage = DAMAGE_NO;
					UTIL_SetOrigin( pEntity3->pev, pev->origin + Vector(1800,4600,370) );
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_hime" );
					if ( pEntity ){//Hime
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_rpgms_inteam = 5;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->angles.y = 270;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(1440,4096,370) );
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_nobita" );
					if ( pEntity ){//Nobita
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_rpgms_inteam = 5;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->angles.y = 270;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(1600,4096,370) );
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_dragon" );
					if ( pEntity ){//Dragon
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_rpgms_inteam = 5;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->angles.y = 270;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(1340,4192,370) );
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_willam" );
					if ( pEntity ){//Willam���
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_rpgms_inteam = 5;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->angles.y = 270;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(1700,4192,370) );
					}
			}
			if(pev->frags == 890){
					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;
					pPlayer->m_flVelocityModifier = -4;
					FX_Explosion( pev->origin, 254 );//��������Ѫ������!
			}
			if(pev->frags == 900){
					pPlayer->MenuItem_add(15);//����Get
					pPlayer->m_fequip1 = TRUE;
					if(pPlayer->pev->max_health < 400){
					pPlayer->pev->max_health = 400;
					}
					pPlayer->pev->health = pPlayer->pev->max_health;
					pPlayer->MenuItem_add(16);//����ѥGet
					pPlayer->m_fequip2 = TRUE;
					//pPlayer->MenuItem_add(17);//�����ñGet
					//pPlayer->m_fequip3 = TRUE;
					g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "mario", "1" );
					pPlayer->MenuItem_add(18);//����Get
					pPlayer->m_fequip4 = TRUE;
					pPlayer->m_air_oxyan_max = 2500;
					pPlayer->m_air_oxyan = pPlayer->m_air_oxyan_max;
					pPlayer->MenuItem_add(19);//��֮����Get
					pPlayer->m_fequip5 = TRUE;
					pPlayer->m_skill_darkhide = 7;
					pPlayer->MenuItem_add(22);//������Get
			}
			if(pev->frags == 910){
					pPlayer->GiveNamedItem( "weapon_dueluzi" );
					pPlayer->GiveNamedItem( "weapon_dualdbarrel" );
					pPlayer->GiveNamedItem( "weapon_darkgrenade" );
					pPlayer->GiveNamedItem( "weapon_medkit" );
			}
			if(pev->frags == 920){
					pPlayer->pev->armorvalue = 0;
					pPlayer->m_skill_maxarmor = 0;
					pPlayer->m_iClientBattery = -1;
					pPlayer->GiveNamedItem( "item_armor3" );
					pPlayer->GiveNamedItem( "item_respawn" );
					pPlayer->GiveNamedItem( "item_godwater" );
					pPlayer->MenuItem_add(12);
			}
			if(pev->frags == 930){
					pPlayer->GiveAmmo( 5, "kmedkit", MEDKIT_MAX_CARRY );
					pPlayer->GiveAmmo( 42, "357", _357_MAX_CARRY );
					pPlayer->GiveAmmo( 180, "762nato", AK_MAX_CARRY );
					pPlayer->GiveAmmo( 9, "rockets", ROCKET_MAX_CARRY );
					pPlayer->GiveAmmo( 30, "338mag", SNIPER_MAX_CARRY );
					pPlayer->GiveAmmo( 120,"buckshot", BUCKSHOT_MAX_CARRY );
					pPlayer->GiveAmmo( 300,"45acp", MAC_MAX_CARRY );
					pPlayer->GiveAmmo( 3,"Dark Grenade", DARKGRENADE_MAX_CARRY);
			}
			if(pev->frags == 960){
			
					sprintf( text, "- Kadoma gained many weapons and items!\n");
					UTIL_SayTextAll( text,this );
					sprintf( text, "- Open the status menu to view new weapons and skills\n");
					UTIL_SayTextAll( text,this );
					sprintf( text, "- New Skill: Cloak of Invisibility\n");
					UTIL_SayTextAll( text,this );
					sprintf( text, "- Press G to become invisible!\n");
					UTIL_SayTextAll( text,this );
					
					pPlayer->m_music_save = 11;
					SERVER_COMMAND("mp3 loop media/music19.mp3\n");
			}
			if(pev->frags == 1080){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 1100){
					if(pPlayer->pev->origin.y >= 3000 && pPlayer->pev->origin.x >= -2800){
						pPlayer->EnableControl(FALSE);
						pPlayer->m_trainning = 1;
						FireTargets( "get_out_shin_event", this, this, USE_TOGGLE, 0 );
						UTIL_Remove( this );
						return;
					}
					else{
						pev->frags = 1090;
					}
			}
	}
	if(pev->armortype == 62){//�¼�62 �뿪����ĺ�̤����;
			if(pev->frags == 0){
				pev->origin.z += 64;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
				SET_VIEW( pPlayer->edict(), edict() );
				pPlayer->m_player_camera = this;
				pPlayer->pev->origin = pev->origin - Vector(0,128,24);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->pev->angles = pev->angles;
			}
			if(pev->frags == 5){
				//ħ�ģ�Misaliya���Ϸ��ɾ������ӣ���Ϊ�油��Ա!
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
					UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(-100,-100,0) );
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_hime" );
					if ( pEntity ){
					pEntity->pev->angles.y = 90;
					pEntity->pev->yaw_speed = 0;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-96,256,-64));
					pEntity->pev->spawnflags = 0;
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_nobita" );
					if ( pEntity ){
					pEntity->pev->angles.y = 90;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-192,256,-64));
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_dragon" );
					if ( pEntity ){
					pEntity->pev->angles.y = 90;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(96,256,-64));
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_willam" );
					if ( pEntity ){
					pEntity->pev->angles.y = 90;
					UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(192,256,-64));
					}
					pEntity = Create( "monster_kadoma2", pev->origin + Vector(0,256,-64), Vector(0,90,0), NULL );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle1" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->SetBodygroup( 0, 3 );
						pEnemyMonster->SetBodygroup( 2, 7 );
						pEntity->pev->flags |= FL_NOTARGET;
					}
					pev->velocity.y = 10;
			}
			if(pev->frags == 45){
				pev->velocity.y = 0;
				
				sprintf( text, "Vanlve: Gman and Doma must die to prevent more attacks.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 95){
				
				sprintf( text, "Vanlve: Left unchecked, I fear the world will end.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 145){
				
				sprintf( text, "Vanlve: Take this bomb, plant it in the Gman Factory, and blow it to end Gman's evil plan.\n");
				
				UTIL_SayTextAll( text,this );

				pev->origin = pev->origin + Vector(100,300,0);
				pev->angles.x += 10;

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
				if ( pEntity ){
				CBaseEntity::Create( "item_nuke", pEntity->pev->origin + Vector(-32,-32,32), Vector(0,270,0), edict() );
				}
			}
			if(pev->frags == 205){
				
				sprintf( text, "Vanlve: Use the Teleport Book to escape before it blows.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 255){
				pev->origin = pev->origin + Vector(-100,-300,0);
				pev->angles.x -= 10;
				
				sprintf( text, "Vanlve: Use the next battles to hone your combat skills and strategies.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 305){
				
				sprintf( text, "Vanlve: Learn your skills, weapons, partner synergies, and so on.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 385){
				pPlayer->Clear_SayText();
				FireTargets( "get_out_gate_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 445){
				
				sprintf( text, "Vanlve: I'm counting on you, Kadoma.\n");
				
				UTIL_SayTextAll( text,this );

				pev->origin = pev->origin + Vector(0,450,0);
				pev->angles.y = 270;
				pev->velocity.y = -20;
			}
			if(pev->frags == 495){
				pPlayer->Clear_SayText();
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 6, 255, FFADE_OUT );
			}
			if(pev->frags == 505){//ħ�ģ�4+2��7�˶��飬Misaliya��������! Lelite��ͻ����!
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_willam" );
				CBaseMonster *pEnemyMonster;
				if ( pEntity ){//Willam���
				pEnemyMonster = pEntity->MyMonsterPointer();
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->Hunt_Stand_Set(2);//ս��
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_nobita" );
				if ( pEntity ){//Nobita���
				pEnemyMonster = pEntity->MyMonsterPointer();
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->Hunt_Stand_Set(2);//ս��
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_hime" );
				if ( pEntity ){//Hime���
				pEnemyMonster = pEntity->MyMonsterPointer();
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->Hunt_Stand_Set(2);//ս��
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_dragon" );
				if ( pEntity ){//Dragon���
				pEnemyMonster = pEntity->MyMonsterPointer();
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->Hunt_Stand_Set(2);//ս��
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){//Misaliya���
				pEnemyMonster = pEntity->MyMonsterPointer();
				pPlayer->TeamMate_add(pEnemyMonster);
				pEntity->pev->frags = 2;
				pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
				if ( pEntity ){//Lelite���
				pEnemyMonster = pEntity->MyMonsterPointer();
				pPlayer->TeamMate_add(pEnemyMonster);
				pEnemyMonster->m_rpgms_level = 33;
				pEnemyMonster->m_follow_mode = 2;
				pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
				}
			}
			if(pev->frags == 515){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 535){
					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;
					pPlayer->pev->velocity = g_vecZero;
					pPlayer->pev->origin = pev->origin + Vector(0,512,256);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->m_music_save = 0;
					pPlayer->MenuItem_add(23);//С�ͺ˵�Get
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );

					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if(FClassnameIs ( pEntity->pev, "trigger_teleport" )){
							if(pEntity->pev->frags == 8 && FStrEq(STRING(pEntity->pev->target), "player_teleport_chang")){
								pEntity->pev->frags = 13;//ȫԱ���͹�ͼ!
								break;
							}
						}
					}

					UTIL_Remove( this );
					return;
			}
	}
	if(pev->armortype == 63){//�¼�63 һת���ƣ�ǰ������������RPGʽ������ս��!
		if(pev->frags == 0){
			pPlayer->pev->origin = pev->origin + Vector(0,1360,0);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,270,0);
			pPlayer->pev->angles = Vector(0,270,0);
			pPlayer->pev->fixangle = TRUE;
			pPlayer->m_game_rate = 77;//��Ϸ����77%
		}
		if(pev->frags == 40){
				CBaseEntity *pEntity = NULL;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
				{
					if(FClassnameIs ( pEntity->pev, "trigger_teleport" )){
						if(pEntity->pev->frags == 8 && FStrEq(STRING(pEntity->pev->target), "player_teleport_chang")){
							pEntity->pev->frags = 13;//ȫԱ���͹�ͼ!
							break;
						}
					}
				}
		}
		if(pev->frags == 80){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 100){
			FireTargets( "start_door", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 130){
			pPlayer->m_music_save = 13;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 19\n");
			//SERVER_COMMAND("mp3 loop media/music21.mp3\n");
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 64){//�¼�64 ����ʯ��
		if(pev->frags == 0){
				pPlayer->pev->origin = pev->origin - Vector(256,512,0);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;
		}
		if(pev->frags == 2){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(64,0,0), Vector(0,0,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );
				pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 6){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
			pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(0,192,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 0;
			}
			if (pPlayer->m_team_npc2 != NULL){
			pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(0,64,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 0;
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(0,-64,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 0;
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(0,-192,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 0;
			}
		}
		if(pev->frags == 10){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
					pSpot->pev->angles.x = 10;
				}
		}
		if(pev->frags == 70){
			pev->effects |= EF_LIGHT;
			pev->origin.x += 160;
			pev->origin.z += 160;
		}
		if(pev->frags == 100){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 1, 255, FFADE_IN );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/Flash3.wav", 1, 0);
			pev->effects = 0;

			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "respawn_stone1" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
			pTel = UTIL_FindEntityByTargetname( NULL, "respawn_stone_btn1" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
		}
		if(pev->frags == 105){//�ҷ�ȫ�帴���ȫ�ָ�����ֵ!!
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_LARGEFUNNEL );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFlareGlow );
			WRITE_SHORT( 1 );
			MESSAGE_END();

			pPlayer->pev->health = pPlayer->pev->max_health;
			pPlayer->TeamMate_Nagamatagi_RespawnStone(1);
		}
		if(pev->frags == 160){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->origin.x -= 512;
				pSpot->pev->origin.y -= 512;
				pSpot->pev->angles.y = 180;
			}
		}
		if(pev->frags == 170){
			FireTargets( "gr_nv_mtdr_enter", this, this, USE_TOGGLE, 0 );
			//ս�����ظ���
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "monster_combat_in" );
			if ( pTel ){
				pTel->pev->origin.y -= 2048;
			}
			pTel = UTIL_FindEntityByTargetname( NULL, "player_combat_in" );
			if ( pTel ){
				pTel->pev->origin.y -= 2048;
			}
		}
		if(pev->frags == 180){//����һ��һ��һ��һ��������������ʵ�壡
				pPlayer->Clear_SayText();
				CBaseEntity *pEntity = NULL;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
				{
					if ( (pEntity->pev->flags & FL_MONSTER) ){
						if(pEntity->Classify() != CLASS_PLAYER_ALLY && pEntity->pev->deadflag != DEAD_NO){
						UTIL_Remove( pEntity );//���ʬ��
						}
					}
				}
		}
		if(pev->frags == 200){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 220){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pev->origin - Vector(150,0,120);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
			UTIL_Remove( pEntity );
			}
		}
		if(pev->frags == 230){
			pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
		}
		if(pev->frags == 240){
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 65){//�¼�65 ����ʯ��2���ٺ���ħBOSSս!
		if(pev->frags == 0){
				pPlayer->pev->origin = pev->origin + Vector(0,1280,0);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;

				pPlayer->m_music_save = 0;
				SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 2){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin - Vector(0,64,0), Vector(0,270,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );
				pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 6){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
			pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(192,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc2 != NULL){
			pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(64,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-192,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
		}
		if(pev->frags == 10){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
					pSpot->pev->angles.x = 10;
				}
		}
		if(pev->frags == 70){
			pev->effects |= EF_LIGHT;
			pev->origin.y -= 160;
			pev->origin.z += 160;
		}
		if(pev->frags == 100){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 1, 255, FFADE_IN );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/Flash3.wav", 1, 0);
			pev->effects = 0;

			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "respawn_stone2" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
			pTel = UTIL_FindEntityByTargetname( NULL, "respawn_stone_btn2" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
		}
		if(pev->frags == 105){//�ҷ�ȫ�帴���ȫ�ָ�����ֵ!!
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_LARGEFUNNEL );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFlareGlow );
			WRITE_SHORT( 1 );
			MESSAGE_END();

			pPlayer->pev->health = pPlayer->pev->max_health;
			pPlayer->TeamMate_Nagamatagi_RespawnStone(1);
		}
		if(pev->frags == 140){
			FireTargets( "monster_blood_dr", this, this, USE_TOGGLE, 0 );
			FireTargets( "fong_road_bw", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 150){//����һ��һ��һ��һ��������������ʵ�壡
				CBaseEntity *pEntity = NULL;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
				{
					if ( (pEntity->pev->flags & FL_MONSTER) ){
						if(pEntity->Classify() != CLASS_PLAYER_ALLY && pEntity->pev->deadflag != DEAD_NO){
						UTIL_Remove( pEntity );//���ʬ��
						}
					}
				}
		}
		if(pev->frags == 160){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_barnacle_boss");
			if ( pSpot ){
			pSpot->pev->takedamage = DAMAGE_NO;
			UTIL_SetOrigin( pSpot->pev, pev->origin + Vector(0,-640,-160));

			FX_Explosion(pSpot->Center(), EXPLOSION_DISPTELEPORT );//Bug Fix 3.0 BOSS�ӿ�����͸�����Ч
			}
		}
		if(pev->frags == 200){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 220){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pev->origin + Vector(0,100,40);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
			UTIL_Remove( pEntity );
			}
		}
		if(pev->frags == 230){
			pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
		}
		if(pev->frags == 250){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_barnacle_boss");
			if ( pSpot ){
				pSpot->pev->takedamage = DAMAGE_AIM;
				pSpot->pev->spawnflags = 0;
				pPlayer->BOSS_Find();
				game_boss_battle = 1;
			}
			pPlayer->m_music_save = 14;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 5\n");
			//SERVER_COMMAND("mp3 loop media/boss6.mp3\n");
			pPlayer->m_game_rate = 79;//��Ϸ����79%
		}
		if(pev->frags == 260){
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 66){//�¼�66 BOSS��ɱ!
		if(pev->frags == 0){
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 60){
			pPlayer->m_game_rate = 80;//��Ϸ����80%
			FireTargets( "gr_nv_mtdr_exit", this, this, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 67){//�¼�67 �����������
		if(pev->frags == 0){
			pPlayer->pev->origin = pev->origin;
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,270,0);
			pPlayer->pev->angles = Vector(0,270,0);
			pPlayer->pev->fixangle = TRUE;
		}
		if(pev->frags == 100){//��BGM
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 68){//�¼�68 ����ʯ��3�����������ڲ�!
		if(pev->frags == 0){
				pPlayer->pev->origin = pev->origin + Vector(0,384,0);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_wdoor_mynpc = NULL;
				pPlayer->m_guard_mynpc = 0;
		}
		if(pev->frags == 2){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin - Vector(0,64,0), Vector(0,270,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );
				pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 6){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
			pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(192,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc2 != NULL){
			pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(64,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-192,0,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
		}
		if(pev->frags == 10){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
					pSpot->pev->angles.x = 10;
				}
		}
		if(pev->frags == 60){
			pev->effects |= EF_LIGHT;
			pev->origin.y -= 160;
			pev->origin.z += 160;
		}
		if(pev->frags == 90){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 1, 255, FFADE_IN );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/Flash3.wav", 1, 0);
			pev->effects = 0;

			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "respawn_stone3" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
			pTel = UTIL_FindEntityByTargetname( NULL, "respawn_stone_btn3" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
		}
		if(pev->frags == 95){//�ҷ�ȫ�帴���ȫ�ָ�����ֵ!!
			pPlayer->pev->health = pPlayer->pev->max_health;
			pPlayer->TeamMate_Nagamatagi_RespawnStone(1);

			FireTargets( "gate_dr_wall1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 140){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pev->origin + Vector(0,100,40);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
			UTIL_Remove( pEntity );
			}
		}
		if(pev->frags == 150){
			pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
		}
		if(pev->frags == 160){
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 69){//�¼�69 ����С�ͺ˵�!
		if(pev->frags == 0){
				SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 5){
				pPlayer->pev->origin = pev->origin + Vector(-640,0,64);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_game_rate = 81;//��Ϸ����81%
		}
		if(pev->frags == 8){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(-16,0,0), pev->angles, NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "duck_combat_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 10){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
			pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,48,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 350;
			}
			if (pPlayer->m_team_npc2 != NULL){
			pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,96,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 350;
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,-48,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 10;
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,-96,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 10;
			}
		}
		if(pev->frags == 15){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
					pSpot->pev->angles.x = 10;
					pSpot->pev->origin.x -= 16;
				}
		}
		if(pev->frags == 60){
			CBaseEntity *pNukeEnt = Create( "npc_aim_flag", pev->origin + Vector(48,0,0), Vector(0,180,0), NULL );
			if ( pNukeEnt ){
				pNukeEnt->pev->health = 3000;
				pNukeEnt->pev->frags = 2;
				SET_MODEL(ENT(pNukeEnt->pev), "models/nuke_box.mdl");
				pPlayer->MenuItem_remove(23);
			}
		}
		if(pev->frags == 100){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->origin.x += 144;
				pSpot->pev->origin.z -= 48;
				pSpot->pev->angles.x = 0;
			}
		}
		if(pev->frags == 120){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "npc_aim_flag");
			if ( pSpot ){
				pSpot->pev->body = 1;
			}
		}
		if(pev->frags == 160){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pev->origin + Vector(0,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
			UTIL_Remove( pEntity );
			}
			FireTargets( "locked_grid1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 170){
			pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
		}
		if(pev->frags == 200){
			char text[256];
			
			sprintf( text, "- Don't let the bomb be defused!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 240){
			FireTargets( "mspawn_door1", this, this, USE_TOGGLE, 0 );
			FireTargets( "mscombat_maker", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 270){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 480){//ս������
			FireTargets( "mscombat_maker2", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1240){//����Լ1�ְ��Ӥ�ս��
			FireTargets( "mscombat_maker", this, this, USE_TOGGLE, 0 );
			FireTargets( "mscombat_maker2", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1280){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			pPlayer->pev->origin = pev->origin + Vector(-640,0,64);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_music_save = 0;
			pPlayer->m_wdoor_mynpc = NULL;
			pPlayer->m_guard_mynpc = 0;
			SERVER_COMMAND("mp3 stop\n");
		}
		if(pev->frags == 1285){//��������!
				CBaseEntity *pEntity = NULL;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
				{
					if (  (pEntity->pev->flags & FL_MONSTER) ){
						if(FClassnameIs ( pEntity->pev, "monster_barney" )
						|| FClassnameIs ( pEntity->pev, "monster_barney_hevshield" )){
						UTIL_Remove( pEntity );
						}
					}
				}
				//׷�ӹ������x2
				pEntity = Create( "monster_barney", Vector(-695,3096,915), Vector(0,270,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_longming = 1;
				pEntity->pev->team = 1;

				pEntity = Create( "monster_barney_hevshield", Vector(1869,3073,915), Vector(0,180,0), NULL );
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_longming = 1;
				pEntity->pev->team = 1;
		}
		if(pev->frags == 1290){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(-16,0,0), pev->angles, NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );
				pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 1295){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
			pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,48,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 350;
			}
			if (pPlayer->m_team_npc2 != NULL){
			pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,96,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 350;
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,-48,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 10;
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-64,-96,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 10;
			}
		}
		if(pev->frags == 1300){
				FireTargets( "mspawn_door1", this, this, USE_TOGGLE, 0 );
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pPlayer->pev->angles = pSpot->pev->angles;
					pSpot->pev->angles.x = 10;
					pSpot->pev->origin.x -= 144;
					pSpot->pev->origin.z += 48;
				}
		}
		if(pev->frags == 1320){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 1340){
				char text[256];
				sprintf( text, "???: Kadoma!!\n");
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1360){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){
				pEntity2->pev->effects = 0;
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 1, 1 );
				pEnemyMonster->SetBodygroup( 2, 2 );
				}
		}
		if(pev->frags == 1370){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
				pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->pev->angles.y = 180;
				}
			}
			if (pPlayer->m_team_npc2 != NULL){
				pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->pev->angles.y = 180;
				}
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->pev->angles.y = 180;
				}
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->pev->angles.y = 180;
				}
			}

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->angles.y = 180;
			}
		}
		if(pev->frags == 1380){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				CLIENT_COMMAND(pPlayer->edict(), "=cammousemove\n");//��ȫ��������
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->angles.y = 180;
				pSpot->pev->origin.x += 160;
				pSpot->pev->angles.x = -12;
				pSpot->pev->origin.z -= 36;
				pSpot->pev->velocity.x = -5;
				pSpot->pev->velocity.z = 10;
				pSpot->pev->avelocity.x = 1;
			}
		}
		if(pev->frags == 1420){
			char text[256];
			
			sprintf( text, "Gman: Why must you oppose me?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1460){
			char text[256];
			
			sprintf( text, "Gman: You think you're in the right? It's okay to destroy this place?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1520){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 12;
			}
			char text[256];
			
			sprintf( text, "Gman: You can't! You won't! You mustn't do this!\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1600){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1610){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = -80;
			}
		}
		if(pev->frags == 1620){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "draw_needle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 2, 1 );
				}
		}
		if(pev->frags == 1640){
			char text[256];
			
			sprintf( text, "Gman: You see this? Do you know what this is??\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1680){
			char text[256];
			
			sprintf( text, "Gman: It's the fruit of our research, and grants Godlike power!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1760){
			char text[256];
			
			sprintf( text, "Gman: The sacrifices were necessary to end suffering and kill Evil God Z.Z.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1840){
			char text[256];
			
			sprintf( text, "Gman: I'll show you the power of Blue Fantasy!!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1920){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1930){
				pPlayer->pev->v_angle = Vector(0,180,0);
				pPlayer->pev->angles = Vector(0,180,0);
				pPlayer->pev->fixangle = TRUE;

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
				if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "use_needle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
		}
		if(pev->frags == 1980){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.5, 1.5, 255, FFADE_OUT );
		}
		if(pev->frags == 2000){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman" );
			if ( pEntity2 ){
			CBaseEntity *pEntity = Create( "monster_gman_boss", pEntity2->pev->origin, g_vecZero, NULL );
			UTIL_Remove( pEntity2 );
			}

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 15;
				pSpot->pev->velocity.z = -10;
				pSpot->pev->avelocity.x = -3;
				pSpot->pev->armortype = 0;
			}

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.5, 1.5, 255, FFADE_IN );
		}
		if(pev->frags == 2050){
			char text[256];
			
			sprintf( text, "Gman: You're wrong. I'm right.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 2060){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 2080){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->armortype = 12;
			}
		}
		if(pev->frags == 2100){
			pPlayer->Clear_SayText();
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
			FX_Explosion( pEntity2->Center(), EXPLOSION_SPARKSHOWER );
			EMIT_SOUND(ENT(pEntity2->pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->m_boltpoison = 40;
			pEnemyMonster->m_FTSmod = 3;
			UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(640,0,0) );
			}
		}
		if(pev->frags == 2110){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pev->origin + Vector(0,0,40);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
			UTIL_Remove( pEntity );
			}

			FireTargets( "gman_locked_tgwall", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 2120){
			pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
		}
		if(pev->frags == 2140){
			pPlayer->m_music_save = 16;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 18\n");
			//SERVER_COMMAND("mp3 loop media/boss7.mp3\n");
			pPlayer->BOSS_Find();//Bug Fix 2.0 Ѫ������
			game_boss_battle = 1;
		}
		if(pev->frags == 3190){
		pev->frags += 1;
		pev->nextthink = gpGlobals->time + 16.0;//ʱͣ!
		return;
		}
		if(pev->frags == 3200){
			pPlayer->m_music_save = 0;
			SERVER_COMMAND("mp3 stop\n");
			game_boss_battle = 0;

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			pPlayer->pev->origin = pev->origin + Vector(-640,0,64);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			pPlayer->m_wdoor_mynpc = NULL;
			pPlayer->m_guard_mynpc = 0;
		}
		if(pev->frags == 3205){
			CBaseMonster *pEnemyMonster;
			if (pPlayer->m_team_npc1 != NULL){
			pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-100,320,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc2 != NULL){
			pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(-36,320,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc3 != NULL){
			pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(64,320,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
			if (pPlayer->m_team_npc4 != NULL){
			pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( pEnemyMonster->pev, pev->origin + Vector(128,320,0) );
				if(pEnemyMonster->pev->deadflag == DEAD_NO){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
			pEnemyMonster->pev->angles.y = 270;
			}
		}
		if(pev->frags == 3210){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->angles.x = 0;
				pSpot->pev->angles.y = 270;
				pSpot->pev->velocity.y = 10;
				pSpot->pev->armortype = 0;
				pSpot->pev->origin = pev->origin + Vector(0,420,70);
				pPlayer->pev->angles = pSpot->pev->angles;
			}

			CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(0,300,0), Vector(0,270,0), NULL );
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "duck_combat_idle" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 0, 3 );
			pEnemyMonster->SetBodygroup( 2, 7 );
			pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 3215){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();

				if ( pEntity2->pev->deadflag == DEAD_NO ){
				pEnemyMonster->Hunt_Stand_Set(0);
				}
				//Bug Fix 2.0 ��ҿ��һ�ɱGman��

				UTIL_SetOrigin( pEntity2->pev, pev->origin + Vector(0,100,0));
				pEntity2->pev->angles.y = 90;
			}
		}
		if(pev->frags == 3250){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
				if ( pEntity2->pev->deadflag == DEAD_NO ){//Bug Fix 2.0 Gman�������޾���Ի�
					char text[256];
					
					sprintf( text, "Gman: You can't hurt me, Kadoma!\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
		}
		if(pev->frags == 3260){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->armortype = 12;
			}
		}
		if(pev->frags == 3280){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
				if ( pEntity2->pev->deadflag == DEAD_NO ){//Bug Fix 2.0 Gman�������޾���Ի�
					char text[256];
					
					sprintf( text, "Gman: Time to end this!\n");
					
					UTIL_SayTextAll( text,this );
				}
				else{//Bug Fix 2.0 Gman�������������ԭ�ظ���
					sprintf( text, "Gman: ..........\n");
					UTIL_SayTextAll( text,this );

					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->Hunt_Stand_Set(1);
				}
			}
		}
		if(pev->frags == 3330){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 3340){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->origin = pev->origin + Vector(32,96,48);
				pSpot->pev->angles.y = 300;
				pSpot->pev->angles.x = 10;
			}
		}
		if(pev->frags == 3350){
			CBaseEntity *pNukeEnt = UTIL_FindEntityByClassname( NULL, "npc_aim_flag");
			if ( pNukeEnt ){
			pNukeEnt->pev->rendermode = 0;
			pNukeEnt->pev->renderfx = kRenderFxExplode;
			pNukeEnt->pev->animtime = gpGlobals->time;
			pNukeEnt->pev->framerate = 1.0;
			pNukeEnt->pev->frame = 0;
			pNukeEnt->pev->rendercolor.x = 255;
			pNukeEnt->pev->rendercolor.y = 255;
			pNukeEnt->pev->rendercolor.z = 255;
			pNukeEnt->pev->effects |= EF_LIGHT;
			}
		}
		if(pev->frags == 3370){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity2->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "use_teleport_book" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			}

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->origin = pev->origin + Vector(0,160,80);
				pSpot->pev->angles.y = 90;
				pSpot->pev->angles.x = 0;
				pSpot->pev->velocity.y = 10;

				pPlayer->pev->v_angle = Vector(0,90,0);
				pPlayer->pev->angles = Vector(0,90,0);
				pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 3420){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman_boss" );
			if ( pEntity2 ){
			UTIL_Remove( pEntity2 );
			}

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->origin = pev->origin + Vector(-48,0,64);
				pSpot->pev->angles.y = 0;
				pSpot->pev->angles.x = 10;

				pPlayer->pev->v_angle = Vector(0,180,0);
				pPlayer->pev->angles = Vector(0,180,0);
				pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 3430){
				char text[256];
				
				sprintf( text, "Gman: Shit, it's gonna blow!\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 3470){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->origin = pev->origin + Vector(768,0,128);
				pSpot->pev->angles.y = 180;
				pSpot->pev->angles.x = 0;
			}
		}
		if(pev->frags == 3480){
			CBaseEntity *pNukeEnt = UTIL_FindEntityByClassname( NULL, "npc_aim_flag");
			if ( pNukeEnt ){
				FX_Explosion( pNukeEnt->Center(), 128 );
				EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.1);
				FireTargets( "unknow_machine_break", this, this, USE_TOGGLE, 0 );
				FireTargets( "unknow_machine_water", this, this, USE_TOGGLE, 0 );
				UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 4, 255, FFADE_OUT );
				UTIL_Remove( pNukeEnt );
			}
		}
		if(pev->frags >= 3481 && pev->frags <= 3486){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "unknow_machine_rot" );
			if ( pTel ){
			UTIL_Remove( pTel );
			}
		}
		if(pev->frags == 3490){
			FireTargets( "unbreak_glass", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 3510){
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_IN );
		}
		if(pev->frags == 3560){
			FireTargets( "nuke_clear_teleport", this, this, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 70){//�¼�70 ���ͺڰ��ռ䣬kadoma��С��!
		if(pev->frags == 5){
				pPlayer->pev->origin = pev->origin + Vector(0,-256,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_game_rate = 82;//��Ϸ����82%
		}
		if(pev->frags == 8){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin, pev->angles, NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "sit_fag" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity->pev->flags	|= FL_NOTARGET;
		}
		if(pev->frags == 10){
				//������� & ����
				pPlayer->TeamMate_Nagamatagi_Allclear(2);
		}
		if(pev->frags == 18){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pPlayer->pev->angles = pSpot->pev->angles;
				pSpot->pev->velocity.y = 5;
				pSpot->pev->velocity.x = 5;
				pSpot->pev->velocity.z = -1;
				pSpot->pev->avelocity.y = -10;
			}

			pPlayer->m_music_save = 15;
			SERVER_COMMAND("mp3 loop media/music22.mp3\n");
		}
		if(pev->frags == 138){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->armortype = 8;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 208){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->armortype = 0;
				pSpot->pev->frags = 0;
				pSpot->pev->velocity = g_vecZero;
				pSpot->pev->avelocity = g_vecZero;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
			if ( pEntity ){
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "console_stand" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 212){//Bug Fix 3.0 ���ɹ���С��ʵ��
			CBaseEntity *pEntity = Create( "monster_generic_item2", pev->origin + Vector(-1700,340,0), Vector(0,0,0), NULL );
			SET_MODEL(ENT(pEntity->pev), "models/props_all.mdl");
			pEntity->pev->body = 8;
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);
		}
		if(pev->frags == 218){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity ){
					UTIL_Remove( pEntity );
				}
				pPlayer->pev->health = pPlayer->pev->max_health;//�Զ���Ѫ
				pPlayer->m_flVelocityModifier = -4;
				pPlayer->m_flash_mode = 2;//��ˮģʽ
				pPlayer->pev->origin = pev->origin + Vector(0,48,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(TRUE);
				pPlayer->m_trainning = 0;
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
		}
		if(pev->frags == 225){
				SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 240){
				char text[256];
				
				sprintf( text, "- Kadoma and his friends were scattered by an unknown force.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 300){
				char text[256];
				
				sprintf( text, "- He stayed in this strange, dark place for several days - it's time to leave.\n");
				
				UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 380){
				pPlayer->Clear_SayText();
				UTIL_Remove( this );
				return;
		}
	}
	if(pev->armortype == 71){//�¼�71 �ڰ���ˮ��
		if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 3){
			pPlayer->m_flash_mode = 3;
			pPlayer->pev->origin = pev->origin;
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->m_flVelocityModifier = 0;
			pPlayer->pev->v_angle = Vector(0,0,0);
			pPlayer->pev->angles = Vector(0,0,0);
			pPlayer->pev->fixangle = TRUE;
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 5){
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 72){//�¼�72 ��ȥ��Vanlve��CG�����ݹ���ս
		if(pev->frags == 5){
				pPlayer->m_music_save = 0;
				SERVER_COMMAND("mp3 stop\n");
				pPlayer->pev->origin = pev->origin + Vector(-1024,32,64);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				pPlayer->m_game_rate = 83;//��Ϸ����83%
		}
		if(pev->frags == 8){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(-32,0,0), Vector(0,270,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_friendidle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				SetBits( pEntity->pev->effects, EF_DIMLIGHT);
		}
		if(pev->frags == 20){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.z = -5;
				pSpot->pev->velocity.x = 5;
				pSpot->pev->velocity.y = -5;
			}
		}
		if(pev->frags == 60){
			char text[256];
			
			sprintf( text, "- What happened?\n");
		
			UTIL_SayTextAll( text,this );

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->armortype = 5;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 80){
			char text[256];
			
			sprintf( text, "- And why?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 100){
			char text[256];
			
			sprintf( text, "- Why keep fighting?\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 130){
			char text[256];
			
			sprintf( text, "- Kadoma doesn't know.\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 190){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 20, 255, FFADE_OUT );
			//����ǰ
			pPlayer->m_iClient_Gameover = 2;
			pPlayer->m_fGameOverTime = gpGlobals->time + 5;
		}
		if(pev->frags == 220){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 3, 255, FFADE_IN );//��Ϲ���

				CBaseEntity *pEntity = Create( "monster_vanlve_combat", pSpot->pev->origin, pSpot->pev->angles, NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fly_float" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEntity->pev->movetype = MOVETYPE_FLY;

				pSpot->pev->origin.x -= 256;
				pSpot->pev->origin.z += 64;
				pSpot->pev->angles.y = 0;
				
				FireTargets( "vanlve_gmanmaker", this, this, USE_TOGGLE, 0 );
			}
		}
		if(pev->frags == 225){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->avelocity.y = 10;
			pSpot->pev->velocity.x = 30;
			pSpot->pev->velocity.y = -15;
			}
		}
		if(pev->frags == 230){
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 17\n");
			//SERVER_COMMAND("mp3 play media/boss2.mp3\n");
		}
		if(pev->frags == 310){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->frags = 1;
			pSpot->pev->armortype = 8;
			}
		}
		if(pev->frags == 350){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->avelocity.y = 80;
			pSpot->pev->velocity.y = -1400;
			pSpot->pev->velocity.x = 0;
			pSpot->pev->velocity.z = 20;
			pSpot->pev->armortype = 0;
			}
	
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			SetBits(pGmanboss->pev->effects, EF_DIMLIGHT);
			pGmanboss->pev->body = 0;
			}
		}
		if(pev->frags == 370){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->avelocity.y = 0;
			pSpot->pev->velocity.y = 10;
			pSpot->pev->velocity.z = 0;
			pPlayer->pev->v_angle = pSpot->pev->angles;
			pPlayer->pev->angles = pSpot->pev->angles;
			pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 400){
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			CBaseMonster *pEnemyMonster = pGmanboss->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "new_guard" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			}
		}
		if(pev->frags == 470){
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			CBaseMonster *pEnemyMonster = pGmanboss->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "new_walk_run" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pGmanboss->pev->movetype = MOVETYPE_FLY;
			ClearBits( pGmanboss->pev->flags, FL_ONGROUND );
			pGmanboss->pev->velocity.y = 400;
			}
		}
		if(pev->frags == 490){
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			pGmanboss->pev->velocity.y = 150;
			}

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
					CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
					if ( pVanlve ){
						CBaseMonster *pEnemyMonster = pVanlve->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "float_shoot" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pVanlve->pev->movetype = MOVETYPE_FLY;
						ClearBits( pGmanboss->pev->flags, FL_ONGROUND );
						pVanlve->pev->velocity.y = 180;
						pVanlve->pev->velocity.z = 10;

						pSpot->pev->angles.y = 0;
						pSpot->pev->velocity.x = 0;
						pSpot->pev->velocity.y = 180;
						pSpot->pev->velocity.z = 0;
						pSpot->pev->origin = pVanlve->pev->origin + Vector(-192,0,64);
						pPlayer->pev->v_angle = pSpot->pev->angles;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
			}
		}
		if(pev->frags == 520){
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
				pGmanboss->pev->velocity.y = 400;

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
				pSpot->pev->angles.y = 270;
				pSpot->pev->velocity.x = 0;
				pSpot->pev->velocity.y = 400;
				pSpot->pev->velocity.z = 0;
				pSpot->pev->origin = pGmanboss->pev->origin + Vector(0,192,96);
				pPlayer->pev->v_angle = pSpot->pev->angles;
				pPlayer->pev->angles = pSpot->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				}
			}
		}
		if(pev->frags == 530){
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
				pGmanboss->pev->body = 2;
				EMIT_SOUND_DYN( ENT(pGmanboss->pev), CHAN_STREAM, "newadd/PowerShield.wav", 1, 0.6, 0, 100);
			}
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				pVanlve->pev->velocity.y = 0;
				pVanlve->pev->velocity.z = 0;
			}
		}
		if(pev->frags >= 533 && pev->frags <= 558){
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
				if(pev->frags == 533){
				FX_Explosion( pGmanboss->pev->origin + Vector(0,96,96), 129 );
				}
				else if(pev->frags == 536){
				FX_Explosion( pGmanboss->pev->origin + Vector(48,96,128), 129 );
				}
				else if(pev->frags == 539){
				FX_Explosion( pGmanboss->pev->origin + Vector(-48,96,64), 129 );
				}
				else if(pev->frags == 542){
				FX_Explosion( pGmanboss->pev->origin + Vector(0,96,64), 129 );
				}
				else if(pev->frags == 545){
				FX_Explosion( pGmanboss->pev->origin + Vector(-48,96,96), 129 );
				}
				else if(pev->frags == 548){
				FX_Explosion( pGmanboss->pev->origin + Vector(0,96,128), 129 );
				}
				else if(pev->frags == 558){
				pGmanboss->pev->body = 1;
				}
			}
		}
		if(pev->frags == 560){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->avelocity.y = 45;
			pSpot->pev->velocity.x = -120;
			pSpot->pev->velocity.y = 360;
			}
		}
		if(pev->frags == 580){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->avelocity.y = 0;
			pSpot->pev->velocity.x = 0;
			pPlayer->pev->v_angle = pSpot->pev->angles;
			pPlayer->pev->angles = pSpot->pev->angles;
			pPlayer->pev->fixangle = TRUE;
			}
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			CBaseMonster *pEnemyMonster = pGmanboss->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "new_run_attack" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pGmanboss->pev->weapons = 520;
			}
		}
		if(pev->frags == 593){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->avelocity.y = 0;
			pSpot->pev->velocity.x = 10;
			pSpot->pev->frags = 1;
			pSpot->pev->armortype = 8;
			}
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			CBaseMonster *pEnemyMonster = pGmanboss->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "new_skill2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pGmanboss->pev->velocity.y = 90;
			pGmanboss->pev->velocity.z = 30;
			}
		}
		if(pev->frags == 600){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				pVanlve->pev->velocity.y = 0;
				pVanlve->pev->velocity.z = 0;
			}

			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
			FX_Explosion( pGmanboss->Center(), EXPLOSION_SPARKSHOWER );
			UTIL_SetOrigin( pGmanboss->pev, pGmanboss->pev->origin + Vector(0,-2100,0) );
			pGmanboss->pev->velocity.y = 0;
			pGmanboss->pev->velocity.z = 0;

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/dragonball_dash.wav", 1, 0);

			CBaseMonster *pEnemyMonster = pGmanboss->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "new_skill2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 602){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
			EMIT_SOUND(ENT(pVanlve->pev), CHAN_WEAPON, "tank/tank_fire.wav", 1.0, 0.4);
			}
		}
		if(pev->frags == 603){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			FX_Explosion( pSpot->pev->origin + Vector(256,0,0), 130 );
			}
		}
		if(pev->frags == 622){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				CBaseMonster *pEnemyMonster = pVanlve->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "float_stab" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 640){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->origin.x -= 32;
			pSpot->pev->origin.y -= 2500;
			pSpot->pev->velocity.y = -10;
			}
		}
		if(pev->frags == 642){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 4, 255, FFADE_OUT );
		}
		if(pev->frags == 650){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			FX_Explosion( pSpot->pev->origin + Vector(256,0,0), 128 );
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.1);
			}
		}
		if(pev->frags == 670){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 2, 255, FFADE_IN );
			CBaseEntity *pGmanboss = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
			if ( pGmanboss ){
				CBaseEntity *pEntity = Create( "monster_gman2", pGmanboss->pev->origin, pGmanboss->pev->angles, NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "stand" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 1 );
				pEnemyMonster->SetBodygroup( 1, 1 );
				pEnemyMonster->SetBodygroup( 2, 2 );
				UTIL_Remove( pGmanboss );
				pEntity->pev->velocity.z = -1;
			}
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				CBaseMonster *pEnemyMonster = pVanlve->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "hurt_crouch" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pVanlve->pev->movetype = MOVETYPE_STEP;
				pVanlve->pev->velocity.z = -1;
				FX_Trail(pVanlve->pev->origin, pVanlve->entindex(), PROJ_BLACKHOLE);
				EMIT_SOUND(ENT(pVanlve->pev), CHAN_AUTO, "weapons/blackhole_exp.wav", 1, ATTN_LOW);
				ClearBits(pVanlve->pev->effects, EF_DIMLIGHT);

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
				pSpot->pev->origin = pVanlve->pev->origin + Vector(0,720,80);
				pSpot->pev->angles.y = 270;
				pSpot->pev->angles.x = 10;
				pSpot->pev->velocity.y = -10;
				pSpot->pev->velocity.z = -1;
				pPlayer->pev->v_angle = pSpot->pev->angles;
				pPlayer->pev->angles = pSpot->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				}
			}
		}
		if(pev->frags == 730){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				FX_Trail(pVanlve->pev->origin, pVanlve->entindex(), PROJ_BLACKHOLE_DETONATE);

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
				pSpot->pev->armortype = 13;
				pSpot->pev->frags = 1;
				}
			}
		}
		if(pev->frags == 770){
			SERVER_COMMAND("mp3 stop\n");
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
			pSpot->pev->armortype = 14;
			}
		}
		if(pev->frags == 780){
			CBaseEntity *pGman2 = UTIL_FindEntityByClassname( NULL, "monster_gman2");
			if ( pGman2 ){
				CBaseMonster *pEnemyMonster = pGman2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "heal_body" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 840){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
				pSpot->pev->armortype = 0;
				pSpot->pev->angles.y = 90;
				pSpot->pev->angles.x = 30;
				pSpot->pev->velocity.y = -15;
				pSpot->pev->armortype = 8;
				pSpot->pev->origin = pVanlve->pev->origin + Vector(0,-64,64);
				}
			}
			CBaseEntity *pGman2 = UTIL_FindEntityByClassname( NULL, "monster_gman2");
			if ( pGman2 ){
			UTIL_Remove( pGman2 );
			}
		}
		if(pev->frags == 860){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				CBaseMonster *pEnemyMonster = pVanlve->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "hurt_die" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 890){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 4, 255, FFADE_OUT );
		}
		if(pev->frags == 920){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "deep_idle2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 2, 7 );
				pEntity2->pev->angles.y = 180;

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
				if ( pSpot ){
					pSpot->pev->origin = pEntity2->pev->origin + Vector(-40,-160,80);
					pSpot->pev->angles.y = 90;
					pSpot->pev->angles.x = 0;
					pSpot->pev->avelocity = g_vecZero;
					pSpot->pev->velocity = g_vecZero;

					pPlayer->pev->v_angle = Vector(0,90,0);
					pPlayer->pev->angles = Vector(0,90,0);
					pPlayer->pev->fixangle = TRUE;
				}
			}
		}
		if(pev->frags == 930){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
				CBaseMonster *pEnemyMonster = pVanlve->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fly_float" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pVanlve->pev->rendermode  = kRenderTransTexture;
				pVanlve->pev->renderfx	 = kRenderFxHologram;
				pVanlve->pev->renderamt	 = 255;
				pEnemyMonster->SetBodygroup( 3, 0 );
				pVanlve->pev->angles.y = 0;
				UTIL_SetOrigin( pVanlve->pev, pev->origin + Vector(-96,0,48) );
				pVanlve->pev->movetype = MOVETYPE_FLY;
				pVanlve->pev->velocity = g_vecZero;
			}
		}
		if(pev->frags == 970){
			
			sprintf( text, "Vanlve: I found Gman, and fought him. I lost.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1010){
			
			sprintf( text, "Vanlve: He's strong, but he isn't at full power.\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1050){
			
			sprintf( text, "Vanlve: It's up to you now. The others were captured by Gman.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1100){
			
			sprintf( text, "Vanlve: Please, Kadoma.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1130){
			
			sprintf( text, "Vanlve: It's all up to you, and your unknown power...\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1200){
			pPlayer->Clear_SayText();
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
			pVanlve->pev->velocity = Vector(-15,0,30);
			}
		}
		if(pev->frags == 1250){
			CBaseEntity *pVanlve = UTIL_FindEntityByClassname( NULL, "monster_vanlve_combat");
			if ( pVanlve ){
			UTIL_Remove( pVanlve );
			}
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->GiveNamedItem( "weapon_darkgrenade" );
			pPlayer->GiveAmmo( 5, "rockets", ROCKET_MAX_CARRY );
			pPlayer->GiveNamedItem( "item_armor3" );
			pPlayer->pev->health = pPlayer->pev->max_health;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,180,0);
			pPlayer->pev->angles = Vector(0,180,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			}	
		}
		if(pev->frags == 1280){
			FireTargets( "mtlgar2", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1300){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 1310){
			pPlayer->m_music_save = 8;
			CLIENT_COMMAND(pPlayer->edict(), "cd loop 14\n");
			//SERVER_COMMAND("mp3 loop media/music17.mp3\n");
		}
		if(pev->frags == 1320){
			FireTargets( "vanlve_elev_msmaker1", this, this, USE_TOGGLE, 0 );
			pPlayer->m_game_rate = 85;//��Ϸ����85%
			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 73){//�¼�73 kadoma����nobita��ͬ����
		if(pev->frags == 0){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
				}
				pPlayer->pev->origin = pev->origin + Vector(0,-128,64);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
		}
		if(pev->frags == 1){
				CBaseEntity *pEntity = Create( "monster_kadoma", pev->origin + Vector(0,0,192), Vector(0,0,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_CROUCHIDLE );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEnemyMonster->SetBodygroup( 2, 7 );
		}
		if(pev->frags == 30){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
				CBaseMonster *pEnemyMonster;
				if ( pSpot ){
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sit_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pSpot->pev->angles.y = 180;
				}
				pSpot = UTIL_FindEntityByClassname( NULL, "monster_nobita");
				if ( pSpot ){
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sit_idle" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pSpot->pev->angles.y = 180;
				}
		}
		if(pev->frags == 40){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->velocity.y = 10;
				pSpot->pev->avelocity.y = 45;
			}
		}
		if(pev->frags == 50){
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 768, 0.3 );
		}
		if(pev->frags == 60){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->armortype = 8;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 120){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
			pPlayer->pev->health = pPlayer->pev->max_health;//Ѫ������
			pPlayer->EnableControl(TRUE);
			pPlayer->m_trainning = 0;
			pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
			pPlayer->m_stuck_origin = pPlayer->pev->origin;
			pPlayer->pev->velocity = g_vecZero;
			pPlayer->pev->v_angle = Vector(0,0,0);
			pPlayer->pev->angles = Vector(0,0,0);
			pPlayer->pev->fixangle = TRUE;
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			UTIL_Remove( pEntity2 );
			}	
		}
		if(pev->frags == 140){
			
			sprintf( text, "Saintna: From the ceiling again, Kadoma?\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 190){
			
			sprintf( text, "Giant: It's you? You scared me.\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 240){
			
			sprintf( text, "Nobita: Looks like everyone got out. Good.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 290){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_dengor");
			if ( pSpot ){
				
				sprintf( text, "Dengor: We're an excellent team.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_ZP14", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 330){
			CBaseEntity *pTelp = UTIL_FindEntityByTargetname( NULL, "computercamera_btn" );
			if ( pTelp ){
			pTelp->pev->impulse = 0;//��������
			}
		}
		if(pev->frags == 370){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 400){
			if(pPlayer->m_iNVG != 0){//�ڿ����?
			pev->frags = 380;
			}
			else{
				if(pPlayer->m_fGlodenKey == TRUE){
					
					sprintf( text, "Nobita: We need to find the Gold Key, okay?\n");
					
				}
				else{
					
					sprintf( text, "Nobita: I think the Gold Key is somewhere in this room.\n");
					
				}
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 450){
			if(pPlayer->m_fGlodenKey == TRUE && pPlayer->m_iNVG == 0){
				
				sprintf( text, "Nobita: Okay, now we can leave!\n");
				
			UTIL_SayTextAll( text,this );
			}
			else{
			pev->frags = 430;
			}
		}
		if(pev->frags == 500){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_lelite");
			if ( pSpot ){
				
				sprintf( text, "Lelite: Alright!\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!LELITE_3", VOL_NORM, 0.5, 0, 95 );
			}
		}
		if(pev->frags == 550){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			pPlayer->pev->health = pPlayer->pev->max_health;//Bug Fix 3.0 Ѫ������ x 2

			CBaseEntity *pTelp = UTIL_FindEntityByTargetname( NULL, "nullclear_npctelphunt" );
			if ( pTelp ){
					Vector telporg = pTelp->pev->origin;
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_mario" );
					CBaseMonster *pEnemyMonster;
					if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
					UTIL_SetOrigin( pEntity->pev, telporg);
					telporg.x += 64;
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
					pEnemyMonster->m_rpgms_level = 70;
					UTIL_SetOrigin( pEntity->pev, telporg);
					telporg.x += 64;
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor" );
					if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
					pEnemyMonster->m_rpgms_level = 60;
					UTIL_SetOrigin( pEntity->pev, telporg);
					telporg.x += 64;
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_nobita" );
					if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
					pEnemyMonster->m_rpgms_level = 65;
					UTIL_SetOrigin( pEntity->pev, telporg);
					telporg.x += 64;
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_saintna" );
					if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEntity->pev->frags = 2;
					pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
					pEnemyMonster->m_rpgms_level = 60;
					UTIL_SetOrigin( pEntity->pev, telporg);
					telporg.x += 64;
					}
					pEntity = UTIL_FindEntityByClassname( NULL, "monster_lelite" );
					if ( pEntity ){
					pEnemyMonster = pEntity->MyMonsterPointer();
					pPlayer->TeamMate_add(pEnemyMonster);
					pEnemyMonster->Hunt_Stand_Set(1);//��ֹ
					pEnemyMonster->m_rpgms_level = 60;
					UTIL_SetOrigin( pEntity->pev, telporg);
					}
			}
			pPlayer->m_game_rate = 86;//��Ϸ����86%
			FireTargets( "unlocknobita_dr", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 560){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 570){
			UTIL_Remove( this );
			return;
		}
	}
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
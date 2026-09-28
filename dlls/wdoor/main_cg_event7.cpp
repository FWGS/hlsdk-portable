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

extern DLL_GLOBAL int			g_causality_add;

extern DLL_GLOBAL int			g_fGameJumpCG;
extern DLL_GLOBAL int			g_fGameSkipCG;
//=========================================================
// ��CG�¼���ͳ��7�����ս�֣�
//=========================================================
class CMain_Event7 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
	char text[256];
};

LINK_ENTITY_TO_CLASS( main_cg_event_new7, CMain_Event7 );//����7

void CMain_Event7::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event7::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��5=================================================//
void CMain_Event7::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}
	if(pev->armortype == 84){//�¼�84 �����������BOSSս!
		if(pev->frags == 0){
			SERVER_COMMAND( "autosave\n" );
		}
		if(pev->frags == 5){
			pPlayer->m_game_rate = 99;//��Ϸ����99%
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "deep_idle2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetBodygroup( 0, 3 );
			pEnemyMonster->SetBodygroup( 1, 0 );
			pEnemyMonster->SetBodygroup( 2, 7 );
			pEntity->pev->angles.y = 270;
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);
			UTIL_SetOrigin( pEntity->pev, pev->origin);
			}
		}
		if(pev->frags == 10){
			pPlayer->pev->origin = pev->origin + Vector(0,36,36);
			pPlayer->TeamMate_Nagamatagi_Teleport(9);
		}
		if(pev->frags == 15){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->avelocity.y = 30;
				pSpot->pev->avelocity.x = -1;
				pSpot->pev->velocity.z = -15;
				pSpot->pev->velocity.y = 30;
			}

			FireTargets( "cshl623godboss_pre", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 20){
			CLIENT_COMMAND(pPlayer->edict(), "cd play 27\n");
			//SERVER_COMMAND("mp3 play media/music26.mp3\n");
		}
		if(pev->frags == 105){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->avelocity.y = 0;
				pSpot->pev->avelocity.x = -1;
				pSpot->pev->velocity.z = -2;
				pSpot->pev->velocity.y = 10;
			}
		}
		if(pev->frags == 150){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->velocity.y = 0;
				pSpot->pev->avelocity.x = 0;
				pSpot->pev->velocity.z = 0;
				pSpot->pev->velocity.y = 0;
			}
			
			sprintf( text, "Z.Z: To get this far, and make me show you my true form, is truly amazing.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 210){
			
			sprintf( text, "Z.Z: But it's done. Mission Accomplished. Story over!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 270){
			
			sprintf( text, "Z.Z: What else is there to do? Do you think you can defy destiny?\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 340){
			
			sprintf( text, "Z.Z: Die!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 380){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_god623_boss");
			if ( pEntity ){
			pEntity->pev->renderfx = kRenderFxExplode;
			pEntity->pev->rendercolor.x = 255;
			pEntity->pev->rendercolor.y = 255;
			pEntity->pev->rendercolor.z = 255;
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 390){
			pPlayer->pev->v_angle = Vector(0,270,0);
			pPlayer->pev->angles = Vector(0,270,0);
			pPlayer->pev->fixangle = TRUE;

			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 4, 255, FFADE_OUT );
			FX_Explosion( pev->origin + Vector(0,-64,64), 128 );
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.1);

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->velocity.y = 30;
			}

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "cower_hit_duck" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetBodygroup( 1, 3 );
			pEntity->pev->movetype = MOVETYPE_FLY;
			pEntity->pev->velocity.y = 30;
			pEntity->pev->effects = 0;
			}
		}
		if(pev->frags == 392){
			pPlayer->TeamMate_Nagamatagi_Allclear(4);//��������
		}
		if(pev->frags == 394){//��ֹ����
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_mario");
			if ( pEntity2 ){
			pEntity2->pev->weapons = 0;
			}
			pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
			if ( pEntity2 ){
			pEntity2->pev->frags = 5;
			}

			pPlayer->pev->fov = pPlayer->m_iFOV = -60;
		}
		if(pev->frags == 396){
			pPlayer->TeamMate_Nagamatagi_Allclear(4);//�������� * 2
		}
		if(pev->frags == 440){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;

				UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_IN );//��Ϲ���
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
				if ( pEntity ){
				UTIL_SetOrigin( pEntity->pev, pSpot->pev->origin + Vector(0,192,0) );
				}

				pSpot->pev->angles.y = 90;
				pSpot->pev->origin = pEntity->pev->origin + Vector(0,-64,32);

				pPlayer->pev->origin = pSpot->pev->origin + Vector(0,-32,36);
				pPlayer->TeamMate_Nagamatagi_Teleport(10);

				pPlayer->pev->fov = pPlayer->m_iFOV = 0;
			}
		}
		if(pev->frags == 510){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				pSpot->pev->angles.y = 270;
				pSpot->pev->origin = pSpot->pev->origin + Vector(0,144,72);
				pSpot->pev->angles.x = 15;
			}
		}
		if(pev->frags == 560){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_god623_boss");
			if ( pEntity ){
			pEntity->pev->renderfx = 0;
			}
			pPlayer->TeamMate_Nagamatagi_Allclear(5);//������ʧ
		}
		if(pev->frags == 620){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				pSpot->pev->velocity.y = 5;
				pSpot->pev->velocity.z = -5;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma");
			if ( pEntity ){
				pEntity->SUB_StartFadeOut3();
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 5.0, 15.0, 255, FFADE_OUT );//��Ϲ���
			}
		}
		if(pev->frags == 680){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
			if ( pSpot ){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){//Gman���ڴ���!
				pEntity2->pev->effects = 0;

				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

				CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle01" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->yaw_speed = 0;
				pEntity2->pev->angles.y = 270;
				pEnemyMonster->SetBodygroup( 0, 0 );
				pEnemyMonster->SetBodygroup( 1, 0 );
				pEnemyMonster->SetBodygroup( 2, 0 );
				UTIL_SetOrigin( pEntity2->pev, pSpot->pev->origin );

				pPlayer->EnableControl(TRUE);
				pPlayer->m_trainning = 0;
				pPlayer->pev->origin = pSpot->pev->origin + Vector(0,-64,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->pev->velocity = g_vecZero;
				pPlayer->pev->v_angle = Vector(0,90,0);
				pPlayer->pev->angles = Vector(0,90,0);
				pPlayer->pev->fixangle = TRUE;
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );

				pPlayer->m_teleprort_in_xen = 2;
				}
				else{//Gman��ʧ��!
				pev->frags = 1330;//����
				}
			}
		}
		if(pev->frags == 740){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: The world is about to end.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP7", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 790){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: I can send you back in time; you will keep your memory and abilities.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 850){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: Go back, or keep fighting?\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 900){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
			if ( pEntity2 ){
				
				sprintf( text, "Gman: Time to choose.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_CHOOSE1", VOL_NORM, 0.5, 0, 100 );
			}
		}
		if(pev->frags == 930){
			FireTargets( "gman_escape_door", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags >= 940 && pev->frags <= 1080){//ѡ���ʱ��
			if(pPlayer->m_teleprort_in_xen == 0){
			pev->impulse = 1;//�ص���ȥ!
			pev->frags = 1100;
			pPlayer->Clear_SayText();
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			}
		}
		if(pev->impulse == 1){//�ص���ȥ��!
			if(pev->frags == 1140){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: Very clever.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP11", VOL_NORM, 0, 0, 100 );
				}
			}
			if(pev->frags == 1170){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: See you next time.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP10", VOL_NORM, 0, 0, 100 );
				}
			}
			if(pev->frags == 1230){
				pPlayer->Game_Save_SecondData();
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1240){
				if(pPlayer->m_player_diamonds == 18){//ȫ�ռ�!
				g_fGameJumpCG = 13;
				}
				else{
				g_fGameJumpCG = 3;//���C����
				}
				SERVER_COMMAND( "map wdoor_ending_c\n" );//������C��������Restart��
				return;
			}
		}
		else{
			if(pev->frags == 960){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1000){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: It's time to choose.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_CHOOSE2", VOL_NORM, 0.5, 0, 100 );
				}
			}
			if(pev->frags == 1080){
				pPlayer->Clear_SayText();
				FireTargets( "gman_escape_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 1100){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: No problem. You've proven yourself to be a decisive person.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP9", VOL_NORM, 0.5, 0, 100 );
				}
			}
			if(pev->frags == 1160){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: We all think you have unlimited potential.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP8", VOL_NORM, 0.5, 0, 100 );
				}
			}
			if(pev->frags == 1220){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					
					sprintf( text, "Gman: See you next time.\n");
					
					UTIL_SayTextAll( text,this );
					EMIT_SOUND_DYN( pEntity2->edict(), CHAN_VOICE, "!GM_ZP10", VOL_NORM, 0.5, 0, 100 );
				}
			}
			if(pev->frags == 1260){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 1290){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_gman");
				if ( pEntity2 ){
					FX_Explosion(pEntity2->Center(), EXPLOSION_DISPTELEPORT );
					UTIL_Remove( pEntity2 );
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
				}
			}
			if(pev->frags == 1320){
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
			}
			if(pev->frags == 1340){
				pPlayer->m_teleprort_in_xen = 1;

				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;

					pPlayer->pev->v_angle = Vector(0,0,0);
					pPlayer->pev->angles = Vector(0,0,0);
					pPlayer->pev->fixangle = TRUE;

					CBaseEntity *pEntity = Create( "monster_kadoma_arm", pSpot->pev->origin, Vector(0,180,0), NULL );
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );

					pEntity = Create( "monster_kadoma_arm2", pSpot->pev->origin+Vector(-128,128,0), Vector(0,180,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "lying_on_back" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 1, 1 );
					pEntity->pev->angles.y = 90;

					pEntity = Create( "monster_kadoma_arm2", pSpot->pev->origin+Vector(-128,0,0), Vector(0,180,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "lying_on_side" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 1, 1 );
					pEntity->pev->angles.y = 45;

					pEntity = Create( "monster_kadoma_arm2", pSpot->pev->origin+Vector(-128,-128,0), Vector(0,180,0), NULL );
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "lying_on_stomach" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 1, 2 );
					pEntity->pev->angles.y = 270;

					pSpot->pev->origin = pSpot->pev->origin + Vector(-64,0,64);
				}
			}
			if(pev->frags == 1380){
				
				sprintf( text, "- Kadoma's spiritual world\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1420){
				pPlayer->Clear_SayText();
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma_arm");
				if ( pEntity ){
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "raflinch" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 1421){
				pPlayer->pev->fov = pPlayer->m_iFOV = -10;
			}
			if(pev->frags == 1424){
				pPlayer->pev->fov = pPlayer->m_iFOV = 0;
			}
			if(pev->frags == 1480){
				
				sprintf( text, "- Upon losing his arm from a hidden attack, he remembers what he forgot.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1540){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
				if ( pSpot ){
					pSpot->pev->origin = pSpot->pev->origin+ Vector(-256,0,-32);
				}
				
				sprintf( text, "- He has died countless times, and has the ability to go back in time.\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1620){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->Clear_SayText();

				CBaseEntity *pEntity = Create( "monster_zdeadeye", pev->origin + Vector(0,-384,192), Vector(0,90,0), NULL );
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );

				pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma_arm");
				if ( pEntity ){
				pEntity->pev->angles.y = 270;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(0,0,0) );
				}

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin = pev->origin + Vector(0,80,64);
					pSpot->pev->angles.y = 270;
					pSpot->pev->angles.x = -10;
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->avelocity = g_vecZero;
				}
			}
			if(pev->frags == 1670){
				sprintf( text, "Z.Z: ......\n");
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 1720){
				pPlayer->Clear_SayText();

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma_arm");
				if ( pEntity ){
				CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "arm_respawn_hit" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin - Vector(0,180,-64);
					pSpot->pev->angles.y = 90;
					pSpot->pev->angles.x = 0;
					pSpot->pev->velocity.y = 10;
					pPlayer->pev->v_angle = Vector(0,90,0);
					pPlayer->pev->angles = Vector(0,90,0);
					pPlayer->pev->fixangle = TRUE;
				}
			}
			if(pev->frags == 1830){
				CLIENT_COMMAND(pPlayer->edict(), "cd play 28\n");
				//SERVER_COMMAND("mp3 play media/music29.mp3\n");

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(512,-256,96);
					pSpot->pev->angles.y = 180;
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->avelocity = g_vecZero;
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
				}
			}
			if(pev->frags == 1900){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin = pev->origin + Vector(0,-80,64);
					pSpot->pev->angles.y = 90;
					pSpot->pev->angles.x = 0;
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->avelocity = g_vecZero;
					pSpot->pev->velocity.y = 4;

					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma_arm");
					if ( pEntity ){
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sword_aim2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
				}

				pPlayer->m_skill_reload = 1;

				pPlayer->m_kadoma_level = 99;
				pPlayer->m_kadoma_exp = 100000;//LVMAX

				pPlayer->pev->max_health = 900;
				pPlayer->m_skill_maxarmor = 300;
				pPlayer->pev->armorvalue = pPlayer->m_skill_maxarmor;
				pPlayer->pev->health = pPlayer->pev->max_health;
	
				sprintf( text, "- Kadoma Level 99!!\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- MAX HP 900 , MAX AP 300!!\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- New Skill: Law of Causality\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- Upon each death, gain:\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- 1 stack of ATK+40%, DEF+10%!\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- Max stacks: 5 (Save/Load retains)\n");
				UTIL_SayTextAll( text,this );
				
				
			}
			if(pev->frags == 2000){
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 2010){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->origin = pev->origin + Vector(0,-128,96);
					pSpot->pev->angles.y = 270;
					pSpot->pev->angles.x = -15;
					pSpot->pev->velocity.z = 10;
					pSpot->pev->velocity.y = 10;
					pSpot->pev->avelocity.x = 1;
				}
				
				sprintf( text, "Z.Z: You Awakened?\n");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 2050){
				
				sprintf( text, "Z.Z: I did not expect such power.");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 2100){
				
				sprintf( text, "Z.Z: Come! Let's end this!");
				
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 2140){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 2160){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
				pPlayer->Clear_SayText();
			}
			if(pev->frags == 2165){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma_arm");
				if ( pEntity ){
				UTIL_Remove( pEntity );
				}
				pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2");
				if ( pEntity ){
				UTIL_Remove( pEntity );
				}
			}
			if(pev->frags == 2170){
				pPlayer->EnableControl(TRUE);
				pPlayer->m_trainning = 0;
				pPlayer->pev->origin = pev->origin + Vector(0,0,36);
				pPlayer->m_stuck_origin = pPlayer->pev->origin;
				pPlayer->pev->velocity = g_vecZero;
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
				SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			}
			if(pev->frags == 2180){//�滻
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_god623_boss");
				if ( pEntity ){
				Create( "monster_god625_boss", pEntity->pev->origin, pEntity->pev->angles, NULL );
				UTIL_Remove( pEntity );
				}
			}
			if(pev->frags == 2190){
				pPlayer->BOSS_Find();
				game_boss_battle = 1;
				pPlayer->m_music_save = 22;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 22\n");
				//SERVER_COMMAND("mp3 loop media/boss10.mp3\n");
			}
			if(pev->frags == 2200){//���դε�ҩ����
				pPlayer->GiveAmmo( 64, "buckshot", BUCKSHOT_MAX_CARRY );
				pPlayer->GiveAmmo( 5, "rockets", ROCKET_MAX_CARRY );
				pPlayer->GiveAmmo( 120, "762nato", AK_MAX_CARRY );
				pPlayer->GiveAmmo( 120, "556nato", M16_MAX_CARRY );
				pPlayer->GiveAmmo( 30, "bolts", BOLT_MAX_CARRY );
				pPlayer->GiveAmmo( 5, "ARgrenades", M203_GRENADE_MAX_CARRY );
				pPlayer->GiveAmmo( 18, "357", _357_MAX_CARRY );
				pPlayer->GiveAmmo( 15, "338mag", SNIPER_MAX_CARRY );
				pPlayer->GiveAmmo( 180, "45acp", MAC_MAX_CARRY );
				pPlayer->GiveAmmo( 180, "9mm", _9MM_MAX_CARRY );
				pPlayer->GiveAmmo( 150, "762natobox", MINIGUN_MAX_CARRY );
				pPlayer->GiveAmmo( 60, "uranium", URANIUM_MAX_CARRY );
				pPlayer->GiveAmmo( 3, "nuke", 5 );
			}
			if(pev->frags == 2220){
				if(g_causality_add < 1){
				g_causality_add = 1;

				MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( "c_lv_1" );
				MESSAGE_END();
				}
			}
			if(pev->frags == 2300){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_god625_boss" );
				if ( pEntity2 ){
					if(pEntity2->pev->deadflag == DEAD_NO || pPlayer->pev->deadflag != DEAD_NO){
						pev->frags = 2280;
					}
				}
			}
			if(pev->frags == 2320){
				game_boss_battle = 0;
				pPlayer->m_music_save = 0;
				SERVER_COMMAND("mp3 stop\n");
			}
			if(pev->frags == 2340){//BOSSս����
				pPlayer->m_mode_float2 = 520;
				SERVER_COMMAND("mp3 play media/music30.mp3\n");
			}
			if(pev->frags == 2430){
				SERVER_COMMAND("autosave\n" );
			}
			if(pev->frags == 2460){//�𽥺���
				pPlayer->EnableControl(FALSE);
				pPlayer->m_trainning = 1;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 15, 255, FFADE_OUT );//��Ϲ���
				pPlayer->Game_Save_SecondData();
			}
			if(pev->frags == 2500){
				if(pPlayer->m_ending_frags <= 20){
					if(pPlayer->m_player_diamonds == 18){//ȫ�ռ�!
					g_fGameJumpCG = 12;
					}
					else{
					g_fGameJumpCG = 2;//���B-������Chaos World��
					}
					SERVER_COMMAND( "map wdoor_ending_b\n" );
				}
				else{
					if(pPlayer->m_player_diamonds == 18){//ȫ�ռ�!
					g_fGameJumpCG = 11;//���S-���ţ�Wrong Door��
					}
					else{
					g_fGameJumpCG = 1;//���A-����New God��
					}
					SERVER_COMMAND( "map wdoor_ending_a\n" );
				}
				return;//��!
			}

		}
	}
	if(pev->armortype == 85){//�¼�85 ���E-��ɱ��Victim��
		if(pev->frags == 10){
			if(g_fGameJumpCG != 15 && g_fGameJumpCG != 5 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999){
			SERVER_COMMAND("disconnect\n");
			}
		}
		if(pev->frags == 20){
			SERVER_COMMAND("mp3 play media/music2.mp3\n");
			
			sprintf( text, "- The Brave Kadoma and his team defeated Demon King Gman and the Evil Dragon Doma.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 40){
			pPlayer->EnableControl(FALSE);
			pPlayer->pev->health = 0;//��ֹ�浵!
			pPlayer->m_trainning = 1;
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.y = -40;
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_WALK_HURT );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.y = -40;
				pEntity->pev->effects = EF_BRIGHTLIGHT;
				pEnemyMonster->SetBodygroup( 2, 3 );
			}
		}
		if(pev->frags == 100){
			
			sprintf( text, "- It came at a heavy price. All of his teammates died, and Kadoma, hurt and powerless, wandered around in the dark.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 170){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->velocity.y = 0;
				pSpot->pev->angles.y = 180;
				pSpot->pev->origin.z -= 8;
				pSpot->pev->origin.x += 96;
				pSpot->pev->origin.y -= 300;
				pPlayer->pev->v_angle = Vector(0,180,0);
				pPlayer->pev->angles = Vector(0,180,0);
				pPlayer->pev->fixangle = TRUE;
			}
			
			sprintf( text, "- Was all of his work for nothing? How did it end like this?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 230){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "die_unstop_deadend" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->velocity.y = 0;
			}
			
			sprintf( text, "- He just knows it's over.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 270){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->velocity.y = 0;
					pSpot->pev->velocity.z = 6;
					pSpot->pev->avelocity.x = -2;
					pSpot->pev->angles.y = 270;
					pSpot->pev->origin = pEntity->pev->origin + Vector(0,64,32);
					pPlayer->pev->v_angle = Vector(0,270,0);
					pPlayer->pev->angles = Vector(0,270,0);
					pPlayer->pev->fixangle = TRUE;
				}
			}
		}
		if(pev->frags == 330){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->velocity.y = -10;
					pSpot->pev->velocity.z = 0;
					pSpot->pev->avelocity.x = 0;
					pSpot->pev->angles.x = 90;
					pSpot->pev->origin = pEntity->pev->origin + Vector(0,90,60);
				}
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 440){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->velocity.y = -1;
					pSpot->pev->origin = pEntity->pev->origin + Vector(12,-36,10);
				}
			}
		}
		if(pev->frags == 490){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 510){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.z = 5;
				pSpot->pev->velocity.x = 5;
			}
		}
		if(pev->frags == 540){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->armortype = 5;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 570){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "end_e_sci");
			if ( pSpot ){
				
				sprintf( text, "Scientist: It can't get any worse.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!SC_QUESTION5", VOL_NORM, 0.3, 0, 100 );
			}
		}
		if(pev->frags == 630){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "end_e_bar");
			if ( pSpot ){
				
				sprintf( text, "Guard: Oh well.\n");
				
				UTIL_SayTextAll( text,this );
				EMIT_SOUND_DYN( pSpot->edict(), CHAN_VOICE, "!BA_MAD1", VOL_NORM, 0.3, 0, 100 );
			}
		}
		if(pev->frags == 690){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 710){
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			sprintf( text, "- ENDING E: Victim 被杀");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 800){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 820 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			SERVER_COMMAND( "map wdoor_credits\n" );
			return;
		}
	}
	if(pev->armortype == 86){//�¼�86 ���D-��ɱ��Sucide��
		if(pev->frags == 10){
			if(g_fGameJumpCG != 14 && g_fGameJumpCG != 4 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999){
			SERVER_COMMAND("disconnect\n");
			}
		}
		if(pev->frags == 20){
			SERVER_COMMAND("mp3 play media/music9.mp3\n");
			
			sprintf( text, "- The Brave Kadoma and his team defeated Demon King Gman and the Evil Dragon Doma.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 40){
			pPlayer->EnableControl(FALSE);
			pPlayer->pev->health = 0;//��ֹ�浵!
			pPlayer->m_trainning = 1;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetBodygroup( 0, 3 );
			}
		}
		if(pev->frags == 90){
			
			sprintf( text, "- It came at a heavy price. All of his teammates died, and Kadoma was left powerless.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 160){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 1;
			}
		}
		if(pev->frags == 200){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "end_d_sci1");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR11", VOL_NORM, 0.3, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: I'm amazed you're still alive!\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 240){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "end_d_sci2");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR12", VOL_NORM, 0.3, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: Wow! This is inconceivable!\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 300){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "end_d_sci3");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP32", VOL_NORM, 0.3, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: I'm saved!\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 360){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "end_d_bar");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_POK2", VOL_NORM, 0.3, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Barney: Come find me later. I'll buy you a beer.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 440){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 15, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 480){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 1;
			}
		}
		if(pev->frags == 520){
			
			sprintf( text, "- His power gone, Kadoma lived a simple life, but this didn't last for long.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 600){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma_cycler");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sit_end2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			}
			
			sprintf( text, "- He could not stand losing all his power, and seeing all his efforts come to naught.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 640){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma_cycler");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sit_end2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 700){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.y = -12;
				pSpot->pev->avelocity.x = 6;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "idle2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				SetBits( pEntity->pev->effects, EF_DIMLIGHT);
				pEnemyMonster->SetBodygroup( 1, 2 );
			}
		}
		if(pev->frags == 780){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 8;
			}
		}
		if(pev->frags == 820){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->origin = pEntity->pev->origin + Vector(128,-128,72);
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 150;
				}
			}
		}
		if(pev->frags == 850){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sucide_jump" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 900){
			
			sprintf( text, "- He killed himself.\n");
			
			UTIL_SayTextAll( text,this );
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4, 15, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 950){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 970){
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			sprintf( text, "- ENDING D: Sucide 自杀");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1050){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1070 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			SERVER_COMMAND( "map wdoor_credits\n" );
			return;
		}
	}
	if(pev->armortype == 87){//�¼�87 ���C-������Restart��
		if(pev->frags == 10){
			if(g_fGameJumpCG != 13 && g_fGameJumpCG != 3 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999){
			SERVER_COMMAND("disconnect\n");
			}
		}
		if(pev->frags == 40){
			pPlayer->EnableControl(FALSE);
			pPlayer->pev->health = 0;//��ֹ�浵!
			pPlayer->m_trainning = 1;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetBodygroup( 0, 3 );
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 70){
			CLIENT_COMMAND(pPlayer->edict(), "cd play 2\n");
			//SERVER_COMMAND("mp3 play media/music1.mp3\n");
		}
		if(pev->frags == 90){
			
			sprintf( text, "- Kadoma went back in time, to where things began.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 160){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = -256;
			}
		}
		if(pev->frags == 200){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = 0;
			}
			
			sprintf( text, "- Things seem different here. Is he in an alternate universe?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 240){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_RUN_SCARED );
				pEntity->pev->movetype = MOVETYPE_NOCLIP;
				pEntity->pev->velocity.x = -320;
			}
		}
		if(pev->frags == 270){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->velocity.x = 0;
				pEntity->pev->angles.y = 90;
			}
			
			sprintf( text, "- In any case, there's only one thing Kadoma can do.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 320){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_CROUCHIDLE );
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = -10;
				pSpot->pev->velocity.z = -10;
				pSpot->pev->armortype = 5;
				pSpot->pev->frags = 1;
			}
			
			sprintf( text, "- Show Gman loyalty!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 380){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 10;
				pSpot->pev->velocity.z = -10;
				pSpot->pev->armortype = 5;
				pSpot->pev->frags = 1;
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 420){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "cycler_gman");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "bigyes2" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 480){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 500){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 90;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
			CBaseMonster *pEnemyMonster;
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_WALK );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = 90;
				pEntity->pev->effects = EF_BRIGHTLIGHT;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor" );
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_WALK );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = 90;
				pEntity->pev->body = 2;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_WALK );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = 90;
				pEntity->pev->body = 0;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_WALK );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = 90;
				pEnemyMonster->SetBodygroup( 0, 3 );
			}
			pEntity = UTIL_FindEntityByTargetname( NULL, "shape_generic");
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_WALK );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = 90;
			}
		}
		if(pev->frags == 600){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pSpot->pev->velocity.x = -60;
				pSpot->pev->velocity.z = 10;
				pSpot->pev->angles.y = 0;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_gman" );
			CBaseMonster *pEnemyMonster;
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->velocity.x = 0;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_dengor" );
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->velocity.x = 0;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_doma" );
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->velocity.x = 0;
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->velocity.x = 0;
			}
			pEntity = UTIL_FindEntityByTargetname( NULL, "shape_generic");
			if ( pEntity ){
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->velocity.x = 0;
			}
		}
		if(pev->frags == 623){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "god623_cycler");
			if ( pEntity ){
			pEntity->pev->effects = EF_BRIGHTLIGHT;
			pEntity->pev->movetype = MOVETYPE_FLY;
			pEntity->pev->velocity.z = -30;
			pEntity->pev->velocity.x = 30;
			}
		}
		if(pev->frags == 640){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->armortype = 5;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 650){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "god623_cycler");
			if ( pEntity ){
			pEntity->pev->velocity.x = 0;
			pEntity->pev->velocity.z = 0;
			}
		}
		if(pev->frags == 670){
			
			sprintf( text, "- A great fight is at hand!\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 710){
			
			sprintf( text, "- Can Gman and Kadoma win this time?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 760){
			
			sprintf( text, "- Who knows?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 800){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "god623_cycler");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->SetActivity( ACT_MELEE_ATTACK1 );
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 830){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 850){
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			sprintf( text, "- ENDING C: Restart");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 930){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 950 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			SERVER_COMMAND( "map wdoor_credits\n" );
			return;
		}
	}
	if(pev->armortype == 88){//�¼�88 ���B-������Chaos World��
		if(pev->frags == 10){
			if(g_fGameJumpCG != 12 && g_fGameJumpCG != 2 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999){
			SERVER_COMMAND("disconnect\n");
			}
		}
		if(pev->frags == 40){
			pPlayer->EnableControl(FALSE);
			pPlayer->pev->health = 0;//��ֹ�浵!
			pPlayer->m_trainning = 1;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEntity->pev->effects = EF_BRIGHTLIGHT;
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->angles.x = 90;
				pSpot->pev->avelocity.y = 16;
				pSpot->pev->velocity.z = -96;
			}
			CLIENT_COMMAND(pPlayer->edict(), "cd play 16\n");
			//SERVER_COMMAND("mp3 play media/music31.mp3\n");
		}
		if(pev->frags == 90){
			
			sprintf( text, "- Kadoma defeated Creator God Z.Z, and gained the power to destroy the world.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 120){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->armortype = 12;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 170){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 270;
					pSpot->pev->avelocity.y = 0;
					pSpot->pev->velocity.z = 0;
					pSpot->pev->origin = pEntity->pev->origin + Vector(0,64,64);

					pPlayer->pev->v_angle = Vector(0,270,0);
					pPlayer->pev->angles = Vector(0,270,0);
					pPlayer->pev->fixangle = TRUE;
				}
			}
			
			sprintf( text, "- However, he didn't not gain restorative powers, and his team remained dead. Kadoma couldn't help anyone.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 250){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "ending_hensing" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 280){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				SET_MODEL(ENT(pEntity->pev), "models/kadoma_ending.mdl");
				pEntity->pev->body = 0;
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "hensing_overidle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				FX_Explosion( pEntity->Center(), EXPLOSION_HEVCHARGER);
			}
		}
		if(pev->frags == 350){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->angles.x = 30;
					pSpot->pev->angles.y = 135;

					pPlayer->pev->v_angle = Vector(0,135,0);
					pPlayer->pev->angles = Vector(0,135,0);
					pPlayer->pev->fixangle = TRUE;

					pSpot->pev->frags = 0;
					pSpot->pev->velocity.x = 64;
					pSpot->pev->velocity.y = -64;
					pSpot->pev->velocity.z = 64;

					pSpot->pev->origin = pEntity->pev->origin + Vector(256,-256,256);
				}
			}
		}
		if(pev->frags == 370){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "sword_swing" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 400){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->frags = 1;
			}
			FireTargets( "xen_break_wall", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 440){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 15.0, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 480){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pPlayer->pev->v_angle = Vector(0,270,0);
				pPlayer->pev->angles = Vector(0,270,0);
				pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 530){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_IDLE7", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Guard: This isn't good.\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 580){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_sci");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_PQUEST11", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: No moral compunctions, then?\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 630){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP19", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Guard: What do you think?\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 700){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "relaxstand" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 90;
				UTIL_SetOrigin( pEntity->pev, pev->origin );
				FX_Explosion(pev->origin+Vector(0,0,48), EXPLOSION_DISPTELEPORT );
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 720){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_sci");
			if ( pEntity ){
				pEntity->pev->angles.y = 270;
			}
			pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				pEntity->pev->angles.y = 270;
			}
		}
		if(pev->frags == 750){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_sci");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP34", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: I have to go!\n");
				
				UTIL_SayTextAll( text,this );
				FireTargets( "rape_sci_scard_run", this, this, USE_TOGGLE, 0 );
			}
		}
		if(pev->frags == 790){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEntity->pev->angles.y = 0;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-775,85,0) );
				FX_Explosion( pev->origin+Vector(0,0,48), EXPLOSION_SPARKSHOWER );
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = -360;
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 810){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->velocity.x = 0;
			}
		}
		if(pev->frags == 820){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_sci");
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "fear2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;

				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_FEAR3", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: No, let me live!\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 860){
			pPlayer->Clear_SayText();
			FireTargets( "field_glass_boy", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 880){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_SCARED0", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Guard: Hey! Stop!\n");
				
				UTIL_SayTextAll( text,this );
			}
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 4.0, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 900){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_sci");
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "befuck" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 0;
				
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity = g_vecZero;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(0,320,4) );

				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEntity2->pev->angles.y = 0;
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "rape" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin - Vector(16,0,0) );
				}
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->origin.x = pEntity->pev->origin.x + 32;
					pSpot->pev->origin.y = pEntity->pev->origin.y - 16;
					pSpot->pev->origin.z = pEntity->pev->origin.z + 64;
					pSpot->pev->avelocity = g_vecZero;
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->angles.x = 0;
					pSpot->pev->angles.y = 160;
				}
			}
		}
		if(pev->frags == 920){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 930 || pev->frags == 950 || pev->frags == 970 || pev->frags == 990){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_sci");
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->PainSound();
			}
		}
		if(pev->frags == 1010){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
			pEntity->pev->angles.y = 270;
			UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(-96,0,0) );
				CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "glass_take_body");
				if ( pEntity2 ){
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(-96,-96,0) );
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "glass_take_body2");
				if ( pEntity2 ){
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(-32,-96,0) );
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "glass_take_body3");
				if ( pEntity2 ){
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(32,-96,0) );
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "glass_take_body4");
				if ( pEntity2 ){
				UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin + Vector(96,-96,0) );
				}
			}
		}
		if(pev->frags == 1030){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					pSpot->pev->origin.x = pEntity->pev->origin.x;
					pSpot->pev->origin.y = pEntity->pev->origin.y + 128;
					pSpot->pev->origin.z = pEntity->pev->origin.z + 128;
					pSpot->pev->avelocity = g_vecZero;
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->angles.x = 15;
					pSpot->pev->angles.y = 270;
				}
			}
		}
		if(pev->frags == 1060){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_POK1", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Guard: Later, maybe?\n");
				
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1120){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "rape_bar");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_TR9", VOL_NORM, 0.1, 0, PITCH_NORM );
				pPlayer->Clear_SayText();
			}
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 4.0, 255, FFADE_IN );//��Ϲ���
		}
		if(pev->frags == 1140){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma_evil_tgsit");
			if ( pEntity ){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEntity2->pev->angles.y = 270;
					pEntity2->pev->movetype = MOVETYPE_FLY;
					pEntity2->pev->velocity = g_vecZero;
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "endsit_evil" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					UTIL_SetOrigin( pEntity2->pev, pEntity->pev->origin);
				}
				UTIL_Remove( pEntity );
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.y = 10;
			}
		}
		if(pev->frags == 1220){
			
			sprintf( text, "- Kadoma rules the world. Everyone shall kneel!");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1320){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->velocity.y = 100;
				pSpot->pev->velocity.z = 6;
			}
		}
		if(pev->frags == 1360){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 5;
			}
		}
		if(pev->frags == 1410){
			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity2 ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity2->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "endsit_evil_atk" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 1440){
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			sprintf( text, "- ENDING B: Chaos World 混世");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1520){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1540 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			SERVER_COMMAND( "map wdoor_credits\n" );
			return;
		}
	}
	if(pev->armortype == 89){//�¼�89 ���A-����New God��
		if(pev->frags == 10){
			if(g_fGameJumpCG != 11 && g_fGameJumpCG != 1 && CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999){
			SERVER_COMMAND("disconnect\n");
			}
		}
		if(pev->frags == 40){
			pPlayer->EnableControl(FALSE);
			pPlayer->pev->health = 0;//��ֹ�浵!
			pPlayer->m_trainning = 1;
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEntity->pev->effects = EF_DIMLIGHT;
			}
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "god_cshl623");
			if ( pSpot ){
				pSpot->pev->effects = EF_DIMLIGHT;
			}
			pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 60){
			CLIENT_COMMAND(pPlayer->edict(), "cd play 26\n");
			//SERVER_COMMAND("mp3 play media/music27.mp3\n");
		}
		if(pev->frags == 100){
			
			sprintf( text, "Z.Z: Good job, Kadoma.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 150){
			
			sprintf( text, "Z.Z: To think that a man as strong as you can outdo me...\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 230){
			
			sprintf( text, "Z.Z: The world is yours. You will know its true nature.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 320){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "god_cshl623");
			if ( pSpot ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pSpot->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_SLEEP );
			}
			pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				pSpot->pev->velocity.x = 10;
				pSpot->pev->velocity.y = 10;
			}
		}
		if(pev->frags == 350){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "god_cshl623");
			if ( pSpot ){
			UTIL_Remove( pSpot );
			}
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 3, 3, 255, FFADE_OUT );
		}
		if(pev->frags == 390){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_IN );
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = -10;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetBodygroup( 0, 3 );
				pEntity->pev->velocity.z = 10;
			}
		}
		if(pev->frags == 440){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "headache" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->velocity.x = -30;
				pEntity->pev->avelocity.z = 30;
			}
		}
		if(pev->frags == 470){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "ending_hensing" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(128,32,0) );
			}
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 3, 3, 255, FFADE_OUT );
			
			sprintf( text, "- Kadoma understands it all. This is a bad game world; all is chaos and nothingness\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 500){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin = pEntity->pev->origin + Vector(64,0,64);
					pSpot->pev->angles.y = 180;
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
					pSpot->pev->velocity = g_vecZero;
				}
			}
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_IN );
		}
		if(pev->frags == 570){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				SET_MODEL(ENT(pEntity->pev), "models/kadoma_ending.mdl");
				pEnemyMonster->SetBodygroup( 0, 1 );
				pEnemyMonster->SetBodygroup( 1, 1 );
				pEntity->pev->impulse = 99;
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "hensing_overidle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				FX_Explosion( pEntity->Center(), 127);
				EMIT_SOUND_DYN ( ENT(pEntity->pev), CHAN_STREAM, "newadd/exp2_frost.wav", 1.0, 0.5, 0, 100);
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 620){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "i_need_more_power" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.3, 0.3, 255, FFADE_IN );//��Ϲ���
			}
		}
		if(pev->frags == 720){
			FireTargets( "moon_glass_wall", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 760){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "god_seq1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 1, 1 );
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.3, 0.3, 255, FFADE_IN );//��Ϲ���
				pEntity->pev->movetype = MOVETYPE_NOCLIP;
				pEntity->pev->velocity.z = 10;

				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					SET_VIEW( pPlayer->edict(), pSpot->edict() );
					pPlayer->m_player_camera = pSpot;
					pSpot->pev->origin = pEntity->pev->origin + Vector(64,0,64);
					pSpot->pev->angles.y = 180;
					pPlayer->pev->v_angle = Vector(0,180,0);
					pPlayer->pev->angles = Vector(0,180,0);
					pPlayer->pev->fixangle = TRUE;
					pSpot->pev->velocity = pEntity->pev->velocity;
					pSpot->pev->velocity.x = 2;
				}
			}
		}
		if(pev->frags == 800){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "god_seq2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 890){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->angles.x = 10;
				pPlayer->pev->v_angle = pSpot->pev->angles;
				pPlayer->pev->angles = pSpot->pev->angles;
				pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 900){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "kadoma_recover_ball");
			if ( pSpot ){
				SET_MODEL(ENT(pSpot->pev), "models/camera_rocket.mdl");
				FX_Trail(pSpot->pev->origin, pSpot->entindex(), PROJ_SUNOFGOD2);
				pSpot->pev->movetype = MOVETYPE_FLY;
			}
		}
		if(pev->frags == 920){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "kadoma_recover_ball");
			if ( pSpot ){
				pSpot->pev->effects |= EF_LIGHT;
				pSpot->pev->velocity.z = -120;
			}
		}
		if(pev->frags == 950){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "kadoma_recover_ball");
			if ( pSpot ){
				FX_Trail(pSpot->pev->origin, pSpot->entindex(), PROJ_REMOVE);
				UTIL_Remove( pSpot );
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_LARGEFUNNEL );
				WRITE_COORD( pSpot->pev->origin.x );
				WRITE_COORD( pSpot->pev->origin.y );
				WRITE_COORD( pSpot->pev->origin.z );
				WRITE_SHORT( g_sModelIndexFlareGlow );
				WRITE_SHORT( 1 );
				MESSAGE_END();

				CBaseEntity *pEntity = NULL;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pSpot->pev->origin, 1536 )) != NULL)
				{
						if (  (pEntity->pev->flags & FL_MONSTER) ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->SetActivity( ACT_IDLE );
						FX_Explosion(pEnemyMonster->Center(), 45 );
						}
				}
			}
		}
		if(pev->frags == 970){
			FireTargets( "heal_glass_wall", this, this, USE_TOGGLE, 0 );
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->avelocity.x = -2;
					pSpot->pev->velocity.z = 10;
				}
		}
		if(pev->frags == 1010){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 3, 3, 255, FFADE_OUT );
			
			sprintf( text, "- Kadoma resurrected his team, and instituted a new world order.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1040){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 1, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pPlayer->pev->v_angle = pSpot->pev->angles;
				pPlayer->pev->angles = pSpot->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pSpot->pev->velocity.y = 10;
			}
		}
		if(pev->frags == 1050){
			
			sprintf( text, "- Resources are infinite, life is eternal. No new life is born.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1070){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 5;
			}
		}
		if(pev->frags == 1130){
			
			sprintf( text, "- Money is worthless. Violence and sex are forbidden.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1210){
			
			sprintf( text, "- Many do not understand nor accept this new world.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1300){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1070 || pev->frags == 1130 || pev->frags == 1190 || pev->frags == 1250){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "multi_get_sci" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "push_button2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 1080 || pev->frags == 1140 || pev->frags == 1200 || pev->frags == 1260){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dmrp_battery" );
			if ( pEntity ){
				pEntity->pev->effects = EF_NODRAW;
				EMIT_SOUND( pEntity->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, 0.5 );
				FX_Explosion(pEntity->pev->origin + Vector(0,0,8), EXPLOSION_SHIELDIMPACT );
			}
		}
		if(pev->frags == 1110 || pev->frags == 1170 || pev->frags == 1230 || pev->frags == 1290){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dmrp_battery" );
			if ( pEntity ){
				pEntity->pev->effects = EF_MUZZLEFLASH;
				EMIT_SOUND_DYN( ENT(pEntity->pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
			}
		}
		if(pev->frags == 1320){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "multi_get_sci");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_ZP20", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: I really miss the 2D world...\n");
			
				UTIL_SayTextAll( text,this );
			}
		}
		if(pev->frags == 1380){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "multi_get_sci");
			if ( pEntity ){
				EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_PLFEAR4", VOL_NORM, 0.1, 0, PITCH_NORM );
				char text[256];
				
				sprintf( text, "Scientist: I have to go.\n");
				
				UTIL_SayTextAll( text,this );

				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "wave" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 270;
			}
		}
		if(pev->frags == 1420){
			pPlayer->Clear_SayText();
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "multi_get_sci");
			if ( pEntity ){
			FX_Explosion(pEntity->Center(), EXPLOSION_DISPTELEPORT );
			UTIL_Remove( pEntity );
			}
			pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma_god_tgsit");
			if ( pEntity ){
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->SetBodygroup( 0, 1 );
			pEnemyMonster->SetBodygroup( 1, 1 );
			}
		}
		if(pev->frags == 1450){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.y = 10;
			}
			
			sprintf( text, "- As God, Kadoma fulfilled people's wishes as best he could.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1490){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				pSpot->pev->velocity.y = 130;
				pSpot->pev->velocity.z = -25;
			}
		}
		if(pev->frags == 1520){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				pSpot->pev->armortype = 5;
				pSpot->pev->frags = 1;
			}
		}
		if(pev->frags == 1580){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = -30;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "vanlve_master");
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 0, 1 );
				pEnemyMonster->SetBodygroup( 1, 1 );
				pEnemyMonster->SetBodygroup( 2, 1 );
			}
		}
		if(pev->frags == 1660){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.3, 0.3, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera7");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 1720){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.3, 0.3, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera8");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 1780){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.3, 0.3, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera9");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = -500;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "dragon_runner");
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetActivity( ACT_RUN );
				pEntity->pev->effects = EF_BRIGHTLIGHT;
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = -500;
			}
		}
		if(pev->frags == 1840){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "wdoor_cam10_tg");
			if ( pEntity ){
				SET_MODEL(ENT(pEntity->pev), "models/camera_rocket.mdl");
				SET_VIEW( pPlayer->edict(), pEntity->edict() );
				pPlayer->m_player_camera = pEntity;
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = -10;
			}
		}
		if(pev->frags == 1910){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "wdoor_cam10_tg");
			if ( pSpot ){
				pSpot->pev->velocity.x = 0;
				pSpot->pev->avelocity.y = -30;
				pSpot->pev->velocity.z = 160;
			}
			pSpot = UTIL_FindEntityByTargetname( NULL, "kadoma_fly_bao_misaliya");
			if ( pSpot ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pSpot->MyMonsterPointer();
				pEnemyMonster->SetBodygroup( 0, 1 );
				pEnemyMonster->SetBodygroup( 1, 1 );
				pSpot->pev->movetype = MOVETYPE_NOCLIP;
				pSpot->pev->velocity.x = 40;
				pSpot->pev->effects |= EF_LIGHT;
			}
			pSpot = UTIL_FindEntityByTargetname( NULL, "misaliya_fly_bao");
			if ( pSpot ){
				pSpot->pev->movetype = MOVETYPE_NOCLIP;
				pSpot->pev->velocity.x = 40;
			}
		}
		if(pev->frags == 1940){
			CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "wdoor_cam10_tg");
			if ( pSpot ){
				pSpot->pev->avelocity.y = 0;
				pSpot->pev->velocity.z = 0;
				pPlayer->pev->v_angle = pSpot->pev->angles;
				pPlayer->pev->angles = pSpot->pev->angles;
				pPlayer->pev->fixangle = TRUE;
			}
		}
		if(pev->frags == 1980){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "wdoor_cam10_tg");
			if ( pEntity ){
				CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "misaliya_fly_bao");
				if ( pEntity2 ){
				pEntity2->pev->velocity.x = 400;
				}
				pEntity2 = UTIL_FindEntityByTargetname( NULL, "kadoma_fly_bao_misaliya");
				if ( pEntity2 ){
				pEntity->pev->velocity.x = 400;
				pEntity2->pev->velocity.x = 400;
				pEntity->pev->origin = pEntity2->pev->origin + Vector(0,-128,64);
				}
			}
		}
		if(pev->frags == 2020){
			CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "misaliya_fly_bao");
			if ( pEntity2 ){
			pEntity2->pev->velocity.x = 500;
			pEntity2->pev->velocity.z = -125;
			}
			pEntity2 = UTIL_FindEntityByTargetname( NULL, "kadoma_fly_bao_misaliya");
			if ( pEntity2 ){
			pEntity2->pev->velocity.x = 500;
			pEntity2->pev->velocity.z = -125;
			}
		}
		if(pev->frags == 2047){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma_misaliya_end");
			if ( pEntity ){
				CBaseEntity *pSpot = UTIL_FindEntityByTargetname( NULL, "kadoma_fly_bao_misaliya");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->SetActivity( ACT_IDLE );
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->effects  = EF_DIMLIGHT;
					pSpot->pev->angles.y = 90;
					UTIL_SetOrigin( pSpot->pev, pEntity->pev->origin + Vector(-48,32,0));
				}
				pSpot = UTIL_FindEntityByTargetname( NULL, "misaliya_fly_bao");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->SetActivity( ACT_IDLE );
					pSpot->pev->velocity = g_vecZero;
					pSpot->pev->effects  = EF_DIMLIGHT;
					pSpot->pev->angles.y = 90;
					UTIL_SetOrigin( pSpot->pev, pEntity->pev->origin + Vector(16,32,0));
				}
			}
		}
		if(pev->frags == 2050){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma_misaliya_end");
			if ( pEntity ){
				CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "wdoor_cam10_tg");
				if ( pEntity2 ){
					pEntity2->pev->origin = pEntity->pev->origin + Vector(-16,-320,160);
					pEntity2->pev->velocity.y = 10;
					pEntity2->pev->velocity.x = 0;
				}
			}
		}
		if(pev->frags == 2100){
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "wdoor_cam10_tg");
			if ( pEntity ){
				pEntity->pev->velocity.y = 192;
				pEntity->pev->velocity.z = 2;
			}
		}
		if(pev->frags == 2150){
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			sprintf( text, "- ENDING A: New God 新神�");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 2230){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 2250 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			SERVER_COMMAND( "map wdoor_credits\n" );
			return;
		}
	}
	if(pev->armortype == 90){//�¼�90 ������
		if(pev->frags == 20){
			pPlayer->pev->flags |= FL_FROZEN;
			pPlayer->m_trainning = 1;
			pPlayer->pev->health = 0;//��ֹ�浵!
		}
		if(pev->frags == 30){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
			}
		}
		if(pev->frags == 50){
			//SERVER_COMMAND("mp3 play media/door.mp3\n");
			CLIENT_COMMAND(pPlayer->edict(), "cd play 25\n");
		}
		if(pev->frags == 120){
			FireTargets( "endcredits_roll", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 2100){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 6.0, 15.0, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 2180 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			if(g_fGameJumpCG == 11){//�������ؽ��S+��������
			SERVER_COMMAND( "map wdoor_ending_s\n" );
			}
			else if(g_fGameJumpCG >= 12){//������������
			SERVER_COMMAND( "map wdoor_bonus_level\n" );
			}
			else {//�ص��������
			SERVER_COMMAND("disconnect\n");
			}
			return;
		}
	}
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
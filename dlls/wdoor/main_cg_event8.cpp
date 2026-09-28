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
// ��CG�¼���ͳ��8��Bug Fix 3.0 ��ȫ�������ݣ�
//=========================================================
class CMain_Event8 : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	CBasePlayer *pPlayer;
	char text[256];
};

LINK_ENTITY_TO_CLASS( main_cg_event_new8, CMain_Event8 );//����8

void CMain_Event8::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate		= 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");

	SetThink (&CMain_Event8::killThink_new);

	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��=================================================//
void CMain_Event8::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}
	
	if(pev->armortype == 91){//�¼�91 ���S-���ţ�Wrong Door��
		if(pev->frags == 10){
			if(g_fGameJumpCG != 11){
			SERVER_COMMAND("disconnect\n");
			return;
			}

			pPlayer->pev->health = 0;//��ֹ�浵!
			pPlayer->m_trainning = 1;

			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				SET_MODEL(ENT(pEntity->pev), "models/kadoma_ending.mdl");
				pEnemyMonster->SetBodygroup( 0, 1 );
				pEnemyMonster->SetBodygroup( 1, 1 );
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEntity->pev->effects = EF_DIMLIGHT;
			}

			pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->SetActivity( ACT_IDLE );
			}

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 20){
			pPlayer->EnableControl(FALSE);
		}
		if(pev->frags == 60){
		
			sprintf( text, "- Kadoma and Misaliya came to a strange room.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 120){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
				pSpot->pev->velocity.x = 64;
			}
		}
		if(pev->frags == 160){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 14;
			}
			SERVER_COMMAND("mp3 play media/music_x1.mp3\n");
		}
		if(pev->frags == 240){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEntity->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(-16,0,0));
			}

			pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEntity->pev->angles.y = 180;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin + Vector(160,-32,0));
			}
		}
		if(pev->frags == 250){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 255){
			
			sprintf( text, "- Kadoma felt that he had entered the wrong door.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 300){
			
			sprintf( text, "- Maybe it's because of the wrong way of opening it.\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 340){
			
			sprintf( text, "- Anyway, he wanted to leave this place.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 390){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->angles.y = 90;
				pSpot->pev->velocity.x = -48;
			}
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 410){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->frags = 1;
				pSpot->pev->armortype = 14;
				pPlayer->pev->v_angle = pSpot->pev->angles;
				pPlayer->pev->angles = pSpot->pev->angles;
				pPlayer->pev->fixangle = TRUE;
			}

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq1" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 440){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->velocity.x = -64;
				pSpot->pev->velocity.y = 64;
			}
			
			sprintf( text, "- But no matter what he did, it was useless.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 480){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->velocity.x = 0;
				pSpot->pev->velocity.y = 0;
			}
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "hensing_overidle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 270;
				pEntity->pev->effects = 0;
				FX_Explosion(pEntity->pev->origin + Vector(0,0,48), EXPLOSION_DISPTELEPORT );
			}
			
			sprintf( text, "- Whether it was time rewinding or spatial teleportation, none of them worked.\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 560){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 570){
			sprintf( text, "Misaliya: Kadoma.\n");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 600){
			pPlayer->Clear_SayText();

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "deep_idle" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->skin = 3;
			}
		}
		if(pev->frags == 610){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					pSpot->pev->angles.y = 0;
					pSpot->pev->origin = pEntity->pev->origin + Vector(-48,0,64);
				}
			}

			sprintf( text, "Misaliya: Let's do it?\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 620){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				pEntity->pev->angles.y = 0;
				UTIL_SetOrigin( pEntity->pev, pev->origin + Vector(96,0,0));
			}
		}
		if(pev->frags == 650){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					pSpot->pev->angles.y = 180;
					pSpot->pev->origin = pEntity->pev->origin + Vector(64,0,72);
				}
			}
		}
		if(pev->frags == 660){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "agree" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 690){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity ){
					pSpot->pev->angles.y = 0;
					pSpot->pev->origin = pEntity->pev->origin + Vector(-48,0,48);
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
				}
			}
		}
		if(pev->frags == 700){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				pEntity->pev->skin = 5;
			}

			sprintf( text, "Misaliya: Take me on!\n");
		
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 740){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
					pSpot->pev->angles.y = 180;
					pSpot->pev->origin = pEntity->pev->origin + Vector(64,0,72);
					pPlayer->pev->v_angle = pSpot->pev->angles;
					pPlayer->pev->angles = pSpot->pev->angles;
					pPlayer->pev->fixangle = TRUE;
				}
			}
		}
		if(pev->frags == 750){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq7" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 800){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
				if ( pEntity2 ){
					pEntity2->pev->skin = 0;
					UTIL_SetOrigin( pEntity->pev, pEntity2->pev->origin + Vector(-36,0,0));

					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->origin = pEntity2->pev->origin + Vector(-192,-96,64);
						pSpot->pev->angles.y = 90;
						pSpot->pev->velocity.x = 64;
						pSpot->pev->armortype = 0;

						pPlayer->pev->v_angle = pSpot->pev->angles;
						pPlayer->pev->angles = pSpot->pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
				}
			}
		}
		if(pev->frags == 820){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->armortype = 14;
				}
		}
		if(pev->frags == 830){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq2" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 860){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				FX_Explosion( pEntity->Center(), EXPLOSION_SPARKSHOWER );
				EMIT_SOUND(ENT(pEntity->pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;

				//����ε���
				CBaseEntity *pEntity2 = Create( "monster_generic_item2", pEntity->pev->origin + Vector(0,0,64), Vector(0,90,0), NULL );
				SET_MODEL(ENT(pEntity2->pev), "models/props_all.mdl");
				pEntity2->pev->body = 11;
				pEntity2->pev->velocity.x = 50;
				pEntity2->pev->velocity.z = 70;
				pEnemyMonster->SetBodygroup( 0, 2 );

				UTIL_SetOrigin( pEntity->pev, Vector(-12,40,112));
			}

			pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq3" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->velocity = g_vecZero;
				pEntity->pev->skin = 9;
				UTIL_SetOrigin( pEntity->pev, Vector(3,42,112));
			}

			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
			if ( pSpot ){
				pSpot->pev->armortype = 0;
				pSpot->pev->velocity.x = 128;
			}
		}
		if(pev->frags == 890){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
				if ( pSpot ){
					pSpot->pev->armortype = 14;
				}
		}
		if(pev->frags == 940){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->origin.x += 5;
				pSpot->pev->origin.y += 5;
				pSpot->pev->origin.z -= 5;
				pSpot->pev->angles.x = 50;
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 960){
			
			sprintf( text, "Misaliya: I still prefer the way you are now.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1010){
			pPlayer->Clear_SayText();
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq4" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = -10;
				//Ϊ�˲���Misaliya������˸ɷ�ѿ��Ķ�ε��ԣ��Ѿ��Լ������ˣ��ٳ���˸Ҳû�취
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->avelocity.x = 4;
				pSpot->pev->velocity.x = 9;
				pSpot->pev->velocity.y = 5;
				pSpot->pev->velocity.z = -4;
			}
		}
		if(pev->frags == 1020){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq4" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 1030){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
			if ( pSpot ){
				pSpot->pev->avelocity.y = -10;
			}
		}
		if(pev->frags == 1050){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );
			
			sprintf( text, "- Later\n");
			
			UTIL_SayTextAll( text,this );
			FireTargets( "room_light", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1075){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq5" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEntity->pev->angles.y = 270;
				pEntity->pev->effects = EF_DIMLIGHT;
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity = g_vecZero;
				UTIL_SetOrigin( pEntity->pev, Vector(67,-11,112));
			}
			pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq5" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				pEnemyMonster->SetBodygroup( 3, 2 );
				pEntity->pev->velocity = g_vecZero;
				pEntity->pev->skin = 10;
				pEntity->pev->effects = EF_DIMLIGHT;
				UTIL_SetOrigin( pEntity->pev, Vector(96,26,128));
			}
		}
		if(pev->frags == 1080){
			pPlayer->Clear_SayText();
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera5");
			if ( pSpot ){
				pSpot->pev->angles.x = 85;
				pSpot->pev->origin.z -= 3;
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 1140){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pSpot->pev->angles.x = 30;
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 1160){
			
			sprintf( text, "- Kadoma gave up the power of the gods and regained his humanity.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1220){
			
			sprintf( text, "- Even gods have things they can't do. Perhaps this is the best ending.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1290){
			FireTargets( "room_light", this, this, USE_TOGGLE, 0 );
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1300){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq6" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 1325){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_misaliya" );
			if ( pEntity ){
				pEntity->pev->skin = 9;
			}
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pSpot->pev->velocity.x = -4;
				pSpot->pev->velocity.y = 4;
				pSpot->pev->velocity.z = -2;
				pSpot->pev->avelocity.x = -3;
			}
		}
		if(pev->frags == 1330){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "s_ending_seq6" );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
			}
		}
		if(pev->frags == 1370){
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera6");
			if ( pSpot ){
				pSpot->pev->angles.x = 0;
				pSpot->pev->angles.y = 180;
				pSpot->pev->origin = pev->origin + Vector(192,0,64);
				pSpot->pev->velocity.x = -8;
				pSpot->pev->avelocity.x = 0;
				pSpot->pev->velocity.y = 0;
				pSpot->pev->velocity.z = 0;
				SET_VIEW( pPlayer->edict(), pSpot->edict() );
				pPlayer->m_player_camera = pSpot;
			}
		}
		if(pev->frags == 1390){
			FireTargets( "wrong_door_x", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1400){
			
			sprintf( text, "- Live a happy life ever after.\n");
			
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1420){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 1440){
			UTIL_ScreenFade( pPlayer, Vector(255,255,255), 2, 2, 255, FFADE_IN );//��Ϲ���
			SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			pPlayer->m_player_camera = NULL;
		}
		if(pev->frags == 1460){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1480){
			sprintf( text, "- ENDING S: Wrong Door 错门");
			UTIL_SayTextAll( text,this );
		}
		if(pev->frags == 1560){
			pPlayer->Clear_SayText();
		}
		if(pev->frags == 1580 || g_fGameSkipCG == 1){
			g_fGameSkipCG = 0;
			SERVER_COMMAND( "map wdoor_bonus_level\n" );
			return;
		}
	}
	if(pev->armortype == 92){//�¼�92 ��������
		if(pev->frags == 20){
			
			sprintf( text, "- Bonus Level\n");
			
			UTIL_SayTextAll( text,this );

			pPlayer->m_trainning = 2;
			pPlayer->m_fequip2 = TRUE;
			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "mario", "1" );

			pPlayer->m_fNextClearTextTime = gpGlobals->time + 5.0;

			SERVER_COMMAND( "autosave\n" );

			UTIL_Remove( this );
			return;
		}
	}
	if(pev->armortype == 93){//�¼�93 BOSS RUSH
		if(pev->frags == 30){
			pPlayer->MenuItem_add(15);//����Get
			pPlayer->m_fequip1 = TRUE;
			pPlayer->MenuItem_add(16);//����ѥGet
			pPlayer->m_fequip2 = TRUE;
			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "mario", "1" );
			pPlayer->MenuItem_add(18);//����Get
			pPlayer->m_fequip4 = TRUE;
			pPlayer->m_air_oxyan_max = 2500;
			pPlayer->m_air_oxyan = pPlayer->m_air_oxyan_max;
			pPlayer->MenuItem_add(19);//��֮����Get
			pPlayer->m_fequip5 = TRUE;
			pPlayer->MenuItem_add(20);//����ˮ��Get
			pPlayer->m_fequip6 = TRUE;

			//pPlayer->GiveNamedItem( "item_respawn" );
			//pPlayer->GiveNamedItem( "item_godwater" );

			pPlayer->m_skill_reload = 1;
			pPlayer->m_skill_defguard = 2;
			pPlayer->m_skill_longjump = 3;
			pPlayer->m_skill_punch = 4;
			pPlayer->m_skill_valvesword = 5;
			pPlayer->m_skill_respawn = 6;
			pPlayer->m_skill_darkhide = 7;
			pPlayer->m_skill_deathmatch = 8;
			pPlayer->m_skill_wrongdoor = 9;
			pPlayer->m_skill_miss = 81;
			pPlayer->m_skill_goddam = 40;
			pPlayer->m_skill_locked = 41;
			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );
			pPlayer->m_kadoma_exp = 100000;//LVMAX

			g_causality_add = 5;

			pPlayer->m_save_allow = 1;//���ô浵
			pPlayer->m_load_check = 1;//����������
		}
		if(pev->frags == 50){
			pPlayer->GiveNamedItem( "item_suit" );
			pPlayer->GiveNamedItem( "item_flashlight" );
			pPlayer->GiveNamedItem( "weapon_crowbar" );
			pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
			pPlayer->GiveNamedItem( "weapon_shotgun" );
			pPlayer->GiveNamedItem( "weapon_9mmAR" );
			pPlayer->GiveNamedItem( "weapon_smg" );
			pPlayer->GiveNamedItem( "weapon_handgrenade" );
			pPlayer->GiveNamedItem( "weapon_tripmine" );
			pPlayer->GiveNamedItem( "weapon_dueluzi" );
			pPlayer->GiveNamedItem( "weapon_357" );
			pPlayer->GiveNamedItem( "weapon_ak47" );
			pPlayer->GiveNamedItem( "weapon_valvesword" );
			pPlayer->GiveNamedItem( "weapon_fist" );
			pPlayer->GiveNamedItem( "weapon_deagle" );
			pPlayer->GiveNamedItem( "weapon_medkit" );
			pPlayer->GiveNamedItem( "weapon_hammer" );
			pPlayer->GiveNamedItem( "weapon_displacer" );
			pPlayer->GiveNamedItem( "weapon_redeemer" );
			pPlayer->GiveNamedItem( "weapon_sniperrifle" );
			pPlayer->GiveNamedItem( "weapon_darkgrenade" );
			pPlayer->GiveNamedItem( "weapon_sg550" );
			pPlayer->GiveNamedItem( "weapon_crossbow" );
			pPlayer->GiveNamedItem( "weapon_egon" );
			pPlayer->GiveNamedItem( "weapon_gauss" );
			pPlayer->GiveNamedItem( "weapon_rpg" );
			pPlayer->GiveNamedItem( "weapon_satchel" );
			pPlayer->GiveNamedItem( "weapon_snark" );
			pPlayer->GiveNamedItem( "weapon_airgun" );
			pPlayer->GiveNamedItem( "weapon_hornetgun" );
			pPlayer->GiveNamedItem( "weapon_minigun" );
			pPlayer->GiveNamedItem( "weapon_dualdbarrel" );
		}
		if(pev->frags == 70){
			pPlayer->GiveAmmo( BUCKSHOT_MAX_CARRY, "buckshot", BUCKSHOT_MAX_CARRY );
			pPlayer->GiveAmmo( ROCKET_MAX_CARRY, "rockets", ROCKET_MAX_CARRY );
			pPlayer->GiveAmmo( AK_MAX_CARRY, "762nato", AK_MAX_CARRY );
			pPlayer->GiveAmmo( M16_MAX_CARRY, "556nato", M16_MAX_CARRY );
			pPlayer->GiveAmmo( BOLT_MAX_CARRY, "bolts", BOLT_MAX_CARRY );
			pPlayer->GiveAmmo( M203_GRENADE_MAX_CARRY, "ARgrenades", M203_GRENADE_MAX_CARRY );
			pPlayer->GiveAmmo( _357_MAX_CARRY, "357", _357_MAX_CARRY );
			pPlayer->GiveAmmo( SNIPER_MAX_CARRY, "338mag", SNIPER_MAX_CARRY );
			pPlayer->GiveAmmo( MAC_MAX_CARRY, "45acp", MAC_MAX_CARRY );
			pPlayer->GiveAmmo( _9MM_MAX_CARRY, "9mm", _9MM_MAX_CARRY );
			pPlayer->GiveAmmo( MINIGUN_MAX_CARRY, "762natobox", MINIGUN_MAX_CARRY );
			pPlayer->GiveAmmo( URANIUM_MAX_CARRY, "uranium", URANIUM_MAX_CARRY );
			pPlayer->GiveAmmo( MEDKIT_MAX_CARRY, "kmedkit", MEDKIT_MAX_CARRY );
			pPlayer->GiveAmmo( DARKGRENADE_MAX_CARRY,"Dark Grenade", DARKGRENADE_MAX_CARRY);
			pPlayer->GiveAmmo( HANDGRENADE_MAX_CARRY, "Hand Grenade", HANDGRENADE_MAX_CARRY );
			pPlayer->GiveAmmo( SATCHEL_MAX_CARRY, "Satchel Charge", SATCHEL_MAX_CARRY );
			pPlayer->GiveAmmo( TRIPMINE_MAX_CARRY, "Trip Mine", TRIPMINE_MAX_CARRY );
			pPlayer->GiveAmmo( SNARK_MAX_CARRY, "Snarks", SNARK_MAX_CARRY );
		}
		if(pev->frags == 80){
				FireTargets( "boss_rush_make1", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 110){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 2000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 5;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 10\n");
				pPlayer->m_player_time = 0;//��ʱ��λ!
		}
		if(pev->frags == 125){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 115;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 175){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 182){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel2" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 200){
				FireTargets( "boss_rush_make2", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 230){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_crasher_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 3000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 6;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 17\n");
		}
		if(pev->frags == 245){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 235;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 295){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 302){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel3" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 320){
				FireTargets( "boss_rush_make3", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 350){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_chainsaw_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 2400;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 9;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 23\n");
		}
		if(pev->frags == 365){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 355;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 415){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 422){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel4" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 440){
				FireTargets( "boss_rush_make4", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 470){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 9000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 4;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 20\n");
		}
		if(pev->frags == 485){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 475;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 535){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 542){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel5" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 560){
				FireTargets( "boss_rush_make5", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 590){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_barnacle_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 25000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 14;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 5\n");
		}
		if(pev->frags == 605){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 595;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 655){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 662){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel6" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 680){
				FireTargets( "boss_rush_make6", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 710){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_doraemon_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 36000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 17;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 9\n");
		}
		if(pev->frags == 725){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 715;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 805){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 812){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel7" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 830){
				FireTargets( "boss_rush_make7", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 860){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_doma_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 90000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
					pSpot->pev->velocity = g_vecZero;
					UTIL_SetOrigin( pSpot->pev, pSpot->pev->origin + Vector(0,0,8));
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 21;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 6\n");
		}
		if(pev->frags == 875){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 865;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 935){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 942){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel8" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 960){
				FireTargets( "boss_rush_make8", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 990){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_god625_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 100000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 22;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 22\n");
		}
		if(pev->frags == 1005){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 995;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 1085){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 1092){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_boss_rush_tel9" );
			if ( pTel ){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
				pPlayer->pev->origin = pTel->pev->origin;
				pPlayer->pev->v_angle = pTel->pev->angles;
				pPlayer->pev->angles = pTel->pev->angles;
				pPlayer->pev->fixangle = TRUE;
				pPlayer->m_flVelocityModifier = 0;
			}
		}
		if(pev->frags == 1110){
				FireTargets( "boss_rush_make9", this, this, USE_TOGGLE, 0 );
		}
		if(pev->frags == 1140){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_gman_boss");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					pSpot->pev->max_health = 72000;
					pSpot->pev->health = pSpot->pev->max_health;
					pSpot->pev->takedamage	= DAMAGE_AIM;
				}
				pPlayer->BOSS_Find();
				pPlayer->m_music_save = 16;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 18\n");
		}
		if(pev->frags == 1155){//BOSS����⿪ʼ
			if(pPlayer->m_boss_on != 0){
				pev->frags = 1145;//ÿ����1�� BOSS�Ƿ��
			}
			else{
				float gtime = pPlayer->m_player_time;
				float gtime_m = gtime / 60;
				if(gtime_m >= 1){
					gtime -= (int)gtime_m * 60;
				}
				float gtime_s = gtime;

				sprintf( text, "- Total: %1.0f:%1.0f\n", gtime_m,gtime_s );
				UTIL_SayTextAll( text,this );

				CLIENT_COMMAND(pPlayer->edict(), "mp3 stop\n");
			}
		}
		if(pev->frags == 1180){//ս��ͳ��!
			pPlayer->m_mode_float2 = 520;
			SERVER_COMMAND("mp3 play media/music30.mp3\n");
		}
		if(pev->frags == 1280){
			pPlayer->Clear_SayText();
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 15, 255, FFADE_OUT );//��Ϲ���
		}
		if(pev->frags == 1320){//��!
			SERVER_COMMAND( "map wdoor_bonus_level\n" );
			return;
		}
	}
	if(pev->armortype == 94){//�¼�94 Headcrab Ball
		if(pev->frags == 20){
			pPlayer->GiveNamedItem( "item_suit" );
			pPlayer->GiveNamedItem( "weapon_crowbar" );
			pPlayer->pev->health = 300;
			pPlayer->pev->max_health = 300;
			pPlayer->m_save_allow = 1;//���ô浵
			pPlayer->m_load_check = 1;//����������
			pPlayer->m_trainning  = 3;//ͷзͶ��С��Ϸģʽ
		}
		if(pev->frags == 30){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_gonome");
				if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_no_pov_limit = 1;
					pSpot->pev->spawnflags = 0;
					//pSpot->pev->effects = EF_DIMLIGHT;
					pSpot->pev->gravity = 0.5;
					pSpot->pev->movetype = MOVETYPE_TOSS;
					pSpot->pev->takedamage = DAMAGE_NO;
				}
				SERVER_COMMAND("mp3 loop media/music12.mp3\n");
		}
	}
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}
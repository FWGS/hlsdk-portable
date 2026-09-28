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
// ��CG�¼���ͳ��
//=========================================================
class CMain_Event : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT killThink_new ( void );
	void	EXPORT prevent_think ( void );
	CBasePlayer *pPlayer;
};

LINK_ENTITY_TO_CLASS( main_cg_event_new, CMain_Event );
LINK_ENTITY_TO_CLASS( cshl623_prevent, CMain_Event );

void CMain_Event::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate = 1.0;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	if(FClassnameIs(pev,"cshl623_prevent")){
	SetThink (&CMain_Event::prevent_think);
	}
	else{
	SetThink (&CMain_Event::killThink_new);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CMain_Event::prevent_think ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}
	if(pev->armortype == 0){
		if(pev->armorvalue == 1){

		char text[256];
		sprintf( text, "- You need kill the all person\n");
		UTIL_SayTextAll( text,this );

		SERVER_COMMAND("mp3 loop media/music5.mp3\n");

		CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "give_shotgun_event" );
		if ( pEntity3 )
		{
			UTIL_Remove( pEntity3 );
		}

		FireTargets( "kill_npc_think", this, this, USE_TOGGLE, 0 );
		FireTargets( "red_666", this, this, USE_TOGGLE, 0 );
		FireTargets( "hurt666", this, this, USE_TOGGLE, 0 );
		FireTargets( "down666", this, this, USE_TOGGLE, 0 );

			if(pPlayer->pev->origin.y >= -1415){
			pPlayer->pev->origin.y = -1450;
			}
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.25, 255, FFADE_IN );//��Ϲ���

			//����ɱ�Ҵ���!

		pev->armortype = 1;

			if(pev->health != 623){//�¼�8����
			FireTargets( "upstair_door1", this, this, USE_TOGGLE, 0 );
			FireTargets( "upstair_door2", this, this, USE_TOGGLE, 0 );
			}

		}
	}
	else if(pev->armortype == 1){
		if(pev->frags >= 9){
		SERVER_COMMAND("mp3 stop\n");
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
			pPlayer->Clear_SayText();

		CBaseEntity *pEntity5 = UTIL_FindEntityByTargetname( NULL, "npc_nosic_scream" );
		if ( pEntity5 )
		{
			UTIL_Remove( pEntity5 );
		}

		FireTargets( "kill_npc_think", this, this, USE_TOGGLE, 0 );
		FireTargets( "hurt666", this, this, USE_TOGGLE, 0 );
		FireTargets( "down666", this, this, USE_TOGGLE, 0 );

		FireTargets( "upstair_door1", this, this, USE_TOGGLE, 0 );
		FireTargets( "upstair_door2", this, this, USE_TOGGLE, 0 );

		if(pev->health == 623){//�¼�8δ����
			CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
			if ( pSpot ){
			CBaseEntity *pGun = Create( "weapon_shotgun_def", pSpot->pev->origin - Vector(80,40,47), Vector(0,90,0), NULL );
			FireTargets( "chou_tip", this, this, USE_TOGGLE, 0 );
			}
		}

		pev->armortype = -1;
		UTIL_Remove( this );
		return;
		}
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

//===================================��ƪ�±��1=================================================//
void CMain_Event::killThink_new ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	if(pev->armortype == -1){//�¼�-1 ��ɽʵ����
		if(pPlayer->m_trainning == 2){//����!
			if(pev->armorvalue == 0){
			pev->frags = 0;
			pev->armorvalue = 1;
			}
			if(pev->frags == 0){
				pPlayer->RemoveAllItems( TRUE );
				pPlayer->EnableControl(FALSE);
				pPlayer->Clear_SayText();
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 6.0, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 20){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR13", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Scientist: Hey! What are you doing!?\n");
						
						UTIL_SayTextAll( text,this );
					}	
			}
			if(pev->frags == 70){
				pPlayer->Clear_SayText();
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "train_telport" );
				if ( pEntity ){
					pPlayer->pev->origin = pEntity->pev->origin;
					pPlayer->EnableControl(TRUE);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;
					pPlayer->m_flVelocityModifier = -4;
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.0, 2.0, 255, FFADE_IN );//��Ϲ���
				}
			}
			if(pev->frags == 90){
				if ( !(pPlayer->pev->weapons & (1<<WEAPON_SUIT)) ){
				pPlayer->GiveNamedItem( "weapon_fist" );
				pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
				pPlayer->m_hasflashlight = TRUE;
				pPlayer->MenuItem_add(1);//�ֵ�ͲGet
				}
				pPlayer->m_trainning = 0;
				UTIL_Remove( this );
				return;
			}
			pev->frags += 1;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}

			if(pev->frags == -95){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "kadoma" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->m_MonsterState = MONSTERSTATE_NONE;
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "idle2" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetThink( NULL );
						//pEnemyMonster->SetBodygroup( 0, 1 );
					}
			}
			else if(pev->frags == -90){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pSpot->pev->frags = 1;
							pSpot->pev->velocity.x = 30;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->EnableControl(FALSE);
						}
			}
			else if(pev->frags == -45){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
					pSpot->pev->armortype = 5;
					}
			}
			else if(pev->frags == 0){
					pPlayer->EnableControl(TRUE);
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
			}
			else if(pev->frags == 5){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr1" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster && pEnemyMonster->pev->sequence == pEnemyMonster->LookupActivity ( ACT_WALK )){
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR1", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
			
						sprintf( text, "Rookie Guard: Here's hoping that target practice was worth it!\n");
					
						UTIL_SayTextAll( text,this );

						pEntity->pev->owner = ENT(pPlayer->pev);

						goto tr_comp;
						}
					}
					pev->frags = 1;
			}
			else if(pev->frags == 50){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR2", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
						
						sprintf( text, "Blue Guard: Make sure to aim for the head.\n");
						
						UTIL_SayTextAll( text,this );
					}
			}
			else if(pev->frags == 100){
				pPlayer->Clear_SayText();
			}
			else if(pev->frags == 140){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR10", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Blue Guard: Don't be scared, you hear?\n");
					
						UTIL_SayTextAll( text,this );
					}
			}
			else if(pev->frags == 150){
					pPlayer->m_fNextClearTextTime = gpGlobals->time + 3.0;		
			}
			else if(pev->frags == 160){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr1" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster->m_hEnemy != NULL){
						pEnemyMonster->m_hPlayer = pPlayer;
						pEnemyMonster->m_rpgms_inteam = 5;
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR3", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Rookie Guard: Alright! I've got this!\n");
						
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		

						goto tr_comp;
						}
					}
					pev->frags = 155;
			}
			else if(pev->frags == 170){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_barney_tr1" );
					if ( pEntity && pEntity2  ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						if(pEnemyMonster->m_hEnemy != NULL){
							float flDist = ( pEnemyMonster->pev->origin - pEnemyMonster->m_hEnemy->pev->origin).Length();
							if(flDist < 240){
							EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR1", VOL_NORM, 0.5, 0, PITCH_NORM );

								char text[256];
							
								sprintf( text, "Scientist: Get out of there! Run! RUN!\n");
							
								UTIL_SayTextAll( text,this );

								pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		

							goto tr_comp;
							}
						}
					}
					pev->frags = 165;
			}
			else if(pev->frags == 180){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_barney_tr1" );
					if ( pEntity && pEntity2  ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						if(pEnemyMonster->pev->health <= pEnemyMonster->pev->max_health * 0.5){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR2", VOL_NORM, 0.5, 0, PITCH_NORM );

								char text[256];
								
								sprintf( text, "Scientist: Don't die for nothing!\n");
							
								UTIL_SayTextAll( text,this );

								pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		

						goto tr_comp;
						}
					}
					pev->frags = 175;
			}
			else if(pev->frags == 190){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_barney_tr1" );
					if ( pEntity2  ){
						if(pEntity2->pev->deadflag != DEAD_NO){
						goto tr_comp;
						}
					}
					pev->frags = 185;
			}
			else if(pev->frags == 220){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_TR7", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
						
						sprintf( text, "Blue Guard: This is all because of you scientists!\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 11.0;		

						goto tr_comp;
					}
					pev->frags = 215;
			}
			else if(pev->frags == 230){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
					FX_Explosion( pev->origin, 254 );//��������Ѫ������!
			}
			else if(pev->frags == 260){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR3", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
						
						sprintf( text, "Scientist: That wasn't our fault!\n");
					
						UTIL_SayTextAll( text,this );

						goto tr_comp;
					}
					pev->frags = 255;
			}
			else if(pev->frags == 290){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_TR8", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Blue Guard: Yes it is! You made those creatures!\n");
						
						UTIL_SayTextAll( text,this );

						goto tr_comp;
					}
					pev->frags = 285;
			}
			else if(pev->frags == 340){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster->pev->sequence == pEnemyMonster->LookupActivity ( ACT_WALK )){
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR5", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
			
						sprintf( text, "Blue Guard: Watch this!\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;	
						pEntity->pev->owner = ENT(pPlayer->pev);

						goto tr_comp;
						}
					}
					pev->frags = 335;
			}
			else if(pev->frags == 410){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR6", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Scientist: Hmmm. This could be interesting.\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		

						goto tr_comp;
					}
					pev->frags = 405;
			}
			else if(pev->frags == 420){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster->m_hEnemy != NULL){
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR6", VOL_NORM, 0.5, 0, PITCH_NORM );

						pEnemyMonster->m_cover_dist = 128;
						pEntity->pev->owner = NULL;//�����BUG�޸�
						pEnemyMonster->m_hPlayer = pPlayer;
						pEnemyMonster->m_rpgms_inteam = 5;

						char text[256];
					
						sprintf( text, "Blue Guard: Die you vampires!\n");
						
						UTIL_SayTextAll( text,this );
						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		

						goto tr_comp;
						}
					}
					pev->frags = 415;
			}
			else if(pev->frags == 450){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster->pev->sequence == pEnemyMonster->LookupActivity ( ACT_RUN )
						|| pEntity->pev->deadflag != DEAD_NO){//�Է���һ��ס
						
						EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR9", VOL_NORM, 0.5, 0, PITCH_NORM );
	
						goto tr_comp;
						}
					}
					pev->frags = 445;
			}
			else if(pev->frags == 470){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_barney_tr2" );
					if ( pEntity ){
						if(pEntity->pev->deadflag != DEAD_NO){
						goto tr_comp;
						}
					}
					pev->frags = 465;
			}
			else if(pev->frags == 510){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR7", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Scientist: As I thought.\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		

						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
						FX_Explosion( pev->origin, 254 );//��������Ѫ������!

						goto tr_comp;
					}		

					pev->frags = 505;
			}
			else if(pev->frags == 540){
					SERVER_COMMAND( "autosave\n" );
			}
			else if(pev->frags == 545){
					pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
					pPlayer->m_hasflashlight = TRUE;
					pPlayer->MenuItem_add(1);//�ֵ�ͲGet

					CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "barney_tr3_start" );
					if ( pEntity2 )
					{
						UTIL_Remove( pEntity2 );
					}
			}
			else if(pev->frags == 550){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR8", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Scientist: Okay, you're up.\n");
						
						UTIL_SayTextAll( text,this );
					
						sprintf( text, "- (You can skip the combat by leaving)\n");
						
						UTIL_SayTextAll( text,this );
						pPlayer->m_fNextClearTextTime = gpGlobals->time + 9.0;	
						
						pEntity = UTIL_FindEntityByTargetname( NULL, "elevdoor" );
						if ( pEntity )
						{
						pEntity->pev->armorvalue = 1;//���ţ�
						}

						goto tr_comp;
					}
					pev->frags = 555;
			}
			else if(pev->frags == 615){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_good_work1" );
					if ( pEntity )
					{
						if(pEntity->pev->solid == SOLID_NOT){
						goto tr_comp;
						}
					}

					pev->frags = 610;
			}
			else if(pev->frags == 645){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), 6, "!SC_TR10", VOL_NORM, 0.4, 0, PITCH_NORM );

						char text[256];
			
						sprintf( text, "Scientist: Good. I think you're ready for the next battle.\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 5.0;		
						goto tr_comp;
					}
					pev->frags = 640;
			}
			else if(pev->frags == 680){
					SERVER_COMMAND( "autosave\n" );
			}
			else if(pev->frags == 700){
						pPlayer->DropPlayerItem("weapon_9mmhandgun");

						char text[256];
					
						sprintf( text, "- Move in a circle and strike the creature from behind!\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 7.0;	
			}
			else if(pev->frags == 715){
					FireTargets( "rabbit_6_start", this, this, USE_TOGGLE, 0 );
			}
			else if(pev->frags == 740){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_good_work2" );
					if ( pEntity )
					{
						if(pEntity->pev->solid == SOLID_NOT){
						goto tr_comp;
						}
					}

					pev->frags = 735;
			}
			else if(pev->frags == 765){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_scientist_tr" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), 6, "!SC_TR9", VOL_NORM, 0.4, 0, PITCH_NORM );

						char text[256];
			
						sprintf( text, "Scientist: Great. It couldn't keep up. Please go to the next area.\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 6.0;		
						goto tr_comp;
					}
					pev->frags = 760;
			}
			else if(pev->frags == 810){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 6.0, 255, FFADE_OUT );//��Ϲ���
			}
			else if(pev->frags == 860){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "info_player_deathmatch" );
					if ( pEntity ){
							pPlayer->RemoveAllItems( TRUE );
							pPlayer->pev->origin = pEntity->pev->origin;
							pPlayer->m_flVelocityModifier = -100;
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.0, 2.0, 255, FFADE_IN );//��Ϲ���
							pev->frags = 893;
					}
			}
			else if(pev->frags == 910){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_medic1" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR11", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Scientist: I'm amazed you're still alive!\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		
						goto tr_comp;
					}
					pev->frags = 905;
			}
			else if(pev->frags == 950){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "sci_medic2" );
					if ( pEntity ){
						EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!SC_TR12", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
					
						sprintf( text, "Scientist: Wow! This is inconceivable!\n");
					
						UTIL_SayTextAll( text,this );

						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		
						goto tr_comp;
					}
					pev->frags = 945;
			}
			if(pev->frags == 990){
					pPlayer->GiveNamedItem( "weapon_fist" );
					pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
					pPlayer->MenuItem_add(1);//�ֵ�ͲGet
					pPlayer->m_flVelocityModifier = 0;
					pPlayer->m_iClientHealth = -1;
			}
			else if(pev->frags == 1000){
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 0){//�¼�0 ��ͷ��Gman���İ�
			if(pev->frags == -10){
						pPlayer->m_trainning = 1;
						SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == -5){
						pPlayer->Game_Load_SecondData();
			}
			if(pev->frags == 0){
						if(pPlayer->m_fSecondWorld == FALSE){
						pPlayer->Game_Load_SecondData();
						}
			}
			if(pev->frags == 5){
						pPlayer->m_player_diamonds = 0;
						if(pPlayer->m_fSecondWorld == TRUE){
						FireTargets( "second_clear", this, this, USE_TOGGLE, 0 );
						}
			}
			if(pev->frags == 10){
						pPlayer->EnableControl(FALSE);
			}
			if(pev->frags == 40){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pSpot->pev->frags = 1;
							pSpot->pev->armortype = 1;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->Clear_SayText();
						}
			}
			if(pev->frags == 50){
				CLIENT_COMMAND(pPlayer->edict(), "cd play 2\n");
				//SERVER_COMMAND("mp3 play media/music1.mp3\n");
			}
			if(pev->frags == 233){
			//	FireTargets( "player_run", this, this, USE_TOGGLE, 0 );
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					UTIL_SetSize(pEntity->pev, Vector( -4, -4, 0), Vector(4, 4, 16));
					pEnemyMonster->SetActivity( ACT_RUN );
					pEntity->pev->movetype = MOVETYPE_FLY;
					pEntity->pev->velocity.x = -280;
					}
			}
			if(pev->frags == 240){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 285){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				pEntity->pev->weapons = 2;
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;//������������ֹͣ��˼��
				pEnemyMonster->SetActivity( ACT_RUN_SCARED );
				pEnemyMonster->SetThink( NULL );
				pEntity->pev->movetype = MOVETYPE_FLY;
				pEntity->pev->velocity.x = -310;
				}
			}
			if(pev->frags == 300){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
							if ( pEntity ){
							pSpot->pev->angles.y = 90;
							pSpot->pev->origin.y -= 240;
							pSpot->pev->origin.z += 10;
							pSpot->pev->origin.x = pEntity->pev->origin.x - 120;
							pSpot->pev->velocity.x = -300;
							}
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 310){
				char text[256];
			
				sprintf( text, "- Kadoma Male Age:24\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 340){
				FireTargets( "bus1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 350){
				char text[256];
			
				sprintf( text, "- He was a veteran black-clad guard at Black Mesa.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 400){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���

				char text[256];
			
				sprintf( text, "- Until an accident happened.\n");
			
				UTIL_SayTextAll( text,this );

				FireTargets( "cshl623_cam", this, this, USE_TOGGLE, 0 );
				FireTargets( "bryan_start_move", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 420){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_Remove( pEntity );
					}
			}
			if(pev->frags == 430){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 2, 1 );
					}
			}
			if(pev->frags == 435){
				FireTargets( "bryan_start_move2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 465){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_RUN );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					pEntity->pev->movetype = MOVETYPE_TOSS;
					ClearBits( pEntity->pev->flags, FL_ONGROUND );
					UTIL_SetOrigin (pEntity->pev, pEntity->pev->origin + Vector ( 0 , 0 , 1) );
					pEnemyMonster->pev->velocity.y = -250;
					pEnemyMonster->pev->velocity.z = 150;
					}
			}
			if(pev->frags == 470){
				pPlayer->Clear_SayText();
				FireTargets( "apache_fly", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 475){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma2" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "lbduck_1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;

						CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "player_in_dirt" );
						if ( pEntity2 )
						{//Bug Fix 2.0 Kadomaû���������ڣ�
						UTIL_SetOrigin (pEntity->pev, pEntity2->pev->origin);
						pev->origin = pEntity2->pev->origin;//��������
						}
					}
			}
			if(pev->frags == 485){
				FireTargets( "hgrunt_start", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 495){
				FireTargets( "multi_d", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 570){
				UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 6.0, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 590){
				FireTargets( "gman_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 600){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_Remove( pEntity );
					}
					//����ǰ
					pPlayer->m_iClient_Gameover = 2;
					pPlayer->m_fGameOverTime = gpGlobals->time + 5;
			}
			if(pev->frags == 635){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
						pPlayer->EnableControl(FALSE);

						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.y = -15;
						}
			}
			if(pev->frags == 700){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.5, 1.5, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 720){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.y = -30;
						}
			}
			if(pev->frags == 730){
					FireTargets( "kadoma_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 735){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_no_pov_limit = 1;
					pEnemyMonster->m_ignoreFail_OFF = 0;
					pEnemyMonster->m_ignoreFail = 1919;
					pEnemyMonster->SetBodygroup( 0, 1 );
					pEnemyMonster->SetBodygroup( 2, 3 );
					}
			}
			if(pev->frags == 740){
					FireTargets( "shoottarget_door", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 750){
					FireTargets( "kadoma_bar_move", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 810){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->avelocity.y = -40;
					pSpot->pev->armortype = 8;
				}
			}
			if(pev->frags == 840){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->velocity.y = 0;
					pSpot->pev->velocity.x = 20;
					pSpot->pev->armortype = 4;
				}
			}
			if(pev->frags == 860){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_COMBAT_IDLE );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 865){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->armortype = 8;
				}
			}
			if(pev->frags == 870 || pev->frags == 875 || pev->frags == 880){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_RANGE_ATTACK1 );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 878){
					FireTargets( "teleport_sci_maker", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 885){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_COMBAT_IDLE );
				pEnemyMonster->ResetSequenceInfo( );
				pEnemyMonster->pev->frame = 0;
				}
			}
			if(pev->frags == 895){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					UTIL_SetOrigin (pEntity->pev, pEntity->pev->origin + Vector ( 60 , 0 , 0) );
					}
			}
			if(pev->frags == 900){
					FireTargets( "kadoma_bar_chase", this, this, USE_TOGGLE, 0 );
					FireTargets( "gman_maker2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 930){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_MonsterState	= MONSTERSTATE_NONE;//������������ֹͣ��˼��
				pEnemyMonster->SetThink( NULL );
				}
			}
			if(pev->frags == 950){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
					if ( pEntity ){
					pSpot->pev->origin = pEntity->pev->origin + Vector(96,0,64);
					pSpot->pev->avelocity.y = 0;
					pSpot->pev->velocity.y = 0;
					pSpot->pev->velocity.x = 0;
					pSpot->pev->angles.y = 180;
					}
				}
			}
			if(pev->frags == 990){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.5, 1.5, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 995){//����һ��һ��һ��һ��������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if ( FClassnameIs ( pEntity->pev, "monstermaker" ) || (pEntity->pev->flags & FL_MONSTER) ){
							if ( FClassnameIs ( pEntity->pev, "monster_apache" )){
							STOP_SOUND( ENT(pEntity->pev), CHAN_STATIC, "apache/ap_rotor2.wav" );
							}
							UTIL_Remove( pEntity );
						}
					}
			}
			if(pev->frags == 1000){
					CBaseEntity *pEntity = Create( "monster_kadoma2", pev->origin, Vector(0,90,0), NULL );
					if ( pEntity ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "i_need_more_power" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
					}
			}
			if(g_fGameSkipCG == 1){
				g_fGameSkipCG = 0;
				SERVER_COMMAND( "map c1a0_wdoor\n" );
				return;
			}
			//Bug Fix 2.0 �ӿ�׹��
			if(pev->frags == 1150){
					FireTargets( "break_in_dirt", this, this, USE_TOGGLE, 0 );
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 1){//�¼�1 ׹�Ӻڰ����뿪ʼ
			if(pev->frags == 1){
							SERVER_COMMAND("mp3 stop\n");
							pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
							pPlayer->m_hasflashlight = TRUE;
							pPlayer->MenuItem_add(1);//�ֵ�ͲGet

							if (g_iSkillLevel == SKILL_EASY){
							pPlayer->m_kadoma_level = 8;
							pPlayer->m_kadoma_exp = 8000;
							pPlayer->pev->health = 180;
							pPlayer->pev->max_health = 180;
							}
							else{
							pPlayer->m_kadoma_level = 2;
							pPlayer->m_kadoma_exp = 2000;
							pPlayer->pev->health = 120;
							pPlayer->pev->max_health = 120;
							}
							
							pPlayer->m_game_rate = 5;//��Ϸ����5%
			}
			if(pev->frags == 60){
							pPlayer->GiveNamedItem( "weapon_fist" );
							pPlayer->m_trainning = 0;
			}
			if(pev->frags == 80){
				SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 100){
			SERVER_COMMAND("mp3 play media/music2.mp3\n");
			FireTargets( "door1", this, this, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 2){//�¼�2 Van����̸1
			if(pev->frags == 0){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 20){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_USE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 40){
				char text[256];
			
				sprintf( text, "???: I need a sofa.\n");
			
				UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 80){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->Clear_SayText();
						pPlayer->m_game_rate = 6;//��Ϸ����6%
			}
			if(pev->frags == 90){
			FireTargets( "van_ftalk_wall", this, this, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 3){//�¼�3 Van����̸2
			if(pev->frags == 0){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->Clear_SayText();
						}
			}
			if(pev->frags == 10){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
					pev->origin = pEntity->pev->origin;
					pev->angles.y = 0;
					pEntity->pev->frags = 3;
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_IDLE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 20){
					FireTargets( "van_move1", this, this, USE_TOGGLE, 0 );
			}
			
			if(pev->frags == 50){
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "buttons/button12.wav", 1, ATTN_NORM);
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, 0 );
			}
			if(pev->frags == 52){
					CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "sofa_push" );
					if ( pEntity2 )
					{
						UTIL_Remove( pEntity2 );
						FireTargets( "door2", this, this, USE_TOGGLE, 0 );
					}
			}
			if(pev->frags == 55){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
					UTIL_SetOrigin (pEntity->pev, pev->origin + Vector(0,0,25));
					pEntity->pev->movetype = MOVETYPE_FLY;
					pEntity->pev->angles = pev->angles;
					pEntity->pev->frags = 2;
					FireTargets( "sofa_hide", this, this, USE_TOGGLE, 0 );
					}
			}
			if(pev->frags == 58){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;
					pEnemyMonster->SetThink( NULL );
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_GUARD );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 85){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_VICTORY_DANCE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}

					char text[256];
					
					sprintf( text, "???: Thank you.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 120){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							FireTargets( "door3", this, this, USE_TOGGLE, 0 );
							pPlayer->Clear_SayText();
						}
			}
			if(pev->frags == 130){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_vanlve" );
					if ( pEntity ){
						if(pPlayer->m_fSecondWorld == TRUE){//����ĿVanlve����һ���ڰ�����!
						Create( "weapon_darkgrenade", pEntity->pev->origin + Vector(0,0,16), g_vecZero, NULL );
						}
					UTIL_Remove( pEntity );
					}
			}
			if(pev->frags == 150){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_game_rate = 7;//��Ϸ����7%
			}
			if(pev->frags == 160){
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 4){//�¼�4 ƥŵ��4��������
			if(pev->frags == 0){
					pPlayer->m_trainning = 1;
					pPlayer->m_flVelocityModifier = 0;
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							pSpot->pev->origin.z -= 20;
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->armortype = 2;
							pSpot->pev->frags = 1;
							FireTargets( "man_sound", this, this, USE_TOGGLE, 0 );
						}
			}
			if(pev->frags == 50){
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if ( FClassnameIs(pEntity->pev, "monster_cof_ms3") )
						{
							if( FStrEq(STRING(pEntity->pev->targetname), "jumpers1")){
								CBaseMonster *pMonster = pEntity->MyMonsterPointer( );
								if(pMonster){
									ClearBits( pMonster->pev->flags, FL_ONGROUND );

									pMonster->pev->movetype = MOVETYPE_TOSS;
									pMonster->pev->velocity.z = 250;
									pMonster->pev->velocity.x = -500;

									pMonster->pev->sequence = pMonster->LookupActivity ( ACT_FALL );
									pMonster->ResetSequenceInfo( );
									pMonster->pev->frame = 0;

									pMonster->m_alert = 100;

									//pMonster->pev->health += 40;
								}
							}
						}
					}
			}
			if(pev->frags == 60){
				FireTargets( "sci_start", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 70){
				FireTargets( "door1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 80){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						SERVER_COMMAND("mp3 play media/music3.mp3\n");
						pPlayer->m_fPlayerHideMode = TRUE;
						pPlayer->m_flVelocityModifier = 0;
			}
			if(pev->frags == 100){
						pPlayer->m_fPlayerHideMode = FALSE;
						pPlayer->pev->flags &= ~FL_NOTARGET;
						pPlayer->m_game_rate = 8;//��Ϸ����8%
			}
			if(pev->frags == 110){
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 5){//�¼�5 �ֻ�����
			if(pev->frags == 0){
						if(pPlayer->pev->deadflag != DEAD_NO)
						return;//�Ѿ������ˣ�����

						pPlayer->m_trainning = 1;
						pPlayer->m_flVelocityModifier = 0;

					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
					SERVER_COMMAND("mp3 stop\n");
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pSpot->pev->origin.z -= 20;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 3){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
					Vector pp = pSpot->pev->origin;
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pp + Vector(64,64,-32), Vector(0,180,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 60;
					pEnemyMonster->pev->flags |= FL_NOTARGET;
					}
			}
			if(pev->frags == 10){
				FireTargets( "clear_people", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 25){
				FireTargets( "cl_move1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 70){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pEntity ){
					UTIL_SetOrigin (pEntity->pev, pEntity->pev->origin + Vector ( 0 , 100 , 1) );
					}
			}
			if(pev->frags == 72){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pEntity ){
					pEntity->pev->angles.y = 90;
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_COMBAT_IDLE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->m_boltpoison = 248;
					}
			}
			if(pev->frags == 75){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
				if ( pSpot ){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity ){
					pSpot->pev->origin.x -= 50;
					pSpot->pev->origin.y += 75;
					pSpot->pev->angles.y = 270;
					pSpot->pev->origin.z += 5;
					}
				}
			}
			if(pev->frags == 79){//ǰ��Ԥ֪��
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster2;
					pEnemyMonster2 = pEntity2->MyMonsterPointer();
					pEnemyMonster2->pev->sequence = pEnemyMonster2->LookupActivity ( ACT_COWER );
					pEnemyMonster2->ResetSequenceInfo( );
					pEnemyMonster2->pev->frame = 0;
					}
			}
			if(pev->frags == 80){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_USE );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->m_alert = 100;
					pEntity->pev->takedamage = DAMAGE_NO;
					pEntity->pev->health = 300;//������!
					pEntity->pev->gravity = 2.0;//����!
					pEnemyMonster->m_hPlayer = pPlayer;
					pEnemyMonster->m_rpgms_inteam = 5;
					}
			}
			if(pev->frags == 100){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->pev->angles = Vector(0,270,0);
						pPlayer->pev->fixangle = TRUE;
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
							if ( pEntity2 ){
							pPlayer->pev->origin = pEntity2->pev->origin + Vector(-32,0,36);
							UTIL_Remove( pEntity2 );
							}
			}
			if(pev->frags == 125){
					char text[256];
					
					sprintf( text, "???: Sorry, I thought you were a monster.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 155){
					char text[256];
					
					sprintf( text, "???: This place is safe; Dont worry.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 205){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 257){
					FireTargets( "clear_people", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 282){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pEntity ){
					Vector og = pEntity->pev->origin;

						CBaseEntity *pMonster1 = Create( "monster_cof_ms3", og + Vector(190,-10,100), Vector(0,180,0), NULL );
						CBaseMonster *pEnemyMonster1;
						pEnemyMonster1 = pMonster1->MyMonsterPointer();
						pEnemyMonster1->m_boltpoison = 30;
						pEnemyMonster1->m_iTriggerCondition = 4;
						pEnemyMonster1->m_iszTriggerTarget = MAKE_STRING("monster_kills");
						pEnemyMonster1->m_chase_mode = 0;

						CBaseEntity *pMonster2 = Create( "monster_cof_ms3", og + Vector(190,-60,100), Vector(0,180,0), NULL );
						CBaseMonster *pEnemyMonster2;
						pEnemyMonster2 = pMonster2->MyMonsterPointer();
						pEnemyMonster2->m_boltpoison = 30;
						pEnemyMonster2->m_iTriggerCondition = 4;
						pEnemyMonster2->m_iszTriggerTarget = MAKE_STRING("monster_kills");
						pEnemyMonster2->m_chase_mode = 0;

						CBaseEntity *pMonster3 = Create( "monster_cof_ms3", og + Vector(190,-110,100), Vector(0,180,0), NULL );
						CBaseMonster *pEnemyMonster3;
						pEnemyMonster3 = pMonster3->MyMonsterPointer();
						pEnemyMonster3->m_boltpoison = 30;
						pEnemyMonster3->m_iTriggerCondition = 4;
						pEnemyMonster3->m_iszTriggerTarget = MAKE_STRING("monster_kills");
						pEnemyMonster3->m_chase_mode = 0;

						CBaseEntity *pMonster4 = Create( "monster_cof_ms3", og + Vector(190,-160,100), Vector(0,180,0), NULL );
						CBaseMonster *pEnemyMonster4;
						pEnemyMonster4 = pMonster4->MyMonsterPointer();
						pEnemyMonster4->m_boltpoison = 30;
						pEnemyMonster4->m_iTriggerCondition = 4;
						pEnemyMonster4->m_iszTriggerTarget = MAKE_STRING("monster_kills");
						pEnemyMonster4->m_chase_mode = 0;

						CBaseEntity *pMonster5 = Create( "monster_cof_ms3", og + Vector(190,-210,100), Vector(0,180,0), NULL );
						CBaseMonster *pEnemyMonster5;
						pEnemyMonster5 = pMonster5->MyMonsterPointer();
						pEnemyMonster5->m_boltpoison = 30;
						pEnemyMonster5->m_iTriggerCondition = 4;
						pEnemyMonster5->m_iszTriggerTarget = MAKE_STRING("monster_kills");
						pEnemyMonster5->m_chase_mode = 0;
					}
			}
			if(pev->frags == 290){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pPlayer->pev->effects |= EF_NODRAW;
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							pSpot->pev->angles.y += 45;
							pSpot->pev->origin.x += 50;
							pSpot->pev->origin.y -= 30;
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 293){
						CBaseEntity *pEntity = NULL;
						CBaseMonster *pEnemyMonster;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 1280 )) != NULL)
						{
							if ( FClassnameIs(pEntity->pev, "monster_cof_ms3") )
							{
								if(pEntity->pev->deadflag == DEAD_NO){
									pEnemyMonster = pEntity->MyMonsterPointer();
									if(pEnemyMonster){
									pEnemyMonster->m_FTSmod = 3;
									pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_MELEE_ATTACK1 );
									pEnemyMonster->ResetSequenceInfo( );
									pEnemyMonster->pev->frame = 0;
									}
								}
							}
						}
			}
			if(pev->frags == 296){
			FireTargets( "break_glass1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 307){
				SERVER_COMMAND("mp3 play media/music3.mp3\n");
			}
			if(pev->frags == 312){
					SET_VIEW( pPlayer->edict(), pPlayer->edict() );
					pPlayer->EnableControl(TRUE);
					pPlayer->m_trainning = 0;

					FireTargets( "giant_push", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 320){
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pEntity2 ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity2->MyMonsterPointer();
					pEnemyMonster->m_alert = 100;
					pEnemyMonster->m_boltpoison = 0;
					pEntity2->pev->takedamage = DAMAGE_YES;
					}
					pPlayer->m_game_rate = 9;//��Ϸ����9%

					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 6){//�¼�6 �ֻ�����
			if(pev->frags == 0){
			pev->frags += 1;
			pev->nextthink = gpGlobals->time + 2.0;//�ӳ�
			return;
			}
			if(pev->frags == 1){
					SERVER_COMMAND("mp3 stop\n");

						if(pPlayer->pev->deadflag != DEAD_NO)
						return;//�Ѿ������ˣ�����

						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->m_trainning = 1;
							pPlayer->pev->origin.z -= 256;
							pSpot->pev->origin.z -= 10;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			if(pev->frags == 2){
					CBaseEntity *pGiant = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pGiant ){
						if(pGiant->pev->deadflag == DEAD_NO){
							CBaseMonster *pEnemyMonster2;
							pEnemyMonster2 = pGiant->MyMonsterPointer();
							if(pEnemyMonster2){
							pGiant->pev->weapons = 1;
							pGiant->TakeDamage ( pev, pev, pGiant->pev->health, DMG_FALL );
							}
						}
					}
			}
			if(pev->frags == 4){
					Vector pp;
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
					pp = pSpot->pev->origin;
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pp + Vector(-30,-96,-50), Vector(0,50,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 200;
					pEnemyMonster->pev->flags |= FL_NOTARGET;
					pEnemyMonster->pev->weapons = 1;
					}

					CBaseEntity *pGiant = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pGiant ){
					pGiant->pev->avelocity.y = 0;
					pGiant->pev->angles.y = 180;
					pGiant->pev->movetype = MOVETYPE_NOCLIP;
					pGiant->pev->velocity = g_vecZero;

					if(pGiant->pev->frags == 0)
					UTIL_SetOrigin (pGiant->pev, pp + Vector(6,-96,-52));
					else
					UTIL_SetOrigin (pGiant->pev, pp + Vector(6,-96,-48));

					CBaseMonster *pEnemyMonster2;
					pEnemyMonster2 = pGiant->MyMonsterPointer();
					pEnemyMonster2->m_boltpoison = 200;
					pEnemyMonster2->pev->flags |= FL_NOTARGET;
					pEnemyMonster2->m_die_dont_move = 1;

						pGiant->pev->body = 0;
						pEnemyMonster2->pev->sequence = pEnemyMonster2->LookupSequence( "sit_dying" );
						pEnemyMonster2->ResetSequenceInfo( );
						pEnemyMonster2->pev->frame = 0;
					}
			}
			if(pev->frags == 25){
					CBaseEntity *pCleaner = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pCleaner ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "dying_friend" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 45){
					CBaseEntity *pGiant = UTIL_FindEntityByClassname( NULL, "monster_giant" );
					if ( pGiant ){
						CBaseMonster *pEnemyMonster2;
						pEnemyMonster2 = pGiant->MyMonsterPointer();

						pGiant->pev->takedamage = DAMAGE_NO;

						pEnemyMonster2->pev->sequence = pEnemyMonster2->LookupSequence( "sit_die" );
						pEnemyMonster2->ResetSequenceInfo( );
						pEnemyMonster2->pev->frame = 0;
					}

					char text[256];
					
					sprintf( text, "???: I'm dying; take this baton.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 85){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->pev->angles = Vector(0,0,0);
						pPlayer->pev->v_angle = Vector(0,0,0);
						pPlayer->pev->fixangle = TRUE;
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
							if ( pEntity2 ){
							pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							UTIL_Remove( pEntity2 );
							}
						pPlayer->GiveNamedItem( "weapon_crowbar" );
			}
			if(pev->frags == 100){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 115){
			FireTargets( "door3", this, this, USE_TOGGLE, 0 );
			FireTargets( "giant_push", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 120){//�ֻ�����ʧ
				CBaseEntity *pGiant = UTIL_FindEntityByClassname( NULL, "monster_giant" );
				if ( pGiant ){
				pGiant->SUB_StartFadeOut();
				}
				pPlayer->m_game_rate = 10;//��Ϸ����10%
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 7){//�¼�7 ·���������ը
			if(pev->frags == 0){
						pPlayer->m_trainning = 1;
						pPlayer->m_flVelocityModifier = -1;
			}
			if(pev->frags == 1){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							pSpot->pev->origin.z -= 16;
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->origin.z -= 256;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->velocity.y += 16;

							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->v_angle = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
						}
			}
			if(pev->frags == 5){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin + Vector(120,60,8), Vector(0,0,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 100;
					pEnemyMonster->pev->flags |= FL_NOTARGET;
					pCleaner1->pev->gravity = 3;
			}
			if(pev->frags == 10){
			FireTargets( "explode_glass", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 14){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->angles = pSpot->pev->angles;
					pSpot->pev->velocity.z = -930;
					pSpot->pev->armortype = 3;
					pSpot->pev->frags = 1;
				}
			}
			if(pev->frags == 26){
						UTIL_ScreenFade( pPlayer, Vector(255,255,255), 0.5, 1.5, 255, FFADE_OUT );//��Ϲ���
			}
			if(pev->frags == 33){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
				if ( pSpot ){
					pSpot->pev->velocity.x = -25;
					pSpot->pev->velocity.y = 0;
					pSpot->pev->velocity.z = 0;
					pSpot->pev->frags = 0;
							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
							if ( pEntity2 ){
							pSpot->pev->origin.z = pEntity2->pev->origin.z + 36;
							}
				}
			}
			if(pev->frags == 36){
					FireTargets( "fire_hide", this, this, USE_TOGGLE, 0 );
					UTIL_ScreenFade( pPlayer, Vector(255,255,255), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 75){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->pev->angles = Vector(0,0,0);
						pPlayer->pev->v_angle = Vector(0,0,0);
						pPlayer->pev->fixangle = TRUE;
						pPlayer->m_flVelocityModifier = -3;
						pPlayer->m_fPlayerHideMode = TRUE;
						pPlayer->m_god_time = gpGlobals->time + 3.5;
						pPlayer->m_air_oxyan = 1;

							CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
							if ( pEntity2 ){
							pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							UTIL_Remove( pEntity2 );
							}
			}
			if(pev->frags == 76){
					FireTargets( "the_high_police", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 90){
					CBaseEntity *pPolice = UTIL_FindEntityByTargetname( NULL, "police_height" );
					if ( pPolice ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pPolice->MyMonsterPointer();
						if(pEnemyMonster){
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "barn_wave" );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->m_hPlayer = pPlayer;
						//pEnemyMonster->m_rpgms_inteam = 5;
						}
						EMIT_SOUND_DYN( pEnemyMonster->edict(), 6, "!POLICE_HEY", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
					char text[256];
				
					sprintf( text, "???: Hey, over here!\n");
				
					UTIL_SayTextAll( text,this );
					
			}
			if(pev->frags == 95){
					pPlayer->m_fPlayerHideMode = FALSE;
					pPlayer->pev->flags &= ~FL_NOTARGET;
			}
			if(pev->frags == 100){
					FireTargets( "exp_crazywomen_dash", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 110){
					FireTargets( "explode_jump_glass", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 120){
					FireTargets( "explode_jump_glass2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 140){
					FireTargets( "break_mdoor", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 150){
					pPlayer->Clear_SayText();
					pPlayer->m_game_rate = 11;//��Ϸ����11%
			}
			if(pev->frags == 300){
					FireTargets( "fuckdown_exp_man_d", this, this, USE_TOGGLE, 0 );
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 8){//�¼�8 ���ɢ��ǹ
			if(pev->frags == 0){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pPlayer->m_trainning = 1;
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->origin.z -= 256;
							pPlayer->pev->angles = pSpot->pev->angles;

							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->v_angle = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
						}
			}
			if(pev->frags == 2){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin - Vector(0,0,24), Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 100;
					pEnemyMonster->pev->flags |= FL_NOTARGET;
			}
			if(pev->frags == 5){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "target_police" );
				if ( pEntity )
				{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEntity->pev->angles.y = 270;
				}
			}
			if(pev->frags == 30){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "target_police" );
				if ( pEntity )
				{
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_ZP19", VOL_NORM, 0.5, 0, PITCH_NORM );
					char text[256];
					
					sprintf( text, "Cop: What are we going to do?\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 40){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "cshl623_prevent");
				if ( pSpot ){
					pSpot->pev->health = 625;
				}
			}
			if(pev->frags == 60){
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "target_police" );
				if ( pEntity )
				{
					EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "!BA_STOP1", VOL_NORM, 0.5, 0, PITCH_NORM );
					char text[256];
				
					sprintf( text, "Cop: I'll stay here, and help anyone that needs it.\n");
					
					UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 120){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
						CBaseEntity *pGun = Create( "weapon_shotgun_def", pSpot->pev->origin - Vector(80,40,47), Vector(0,90,0), NULL );
						FireTargets( "chou_tip", this, this, USE_TOGGLE, 0 );
						pSpot->pev->origin.x -= 52;
						pSpot->pev->origin.y -= 15;
						pSpot->pev->origin.z -= 5;
						pSpot->pev->angles.x = 35;
						pPlayer->Clear_SayText();
						}
			}
			if(pev->frags == 140){
						CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
						if ( pEntity2 ){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->fixangle = TRUE;
						pPlayer->m_flVelocityModifier = -1;
						pPlayer->pev->origin = pEntity2->pev->origin + Vector(0,0,36);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;

						UTIL_Remove( pEntity2 );
						}
					
			}
			if(pev->frags == 144){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "weapon_shotgun");
						if ( pSpot ){
						pSpot->pev->origin.x += 32;
						pSpot->pev->origin.y -= 32;
						pSpot->pev->origin.z += 23;
						UTIL_SetOrigin( pSpot->pev, pSpot->pev->origin );
						}
			}
			if(pev->frags == 150){
					CBaseEntity *pPolice = UTIL_FindEntityByTargetname( NULL, "target_police" );
					if ( pPolice ){
					EMIT_SOUND_DYN( pPolice->edict(), CHAN_VOICE, "!POLICE_WP", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
					char text[256];
				
					sprintf( text, "Cop: Make sure to bring this.\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 180){
					CBaseEntity *pPolice = UTIL_FindEntityByTargetname( NULL, "target_police" );
					if ( pPolice ){
					EMIT_SOUND_DYN( pPolice->edict(), CHAN_VOICE, "!BA_WAIT1", VOL_NORM, 0.5, 0, PITCH_NORM );
					}
					char text[256];
					
					sprintf( text, "Cop: You'll have to go alone.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 190){
					FireTargets( "upstair_door1", this, this, USE_TOGGLE, 0 );
					FireTargets( "upstair_door2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 240){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 250){
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 9){//�¼�9 ���Ͽ���
			if(pev->frags == 0){
					pPlayer->m_trainning = 1;
					pPlayer->m_flVelocityModifier = 0;
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->origin.y = pev->origin.y - 128;
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
							pSpot->pev->velocity.z += 1;
						}
			}
			if(pev->frags == 2){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin - Vector(0,0,24), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 100;
					pEnemyMonster->pev->flags |= FL_NOTARGET;
					pEnemyMonster->SetBodygroup( 1, 1 );
			}
			if(pev->frags == 6){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "kouzhao" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 10){
					EMIT_SOUND( ENT(pev), CHAN_ITEM, "items/ammopickup.wav", 1, 0 );
					char text[256];
				
					sprintf( text, "- Kadoma takes the gas mask.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 50){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->pev->angles = Vector(0,270,0);
						pPlayer->pev->v_angle = Vector(0,270,0);
						pPlayer->pev->fixangle = TRUE;
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->pev->origin = pev->origin;
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity2 ){
					UTIL_Remove( pEntity2 );
					}
					CBaseEntity *pEntity3 = UTIL_FindEntityByClassname( NULL, "env_light_glow" );
					if ( pEntity3 ){
					UTIL_Remove( pEntity3 );
					}	
			}
			if(pev->frags == 60){
					pPlayer->Clear_SayText();
			}
			if(pev->frags == 65){
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 10){//�¼�10 ��������
			if(pev->frags == 0){
						pPlayer->m_trainning = 1;
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->punchangle.x = 0;
						pPlayer->pev->punchangle.y = 0;
						pPlayer->pev->punchangle.z = 0;
						pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 2){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_longming = 0;
					pSpot->pev->owner = NULL;//�����BUG�޸�
					}
			}
			if(pev->frags == 4){
				CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
			}
			if(pev->frags == 10){//Bug Fix 3.0 ɾ���������ξ���
				CBaseEntity *pEntity4 = UTIL_FindEntityByTargetname( NULL, "crazy_police" );
				if ( pEntity4 )
				{
					UTIL_Remove( pEntity4 );
				}
			}
			if(pev->frags == 20){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->origin.y += 300;
							pSpot->pev->velocity.y -= 6;
							pSpot->pev->velocity.x += 1;
							pPlayer->pev->angles = Vector(0,0,0);
							pPlayer->pev->v_angle = Vector(0,0,0);
							pPlayer->pev->fixangle = TRUE;
							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���
						}
			}
			if(pev->frags == 22){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin - Vector(0,40,0), Vector(0,90,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 1, 1 );
					pEnemyMonster->pev->health = 1;
			}
			if(pev->frags == 25){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->SetActivity ( ACT_IDLE_ANGRY );
					pEnemyMonster->m_facing_fucking_mode = 1;
					pEnemyMonster->pev->weapons = 1;
					pEnemyMonster->pev->yaw_speed = 0;
					pEnemyMonster->pev->angles.y = 270;
					}
			}
			if(pev->frags == 30){
					char text[256];
				
					sprintf( text, "???: Freeze!\n");
					
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 65){//Bug Fix 3.0 �浵��ǰ��������bug��ͻ!
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 75){
					pPlayer->m_fSelectMode = TRUE;
					pPlayer->ShowVGUIMenu(31);
					pPlayer->m_load_check = 1;
			}
			if(pev->frags == 80){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
					if ( pSpot ){
						pSpot->pev->velocity.y = 0;
						pSpot->pev->velocity.x = 0;
					}
			}
			if(pev->frags == 90){
					if(pPlayer->m_fSelectNumber == 0){
							pev->frags = 81;
							if(pPlayer->m_load_check == 0){//��Ҵ������BUG��
								pPlayer->ShowVGUIMenu(31);
								pPlayer->m_load_check = 1;
							}
					}
					else{
						pPlayer->m_fNextClearTextTime = gpGlobals->time + 0.1;
					}
			}
			if(pev->frags == 91){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();

								if(pPlayer->m_fSelectNumber == 1){
								pEnemyMonster->SetActivity ( ACT_COMBAT_IDLE );
								pEnemyMonster->pev->weapons = 3;
								pEnemyMonster->pev->frags = 3;
								pEnemyMonster->pev->health = 0;
								pEnemyMonster->pev->takedamage = DAMAGE_YES;
								pEnemyMonster->SetBodygroup( 2, 3 );
								}
								else if(pPlayer->m_fSelectNumber == 3){
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fear" );
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								}
								else if(pPlayer->m_fSelectNumber == 4){
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "fuck_you" );
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								pEnemyMonster->SetBodygroup( 2, 3 );
								pEnemyMonster->pev->frags = 3;
								pEnemyMonster->pev->health = 0;
								pEnemyMonster->pev->takedamage = DAMAGE_YES;
								}
					}					
			}
			if(pPlayer->m_fSelectNumber == 1){
				if(pev->frags == 95){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "shoot2" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->pev->yaw_speed = 0;
					pEnemyMonster->pev->angles.y = 270;
					}
				}
				if(pev->frags == 120){
					pPlayer->Killed( pev, GIB_NEVER );//����ɱ!
					return;
				}
			}
			if(pev->frags == 120){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
								CBaseMonster *pEnemyMonster;
								pEnemyMonster = pSpot->MyMonsterPointer();
								if(pPlayer->m_fSelectNumber == 4){
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence ( "shoot2" );
								}
								else if(pPlayer->m_fSelectNumber == 2){
								pPlayer->m_fMask = FALSE;
								pEnemyMonster->m_lovehate += 10;
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_DISARM );
								}
								else if(pPlayer->m_fSelectNumber == 3){
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_DISARM );
								}
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								pEnemyMonster->pev->yaw_speed = 0;
								pEnemyMonster->pev->angles.y = 270;
					}
			}
			if(pev->frags == 150){
						if(pPlayer->m_fSelectNumber == 4){
						pPlayer->Killed( pev, GIB_NEVER );//����ɱ!
						return;
						}
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->m_flVelocityModifier = -1;
						pPlayer->pev->origin = pev->origin - Vector(0,50,0);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->fixangle = TRUE;
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 0.5, 255, FFADE_IN );//��Ϲ���

					CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity2 ){
					UTIL_Remove( pEntity2 );
					}	
			}
			if(pev->frags == 151){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pSpot->pev->weapons = 0;
					pEnemyMonster->m_facing_fucking_mode = 0;
					pEnemyMonster->m_rpgms_inteam = 5;
					}
			}

			if(pev->frags == 156){
					char text[256];
					
					sprintf( text, "???: I guess you're not him.\n");
				
					UTIL_SayTextAll( text,this );
			}

			if(pev->frags == 207){
			pPlayer->Clear_SayText();
			UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000 );
			}

			if(pev->frags == 230){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");
						if ( pSpot ){
							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->origin.y += 300;
							pPlayer->pev->angles = Vector(0,250,0);
							pPlayer->pev->v_angle = Vector(0,250,0);
							pPlayer->pev->fixangle = TRUE;
							pSpot->pev->origin.x = pev->origin.x - 100;
							pSpot->pev->origin.y = pev->origin.y + 50;
							pSpot->pev->angles.y = 250;
							pSpot->pev->velocity.z += 3;
						}
			}

			if(pev->frags == 235){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					UTIL_SetOrigin ( pSpot->pev, pev->origin + Vector(0,75,0) );
					pEnemyMonster->m_facing_fucking_mode = 0;
					pSpot->pev->flags |= FL_FROZEN;
					pSpot->pev->takedamage = DAMAGE_AIM;
					pEnemyMonster->m_notarget_hide = 35;
					pEnemyMonster->m_boltpoison = 55;
					}
			}

			if(pev->frags == 240){
			FireTargets( "elev_tyant_boss", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 241){
			FireTargets( "tyant_break", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 243){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, "tyant_boss/drop.wav", 1.0, 0.5, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
					FX_Explosion( pEnemyMonster->pev->origin + Vector(0,0,4), EXPLOSION_SPARKSHOWER );
					pSpot->pev->health = pSpot->pev->max_health * 0.2;//ֻʹ��20%ʵ��
					pSpot->pev->weapons = 1;//��ʹ�ó��������
					pEnemyMonster->m_killed_exp = 200;//��ɱ����ֵ����
					}
			}
			if(pev->frags == 270){
					SERVER_COMMAND("mp3 play media/music5.mp3\n");
			}
			if(pev->frags == 275){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->m_flVelocityModifier = -1;
						pPlayer->pev->origin = pev->origin - Vector(0,50,0);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->m_fPlayerHideMode = TRUE;
						pPlayer->pev->flags |= FL_NOTARGET;
						pPlayer->m_fSelectMode = FALSE;
			}

			if(pev->frags == 285){
					char text[256];
					sprintf( text, "???: !!!\n");
					UTIL_SayTextAll( text,this );
			}

			if(pev->frags == 290){
					pPlayer->m_fPlayerHideMode = FALSE;
					pPlayer->pev->flags &= ~FL_NOTARGET;
			}

			if(pev->frags == 325){
					pPlayer->Clear_SayText();
			}

			if(pev->frags == 340){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
					CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera3");

					if ( pSpot ){
						if(pSpot->pev->deadflag == DEAD_NO){
							pev->frags = 310;
						}
						else{
							if(pPlayer->pev->deadflag != DEAD_NO)
							return;//�Ѿ������ˣ�����

							SERVER_COMMAND("mp3 stop\n");

							UTIL_SetSize( pSpot->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
							UTIL_SetOrigin (pSpot->pev, pev->origin + Vector ( -120 , -50 , 1) );
							pSpot->pev->angles.y = 60;
							pSpot->pev->yaw_speed = 0;
							DROP_TO_FLOOR ( ENT(pSpot->pev) );

							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							SET_VIEW( pPlayer->edict(), pSpot2->edict() );
							pPlayer->m_player_camera = pSpot2;
							pPlayer->pev->origin.y = pSpot->pev->origin.x + 60;
							pPlayer->pev->origin.y = pSpot->pev->origin.y + 300;
							pPlayer->pev->angles = Vector(0,250,0);
							pPlayer->pev->v_angle = Vector(0,250,0);
							pPlayer->pev->fixangle = TRUE;
							pSpot2->pev->origin.x = pSpot->pev->origin.x;
							pSpot2->pev->origin.y = pSpot->pev->origin.y + 100;
							pSpot2->pev->origin.z = pev->origin.z + 20;
							pSpot2->pev->angles.y = 270;
							pSpot2->pev->velocity.z = 3;

							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.25, 255, FFADE_IN );//��Ϲ���
						}
					}
			}

			if(pev->frags == 390){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
						if(pSpot->pev->deadflag == DEAD_NO){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						UTIL_SetOrigin (pSpot->pev, pev->origin + Vector(0,75,0) );
						pEnemyMonster->m_facing_fucking_mode = 0;
						pEnemyMonster->m_lovehate = 1;
						pEnemyMonster->pev->takedamage = DAMAGE_NO;
						}
					}
			}

			if(pev->frags == 407){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->m_flVelocityModifier = -1;
						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->fixangle = TRUE;
						pPlayer->pev->origin = pev->origin - Vector(0,50,0);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->pev->flags |= FL_NOTARGET;
			}

			if(pev->frags == 415){
					char text[256];
					
					sprintf( text, "???: Thank you for saving me.\n");
					
					UTIL_SayTextAll( text,this );
			}

			if(pev->frags == 450){
				FireTargets( "elev_door_inside", this, this, USE_TOGGLE, 0 );
			}

			if(pev->frags == 455){
				FireTargets( "elev_door_outside", this, this, USE_TOGGLE, 0 );
			}

			if(pev->frags == 460){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_longming = 1;
					}
			}

			if(pev->frags == 470){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						UTIL_SetOrigin (pSpot->pev, Vector(pev->origin.x-55,pev->origin.y+75,pSpot->pev->origin.z) );
						pEnemyMonster->SetActivity ( ACT_CROUCH );
						pEnemyMonster->pev->yaw_speed = 0;
						pEnemyMonster->pev->angles.y = 90;
						pEnemyMonster->pev->weapons = 2;
						//pEnemyMonster->TakeHealth(40, DMG_GENERIC);//�ָ���������ֵ
						pPlayer->TeamMate_add(pEnemyMonster);//������Ҥζ���
						pev->origin = pev->origin + Vector(-55,155,0);
					}
			}
			if(pev->frags == 472){
						pPlayer->Clear_SayText();
			}
			if(pev->frags == 475){//����һ��һ��һ��һ��������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if(pEntity->pev->deadflag != DEAD_NO && (pEntity->pev->flags & FL_MONSTER)){
						UTIL_Remove( pEntity );//�������ʬ��
						}
					}
			}
			if(pev->frags == 480){
						if(pPlayer->pev->origin.y >= pev->origin.y || fabs( pev->origin.x - pPlayer->pev->origin.x ) > 200){
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						FireTargets( "npc_nosic_scream", this, this, USE_TOGGLE, 0 );
						}
						else{
						pev->frags = 475;
						}
			}
			if(pev->frags == 490){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "cshl623_prevent");
				if ( pSpot ){
					UTIL_Remove( pSpot );
				}
				CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "give_shotgun_event" );
				if ( pEntity3 )
				{
					UTIL_Remove( pEntity3 );
				}
			}
			if(pev->frags == 492){
					CBaseEntity *pAirWall = Create( "wrongdoor_airwall", pev->origin+Vector(-10,-64,-64), Vector(0,0,0), NULL );
					UTIL_SetSize ( pAirWall->pev, Vector(-64,-16,-96), Vector(64,16,96));
					//���ݤο���ǽ
					FireTargets( "elev_out_monsters", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 495){
					pPlayer->EnableControl(TRUE);
					pPlayer->m_flVelocityModifier = 0;

					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->pev->takedamage = DAMAGE_YES;
						pEnemyMonster->m_alert = 100;
						pEnemyMonster->m_lovehate = 120;//ʥ�κø�120
						pEnemyMonster->m_longming = 0;
						pEnemyMonster->m_enemyfollower = 1;
						pEnemyMonster->m_facing_fucking_mode = 0;
						pEnemyMonster->pev->weapons = 0;
					}
			}
			if(pev->frags == 498){
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 505){
					FireTargets( "outside_j_wood1", this, this, USE_TOGGLE, 0 );
					FireTargets( "outside_j_glass1", this, this, USE_TOGGLE, 0 );
					FireTargets( "outside_j_monster1", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 507){
					SERVER_COMMAND("mp3 play media/music7.mp3\n");
			}
			if(pev->frags == 510){
					FireTargets( "outside_j_wood2", this, this, USE_TOGGLE, 0 );
					FireTargets( "outside_j_glass2", this, this, USE_TOGGLE, 0 );
					FireTargets( "outside_j_monster2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 530){
			pPlayer->m_game_rate = 16;//��Ϸ����16%
			UTIL_Remove( this );
			return;
			}
	}
	else if(pev->armortype == 11){//�¼�11 ͨ������
			if(pev->frags == 30){
				SERVER_COMMAND("mp3 stop\n");
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "computer_camera" );
				if ( pEntity )
				{
					Create( "wrongdoor_camera4", pEntity->pev->origin, Vector(0,270,0), NULL );
				}
			}
			if(pev->frags == 40){
					FireTargets( "ting_corpse_door", this, this, USE_TOGGLE, 0 );

					//Bug Fix 3.0 ���վλ�����ݳ�����
					pPlayer->pev->origin = pev->origin + Vector(0,0,16);
					pPlayer->m_stuck_origin = pPlayer->pev->origin;

					pPlayer->m_trainning = 1;
					pPlayer->m_flVelocityModifier = 0;

					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3, 3, 255, FFADE_IN );//��Ϲ���
			}
			if(pev->frags == 45){//����һ��һ��һ��һ��������������ʵ�壡
					int alive_npc = 0;
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if ( !FClassnameIs ( pEntity->pev, "monster_saintna" ) && (pEntity->pev->flags & FL_MONSTER) ){
							if(pEntity->Classify() == CLASS_PLAYER_ALLY && pEntity->pev->deadflag == DEAD_NO){
							alive_npc += 1;
							}
							else if(pEntity->Classify() != CLASS_PLAYER_ALLY && pEntity->pev->deadflag != DEAD_NO){
							UTIL_Remove( pEntity );//���ʬ��
							}
						}
						if ( FClassnameIs ( pEntity->pev, "gib" ) ){
							UTIL_Remove( pEntity );//������
						}
					}
			}
			if(pev->frags == 50){
				Vector org1,ang1,org2,ang2;
				CBaseEntity *pDeadpoint = UTIL_FindEntityByTargetname( NULL, "dead_corpse_sitpoint" );
				if ( pDeadpoint )
				{
					org1 = pDeadpoint->pev->origin + Vector(0,0,4);
					ang1 = pDeadpoint->pev->angles;
				}
				CBaseEntity *pDeadpoint2 = UTIL_FindEntityByTargetname( NULL, "dead_corpse_sitpoint2" );
				if ( pDeadpoint2 )
				{
					org2 = pDeadpoint2->pev->origin + Vector(0,0,4);
					ang2 = pDeadpoint2->pev->angles;
				}
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc1" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org1;
					pEntity->pev->angles = ang1;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc2" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org1 + Vector(90,0,0);
					pEntity->pev->angles = ang1;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc3" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org1 + Vector(180,0,0);
					pEntity->pev->angles = ang1;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc4" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org1 + Vector(270,0,0);
					pEntity->pev->angles = ang1;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc5" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org1 + Vector(360,0,0);
					pEntity->pev->angles = ang1;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc6" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org2;
					pEntity->pev->angles = ang2;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc7" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org2 + Vector(90,0,0);
					pEntity->pev->angles = ang2;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "surive_npc8" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org2 + Vector(180,0,0);
					pEntity->pev->angles = ang2;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
				pEntity = UTIL_FindEntityByTargetname( NULL, "target_police" );
				if ( pEntity )
				{
					if(pEntity->pev->deadflag != DEAD_NO){
					pEntity->pev->origin = org2 + Vector(270,0,0);
					pEntity->pev->angles = ang2;
					}
					else{
					UTIL_Remove( pEntity );
					}
				}
			}
			if(pev->frags == 60){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot ){
							pev->origin = pSpot->pev->origin;

							pPlayer->EnableControl(FALSE);
							pPlayer->Clear_SayText();
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
						}
			}
			
			if(pev->frags == 62){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
						
						pSpot->pev->flags |= FL_NOTARGET;
						pSpot->pev->takedamage = DAMAGE_NO;
						pSpot->pev->angles.y = 0;
						pSpot->pev->yaw_speed = 0;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->ClearSchedule();
						pEnemyMonster->m_enemyfollower = 0;
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->RouteClear();
						pEnemyMonster->SetActivity( ACT_IDLE );
						pEnemyMonster->pev->velocity.x = 0;
						pEnemyMonster->pev->velocity.y = 0;
						pEnemyMonster->pev->velocity.z += 1;

						CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
						if ( pSpot2 ){
						UTIL_SetOrigin (pSpot->pev, pSpot2->pev->origin - Vector(64,128,48) );//ʥ�Τ�˲�䴫��
						}
					}
			}

			if(pev->frags == 64){
					Vector cl_org;
					CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot2 ){
					cl_org = pSpot2->pev->origin - Vector(-64,128,48);
					}
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", cl_org, Vector(0,180,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 1, 1 );
			}

			if(pev->frags == 100){
					char text[256];
				
					sprintf( text, "???: I should introduce myself first.\n");
				
					UTIL_SayTextAll( text,this );

					pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		
			}

			if(pev->frags == 141){
					CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot2 ){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
						if ( pSpot ){
						pSpot2->pev->angles.y = 180;
						pSpot2->pev->origin = pSpot->pev->origin + Vector(30,15,60);
						}
					}
					char text[256];
			
					sprintf( text, "Saintna: I'm Saintna. And you?\n");
					
					UTIL_SayTextAll( text,this );
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );

					pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		
			}
			if(pev->frags == 183){
					CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					if ( pSpot2 ){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_cleaner");
						if ( pSpot ){
						pSpot2->pev->angles.y = 0;
						pSpot2->pev->origin = pSpot->pev->origin + Vector(-30,15,65);
						}
					}
			}
			if(pev->frags == 213){
					CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera4");
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot2 ){
						if ( pSpot ){
						pSpot2->pev->angles.y = 180;
						pSpot2->pev->origin = pSpot->pev->origin + Vector(30,15,60);
						}
					}
					char text[256];
		
					sprintf( text, "Saintna: Kadoma? Okay.\n");
				
					UTIL_SayTextAll( text,this );
					pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		
					pPlayer->m_mode_float1 = pSpot->pev->health;
			}
			if(pev->frags == 257){
					char text[256];
					
					sprintf( text, "Saintna: I found a weird key earlier.\n");
				
					UTIL_SayTextAll( text,this );
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2, 2, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->m_flVelocityModifier = -1;
						pPlayer->pev->angles = Vector(0,180,0);
						pPlayer->pev->v_angle = Vector(0,180,0);
						pPlayer->pev->fixangle = TRUE;
						pPlayer->pev->origin = pev->origin - Vector(-64,128,0);
						pPlayer->m_stuck_origin = pPlayer->pev->origin;
						pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;	
			}
			if(pev->frags == 262){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity ){
				UTIL_Remove( pEntity );
				}
			}
			if(pev->frags == 264){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					//CBaseEntity *pSpot = Create("monster_saintna", Vector(426,-1695,639), Vector(0,0,0) );
					if ( pSpot ){
						UTIL_SetOrigin (pSpot->pev, Vector(426,-1695,639) );
						pSpot->pev->angles.y = 0;
						pSpot->pev->yaw_speed = 0;
						pSpot->pev->takedamage = DAMAGE_NO;
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->SetActivity( ACT_TWITCH );
						//pEnemyMonster->SetThink( NULL );
						pEnemyMonster->SetState( MONSTERSTATE_HUNT );
						pEnemyMonster->SetBodygroup( 6, 1 );
						//pEnemyMonster->m_lovehate = 0;
					}
			}
			if(pev->frags == 277){
						UTIL_CenterPrintAll( "You got the Gold Key!" );
						pPlayer->m_fGlodenKey += 1;
						if(!pPlayer->HasMenuItem_Full()){
						pPlayer->MenuItem_add(3);//��ɫԿ��Get
						}
						MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
						WRITE_STRING( "monster_eatkey" );
						MESSAGE_END();

						pPlayer->m_game_rate = 17;//��Ϸ����17%
			}
			if(pev->frags == 295){
					FireTargets( "upstair_door1", this, this, USE_TOGGLE, 0 );
					FireTargets( "upstair_door2", this, this, USE_TOGGLE, 0 );
			}
			if(pev->frags == 310){
					char text[256];
				
					sprintf( text, "Saintna: I'm tired. I'll rest here.\n");
					
					UTIL_SayTextAll( text,this );
					pPlayer->m_fNextClearTextTime = gpGlobals->time + 4.0;		
			}
			if(pev->frags == 380){
					char text[256];
					
					sprintf( text, "Saintna: I'm sorry for being so aggressive.\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 420){
					char text[256];
					
					sprintf( text, "Saintna: My team was attacked by someone that looks like you.\n");
				
					UTIL_SayTextAll( text,this );	
			}
			if(pev->frags == 460){
					char text[256];
				
					sprintf( text, "Saintna: I hope they're okay. I don't know how they're doing...\n");
				
					UTIL_SayTextAll( text,this );
					pPlayer->m_fNextClearTextTime = gpGlobals->time + 6.0;		
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 12){//�¼�12 ��ˮ��ǹ��
			if(pev->frags == 0){
					pPlayer->m_trainning = 1;
					pPlayer->m_flVelocityModifier = 0;
					//�������
					pPlayer->TeamMate_Nagamatagi_Allclear(1);
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 2, 255, FFADE_IN );//��Ϲ���
						pPlayer->pev->origin.y += 64;
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->pev->angles = Vector(0,0,0);
						pPlayer->pev->v_angle = Vector(0,0,0);
						pPlayer->pev->punchangle.x = 0;
						pPlayer->pev->punchangle.y = 0;
						pPlayer->pev->punchangle.z = 0;
						pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 6){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->m_music_save = 1;
							CLIENT_COMMAND(pPlayer->edict(), "cd loop 4\n");
							//SERVER_COMMAND("mp3 loop media/music8.mp3\n");
							pSpot->pev->velocity.x = 20;
						}
			}
			if(pev->frags == 12){
					CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin + Vector(0,-32,0), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pCleaner1->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 2, 4 );
					pEnemyMonster->SetBodygroup( 1, 1 );
					pEnemyMonster->pev->weapons = 4;
					pEnemyMonster->m_walkaround = TRUE;
					SetBits(pCleaner1->pev->effects, EF_DIMLIGHT);
			}
			if(pev->frags == 75){
				CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
				if ( pEntity3 )
				{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity3->MyMonsterPointer();
					pEnemyMonster->m_MonsterState		= MONSTERSTATE_NONE;//ʥ�Ρ�ֹͣ��˼��
					pEnemyMonster->SetActivity( ACT_GUARD );
					pEnemyMonster->SetThink( NULL );
					pEnemyMonster->SetBodygroup( 1, 3 );
					pEnemyMonster->SetBodygroup( 5, 2 );
					SetBits( pEntity3->pev->effects, EF_DIMLIGHT);

					EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, "!BA_TR6", VOL_NORM, 0.5, 0, PITCH_NORM-30 );
					//Bug Fix 3.0 ��ɷ��ӵľ����������
					char text[256];
				
					sprintf( text, "???: Die you vampire!\n");
				
					UTIL_SayTextAll( text,this );
				}
			}
			if(pev->frags == 78){
				CBaseEntity *pEntity4 = UTIL_FindEntityByTargetname( NULL, "crazy_police" );
				if ( pEntity4 )
				{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity4->MyMonsterPointer();
					pEnemyMonster->m_facing_fucking_mode = 1;
					pEnemyMonster->m_aimenemy_mod = 6;
					pEnemyMonster->m_chase_mode = -1;
					pEnemyMonster->m_no_cover_mode = 1;
					pEnemyMonster->m_no_pov_limit = 1;
					pEnemyMonster->SetBodygroup( 2, 2 );//�ֵ�Ͳ����
				}
			}
			if(pev->frags == 80){
				CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
				if ( pEntity3 )
				{
					pEntity3->pev->origin.y -= 10;
					CBaseEntity *pAimTarget = Create( "monster_cof_ms1", pEntity3->pev->origin + Vector(32,4,-16), Vector(0,270,0), NULL );
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pAimTarget->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 600;
					pEnemyMonster->pev->solid	= SOLID_NOT;
					pEnemyMonster->pev->effects |= EF_NODRAW;
					pEnemyMonster->pev->targetname = MAKE_STRING("police_shoot_target_ms1");
				}
			}
			if(pev->frags == 85){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_movementActivity = ACT_RUN;
				pEnemyMonster->m_flDistLook		= 96.0;
				}
			}
			if(pev->frags == 100){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->avelocity.y = -50;
				}
			}
			if(pev->frags == 120){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pSpot->pev->avelocity.y = 0;
							pSpot->pev->frags = 1;
							pSpot->pev->armortype = 4;
							pSpot->pev->velocity.y -= 1;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
							pPlayer->m_fNextClearTextTime = gpGlobals->time + 0.2;		
						}
			}
			if(pev->frags == 130){
				CBaseEntity *pEntity4 = UTIL_FindEntityByTargetname( NULL, "crazy_police" );
				if ( pEntity4 )
				{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity4->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 35;
					pEnemyMonster->m_flFieldOfView = 0.5;
					pEnemyMonster->m_godmode = FALSE;
					pEnemyMonster->pev->health = 10;
					pEnemyMonster->pev->max_health = 10;
				}
			}
			if(pev->frags == 140){
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity ){
				UTIL_Remove( pEntity );
				}
			}
			if(pev->frags == 170){
				CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
				if ( pSpot ){
					pSpot->pev->avelocity.y = -40;
				}
			}
			if(pev->frags == 185){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
					if ( pSpot ){
						pSpot->pev->armortype = 5;
					}
			}
			if(pev->frags == 190){
					FireTargets( "wait_for_police_ms8", this, this, USE_TOGGLE, 0 );
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera1");
						if ( pSpot ){
							pSpot->pev->avelocity.y = 0;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->fixangle = TRUE;
						}
			}
			if(pev->frags == 192){
				CBaseEntity *pEntity4 = UTIL_FindEntityByTargetname( NULL, "crazy_police" );
				if ( pEntity4 )
				{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity4->MyMonsterPointer();
					pEnemyMonster->m_boltpoison = 50;
				}
			}
			if(pev->frags == 210){
				CBaseEntity *pEntity4 = UTIL_FindEntityByTargetname( NULL, "police_shoot_target_ms1" );
				if ( pEntity4 )
				{
					UTIL_Remove( pEntity4 );
				}
			}
			if(pev->frags == 230){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->m_trainning = 0;
						pPlayer->m_flVelocityModifier = -1;

						pPlayer->pev->angles = Vector(0,0,0);
						pPlayer->pev->fixangle = TRUE;

						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
							
						CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
						if ( pEntity3 )
						{
							pPlayer->pev->origin = pEntity3->pev->origin - Vector(300,-100,-40);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity3->MyMonsterPointer();
							pEnemyMonster->m_godmode = FALSE;
							pEnemyMonster->m_selfmode = FALSE;
							pEnemyMonster->m_longming = 0;
							pEntity3->pev->origin.y += 10;
							pEntity3->Spawn();
							pEnemyMonster->SetBodygroup( 5, 2 );
							if(pPlayer->m_mode_float1 > 0){
							pEntity3->pev->health = pPlayer->m_mode_float1;//�̳��ϴε�����ֵ
							}
							//ALERT ( at_console, "Saintna Health %f\n", pPlayer->m_mode_float1 );
							pEnemyMonster->TakeHealth(pEntity3->pev->max_health * 0.6, DMG_GENERIC);//�ָ�60%����ֵ
							pEnemyMonster->m_rpgms_level += 2;
							pPlayer->TeamMate_add(pEnemyMonster);
							//pEnemyMonster->m_groundElev2 = TRUE;//�����ݿ�ס
						}
			}
			if(pev->frags == 240){
						CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
						if ( pEntity3 )
						{
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity3->MyMonsterPointer();
							pEnemyMonster->m_alert = 100;
							pEnemyMonster->m_enemyfollower = 1;
						}
			}
			if(pev->frags == 250){
					CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "monster_clip_node_wall" );
					if ( pEntity3 )
					{
						UTIL_Remove( pEntity3 );
					}
					char text[256];
				
					sprintf( text, "Saintna: Where did you go? I was looking for you...\n");
				
					UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 290){
						CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
						if ( pEntity3 )
						{
							CBaseMonster *pEnemyMonster;
							pEnemyMonster = pEntity3->MyMonsterPointer();
							pEnemyMonster->m_alert = 100;
							pEnemyMonster->m_FTSmod = 4;
						}
						char text[256];
					
						sprintf( text, "Saintna: Come on, let's get out here.\n");
					
						UTIL_SayTextAll( text,this );
			}
			if(pev->frags == 300){//����һ��һ��һ��һ��������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if ( FClassnameIs ( pEntity->pev, "monster_cof_ms9" ) 
						|| FClassnameIs ( pEntity->pev, "monster_scientist_yell" ) ){
						UTIL_Remove( pEntity );//ɾ��¥�ϲ���Ҫ��ʵ��
						}
					}
			}
			if(pev->frags == 350){
					pPlayer->Clear_SayText();
					//if (g_iSkillLevel == SKILL_HARD){//����ģʽ�ӹ�
					//FireTargets( "follower_cofms7", this, this, USE_TOGGLE, 0 );
					//}
					pPlayer->m_game_rate = 20;//��Ϸ����20%
			}
			if(pev->frags == 400){
				CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
				if ( pEntity3 )
				{
					if ( pEntity3->pev->deadflag == DEAD_NO ){
					char text[256];
				
					sprintf( text, "Saintna: I think I had a misunderstanding with that cop.\n");
					
					UTIL_SayTextAll( text,this );
					}
				}
			}
			if(pev->frags == 450){
				CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "saintna_sew" );
				if ( pEntity3 )
				{
					if ( pEntity3->pev->deadflag == DEAD_NO ){
						char text[256];
					
						sprintf( text, "Saintna: I know the way. Follow me.\n");
					
					UTIL_SayTextAll( text,this );
					}
				}
			}
			if(pev->frags == 520){
					pPlayer->Clear_SayText();
					UTIL_Remove( this );
					return;
			}
	}
	else if(pev->armortype == 13){//�¼�13 ��������BOSSս
			if(pev->frags == 0){
						pPlayer->m_trainning = 1;
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->m_music_save = 0;
						pPlayer->m_level_up_switch = TRUE;
					SERVER_COMMAND( "autosave\n" );
			}
			if(pev->frags == 1){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 2, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->pev->angles = Vector(0,180,0);
						pPlayer->pev->v_angle = Vector(0,180,0);
						pPlayer->pev->punchangle.x = 0;
						pPlayer->pev->punchangle.y = 0;
						pPlayer->pev->punchangle.z = 0;
						pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 2){
				SERVER_COMMAND( "mp3 stop\n" );
			}
			if(pev->frags == 3){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->m_longming = 1;
					pEnemyMonster->m_boltpoison = 50;
					pEnemyMonster->m_enemyfollower = 0;
					pEnemyMonster->m_hEnemy = NULL;
					pEnemyMonster->m_hOldEnemy[0] = NULL;
					pEnemyMonster->m_hOldEnemy[1] = NULL;
					pEnemyMonster->m_hOldEnemy[2] = NULL;
					pEnemyMonster->m_hOldEnemy[3] = NULL;
					pEnemyMonster->ClearSchedule();
					pEnemyMonster->SetActivity( ACT_IDLE );
					pSpot->pev->frags = 2;
					pEnemyMonster->m_lovehate += 30;
					pEnemyMonster->RouteClear();	
					pSpot->pev->weapons = 5;
					UTIL_SetOrigin (pSpot->pev, Vector(pev->origin.x + 40,pev->origin.y - 64,pSpot->pev->origin.z) );
					}
			}
			if(pev->frags == 6){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pSpot->pev->origin.x = pev->origin.x + 18;
							pSpot->pev->origin.y = pev->origin.y;
						}
			}
			if(game_boss_battle == 1){//BOSSս��ͷ��������
				if(pev->frags == 8){
						FireTargets( "boss_clear_ents", this, this, USE_TOGGLE, 0 );
				}

				if(pev->frags == 10){//����һ��һ��һ��һ��������������ʵ�壡
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) ){
								if ( !FClassnameIs(pEntity->pev, "monster_cleaner") 
								&& !FClassnameIs(pEntity->pev, "monster_saintna") 
								&& !FClassnameIs(pEntity->pev, "monster_eatkey") 
								&& !FClassnameIs(pEntity->pev, "monster_tyant_boss") ){
								UTIL_Remove( pEntity );
								}
							}
						}
				}
				if(pev->frags == 12){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->avelocity.y = -90;
						pSpot->pev->velocity.x = -30;
					}
				}
				if(pev->frags == 32){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->avelocity.y = 0;
						pSpot->pev->velocity.x = 0;
					}
				}
				if(pev->frags == 33){
					FireTargets( "tyant_boss_maker", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 37){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_longming = 1;
						pEnemyMonster->pev->sequence = 7;
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->m_boltpoison = 40;
						pEnemyMonster->pev->flags |= FL_NOTARGET;
						}
				}
				if(pev->frags == 40){
					FireTargets( "tyant_break_door", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 60){
							SET_VIEW( pPlayer->edict(), pPlayer->edict() );
							pPlayer->EnableControl(TRUE);
							pPlayer->m_trainning = 0;
							pPlayer->m_flVelocityModifier = 0;

							pPlayer->pev->angles = Vector(0,90,0);
							pPlayer->pev->fixangle = TRUE;

							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���

							pPlayer->pev->origin = pev->origin - Vector(0,512,0);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							pPlayer->BOSS_Find();
				}
				if(pev->frags == 62){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_longming = 0;
						pEnemyMonster->m_enemyfollower = 1;
						pEnemyMonster->m_alert = 100;
						pEnemyMonster->RouteClear();
						UTIL_SetOrigin (pSpot->pev, Vector(pev->origin.x + 64,pev->origin.y - 512,pSpot->pev->origin.z) );
						}
				}
				if(pev->frags == 64){
						FireTargets( "boss_pusher", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 65){
						pPlayer->m_music_save = 5;
						CLIENT_COMMAND(pPlayer->edict(), "cd loop 10\n");
						//SERVER_COMMAND("mp3 loop media/boss1.mp3\n");
				}
				if(pev->frags == 70){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_iTriggerCondition = 4;
						pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("tyant_boss_die_event");
						}
				}
				if(pev->frags == 80){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->pev->flags &= ~FL_NOTARGET;
						}
				}
				if(pev->frags == 100){
						UTIL_Remove( this );
						return;
				}
			}
			else{
				if(pev->frags == 10){
						CBaseEntity *pCleaner1 = Create( "monster_cleaner", pev->origin + Vector(0,-64,0), Vector(0,270,0), NULL );
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pCleaner1->MyMonsterPointer();
						pEnemyMonster->SetBodygroup( 2, 3 );
						pEnemyMonster->SetBodygroup( 1, 1 );
						pEnemyMonster->m_walkaround = TRUE;
				}
				if(pev->frags == 15){
						FireTargets( "saintna_boss_walk", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 18){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pSpot->pev->velocity.y = -20;
						}
				}
				if(pev->frags == 30){
						FireTargets( "boss_clear_ents", this, this, USE_TOGGLE, 0 );
				}

				if(pev->frags == 40){//����һ��һ��һ��һ��������������ʵ�壡
						CBaseEntity *pEntity = NULL;
						while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
						{
							if (  (pEntity->pev->flags & FL_MONSTER) ){
								if ( !FClassnameIs(pEntity->pev, "monster_cleaner") 
								&& !FClassnameIs(pEntity->pev, "monster_saintna") 
								&& !FClassnameIs(pEntity->pev, "monster_eatkey") 
								&& !FClassnameIs(pEntity->pev, "monster_tyant_boss") ){
								UTIL_Remove( pEntity );
								}
							}
						}
				}
				if(pev->frags == 72){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->avelocity.y = -90;
						pSpot->pev->velocity.y = 30;
						pSpot->pev->velocity.x = -30;
					}
				}
				if(pev->frags == 92){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->avelocity.y = 0;
						pSpot->pev->velocity.x = 0;
					}
				}
				if(pev->frags == 105){
					FireTargets( "tyant_boss_maker", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 112){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_longming = 1;
						pEnemyMonster->pev->sequence = 7;
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						pEnemyMonster->m_boltpoison = 63;
						pEnemyMonster->pev->flags |= FL_NOTARGET;
						}
				}
				if(pev->frags == 115){
					FireTargets( "tyant_break_door", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 116){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->velocity.y = 10;
					}
				}
				if(pev->frags == 124){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_walkaround = TRUE;
						}
				}
				if(pev->frags == 140){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity ){
					UTIL_Remove( pEntity );
					}
				}
				if(pev->frags == 163){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->m_longming = 0;
						pEnemyMonster->m_enemyfollower = 1;
						pEnemyMonster->m_alert = 100;
						pEnemyMonster->m_groundElev2 = FALSE;
						pEnemyMonster->RouteClear();
						}
				}
				if(pev->frags == 165){
							SET_VIEW( pPlayer->edict(), pPlayer->edict() );
							pPlayer->EnableControl(TRUE);
							pPlayer->m_trainning = 0;
							pPlayer->m_flVelocityModifier = 0;

							pPlayer->pev->angles = Vector(0,90,0);
							pPlayer->pev->fixangle = TRUE;

							UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���

							pPlayer->pev->origin = pev->origin - Vector(0,512,0);
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							pPlayer->BOSS_Find();
				}
				if(pev->frags == 168){
						pPlayer->m_music_save = 5;
						CLIENT_COMMAND(pPlayer->edict(), "cd loop 10\n");
						//SERVER_COMMAND("mp3 loop media/boss1.mp3\n");
				}
				if(pev->frags == 170){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->RouteClear();
						pEnemyMonster->m_walkaround = FALSE;
						pEnemyMonster->m_iTriggerCondition = 4;
						pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("tyant_boss_die_event");
						}
				}
				if(pev->frags == 177){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->pev->flags &= ~FL_NOTARGET;
						}
				}
				if(pev->frags == 180){
						FireTargets( "boss_pusher", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 200){
						game_boss_battle = 1;
					//	FireTargets( "tyant_pys_maker", this, this, USE_TOGGLE, 0 );
						UTIL_Remove( this );
						return;
				}
			}
			
	}
	else if(pev->armortype == 14){//�¼�14 ��һ���½��
			if(pev->frags == 0){
					SERVER_COMMAND("mp3 stop\n");
						if(pPlayer->pev->deadflag != DEAD_NO)
						return;//�Ѿ������ˣ�����

						pPlayer->m_trainning = 1;
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->m_music_save = 0;

						game_boss_battle = 0;
			}
			if(pev->frags == 1){
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1, 1, 255, FFADE_IN );//��Ϲ���
						pPlayer->EnableControl(FALSE);
						pPlayer->Clear_SayText();
						pPlayer->pev->angles = Vector(0,90,0);
						pPlayer->pev->v_angle = Vector(0,90,0);
						pPlayer->pev->punchangle.x = 0;
						pPlayer->pev->punchangle.y = 0;
						pPlayer->pev->punchangle.z = 0;
						pPlayer->pev->fixangle = TRUE;
			}
			if(pev->frags == 3){//����һ��һ��һ��һ��������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
					{
						if ( FClassnameIs ( pEntity->pev, "monstermaker" ) || (pEntity->pev->flags & FL_MONSTER) ){
							if ( !FClassnameIs(pEntity->pev, "monster_saintna") 
							&& !FClassnameIs(pEntity->pev, "monster_tyant_boss")
							&& !FClassnameIs(pEntity->pev, "monster_eatkey") ){
							UTIL_Remove( pEntity );
							}
						}
					}
			}
			if(pev->frags == 5){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							SET_VIEW( pPlayer->edict(), pSpot->edict() );
							pPlayer->m_player_camera = pSpot;
							pPlayer->pev->angles = pSpot->pev->angles;
							pPlayer->pev->origin = pev->origin + Vector(0,-512,0);
							pSpot->pev->origin = pev->origin + Vector(0,-128,8);
							pSpot->pev->velocity.z += 2;
							pSpot->pev->velocity.y = 0;
							pSpot->pev->velocity.x = 0;
						}
			}
		
			if(pev->frags == 6){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->RouteClear();
					pEnemyMonster->SetState( MONSTERSTATE_NONE );//ʥ�Ρ�ֹͣ��˼��
					pEnemyMonster->SetActivity( ACT_IDLE );
					pEnemyMonster->SetBodygroup( 5, 0 );
					pEnemyMonster->ClearSchedule();
					pEnemyMonster->m_longming = 1;
					pEnemyMonster->m_enemyfollower = 0;
					pEnemyMonster->m_hEnemy = NULL;
					pEnemyMonster->m_hOldEnemy[0] = NULL;
					pEnemyMonster->m_hOldEnemy[1] = NULL;
					pEnemyMonster->m_hOldEnemy[2] = NULL;
					pEnemyMonster->m_hOldEnemy[3] = NULL;
					UTIL_SetOrigin (pSpot->pev, Vector(pev->origin.x,pev->origin.y - 256,pSpot->pev->origin.z) );
					DROP_TO_FLOOR ( ENT(pSpot->pev) );
					pSpot->pev->angles.y = 90;
					}
			}

			if(pev->frags == 7){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->SetState( MONSTERSTATE_NONE );
					pEnemyMonster->ClearSchedule();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_CROUCH );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					UTIL_SetOrigin (pSpot->pev, pev->origin );
					pSpot->pev->angles.y = 270;
					DROP_TO_FLOOR ( ENT(pSpot->pev) );
					}
			}
			if(pev->frags == 70){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
						if(pSpot->pev->deadflag != DEAD_NO){//ʥ����͸��
							pev->weapons = 1419;
							pev->frags = 359;
						}
					}
			}
			if(pev->frags == 72){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(32,-64,18);
						pSpot->pev->velocity.z = 0;
						pSpot->pev->velocity.y = 0;
						pSpot->pev->velocity.x = -2;
						pSpot->pev->angles.y = 180;
					}
			}
			if(pev->frags == 74){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pSpot->pev->origin =  Vector(pev->origin.x,pev->origin.y - 110,pSpot->pev->origin.z);
					pSpot->pev->angles.y = 90;
					}
			}
			if(pev->frags == 78){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_ARM );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					}
			}
			if(pev->frags == 120){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->angles.y = 0;
						pSpot->pev->origin = pev->origin + Vector(-32,-64,16);
					}
			}
			if(pev->frags == 154){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->velocity.x = -60;
						pSpot->pev->velocity.y = -15;
						pSpot->pev->velocity.z = 20;
					}
			}
			if(pev->frags == 156){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_tyant_boss");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_GUARD );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					UTIL_SetOrigin (pSpot->pev, pev->origin );
					pSpot->pev->angles.y = 270;
					DROP_TO_FLOOR ( ENT(pSpot->pev) );
					}
			}
			if(pev->frags == 160){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
					if ( pSpot ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pSpot->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_LEAP );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->pev->solid = SOLID_NOT;
					}
			}
			if(pev->weapons == 1419){//������
				if(pev->frags == 360){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 5.0, 255, FFADE_OUT );//��Ϲ���
				}
				if(pev->frags == 385){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "bluecar_toggle" );
					if ( pEntity )
					{
						pEntity->pev->rendermode = 0;
					}
				}
				if(pev->frags == 393){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 1.0, 1.0, 255, FFADE_IN );
					pPlayer->m_ending_frags -= 5;//ʥ����������Ʒֵ-5%
				}
				if(pev->frags == 395){//����һ��һ��һ��һ��������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if (  (pEntity->pev->flags & FL_MONSTER) ){
						UTIL_Remove( pEntity );
						}
					}
				}
				if(pev->frags == 400){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(-64,-320,48);
						pSpot->pev->angles.y = 270;
						pSpot->pev->velocity = g_vecZero;
						pSpot->pev->velocity.z = 8;
					}
				}
				if(pev->frags == 420){
					FireTargets( "exit_cardoor", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 450){
					FireTargets( "bluecar_toggle", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 490){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 3.0, 5.0, 255, FFADE_OUT );//��Ϲ���
				}
				if(pev->frags == 525){
							SET_VIEW( pPlayer->edict(), pPlayer->edict() );
							pPlayer->EnableControl(TRUE);
							pPlayer->m_trainning = 0;
							pPlayer->m_flVelocityModifier = 0;
							pPlayer->RemoveAllItems( TRUE );

							CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "change_map_teldes" );
							if ( pEntity3 ){
							pPlayer->pev->origin = pEntity3->pev->origin;
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							}

						UTIL_Remove( this );
						return;
				}
			}
			else{//�����
				if(pev->frags == 180){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pSpot->pev->velocity.x = 0;
							pSpot->pev->velocity.z = 0;
						}
				}

				if(pev->frags == 200){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
						if ( pSpot ){
							pSpot->pev->velocity.y = 0;
						}
				}
				if(pev->frags == 203){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(-120,-50,50);
						pSpot->pev->angles.y = 0;
						pSpot->pev->velocity.y = 10;
						pSpot->pev->velocity.x = 0;
						pSpot->pev->velocity.z = 0;
					}
					Create( "monster_cleaner", pev->origin - Vector(0,220,0), Vector(0,90,0), NULL );
				}
				if(pev->frags == 205){
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
					if ( pEntity ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "boss_kill1" );
					pEnemyMonster->ResetSequenceInfo( );
					pEnemyMonster->pev->frame = 0;
					pEnemyMonster->SetBodygroup( 2, 5 );
					//pEnemyMonster->SetBodygroup( 3, 2 );
					}
				}
				if(pev->frags == 233){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->SetBodygroup( 5, 3 );
						pSpot->pev->origin = pSpot->pev->origin + Vector(-40,60,0);
						}
				}
				if(pev->frags == 235){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(0,150,20);
						pSpot->pev->angles.y = 270;
						pSpot->pev->velocity.y = 0;
						pSpot->pev->velocity.x = 0;
						pSpot->pev->velocity.z = 0;
					}
				}
				if(pev->frags == 245){
						CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "monster_saintna");
						if ( pSpot ){
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pSpot->MyMonsterPointer();
						pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_VICTORY_DANCE );
						pEnemyMonster->ResetSequenceInfo( );
						pEnemyMonster->pev->frame = 0;
						}
				}

				if(pev->frags == 275){
					UTIL_ScreenFade( pPlayer, Vector(0,0,0), 2.0, 4.0, 255, FFADE_IN );
				}
				if(pev->frags == 276){
					pPlayer->m_fNextClearTextTime = gpGlobals->time + 3.5;		

					char text[256];
			
					sprintf( text, "Saintna: Thank you.....Kadoma\n");
					
					UTIL_SayTextAll( text,this );
				}
				if(pev->frags == 277){//����һ��һ��һ��һ��������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if (  (pEntity->pev->flags & FL_MONSTER) ){
						UTIL_Remove( pEntity );
						}
					}
				}
				if(pev->frags == 300){
					CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "bluecar_toggle" );
					if ( pEntity )
					{
						pEntity->pev->rendermode = 0;
					}
				}
				if(pev->frags == 310){
					CBaseEntity *pSpot = UTIL_FindEntityByClassname( NULL, "wrongdoor_camera2");
					if ( pSpot ){
						pSpot->pev->origin = pev->origin + Vector(-64,-320,48);
						pSpot->pev->angles.y = 270;
						pSpot->pev->velocity = g_vecZero;
						pSpot->pev->velocity.z = 8;
					}
				}
				if(pev->frags == 330){
					FireTargets( "happyend_white", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 350){
					FireTargets( "exit_cardoor", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 380){
					FireTargets( "bluecar_toggle", this, this, USE_TOGGLE, 0 );
				}
				if(pev->frags == 420){
					UTIL_ScreenFade( pPlayer, Vector(255,255,255), 3.0, 5.0, 255, FFADE_OUT );//��Ϲ���
				}
				if(pev->frags == 455){
							SET_VIEW( pPlayer->edict(), pPlayer->edict() );
							pPlayer->EnableControl(TRUE);
							pPlayer->m_trainning = 0;
							pPlayer->m_flVelocityModifier = 0;
							pPlayer->RemoveAllItems( TRUE );

							//�������
							pPlayer->TeamMate_Nagamatagi_Allclear(0);

							CBaseEntity *pEntity3 = UTIL_FindEntityByTargetname( NULL, "change_map_teldes" );
							if ( pEntity3 ){
							pPlayer->pev->origin = pEntity3->pev->origin;
							pPlayer->m_stuck_origin = pPlayer->pev->origin;
							}

						UTIL_Remove( this );
						return;
				}

			}

	}
tr_comp:
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
}

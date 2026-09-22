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


//=========================================================
// �ڶ�
//=========================================================
class CBlackHole : public CBaseEntity
{
public:
	void  Spawn( void );
	void explodeThink ( void );
};

LINK_ENTITY_TO_CLASS( black_hole, CBlackHole );

void CBlackHole::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->frags = 60;

	SetThink (&CBlackHole::explodeThink);
	pev->nextthink = gpGlobals->time + 0.01;
}

void CBlackHole::explodeThink ( void )
{
	if(pev->frags == 60){
	FX_Trail(pev->origin, entindex(), PROJ_BLACKHOLE);
	EMIT_SOUND(ENT(pev), CHAN_AUTO, "weapons/blackhole_exp.wav", 1, ATTN_LOW);
	}
	if(pev->frags <= 0){
	entvars_t *pevOwner = VARS( pev->owner );
	::RadiusDamage3( pev->origin, pev, pevOwner, 150, 600, CLASS_NONE, DMG_GENERIC);
	FX_Trail( pev->origin, entindex(), PROJ_BLACKHOLE_DETONATE );
	SetThink( NULL );
	UTIL_Remove( this );
	return;
	}

	float dmg = 40;
	int radius = 600;

	entvars_t *pevOwner = VARS( pev->owner );
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, radius, 0.1 );
	::RadiusDamage3( pev->origin, pev, pevOwner, dmg, radius, CLASS_NONE, DMG_DARK);

	pev->frags -= 1;
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// ������Ʒ
//=========================================================
class CProp_Items : public CBaseEntity
{
public:
	void  Spawn( void );

	void EXPORT TypeThink ( void );

	void SetObjectCollisionBox( void );

    int ObjectCaps(void) { return FCAP_CONTINUOUS_USE; }
    void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS( item_dropusekey, CProp_Items );

void CProp_Items::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16); 
}


void CProp_Items::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int pickup_item = 0;
	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pActivator->pev);
	if ( pPlayer->IsAlive() && pPlayer->m_rpg_menu_actor1 == 1 )
	{
		if(pPlayer->HasMenuItem_Full() && pev->frags != 8){
		UTIL_CenterPrintAll( "Items Full!" );
		return;
		}

		if(pev->frags == 1){//�����ҩˮ
			pPlayer->MenuItem_add(13);
			pickup_item = 1;
			pev->message = MAKE_STRING( "item_respawn" );
		}
		else if(pev->frags == 2){//�޵С�ҩˮ
			pPlayer->MenuItem_add(21);
			pickup_item = 1;
			pev->message = MAKE_STRING( "item_godwater" );
		}
		else if(pev->frags == 3){//DDF���ֹ�ս��
			pPlayer->MenuItem_add(12);
			pickup_item = 1;
			pev->message = MAKE_STRING( "item_armor3" );
		}
		else if(pev->frags == 4){//PCV������
			pPlayer->MenuItem_add(11);
			pickup_item = 1;
			pev->message = MAKE_STRING( "item_armor2" );
		}
		else if(pev->frags == 5){//CS��������
			pPlayer->MenuItem_add(9);
			pickup_item = 1;
			pev->message = MAKE_STRING( "item_armor1" );
		}
		else if(pev->frags == 6){//��ɽ���ط�����
			pPlayer->MenuItem_add(10);
			pickup_item = 1;
			pev->message = MAKE_STRING( "item_armor4" );
		}
		else if(pev->frags == 7){//���ƿ���
			pPlayer->MenuItem_add(2);
			pPlayer->m_fMask = TRUE;
			pickup_item = 1;
			pev->message = MAKE_STRING( "monster_eatkey" );
		}
		else if(pev->frags == 8){//��ʯ
			if(pPlayer->m_player_diamonds == 18){
			return;
			}

			pPlayer->m_player_diamonds += 1;

			UTIL_CenterPrintAll( "You get the Diamonds!" );

			char text[256];
			sprintf( text, "- Diamonds: %d/18\n", pPlayer->m_player_diamonds);
			UTIL_SayTextAll( text,this );

			if(pPlayer->m_player_diamonds == 1){
				
				sprintf( text, "- There is a diamond in each chapter. When you have collected all of them\n");
				UTIL_SayTextAll( text,this );

				sprintf( text, "- you can enter the bonus level after the end of the game.\n");
				UTIL_SayTextAll( text,this );
				pPlayer->m_fNextClearTextTime = gpGlobals->time + 12.0;
			}
			else if(pPlayer->m_player_diamonds == 18){
		
				sprintf( text, "- Congratulations on collecting all the diamonds.\n");
				UTIL_SayTextAll( text,this );

				
				sprintf( text, "- you can enter the bonus level after the end of the game.\n");
				UTIL_SayTextAll( text,this );

				pPlayer->m_fNextClearTextTime = gpGlobals->time + 12.0;
			}
			else{
				pPlayer->m_fNextClearTextTime = gpGlobals->time + 6.0;
			}

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			UTIL_Remove( this );
			return;
		}

		if(pickup_item == 1){

			if ( pev->message )//Կ������ʵ��
			{
				FireTargets( STRING(pev->message), this, this, USE_TOGGLE, 0 );
			}

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->message) );
			MESSAGE_END();
			UTIL_Remove( this );
		}
	}
}

void CProp_Items::TypeThink ( void )
{
	if(pev->frags == 1){//�����ҩˮ
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 2;
	}
	else if(pev->frags == 2){//�޵С�ҩˮ
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 16;
	}
	else if(pev->frags == 3){//DDF���ֹ�ս��
	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
	pev->body = 1;
	}
	else if(pev->frags == 4){//PCV������
	SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
	pev->body = 1;
	}
	else if(pev->frags == 5){//CS��������
	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
	pev->body = 0;
	}
	else if(pev->frags == 6){//��ɽ���ط�����
	SET_MODEL(ENT(pev), "models/w_all_items2.mdl");
	pev->body = 10;
	}
	else if(pev->frags == 7){//���ƿ���
	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
	pev->body = 11;
	}
	else if(pev->frags == 8){//��ʯ
	SET_MODEL(ENT(pev), "models/w_all_items3.mdl");
	pev->body = 9;
	pev->effects = EF_DIMLIGHT | EF_BRIGHTFIELD;//Bug Fix 3.0 �����ɫ��Ч������һЩ
	}

	UTIL_SetSize( pev, g_vecZero, g_vecZero );
}

void CProp_Items::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );

	SetThink (&CProp_Items::TypeThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


//=========================================================
// �Ԥλ�
//=========================================================
class CDraw_Event : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT DrawThink ( void );
	CBasePlayer *pPlayer;
};

LINK_ENTITY_TO_CLASS( draw_event, CDraw_Event );

void CDraw_Event::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NOCLIP;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	SetThink (&CDraw_Event::DrawThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDraw_Event::DrawThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

		if( pev->armortype != 9 && pev->weapons == 0 && FNullEnt(FIND_CLIENT_IN_PVS(edict())) ){
		return;
		}
		if(pev->health > 0){
					if(pev->weapons == 1){
						pev->health -= 1;
					}
					else if(pev->armortype == 9 && pPlayer->m_mode_int2 > 0){
						pev->weapons = 1;
						pPlayer->m_mode_int2 = 0;
					}
					else if(pPlayer->pev->movetype != MOVETYPE_NOCLIP 
						&& pPlayer->FVisible( this )
						&& pPlayer->FInViewCone( this )){
						pev->health -= 1;
					}
		}
		else{
					pev->frags += 1;
					if(pev->frags == 1){
					pev->origin.y -= 75;
						if(pev->armortype == 2){
						pPlayer->m_trainning = 1;
						SERVER_COMMAND( "autosave\n" );
						}
					pev->origin.z -= 15;
					pPlayer->pev->origin = pev->origin;
					pPlayer->m_flVelocityModifier = 0;
					DROP_TO_FLOOR ( ENT(pPlayer->pev) );
					}
					if(pev->frags == 2){
					pev->angles.y = 90;
					pPlayer->EnableControl(FALSE);
					pPlayer->pev->effects |= EF_NODRAW;
					SET_VIEW( pPlayer->edict(), edict() );
					}
					if(pev->frags == 5){
						if(pev->armortype == 1){//���������ؿ�
						CBaseEntity *pBarncle = Create( "monster_barnacle_holy", pev->origin, pev->angles, NULL );
						pBarncle->pev->effects |= EF_NODRAW;
						pBarncle->pev->solid = SOLID_NOT;
						UTIL_SetOrigin ( pBarncle->pev, pev->origin + Vector(0,0,100) );
						}
					}
					if(pev->frags == 33){
						SET_VIEW( pPlayer->edict(), pPlayer->edict() );
						pPlayer->EnableControl(TRUE);
						pPlayer->pev->effects &= ~EF_NODRAW;
						pPlayer->m_flVelocityModifier = 0;
						pPlayer->pev->v_angle = pev->angles;
						pPlayer->pev->fixangle = TRUE;
					}
					if(pev->frags == 35){
						if(pev->armortype == 1){//���������ؿ�
						pPlayer->m_mode_int1 += 1;
						}
						else if(pev->armortype == 2){//���������NM$L
						pPlayer->m_mode_int1 += 1;
						FireTargets( "nmsl_game_start", this, this, USE_TOGGLE, 0 );
							char text[256];
							sprintf( text, "Picture: Let's play hide and seek!\n");
							UTIL_SayTextAll( text,this );
							pPlayer->m_trainning = 0;
							pPlayer->m_fNextClearTextTime = gpGlobals->time + 5.0;
						}
						else if(pev->armortype == 3){//��
						pPlayer->m_mode_int1 += 1;
						pPlayer->pev->punchangle.y += -30.0;
						pPlayer->pev->punchangle.z += -30.0;
						UTIL_ScreenFade( pPlayer, Vector(192,0,0), 0.5, 0.2, 255, FFADE_IN );//��Ϲ���
						EMIT_SOUND(ENT(pev), CHAN_BODY, "common/bodysplat.wav", 1, ATTN_NORM);	
						pPlayer->TakeDamage ( pev, pev, 75, DMG_SLASH);
						FX_Explosion(pPlayer->pev->origin, 236 );
						pPlayer->m_flVelocityModifier = 1;
						pPlayer->pev->velocity.y -= 300;
						}
						else if(pev->armortype == 4){//ʮ�ּ�
						pPlayer->m_mode_int1 += 1;
						UTIL_ScreenFade( pPlayer, Vector(255,255,128), 0.5, 0.2, 255, FFADE_IN );//��Ϲ���
						pPlayer->m_needleheal2 += 100;
						}
						else if(pev->armortype == 5){//ҹ��ģʽ
						pPlayer->m_mode_int1 += 1;
						FireTargets( "break_light", this, this, USE_TOGGLE, 0 );
						}
						else if(pev->armortype == 6){//��ȭ����
						pPlayer->m_mode_int1 += 1;
						EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/fist_hitbod1.wav", 1, ATTN_NORM);	
						pPlayer->TakeDamage ( pev, pev, 20, DMG_GENERIC);
						pPlayer->m_flVelocityModifier = 1;
						pPlayer->pev->velocity.y -= 800;
						pPlayer->pev->velocity.z += 200;
						pPlayer->pev->punchangle.x += 20.0;
						}
						else if(pev->armortype == 7){//�ӽ�Ť��
						pPlayer->m_mode_int1 += 1;
						pPlayer->pev->fov = pPlayer->m_iFOV = -60;
						}
						else if(pev->armortype == 8){//��������ķ����£�
							pPlayer->m_mode_int1 += 1;
						int rd = RANDOM_LONG(1,7);
							CBaseEntity *pEntity = NULL;
							while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
							{
								if ( FClassnameIs( pEntity->pev, "draw_event" ) ){
									if(rd == 1 && pEntity->pev->armortype == 1){
										FireTargets( "draw_button1", this, this, USE_TOGGLE, 0 );
										pev->weapons = 2;
										break;
									}
									else if(rd == 3 && pEntity->pev->armortype == 3){
										FireTargets( "draw_button3", this, this, USE_TOGGLE, 0 );
										pev->weapons = 2;
										break;
									}
									else if(rd == 4 && pEntity->pev->armortype == 4){
										FireTargets( "draw_button4", this, this, USE_TOGGLE, 0 );
										pev->weapons = 2;
										break;
									}
									else if(rd == 5 && pEntity->pev->armortype == 5){
										FireTargets( "draw_button5", this, this, USE_TOGGLE, 0 );
										pev->weapons = 2;
										break;
									}
									else if(rd == 6 && pEntity->pev->armortype == 6){
										FireTargets( "draw_button6", this, this, USE_TOGGLE, 0 );
										pev->weapons = 2;
										break;
									}
									else if(rd == 7 && pEntity->pev->armortype == 7){
										FireTargets( "draw_button7", this, this, USE_TOGGLE, 0 );
										pev->weapons = 2;
										break;
									}
								}
							}
							if(pev->weapons != 2){
							pPlayer->GiveNamedItem( "item_armor1" );
							}
						}
						else if(pev->armortype == 9){//��Ϸʤ��
						FireTargets( "nmsl_game_win", this, this, USE_TOGGLE, 0 );
						UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
						pPlayer->pev->fov = pPlayer->m_iFOV = 0;
							char text[256];
							sprintf( text, "Picture: You found me! The door is unlocked. Go on!\n");
							UTIL_SayTextAll( text,this );

						
							sprintf( text, "Attempts: %d\n",pPlayer->m_mode_int1);
							UTIL_SayTextAll( text,this );

							if(pPlayer->m_mode_int1 == 1){//Bug Fix 3.0 ������ظ��Ի���ʾ
							pPlayer->GiveNamedItem( "item_leveluper" );
							}

							pPlayer->m_fNextClearTextTime = gpGlobals->time + 6.0;
							pPlayer->m_mode_int1 = 0;
						}

						UTIL_Remove( this );
						return;
					}
					
		}
}


//=========================================================
//
// Ͱ����
//
//=========================================================
class CBarrel_point : public CBaseEntity
{
public:
	void	Spawn( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
};
LINK_ENTITY_TO_CLASS( func_vent_point, CBarrel_point );
LINK_ENTITY_TO_CLASS( func_barrel_point, CBarrel_point );
LINK_ENTITY_TO_CLASS( func_chair_point, CBarrel_point );
void CBarrel_point::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	if ( FClassnameIs( pev, "func_barrel_point" ) ){
	SET_MODEL( ENT(pev), "models/props_all.mdl" );
	UTIL_SetSize(pev, Vector(-18, -18, 0), Vector(18, 18, 56));
	UTIL_SetOrigin( pev, pev->origin );
	}
	else if ( FClassnameIs( pev, "func_vent_point" ) ){
	SET_MODEL( ENT(pev), "models/props_all.mdl" );
	UTIL_SetSize(pev, Vector(-24, -24, 0), Vector(24, 24, 64));
	pev->body = 2;
	UTIL_SetOrigin( pev, pev->origin );
	}
	else{
	SET_MODEL( ENT(pev), "models/props_all.mdl" );
	pev->body = 1;
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );
	pev->movetype = MOVETYPE_STEP;
	}
}

void CBarrel_point::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	UTIL_Sparks( ptr->vecEndPos );
}
//=========================================================
//
// �㶫�����
//
//=========================================================
class CWrongDoor_camera : public CBaseMonster
{
public:
	void	Spawn( void );
	void	EXPORT killThink ( void );
	void	EXPORT Materialize ( void );
	CBasePlayer *pPlayer;
};

LINK_ENTITY_TO_CLASS( env_light_glow, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( env_smoker_fx, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera1, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera2, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera3, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera4, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera5, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera6, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera7, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera8, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_camera9, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_kadoma_moon, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_kadoma_camera, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_airwall, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( wrongdoor_blockfloor, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( npc_aim_flag, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( npc_attack_flag, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( player_aim_flag, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( player_attack_flag, CWrongDoor_camera );
LINK_ENTITY_TO_CLASS( xen_unknow_light, CWrongDoor_camera );

void CWrongDoor_camera::Spawn( void )
{
	if ( FClassnameIs( pev, "wrongdoor_kadoma_moon" )){
	SET_MODEL(ENT(pev), "sprites/moon.spr");
	pev->movetype = MOVETYPE_NONE;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 0.1;
	}
	else if ( FClassnameIs( pev, "player_aim_flag" )){
	pev->solid			= SOLID_NOT;
	pev->health = 1;
	pev->movetype		= MOVETYPE_NONE;
	pev->flags |= FL_MONSTER;
	pev->effects		= 0;

	SET_MODEL(ENT(pev), "models/flag.mdl");

	DROP_TO_FLOOR(ENT(pev));
	pev->scale = 0;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	ResetSequenceInfo();

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink (&CWrongDoor_camera::killThink);

	pev->nextthink = gpGlobals->time + 0.1;
	}
	else if ( FClassnameIs( pev, "player_attack_flag" ) ){
	pev->solid			= SOLID_NOT;
	pev->health = 1;
	pev->movetype		= MOVETYPE_NONE;
	pev->flags		   |= FL_MONSTER;
	pev->effects		= 0;

	SET_MODEL(ENT(pev), "models/deep_target.mdl");

	pev->skin = 1;
	pev->scale = 0;
	pev->renderfx = kRenderFxGlowShell;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	ResetSequenceInfo();

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink (&CWrongDoor_camera::killThink);

	pev->nextthink = gpGlobals->time + 0.1;
	}
	else if ( FClassnameIs( pev, "npc_aim_flag" ) || FClassnameIs( pev, "npc_attack_flag" ) ){
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= 0;
	pev->flags		   |= FL_MONSTER;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	SetThink (&CWrongDoor_camera::killThink);
	pev->nextthink = gpGlobals->time + 0.1;
	}
	else if ( FClassnameIs( pev, "xen_unknow_light" ) ){
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= 0;
	pev->framerate = 1.0;
	SET_MODEL(ENT(pev), "models/xen_mflight.mdl");
	SetThink (&CWrongDoor_camera::killThink);
	pev->avelocity.y = 30;
	pev->nextthink = gpGlobals->time + 0.1;
	}
	else if ( FClassnameIs( pev, "wrongdoor_airwall" ) ){
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	}
	else if ( FClassnameIs( pev, "wrongdoor_blockfloor" ) ){
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	SET_MODEL(ENT(pev), "models/props_all.mdl");
	pev->body = 6;
	UTIL_SetSize(pev, Vector(-64, -64, -8), Vector(64, 64, 8));
	pev->effects = EF_DIMLIGHT;
	}
	else{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_FLY;
	pev->effects		= 0;
	pev->health			= 623;
	pev->framerate = 1.0;
	pev->flags		   |= FL_NOTARGET;
	SET_MODEL(ENT(pev), "models/camera_rocket.mdl");
	SetThink (&CWrongDoor_camera::killThink);
	pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CWrongDoor_camera::Materialize(void)
{
	pev->scale += 0.01;
	pev->renderamt += 5;

	if (pev->renderamt >= 255)
	{
		pev->renderamt = 255;
		SetThink (&CWrongDoor_camera::killThink);
	}

	pev->nextthink = gpGlobals->time + 0.01;
}
void CWrongDoor_camera::killThink ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	//if ( FClassnameIs( pev, "player_aim_flag" ) ){
	//	pev->renderamt = 255;
	//		float flDist = ( pPlayer->pev->origin - pev->origin).Length();
	//		if(flDist < 80){
	//		UTIL_Remove( this );
	//		return;
	//		}
	//	pev->nextthink = gpGlobals->time + 0.1;
	//}
	if ( FClassnameIs( pev, "npc_aim_flag" ) || FClassnameIs( pev, "npc_attack_flag" ) ){
		if(pev->health < 1000){
		pev->health -= 1;
		}
		if(pev->health <= 0){
		UTIL_Remove( this );
		return;
		}

		if(pev->team == 1 && pev->owner == NULL){
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 192 )) != NULL)
			{
				if ( FClassnameIs( pEntity->pev, "func_breakable" ) || FClassnameIs( pEntity->pev, "func_door_breaker" )
				|| FClassnameIs( pEntity->pev, "func_tankmortar" )){
					pev->owner = ENT(pEntity->pev);
					pev->health = 2000;
				}
			}
		}
		else if(pev->team == 1){
			entvars_t *pevOwner = VARS( pev->owner );
			if ( pevOwner->health <= 0 ){
			UTIL_Remove( this );
			return;
			}
		}
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else if ( FClassnameIs( pev, "xen_unknow_light" ) ){
		if( pev->frags == 1 ){

			if (!FStringNull(pev->target)){
			FireTargets( STRING(pev->target), this, this, USE_TOGGLE, 0 );
			pev->target = 0;
			FX_Trail( pev->origin, entindex(), 135 );
			//pev->effects |= EF_LIGHT;
			}
			
			//if(!FNullEnt(FIND_CLIENT_IN_PVS(edict()))){
			
			//}
			//else{
		//	pev->effects = 0;
			//}
		}
		pev->nextthink = gpGlobals->time + 0.3;
	}
	else if ( FClassnameIs( pev, "env_smoker_fx" ) ){
		if( !FNullEnt(FIND_CLIENT_IN_PVS(edict())) ){
		FX_Trail( pev->origin, entindex(), PROJ_NERVEGREN );
		}
		pev->nextthink = gpGlobals->time + 0.2;
	}
	else if ( FClassnameIs( pev, "env_light_glow" ) ){
		if( !FNullEnt(FIND_CLIENT_IN_PVS(edict())) ){
		pev->effects |= EF_LIGHT;
		}
		else{
		pev->effects = 0;
		}
		pev->nextthink = gpGlobals->time + 0.5;
	}
	else{
		if(pev->frags >= 1){
		pev->frags += 1;
		}

		pev->nextthink = gpGlobals->time + 0.1;

		if(pev->armortype == 1){//�����㡤1
			if(pev->frags >= 140){
				if(pev->velocity.Length() >= 1.0){
				pev->velocity = pev->velocity * 0.9;
				}
				else{
				pev->velocity = g_vecZero;
				}
			}
			else if(pev->velocity.y > -20){
			pev->velocity.y -= 1;
			}
		}
		else if(pev->armortype == 2){//�����㡤2
			if(pev->frags >= 30){
				if(pev->velocity.Length() >= 1.0){
				pev->velocity = pev->velocity * 0.9;
				}
				else{
				pev->velocity = g_vecZero;
				}
				if(	pev->avelocity.y > 0){
				pev->avelocity.y -= 1;
				}
			}
			else if(pev->velocity.y > -250){
			pev->velocity.x -= 1;
			pev->velocity.z += 1;
			pev->velocity.y -= 5;
			pev->avelocity.y += 1;
			}
		}
		else if(pev->armortype == 3){//�����㡤3
			if(pev->frags >= 7){
				if(pev->velocity.Length() >= 1.0){
				pev->velocity = pev->velocity * 0.75;
				}
				else{
				pev->velocity = g_vecZero;
				}
			}
			else if(pev->frags >= 1){
				pev->velocity.y = 450;
			}
		}
		else if(pev->armortype == 4){//�����㡤4
				if(pev->velocity.Length() <= 90.0){
				pev->velocity = pev->velocity * 1.05;
				}
		}
		else if(pev->armortype == 5){//�����㡤5
				if(pev->velocity.Length() > 0){
				pev->velocity = pev->velocity * 0.95;
				}
		}
		else if(pev->armortype == 6){//�����㡤6
				if(pev->avelocity.x > 0){
				pev->avelocity.x = pev->avelocity.x * 0.95;
				}
				if(pev->velocity.Length() > 0){
				pev->velocity = pev->velocity * 0.95;
				}
		}
		else if(pev->armortype == 7){//�����㡤7
				if(pev->avelocity.Length() > 0){
				pev->avelocity = pev->avelocity * 0.95;
				}
		}
		else if(pev->armortype == 8){//�����㡤8
				if(pev->velocity.Length() > 0){
				pev->velocity = pev->velocity * 0.95;
				}
				if(pev->avelocity.Length() > 0){
				pev->avelocity = pev->avelocity * 0.95;
				}
		}
		else if(pev->armortype == 9){//�����㡤9
				if(pev->velocity.Length() > 0){
				pev->velocity = pev->velocity * 1.05;
				}
		}
		else if(pev->armortype == 10){//�����㡤10
				if(pev->velocity.Length() > 0){
				pev->velocity = pev->velocity * 0.98;
				}
				if(pev->avelocity.Length() > 0){
				pev->avelocity = pev->avelocity * 0.99;
				}
				pev->nextthink = gpGlobals->time + 0.05;
		}
		else if(pev->armortype == 11){//�����㡤11
				if(pev->avelocity.Length() > 0){
				pev->avelocity = pev->avelocity * 1.05;
				}
				pev->nextthink = gpGlobals->time + 0.05;
		}
		else if(pev->armortype == 12){//�����㡤12
				if(pev->velocity.Length() > 0){
				pev->velocity = pev->velocity * 0.95;
				}
				if(pev->avelocity.Length() > 0){
				pev->avelocity = pev->avelocity * 0.95;
				}
				pev->nextthink = gpGlobals->time + 0.05;
		}
		else if(pev->armortype == 13){//�����㡤13
				pev->velocity = pev->velocity * 1.2;
		}
		else if(pev->armortype == 14){//�����㡤14
				pev->velocity = pev->velocity * 0.8;
		}
	}
	
}

//=========================================================
//
// ���׼PreThink
//
//=========================================================

class Chl623_prethink : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT Player_Think ( void );
};

LINK_ENTITY_TO_CLASS( chl623_prethink, Chl623_prethink );

void Chl623_prethink::Spawn( void )
{
	Precache();

	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= 0;
	pev->frame			= 0;
	pev->framerate		= 1.0;

	SetThink (&Chl623_prethink::Player_Think);
	pev->nextthink = gpGlobals->time + 0.01;

}

void Chl623_prethink::Player_Think ( void )
{
	pev->nextthink = gpGlobals->time + 0.01;
}
//=========================================================
//
// ��֭�����
//
//=========================================================

class Cnode_pointer : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
};

void Cnode_pointer :: Precache ( void )
{
	PRECACHE_MODEL("models/testsphere.mdl");
}

LINK_ENTITY_TO_CLASS( node_pointer, Cnode_pointer );

void Cnode_pointer::Spawn( void )
{
	Precache();

	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= 0;
	pev->frame			= 0;
	pev->framerate		= 1.0;

	SET_MODEL(ENT(pev), "models/testsphere.mdl"); 
}


//=========================================================
//
// ʥ������������
//
//=========================================================

class CHoly_valvesword : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT Sword_Think ( void );
	void	EXPORT Sword_Touch ( CBaseEntity *pOther );
	int		m_iTrail;
	CBasePlayer *pPlayer;
};

void CHoly_valvesword :: Precache ( void )
{
	PRECACHE_MODEL("models/holysword_skill.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
	PRECACHE_MODEL("sprites/sword_target.spr");
	PRECACHE_SOUND ("weapons/mortarhit.wav");
}

LINK_ENTITY_TO_CLASS( holy_valve_sword, CHoly_valvesword );
LINK_ENTITY_TO_CLASS( sword_aim_target, CHoly_valvesword );

void CHoly_valvesword::Spawn( void )
{
	Precache();
	if ( FClassnameIs(pev,"holy_valve_sword") ){
			pev->solid			= SOLID_BBOX;
			pev->movetype		= MOVETYPE_FLY;
			pev->effects		= 0;
			
			pev->frags			= 0;
			pev->armorvalue		= 1;

			SET_MODEL(ENT(pev), "models/holysword_skill.mdl"); 

			UTIL_SetSize ( pev, Vector(-4,-4,0), Vector(4,4,0));
			SetThink (&CHoly_valvesword::Sword_Think);
			SetTouch (&CHoly_valvesword::Sword_Touch );
			pev->nextthink = gpGlobals->time + 0.05;
			
			pev->effects		= EF_NODRAW;

			pev->sequence		= 1;
			pev->frame			= 0;
			pev->framerate		= 1.0;
	}
	else{
		SET_MODEL(ENT(pev), "sprites/sword_target.spr");

		pev->solid			= SOLID_NOT;
		pev->movetype		= MOVETYPE_NONE;
		UTIL_SetSize ( pev, g_vecZero, g_vecZero);
		pev->frame			= 0;
		pev->framerate		= 1.0;
		pev->effects		= 0;

		pev->rendermode = kRenderTransAdd;
		//pev->renderfx = kRenderFxNoDissipation;
		pev->renderamt = 255;

		SetThink (&CHoly_valvesword::Sword_Think);
		pev->nextthink = gpGlobals->time + 0.05;
	}
}

void CHoly_valvesword::Sword_Touch ( CBaseEntity *pOther )
{
	if(pev->armorvalue == 0){
		if ( pOther->pev->flags & FL_MONSTER ){
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pOther->MyMonsterPointer();
			if(pEnemyMonster){
			pEnemyMonster->m_trouch_full_radiusdmg += 1;
			}
		}

		EMIT_SOUND(ENT(pev), CHAN_AUTO, "weapons/mortarhit.wav", 1, ATTN_LOW);
		pev->framerate = 0;
		TraceResult tr;
		Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
		UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );
		entvars_t *pevOwner = VARS( pev->owner );
		::RadiusDamage_limit( pev->origin + (tr.vecPlaneNormal * 30), pev, pevOwner, 1000, 480, 623, DMG_VALVE_SWORD);

		FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 12), 100 );
		pev->armorvalue = -1;
		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_NONE;
		pev->solid	  = SOLID_NOT;
		pev->effects  = 0;
	}
}	


void CHoly_valvesword::Sword_Think ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

			pev->frags += 1;

			if(pev->frags <= 32 && FClassnameIs(pev,"holy_valve_sword")){
					if(pPlayer->m_fPlayerUseHolySword == TRUE){
						if(pev->frags == 32){
								pPlayer->m_fPlayerUseHolySword = FALSE;

								if(pPlayer->pev->viewmodel == 0 && pPlayer->m_barnacle_RTP == 0 && pPlayer->m_fMoveItem == NULL){
									if(pPlayer->m_pActiveItem != NULL){
									pPlayer->m_pActiveItem->Deploy();
									}
								}
						}
					}
			}
			else if(pev->frags <= 180 && FClassnameIs(pev,"sword_aim_target")){
					if(pPlayer->m_fPlayerUseHolySword == TRUE){
					UTIL_MakeVectors( pPlayer->pev->v_angle );
					Vector vecSrc = pPlayer->GetGunPosition( );
					Vector vecAiming = gpGlobals->v_forward;

					TraceResult tr;
					UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, ignore_monsters, ENT(pPlayer->pev), &tr );
						if ( tr.flFraction < 1.0 ){
						tr.vecEndPos = tr.vecEndPos + tr.vecPlaneNormal * 32;
						}
					UTIL_TraceLine ( tr.vecEndPos, tr.vecEndPos + Vector(0,0,-8192), ignore_monsters, ENT(pPlayer->pev), &tr );
					UTIL_SetOrigin( pev, tr.vecEndPos );
					UTIL_TraceLine ( tr.vecEndPos, tr.vecEndPos + Vector(0,0,8192), ignore_monsters, ENT(pPlayer->pev), &tr );
						if ( tr.flFraction < 1.0 ){
						pPlayer->m_sword_aim_origin = tr.vecEndPos + tr.vecPlaneNormal * 16;
						}
						else{
						pPlayer->m_sword_aim_origin = tr.vecEndPos;
						}
					}
			}

			if(FClassnameIs(pev,"holy_valve_sword")){
					if(pev->frags >= 32){
						//pPlayer->pev->origin = pev->origin;
						if(pev->armorvalue > 0){
						pev->effects	= EF_LIGHT;
						pev->armorvalue = 0;
						}
						if(pev->frags >= 32 && pev->frags <= 33){
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
						WRITE_SHORT(m_iTrail );	// model
						WRITE_BYTE( 10 ); // life
						WRITE_BYTE( 2 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 128 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
						WRITE_SHORT(m_iTrail );	// model
						WRITE_BYTE( 10 ); // life
						WRITE_BYTE( 2 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 128 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
						WRITE_SHORT(m_iTrail );	// model
						WRITE_BYTE( 10 ); // life
						WRITE_BYTE( 2 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 128 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMFOLLOW );
						WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
						WRITE_SHORT(m_iTrail );	// model
						WRITE_BYTE( 10 ); // life
						WRITE_BYTE( 2 );  // width
						WRITE_BYTE( 255 );	// R
						WRITE_BYTE( 128 );	// G
						WRITE_BYTE( 128 );	// B
						WRITE_BYTE( 192 );	// brightness
						MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
						}
						pev->velocity.z = -2000;
					}
					else{
					pev->origin = pPlayer->m_sword_aim_origin;
				//	char text[256];
				//	sprintf( text, "x:%1.0f y:%1.0f z:%1.0f\n",pev->origin.x,pev->origin.y,pev->origin.z);
				//	UTIL_SayTextAll( text,this );
					}
			}

			if ( FClassnameIs(pev,"sword_aim_target") ){
				if(pev->frags > 180){
					SetThink (NULL);
					UTIL_Remove( this );
					return;
				}
				pev->nextthink = gpGlobals->time + 0.01;
			}
			else{	
				if(pev->frags > 250){
					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_KILLBEAM );
					WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
					MESSAGE_END();

					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_KILLBEAM );
					WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
					MESSAGE_END();
		
					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_KILLBEAM );
					WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
					MESSAGE_END();

					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_KILLBEAM );
					WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
					MESSAGE_END();

					SetThink (NULL);
					UTIL_Remove( this );
					return;
				}
				pev->nextthink = gpGlobals->time + 0.05;
			}

		if(pev->frags > 30 && pev->armorvalue == 0){
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, 400, 0.2 );
		}
}


//=========================================================
//
// ���ͺ�����
//
//=========================================================

class Cdisplacer_ball : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT Displacer_Think ( void );
	void	EXPORT Displacer_Touch ( CBaseEntity *pOther );
	void	Killed(entvars_t *pevAttacker, int iGib);
};

void Cdisplacer_ball :: Precache ( void )
{
	PRECACHE_MODEL("sprites/displacer_ring.spr");
}

LINK_ENTITY_TO_CLASS( displacer_ball, Cdisplacer_ball );

void Cdisplacer_ball::Spawn( void )
{
	Precache();

	pev->classname = MAKE_STRING( "displacer_ball" );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->effects		= 0;
	pev->frame			= 0;
	pev->framerate		= 1.0;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 128;
	pev->rendercolor.x = 128;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 128;
	pev->scale = 1.2;

	pev->frags = 0;

	SET_MODEL(ENT(pev), "sprites/anim_spr12.spr"); 

	UTIL_SetSize ( pev, Vector(-4,-4,-4), Vector(4,4,4));
	SetThink (&Cdisplacer_ball::Displacer_Think);
	SetTouch ( &Cdisplacer_ball::Displacer_Touch );
	pev->nextthink = gpGlobals->time + 0.1;

	FX_Trail(pev->origin, entindex(), PROJ_DISPLACER);
}

void Cdisplacer_ball::Killed (entvars_t *pevAttacker, int iGib)
{
	FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
	UTIL_Remove( this );
}

void Cdisplacer_ball::Displacer_Touch ( CBaseEntity *pOther )
{
		if ( pOther->pev->takedamage && pOther->pev->deadflag == DEAD_NO ){
			entvars_t *pevOwner = VARS( pev->owner );
			pOther->TakeDamage ( pev, pevOwner, 80, DMG_ENERGYBLAST | DMG_CONCUSSION);//ѣ�γ��!
		}

		FX_Trail( Center(), entindex(), PROJ_DISPLACER_DETONATE_WATER);

		pev->health = 55;
		pev->movetype = MOVETYPE_NONE;
		SetTouch ( NULL );
}	


void Cdisplacer_ball::Displacer_Think ( void )
{
		pev->health += 1;
		if(pev->health >= 60){
			TraceResult tr;
			Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
			UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );
			entvars_t *pevOwner = VARS( pev->owner );
			::RadiusDamage_limit( pev->origin + (tr.vecPlaneNormal * 30), pev, pevOwner, 320, 400, CLASS_NONE, DMG_ENERGYBLAST);
			FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
			FX_Trail( Center(), entindex(), PROJ_DISPLACER_DETONATE );
			SetThink (NULL);
			UTIL_Remove( this );
		}

		pev->frame = ((int)pev->frame + 1) % 11;
		entvars_t *pevOwner = VARS( pev->owner );
		pev->nextthink = gpGlobals->time + 0.1;
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, 400, 0.2 );
		::RadiusDamage2( pev->origin, pev, pevOwner, 16, 128, CLASS_PLAYER, DMG_ENERGYBLAST);
}

//=========================================================
// ��ͼ���ã����ĵ�ͼ�趨��
//=========================================================
class CCSHL623_MAP_SET : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT set_map ( void );
	CBasePlayer *pPlayer;
	char text[256];
};

LINK_ENTITY_TO_CLASS( cshl623_map_set, CCSHL623_MAP_SET );

void CCSHL623_MAP_SET::Precache()
{
	//Bug Fix 3.0 ��ͼ������������
	if ( !strcmp( STRING( gpGlobals->mapname ), "c1a3_wdoor") ){
		
		UTIL_PrecacheOther( "monster_nirvana_death" );
		UTIL_PrecacheOther( "monster_sewblade" );
	}
	else if ( !strcmp( STRING( gpGlobals->mapname ), "c4a2_wdoor") ){
		UTIL_PrecacheOther( "monster_doma" );
	}
}

void CCSHL623_MAP_SET::Spawn( void )
{
	Precache( );
	SetThink (&CCSHL623_MAP_SET::set_map);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CCSHL623_MAP_SET::set_map ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	if ( strcmp( STRING( gpGlobals->mapname ), STRING( pev->targetname )) ){

		if ( !strcmp( STRING( gpGlobals->mapname ), "t0a0_wdoor") ){
			//���������С��
			CBaseEntity *pEntity = Create( "monster_generic_item2", Vector(550,-875,-34), Vector(0,0,0), NULL );
			SET_MODEL(ENT(pEntity->pev), "models/props_all.mdl");
			pEntity->pev->body = 8;
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c1a0_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-2010,35,900), g_vecZero );
			pDropItem->pev->frags = 8;

			if(pPlayer->m_fSecondWorld == TRUE){//����Ŀ? ����20�������!
				CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "second_world" );
				if ( pEntity ){
				pEntity->pev->spawnflags = 32;
				FireTargets( "second_world", this, this, USE_TOGGLE, 0 );
				}
				CBaseEntity *pDropItem = Create("item_leveluper_s", Vector(-1268,829,872), g_vecZero );
			}
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c1a1_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(139,-276,592), g_vecZero );
			pDropItem->pev->frags = 8;

			//��ǹ�ֲ��ٲ���
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, Vector(-3004,-3054,575), 256 )) != NULL)
			{
				if ( FClassnameIs( pEntity->pev, "monstermaker" ) ){
				UTIL_Remove( pEntity );
				}
			}
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c1a2_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(1067,-756,1070), g_vecZero );
			pDropItem->pev->frags = 8;

			//Bug Fix 3.0 ���������ķ��ξ���
			CBaseEntity *pEntity = Create("monster_police", Vector(808,-60,622), Vector(0,270,0) );
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->SetBodygroup( 0, 1);
			pEnemyMonster->SetBodygroup( 1, 3);
			pEntity->pev->netname = MAKE_STRING( "Crazy.Police" );
			pEntity->pev->health  = 120;
			pEntity->pev->impulse = 1;
			pEntity->pev->spawnflags |= SF_MONSTER_GAG;
			pEntity->pev->targetname = MAKE_STRING("crazy_police");
			pEnemyMonster->m_killed_exp = 150;
			pEnemyMonster->m_rpgms_level = 20;
			pEnemyMonster->m_lovehate = 10;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c1a3_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(300,1207,250), g_vecZero );
			pDropItem->pev->frags = 8;

			//���������С��
			CBaseEntity *pEntity = Create( "monster_generic_item2", Vector(1512,-1992,-15), Vector(0,0,0), NULL );
			SET_MODEL(ENT(pEntity->pev), "models/props_all.mdl");
			pEntity->pev->body = 8;
			SetBits( pEntity->pev->effects, EF_DIMLIGHT);
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c1a4_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(2682,-2255,-500), g_vecZero );
			pDropItem->pev->frags = 8;

			Create("weapon_9mmhandgun", Vector(2676,-2245,-500), g_vecZero );
			//�ѶȽ��Ͷ������ǹ����

			//���������С��
			CBaseEntity *pEntity2 = Create( "monster_generic_item2", Vector(2700,-2099,-490), Vector(0,0,0), NULL );
			SET_MODEL(ENT(pEntity2->pev), "models/props_all.mdl");
			pEntity2->pev->body = 8;
			SetBits( pEntity2->pev->effects, EF_DIMLIGHT);
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c2a1_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-1267,666,1262), g_vecZero );
			pDropItem->pev->frags = 8;

			Create("weapon_9mmhandgun", Vector(-558,2909,-125), g_vecZero );
			//�ѶȽ��Ͷ������ǹ����
			
			//û�Ű��᲻������ҪNPC���ұ�����
			CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, "no_way_barney" );
			if ( pEntity ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				pEnemyMonster->m_iTriggerCondition = 0;
				pEntity->pev->solid	= SOLID_NOT;
			}
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c2a2_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(2517,-1608,350), g_vecZero );
			pDropItem->pev->frags = 8;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c2a3_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-713,1218,-94), g_vecZero );
			pDropItem->pev->frags = 8;

			Create("item_leveluper", Vector(-3754,-2976,-950), g_vecZero );
			//���⽱������
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c2a4_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-902,-796,-2098), g_vecZero );
			pDropItem->pev->frags = 8;

			if(pPlayer->m_fSecondWorld == TRUE){
			//Create("wrongdoor_blockfloor", Vector(-974,100,-2520), g_vecZero );
			//Create("wrongdoor_blockfloor", Vector(-974,-150,-2600), g_vecZero );
			Create("weapon_redeemer", Vector(-1519,-70,-1631), g_vecZero );
			}//����Ŀ�ö��⽱������

			Create("weapon_357", Vector(-2108,-477,-2088), g_vecZero );
			//�ѶȽ��Ͷ������ǹ����

			Create("monster_alien_slave", Vector(339,-2168,-2024), Vector(0,90,0) );
			//����ĸ��ٸ��˰���!
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c2a4d_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(470,-2264,-298), g_vecZero );
			pDropItem->pev->frags = 8;

			Create("weapon_357", Vector(2776,-959,-760), g_vecZero );
			//���ص���ǹ����
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c2a5_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(100,-2035,-60), g_vecZero );
			pDropItem->pev->frags = 8;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c3a1_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-2569,-3318,1030), g_vecZero );
			pDropItem->pev->frags = 8;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c3a2_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-1955,-959,784), g_vecZero );
			pDropItem->pev->frags = 8;

			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "item_flashlight" );
			if ( pEntity ){
				//�����ֵ�Ͳ�滻�ɶ��������С��
				CBaseEntity *pEntity2 = Create( "monster_generic_item2", pEntity->pev->origin, Vector(0,0,0), NULL );
				SET_MODEL(ENT(pEntity2->pev), "models/props_all.mdl");
				pEntity2->pev->body = 8;
				SetBits( pEntity2->pev->effects, EF_DIMLIGHT);
				UTIL_Remove( pEntity );
			}
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c4a1_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-2753,-2251,2430), g_vecZero );
			pDropItem->pev->frags = 8;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c4a1a_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(1908,-1921,-300), g_vecZero );
			pDropItem->pev->frags = 8;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c4a2_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(1600,159,-3500), g_vecZero );
			pDropItem->pev->frags = 8;

			//Bug Fix 3.0 �������������ؤ�Doma
			CBaseEntity *pEntity = Create("monster_doma", Vector(2074,485,-3788), Vector(0,270,0) );
			CBaseMonster *pEnemyMonster;
			pEnemyMonster = pEntity->MyMonsterPointer();
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->SetActivity( ACT_IDLE );
			pEnemyMonster->m_singdelay_max = 4;
			pEnemyMonster->m_singdelay_use = pEnemyMonster->m_singdelay_max;
			pEntity->pev->takedamage  = DAMAGE_NO;
			pEntity->pev->impulse = 13;
			pEntity->pev->solid = SOLID_NOT;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c4a3_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(-3446,1351,3128), g_vecZero );
			pDropItem->pev->frags = 8;
		}
		else if ( !strcmp( STRING( gpGlobals->mapname ), "c5a1_wdoor") ){
			//��ʯ����
			CBaseEntity *pDropItem = Create("item_dropusekey", Vector(3157,2698,380), g_vecZero );
			pDropItem->pev->frags = 8;
		}

		pev->targetname = gpGlobals->mapname;
		//UTIL_CenterPrintAll( STRING( gpGlobals->mapname ) );
	}

	pev->nextthink = gpGlobals->time + 1.0;
}

//=========================================================
// ��������
//=========================================================
class CCSHL623_BOUNCE_BOX : public CBaseEntity
{
	public:
	void	Spawn( void );
	void	EXPORT combat_touch ( CBaseEntity *pOther );
	void	SetObjectCollisionBox( void );
};

LINK_ENTITY_TO_CLASS( cshl623_bounce_box, CCSHL623_BOUNCE_BOX );

void CCSHL623_BOUNCE_BOX :: SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-32, -32, 0);
	pev->absmax = pev->origin + Vector(32, 32, 32); 
}

void CCSHL623_BOUNCE_BOX::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( pev, pev->origin );

	if(pev->armortype == 8){
	SET_MODEL(ENT(pev), "models/w_all_items3.mdl");
	pev->body = 3;
	}
	else{
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 18;
	}

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.

	SetTouch (&CCSHL623_BOUNCE_BOX::combat_touch);
}

void CCSHL623_BOUNCE_BOX::combat_touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->flags & FL_CLIENT ){
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pOther->pev);

		if ( FClassnameIs( pev, "cshl623_bounce_box" ) ){
			if(pev->armortype == 1){//ҽ�ư�x3
				pPlayer->GiveNamedItem( "weapon_medkit" );
				pPlayer->GiveNamedItem( "weapon_medkit" );
				pPlayer->GiveNamedItem( "weapon_medkit" );
			}
			else if(pev->armortype == 2){//ըҩ��x3
				pPlayer->GiveNamedItem( "weapon_satchel" );
				pPlayer->GiveNamedItem( "weapon_satchel" );
				pPlayer->GiveNamedItem( "weapon_satchel" );
			}
			else if(pev->armortype == 3){//HECU��װ
				pPlayer->GiveNamedItem( "weapon_9mmAR" );
				pPlayer->GiveNamedItem( "weapon_sg550" );
				pPlayer->GiveNamedItem( "item_armor2" );
				pPlayer->GiveAmmo( 180, "556nato", M16_MAX_CARRY );
				pPlayer->GiveAmmo( 2, "ARgrenades", M203_GRENADE_MAX_CARRY );
			}
			else if(pev->armortype == 4){//Ů�̿���װ
				pPlayer->GiveNamedItem( "weapon_crossbow" );
				pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
				pPlayer->GiveNamedItem( "weapon_handgrenade" );
				pPlayer->GiveNamedItem( "ammo_9mmbox" );
				pPlayer->GiveAmmo( 30, "bolts", BOLT_MAX_CARRY );
			}
			else if(pev->armortype == 5){//ȫ��ҩ��������!��С��
				pPlayer->GiveAmmo( 40, "buckshot", BUCKSHOT_MAX_CARRY );
				pPlayer->GiveAmmo( 3, "rockets", ROCKET_MAX_CARRY );
				pPlayer->GiveAmmo( 90, "762nato", AK_MAX_CARRY );
				pPlayer->GiveAmmo( 90, "556nato", M16_MAX_CARRY );
				pPlayer->GiveAmmo( 15, "bolts", BOLT_MAX_CARRY );
				pPlayer->GiveAmmo( 3, "ARgrenades", M203_GRENADE_MAX_CARRY );
				pPlayer->GiveAmmo( 14, "357", _357_MAX_CARRY );
				pPlayer->GiveAmmo( 10, "338mag", SNIPER_MAX_CARRY );
				pPlayer->GiveAmmo( 120, "45acp", MAC_MAX_CARRY );
				pPlayer->GiveAmmo( 120, "9mm", _9MM_MAX_CARRY );
				pPlayer->GiveAmmo( 100, "762natobox", MINIGUN_MAX_CARRY );
				pPlayer->GiveAmmo( 20, "uranium", URANIUM_MAX_CARRY );
			}
			else if(pev->armortype == 6){//������װ
				pPlayer->GiveNamedItem( "weapon_hornetgun" );
				pPlayer->GiveNamedItem( "weapon_snark_full" );
			}
			else if(pev->armortype == 7){//��ǹ��װ
				pPlayer->GiveNamedItem( "weapon_minigun" );
				pPlayer->GiveAmmo( 200, "762natobox", MINIGUN_MAX_CARRY );
			}
			else if(pev->armortype == 8){//ȫ��ҩ��������!����
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
			}
			SetThink( NULL );
			UTIL_Remove( this );
			return;
		}
	}
}

//=========================================================
// ��ͼս���㡤��
//=========================================================
class CCSHL623_COMBAT_POINT_LONG : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT combat_think ( void );
	void	EXPORT combat_touch ( CBaseEntity *pOther );
	void	SetObjectCollisionBox( void );

	CBasePlayer *pPlayer;
	CBaseMonster *pEnemyMonster;
	CBaseEntity *pMonsterEntity;
	Vector mso,mso2,mso3,mso4;
};

void CCSHL623_COMBAT_POINT_LONG :: SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-64, -64, 0);
	pev->absmax = pev->origin + Vector(64, 64, 64); 
}

LINK_ENTITY_TO_CLASS( cshl623_combat_mode_long, CCSHL623_COMBAT_POINT_LONG );//����ս������

void CCSHL623_COMBAT_POINT_LONG::Precache()
{
	//��һ��
	UTIL_PrecacheOther( "monster_revenant" );
	UTIL_PrecacheOther( "monster_sewblade" );
	UTIL_PrecacheOther( "monster_thrower" );
	UTIL_PrecacheOther( "monster_chainsaw_boss" );
	//�ڶ���
	UTIL_PrecacheOther( "monster_hellslave" );
	UTIL_PrecacheOther( "monster_alien_grunt_big" );
	UTIL_PrecacheOther( "monster_alien_xing_tian" );
	UTIL_PrecacheOther( "monster_alien_controller_big" );
	//������
	UTIL_PrecacheOther( "monster_majo" );
	UTIL_PrecacheOther( "monster_stone_devil" );
	UTIL_PrecacheOther( "monster_devil_wing" );
	UTIL_PrecacheOther( "monster_purple_guy" );
	//���Ĺ�
	UTIL_PrecacheOther( "monster_gargantua_hell" );
	UTIL_PrecacheOther( "monster_hydra_boss" );
	//�����
	UTIL_PrecacheOther( "monster_human_grunt" );
	UTIL_PrecacheOther( "monster_human_grunt_shape" );
	UTIL_PrecacheOther( "monster_luigi" );
	UTIL_PrecacheOther( "monster_human_assault" );
	UTIL_PrecacheOther( "monster_human_fassn" );
	UTIL_PrecacheOther( "monster_apache" );
}

void CCSHL623_COMBAT_POINT_LONG::Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/flag.mdl");

	pev->movetype = MOVETYPE_NONE;

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( pev, pev->origin );

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.

	SetThink (&CCSHL623_COMBAT_POINT_LONG::combat_think);
	SetTouch (&CCSHL623_COMBAT_POINT_LONG::combat_touch);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CCSHL623_COMBAT_POINT_LONG::combat_touch ( CBaseEntity *pOther )
{
	if ( (pOther->pev->flags & FL_CLIENT) && pev->health == 0 ){
		if(!pPlayer){//��ҡ���Fa�㷨
		pPlayer = GetClassPtr((CBasePlayer *)pOther->pev);
		}
		pev->health = 1;
		pev->armortype += 1;
		pPlayer->pev->v_angle = Vector(0,90,0);
		pPlayer->pev->angles = Vector(0,90,0);
		pPlayer->pev->fixangle = TRUE;
		pPlayer->EnableControl(FALSE);
		pPlayer->TeamMate_Nagamatagi_RespawnStone(0);
		pPlayer->TeamMate_Nagamatagi_Teleport(3);
		pev->effects = EF_NODRAW;

		pev->nextthink = gpGlobals->time + 0.1;

		if(pev->armortype >= 2){
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_wall_backwards" );
			if ( pTel ){
				if(pev->armortype == 5){
				pTel->pev->origin.y += 2120;
				}
				else{
				pTel->pev->origin.y += 1200;
				}
			UTIL_SetOrigin( pTel->pev, pTel->pev->origin);
			}
			pTel = UTIL_FindEntityByTargetname( NULL, "monster_multi_r_sp" );
			if ( pTel ){
				if(pev->armortype == 5){
				pTel->pev->origin.y += 2400;
				}
				else{
				pTel->pev->origin.y += 1200;
				}
			}
		}
	}
}

void CCSHL623_COMBAT_POINT_LONG::combat_think ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	if(pev->health > 0){//��ʼ����

			if(pev->frags == 145){//����һ��һ��һ��һ�������������ʵ�壡
					CBaseEntity *pEntity = NULL;
					while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 4096 )) != NULL)
					{
						if(pev->armortype == 5){
							if(FClassnameIs( pEntity->pev, "func_tankmortar") && pEntity->pev->frags > 0){
								pev->frags = 130;
								break;
							}
						}
						if ( (pEntity->pev->flags & FL_MONSTER) ){
							if( pEntity->pev->origin.z < (pev->origin.z + 800) ){
								if(pEntity->Classify() != CLASS_PLAYER_ALLY && pEntity->Classify() != CLASS_NONE
								&& pEntity->Classify() != CLASS_PLAYER_BIOWEAPON){
								pev->frags = 130;
								break;
								}
							}
						}
					}
			}

			if(pev->frags == 160){//ͨ��!
				pev->health = 0;
				pev->effects = 0;
				pev->frags = 0;
				
				if(pev->armortype == 5){
					SERVER_COMMAND("mp3 stop\n");
					pPlayer->m_music_save = 0;
					FireTargets( "brktank_body", this, this, USE_TOGGLE, 0 );
					FireTargets( "longlong_combat_over", this, this, USE_TOGGLE, 0 );
					UTIL_Remove( this );//������!
					return;
				}

				CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_wall_forward" );
				if ( pTel ){
						if(pev->armortype == 4){
						UTIL_Remove( pTel );//ɾ��
						pev->origin.y += 2400;
						UTIL_SetOrigin( pev, pev->origin);
						}
						else{
						pTel->pev->origin.y += 1200;
						pev->origin.y += 1200;
						UTIL_SetOrigin( pev, pev->origin);
						UTIL_SetOrigin( pTel->pev, pTel->pev->origin);
						}
				}
				return;
			}

	if(pev->armortype <= 4 && pev->frags <= 40){
		CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "monster_multi_r_sp" );
		if ( pTel ){

			if(pev->armortype == 1){//ս��1���־�֮��+����Σ��
				mso = pTel->pev->origin + Vector(-128,0,64);
				mso2 = pTel->pev->origin + Vector(1024,0,64);
				mso3 = pTel->pev->origin + Vector(32,0,0);
				if(pev->frags == 5){//Ͷ����x2
					pMonsterEntity = Create( "monster_thrower", mso, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					pMonsterEntity = Create( "monster_thrower", mso2, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					for ( int i = 0; i < 6; i++ ){//������x6
					pMonsterEntity = Create( "monster_revenant", mso3 + Vector(160*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
				}
				if(pev->frags == 10){//Ͷ����x2
					mso.y -= 256;
					mso2.y -= 256;
					mso3.y -= 128;

					pMonsterEntity = Create( "monster_thrower", mso, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					pMonsterEntity = Create( "monster_thrower", mso2, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					for ( int i = 0; i < 6; i++ ){//��ˮ��x6
					pMonsterEntity = Create( "monster_sewblade", mso3 + Vector(160*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					//�ھ�
					pMonsterEntity = Create( "monster_chainsaw_boss", mso3 + Vector(400,-150,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				if(pev->frags == 20){
					SERVER_COMMAND( "autosave\n" );
				}
				if(pev->frags == 40){
					pPlayer->EnableControl(TRUE);
					pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_mswall_forward" );
					if ( pTel ){
					pTel->pev->origin.y += 1200;
					UTIL_SetOrigin( pTel->pev, pTel->pev->origin);
					}
					pPlayer->m_music_save = 19;
					CLIENT_COMMAND(pPlayer->edict(), "cd loop 24\n");
					//SERVER_COMMAND("mp3 loop media/music24.mp3\n");
				}
			}

			if(pev->armortype == 2){//ս��2����������
				mso = pTel->pev->origin + Vector(0,0,512);
				mso2 = pTel->pev->origin + Vector(-128,-64,64);
				mso3 = pTel->pev->origin;
				mso4 = pTel->pev->origin + Vector(1024,-64,64);
				if(pev->frags == 5){
					for ( int i = 0; i < 4; i++ ){//��ͷx4
					pMonsterEntity = Create( "monster_alien_controller_big", mso + Vector(256*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					for ( int i = 0; i < 3; i++ ){//�������ظ�x3����
					pMonsterEntity = Create( "monster_hellslave", mso2 + Vector(0,-128*i,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					for ( int i = 0; i < 3; i++ ){//�������ظ�x3����
					pMonsterEntity = Create( "monster_hellslave", mso4 + Vector(0,-128*i,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					for ( int i = 0; i < 4; i++ ){//�����ֹ�x4
					pMonsterEntity = Create( "monster_alien_grunt_big", mso3 + Vector(256*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					//����
					pMonsterEntity = Create( "monster_alien_xing_tian", mso3 + Vector(384,-256,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				if(pev->frags == 40){
					pPlayer->EnableControl(TRUE);
					pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_mswall_forward" );
					if ( pTel ){
					pTel->pev->origin.y += 1200;
					UTIL_SetOrigin( pTel->pev, pTel->pev->origin);
					}
				}
			}

			if(pev->armortype == 3){//ս��3����������
				mso = pTel->pev->origin + Vector(0,0,512);
				mso2 = pTel->pev->origin + Vector(-128,-64,64);
				mso3 = pTel->pev->origin;
				mso4 = pTel->pev->origin + Vector(1024,-64,64);
				if(pev->frags == 5){
					for ( int i = 0; i < 6; i++ ){//����ħŮx6
					pMonsterEntity = Create( "monster_majo", mso + Vector(180*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					pEnemyMonster->pev->body = 1;
					}
					for ( int i = 0; i < 3; i++ ){//��ħ֮��x3����
					pMonsterEntity = Create( "monster_devil_wing", mso2 + Vector(0,-150*i,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					for ( int i = 0; i < 3; i++ ){//��ħ֮��x3����
					pMonsterEntity = Create( "monster_devil_wing", mso4 + Vector(0,-150*i,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
					for ( int i = 0; i < 4; i++ ){//ʯ֮��ħx4
					pMonsterEntity = Create( "monster_stone_devil_s", mso3 + Vector(256*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
					}
				}
				if(pev->frags == 10){
					//��ɫ�ֹ�x3
					pMonsterEntity = Create( "monster_purple_guy", mso3 + Vector(192,-256,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					pMonsterEntity = Create( "monster_purple_guy", mso3 + Vector(384,-256,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					pMonsterEntity = Create( "monster_purple_guy", mso3 + Vector(576,-256,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				if(pev->frags == 40){
					pPlayer->EnableControl(TRUE);
					pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_mswall_forward" );
					if ( pTel ){
					pTel->pev->origin.y += 1200;
					UTIL_SetOrigin( pTel->pev, pTel->pev->origin);
					}
				}
			}

			if(pev->armortype == 4){//ս��4������ͷx3
				mso = pTel->pev->origin + Vector(120,-128,0);
				mso2 = pTel->pev->origin + Vector(800,-128,0);
				mso3 = pTel->pev->origin + Vector(480,-256,0);
				if(pev->frags == 5){
					//��������x2
					pMonsterEntity = Create( "monster_gargantua_hell", mso, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					pMonsterEntity = Create( "monster_gargantua_hell", mso2, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;

					//���ʷ�Ϲ�x1
					pMonsterEntity = Create( "monster_hydra_boss", mso3, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 1;
				}
				if(pev->frags == 40){
					pPlayer->EnableControl(TRUE);
					pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_mswall_forward" );
					if ( pTel ){
					pTel->pev->origin.y += 2400;
					UTIL_SetOrigin( pTel->pev, pTel->pev->origin);
					}
				}
			}
		}
	}
	else{
		if(pev->armortype == 5){//ս��5��������
	
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "monster_multi_r_sp2" );
			if ( pTel ){
				mso = pTel->pev->origin;
			}
			pTel = UTIL_FindEntityByTargetname( NULL, "monster_multi_r_sp3" );
			if ( pTel ){
				mso2 = pTel->pev->origin;
				mso3 = pTel->pev->origin + Vector(-192,256,32);
			}

				if(pev->frags == 5){
					//Ů�̿͡���x6
					for ( int i = 0; i < 6; i++ ){
					pMonsterEntity = Create( "monster_human_fassn", mso + Vector(192*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					pEnemyMonster->m_makerspawn_call = 1;
					pEnemyMonster->pev->weapons = 2;
					}
					//��ͨHECU x10
					for ( int i = 0; i < 10; i++ ){
					pMonsterEntity = Create( "monster_human_grunt", mso3 + Vector(128*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					pEnemyMonster->m_makerspawn_call = 1;
					pEnemyMonster->pev->weapons = 1;
					}
				}
				if(pev->frags == 10){
					//��ǹHECU��x2
					pMonsterEntity = Create( "monster_human_assault", mso2, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;

					pMonsterEntity = Create( "monster_human_assault", mso2 + Vector(820,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;

					//HECU Shape
					pMonsterEntity = Create( "monster_human_grunt_shape", mso2 + Vector(450,-384,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;

					//·�׼�
					pMonsterEntity = Create( "monster_luigi", mso2 + Vector(424,0,120), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					pEnemyMonster->m_die_for_back = 3;//ʣ��������������
					pEnemyMonster->pev->armorvalue = 50;//���ּ���

					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "brktank_mortar" );
					if ( pTel ){
					pTel->pev->armortype = 0;
					pTel->pev->frags = 1;//���ƻ�
					}
				}
				if(pev->frags == 40){
					pPlayer->EnableControl(TRUE);
					pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "longlong_mswall_forward" );
					if ( pTel ){
					UTIL_Remove( pTel );
					}
				}
				if(pev->frags == 90){//ʱͣ����Ԯ
				pev->frags += 1;
				pev->nextthink = gpGlobals->time + 5.0;
				return;
				}
				if(pev->frags == 93){//�ս�����Ԯ x 5
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "apache_track1" );
					if ( pTel ){
					pMonsterEntity = Create( "monster_grunt_repel", pTel->pev->origin + Vector(0,1280,0), pTel->pev->angles, edict() );
					pMonsterEntity->pev->frags = 2;
					pMonsterEntity = Create( "monster_grunt_repel", pTel->pev->origin + Vector(-128,1280,0), pTel->pev->angles, edict() );
					pMonsterEntity->pev->frags = 2;
					pMonsterEntity = Create( "monster_grunt_repel", pTel->pev->origin + Vector(-256,1280,0), pTel->pev->angles, edict() );
					pMonsterEntity->pev->frags = 2;
					pMonsterEntity = Create( "monster_grunt_repel", pTel->pev->origin + Vector(128,1280,0), pTel->pev->angles, edict() );
					pMonsterEntity->pev->frags = 2;
					pMonsterEntity = Create( "monster_grunt_repel", pTel->pev->origin + Vector(256,1280,0), pTel->pev->angles, edict() );
					pMonsterEntity->pev->frags = 2;
					}
				}
				if(pev->frags == 95){//ʱͣ����Ԯ
				pev->frags += 1;
				pev->nextthink = gpGlobals->time + 4.0;
				return;
				}
				if(pev->frags == 100){//ֱ��������Ԯ
					CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "apache_track1" );
					if ( pTel ){
					pMonsterEntity = Create( "monster_apache", pTel->pev->origin, pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->pev->target = MAKE_STRING("apache_track1");
					}
				}
				if(pev->frags == 115){//ʱͣ����Ԯ
				pev->frags += 1;
				pev->nextthink = gpGlobals->time + 4.0;
				return;
				}
				if(pev->frags == 120){//С�Ť���Ԯ
				FireTargets( "hecu_fiona_add_dr", this, this, USE_TOGGLE, 0 );
				}
		}
	}
		pev->frags += 1;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// ��ͼս����
//=========================================================
class CCSHL623_COMBAT_POINT : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT combat_think ( void );
	void	EXPORT combat_think2 ( void );
	void	EXPORT combat_touch ( CBaseEntity *pOther );
	void	SetObjectCollisionBox( void );

	void DeathNotice ( entvars_t *pevChild );
	CBasePlayer *pPlayer;
	CBaseMonster *pEnemyMonster;
	CBaseEntity *pMonsterEntity;
	Vector mso,mso2,mso3,mso4;
};

LINK_ENTITY_TO_CLASS( cshl623_combat_point, CCSHL623_COMBAT_POINT );
LINK_ENTITY_TO_CLASS( cshl623_combat_point2, CCSHL623_COMBAT_POINT );//��ߴ�
LINK_ENTITY_TO_CLASS( cshl623_combat_mode, CCSHL623_COMBAT_POINT );//����ս��

void CCSHL623_COMBAT_POINT::Precache()
{//RPG Mode������б�
	//��һ�� x 12
	UTIL_PrecacheOther( "monster_else_rabbit" );
	UTIL_PrecacheOther( "monster_bloodsucker" );
	UTIL_PrecacheOther( "monster_skull_bear" );
	UTIL_PrecacheOther( "monster_unknow_melon" );
	UTIL_PrecacheOther( "monster_stone_devil" );
	UTIL_PrecacheOther( "monster_majo" );
	UTIL_PrecacheOther( "monster_else_zombie" );
	UTIL_PrecacheOther( "monster_vortigaunt" );
	UTIL_PrecacheOther( "monster_bullchicken_big" );
	UTIL_PrecacheOther( "monster_human_grunt" );
	UTIL_PrecacheOther( "monster_human_fassn" );
	UTIL_PrecacheOther( "monster_gargantua_hell" );
	//�ڶ��� x 10
	UTIL_PrecacheOther( "monster_nirvana_death" );
	UTIL_PrecacheOther( "monster_alien_grunt_big" );
	UTIL_PrecacheOther( "monster_alien_controller_big" );
	UTIL_PrecacheOther( "monster_houndeye_big" );
	UTIL_PrecacheOther( "monster_shadow" );
	UTIL_PrecacheOther( "monster_cthonian" );
	UTIL_PrecacheOther( "monster_snake_women" );
	UTIL_PrecacheOther( "monster_hunt_sworder" );
	UTIL_PrecacheOther( "monster_barnacle_fantasy" );
	UTIL_PrecacheOther( "monster_human_assault" );
}

void CCSHL623_COMBAT_POINT :: SetObjectCollisionBox( void )
{
	if ( FClassnameIs( pev, "cshl623_combat_point2" ) ){
	pev->absmin = pev->origin + Vector(-384, -384, 0);
	pev->absmax = pev->origin + Vector(384, 384, 384); 
	}
	else if ( FClassnameIs( pev, "cshl623_combat_point" ) ){
	pev->absmin = pev->origin + Vector(-192, -192, 0);
	pev->absmax = pev->origin + Vector(192, 192, 384); 
	}
	else{
	pev->absmin = pev->origin + Vector(-32, -32, 0);
	pev->absmax = pev->origin + Vector(32, 32, 32); 
	}
}

void CCSHL623_COMBAT_POINT :: DeathNotice ( entvars_t *pevChild )
{
	pev->health--;
	pevChild->owner = NULL;
}

void CCSHL623_COMBAT_POINT::Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/player.mdl");

	//pev->solid	  = SOLID_BBOX;
	pev->movetype = MOVETYPE_NONE;

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( pev, pev->origin );

	pev->effects |= EF_NODRAW;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.

	//pev->effects |= EF_BRIGHTFIELD;
	//pev->framerate = 1.0;
	if(FClassnameIs( pev, "cshl623_combat_mode")){
	SetThink (&CCSHL623_COMBAT_POINT::combat_think2);
	}
	else{
	SetTouch (&CCSHL623_COMBAT_POINT::combat_touch);
	SetThink (&CCSHL623_COMBAT_POINT::combat_think);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CCSHL623_COMBAT_POINT::combat_touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->flags & FL_CLIENT ){
		if(!pPlayer){//��ҡ���Fa�㷨
		pPlayer = GetClassPtr((CBasePlayer *)pOther->pev);
		}

		CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "combat_in_target" );
		if ( pTel ){
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_NETWORKVOICE_BASE, "debris/beamstart3.wav", 1, 0.7);
		UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 1, 255, FFADE_OUT );//��Ϲ���
		pev->frags = 1;//ս��������ʼ!
		pPlayer->m_music_save = 0;
		SERVER_COMMAND("mp3 stop\n");
		pPlayer->EnableControl(FALSE);

		if (pPlayer->m_team_npc1 != NULL){
			if(pPlayer->m_team_npc1->IsAlive()){
				pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
				if ( pEnemyMonster ){
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->angles.y = 90;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->m_hEnemy = NULL;
				}
			}
		}

		if (pPlayer->m_team_npc2 != NULL){
			if(pPlayer->m_team_npc2->IsAlive()){
				pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
				if ( pEnemyMonster ){
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->angles.y = 90;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->m_hEnemy = NULL;
				}
			}
		}

		if (pPlayer->m_team_npc3 != NULL){
			if(pPlayer->m_team_npc3->IsAlive()){
				pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
				if ( pEnemyMonster ){
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->angles.y = 90;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->m_hEnemy = NULL;
				}
			}
		}

		if (pPlayer->m_team_npc4 != NULL){
			if(pPlayer->m_team_npc4->IsAlive()){
				pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
				if ( pEnemyMonster ){
				pEnemyMonster->ClearSchedule();
				pEnemyMonster->SetActivity( ACT_IDLE );
				pEnemyMonster->SetState( MONSTERSTATE_HUNT );
				pEnemyMonster->pev->angles.y = 90;
				pEnemyMonster->pev->yaw_speed = 0;
				pEnemyMonster->m_hEnemy = NULL;
				}
			}
		}

		pev->solid	  = SOLID_NOT;
		pev->effects  = EF_NODRAW;
		}
		pTel = UTIL_FindEntityByTargetname( NULL, "player_combat_out" );
		if ( pTel ){
		pTel->pev->origin = pev->origin + Vector(0,0,36);
		}
	}
}

void CCSHL623_COMBAT_POINT::combat_think ( void )
{
	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	if(pev->frags == 5){
		CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_combat_in" );
		if ( pTel ){
		pPlayer->pev->origin = pTel->pev->origin;
		pPlayer->pev->angles = pTel->pev->angles;
		pPlayer->pev->v_angle = pPlayer->pev->angles;
		pPlayer->pev->velocity = g_vecZero;
		pPlayer->pev->fixangle = TRUE;
		UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
		FX_Explosion( pev->origin, 254 );//��������Ѫ������!
		}
	}

	if(pev->frags == 15){//�������
		CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "monster_combat_in" );
		if ( pTel ){
			if(pev->armortype == 1){//��ͨ����x10
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 10; i++ ){
				pMonsterEntity = Create( "monster_else_rabbit", mso + Vector(64*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_else_rabbit", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 10;
			}
			else if(pev->armortype == 2){//��������x10
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 10; i++ ){
				pMonsterEntity = Create( "monster_else_rabbit", mso + Vector(64*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				pMonsterEntity->pev->skin = 1;
				}
				pMonsterEntity = Create( "monster_else_rabbit", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->pev->skin = 1;
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 10;
			}
			else if(pev->armortype == 3){//��Ѫħx8
				mso = pTel->pev->origin - Vector(400,200,0);
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_bloodsucker", mso + Vector(120*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_bloodsucker", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 8;
			}
			else if(pev->armortype == 4){//������x8
				mso = pTel->pev->origin - Vector(400,200,0);
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_skull_bear", mso + Vector(120*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_skull_bear", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 8;
			}
			else if(pev->armortype == 5){//������x12
				mso = pTel->pev->origin - Vector(300,150,0);
				for ( int i = 0; i < 12; i++ ){
				pMonsterEntity = Create( "monster_unknow_melon", mso + Vector(120*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
					if(i == 5){
					mso = mso - Vector(720,150,0);
					}
				}
				pMonsterEntity = Create( "monster_unknow_melon", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 6){//ʯ֮��ħx5
				mso = pTel->pev->origin - Vector(300,0,0);
				for ( int i = 0; i < 5; i++ ){
				pMonsterEntity = Create( "monster_stone_devil_h", mso + Vector(150*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_stone_devil_h", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 5;
			}
			else if(pev->armortype == 7){//ħŮx7
				mso = pTel->pev->origin - Vector(300,100,0);
				for ( int i = 0; i < 7; i++ ){
				pMonsterEntity = Create( "monster_majo", mso + Vector(100*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_majo", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 7;
			}
			else if(pev->armortype == 8){//��ͽ��ʬx18
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 18; i++ ){
				pMonsterEntity = Create( "monster_else_zombie", mso + Vector(80*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
					if(i == 8){
					mso = mso - Vector(720,100,0);
					}
				}
				pMonsterEntity = Create( "monster_else_zombie", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 18;
			}
			else if(pev->armortype == 9){//�ƽ���Ѫħx8
				mso = pTel->pev->origin - Vector(400,200,0);
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_bloodsucker", mso + Vector(120*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				pMonsterEntity->pev->body = 1;
				}
				pMonsterEntity = Create( "monster_bloodsucker", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->pev->body = 1;
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 8;
			}
			else if(pev->armortype == 10){//���͸��ٸ�x7
				mso = pTel->pev->origin - Vector(384,0,0);
				for ( int i = 0; i < 7; i++ ){
				pMonsterEntity = Create( "monster_vortigaunt", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_vortigaunt", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 7;
			}
			else if(pev->armortype == 11){//����ţ����x9
				mso = pTel->pev->origin - Vector(400,100,0);
				for ( int i = 0; i < 9; i++ ){
				pMonsterEntity = Create( "monster_bullchicken_big", mso + Vector(100*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_bullchicken_big", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 9;
			}
			else if(pev->armortype == 12){//���͸��ٸ�x7 + ����ţ����x8
				mso = pTel->pev->origin - Vector(384,0,0);
				for ( int i = 0; i < 15; i++ ){
					if(i < 7){
					pMonsterEntity = Create( "monster_vortigaunt", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					}
					else{
						if(i == 7){
						mso = mso - Vector(800,200,0);
						}
						pMonsterEntity = Create( "monster_bullchicken_big", mso + Vector(110*i,0,0), pTel->pev->angles, edict() );
						pEnemyMonster = pMonsterEntity->MyMonsterPointer();
						pEnemyMonster->m_fightmode = 2;
					}
				}
				pMonsterEntity = Create( "monster_vortigaunt", pev->origin + Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��x1
				pMonsterEntity = Create( "monster_bullchicken_big", pev->origin - Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��x2
				pev->health = 15;
			}
			else if(pev->armortype == 13){//ʯ֮��ħx6 + �ƽ���Ѫħx8
				mso = pTel->pev->origin - Vector(300,0,0);
				for ( int i = 0; i < 14; i++ ){
					if(i < 6){
					pMonsterEntity = Create( "monster_stone_devil", mso + Vector(150*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					pEnemyMonster->m_flDistLook = 4096;
					}
					else{
						if(i == 6){
						mso = mso - Vector(840,200,0);
						}
						pMonsterEntity = Create( "monster_bloodsucker", mso + Vector(120*i,0,0), pTel->pev->angles, edict() );
						pEnemyMonster = pMonsterEntity->MyMonsterPointer();
						pMonsterEntity->pev->body = 1;
						pEnemyMonster->m_fightmode = 2;
					}
				}
				pMonsterEntity = Create( "monster_stone_devil_h", pev->origin + Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��x1
				pMonsterEntity = Create( "monster_bloodsucker", pev->origin - Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->pev->body = 1;
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��x2
				pev->health = 14;
			}
			else if(pev->armortype == 14){//��ͨħŮx6 + ����ħŮx6
				mso = pTel->pev->origin - Vector(300,150,0);
				for ( int i = 0; i < 12; i++ ){
				pMonsterEntity = Create( "monster_majo", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
					if(i == 5){
					mso = mso - Vector(768,150,-450);
					}
					if(i >= 6){
					pMonsterEntity->pev->body = 1;
					}
				}
				pMonsterEntity = Create( "monster_majo", pev->origin + Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pMonsterEntity = Create( "monster_majo", pev->origin - Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 15){//������x12 + ����ħŮx6
				mso = pTel->pev->origin - Vector(400,150,0);
				for ( int i = 0; i < 18; i++ ){
					if(i < 12){
					pMonsterEntity = Create( "monster_unknow_melon", mso + Vector(70*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					}
					else if(i >= 12){
						if(i == 12){
						mso = mso - Vector(1450,0,-450);
						}
					pMonsterEntity = Create( "monster_majo", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					pMonsterEntity->pev->body = 1;
					}
				}
				pMonsterEntity = Create( "monster_unknow_melon", pev->origin + Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pMonsterEntity = Create( "monster_majo", pev->origin - Vector(64,0,0), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 18;
			}
			else if(pev->armortype == 16){//HECUx12
				mso = pTel->pev->origin - Vector(400,100,0);
				for ( int i = 0; i < 12; i++ ){
				pMonsterEntity = Create( "monster_human_grunt", mso + Vector(75*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
					if(i == 1 || i == 3 || i == 5 || i == 7 || i == 9 || i == 11){
					pMonsterEntity->pev->weapons = 16;
					}
					else{
					pMonsterEntity->pev->weapons = 1;
					}
					pEnemyMonster->m_makerspawn_call = 1;
				}
				pMonsterEntity = Create( "monster_human_grunt", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->pev->weapons = 0;
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 17){//Fassnx9
				mso = pTel->pev->origin - Vector(400,100,0);
				for ( int i = 0; i < 9; i++ ){
				pMonsterEntity = Create( "monster_human_fassn", mso + Vector(100*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
					if(i == 1 || i == 3 || i == 5 || i == 7){
					pMonsterEntity->pev->weapons = 2;
					}
					else{
					pMonsterEntity->pev->weapons = 1;
					}
					pEnemyMonster->m_makerspawn_call = 1;
				}
				pMonsterEntity = Create( "monster_human_fassn", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 9;
			}
			else if(pev->armortype == 18){//Garg Hell x2
				mso = pTel->pev->origin - Vector(200,0,0);
				for ( int i = 0; i < 2; i++ ){
				pMonsterEntity = Create( "monster_gargantua_hell", mso + Vector(400*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pev->health = 2;
			}
			else if(pev->armortype == 19){//��Ӱx6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_shadow", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_shadow", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 20){//�ڶ���x6
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_nirvana_death", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_nirvana_death", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 21){//����Ȯx8
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_houndeye_big", mso + Vector(96*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_houndeye_big", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 8;
			}
			else if(pev->armortype == 22){//������ͷx6
				mso = pTel->pev->origin - Vector(320,0,-320);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_alien_controller_big", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_alien_controller_big", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 23){//�����x6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_cthonian", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_cthonian", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 24){//�����ֹ�x6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_alien_grunt_big", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_alien_grunt_big", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 25){//�����x6 + ������ͷx6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_cthonian", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;

				pMonsterEntity = Create( "monster_alien_controller_big", mso + Vector(128*i,0,320), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_cthonian", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pMonsterEntity = Create( "monster_alien_controller_big", pev->origin + Vector(0,0,128), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 26){//��Ůx6
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_snake_women", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
					if(i >= 3){
					pMonsterEntity->pev->body = i - 3;
					}
					else{
					pMonsterEntity->pev->body = i;
					}
				}
				pMonsterEntity = Create( "monster_snake_women", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 27){//��ɱ����x6
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_hunt_sworder", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_hunt_sworder", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 6;
			}
			else if(pev->armortype == 28){//�����ٺ�x6
				mso = pTel->pev->origin - Vector(320,0,-320);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_barnacle_fantasy", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pev->health = 6;
			}
			else if(pev->armortype == 29){//��ɱ����x6 + �����ٺ�x6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_hunt_sworder", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;

				pMonsterEntity = Create( "monster_barnacle_fantasy", mso + Vector(128*i,0,320), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_hunt_sworder", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 30){//�����ֹ�x6 + ������ͷx6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_alien_grunt_big", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;

				pMonsterEntity = Create( "monster_alien_controller_big", mso + Vector(128*i,0,320), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_alien_grunt_big", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pMonsterEntity = Create( "monster_alien_controller_big", pev->origin + Vector(0,0,128), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 31){//�����x6 + �����ٺ�Rx6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_cthonian", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;

				pMonsterEntity = Create( "monster_barnacle_fantasy_r", mso + Vector(128*i,0,320), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_cthonian", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 12;
			}
			else if(pev->armortype == 32){//����Ȯx8 + ��Ůx6
				mso = pTel->pev->origin - Vector(320,0,0);
				for ( int i = 0; i < 14; i++ ){
					if(i < 8){
					pMonsterEntity = Create( "monster_houndeye_big", mso + Vector(96*i,0,0), pTel->pev->angles, edict() );
					pEnemyMonster = pMonsterEntity->MyMonsterPointer();
					pEnemyMonster->m_fightmode = 2;
					}
					else{
						if(i == 8){
						mso = pTel->pev->origin - Vector(1328,200,0);
						}
						pMonsterEntity = Create( "monster_snake_women", mso + Vector(128*i,0,0), pTel->pev->angles, edict() );
						pEnemyMonster = pMonsterEntity->MyMonsterPointer();
						pEnemyMonster->m_fightmode = 2;
						if(i >= 11){
						pMonsterEntity->pev->body = i - 3;
						}
						else{
						pMonsterEntity->pev->body = i;
						}
					}
				}
				pMonsterEntity = Create( "monster_houndeye_big", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pMonsterEntity = Create( "monster_snake_women", pev->origin + Vector(0,0,64), pTel->pev->angles, edict() );
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 14;
			}
			else if(pev->armortype == 33){//��ǹ����x8
				mso = pTel->pev->origin - Vector(320,100,0);
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_human_assault", mso + Vector(96*i,0,0), pTel->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 2;
				}
				pMonsterEntity = Create( "monster_human_assault", pev->origin, pTel->pev->angles, edict() );
				pMonsterEntity->pev->weapons = 0;
				pMonsterEntity->Killed( pev, GIB_NEVER );//����ʬ��
				pev->health = 8;
			}
		}
	}

	if(pev->frags == 30){//�ӿ�ս�� Bug Fix 1.0
	pPlayer->TeamMate_Nagamatagi_Teleport(8);
	}

	if(pev->frags == 40){
	FireTargets( "combat_stuck_wall", this, this, USE_TOGGLE, 0 );
	pPlayer->EnableControl(TRUE);
	pPlayer->TeamMate_Nagamatagi_RespawnStone(2);
	CLIENT_COMMAND(pPlayer->edict(), "cd play 7\n");
	//SERVER_COMMAND("mp3 play media/music28.mp3\n");
	}

	if(pev->frags == 100){
		if(pev->health != 0){
		pev->frags = 90;
		}
	}
	if(pev->frags == 110){
		SERVER_COMMAND("mp3 stop\n");
	}
	if(pev->frags == 140){
		if(pev->health == 0){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_OUT );//��Ϲ���
		}
	}
	if(pev->frags == 147){
		if(pev->health == 0){
			FireTargets( "combat_stuck_wall", this, this, USE_TOGGLE, 0 );
			CBaseEntity *pTel = UTIL_FindEntityByTargetname( NULL, "player_combat_out" );
			if ( pTel ){
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 0.5, 0.5, 255, FFADE_IN );//��Ϲ���
			pPlayer->pev->origin = pTel->pev->origin;
			pPlayer->TeamMate_Nagamatagi_Teleport(11);
			pPlayer->m_flVelocityModifier = -1;
			}
		}
	}
	if(pev->frags == 160){
		pPlayer->m_music_save = 13;
		CLIENT_COMMAND(pPlayer->edict(), "cd loop 19\n");
	//	SERVER_COMMAND("mp3 loop media/music21.mp3\n");
	}
	if(pev->frags == 170){
		if(pev->health == 0){
			SetThink( NULL );
			UTIL_Remove( this );
			return;
		}
	}

	if(pev->frags >= 1){
	pev->frags += 1;
	pev->nextthink = gpGlobals->time + 0.1;
	}
	else{
	pev->nextthink = gpGlobals->time + 1.0;
	}
}

void CCSHL623_COMBAT_POINT::combat_think2 ( void )
{//����ս��

	if(!pPlayer){//��ҡ���Fa�㷨
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity ){
		pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		}
	}

	if(pev->frags == 80 && pev->health <= 0){//����������ֱ�
		pev->frags = 0;

		CBaseEntity *pT_R_F = UTIL_FindEntityByTargetname( NULL, "monster_multi_r_f" );
		CBaseEntity *pT_R_C = UTIL_FindEntityByTargetname( NULL, "monster_multi_r_c" );
		CBaseEntity *pT_L_F = UTIL_FindEntityByTargetname( NULL, "monster_multi_l_f" );
		CBaseEntity *pT_L_C = UTIL_FindEntityByTargetname( NULL, "monster_multi_l_c" );
			if(pev->armortype == 0){//��1�����ˡ���ͨ����x20
				mso = pT_R_F->pev->origin + Vector(-96,0,64);//�ҵ�
				for ( int i = 0; i < 10; i++ ){
				pMonsterEntity = Create( "monster_else_rabbit", mso + Vector(0,140*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(96,0,64);//���
				for ( int i2 = 0; i2 < 10; i2++ ){
				pMonsterEntity = Create( "monster_else_rabbit", mso2 + Vector(0,140*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 20;//����24ֻ����
				pev->armortype = 1;
			}
			else if(pev->armortype == 1){//��2�����ˡ�˭������x18
				mso = pT_R_F->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i = 0; i < 9; i++ ){
				pMonsterEntity = Create( "monster_unknow_melon", mso + Vector(0,150*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(32,0,32);//���
				for ( int i2 = 0; i2 < 9; i2++ ){
				pMonsterEntity = Create( "monster_unknow_melon", mso2 + Vector(0,150*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 18;
				pev->armortype = 2;
			}
			else if(pev->armortype == 2){//��3�����ˡ���Ѫħx16
				mso = pT_R_F->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_bloodsucker", mso + Vector(0,170*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(32,0,32);//���
				for ( int i2 = 0; i2 < 8; i2++ ){
				pMonsterEntity = Create( "monster_bloodsucker", mso2 + Vector(0,170*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 16;
				pev->armortype = 3;
			}
			else if(pev->armortype == 3){//��4�����ˡ�������x16
				mso = pT_R_F->pev->origin + Vector(-128,0,32);//�ҵ�
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_skull_bear", mso + Vector(0,170*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(128,0,32);//���
				for ( int i2 = 0; i2 < 8; i2++ ){
				pMonsterEntity = Create( "monster_skull_bear", mso2 + Vector(0,170*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 16;
				pev->armortype = 4;
			}
			else if(pev->armortype == 4){//��5�����ˡ�ħŮx16
				mso = pT_R_F->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i = 0; i < 4; i++ ){
				pMonsterEntity = Create( "monster_majo", mso + Vector(0,350*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(32,0,32);//���
				for ( int i2 = 0; i2 < 4; i2++ ){
				pMonsterEntity = Create( "monster_majo", mso2 + Vector(0,350*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso3 = pT_R_C->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i3 = 0; i3 < 4; i3++ ){
				pMonsterEntity = Create( "monster_majo", mso3 + Vector(0,350*i3,0), pT_R_C->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				pMonsterEntity->pev->body = 1;
				}
				mso4 = pT_L_C->pev->origin + Vector(32,0,32);//���
				for ( int i4 = 0; i4 < 4; i4++ ){
				pMonsterEntity = Create( "monster_majo", mso4 + Vector(0,350*i4,0), pT_L_C->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				pMonsterEntity->pev->body = 1;
				}
				pev->health = 16;
				pev->armortype = 5;
			}
			else if(pev->armortype == 5){//��6�����ˡ�ʯ֮��ħx8
				mso = pT_R_F->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i = 0; i < 4; i++ ){
				pMonsterEntity = Create( "monster_stone_devil_h", mso + Vector(0,350*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(32,0,32);//���
				for ( int i2 = 0; i2 < 4; i2++ ){
				pMonsterEntity = Create( "monster_stone_devil_h", mso2 + Vector(0,350*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 8;
				pev->armortype = 7;
			}
			else if(pev->armortype == 7){//ȫ��ָ�����һ���ֽ���
				UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 1, 255, FFADE_IN );
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/Flash3.wav", 1, 0);
				pPlayer->pev->health = pPlayer->pev->max_health;
				CBaseMonster *pEnemyMonster;
				if (pPlayer->m_team_npc1 != NULL){
				pEnemyMonster = pPlayer->m_team_npc1->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag != DEAD_NO){
				UTIL_SetOrigin ( pEnemyMonster->pev, pEnemyMonster->pev->origin + Vector(0,0,256) );
				}
				pEnemyMonster->Hunt_Stand_Set(3);
				}
				if (pPlayer->m_team_npc2 != NULL){
				pEnemyMonster = pPlayer->m_team_npc2->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag != DEAD_NO){
				UTIL_SetOrigin ( pEnemyMonster->pev, pEnemyMonster->pev->origin + Vector(0,0,256) );
				}
				pEnemyMonster->Hunt_Stand_Set(3);
				}
				if (pPlayer->m_team_npc3 != NULL){
				pEnemyMonster = pPlayer->m_team_npc3->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag != DEAD_NO){
				UTIL_SetOrigin ( pEnemyMonster->pev, pEnemyMonster->pev->origin + Vector(0,0,256) );
				}
				pEnemyMonster->Hunt_Stand_Set(3);
				}
				if (pPlayer->m_team_npc4 != NULL){
				pEnemyMonster = pPlayer->m_team_npc4->MyMonsterPointer();
				if(pEnemyMonster->pev->deadflag != DEAD_NO){
				UTIL_SetOrigin ( pEnemyMonster->pev, pEnemyMonster->pev->origin + Vector(0,0,256) );
				}
				pEnemyMonster->Hunt_Stand_Set(3);
				}
				pev->armortype = 8;
			}
			else if(pev->armortype == 8){//��7�����ˡ����͸��ٸ�x12
				mso = pT_R_F->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i = 0; i < 6; i++ ){
				pMonsterEntity = Create( "monster_vortigaunt", mso + Vector(0,230*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(32,0,32);//���
				for ( int i2 = 0; i2 < 6; i2++ ){
				pMonsterEntity = Create( "monster_vortigaunt", mso2 + Vector(0,230*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 12;
				pev->armortype = 9;
			}
			else if(pev->armortype == 9){//��8�����ˡ�����ţ����x16
				mso = pT_R_F->pev->origin + Vector(-32,0,32);//�ҵ�
				for ( int i = 0; i < 8; i++ ){
				pMonsterEntity = Create( "monster_bullchicken_big", mso + Vector(0,170*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(32,0,32);//���
				for ( int i2 = 0; i2 < 8; i2++ ){
				pMonsterEntity = Create( "monster_bullchicken_big", mso2 + Vector(0,170*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 16;
				pev->armortype = 10;
			}
			else if(pev->armortype == 10){//��9�����ˡ����͹���x20
				mso = pT_R_F->pev->origin + Vector(-96,0,64);//�ҵ�
				for ( int i = 0; i < 10; i++ ){
				pMonsterEntity = Create( "monster_houndeye_big", mso + Vector(0,140*i,0), pT_R_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				mso2 = pT_L_F->pev->origin + Vector(96,0,64);//���
				for ( int i2 = 0; i2 < 10; i2++ ){
				pMonsterEntity = Create( "monster_houndeye_big", mso2 + Vector(0,140*i2,0), pT_L_F->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				}
				pev->health = 20;
				pev->armortype = 11;
			}
			else if(pev->armortype == 11){//��10�����ˡ����ͷ�ͷx10
				mso3 = pT_R_C->pev->origin + Vector(-32,32,32);//�ҵ�
				for ( int i3 = 0; i3 < 5; i3++ ){
				pMonsterEntity = Create( "monster_alien_controller_big", mso3 + Vector(0,280*i3,0), pT_R_C->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				pMonsterEntity->pev->body = 1;
				}
				mso4 = pT_L_C->pev->origin + Vector(32,-32,32);//���
				for ( int i4 = 0; i4 < 5; i4++ ){
				pMonsterEntity = Create( "monster_alien_controller_big", mso4 + Vector(0,280*i4,0), pT_L_C->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				pMonsterEntity->pev->body = 1;
				}
				pev->health = 10;
				pev->armortype = 12;
			}
			else if(pev->armortype == 12){//��11�����ˡ������ٺ�x16
				mso3 = pT_R_C->pev->origin + Vector(-32,32,32);//�ҵ�
				for ( int i3 = 0; i3 < 8; i3++ ){
				pMonsterEntity = Create( "monster_barnacle_fantasy", mso3 + Vector(0,170*i3,0), pT_R_C->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				pMonsterEntity->pev->body = 1;
				}
				mso4 = pT_L_C->pev->origin + Vector(32,-32,32);//���
				for ( int i4 = 0; i4 < 8; i4++ ){
				pMonsterEntity = Create( "monster_barnacle_fantasy", mso4 + Vector(0,170*i4,0), pT_L_C->pev->angles, edict() );
				pEnemyMonster = pMonsterEntity->MyMonsterPointer();
				pEnemyMonster->m_fightmode = 3;
				pMonsterEntity->pev->body = 1;
				}
				pev->health = 16;
				pev->armortype = 13;
			}
	}

	if(pev->health <= 0){
	pev->frags += 1;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

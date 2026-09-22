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
#include	"player.h"
#include	"effects.h"
#include	"shake.h"

extern int gmsgItemPickup;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CEatKey : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	
	CSprite		*m_pEyeGlow;		// Glow around the eyes

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

};

LINK_ENTITY_TO_CLASS( monster_eatkey, CEatKey );
LINK_ENTITY_TO_CLASS( monster_eatkey_orcard, CEatKey );
LINK_ENTITY_TO_CLASS( monster_eatkey_valve, CEatKey );
LINK_ENTITY_TO_CLASS( monster_eatkey_teleport, CEatKey );
LINK_ENTITY_TO_CLASS( monster_eatkey_equip, CEatKey );

TYPEDESCRIPTION	CEatKey::m_SaveData[] = 
{
	DEFINE_FIELD( CEatKey, m_pEyeGlow, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CEatKey, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CEatKey :: Classify ( void )
{
	return	CLASS_PLAYER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEatKey :: SetYawSpeed ( void )
{
	pev->yaw_speed = 0;
}

//=========================================================
// Spawn
//=========================================================
void CEatKey :: Spawn()
{
	Precache();
	
	pev->movetype		= MOVETYPE_TOSS;
	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");

	m_pEyeGlow = CSprite::SpriteCreate( "sprites/glow01.spr", pev->origin, FALSE );

	if(pev->armortype == 1){//ѪԿ��
	pev->body = 9;
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 128, 128, 125, kRenderFxNoDissipation );
	}
	else if(pev->armortype == 2){//��ȫԿ��
	pev->body = 7;
	m_pEyeGlow->SetTransparency( kRenderGlow, 128, 128, 255, 125, kRenderFxNoDissipation );
	}
	else if(pev->armortype == 3){//����
	pev->body = 11;
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 175, kRenderFxNoDissipation );
	}
	else if(pev->armortype == 4){//ͨ��Կ��
	pev->body = 10;
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 155, kRenderFxNoDissipation );
	}
	else if(pev->armortype == 5){//��ɫ�ſ�
	pev->body = 12;
	m_pEyeGlow->SetTransparency( kRenderGlow, 128, 255, 128, 125, kRenderFxNoDissipation );
	}
	else if(pev->armortype == 6 || FClassnameIs(pev, "monster_eatkey_orcard")){//��ȫ�ſ�
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 6;
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 192, 128, 125, kRenderFxNoDissipation );
	}
	else if(FClassnameIs(pev, "monster_eatkey_teleport") || pev->armortype == 7){//������
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 17;
	pev->armortype = 7;
	m_pEyeGlow->SetTransparency( kRenderGlow, 128, 255, 128, 125, kRenderFxNoDissipation );
	}
	else if(FClassnameIs(pev, "monster_eatkey_valve")){//����
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 9;
	pev->armortype = 9;
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 32, 32, 255, kRenderFxNoDissipation );
	}
	else if(FClassnameIs(pev, "monster_eatkey_equip")){//Կ�ס�װ��
		SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
		if(pev->armortype == 10){//����
		pev->body = 11;
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 128, 0, 128, kRenderFxNoDissipation );
		}
		else if(pev->armortype == 11){//����ѥ
		pev->body = 12;
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		}
		else if(pev->armortype == 12){//�����ñ
		pev->body = 13;
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 64, 64, 128, kRenderFxNoDissipation );
		}
		else if(pev->armortype == 13){//����
		pev->body = 14;
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		}
		else if(pev->armortype == 14){//��֮����
		pev->body = 15;
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		}
	}
	else{//��Կ��
	pev->body = 8;
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 128, 125, kRenderFxNoDissipation );
	}

	m_pEyeGlow->SetAttachment( edict(), 1 );
	m_pEyeGlow->pev->scale = 0.35;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->solid			= SOLID_TRIGGER;
	
	m_bloodColor		= DONT_BLEED;
	pev->health			= 10;
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	pev->takedamage = DAMAGE_NO;
	pev->spawnflags |= SF_MONSTER_PRISONER;

	if(pev->armortype == 15){//����ˮ��
		SET_MODEL(ENT(pev), "models/mfsj.mdl");
		UTIL_SetSize(pev, Vector( -8, -8, 0), Vector(8, 8, 64));

		pev->solid			= SOLID_BBOX;

		pev->skin = 1;
		m_pEyeGlow->SetTransparency( kRenderGlow, 255, 0, 255, 128, kRenderFxNoDissipation );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEatKey :: Precache()
{
	PRECACHE_MODEL("sprites/glow01.spr");
	PRECACHE_MODEL("models/mfsj.mdl");
}	

void CEatKey::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pActivator->pev);
	if(pPlayer){

		if(!FVisible( pPlayer ) || m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE){
		return;
		}

		if(pPlayer->HasMenuItem_Full()){
		UTIL_CenterPrintAll( "Items Full!" );
		return;
		}

		if ( pev->message )//Կ������ʵ��
		{
			FireTargets( STRING(pev->message), this, this, USE_TOGGLE, 0 );
		}

		if(pev->armortype == 0){
		pPlayer->MenuItem_add(3);//��ɫԿ��Get
		UTIL_CenterPrintAll( "You get the Gold Key!" );
		pPlayer->m_fGlodenKey += 1;
		}
		else if(pev->armortype == 1){
		pPlayer->MenuItem_add(6);//ȾѪԿ��Get
		UTIL_CenterPrintAll( "You get the Bloodly Key!" );
		pPlayer->m_fBloodlyKey = TRUE;
		}
		else if(pev->armortype == 2){
		pPlayer->MenuItem_add(5);//��ȫԿ��Get
		UTIL_CenterPrintAll( "You get the Security Key!" );
		pPlayer->m_fSecurityKey = TRUE;
		}
		else if(pev->armortype == 3){
		pPlayer->MenuItem_add(2);//���ƿ���Get
		pPlayer->m_fMask = TRUE;
		FireTargets("player_use_mask", this, this, USE_TOGGLE, 0);
		}
		else if(pev->armortype == 4){
		pPlayer->MenuItem_add(4);//ͨ��Կ��Get
		UTIL_CenterPrintAll( "You get the Generic Key!" );
		pPlayer->m_fGenerenKey = TRUE;
		}
		else if(pev->armortype == 5){
		pPlayer->MenuItem_add(7);//��ɫ�ſ�Get
		UTIL_CenterPrintAll( "You get the Green Card!" );
		pPlayer->m_fGreenCard = TRUE;
		}
		else if(pev->armortype == 6){
		pPlayer->MenuItem_add(8);//��ȫ�ſ�Get
		UTIL_CenterPrintAll( "You get the Security Card!" );
		pPlayer->m_fSecurityCard = TRUE;
		}
		else if(pev->armortype == 7){
		pPlayer->MenuItem_add(22);//������Get
		UTIL_CenterPrintAll( "You get the Teleport Book!" );
		}
		else if(pev->armortype == 9){
		pPlayer->MenuItem_add(14);//����Get
		UTIL_CenterPrintAll( "You get the Valve!" );
		pPlayer->m_fValve = TRUE;
		}
		else if(pev->armortype == 10){
		pPlayer->MenuItem_add(15);//����Get
		//UTIL_CenterPrintAll( "You get the Red Heart!" );
		pPlayer->m_fequip1 = TRUE;
		if(pPlayer->pev->max_health < 400){
		pPlayer->pev->max_health = 400;
		}
		pPlayer->pev->health = pPlayer->pev->max_health;
		}
		else if(pev->armortype == 11){
		pPlayer->MenuItem_add(16);//����ѥGet
		//UTIL_CenterPrintAll( "You get the Equip-2!" );
		pPlayer->m_fequip2 = TRUE;
		g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "mario", "1" );
		}
		else if(pev->armortype == 12){
		//pPlayer->MenuItem_add(17);//�����ñGet
		//UTIL_CenterPrintAll( "You get the Equip-3!" );
		//pPlayer->m_fequip3 = TRUE;
		//g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "mario", "1" );
		}
		else if(pev->armortype == 13){
		pPlayer->MenuItem_add(18);//����Get
		//UTIL_CenterPrintAll( "You get the Equip-4!" );
		pPlayer->m_fequip4 = TRUE;
		pPlayer->m_air_oxyan_max = 2500;
		pPlayer->m_air_oxyan = pPlayer->m_air_oxyan_max;
		}
		else if(pev->armortype == 14){
		pPlayer->MenuItem_add(19);//��֮����Get
		//UTIL_CenterPrintAll( "You get the Equip-5!" );
		pPlayer->m_fequip5 = TRUE;
		pPlayer->m_skill_darkhide = 7;
		}
		else if(pev->armortype == 15){
		pPlayer->MenuItem_add(20);//����ˮ��Get
		UTIL_CenterPrintAll( "Attack+100%%!" );
		pPlayer->m_fequip6 = TRUE;
		UTIL_ScreenFade( pPlayer, Vector(255,128,255), 1, 1, 192, FFADE_IN );
		EMIT_SOUND(ENT(pev), CHAN_NETWORKVOICE_BASE, "debris/beamstart10.wav", 1, 0.7);	
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
		WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();

		if(m_pEyeGlow){
		UTIL_Remove( m_pEyeGlow );
		m_pEyeGlow = NULL;
		}

		UTIL_Remove( this );
	}

}


void CEatKey :: Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
		m_die = 1;
		pev->model = iStringNull;// make invisible
		SetThink( &CEatKey::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;

		if(m_pEyeGlow){
		UTIL_Remove( m_pEyeGlow );
		m_pEyeGlow = NULL;
		}

		pev->takedamage = DAMAGE_NO;
	}
	return;
}
#ifndef HORNETGUN_H
#define HORNETGUN_H


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"
#include "decals.h"
#include "BMOD_messaging.h"

extern cvar_t bm_hornet_mod;

#define 	HGUN_MAX_BEAMS 		6
#define		HGUN_CHARGE_TIME	0.8f / HGUN_MAX_BEAMS
#define		HGUN_ZAP_TIME		1.0f

class CHgun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void Recharge( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;

	float m_flRechargeTime;
	
	int m_iFirePhase;// don't save me.
	float m_fNextPhaseTime;
	CBeam *m_pBeam[HGUN_MAX_BEAMS];
 	void ClearBeams( );
    void ArmBeam( Vector color );
    void ZapBeam( void );
    void BeamGlow( void );
	void FreezeRay( void );
	void ZapGun (void );
	void MultiZapGun (void );
	void SquidSpit ( void );
	void LaunchSnark ( void );
	void OldPrimaryAttack( void );
	void OldSecondaryAttack( void );
	
	int m_iFireMode;
	float m_fModeSwitchDelay;
    int m_iBeams;
	short m_sGlowSpr;

	int m_iMaxammo;
	short                        iZapBeamSpr;
private:
	unsigned short m_usHornetFire;

};


#endif

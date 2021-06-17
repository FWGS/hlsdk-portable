#ifndef EGON_H
#define EGON_H

#include "weapons.h"

#define BUBBLE_SOUND_OFF			"debris/flesh5.wav"
#define BUBBLE_SOUND_RUN			"debris/flesh5.wav"
#define BUBBLE_SOUND_STARTUP		"debris/flesh6.wav"
#define	EGON_PRIMARY_VOLUME		450

extern cvar_t bm_gluon_mod;

class CEgon : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );

	void CreateEffect ( void );
	void DestroyEffect ( void );

	void EndAttack( void );
	void Attack( void );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	BOOL HasAmmo( void );

	void UseAmmo( int count );
	
	enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	unsigned short m_usEgonStop;

	// BMOD Begin - bubblegun
	void BubbleAttack( void );
	void FireBubbles( void );
	float m_bubbletime;
	void FireHeal( void );
	static int g_fireAnims1[];
	static int g_fireAnims2[];
	float				m_healAmmoUsed;
	float				m_healAmmoUseTime;
	// BMOD End - bubblegun

private:
	float				m_shootTime;
	EGON_FIREMODE		m_fireMode;
	float				m_shakeTime;
	BOOL				m_deployed;

	unsigned short m_usEgonFire;
};

#endif

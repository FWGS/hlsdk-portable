#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "shake.h"

/*void GetNearestEarthTarget( void )
{
	CBaseEntity *pEarth = NULL;
	CBaseEntity *pNearest = NULL;
	float dist, closest;

	closest = 1024;

	while ((pEarth = UTIL_FindEntityInSphere( pEarth, pev->origin, 1024 )) != NULL)
	{
		if ( FClassnameIs( pEarth->pev, "info_displacer_earth_target" ) )
		{
			dist = (pev->origin - pEarth->pev->origin).Length();
			if ( dist < closest )
			{
				closest = dist;
				pNearest = pEarth;
			}
		}
	}

	if ( !pNearest )
	{
		ALERT( at_console, "Can't find a nearby info_displacer_earth_target !!!\n" );
		return;
	} else
        pEarthTarget = pNearest;
}*/

enum displacer_e {
	DISPLACER_IDLE1 = 0,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER1
};

LINK_ENTITY_TO_CLASS( weapon_displacer, CDisplacer );

LINK_ENTITY_TO_CLASS( displacer_teleporter, CTeleBall);


TYPEDESCRIPTION	CTeleBall::m_SaveData[] = 
{
	DEFINE_FIELD( CTeleBall, m_vecIdeal, FIELD_VECTOR ),
	DEFINE_FIELD( CTeleBall, m_hTouch, FIELD_EHANDLE ),
};


IMPLEMENT_SAVERESTORE( CTeleBall, CBaseMonster );

void CTeleBall :: Spawn( void )
{
	Precache( );
	
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
    
	SET_MODEL(edict(), "sprites/exit1.spr");
	
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->scale = 1.0;
	
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	
    UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );
	
	SetThink( TeleportThink );
    SetTouch( TeleportTouch );
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/displacer_fire.wav", 1, ATTN_NORM);
	
	int i;
	for (i=1;i<5;i++)
	{
		pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr",200);
		pBeam[i]->pev->origin = pev->origin;
		
		pBeam[i]->SetColor( 211,255,81 );
		pBeam[i]->SetNoise( 70 );
		pBeam[i]->SetBrightness( 120 );//was 150
		pBeam[i]->SetWidth( 45 );
		pBeam[i]->SetScrollRate( 35 );
		pBeam[i]->SetThink( SUB_Remove );
		pBeam[i]->pev->nextthink = 0; //was gpGlobals->time + 1
	}
	
	pev->nextthink = 0.1;
}

void CTeleBall:: Precache( void )
{
	ring_sprite = PRECACHE_MODEL("sprites/disp_ring.spr");
	PRECACHE_MODEL("sprites/exit1.spr");
	PRECACHE_MODEL( "sprites/lgtning.spr" );

	PRECACHE_SOUND("weapons/displacer_teleport.wav");
    PRECACHE_SOUND("weapons/displacer_teleport_player.wav");
	PRECACHE_SOUND("weapons/displacer_fire.wav");
}

void CTeleBall:: PlayEffect( Vector Origin, CBaseEntity *pEntity )
{
	CBeam *pBeam[15];
	TraceResult tr;
    Vector vecDest;

	EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart7.wav", 1, ATTN_NORM );
    UTIL_ScreenFade( pEntity, Vector(0,255,0), 0.5, 0.25, 255, FFADE_IN ); 

	int i;
	for (i=1;i<15;i++)
	{
	vecDest = 500 * (Vector(RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000)).Normalize());
	UTIL_TraceLine( Origin, Origin + vecDest, ignore_monsters, NULL, &tr);
	if (tr.flFraction != 1.0)
		{
			// we hit something.
		    pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr",200);
			pBeam[i]->pev->origin = Origin;
			pBeam[i]->PointsInit( Origin, tr.vecEndPos );
			pBeam[i]->SetColor( 0, 255, 0 ); //Blue-Shift style
			pBeam[i]->SetNoise( 65 );
			pBeam[i]->SetBrightness( 150 );
			pBeam[i]->SetWidth( 18 );
			pBeam[i]->SetScrollRate( 35 );
			pBeam[i]->SetThink( SUB_Remove );
			pBeam[i]->pev->nextthink = gpGlobals->time + 1; //was 0.1
		}
	}
  	EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart2.wav", 1, ATTN_NORM );

	CSprite *pSpr = CSprite::SpriteCreate( "sprites/Fexplo1.spr", Origin, TRUE );
    pSpr->AnimateAndDie( 10 );
	pSpr->SetTransparency(kRenderGlow,  77, 210, 130,  255, kRenderFxNoDissipation);

	pSpr = CSprite::SpriteCreate( "sprites/XFlare1.spr", Origin, TRUE );
    pSpr->AnimateAndDie( 10 );
	pSpr->SetTransparency(kRenderGlow,  184, 250, 214,  255, kRenderFxNoDissipation);
}

void CTeleBall:: TeleportThink( void  )
{
    TraceResult tr;
    Vector vecDest;

	int i;
	for (i=1;i<5;i++)
	{
	vecDest = 500 * (Vector(RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000)).Normalize());
	UTIL_TraceLine( pev->origin, pev->origin + vecDest, ignore_monsters, NULL, &tr);
	if (tr.flFraction != 1.0)
		{
			// we hit something.
			pBeam[i]->pev->origin = pev->origin;
			pBeam[i]->PointsInit( pev->origin, tr.vecEndPos );
		}
	}
		
    pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = (int)(pev->frame + 1) % 20; //comment this to disable sprite animation
}

void CTeleBall::TeleportTouch( CBaseEntity *pOther )
{
	if (pOther->Classify() != CLASS_PLAYER)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/displacer_teleport.wav", 1, ATTN_NORM);
	} 
	else EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/displacer_teleport_player.wav", 1, ATTN_NORM);
	
	//if we shoot player - teleport him before energy ball killed him
    if (pOther->IsPlayer()) //if we hit player
	{
	#ifdef CLIENT_DLL
		if (bIsMultiplayer())
	#else
		if (g_pGameRules->IsMultiplayer() ) //and if we are in MP game
	#endif
		{
			CBaseEntity *pSpot = NULL;
			CBaseEntity *pEntity = NULL; 
			int count = 0;   
			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_player_deathmatch")) != NULL) 
			{    
				count++; 
			}
			if (count>0) 
			{
				for ( int i = RANDOM_LONG(1,count); i > 0; i-- ) //Randomize teleport spot
					pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
				Vector tmp = pSpot->pev->origin;
				//tmp.z -= pOther->pev->mins.z;
				//tmp.z++;
				UTIL_SetOrigin( pOther->pev, tmp ); //teleport player
				PlayEffect( tmp, pOther);
			}
		}
	}
	SetTouch( NULL );
	pev->velocity = Vector( 0, 0, 0 ); //stop energy ball, otherwise it will "slide"
	
	SetThink(TeleportKill);
	pev->nextthink = gpGlobals->time+0.2; //allows energy ball to stay alive after it touched something
}

void CTeleBall::TeleportKill( void )
{
	int flAdjustedDamage;
	float flDist;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );// coord coord coord (center position)
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( pev->origin.x );// coord coord coord (axis and radius)
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16 + (300 / 2)/ .2);
		WRITE_SHORT( ring_sprite ); // short (sprite index)
		WRITE_BYTE( 0 );  // byte (starting frame)
		WRITE_BYTE( 0 );  // byte (frame rate in 0.1's)
		WRITE_BYTE( 4 );  // byte (life in 0.1's)
		WRITE_BYTE( 16 ); // byte (line width in 0.1's)
		WRITE_BYTE( 0 );  // byte (noise amplitude in 0.01's)
		WRITE_BYTE( 188 );// byte,byte,byte (color)
		WRITE_BYTE( 220 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( pev->renderamt );// byte (brightness)
		WRITE_BYTE( 0 );	         // byte (scroll speed in 0.1's)
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 10 );		// radius * 0.1
		WRITE_BYTE( 216 );		// r
		WRITE_BYTE( 239 );		// g
		WRITE_BYTE( 80 );		// b
		WRITE_BYTE( 5 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );
    //there was SetTouch( NULL );	
	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 300 )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
				// houndeyes do FULL damage if the ent in question is visible. Half damage otherwise.
				// This means that you must get out of the houndeye's attack range entirely to avoid damage.
				// Calculate full damage first
				flAdjustedDamage = 250;

				flDist = (pEntity->Center() - pev->origin).Length();

				flAdjustedDamage -= ( flDist / 300 ) * flAdjustedDamage;

				if ( !FVisible( pEntity ) )
				{
					if ( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) ) 
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0;
					}
				}
				if (flAdjustedDamage > 0 )
				{
					//pev, pev
					entvars_t	*pevOwner;
					pevOwner = VARS( pev->owner );

					pEntity->TakeDamage ( pev, pevOwner, flAdjustedDamage, DMG_SONIC | DMG_ALWAYSGIB );
				}
		}
	}
	
	for (int i=1;i<5;i++) UTIL_Remove( pBeam[i] ); //remove beams
    SetThink ( SUB_Remove );
	pev->nextthink = 0.2;
}

void CDisplacer::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DISPLACER;
	SET_MODEL(ENT(pev), "models/w_displacer.mdl");

	m_iDefaultAmmo = DISPLACER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CDisplacer::Precache( void )
{
	PRECACHE_MODEL("models/w_displacer.mdl");
	PRECACHE_MODEL("models/v_displacer.mdl");
	PRECACHE_MODEL("models/p_displacer.mdl");

	PRECACHE_SOUND("weapons/displacer_fire.wav");
    PRECACHE_SOUND("weapons/displacer_impact.wav");
    PRECACHE_SOUND("weapons/displacer_self.wav");
    PRECACHE_SOUND("weapons/displacer_spin.wav");
    PRECACHE_SOUND("weapons/displacer_spin2.wav");
    PRECACHE_SOUND("weapons/displacer_start.wav");
    PRECACHE_SOUND("weapons/displacer_teleport.wav");
    PRECACHE_SOUND("weapons/displacer_teleport_player.wav");

	PRECACHE_MODEL("sprites/Fexplo1.spr");
	PRECACHE_MODEL("sprites/XFlare1.spr");
	PRECACHE_SOUND ("debris/beamstart7.wav");
	UTIL_PrecacheOther( "displacer_teleporter" );
}

int CDisplacer::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CDisplacer::Holster( int skiplocal)
{
	SendWeaponAnim( DISPLACER_HOLSTER1 );
}

BOOL CDisplacer::Deploy( )
{
	return DefaultDeploy( "models/v_displacer.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "displacer" );
}

int CDisplacer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DISPLACER_MAX_CLIP;
	p->iSlot = 4;     // slot 5, position 5
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_DISPLACER;
	p->iFlags = 0;
	p->iWeight = DISPLACER_WEIGHT;

	return 1;
}

BOOL CDisplacer::PlayEmptySound( void )
{
	//button11.wav
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

void CDisplacer::SpinupThink( void )
{
	SendWeaponAnim( DISPLACER_SPINUP );
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/displacer_spin.wav", 1, ATTN_NORM);

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		ResetEmptySound( );
		//m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	SetThink(FireThink); 
	pev->nextthink=gpGlobals->time + 1.0;
	//m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 3.0;
}

void CDisplacer::FireThink( void )
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 20 )
	{
		PlayEmptySound( );
		SendWeaponAnim( DISPLACER_IDLE1 );
		//m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

    SendWeaponAnim( DISPLACER_FIRE );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle  );
    Vector vecSrc = m_pPlayer->GetGunPosition( );
	
	CTeleBall *pEntity = (CTeleBall *)Create( "displacer_teleporter", vecSrc, m_pPlayer->pev->angles, m_pPlayer->edict() );
	pEntity->pev->owner = m_pPlayer->edict();
	pEntity->pev->velocity = gpGlobals->v_forward * 500;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 20;

	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

void CDisplacer::PrimaryAttack( void )
{
	SetThink(SpinupThink); 
	pev->nextthink=gpGlobals->time + 0.1;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.5;
}

void CDisplacer::SecondaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 60)
	{
		//if is multiplayer game
	#ifdef CLIENT_DLL
		if (bIsMultiplayer())
	#else
		if (g_pGameRules->IsMultiplayer() ) 
	#endif
		{
			CBaseEntity *pSpot = NULL;
			CBaseEntity *pEntity = NULL; 
			int count = 0;   
			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_player_deathmatch")) != NULL) 
			{    
				count++; 
			}
			if (count>0) 
			{
				for ( int i = RANDOM_LONG(1,count); i > 0; i-- ) //Randomize teleport spot
					pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
				
				//spawn CTeleBall here
				CTeleBall *pEntity = (CTeleBall *)Create( "displacer_teleporter", pev->origin, m_pPlayer->pev->angles, m_pPlayer->edict() );
				pEntity->pev->owner = m_pPlayer->edict();
				pEntity->pev->velocity = -gpGlobals->v_up * 150;
				//
				UTIL_SetOrigin( m_pPlayer->pev, pSpot->pev->origin );
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 60;
				
				PlayEffect( pSpot->pev->origin ); 
			}
		}
		else 
		//if is singleplayer game
		{
			CDisplacerTarget *pSpot = NULL;
			
			while ( (pSpot = (CDisplacerTarget*)UTIL_FindEntityByClassname( pSpot, "info_displacer_xen_target" )) && (!FNullEnt(pSpot->edict())) )
			{
				if ( pSpot->m_iPlayerIndex == m_pPlayer->m_iDecayId )
					break;
			}
			
			if ( (pSpot != NULL) && (pSpot->m_iPlayerIndex == m_pPlayer->m_iDecayId) )
			{
				Vector tmp = pSpot->pev->origin;
				tmp.z -= m_pPlayer->pev->mins.z;
				tmp.z++;	
				
				//spawn CTeleBall here
				CTeleBall *pEntity = (CTeleBall *)Create( "displacer_teleporter", pev->origin, m_pPlayer->pev->angles, m_pPlayer->edict() );
				pEntity->pev->owner = m_pPlayer->edict();
				pEntity->pev->velocity = -gpGlobals->v_up * 150;
				//
				UTIL_SetOrigin( m_pPlayer->pev, tmp);
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 60;
				PlayEffect( tmp ); 

				if ( !FStringNull( pSpot->pev->target ) )
					FireTargets( STRING(pSpot->pev->target), this, this, USE_TOGGLE, 0.0 );

			} else
				PlayEmptySound( );
		}
		m_flNextSecondaryAttack = gpGlobals->time + 4.0;
	}
	else
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}
}

void CDisplacer::PlayEffect( Vector Origin )
{
	CBeam *pBeam[15];
	TraceResult tr;
    Vector vecDest;

	EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart7.wav", 1, ATTN_NORM );
    UTIL_ScreenFade( m_pPlayer, Vector(0,255,0), 0.5, 0.25, 255, FFADE_IN ); 

	int i;
	for (i=1;i<15;i++)
	{
	vecDest = 500 * (Vector(RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000), RANDOM_FLOAT(-1000,1000)).Normalize());
	UTIL_TraceLine( Origin, Origin + vecDest, ignore_monsters, NULL, &tr);
	if (tr.flFraction != 1.0)
		{
			// we hit something.
		    pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr",200);
			pBeam[i]->pev->origin = Origin;
			pBeam[i]->PointsInit( Origin, tr.vecEndPos );
			pBeam[i]->SetColor( 0, 255, 0 ); //Blue-Shift style
			//197 243 169 //c1a1b
			pBeam[i]->SetNoise( 65 );
			pBeam[i]->SetBrightness( 150 );
			pBeam[i]->SetWidth( 18 );
			pBeam[i]->SetScrollRate( 35 );
			pBeam[i]->SetThink( SUB_Remove );
			pBeam[i]->pev->nextthink = gpGlobals->time + 1; //was 0.1
		}
	}
  	EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart2.wav", 1, ATTN_NORM );

	CSprite *pSpr = CSprite::SpriteCreate( "sprites/Fexplo1.spr", Origin, TRUE );
	pSpr->AnimateAndDie( 10 );
	pSpr->SetTransparency(kRenderGlow,  77, 210, 130,  255, kRenderFxNoDissipation);

	pSpr = CSprite::SpriteCreate( "sprites/XFlare1.spr", Origin, TRUE );
	pSpr->AnimateAndDie( 10 );
	pSpr->SetTransparency(kRenderGlow,  184, 250, 214,  255, kRenderFxNoDissipation);
//	pev->nextthink = 0; //was 0
}

void CDisplacer::Reload( void )
{

}

void CDisplacer::WeaponIdle( void )
{
	ResetEmptySound( );
	
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;
	
	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.5)
	{
		iAnim = DISPLACER_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;//UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
	else
	{
		iAnim = DISPLACER_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}
	
	SendWeaponAnim( iAnim );
}
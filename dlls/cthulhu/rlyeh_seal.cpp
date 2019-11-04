
#include "rlyeh_seal.h"


TYPEDESCRIPTION CFuncRlyehLock::m_SaveData[] =
{
	DEFINE_FIELD( CFuncRlyehLock, m_sMaster, FIELD_STRING ),
	DEFINE_FIELD( CFuncRlyehLock, m_sTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncRlyehLock, CBaseEntity );

LINK_ENTITY_TO_CLASS( func_rlyehlock, CFuncRlyehLock );

void CFuncRlyehLock :: Precache( void )
{
	char* szSoundFile = (char*) STRING(pev->message);

	if ( !FStringNull( pev->message ) && strlen( szSoundFile ) > 1 )
	{
		if (*szSoundFile != '!')
			PRECACHE_SOUND(szSoundFile);
	}
	CBaseEntity::Precache();
}

void CFuncRlyehLock :: Spawn( void )
{
	pev->angles		= g_vecZero;
	pev->movetype	= MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid		= SOLID_BSP;
	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	// If it can't move/go away, it's really part of the world
	if (!m_pMoveWith) //LRC
		pev->flags |= FL_WORLDBRUSH;

	Precache( );
}

BOOL CFuncRlyehLock :: IsLockedByMaster( void ) 
{
	return !UTIL_IsMasterTriggered(m_sMaster, NULL); 
}

void CFuncRlyehLock :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType, (int)(pev->frame)) )
		pev->frame = 1 - pev->frame;
}

void CFuncRlyehLock::KeyValue( KeyValueData *pkvd )
{
	pkvd->fHandled = TRUE;

	if ( FStrEq(pkvd->szKeyName, "master") )
		m_sMaster = ALLOC_STRING( pkvd->szValue );
	if ( FStrEq(pkvd->szKeyName, "targetonlock") )
		m_sTarget = ALLOC_STRING( pkvd->szValue );
	else
		CBaseEntity::KeyValue( pkvd );
}

void CFuncRlyehLock::FireTarget( )
{
	CBaseEntity* pTarget = NULL;
	pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(m_sTarget));

	if (FNullEnt(pTarget))
		return;

	pTarget->Use( NULL, this, USE_TOGGLE, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////////

enum rlyeh_seal_e {
	RLYEH_SEAL_DRAW = 0,
	RLYEH_SEAL_IDLE1,
	RLYEH_SEAL_IDLE2,
	RLYEH_SEAL_IDLE3,
	RLYEH_SEAL_CAST,
	RLYEH_SEAL_HOLSTER,
	RLYEH_SEAL_WORLD,
	RLYEH_SEAL_GROUND
};


LINK_ENTITY_TO_CLASS( monster_rlyeh_seal, CRlyehSealed );

//TYPEDESCRIPTION	CRlyehSealed::m_SaveData[] = 
//{
//};

//IMPLEMENT_SAVERESTORE( CRlyehSealed, CBaseMonster );


void CRlyehSealed :: Spawn( void )
{
	Precache( );

	m_afCapability		= 0;

	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/v_rlyeh_seal.mdl");
	pev->frame = 0;
	pev->body = 3;
	//pev->sequence = RLYEH_SEAL_IDLE1;
	pev->sequence = RLYEH_SEAL_WORLD;
	ResetSequenceInfo( );
	//pev->framerate = 6;
	
	UTIL_SetSize(pev, Vector( -4, -4, -4), Vector(4, 4, 4));
	UTIL_SetOrigin( this, pev->origin );

	pev->solid = SOLID_BBOX;
	DontThink ();

	pev->takedamage = DAMAGE_YES;
	pev->health = 5; // don't let die normally

	//MonsterInit();
	SetBits (pev->flags, FL_MONSTER);

	pev->solid = SOLID_BBOX;
	m_bloodColor = DONT_BLEED;
}

void CRlyehSealed :: Precache( void )
{
	PRECACHE_MODEL("models/v_rlyeh_seal.mdl");
}

int CRlyehSealed :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (flDamage < pev->health)
	{
		SetThink( SUB_Remove );
		SetNextThink( 0.1 );
		return FALSE;
	}
	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CRlyehSealed :: RlyehThink ( void )
{
	SetNextThink(1);
}

void CRlyehSealed :: Killed( entvars_t *pevAttacker, int iGib )
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 10 ); 
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	UTIL_Remove(this);
}

int	CRlyehSealed :: Classify ( void )
{
	return	CLASS_ALIEN_PASSIVE;
}

//////////////////////////////////////////////////////////////////////////////////

LINK_ENTITY_TO_CLASS( weapon_rlyeh_seal, CRlyehSeal );


void CRlyehSeal::Spawn( )
{
	Precache( );
	m_iId = WEAPON_RLYEH_SEAL;
	SET_MODEL(ENT(pev), "models/v_rlyeh_seal.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = RLYEH_SEAL_GROUND;
	//ResetSequenceInfo( );
	pev->framerate = 0;

	FallInit();// get ready to fall down

	m_iDefaultAmmo = 1;

	if ( !g_pGameRules->IsDeathmatch() )
	{
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 28) ); 
	}
}

void CRlyehSeal::Precache( void )
{
	PRECACHE_MODEL ("models/v_rlyeh_seal.mdl");
	UTIL_PrecacheOther( "monster_rlyeh_seal" );
	// sound for trying to put it somewhere other than a R'Lyeh Lock
	PRECACHE_SOUND("voiceover/rs_notthere.wav");
}

int CRlyehSeal::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "R'lyeh Seal";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_RLYEH_SEAL;
	p->iWeight = 1;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CRlyehSeal::Deploy( )
{
	pev->body = 0;
	return DefaultDeploy( "models/v_rlyeh_seal.mdl", "", RLYEH_SEAL_DRAW, "rlyehseal" );
}


void CRlyehSeal::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// out of mines
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_RLYEH_SEAL);
		SetThink( DestroyItem );
		SetNextThink( 0.1 );
	}

	SendWeaponAnim( RLYEH_SEAL_HOLSTER );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

int CRlyehSeal::AddToPlayer( CBasePlayer *pPlayer )
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


void CRlyehSeal::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if (tr.flFraction < 1.0)
	{
		// ALERT( at_console, "hit %f\n", tr.flFraction );

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if (pEntity && !(pEntity->pev->flags & FL_CONVEYOR))
		{
			// is this a R'lyeh Lock?
			if (strcmp(STRING(pEntity->pev->classname),"func_rlyehlock"))
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "voiceover/rs_notthere.wav", 1, ATTN_NORM, 0, 100);
			}
			else
			{
				CFuncRlyehLock* pLock = (CFuncRlyehLock*)pEntity;
				if (pLock->IsLockedByMaster())
				{
					// play locked sound if there is one...
					char* szSoundFile = (char*) STRING(pLock->pev->message);

					if ( !FStringNull( pLock->pev->message ) && strlen( szSoundFile ) > 1 )
					{
						EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, szSoundFile, 1, ATTN_NORM, 0, 100);
					}
				}
				else
				{
					Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

					edict_t	*pentity;
					entvars_t		*pevCreate;

				//	ALERT(at_console,"Making Monster NOW\n");

					pentity = CREATE_NAMED_ENTITY( MAKE_STRING("monster_rlyeh_seal") );

					pevCreate = VARS( pentity );
					pevCreate->origin = tr.vecEndPos + tr.vecPlaneNormal * 8;
					pevCreate->angles = angles;

					DispatchSpawn( ENT( pevCreate ) );
					pevCreate->owner = edict();

					//LRC - custom monster behaviour
					CBaseEntity *pEnt = CBaseEntity::Instance( pevCreate );

			
			
					//CBaseEntity *pEnt = CBaseEntity::Create( "monster_rlyeh_seal", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );
					//CBaseMonster *pNewMonster = pEnt->MyMonsterPointer( );
					//pEnt->pev->spawnflags |= 1;

					CRlyehSealed *pRlyehSealed = (CRlyehSealed *)pEnt;
					pRlyehSealed->pev->origin = (pLock->pev->absmin + pLock->pev->absmax)/2;
					pRlyehSealed->pev->origin.z = pLock->pev->absmax.z + 8;

					pLock->FireTarget();

					m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

					// player "shoot" animation
					m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

					if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
					{
						SendWeaponAnim( RLYEH_SEAL_DRAW );
					}
					else
					{
						// no more mines! 
						RetireWeapon();
						return;
					}
				}
			}
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	else
	{

	}

	m_flNextPrimaryAttack = gpGlobals->time + 0.3;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}

void CRlyehSeal::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim( RLYEH_SEAL_DRAW );
	}
	else
	{
		RetireWeapon(); 
		return;
	}

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.25)
	{
		iAnim = RLYEH_SEAL_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = RLYEH_SEAL_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 30.0;
	}
	else
	{
		iAnim = RLYEH_SEAL_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + 100.0 / 30.0;
	}

	SendWeaponAnim( iAnim );
	
}




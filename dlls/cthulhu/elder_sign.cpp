
#include "elder_sign.h"

enum elder_sign_e {
	ELDER_SIGN_DRAW = 0,
	ELDER_SIGN_IDLE1,
	ELDER_SIGN_IDLE2,
	ELDER_SIGN_IDLE3,
	ELDER_SIGN_CAST,
	ELDER_SIGN_HOLSTER,
	ELDER_SIGN_WORLD,
	ELDER_SIGN_GROUND
};


LINK_ENTITY_TO_CLASS( monster_elder_sign, CElderSignArea );

//TYPEDESCRIPTION	CElderSignArea::m_SaveData[] = 
//{
//};

//IMPLEMENT_SAVERESTORE( CElderSignArea, CBaseMonster );


void CElderSignArea :: Spawn( void )
{
	Precache( );

	m_afCapability		= bits_CAP_RANGE_ATTACK1;

	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/v_elder_sign.mdl");
	pev->frame = 0;
	pev->body = 3;
	//pev->sequence = ELDER_SIGN_IDLE1;
	pev->sequence = ELDER_SIGN_WORLD;
	ResetSequenceInfo( );
	pev->framerate = 0;
	
	UTIL_SetSize(pev, Vector( -8, 0, -8), Vector(8, 4, 8));
	UTIL_SetOrigin( this, pev->origin );

	//pev->solid = SOLID_BBOX;
	SetThink ( ElderSignThink );
	SetNextThink( 0.2 );

	pev->takedamage = DAMAGE_YES;
	pev->health = 5; // don't let die normally

	//MonsterInit();
	SetBits (pev->flags, FL_MONSTER);

	// make solid
	//pev->solid = SOLID_BBOX;
	UTIL_SetOrigin( this, pev->origin );
	m_bloodColor = DONT_BLEED;
}

void CElderSignArea :: Precache( void )
{
	PRECACHE_MODEL("models/v_elder_sign.mdl");
}

int CElderSignArea :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (flDamage < pev->health)
	{
		SetThink( SUB_Remove );
		SetNextThink( 0.1 );
		return FALSE;
	}
	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

BOOL CElderSignArea :: CheckRangeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

void CElderSignArea :: ElderSignThink ( void )
{
	int iMonsters;
	
	// wait until we are free of monsters (incl. players) in our box and then become solid
	if (pev->solid == SOLID_NOT)
	{
		iMonsters = UTIL_EntitiesInBox(mpEntInSphere, MAX_MONSTER, pev->origin - Vector(8,2,8), pev->origin + Vector(8,2,8), (FL_CLIENT|FL_MONSTER));

		// are we (the elder sign) the only monster in the area
		if (iMonsters <= 1)
		{
			pev->solid = SOLID_BBOX;
		}
	}

	iMonsters = UTIL_MonstersInSphere(mpEntInSphere, MAX_MONSTER, pev->origin, 96);
		
	for (int i = 0; i < iMonsters; i++)
	{
		if (!(mpEntInSphere[i]->pev->flags & FL_MONSTER)) continue;

		Repel((CBaseMonster*)mpEntInSphere[i]);
	}

	SetNextThink( 0.1 );
}

void CElderSignArea :: Killed( entvars_t *pevAttacker, int iGib )
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 5  ); 
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	UTIL_Remove(this);
}

int	CElderSignArea :: Classify ( void )
{
	return	CLASS_ALIEN_PASSIVE;
}

void CElderSignArea::Repel( CBaseMonster* pEnt )
{
	int iClass = pEnt->Classify();

	if (iClass != CLASS_ALIEN_MILITARY 
		&& iClass != CLASS_ALIEN_MONSTER 
		&& iClass != CLASS_ALIEN_PREY 
		&& iClass != CLASS_ALIEN_PREDATOR)
		return;

	entvars_t* pevEnt = pEnt->pev;

	if ( !FBitSet ( pevEnt->flags , FL_MONSTER ) ) 
	{// touched by a non-monster.
		return;
	}

	pevEnt->origin.z += 1;

	if ( FBitSet ( pevEnt->flags, FL_ONGROUND ) ) 
	{// clear the onground so physics don't bitch
		pevEnt->flags &= ~FL_ONGROUND;
	}

	// flying monsters do not work the same way, so we panic them instead.
	if (pEnt->pev->movetype == MOVETYPE_FLY)
	{
		CBaseEntity* pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		pEnt->Panic(pPlayer->pev);
		return;
	}

	// calculate the opposite direction, and push in that direction
	Vector vecDirToEnemy = ( pevEnt->origin - pev->origin );
	vecDirToEnemy.z = 0;
	//Vector angDir = UTIL_VecToAngles( vecDirToEnemy );
	vecDirToEnemy = vecDirToEnemy.Normalize();
	
	pevEnt->velocity = vecDirToEnemy * 256;
	pevEnt->velocity.z += 128;
}

//////////////////////////////////////////////////////////////////////////////////

LINK_ENTITY_TO_CLASS( weapon_eldersign, CElderSign );


void CElderSign::Spawn( )
{
	Precache( );
	m_iId = WEAPON_ELDER_SIGN;
	SET_MODEL(ENT(pev), "models/v_elder_sign.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = ELDER_SIGN_GROUND;
	//ResetSequenceInfo( );
	pev->framerate = 0;

	FallInit();// get ready to fall down

	m_iDefaultAmmo = ELDER_SIGN_DEFAULT_GIVE;

	if ( !g_pGameRules->IsDeathmatch() )
	{
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 28) ); 
	}
}

void CElderSign::Precache( void )
{
	PRECACHE_MODEL ("models/v_elder_sign.mdl");
//	PRECACHE_MODEL ("models/p_elder_sign.mdl");
	UTIL_PrecacheOther( "monster_elder_sign" );
}

int CElderSign::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Elder Sign";
	p->iMaxAmmo1 = ELDER_SIGN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_ELDER_SIGN;
	p->iWeight = ELDER_SIGN_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CElderSign::Deploy( )
{
	pev->body = 0;
	return DefaultDeploy( "models/v_elder_sign.mdl", "", ELDER_SIGN_DRAW, "eldersign" );
}


void CElderSign::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// out of mines
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_ELDER_SIGN);
		SetThink( DestroyItem );
		SetNextThink( 0.1 );
	}

	SendWeaponAnim( ELDER_SIGN_HOLSTER );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

int CElderSign::AddToPlayer( CBasePlayer *pPlayer )
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


void CElderSign::PrimaryAttack( void )
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
			Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

			edict_t	*pentity;
			entvars_t		*pevCreate;

		//	ALERT(at_console,"Making Monster NOW\n");

			pentity = CREATE_NAMED_ENTITY( MAKE_STRING("monster_elder_sign") );

			pevCreate = VARS( pentity );
			//pevCreate->origin = tr.vecEndPos + tr.vecPlaneNormal * 8;
			pevCreate->origin = tr.vecEndPos + tr.vecPlaneNormal * 0.1;
			pevCreate->angles = angles;

			DispatchSpawn( ENT( pevCreate ) );
			pevCreate->owner = edict();

			//LRC - custom monster behaviour
			CBaseEntity *pEnt = CBaseEntity::Instance( pevCreate );




			//CBaseEntity *pEnt = CBaseEntity::Create( "monster_elder_sign", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );
			//CBaseMonster *pNewMonster = pEnt->MyMonsterPointer( );
			//pEnt->pev->spawnflags |= 1;

			//CElderSignArea *pElderSignArea = (CElderSignArea *)pEnt;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
			{
				SendWeaponAnim( ELDER_SIGN_DRAW );
			}
			else
			{
				// no more mines! 
				RetireWeapon();
				return;
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

void CElderSign::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim( ELDER_SIGN_DRAW );
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
		iAnim = ELDER_SIGN_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = ELDER_SIGN_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 30.0;
	}
	else
	{
		iAnim = ELDER_SIGN_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + 100.0 / 30.0;
	}

	SendWeaponAnim( iAnim );
	
}




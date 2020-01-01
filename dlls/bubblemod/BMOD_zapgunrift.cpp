// ---------------------------------------------------------------
// Zap Rift Entity
//
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE
//
//        Permission is granted to anyone to use this  software  for
//        any purpose on any computer system, and to redistribute it
//        in any way, subject to the following restrictions:
//
//        1. The author is not responsible for the  consequences  of
//           use of this software, no matter how awful, even if they
//           arise from defects in it.
//        2. The origin of this software must not be misrepresented,
//           either by explicit claim or by omission.
//        3. Altered  versions  must  be plainly marked as such, and
//           must  not  be  misrepresented  (by  explicit  claim  or
//           omission) as being the original software.
//        3a. It would be nice if I got  a  copy  of  your  improved
//            version  sent to halflife@bubblemod.org.
//        4. This notice must not be removed or altered.
//
// ---------------------------------------------------------------

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "effects.h"
#include "BMOD_zapgunrift.h"
#include "shake.h"

#define RIFTSPR "sprites/cnt1.spr"
#define RIFTSPR2 "sprites/cnt1.spr"

LINK_ENTITY_TO_CLASS( zaprift, CZapRift );

void CZapRift::Precache( void )
{
        PRECACHE_SOUND("debris/zap4.wav");
        PRECACHE_SOUND("weapons/electro4.wav");
        PRECACHE_SOUND("hassault/hw_shoot1.wav");
        PRECACHE_MODEL( RIFTSPR );
        PRECACHE_MODEL( RIFTSPR2 );
}

void CZapRift::Spawn( void )
{
        pev->classname = MAKE_STRING( "zapgun" );

        pev->movetype = MOVETYPE_FLY;
        pev->solid = SOLID_NOT;
 
		pev->rendermode = kRenderGlow;
        pev->renderamt = 240;
        pev->renderfx = kRenderFxNoDissipation;

		UTIL_MakeAimVectors( pev->angles );
		//pev->origin = pev->origin + gpGlobals->v_forward * 8;

        SET_MODEL(ENT(pev), RIFTSPR);
        pev->frame = 0;
        pev->scale = 2;
        m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

        // Create Inner Animated Sprite
        m_pSprite = CSprite::SpriteCreate( RIFTSPR2, pev->origin, TRUE );
        m_pSprite->Animate( 1 );
        m_pSprite->pev->scale = 1;
        m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 180, kRenderFxNoDissipation );
        m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;

        SetThink( &CZapRift::Animate );
        pev->nextthink = gpGlobals->time + 0.1f;
        m_fLifeSpan = gpGlobals->time + 3.0f;
        m_fNextElectrify = gpGlobals->time + 0.1f;

        EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0,100 );
}

void CZapRift::Animate( void )
{
        pev->nextthink = gpGlobals->time + 0.1f;



        if ( pev->frame++ )
        {
                if ( pev->frame > m_maxFrame )
                {
                        pev->frame = 0;
                }
        }
                
        m_pSprite->pev->scale = RANDOM_FLOAT(.5, 3);                

        if ( m_fLifeSpan < gpGlobals->time)
        {
			pev->effects |= EF_NODRAW;
            m_pSprite->Expand( 10, 100 );        
            SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 5.0f;
        }

        if ( m_fNextElectrify < gpGlobals->time )
        {
                // Send out electric streamers and do area effect damage

				for (int i = 0; i < 3; i++)
				{
					Vector vecTemp;
					Vector vecEnd = Vector (0,0,0);
					
					// Hopefully find a long beam
					for (int j = 0; j < 5; j++)
					{
						vecTemp = FindBeam();
						if ( vecTemp.Length() > vecEnd.Length() )
							vecEnd = vecTemp;
					}

					// Draw the streamer
					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMPOINTS );
						WRITE_COORD(pev->origin.x);
						WRITE_COORD(pev->origin.y);
						WRITE_COORD(pev->origin.z);
						WRITE_COORD( vecEnd.x );
						WRITE_COORD( vecEnd.y );
						WRITE_COORD( vecEnd.z );
						WRITE_SHORT( g_sModelIndexLightning );
						WRITE_BYTE( 0 ); // Starting frame
						WRITE_BYTE( 0  ); // framerate * 0.1
						WRITE_BYTE( 20 ); // life * 0.1
						WRITE_BYTE( 80 ); // width
						WRITE_BYTE( 50 ); // noise
						WRITE_BYTE( 180 ); // color r,g,b
						WRITE_BYTE( 255 ); // color r,g,b
						WRITE_BYTE( 96 ); // color r,g,b
						WRITE_BYTE( 255 ); // brightness
						WRITE_BYTE( 0 ); // scroll speed
					MESSAGE_END();

				}

				// Deal damage. 
				CBaseEntity *pEntity= NULL;
				TraceResult tr;
				while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 512 )) != NULL)
				{
					// Only look for players
					if ( pEntity->pev->takedamage != DAMAGE_NO )
					{
						// blast's don't tavel  out of water,
						// so ignore players that lie on the other side.
						if (pEntity->pev->waterlevel == 0)
							continue;

						// Trace to the target
						UTIL_TraceLine( pev->origin, pEntity->BodyTarget( pev->origin ) , dont_ignore_monsters, ENT(pev), &tr);
						if (tr.flFraction == 1.0f || tr.pHit == pEntity->edict())
						{
							ClearMultiDamage( );
							pEntity->TraceAttack( VARS( pev->owner ), 30, Vector(0,0,1), &tr, DMG_SHOCK | DMG_ALWAYSGIB );
							ApplyMultiDamage( pev, VARS( pev->owner ) );
							UTIL_ScreenFade( pEntity, Vector(180,255,96), 2, 0.5, 128, FFADE_IN ); 
						}
					}
				}
                m_fNextElectrify = gpGlobals->time + 1; 
        }

}

Vector CZapRift::FindBeam( void )
{
	TraceResult tr;

	// Direction is within the forward hemisphere.
	UTIL_MakeAimVectors( pev->angles );
	Vector vecDir = gpGlobals->v_right   * RANDOM_FLOAT( -1,  1) 
				  + gpGlobals->v_up      * RANDOM_FLOAT( -1,  1)
				  + gpGlobals->v_forward * RANDOM_FLOAT(  0,  1);
	vecDir = vecDir.Normalize();

	// Maximum distance
	float fDist = 1024;

	// End point of the streamer
	Vector vecEnd = pev->origin + vecDir * fDist;

	// Trace to the end of the streamer to see if 
	// there is architecture in the way.
	UTIL_TraceLine(pev->origin, vecEnd, ignore_monsters, ENT(pev), &tr);
	fDist = (tr.vecEndPos - pev->origin).Length();

	// Now we need to find the distance to the water transition. 
	// (if there is one)
	if (UTIL_PointContents(tr.vecEndPos) != CONTENTS_WATER)
	{
		Vector vecMin = pev->origin;
		Vector vecMax = pev->origin + vecDir * fDist;
		Vector vecMid;
		float diff = fDist;

		while (diff > 1.0f)
		{
			vecMid = vecMin + (vecMax - vecMin).Normalize() * diff / 2;
			if 	(UTIL_PointContents(vecMid) == CONTENTS_WATER)	
			{
				vecMin = vecMid;
			}
			else
			{
				vecMax = vecMid;
			}
			diff = (vecMax - vecMin).Length();
		}
		
		fDist = (vecMid - pev->origin).Length();
	}

	// Return new endpoint. 
	return pev->origin + vecDir * fDist;
}

LINK_ENTITY_TO_CLASS( zapbounce, CZapBounce );

void CZapBounce::Precache( void )
{
        PRECACHE_SOUND("debris/zap4.wav");
        PRECACHE_SOUND("weapons/electro4.wav");
        PRECACHE_SOUND("hassault/hw_shoot1.wav");
        PRECACHE_MODEL( RIFTSPR );
        PRECACHE_MODEL( RIFTSPR2 );
}

void CZapBounce::Spawn( void )
{
        pev->classname = MAKE_STRING( "multizapper" );

        pev->movetype = MOVETYPE_FLY;
        pev->solid = SOLID_NOT;
 
		//UTIL_MakeAimVectors( pev->angles );
		m_vecStart = pev->origin;
		m_vecDir = pev->angles;
		m_fDamage = 90;
		m_iBounce = 5;
		m_bFirstZap = TRUE;

        SetThink( &CZapBounce::BounceThink );
        pev->nextthink = gpGlobals->time + 0.2f;

        EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0,100 );
}

void CZapBounce::BounceThink( void )
{
		TraceResult tr;
        CBaseEntity *pEntity;

        pev->nextthink = gpGlobals->time + 0.05f;
		
		// Zap Forward with some randomness.
		if (!m_bFirstZap) 
		{
			m_vecDir = m_vecDir * 5 + Vector(RANDOM_FLOAT( -1,  1),
											   RANDOM_FLOAT( -1,  1),
											   RANDOM_FLOAT( -1,  1)
											   );
		}
		m_vecDir = m_vecDir.Normalize();

		// Maximum distance for this segment.
		float fDist = 128;

		// End point of the streamer
		Vector vecEnd = m_vecStart + m_vecDir * fDist;

		// Trace to the end of the streamer 
		UTIL_TraceLine(m_vecStart, vecEnd, dont_ignore_monsters, pentIgnore, &tr);
		fDist = (tr.vecEndPos - m_vecStart).Length();
		
		// draw lightning
		for (int i = 0; i < 2; i++)
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_BEAMPOINTS );
					WRITE_COORD(m_vecStart.x);
					WRITE_COORD(m_vecStart.y);
					WRITE_COORD(m_vecStart.z);
					WRITE_COORD( tr.vecEndPos.x );
					WRITE_COORD( tr.vecEndPos.y );
					WRITE_COORD( tr.vecEndPos.z );
					WRITE_SHORT( g_sModelIndexLightning );
					WRITE_BYTE( 0 ); // Starting frame
					WRITE_BYTE( 0  ); // framerate * 0.1
					WRITE_BYTE( 10 ); // life * 0.1
					WRITE_BYTE( m_iBounce * 10 ); // width
					WRITE_BYTE( 10 * (6 - m_iBounce )); // noise
					WRITE_BYTE( 96 ); // color r,g,b
					WRITE_BYTE( 180 ); // color r,g,b
					WRITE_BYTE( 255 ); // color r,g,b
					WRITE_BYTE( m_iBounce *  50); // brightness
					WRITE_BYTE( 0 ); // scroll speed
			MESSAGE_END();
		}

		m_vecStart = tr.vecEndPos;

		// Did we hit an entity? Then do damage.
        pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity) {
			if (pEntity->pev->takedamage)
			{
				ClearMultiDamage( );
				pEntity->TraceAttack( VARS(pev->owner), m_fDamage, m_vecDir, &tr, DMG_SHOCK | DMG_ALWAYSGIB );
				ApplyMultiDamage( VARS(pev->owner), VARS(pev->owner) );
				UTIL_ScreenFade( pEntity, Vector(96,180,255), 2, 0.5, 128, FFADE_IN ); 
			}
			// Did we hit a wall? Then reflect.
			else 
			{
				float n;
				n = -DotProduct(tr.vecPlaneNormal, m_vecDir);

				Vector r;	
				r = 2.0f * tr.vecPlaneNormal * n + m_vecDir;
				m_vecDir = r;
			}
			pentIgnore = ENT(pEntity->pev);
		}

		// We can hurt ourselves after the first beam.
		m_bFirstZap = FALSE;

		// bounce once
		m_iBounce--;
		if (m_iBounce < 1)
		{
			// UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> Zap bounce point destroyed.\n");
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1f;
			return;
		}

		// split beam?
		if (RANDOM_LONG(0,100) < 80) 
		{
			m_fDamage *= 0.75f;

			CZapBounce* pRift = (CZapBounce*)CBaseEntity::Create( "zapbounce", 
				m_vecStart, 
				m_vecDir, 
				ENT(pev->owner) );
			pRift->m_fDamage = m_fDamage;
			pRift->m_iBounce = m_iBounce;
			pRift->m_bFirstZap = FALSE;
			pRift->pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.2, 0.3 );
		}
}


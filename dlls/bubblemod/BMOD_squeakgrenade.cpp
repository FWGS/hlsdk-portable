// ---------------------------------------------------------------
// BubbleMod
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
// Extra Snark Functions

#include "squeakgrenade.h"
#include "BMOD_snarkmine.h"
extern cvar_t bm_snarks_mod;

void CSqueak::SecondaryAttack( void )
{

if (!bm_snarks_mod.value || m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 5)
                return;

        UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
        Vector vecSrc    = m_pPlayer->GetGunPosition( );
        Vector vecAiming = gpGlobals->v_forward;

        TraceResult tr;

        UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

        if (tr.flFraction < 1.0f)
        {
                // ALERT( at_console, "hit %f\n", tr.flFraction );

                CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
                if (pEntity && !(pEntity->pev->flags & FL_CONVEYOR))
                {
                        Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

                        CBaseEntity *pEnt = CBaseEntity::Create( "monster_tripsnark", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );

                        CTripSnarkGrenade *pMine = (CTripSnarkGrenade *)pEnt;

                        m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 5;

                        // player "shoot" animation
                        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

                        if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
                        {
                                SendWeaponAnim( SQUEAK_UP );
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

        m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
        m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT ( 10, 15 );
}

//=========================================================
// BestVisibleEnemy - this functions searches the link
// list whose head is the caller's m_pLink field, and returns
// a pointer to the enemy entity in that list that is nearest the 
// caller.
//
// !!!UNDONE - currently, this only returns the closest enemy.
// we'll want to consider distance, relationship, attack types, back turned, etc.
//=========================================================
CBaseEntity *CSqueakGrenade :: BMOD_BestVisibleEnemy ( void )
{
	CBaseEntity	*pReturn;
	CBaseEntity	*pNextEnt;
	BOOL		typer;
	int			iNearest;
	int			iDist;
	int			iBestRelationship;

	iNearest = 8192;// so first visible entity will become the closest.
	pNextEnt = m_pLink;
	pReturn = NULL;
	typer = 0;
	iBestRelationship = R_NO;

	while ( pNextEnt != NULL )
	{
		if (pNextEnt->IsPlayer()) {
			typer = ((CBasePlayer*)pNextEnt)->BMOD_IsTyping();
		}
		else {
			typer = 0;
		}

		if ( pNextEnt->IsAlive() && !typer)
		{
			if ( IRelationship( pNextEnt) > iBestRelationship )
			{
				// this entity is disliked MORE than the entity that we 
				// currently think is the best visible enemy. No need to do 
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship ( pNextEnt );
				iNearest = ( pNextEnt->pev->origin - pev->origin ).Length();
				pReturn = pNextEnt;
			}
			else if ( IRelationship( pNextEnt) == iBestRelationship )
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = ( pNextEnt->pev->origin - pev->origin ).Length();
				
				if ( iDist <= iNearest )
				{
					iNearest = iDist;
					iBestRelationship = IRelationship ( pNextEnt );
					pReturn = pNextEnt;
				}
			}
		}

		pNextEnt = pNextEnt->m_pLink;
	}

	return pReturn;
}

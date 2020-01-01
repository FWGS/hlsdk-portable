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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "BMOD_flyingcrowbar.h"

LINK_ENTITY_TO_CLASS( flying_crowbar, CFlyingCrowbar );

void CFlyingCrowbar::Spawn( )
{
   Precache( );

   // The flying crowbar is MOVETYPE_TOSS, and SOLID_BBOX.
   // We want it to be affected by gravity, and hit objects
   // within the game.
   pev->movetype = MOVETYPE_TOSS;
   pev->solid = SOLID_BBOX;

   // Use the world crowbar model.
   SET_MODEL(ENT(pev), "models/w_crowbar.mdl");

   // Set the origin and size for the HL engine collision
   // tables.
   UTIL_SetOrigin( pev, pev->origin );
   UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

   // Store the owner for later use. We want the owner to be able 
   // to hit themselves with the crowbar. The pev->owner gets cleared
   // later to avoid hitting the player as they throw the crowbar. 
   if ( pev->owner )
      m_hOwner = Instance( pev->owner );

   // Set the think funtion. 
   SetThink( &CFlyingCrowbar::BubbleThink );
   pev->nextthink = gpGlobals->time + 0.25f;

   // Set the touch function.
   SetTouch( &CFlyingCrowbar::SpinTouch );
}


void CFlyingCrowbar::Precache( )
{
   PRECACHE_MODEL ("models/w_crowbar.mdl");
   PRECACHE_SOUND ("weapons/cbar_hitbod1.wav");
   PRECACHE_SOUND ("weapons/cbar_hit1.wav");
   PRECACHE_SOUND ("weapons/cbar_miss1.wav");
}


void CFlyingCrowbar::SpinTouch( CBaseEntity *pOther )
{
   // We touched something in the game. Look to see if the object
   // is allowed to take damage. 
   if (pOther->pev->takedamage)
   {
      // Get the traceline info to the target.
      TraceResult tr = UTIL_GetGlobalTrace( );
   
      // Apply damage to the target. If we have an owner stored, use that one, 
      // otherwise count it as self-inflicted.
      ClearMultiDamage( );
      pOther->TraceAttack(pev, 90, pev->velocity.Normalize(), &tr, 
                          DMG_NEVERGIB ); 
      if( m_hOwner != 0 )
         ApplyMultiDamage( pev, m_hOwner->pev );
      else
         ApplyMultiDamage( pev, pev );
   }

   // If we hit a player, make a nice squishy thunk sound. Otherwise
   // make a clang noise and throw a bunch of sparks. 
   if (pOther->IsPlayer())
      EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 
                     1.0, ATTN_NORM, 0, 100);
   else 
   {
      EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hit1.wav", 
                     1.0, ATTN_NORM, 0, 100);
      if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
      {
         UTIL_Sparks( pev->origin );
         UTIL_Sparks( pev->origin );
         UTIL_Sparks( pev->origin );
      }
   }

   // Don't draw the flying crowbar anymore. 
   pev->effects |= EF_NODRAW;
   pev->solid = SOLID_NOT;

   // Spawn a crowbar weapon
   CBasePlayerWeapon *pItem = (CBasePlayerWeapon *)Create( "weapon_crowbar", 
                               pev->origin , pev->angles, edict() );

   // Spawn a weapon box
   CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( 
                  "weaponbox", pev->origin, pev->angles, edict() );

   // don't let weapon box tilt.
   pWeaponBox->pev->angles.x = 0;
   pWeaponBox->pev->angles.z = 0;

   // remove the weapon box after 4 mins.
   pWeaponBox->pev->nextthink = gpGlobals->time + 240; 
   pWeaponBox->SetThink( &CWeaponBox::Kill );

   // Pack the crowbar in the weapon box
   pWeaponBox->PackWeapon( pItem );

   // *** Start BubbleMod Rune code ***
   // Check to see if the crowbar rune is active on the owner,
   // if it is, then remove this crowbar after 3 seconds
   // instead if 2 mins. This prevents too many crowbars from
   // piling up on the server.  
   //
   // REMOVE THIS IF YOU ARE NOT USING THE RUNE CODE
   if (m_pPlayer->m_RuneFlags == RUNE_CROWBAR)
      pWeaponBox->pev->nextthink = gpGlobals->time + 3.0f;
   // *** End BubbleMod Rune Code ***

   // Get the unit vector in the direction of motion.
   Vector vecDir = pev->velocity.Normalize( );

   // Trace a line along the velocity vector to get the normal at impact.
   TraceResult tr;
   UTIL_TraceLine(pev->origin, pev->origin + vecDir * 100, 
                  dont_ignore_monsters, ENT(pev), &tr);

   // Throw the weapon box along the normal so it looks kinda
   // like a ricochet. This would be better if I actually 
   // calcualted the reflection angle, but I'm lazy. :)
   pWeaponBox->pev->velocity = tr.vecPlaneNormal * 300;

   // Remove this flying_crowbar from the world.
   SetThink( &CBaseEntity::SUB_Remove );
   pev->nextthink = gpGlobals->time + 0.1f;
}

void CFlyingCrowbar::BubbleThink( void )
{
   // We have no owner. We do this .25 seconds AFTER the crowbar
   // is thrown so that we don't hit the owner immediately when throwing
   // it. If is comes back later, we want to be able to hit the owner.
   pev->owner = NULL;

   // Only think every .25 seconds. 
   pev->nextthink = gpGlobals->time + 0.25f;

   // Make a whooshy sound. 
   EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/cbar_miss1.wav", 
                  1, ATTN_NORM, 0, 120);

   // If the crowbar enters water, make some bubbles. 
   if (pev->waterlevel)
	   UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 1 );
}

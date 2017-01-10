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

#include "BMOD_hornetgun.h"

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class BMODSquidSpit : public CBaseEntity
{
public:
        void Spawn( void );
		void Precache( void );

        static void Shoot( entvars_t *Owner, Vector vecStart, Vector vecVelocity );
        void Touch( CBaseEntity *pOther );
        void EXPORT Animate( void );

        static  TYPEDESCRIPTION m_SaveData[];

        int  m_maxFrame;
	entvars_t *pevOwner;

};
LINK_ENTITY_TO_CLASS( bmod_squidspit, BMODSquidSpit );


void BMODSquidSpit::Precache( void )
{

}

void BMODSquidSpit::Spawn( void )
{
        pev->movetype = MOVETYPE_FLY;
        pev->classname = MAKE_STRING( "bmod_squidspit" );

        pev->solid = SOLID_BBOX;
        pev->rendermode = kRenderTransAlpha;
        pev->renderamt = 255;

        SET_MODEL(ENT(pev), "sprites/bigspit.spr");
        pev->frame = 0;
        pev->scale = 0.5;

        UTIL_SetSize( pev, Vector( -4, -4, -4), Vector(4, 4, 4) );

        m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
        	WRITE_BYTE( TE_BEAMFOLLOW );
	        WRITE_SHORT(entindex()); // entity
	        WRITE_SHORT(g_sModelIndexSmokeTrail ); // model
	        WRITE_BYTE( 2 ); // life
	        WRITE_BYTE( 4 ); // width
	        WRITE_BYTE( 128 ); // r, g, b
	        WRITE_BYTE( 200 ); // r, g, b
	        WRITE_BYTE( 0 ); // r, g, b
	        WRITE_BYTE( 255 ); // brightness
        MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

}

void BMODSquidSpit::Animate( void )
{
        pev->nextthink = gpGlobals->time + 0.1;

        if ( pev->frame++ )
        {
                if ( pev->frame > m_maxFrame )
                {
                        pev->frame = 0;
                }
        }
}

void BMODSquidSpit::Shoot( entvars_t *Owner, Vector vecStart, Vector vecVelocity )
{
        BMODSquidSpit *pSpit = GetClassPtr( (BMODSquidSpit *)NULL );
        pSpit->Spawn();

        UTIL_SetOrigin( pSpit->pev, vecStart );
        pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(Owner);
        pSpit->pevOwner = Owner;

        pSpit->SetThink( &BMODSquidSpit::Animate );
        pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void BMODSquidSpit::Touch ( CBaseEntity *pOther )
{
        TraceResult tr;
        int             iPitch;

        // splat sound
        iPitch = RANDOM_FLOAT( 90, 110 );

        EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );

        switch ( RANDOM_LONG( 0, 1 ) )
        {
        case 0:
                EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );
                break;
        case 1:
                EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );
                break;
        }
        
        // make a splat on the wall
        UTIL_TraceLine( pev->origin, pev->origin + pev->velocity, dont_ignore_monsters, ENT( pev ), &tr );
        UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0,1));
	
        // make some flecks
        MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
        	WRITE_BYTE( TE_SPRITE_SPRAY );
                WRITE_COORD( tr.vecEndPos.x);   // pos
                WRITE_COORD( tr.vecEndPos.y);
                WRITE_COORD( tr.vecEndPos.z);
                WRITE_COORD( tr.vecPlaneNormal.x);      // dir
                WRITE_COORD( tr.vecPlaneNormal.y);
                WRITE_COORD( tr.vecPlaneNormal.z);
                WRITE_SHORT( g_sModelIndexSpit );        // model
                WRITE_BYTE ( 5 );                       // count
                WRITE_BYTE ( 30 );                      // speed
                WRITE_BYTE ( 80 );                      // noise ( client will divide by 100 )
        MESSAGE_END();

        if ( pOther->IsPlayer() )
	{
		ClearMultiDamage( );
       	       	pOther->TraceAttack( pevOwner, 30, pev->origin + pev->velocity, &tr, DMG_GENERIC );
		ApplyMultiDamage( pev, pevOwner );
	}

        SetThink( &CBaseEntity::SUB_Remove );
        pev->nextthink = gpGlobals->time;
}


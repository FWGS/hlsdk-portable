// botman's Half-Life bot example (http://jump.to/botman/)

#include "extdll.h"
#include "util.h"
#include "client.h"
#include "cbase.h"
#include "player.h"
#include "items.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"

#include "bot.h"

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;

// Set in combat.cpp.  Used to pass the damage inflictor for death messages.
extern entvars_t *g_pevLastInflictor;

extern int gmsgHealth;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;

int f_Observer = 0;  // flag to indicate if we are in observer mode

ammo_check_t ammo_check[] = {
/*   {"ammo_glockclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmAR", "9mm", _9MM_MAX_CARRY},
   {"ammo_mp5clip", "9mm", _9MM_MAX_CARRY},
   {"ammo_9mmbox", "9mm", _9MM_MAX_CARRY},
   {"ammo_chainboxclip", "9mm", _9MM_MAX_CARRY},
   {"ammo_mp5grenades", "ARgrenades", M203_GRENADE_MAX_CARRY},
   {"ammo_ARgrenades", "ARgrenades", M203_GRENADE_MAX_CARRY},
   {"ammo_buckshot", "buckshot", BUCKSHOT_MAX_CARRY},
   {"ammo_crossbow", "bolts", BOLTGUN_MAX_CARRY},
   {"ammo_357", "357", _357_MAX_CARRY},
   {"ammo_rpgclip", "rockets", ROCKET_MAX_CARRY},
   {"ammo_egonclip", "uranium", URANIUM_MAX_CARRY},
   {"ammo_gaussclip", "uranium", URANIUM_MAX_CARRY},
*/
	{"", 0, 0}};

LINK_ENTITY_TO_CLASS( bot, CBot );


inline edict_t *CREATE_FAKE_CLIENT( const char *netname )
{
    return (*g_engfuncs.pfnCreateFakeClient)( netname );
}

inline char *GET_INFOBUFFER( edict_t *e )
{
    return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
    (*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}


void BotCreate( void )
{
   edict_t *BotEnt;

   BotEnt = CREATE_FAKE_CLIENT( "Bot" );

   if (FNullEnt( BotEnt ))
      ALERT ( at_console, "NULL Ent in CREATE_FAKE_CLIENT!\n" );
   else
   {
      char ptr[128];  // allocate space for message from ClientConnect
      CBot *BotClass;
      char *infobuffer;
      int clientIndex;

      BotClass = GetClassPtr( (CBot *) VARS(BotEnt) );
      infobuffer = GET_INFOBUFFER( BotClass->edict( ) );
      clientIndex = BotClass->entindex( );

      SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "gina" );

      ClientConnect( BotClass->edict( ), "Bot", "127.0.0.1", ptr );
      DispatchSpawn( BotClass->edict( ) );
   }
}


void CBot::Spawn( )
{
   pev->classname    = MAKE_STRING( "player" );
   pev->health       = 100;
   pev->armorvalue   = 0;
   pev->takedamage   = DAMAGE_AIM;
   pev->solid        = SOLID_SLIDEBOX;
   pev->movetype     = MOVETYPE_WALK;
   pev->max_health   = pev->health;
   pev->flags        = FL_CLIENT | FL_FAKECLIENT;
   pev->air_finished = gpGlobals->time + 12;
   pev->dmg          = 2;     // initial water damage
   pev->effects      = 0;
   pev->deadflag     = DEAD_NO;
   pev->dmg_take     = 0;
   pev->dmg_save     = 0;

   m_bitsHUDDamage   = -1;
   m_bitsDamageType  = 0;
   m_afPhysicsFlags  = 0;
   m_fLongJump       = FALSE; // no longjump module. 
   m_iFOV            = 0;     // init field of view.
   m_iClientFOV      = -1;    // make sure fov reset is sent

   m_flNextDecalTime = 0;     // let this player decal as soon as he spawns.

   // wait a few seconds until user-defined message registrations are recieved by all clients
   m_flgeigerDelay   = gpGlobals->time + 2.0;
   
   m_flTimeStepSound = 0;
   m_iStepLeft       = 0;

   // some monsters use this to determine whether or not the player is looking at them.
   m_flFieldOfView   = 0.5;

   m_bloodColor      = BLOOD_COLOR_RED;
   m_flNextAttack    = gpGlobals->time;
   StartSneaking( );

   m_iFlashBattery   = 99;
   m_flFlashLightTime = 1;    // force first message

   // dont let uninitialized value here hurt the player
   m_flFallVelocity  = 0;

   g_pGameRules->SetDefaultPlayerTeam( this );
   g_pGameRules->GetPlayerSpawnSpot( this );

   SET_MODEL( ENT( pev ), "models/player.mdl" );

   g_ulModelIndexPlayer = pev->modelindex;

   pev->sequence = LookupActivity( ACT_IDLE );

   if (FBitSet( pev->flags, FL_DUCKING )) 
      UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
   else
      UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );

   pev->view_ofs = VEC_VIEW;

   Precache( );

   m_HackedGunPos = Vector( 0, 32, 0 );

   if (m_iPlayerSound == SOUNDLIST_EMPTY)
   {
      ALERT ( at_console, "Couldn't alloc player sound slot!\n" );
   }

   m_fNoPlayerSound = FALSE;  // normal sound behavior.

   m_pLastItem = NULL;
   m_fInitHUD = TRUE;
   m_iClientHideHUD = -1;     // force this to be recalculated
   m_fWeapon = FALSE;
   m_pClientActiveItem = NULL;
   m_iClientBattery = -1;

   // reset all ammo values to 0
   for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
   {
      // client ammo values also have to be reset
      // (the death hud clear messages does on the client side)
      m_rgAmmo[i] = 0;
      m_rgAmmoLast[i] = 0;
   }

   m_lastx = m_lasty = 0;

   g_pGameRules->PlayerSpawn( this );

   SetThink( BotThink );
   pev->nextthink = gpGlobals->time + .1;

   v_prev_origin = pev->origin;  // save previous position
   f_shoot_time = 0;

   f_max_speed = CVAR_GET_FLOAT("sv_maxspeed") * .60;
   f_speed_check_time = gpGlobals->time + 1.0;
   ladder_dir = 0;

   if (RANDOM_LONG(1, 100) & 1)
      wander_dir = WANDER_LEFT;
   else
      wander_dir = WANDER_RIGHT;

   f_pause_time = 0;
   f_find_item = 0;

   pBotEnemy = NULL;
}


void CBot::Killed( entvars_t *pevAttacker, int iGib )
{
   CSound *pSound;

   g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );

   if (m_pTank != NULL)
   {
      m_pTank->Use( this, this, USE_OFF, 0 );
      m_pTank = NULL;
   }

   // this client isn't going to be thinking for a while, so reset the sound
   // until they respawn
   pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex(edict( )) );
   {
      if (pSound)
      {
         pSound->Reset( );
      }
   }

   SetAnimation( PLAYER_DIE );
   
   pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

#if !defined(DUCKFIX)
   pev->view_ofs      = Vector( 0, 0, -8 );
#endif
   pev->deadflag      = DEAD_DYING;
   pev->solid         = SOLID_NOT;
   pev->movetype      = MOVETYPE_TOSS;
//   pev->movetype      = MOVETYPE_NONE;  // should we use this instead???

   ClearBits( pev->flags, FL_ONGROUND );

   if (pev->velocity.z < 10)
      pev->velocity.z += RANDOM_FLOAT( 0, 300 );

   // clear out the suit message cache so we don't keep chattering
   SetSuitUpdate( NULL, FALSE, 0 );

   // send "health" update message to zero
   m_iClientHealth = 0;

   MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
      WRITE_BYTE( m_iClientHealth );
   MESSAGE_END( );

   // Tell Ammo Hud that the player is dead
   MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
      WRITE_BYTE( 0 );
      WRITE_BYTE( 0xFF );
      WRITE_BYTE( 0xFF );
   MESSAGE_END( );

   // reset FOV
   m_iFOV = m_iClientFOV = 0;

   MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
      WRITE_BYTE( 0 );
   MESSAGE_END( );

   // UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
   // UTIL_ScreenFade( edict( ), Vector( 128, 0, 0 ), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

   if (( pev->health < -40 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS)
   {
      GibMonster( );   // This clears pev->model

// TAKE THIS OUT!!! THIS HAPPENS SOMETIMES WHEN HEALTH IS < -40, THEN THE
// THINK FUNCTION DOESN'T EVER GET SET TO PlayerDeathThink
// (should we REALLY be doing this???)
//      pev->effects |= EF_NODRAW;
//      return;
   }

   DeathSound( );
   
   pev->angles.x = 0;  // don't let the player tilt
   pev->angles.z = 0;

   SetThink( PlayerDeathThink );
   pev->nextthink = gpGlobals->time + 0.1;
}


void CBot::PlayerDeathThink( void )
{
   float flForward;

   pev->nextthink = gpGlobals->time + 0.1;

   if (FBitSet( pev->flags, FL_ONGROUND ))
   {
      flForward = pev->velocity.Length( ) - 20;
      if (flForward <= 0)
         pev->velocity = g_vecZero;
      else    
         pev->velocity = flForward * pev->velocity.Normalize( );
   }

   if (HasWeapons( ))
   {
      // we drop the guns here because weapons that have an area effect and can kill their user
      // will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
      // player class sometimes is freed. It's safer to manipulate the weapons once we know
      // we aren't calling into any of their code anymore through the player pointer.

      PackDeadPlayerItems( );
   }

   if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
   {
      StudioFrameAdvance( );

      m_iRespawnFrames++;
      if (m_iRespawnFrames < 60)  // animations should be no longer than this
         return;
   }

   if (pev->deadflag == DEAD_DYING)
   {
      pev->deadflag = DEAD_DEAD;
      DROP_TO_FLOOR ( ENT(pev) );  // put the body on the ground
   }
   
   StopAnimation( );

   pev->effects |= EF_NOINTERP;
   pev->framerate = 0.0;

   if (pev->deadflag == DEAD_DEAD)
   {
      if (g_pGameRules->FPlayerCanRespawn( this ))
      {
         m_fDeadTime = gpGlobals->time;
         pev->deadflag = DEAD_RESPAWNABLE;
      }
      
      return;
   }

   // check if time to respawn...
   if (gpGlobals->time > (m_fDeadTime + 5.0))
   {
      pev->button = 0;
      m_iRespawnFrames = 0;

      //ALERT( at_console, "Respawn\n" );

      respawn( pev, !(m_afPhysicsFlags & PFLAG_OBSERVER) );
      pev->nextthink = -1;
   }
}


void CBot::BotDebug( char *buffer )
{
   // print out debug messages to the HUD of all players
   // this allows you to print messages from bots to your display
   // as you are playing the game.

   UTIL_ClientPrintAll( HUD_PRINTNOTIFY, buffer );
}


void CBot::BotOnLadder( void )
{
   // moves the bot up or down a ladder.  if the bot can't move
   // (i.e. get's stuck with someone else on ladder), the bot will
   // change directions and go the other way on the ladder.

   if (ladder_dir == LADDER_UP)  // are we currently going up?
   {
      pev->v_angle.x = -60;  // look upwards

      Vector v_diff = v_prev_origin - pev->origin;

      // check if we haven't moved much since the last location...
      if (v_diff.Length() <= 1)
      {
         // we must be stuck, change directions...

         pev->v_angle.x = 60;  // look downwards
         ladder_dir = LADDER_DOWN;
      }
   }
   else if (ladder_dir == LADDER_DOWN)  // are we currently going down?
   {
      pev->v_angle.x = 60;  // look downwards

      Vector v_diff = v_prev_origin - pev->origin;

      // check if we haven't moved much since the last location...
      if (v_diff.Length() <= 1)
      {
         // we must be stuck, change directions...

         pev->v_angle.x = -60;  // look upwards
         ladder_dir = LADDER_UP;
      }
   }
   else  // we haven't picked a direction yet, try going up...
   {
      pev->v_angle.x = -60;  // look upwards
      ladder_dir = LADDER_UP;
   }

   // move forward (i.e. in the direction we are looking, up or down)
   pev->button |= IN_FORWARD;

   f_move_speed = f_max_speed;
}


CBaseEntity * CBot::BotFindEnemy( void )
{
   if (pBotEnemy != NULL)  // do we already have an enemy?
   {
      if (!pBotEnemy->IsAlive( ))
      {
         // the enemy is dead, jump for joy every once in a while...
         if (RANDOM_LONG(1, 10) == 1)
            pev->button |= IN_JUMP;
      }
      else if (FVisible( pBotEnemy ) && FInViewCone( pBotEnemy ))
      {
         // if enemy is still visible and in field of view, keep it
         return (pBotEnemy);
      }
   }

   int i;
   float nearestdistance = 1000;
   CBaseEntity *pNewEnemy = NULL;

   // search the world for players...
   for (i = 1; i <= gpGlobals->maxClients; i++)
   {
      CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

      // skip invalid players and skip self (i.e. this bot)
      if ((!pPlayer) || (pPlayer == this))
         continue;

      // skip this player if not alive (i.e. dead or dying)
      if (pPlayer->pev->deadflag != DEAD_NO)
         continue;

      // skip players that are netclients (i.e. not bot, observer mode)
      if (pPlayer->IsNetClient() && f_Observer)
         continue;

      // see if you can see the player (within your field of view)...
      if (FVisible( pPlayer ) && FInViewCone( pPlayer ))
      {
         float distance = (pPlayer->pev->origin - pev->origin).Length( );
         if (distance < nearestdistance)
         {
            nearestdistance = distance;
            pNewEnemy = pPlayer;
         }
      }
   }

   return (pNewEnemy);
}


void CBot::BotShootAtEnemy( void )
{
   float f_length;

   // aim for the head and/or body
   Vector v_src;  // dummy vector for Player::BodyTarget()
   Vector v_enemy = pBotEnemy->BodyTarget( v_src ) - GetGunPosition( );
   
   // face the enemy
   pev->v_angle = UTIL_VecToAngles( v_enemy );
   pev->v_angle.x = -pev->v_angle.x;  //adjust pitch to point gun

   v_enemy.z = 0;  // ignore z component (up & down)

   f_length = v_enemy.Length();  // how far away is the enemy scum?

   if (f_length > 200)      // run if distance to enemy is far
      f_move_speed = f_max_speed;
   else if (f_length > 10)  // walk if distance is closer
      f_move_speed = f_max_speed / 2;
   else                     // don't move if close enough
      f_move_speed = 0.0;

   // is it time to shoot yet?
   if (f_shoot_time <= gpGlobals->time)
   {
      pev->button |= IN_ATTACK;  // use primary attack (bang! bang!)

      // set next time to shoot
      f_shoot_time = gpGlobals->time + 0.3 + RANDOM_FLOAT(0, 0.2);
   }
}


void CBot::BotFindItem( void )
{
   CBaseEntity *pEntity = NULL;
   CBasePlayerItem *pItem = NULL;
   float    radius = 100;

   while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, radius )) != NULL)
   {
      // check to make sure that the entity is visible and in field of view...
      if (FVisible( pEntity ) && FInViewCone( pEntity ))
      {
         // check if entity is a weapon...
         if (strncmp("weapon_", STRING(pEntity->pev->classname), 7) == 0)
         {
            CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)pEntity;

            if ((pWeapon->m_pPlayer) || (pWeapon->pev->effects & EF_NODRAW))
            {
               // someone owns this weapon or it hasn't respawned yet
               continue;
            }

            if (g_pGameRules->CanHavePlayerItem( this, pWeapon ))
            {
               // let's head off toward that item...
               Vector v_item = pEntity->pev->origin - pev->origin;
               pev->v_angle = UTIL_VecToAngles( v_item );
            }
         }

         // check if entity is ammo...
         if (strncmp("ammo_", STRING(pEntity->pev->classname), 5) == 0)
         {
            CBasePlayerAmmo *pAmmo = (CBasePlayerAmmo *)pEntity;
            char ammo_name[40];
            int i;
            BOOL can_pickup = FALSE;

            // check if the item is not visible (i.e. has not respawned)
            if (pAmmo->pev->effects & EF_NODRAW)
               continue;

            strcpy(ammo_name, STRING(pEntity->pev->classname));

            i = 0;
            while (ammo_check[i].ammo_name[0])
            {
               if (strcmp(ammo_check[i].ammo_name, ammo_name) == 0)
               {
                  // see if we can pick up this item...
                  if (g_pGameRules->CanHaveAmmo( this,
                      ammo_check[i].weapon_name, ammo_check[i].max_carry))
                  {
                     can_pickup = TRUE;
                     break;
                  }
               }

               i++;
            }

            if (can_pickup)
            {
               // let's head off toward that item...
               Vector v_item = pEntity->pev->origin - pev->origin;
               pev->v_angle = UTIL_VecToAngles( v_item );
            }
         }

         // check if entity is a battery...
         if (strcmp("item_battery", STRING(pEntity->pev->classname)) == 0)
         {
            CItem *pBattery = (CItem *)pEntity;

            // check if the item is not visible (i.e. has not respawned)
            if (pBattery->pev->effects & EF_NODRAW)
               continue;

            // check if the bot can use this item...
            if ((pev->armorvalue < MAX_NORMAL_BATTERY) &&
                (pev->weapons & (1<<WEAPON_SUIT)))
            {
               // let's head off toward that item...
               Vector v_item = pEntity->pev->origin - pev->origin;
               pev->v_angle = UTIL_VecToAngles( v_item );
            }
         }

         // check if entity is a healthkit...
         if (strcmp("item_healthkit", STRING(pEntity->pev->classname)) == 0)
         {
            CItem *pHealthKit = (CItem *)pEntity;

            // check if the item is not visible (i.e. has not respawned)
            if (pHealthKit->pev->effects & EF_NODRAW)
               continue;

            // check if the bot can use this item...
            if (pev->health <= 90)
            {
               // let's head off toward that item...
               Vector v_item = pEntity->pev->origin - pev->origin;
               pev->v_angle = UTIL_VecToAngles( v_item );
            }
         }

         // check if entity is a packed up weapons box...
         if (strcmp("weaponbox", STRING(pEntity->pev->classname)) == 0)
         {
            // let's head off toward that item...
            Vector v_item = pEntity->pev->origin - pev->origin;
            pev->v_angle = UTIL_VecToAngles( v_item );
         }
      }
   }
}


void CBot::BotThink( void )
{
   if (!IsInWorld( ))
   {
      SetTouch( NULL );
      UTIL_Remove( this );
      return;
   }

   if (!IsAlive( ))
   {
      // if bot is dead, don't take up any space in world (set size to 0)
      UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0));

      return;
   }

   // see if it's time to check for sv_maxspeed change...
   if (f_speed_check_time <= gpGlobals->time)
   {
      // store current sv_maxspeed setting for quick access
      f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");

      // set next check time to 2 seconds from now
      f_speed_check_time = gpGlobals->time + 2.0;
   }

   pev->button = 0;  // make sure no buttons are pressed


   if (IsOnLadder( ))  // check if the bot is on a ladder...
   {
      BotOnLadder( );  // go handle the ladder movement
   }
   else
   {
      // bot is not on a ladder so clear ladder direction flag...
      ladder_dir = 0;

      pBotEnemy = BotFindEnemy( );  // try to find an enemy

      if (pBotEnemy != NULL)  // does an enemy exist?
      {
         BotShootAtEnemy( );  // go handle shooting at the enemy
      }
      else if (f_pause_time > gpGlobals->time)  // are we "paused"?
      {
         // you could make the bot look left then right, or look up
         // and down, to make it appear that the bot is hunting for
         // something (don't do anything right now)

         f_move_speed = 0;
      }
      else
      {
         // no enemy, let's just wander around...

         // check if we should look for items now or not...
         if (f_find_item < gpGlobals->time)
         {
            BotFindItem( );  // see if there are any visible items
         }

         pev->v_angle.x = 0;  // reset pitch to 0 (level horizontally)
         pev->v_angle.z = 0;  // reset roll to 0 (straight up and down)

         Vector v_diff = v_prev_origin - pev->origin;

         // check if we haven't moved much since the last location...
         if (v_diff.Length() <= 1)
         {
            // we must be stuck!

            // turn randomly between 10 and 30 degress from current direction
            if (wander_dir == WANDER_LEFT)
               pev->v_angle.y += RANDOM_LONG(10, 30);
            else
               pev->v_angle.y -= RANDOM_LONG(10, 30);

            // check for wrap around of angle...
            if (pev->v_angle.y > 180)
               pev->v_angle.y -= 360;
            if (pev->v_angle.y < -180)
               pev->v_angle.y += 360;

            // also don't look for items for 5 seconds since we could be
            // stuck trying to get to an item
            f_find_item = gpGlobals->time + 5.0;
         }

         f_move_speed = f_max_speed;

         if (RANDOM_LONG(1, 1000) <= 4)  // should we pause for a while here?
         {
            // set the time that the bot will stop "pausing"
            f_pause_time = gpGlobals->time + RANDOM_FLOAT(0.5, 1.0);
         }
      }
   }

   v_prev_origin = pev->origin;  // save current position as previous

//   g_engfuncs.pfnRunPlayerMove(<bot edict>, <bot's view angles>,
//             <forward move>, <side move>, <up move> (does nothing?),
//             <buttons> (IN_ATTACK, etc.), <impulse number>, <msec value>);

   g_engfuncs.pfnRunPlayerMove( edict( ), pev->v_angle, f_move_speed,
                                0, 0, pev->button, 0, 50 );
}


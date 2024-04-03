// botman's Half-Life bot example
//
// http://planethalflife.com/botman/
//
// bot.h
//

#ifndef BOT_H
#define BOT_H

#define LADDER_UP    1
#define LADDER_DOWN  2

#define WANDER_LEFT  1
#define WANDER_RIGHT 2

#define MODEL_GINA    1
#define MODEL_COLETTE 2

#define BOT_YAW_SPEED 20  // degrees per 10th of second turning speed

#define BOT_SKIN_LEN 128
#define BOT_NAME_LEN 31

#define ENEMY_HEADCRAB 0
#define ENEMY_ZOMBIE 1
#define ENEMY_BULLSQUID 2
#define ENEMY_ASLAVE 3
#define ENEMY_HOUNDEYE 4
#define ENEMY_AGRUNT 5
#define ENEMY_HGRUNT 6
#define ENEMY_CONTROLLER 7
#define ENEMY_FLYER 8
#define ENEMY_UNKNOWN 254
#define ENEMY_NONE 255

typedef struct  // used in checking if bot can pick up ammo
{
   const char *ammo_name;
   const char *weapon_name;
   int max_carry;
} ammo_check_t;

#define BOT_IDLE             0
#define BOT_NEED_TO_KICK     1
#define BOT_NEED_TO_RESPAWN  2
#define BOT_IS_RESPAWNING    3

typedef struct  // used to respawn bot at end of round (time/frag limit)
{
   BOOL is_used;  // is this slot in use?
   int  state;    // current state of the bot
   char skin[BOT_SKIN_LEN+1];
   char name[BOT_NAME_LEN+1];
   char skill[2];
   CBasePlayer *pBot;
} respawn_t;

#define GI_SND1 "gina/gina_attacked0.wav"
#define GI_SND2 "gina/gina_attacked1.wav "
#define GI_SND3 "gina/gina_pain1.wav "
#define GI_SND4 "gina/gina_pain2.wav "
#define GI_SND5 "gina/gina_pain3.wav "

#define CO_SND1 "colette/colette_attacked0.wav"
#define CO_SND2 "colette/colette_attacked1.wav"
#define CO_SND3 "colette/colette_pain0.wav"
#define CO_SND4 "colette/colette_pain2.wav"
#define CO_SND5 "colette/colette_pain3.wav"

//
// joy after successful enemy kill
//

#define CO_TNT1 "colette/colette_kill0.wav"
#define CO_TNT2 "colette/colette_kill1.wav"
#define CO_TNT3 "colette/colette_kill2.wav"
#define CO_TNT4 "colette/colette_kill3.wav"
#define CO_TNT5 "colette/colette_kill4.wav"

#define GI_TNT1 "gina/gina_kill0.wav"
#define GI_TNT2 "gina/gina_kill1.wav"
#define GI_TNT3 "gina/gina_kill2.wav"
#define GI_TNT4 "gina/gina_kill3.wav"
#define GI_TNT5 "gina/gina_kill4.wav"

#define USE_TEAMPLAY_SND "barney/teamup2.wav"
#define USE_TEAMPLAY_LATER_SND "barney/seeya.wav"
#define USE_TEAMPLAY_ENEMY_SND "barney/ba_raincheck.wav"

void BotDebug( char *buffer );  // print out message to HUD for debugging


class CBot : public CBasePlayer //Derive a bot class from CBasePlayer
{
   public:
      Vector v_prev_origin;   // previous origin (i.e. location)
      float  f_shoot_time;    // next time to shoot weapon at
      float  f_max_speed;     // last sv_maxspeed setting
      float  f_speed_check_time;  // check sv_maxspeed every so often
      float  f_move_speed;    // speed at which the bot will move
      int    ladder_dir;      // direction traveling on ladder (UP or DOWN)
      int    wander_dir;      // randomly wander left or right
      float  f_pause_time;    // timeout for periods when the bot pauses
      float  f_find_item;     // timeout for not looking for items
      char   model_name[20];
      int    bot_model;
      int    bot_skill;       // bot skill level (0=very good, 4=very bad)
      float  f_pain_time;     // time when pain sound can be spoken
      BOOL   b_use_health_station;  // set if bot should "use" health station
      float  f_use_health_time;  // time when b_use_health_station is set
      BOOL   b_use_HEV_station;  // set if bot should "use" HEV station
      float  f_use_HEV_time;  // time when b_use_HEV_station is set
      BOOL   b_use_button;    // set if bot should "use" button
      float  f_use_button_time;  // time when b_use_button is set
      BOOL   b_lift_moving;   // flag set when lift (elevator) is moving
      float  f_use_ladder_time;  // time when bot sees a ladder
      BOOL   b_see_tripmine;  // set if bot "sees" a tripmine
      BOOL   b_shoot_tripmine;  // set if bot should shoot a tripmine
      Vector v_tripmine_origin;  // origin of tripmine
      float  f_fire_gauss;    // time to release secondary fire on gauss gun
      BOOL   bot_was_paused;  // TRUE if bot was previously "paused"
      float  f_weapon_inventory_time;  // time to check weapon inventory
      int    respawn_index;   // index in respawn structure for this bot
      float  f_dont_avoid_wall_time;  // time when avoiding walls is OK
      float  f_bot_use_time;  // time the bot was "used" by player
      float  f_wall_on_left;  // time since bot has had a wall on the left
      float  f_wall_on_right; // time since bot has had a wall on the right

      CBaseEntity *pBotEnemy; // pointer to bot's enemy
	  EOFFSET pBotEnemyOffset;
	  byte pBotEnemyClass;
      CBaseEntity *pBotUser;  // pointer to player using bot
      CBaseEntity *pBotPickupItem;  // pointer to item we are trying to get
      CBasePlayerItem *weapon_ptr[MAX_WEAPONS];  // pointer array to weapons
      int primary_ammo[MAX_WEAPONS];  // amount of primary ammo available
      int secondary_ammo[MAX_WEAPONS];  // amount of secondary ammo available

      char   message[256];    // buffer for debug messages

      void Spawn( void );
      void BotThink( void );  // think function for the bot

      // Bots should return FALSE for this, they can't receive NET messages
      virtual BOOL IsNetClient( void ) { return FALSE; }

      int BloodColor() { return BLOOD_COLOR_RED; }
      int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker,
                      float flDamage, int bitsDamageType );
      int ObjectCaps() { return FCAP_IMPULSE_USE; };
      void Use( CBaseEntity *pActivator, CBaseEntity *pCaller,
                USE_TYPE useType, float value );

      int BotInFieldOfView( Vector dest );
      BOOL BotEntityIsVisible( Vector dest );
      float BotChangeYaw( float speed );
      void BotOnLadder( float moved_distance );
      void BotUnderWater( void );
      CBaseEntity * BotFindEnemy( void );
      Vector BotBodyTarget( CBaseEntity *pBotEnemy );
      void BotWeaponInventory( void );
      BOOL BotFireWeapon( Vector enemy, int weapon_choice = 0, BOOL primary = TRUE );
      void BotShootAtEnemy( void );
      void BotFindItem( void );
      void BotUseLift( float moved_distance );
      void BotTurnAtWall( TraceResult *tr );
      BOOL BotCantMoveForward( TraceResult *tr );
      BOOL BotCanJumpUp( void );
      BOOL BotCanDuckUnder( void );
      BOOL BotShootTripmine( void );
      BOOL BotFollowUser( void );  // returns FALSE if can find "user"
      BOOL BotCheckWallOnLeft( void );
      BOOL BotCheckWallOnRight( void );
};

#endif // BOT_H


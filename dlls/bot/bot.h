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

#define MODEL_HGRUNT    1
#define MODEL_BARNEY    2
#define MODEL_SCIENTIST 3

#define BOT_YAW_SPEED 20  // degrees per 10th of second turning speed

#define BOT_SKIN_LEN 16
#define BOT_NAME_LEN 31


typedef struct  // used in checking if bot can pick up ammo
{
   char *ammo_name;
   char *weapon_name;
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

#define HG_SND1 "hgrunt/gr_pain1.wav"
#define HG_SND2 "hgrunt/gr_pain2.wav"
#define HG_SND3 "hgrunt/gr_pain3.wav"
#define HG_SND4 "hgrunt/gr_pain4.wav"
#define HG_SND5 "hgrunt/gr_pain5.wav"

#define BA_SND1 "barney/ba_bring.wav"
#define BA_SND2 "barney/ba_pain1.wav"
#define BA_SND3 "barney/ba_pain2.wav"
#define BA_SND4 "barney/ba_pain3.wav"
#define BA_SND5 "barney/ba_die1.wav"

#define SC_SND1 "scientist/sci_fear8.wav"
#define SC_SND2 "scientist/sci_fear13.wav"
#define SC_SND3 "scientist/sci_fear14.wav"
#define SC_SND4 "scientist/sci_fear15.wav"
#define SC_SND5 "scientist/sci_pain3.wav"

#define BA_TNT1 "barney/ba_another.wav"
#define BA_TNT2 "barney/ba_endline.wav"
#define BA_TNT3 "barney/ba_gotone.wav"
#define BA_TNT4 "barney/ba_seethat.wav"
#define BA_TNT5 "barney/ba_tomb.wav"

#define SC_TNT1 "scientist/odorfromyou.wav"
#define SC_TNT2 "scientist/smellburn.wav"
#define SC_TNT3 "scientist/somethingfoul.wav"
#define SC_TNT4 "scientist/youlookbad.wav"
#define SC_TNT5 "scientist/youneedmedic.wav"

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


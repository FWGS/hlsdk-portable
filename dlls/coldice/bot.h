#ifndef BOT_H
#define BOT_H

// botman's Half-Life bot example (http://jump.to/botman/)

#define LADDER_UP    1
#define LADDER_DOWN  2

#define WANDER_LEFT  1
#define WANDER_RIGHT 2

typedef struct  // used in checking if bot can pick up ammo
{
   char *ammo_name;
   char *weapon_name;
   int max_carry;
} ammo_check_t;


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

      CBaseEntity *pBotEnemy; // pointer to bot's enemy

      char   message[256];    // buffer for debug messages

      void Spawn( void );
      void BotThink( void );
      void Killed(entvars_t *pevAttacker, int iGib);
      void PlayerDeathThink( void );

      // Bots should return FALSE for this, they can't receive NET messages
      virtual BOOL IsNetClient( void ) { return FALSE; }

      int BloodColor() { return BLOOD_COLOR_RED; }

      void BotDebug( char *message );
      void BotOnLadder( void );
      CBaseEntity * CBot::BotFindEnemy( void );
      void BotShootAtEnemy( void );
      void BotFindItem( void );
};

#endif // BOT_H


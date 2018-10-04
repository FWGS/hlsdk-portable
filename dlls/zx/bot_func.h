//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_func.h
//

#ifndef BOT_FUNC_H
#define BOT_FUNC_H


//prototypes of bot functions...

void BotSpawnInit( bot_t *pBot );
void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4, const char *arg5 );
void BotStartGame( bot_t *pBot );
int BotInFieldOfView( bot_t *pBot, Vector dest );
bool BotEntityIsVisible( bot_t *pBot, Vector dest );
void BotPickLogo(bot_t *pBot);
void BotSprayLogo(edict_t *pEntity, char *logo_name);
void BotFindItem( bot_t *pBot );
bool BotLookForMedic( bot_t *pBot );
bool BotLookForGrenades( bot_t *pBot );
void BotThink( bot_t *pBot );

void LoadBotModels(void);

void BotFixIdealPitch( edict_t *pEdict );
float BotChangePitch( bot_t *pBot, float speed );
void BotFixIdealYaw( edict_t *pEdict );
float BotChangeYaw( bot_t *pBot, float speed );
bool BotFindWaypoint( bot_t *pBot );
bool BotHeadTowardWaypoint( bot_t *pBot );
void BotOnLadder( bot_t *pBot, float moved_distance );
void BotUnderWater( bot_t *pBot );
void BotUseLift( bot_t *pBot, float moved_distance );
bool BotStuckInCorner( bot_t *pBot );
void BotTurnAtWall( bot_t *pBot, TraceResult *tr, bool negative );
bool BotCantMoveForward( bot_t *pBot, TraceResult *tr );
bool BotCanJumpUp( bot_t *pBot, bool *bDuckJump );
bool BotCanDuckUnder( bot_t *pBot );
void BotRandomTurn( bot_t *pBot );
bool BotFollowUser( bot_t *pBot );
bool BotCheckWallOnLeft( bot_t *pBot );
bool BotCheckWallOnRight( bot_t *pBot );
void BotLookForDrop( bot_t *pBot );

void BotCheckTeamplay(void);
edict_t *BotFindEnemy( bot_t *pBot );
Vector BotBodyTarget( edict_t *pBotEnemy, bot_t *pBot );
bool BotFireWeapon( Vector v_enemy, bot_t *pBot, int weapon_choice);
void BotShootAtEnemy( bot_t *pBot );
bool BotShootTripmine( bot_t *pBot );
bool BotGrenadeArm( bot_t *pBot );
void BotGrenadeThrow( bot_t *pBot );


#endif // BOT_FUNC_H


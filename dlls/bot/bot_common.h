#ifndef BOT_COMMON_H
#define BOT_COMMON_H
#define SKIP_BOT_EXPORTS //prevent defination conflicts
// halflife common
#include        "extdll.h"
#include        "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "client.h"
// regamedll
#define NOXREF
#include "bot_gameevents.h"
#include "bot/botsengcallback.h"
#include "bot/crtwrap.h"

#include "bot/pm_math.h"
#include "bot/mp_parse.h"
#include "bot/shared_util.h"
#include "bot/utllinkedlist.h"
#include "bot/utlvector.h"
#define MAX_CLIENTS 32

// manager
class CNavNode;
#include "bot/steam_util.h"
#include "bot/manager/nav.h"
#include "bot/manager/nav_area.h"
#include "bot/manager/nav_node.h"
#include "bot/manager/nav_file.h"
#include "bot/manager/improv.h"
#include "bot/manager/bot_util.h"

#include "bot/manager/nav_path.h"



#include "bot/manager/bot.h"

#include "bot/manager/bot_manager.h"

#include "bot/manager/bot_constants.h"
#include "bot/manager/bot_profile.h"

// zbot
#include "bot/hl_bot.h"

#endif

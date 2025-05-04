#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "player.h"
#include "saverestore.h"
#define Ent_IsValidEdict( e )	( e && !e->free )

// stop any actions with players
static cvar_t mp_enttools_players = { "mp_enttools_players", "0", FCVAR_SERVER };
// prevent ent_fire with entities not created with enttools
static cvar_t mp_enttools_lockmapentities = { "mp_enttools_lockmapentities", "0", FCVAR_SERVER };
static cvar_t mp_enttools_checkowner = { "mp_enttools_checkowner", "0", FCVAR_SERVER };


bool Q_isdigit( const char *str )
{
	if( str && *str )
	{
		while( isdigit( *str )) str++;
		if( !*str ) return true;
	}
	return false;
}

typedef struct entblacklist_s
{
	struct entblacklist_s *next;
	char pattern[32];
	int limit;
	int behaviour;
	bool clear;
} entblacklist_t;

entblacklist_t *entblacklist;

void Ent_AddToBlacklist_f( void )
{
	if( CMD_ARGC() < 4 )
	{
		ALERT( at_console, "Usage: mp_enttools_blacklist <pattern> <per minute limit> <behaviour (0 - block, 1 - kick, 2 - ban)> <clear>\n"  );
	}

	entblacklist_t *node = (entblacklist_t *)malloc( sizeof( entblacklist_t ) );

	node->next = entblacklist;
	strlcpy( node->pattern, CMD_ARGV(1), 32 );
	node->pattern[31] = 0;
	node->limit = atoi( CMD_ARGV(2) );
	node->behaviour = atoi( CMD_ARGV( 3 ) );
	node->clear = !!atoi( CMD_ARGV( 4 ) );
	entblacklist = node;
}

void Ent_ClearBlacklist_f( void )
{
	while( entblacklist )
	{
		entblacklist_t *node = entblacklist;
		entblacklist = node->next;
		free( node );
	}
}

bool Ent_CheckFire( edict_t *player, edict_t *ent, const char *command )
{
	if( !mp_enttools_players.value && ENTINDEX( ent ) < gpGlobals->maxClients + 1 )
		return false;

	CBaseEntity *pEntity = CBaseEntity::Instance( ent );

	if( pEntity )
	{
		if( mp_enttools_lockmapentities.value && !pEntity->enttools_data.enttools )
			return false;

		// only if player online or registered
		if( mp_enttools_checkowner.value == 1 )
		{
			if( GGM_PlayerByID( pEntity->enttools_data.ownerid ) )
				return !strcmp( pEntity->enttools_data.ownerid, GGM_GetPlayerID( player ) );
			if( pEntity->enttools_data.enttools == 2 )
				return false;
		}

		if( mp_enttools_checkowner.value == 2 )
		{
				return !strcmp( pEntity->enttools_data.ownerid, GGM_GetPlayerID( player ) );
		}
	}

	return true;
}

bool Ent_CheckCreate( edict_t *player, const char *classname )
{
	CBasePlayer *p = (CBasePlayer*)CBaseEntity::Instance(player);
	entblacklist_t *node;

	if( !p )
		return false;

	if( p->m_ggm.flEntScore > 1 )
		return false;

	if( gpGlobals->time - p->m_ggm.flEntTime > 60 )
	{
		p->m_ggm.flEntTime = gpGlobals->time;
		p->m_ggm.flEntScore = 0;
	}

	for( node = entblacklist; node; node = node->next )
	{
		if( Q_stricmpext(node->pattern, classname ) )
		{
			if( !node->limit || ( p->m_ggm.flEntScore + 1.0f / (float)node->limit > 1 ) )
			{
				// remove all created entities
				if( node->clear )
					Ent_RunGC( GC_ENTTOOLS, GGM_GetPlayerID( player ) );

				if( node->behaviour == 2 )
				{
					SERVER_COMMAND( UTIL_VarArgs("banid 0 #%d kick\nwriteid\n", GETPLAYERUSERID( player ) ) );
				}
				else if( node->behaviour == 1 )
				{
					SERVER_COMMAND( UTIL_VarArgs("kick #%d\nwriteid\n", GETPLAYERUSERID( player ) ) );
				}
				return false;
			}
			p->m_ggm.flEntScore += 1.0f / (float)node->limit;
		}
	}

	return true;
}

void Ent_ClientPrintf( edict_t *player, const char *format, ... )
{
	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[sizeof( string ) - 1] = 0;

	//ClientPrint( &player->v, HUD_PRINTCONSOLE, string );
	CLIENT_PRINTF( player, print_console, string );
}

static edict_t *Ent_GetCrossEnt( edict_t *player )
{
	edict_t *ent = g_engfuncs.pfnPEntityOfEntIndex(1);
	edict_t *closest = NULL;
	float flMaxDot = 0.94;
	Vector forward;
	int i;
	float maxLen = 1000;

	UTIL_MakeVectorsPrivate( player->v.v_angle, forward, NULL, NULL );

	Vector viewPos = player->v.origin + player->v.view_ofs;

	// find bmodels by trace
	{
		TraceResult tr;

		UTIL_TraceLine(viewPos, viewPos + forward * 1000, missile, player, &tr);
		closest = tr.pHit;
		maxLen = (viewPos - tr.vecEndPos).Length() + 30;
	}

	// check untraceable entities
	for ( i = 1; i < gpGlobals->maxEntities; i++, ent++ )
	{
		Vector vecLOS;
		Vector vecOrigin;
		float flDot, traceLen;

		if( ent->free )
			continue;

		if( ent->v.solid == SOLID_BSP || ent->v.movetype == MOVETYPE_PUSHSTEP )
			continue; //bsp models will be found by trace later

		// do not touch following weapons
		if( ent->v.movetype == MOVETYPE_FOLLOW )
			continue;

		if( ent == player )
			continue;

		vecOrigin = VecBModelOrigin(&ent->v);

		vecLOS = vecOrigin - viewPos;
		traceLen = vecLOS.Length();

		if( traceLen > maxLen )
			continue;

		vecLOS = UTIL_ClampVectorToBox(vecLOS, ent->v.size * 0.5);

		flDot = DotProduct(vecLOS , forward);
		if ( flDot <= flMaxDot )
			continue;

		TraceResult tr;
		UTIL_TraceLine(viewPos, vecOrigin, missile, player, &tr);
		if( (tr.vecEndPos - viewPos).Length() + 30 < traceLen )
			continue;
		closest = ent, flMaxDot = flDot;
	}

	return closest;
}

/*
===============
Ent_List_f

Print list of entities to client
===============
*/
void Ent_List_f( edict_t *player )
{
	edict_t	*ent = NULL;
	int	i;

	for( i = 0; i < gpGlobals->maxEntities; i++ )
	{
		ent = g_engfuncs.pfnPEntityOfEntIndex( i );
		if( !Ent_IsValidEdict( ent )) continue;

		// filter by string
		if( CMD_ARGC() > 1 )
			if( !Q_stricmpext( CMD_ARGV( 1 ), STRING( ent->v.classname ) ) && !Q_stricmpext( CMD_ARGV( 1 ), STRING( ent->v.targetname ) ) )
				continue;
		Vector borigin = VecBModelOrigin( &ent->v );

		Ent_ClientPrintf( player, "%5i origin: %.f %.f %.f", i, ent->v.origin[0], ent->v.origin[1], ent->v.origin[2] );
		Ent_ClientPrintf( player, "%5i borigin: %.f %.f %.f", i, borigin[0], borigin[1], borigin[2] );

		if( ent->v.classname )
			Ent_ClientPrintf( player, ", class: %s", STRING( ent->v.classname ));

		if( ent->v.globalname )
			Ent_ClientPrintf( player, ", global: %s", STRING( ent->v.globalname ));

		if( ent->v.targetname )
			Ent_ClientPrintf( player, ", name: %s", STRING( ent->v.targetname ));

		if( ent->v.target )
			Ent_ClientPrintf( player, ", target: %s", STRING( ent->v.target ));

		if( ent->v.model )
			Ent_ClientPrintf( player, ", model: %s", STRING( ent->v.model ));

		Ent_ClientPrintf( player, "\n" );
	}
}



edict_t *Ent_FindSingle( edict_t *player, const char *pattern )
{
	edict_t	*ent = NULL;
	int	i = 0;

	if( Q_isdigit( pattern ) )
	{
		i = atoi( pattern );

		if( i >= gpGlobals->maxEntities )
			return NULL;
	}
	else if( !stricmp( pattern, "!cross" ) )
	{
		ent = Ent_GetCrossEnt( player );

		if( !Ent_IsValidEdict( ent ) )
			return NULL;

		i = ENTINDEX( ent );
	}
	else if( pattern[0] == '!' ) // Check for correct instanse with !(num)_(serial)
	{
		const char *p = pattern + 1;
		i = atoi( p );

		while( isdigit(*p) ) p++;

		if( *p++ != '_' )
			return NULL;

		if( i >= gpGlobals->maxEntities )
			return NULL;

		ent = g_engfuncs.pfnPEntityOfEntIndex( i );

		if( ent->serialnumber != atoi( p ) )
			return NULL;
	}
	else
	{
		for( i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++ )
		{
			ent = g_engfuncs.pfnPEntityOfEntIndex( i );

			if( !Ent_IsValidEdict( ent ) )
				continue;

			if( Q_stricmpext( pattern, STRING( ent->v.targetname ) ) )
				break;
		}
	}

	ent = g_engfuncs.pfnPEntityOfEntIndex( i );

	if( !Ent_IsValidEdict( ent ) )
		return NULL;

	return ent;
}


/*
===============
Ent_Info_f

Print specified entity information to client
===============
*/
void Ent_Info_f( edict_t *player )
{
	edict_t	*ent = NULL;

	if( CMD_ARGC() != 2 )
	{
		Ent_ClientPrintf( player, "Use ent_info <index|name|inst>\n" );
		return;
	}

	ent = Ent_FindSingle( player,  CMD_ARGV( 1 ) );

	if( !Ent_IsValidEdict( ent )) return;

	Vector borigin = VecBModelOrigin( &ent->v );

	Ent_ClientPrintf( player, "origin: %.f %.f %.f\n", ent->v.origin[0], ent->v.origin[1], ent->v.origin[2] );

	Ent_ClientPrintf( player, "angles: %.f %.f %.f\n", ent->v.angles[0], ent->v.angles[1], ent->v.angles[2] );

	Ent_ClientPrintf( player, "borigin: %.f %.f %.f\n", borigin[0], borigin[1], borigin[2] );

	if( ent->v.classname )
		Ent_ClientPrintf( player, "class: %s\n", STRING( ent->v.classname ));

	if( ent->v.globalname )
		Ent_ClientPrintf( player, "global: %s\n", STRING( ent->v.globalname ));

	if( ent->v.targetname )
		Ent_ClientPrintf( player, "name: %s\n", STRING( ent->v.targetname ));

	if( ent->v.target )
		Ent_ClientPrintf( player, "target: %s\n", STRING( ent->v.target ));

	if( ent->v.model )
		Ent_ClientPrintf( player, "model: %s\n", STRING( ent->v.model ));

	Ent_ClientPrintf( player, "health: %.f\n", ent->v.health );

	if( ent->v.gravity != 1.0f )
		Ent_ClientPrintf( player, "gravity: %.2f\n", ent->v.gravity );

	Ent_ClientPrintf( player, "movetype: %d\n", ent->v.movetype );

	Ent_ClientPrintf( player, "rendermode: %d\n", ent->v.rendermode );
	Ent_ClientPrintf( player, "renderfx: %d\n", ent->v.renderfx );
	Ent_ClientPrintf( player, "renderamt: %f\n", ent->v.renderamt );
	Ent_ClientPrintf( player, "rendercolor: %f %f %f\n", ent->v.rendercolor[0], ent->v.rendercolor[1], ent->v.rendercolor[2] );

	Ent_ClientPrintf( player, "maxspeed: %f\n", ent->v.maxspeed );

	if( ent->v.solid )
		Ent_ClientPrintf( player, "solid: %d\n", ent->v.solid );
	Ent_ClientPrintf( player, "flags: 0x%x\n", ent->v.flags );
	Ent_ClientPrintf( player, "spawnflags: 0x%x\n", ent->v.spawnflags );
}

void Ent_SendVars( edict_t *player, edict_t *ent )
{
	if( !ent )
		return;

	CLIENT_COMMAND( player, UTIL_VarArgs( "set ent_last_name \"%s\"\n", STRING( ent->v.targetname ) ));
	CLIENT_COMMAND( player, UTIL_VarArgs( "set ent_last_num %i\n", ENTINDEX( ent ) ));
	CLIENT_COMMAND( player, UTIL_VarArgs( "set ent_last_inst !%i_%i\n", ENTINDEX( ent ), ent->serialnumber ));
	CLIENT_COMMAND( player, UTIL_VarArgs( "set ent_last_origin \"%f %f %f\"\n", ent->v.origin[0], ent->v.origin[1], ent->v.origin[2]));
	CLIENT_COMMAND( player, UTIL_VarArgs( "set ent_last_class \"%s\"\n", STRING( ent->v.classname )));
	CLIENT_COMMAND( player, "ent_getvars_cb\n" );
}

void Ent_GetVars_f( edict_t *player )
{
	edict_t *ent = NULL;

	if( CMD_ARGC() != 2 )
	{
		Ent_ClientPrintf( player, "Use ent_getvars <index|name|inst>\n" );
		return;
	}

	ent = Ent_FindSingle( player, CMD_ARGV( 1 ) );
	if( CMD_ARGC() )
	if( !Ent_IsValidEdict( ent )) return;
	Ent_SendVars( player,  ent );
}

void Ent_HelpFire( edict_t *player )
{
	Ent_ClientPrintf( player, "Availiavle commands:\n" );
	Ent_ClientPrintf( player, "Set fields:\n" );
	Ent_ClientPrintf( player, "        (Only set entity field, does not call any functions)\n" );
	Ent_ClientPrintf( player, "    health\n" );
	Ent_ClientPrintf( player, "    gravity\n" );
	Ent_ClientPrintf( player, "    movetype\n" );
	Ent_ClientPrintf( player, "    solid\n" );
	Ent_ClientPrintf( player, "    rendermode\n" );
	Ent_ClientPrintf( player, "    rendercolor (vector)\n" );
	Ent_ClientPrintf( player, "    renderfx\n" );
	Ent_ClientPrintf( player, "    renderamt\n" );
	Ent_ClientPrintf( player, "    hullmin (vector)\n" );
	Ent_ClientPrintf( player, "    hullmax (vector)\n" );
	Ent_ClientPrintf( player, "Actions\n" );
	Ent_ClientPrintf( player, "    rename: set entity targetname\n" );
	Ent_ClientPrintf( player, "    settarget: set entity target (only targetnames)\n" );
	Ent_ClientPrintf( player, "    setmodel: set entity model\n" );
	Ent_ClientPrintf( player, "    set: set <key> <value> by server library\n" );
	Ent_ClientPrintf( player, "        See game FGD to get list.\n" );
	Ent_ClientPrintf( player, "        command takes two arguments\n" );
	Ent_ClientPrintf( player, "    touch: touch entity by current player.\n" );
	Ent_ClientPrintf( player, "    use: use entity by current player.\n" );
	Ent_ClientPrintf( player, "    movehere: place entity in player fov.\n" );
	Ent_ClientPrintf( player, "    drop2floor: place entity to nearest floor surface\n" );
	Ent_ClientPrintf( player, "    move: move entity up/forward\n" );
	Ent_ClientPrintf( player, "    relink: relink entity to world to fix collisions\n" );
	Ent_ClientPrintf( player, "Flags:\n" );
	Ent_ClientPrintf( player, "        (Set/clear specified flag bit, arg is bit number)\n" );
	Ent_ClientPrintf( player, "    setflag\n" );
	Ent_ClientPrintf( player, "    clearflag\n" );
	Ent_ClientPrintf( player, "    setspawnflag\n" );
	Ent_ClientPrintf( player, "    clearspawnflag\n" );
}

// xash3d only
// maybe incompatible with BSP2 engine
#include <com_model.h>
#include <physint.h>
bool Ent_CheckModel( const char *model )
{
	if( !mp_enttools_checkmodels.value )
		return true;

	// null model is safe
	if( !model || !model[0] )
		return true;

	// check for brush submodel safety
	if( model[0] == '*' )
	{
		if( !model[1] || !Q_isdigit(model + 1) ) // will crash client engine before 0.19.3
			return false;
		model_t *world = (model_t*)g_physfuncs.pfnGetModel(1);
		int submodel = atoi(model + 1);
		if( submodel < 1 || ( world && submodel >= world->numsubmodels ) )
			return false;
	} // do not allow to set different model types. bsp models will destroy all submodels on map
	else if( !strstr( model, ".mdl" ) && !strstr( model, ".spr" ) )
		return false;
	return true;
}

class CDumper: public CSave
{
public:
	CDumper( edict_t *cl, bool E, bool EV ) : CSave(NULL), client(cl), dumpEmpty(E), dumpEntvars(EV) {};
	/*CSave& operator CSave&()
	{
		return (CSave&)*this;
	}*/
	int WriteFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
	int WriteEntVars( const char *pname, entvars_t *pev )
	{
		if( dumpEntvars )
			return CSave::WriteEntVars( pname, pev );
		return true;
	}
private:
	edict_t *client;
	bool dumpEmpty;
	bool dumpEntvars;
};
#ifdef __GNUC__
extern "C" char *__cxa_demangle(const char* mangled_name,
								char* output_buffer, size_t* length,
								int* status);
#endif
#define PRINTARR(pat, type, add) \
	for( j = 0; j < pTest->fieldSize - 1; j++ ) \
		Ent_ClientPrintf( client, pat", ", ((type*)pOutputData)[j] add ); \
	Ent_ClientPrintf( client, pat"\n", ((type*)pOutputData)[pTest->fieldSize - 1] add);

int CDumper::WriteFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	int i, j;

	if( !dumpEntvars && !strcmp( pname, "ENTVARS") )
		return 0;

	Ent_ClientPrintf( client, "%s:\n", pname );

	for( i = 0; i < fieldCount; i++ )
	{
		TYPEDESCRIPTION	*pTest = &pFields[i];
		void *pOutputData = ( (char *)pBaseData + pTest->fieldOffset );

		if( !dumpEmpty && FieldEmpty(pTest, pOutputData) )
			continue;

		if( pTest->fieldSize > 1 )
			Ent_ClientPrintf( client, "%s : \n", pTest->fieldName );
		else
			Ent_ClientPrintf( client, "%s : ", pTest->fieldName );

		switch( pTest->fieldType )
		{
		case FIELD_FLOAT:
			PRINTARR("%f",float,)
			break;
		case FIELD_TIME:
			PRINTARR("%+fs",float,-gpGlobals->time)
			break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			for( j = 0; j < pTest->fieldSize - 1; j++ )
				if(((string_t*)pOutputData)[j])
					Ent_ClientPrintf( client, "\"%s\", ", STRING(((string_t*)pOutputData)[j]) );
				else
					Ent_ClientPrintf( client, "null, ");

			if(((string_t*)pOutputData)[pTest->fieldSize - 1])
				Ent_ClientPrintf( client, "\"%s\"\n", STRING(((string_t*)pOutputData)[pTest->fieldSize - 1]) );
			else
				Ent_ClientPrintf( client, "null\n");
			break;
		case FIELD_CLASSPTR:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			for( j = 0; j < pTest->fieldSize; j++ )
			{
				CBaseEntity *ent;
				switch( pTest->fieldType )
				{
					case FIELD_EVARS:
						ent = CBaseEntity::Instance( ( (entvars_t **)pOutputData )[j] );
						break;
					case FIELD_CLASSPTR:
						ent = ( (CBaseEntity **)pOutputData )[j];
						break;
					case FIELD_EDICT:
						ent = CBaseEntity::Instance( ( (edict_t **)pOutputData )[j] );
						break;
					case FIELD_ENTITY:
						ent = CBaseEntity::Instance( ( (EOFFSET *)pOutputData )[j] );
						break;
					case FIELD_EHANDLE:
						ent = (CBaseEntity *)( ( (EHANDLE *)pOutputData)[j] );
						break;
					default:
						break;
				}
				if( ent )
				{
					const char *classname = "";
					if( ent->pev->classname )
						classname = STRING(ent->pev->classname);

					Ent_ClientPrintf( client, "%i %s\n", ENTINDEX(ent->edict()), classname);
				}
				else
					Ent_ClientPrintf( client, "null\n");

			}
			break;
		case FIELD_POSITION_VECTOR:
		case FIELD_VECTOR:
			for( j = 0; j < pTest->fieldSize; j++ )
				Ent_ClientPrintf( client, "%f %f %f\n", ((float*)pOutputData)[j*3],((float*)pOutputData)[j*3+1],((float*)pOutputData)[j*3+2] );
			break;
		case FIELD_BOOLEAN:
		case FIELD_INTEGER:
			PRINTARR("%i",int,)
			break;
		case FIELD_SHORT:
			PRINTARR("%i",short,)
			break;
		case FIELD_CHARACTER:
			for( j = 0; j < pTest->fieldSize; j++ )
			{
				signed char c =  ((char*)pOutputData)[j];
				if( c < 32 ) // allow bool/int types
					Ent_ClientPrintf( client, "\\%d", c);
				else
					Ent_ClientPrintf( client, "%c", c);
				if( c == 0 )
					break;
			}
			Ent_ClientPrintf( client, "\n");
			break;
		// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			PRINTARR("%p",void*,)
			break;
		case FIELD_FUNCTION:
			for( j = 0; j < pTest->fieldSize; j++ )
			{
				const char *name = NAME_FOR_FUNCTION(((void**)pOutputData)[j]);
#ifdef __GNUC__
				char *demangled = __cxa_demangle( name, NULL, NULL, NULL );
				if( demangled ) name = demangled;
				Ent_ClientPrintf( client, "%s\n", name );
				if( demangled ) free( demangled );
#else
				Ent_ClientPrintf( client, "%s\n", name );
#endif

			}
			break;
		default:
			ALERT( at_error, "Bad field type\n" );
		}
	}
	return 1;
}


void Ent_Dump_f(edict_t *player)
{
	CDumper dumper( player, atoi(CMD_ARGV(2)), atoi(CMD_ARGV(3)) );
	edict_t *pent = Ent_FindSingle(player, CMD_ARGV(1));
	CBaseEntity *ent = CBaseEntity::Instance( pent );
	if( ent )
		ent->Save( dumper );
}

/*
===============
Ent_Fire_f

Perform some actions
===============
*/
void Ent_Fire_f( edict_t *player )
{
	edict_t	*ent = NULL;
	int	i = 1, count = 0;
	qboolean single; // true if user specified something that match single entity

	if( CMD_ARGC() < 3 )
	{
		Ent_ClientPrintf( player, "Use ent_fire <index||pattern> <command> [<values>]\n"
			"Use ent_fire help to get command list\n" );
		if( CMD_ARGC() == 2 )
			Ent_HelpFire( player );
		return;
	}

	if( ( single = Q_isdigit( CMD_ARGV( 1 ) ) ) )
	{
		i = atoi( CMD_ARGV( 1 ) );

		if( i < 0 || i >= gpGlobals->maxEntities )
			return;

		ent = g_engfuncs.pfnPEntityOfEntIndex( i );
	}
	else if( ( single = !stricmp( CMD_ARGV( 1 ), "!cross" ) ) )
	{
		ent = Ent_GetCrossEnt( player );

		if( !Ent_IsValidEdict( ent ) )
			return;

		i = ENTINDEX( ent );
	}
	else if( ( single = ( CMD_ARGV( 1 )[0] == '!') ) ) // Check for correct instanse with !(num)_(serial)
	{
		const char *cmd = CMD_ARGV( 1 ) + 1;
		i = atoi( cmd );

		while( isdigit( *cmd ) ) cmd++;

		if( *cmd++ != '_' )
			return;

		if( i < 0 || i >= gpGlobals->maxEntities )
			return;

		ent = g_engfuncs.pfnPEntityOfEntIndex( i );
		if( ent && ent->serialnumber != atoi( cmd ) )
			return;

		if( !Ent_IsValidEdict( ent ) )
			return;
	}
	else
	{
		i = gpGlobals->maxClients + 1;
	}

	for( ; ( i <  gpGlobals->maxEntities ) && ( count < mp_enttools_maxfire.value ); i++ )
	{
		ent = g_engfuncs.pfnPEntityOfEntIndex( i );
		if( !Ent_IsValidEdict( ent ))
		{
			if( single )
				break;
			continue;
		}

		// if user specified not a number, try find such entity
		if( !single )
		{
			if( !Q_stricmpext( CMD_ARGV( 1 ), STRING( ent->v.targetname ) ) && !Q_stricmpext( CMD_ARGV( 1 ), STRING( ent->v.classname ) ))
				continue;
		}
		const char *cmd = CMD_ARGV( 2 );

		if( !stricmp( cmd, "help" ) )
		{
			Ent_HelpFire( player );
			return;
		}

		if( !Ent_CheckFire( player, ent, CMD_ARGV( 2 ) ) )
		{
			if( !single )
				continue;
			else
				break;
		}

		Ent_ClientPrintf( player, "entity %i\n", i );

		if( single && count > 0 )
			break;


		count++;

		if( !stricmp( cmd, "health" ) )
		{
			ent->v.health = atoi( CMD_ARGV ( 3 ) );
		}
		if( !stricmp( cmd, "gravity" ) )
		{
			ent->v.gravity = atof( CMD_ARGV ( 3 ) );
		}
		if( !stricmp( cmd, "movetype" ) )
		{
			ent->v.movetype = atoi( CMD_ARGV ( 3 ) );
		}
		if( !stricmp( cmd, "solid" ) )
		{
			ent->v.solid = atoi( CMD_ARGV ( 3 ) );
		}
		if( !stricmp( cmd, "rename" ) )
		{
			ent->v.targetname = ALLOC_STRING( CMD_ARGV ( 3 ) );
		}
		if( !stricmp( cmd, "settarget" ) )
		{
			ent->v.target = ALLOC_STRING( CMD_ARGV ( 3 ) );
		}
		if( !stricmp( cmd, "setmodel" ) )
		{
			const char *model = CMD_ARGV( 3 );
			if( !Ent_CheckModel( model ) )
			{
				Ent_ClientPrintf(player, "Bad model %s\n", model );
				return;
			}
			SET_MODEL( ent, model );
		}
		if( !stricmp( cmd, "set" ) )
		{
			char keyname[256];
			char value[256];
			KeyValueData	pkvd;
			if( CMD_ARGC() != 5 )
				break;
			pkvd.szClassName = (char*)STRING( ent->v.classname );
			strlcpy( keyname, CMD_ARGV( 3 ), sizeof( keyname ) );
			strlcpy( value, CMD_ARGV( 4 ), sizeof( value ) );
			pkvd.szKeyName = keyname;
			pkvd.szValue = value;
			pkvd.fHandled = false;
			DispatchKeyValue( ent, &pkvd );
			if( pkvd.fHandled )
				Ent_ClientPrintf( player, "value set successfully!\n" );
		}
		if( !stricmp( cmd, "touch" ) )
		{
			if( CMD_ARGC() == 4 )
			{
				edict_t *other = Ent_FindSingle( player,  CMD_ARGV( 3 ) );
				if( other && other->pvPrivateData )
					DispatchTouch( ent, other  );
			}
			else
				DispatchTouch( ent, player );
		}
		if( !stricmp( cmd, "use" ) )
		{
			if( CMD_ARGC() == 4 )
			{
				edict_t *other = Ent_FindSingle( player,  CMD_ARGV( 3 ) );
				if( other && other->pvPrivateData )
					DispatchUse( ent, other );
			}
			else
				DispatchUse( ent, player );
		}
		if( !stricmp( cmd, "movehere" ) )
		{
			UTIL_SetOrigin( &ent->v, player->v.origin + Vector( 100 * cos( player->v.angles[1]/180*M_PI ), 100 * sin( player->v.angles[1]/180*M_PI), 25 ) );
		}
		if( !stricmp( cmd, "drop2floor" ) )
		{
			DROP_TO_FLOOR( ent );
		}
		if( !stricmp( cmd, "moveup" ) ||  !stricmp( cmd, "move" ) )
		{
			float dist = 25;

			if( CMD_ARGC() >= 4 )
				dist = atof( CMD_ARGV( 3 ) );

			ent->v.origin[2] +=  dist;

			if( CMD_ARGC() >= 5 )
				ent->v.origin = ent->v.origin + Vector( cos( player->v.angles[1]/180*M_PI ), sin( player->v.angles[1]/180*M_PI), 0 ) * atof( CMD_ARGV( 4 ) );
		}
		if( !stricmp( cmd, "becomeowner" ) )
		{
			if( CMD_ARGC() == 4 )
				ent->v.owner = Ent_FindSingle( player,  CMD_ARGV( 3 ) );
			else
				ent->v.owner = player;
		}
		if( !stricmp( cmd, "becomeenemy" ) )
		{
			if( CMD_ARGC() == 4 )
				ent->v.enemy = Ent_FindSingle( player,  CMD_ARGV( 3 ) );
			else
				ent->v.enemy = player;
		}
		if( !stricmp( cmd, "becomeaiment" ) )
		{
			if( CMD_ARGC() == 4 )
				ent->v.aiment= Ent_FindSingle( player,  CMD_ARGV( 3 ) );
			else
				ent->v.aiment = player;
		}
		if( !stricmp( cmd, "relink" ) )
		{
			UTIL_SetOrigin( &ent->v, ent->v.origin );
		}
		if( !stricmp( cmd, "hullmin" ) )
		{
			if( CMD_ARGC() != 6 )
				return;
			ent->v.mins[0] = atof( CMD_ARGV( 3 ) );
			ent->v.mins[1] = atof( CMD_ARGV( 4 ) );
			ent->v.mins[2] = atof( CMD_ARGV( 5 ) );
		}
		if( !stricmp( cmd, "hullmax" ) )
		{
			if( CMD_ARGC() != 6 )
				return;
			ent->v.maxs[0] = atof( CMD_ARGV( 3 ) );
			ent->v.maxs[1] = atof( CMD_ARGV( 4 ) );
			ent->v.maxs[2] = atof( CMD_ARGV( 5 ) );
		}
		if( !stricmp( cmd, "rendercolor" ) )
		{
			if( CMD_ARGC() != 6 )
				return;
			ent->v.rendercolor[0] = atof( CMD_ARGV( 3 ) );
			ent->v.rendercolor[1] = atof( CMD_ARGV( 4 ) );
			ent->v.rendercolor[2] = atof( CMD_ARGV( 5 ) );
		}
		if( !stricmp( cmd, "renderamt" ) )
		{
			ent->v.renderamt = atof( CMD_ARGV( 3 ) );
		}
		if( !stricmp( cmd, "renderfx" ) )
		{
			ent->v.renderfx = atoi( CMD_ARGV( 3 ) );
		}
		if( !stricmp( cmd, "rendermode" ) )
		{
			ent->v.rendermode = atoi( CMD_ARGV( 3 ) );
		}
		if( !stricmp( cmd, "angles" ) )
		{
			ent->v.angles[0] = atof( CMD_ARGV( 3 ) );
			ent->v.angles[1] = atof( CMD_ARGV( 4 ) );
			ent->v.angles[2] = atof( CMD_ARGV( 5 ) );
		}
		if( !stricmp( cmd, "setflag" ) )
		{
			ent->v.flags |= 1U << atoi( CMD_ARGV ( 3 ) );
			Ent_ClientPrintf( player, "flags set to 0x%x\n", ent->v.flags );
		}
		if( !stricmp( cmd, "clearflag" ) )
		{
			ent->v.flags &= ~( 1U << atoi( CMD_ARGV ( 3 ) ) );
			Ent_ClientPrintf( player, "flags set to 0x%x\n", ent->v.flags );
		}
		if( !stricmp( cmd, "setspawnflag" ) )
		{
			ent->v.spawnflags |= 1U << atoi( CMD_ARGV ( 3 ) );
			Ent_ClientPrintf( player, "spawnflags set to 0x%x\n", ent->v.spawnflags );
		}
		if( !stricmp( cmd, "clearspawnflag" ) )
		{
			ent->v.spawnflags &= ~( 1U << atoi( CMD_ARGV ( 3 ) ) );
			Ent_ClientPrintf( player, "spawnflags set to 0x%x\n", ent->v.flags );
		}

		if( single )
			break;
	}
}

/*
===============
Ent_Create_f

Create new entity with specified name.
===============
*/
void Ent_Create_f( edict_t *player )
{
	edict_t	*ent = NULL;
	int	i = 0;
	string_t classname;

	if( CMD_ARGC() < 2 )
	{
		Ent_ClientPrintf( player, "Use ent_create <classname> <key1> <value1> <key2> <value2> ...\n" );
		return;
	}

	if( !Ent_CheckCreate( player, CMD_ARGV(1) ) )
		return;


	classname = ALLOC_STRING( CMD_ARGV( 1 ) );

	if( !strcmp(CMD_ARGV( 1 ), "monstermaker" ) )
	{
		// apply keyvales
		for( i = 2; i < CMD_ARGC() - 1; i++ )
			if( !strcmp( CMD_ARGV(i), "monstertype") && !Ent_CheckCreate( player, CMD_ARGV( i + 1 ) ) )
				return;
	}

	ent = CREATE_NAMED_ENTITY( classname );

	if( !ent )
	{
		Ent_ClientPrintf( player, "Invalid entity!\n" );
		return;
	}

	// choose default origin
	UTIL_SetOrigin( &ent->v, player->v.origin + Vector( 100 * cos( player->v.angles[1]/180*M_PI ), 100 * sin( player->v.angles[1]/180*M_PI), 25 ));

	// apply keyvales
	for( i = 2; i < CMD_ARGC() - 1; i++ )
	{
		KeyValueData	pkvd;

		// allow split keyvalues to prespawn and postspawn
		if( !strcmp( CMD_ARGV( i ), "|" ) )
		{
			i++;
			break;
		}

		pkvd.fHandled = false;
		pkvd.szClassName = (char*)STRING( ent->v.classname );
		pkvd.szKeyName = (char*)CMD_ARGV( i );
		i++;
		pkvd.szValue = (char*)CMD_ARGV( i );

		DispatchKeyValue( ent, &pkvd );
		if( pkvd.fHandled )
			Ent_ClientPrintf( player, "value \"%s\" set to \"%s\"!\n", pkvd.szKeyName, pkvd.szValue );
	}


	// set default targetname
	if( !ent->v.targetname )
	{
		char newname[256], clientname[256];
		int j;

		for( j = 0; j < 32; j++ )
		{
			char c = tolower( (STRING( player->v.netname ))[j] );
			if( c < 'a' || c > 'z' )
				c = '_';
			if( !(STRING( player->v.netname ))[j] )
			{
				clientname[j] = 0;
				break;
			}
			clientname[j] = c;
		}

		// generate name based on nick name and index
		safe_snprintf( newname, sizeof( newname ),  "%s_%i_e%i", clientname, GETPLAYERUSERID( player ), ENTINDEX( ent ) );

		// i know, it may break strict aliasing rules
		// but we will not lose anything in this case.
		//Q_strnlwr( newname, newname, 256 );
		ent->v.targetname = ALLOC_STRING( newname );
		Ent_SendVars( player,  ent );
	}

	Ent_ClientPrintf( player, "Created %i: %s, targetname %s\n", ENTINDEX( ent ), CMD_ARGV( 1 ), STRING( ent->v.targetname ) );

	DispatchSpawn( ent );

	// now drop entity to floor.
	g_engfuncs.pfnDropToFloor( ent );

	// force think. Otherwise given weapon may crash server if player touch it before.
	DispatchThink( ent );
	g_engfuncs.pfnDropToFloor( ent );

	// apply postspawn keyvales

	for( i = i; i < CMD_ARGC() - 1; i++ )
	{
		KeyValueData	pkvd;

		pkvd.fHandled = false;
		pkvd.szClassName = (char*)STRING( ent->v.classname );
		pkvd.szKeyName = (char*)CMD_ARGV( i );
		i++;
		pkvd.szValue = (char*)CMD_ARGV( i );
		DispatchKeyValue( ent, &pkvd );
		if( pkvd.fHandled )
			Ent_ClientPrintf( player, "value \"%s\" set to \"%s\"!\n", pkvd.szKeyName, pkvd.szValue );
	}

	if( ent && ent->v.model && !Ent_CheckModel( STRING( ent->v.model ) ) )
	{
		Ent_ClientPrintf( player, "Bad model %s, removing entity\n", STRING( ent->v.model ) );
		ent->v.effects |= EF_NODRAW;
		ent->v.flags |= FL_KILLME;
		return;
	}

	// onwership
	CBaseEntity *entity = CBaseEntity::Instance( ent );
	if( entity )
	{
		const char *plid = GGM_GetPlayerID( player );
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( player );
		entity->enttools_data.enttools = 1;
		if( pPlayer && pPlayer->IsPlayer() && pPlayer->m_ggm.pState && pPlayer->m_ggm.pState->fRegistered )
			entity->enttools_data.enttools = 2;

		if( plid )
			strcpy( entity->enttools_data.ownerid, plid );
	}
}
typedef struct ucmd_s
{
	const char	*name;
	void		(*func)( edict_t *player );
} ucmd_t;

ucmd_t enttoolscmds[] =
{
{ "ent_list", Ent_List_f },
{ "ent_info", Ent_Info_f },
{ "ent_fire", Ent_Fire_f },
{ "ent_create", Ent_Create_f },
{ "ent_getvars", Ent_GetVars_f },
{ "ent_dump", Ent_Dump_f },

{ NULL, NULL }
};

bool Ent_ProcessClientCommand( edict_t *player )
{
	ucmd_t	*u;

	if( !mp_enttools_enable.value )
		return false;

	CBaseEntity *pl = CBaseEntity::Instance( player );

	if( !pl || !pl->IsPlayer() )
		return false;

	for( u = enttoolscmds; u->name; u++ )
	{
		if( !strcmp( CMD_ARGV( 0 ), u->name ))
		{
			ALERT( at_console, "enttools->%s(): %s\n", u->name, CMD_ARGS() );

			ALERT( at_logged, "\"%s<%i><%s><%s>\" performed: %s\n", STRING(player->v.netname),
						GETPLAYERUSERID(player), GETPLAYERAUTHID(player), g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( player ), "ip" ), CMD_ARGS() );

			if( u->func ) u->func( player );
			return true;
		}
	}

	return false;
}

cvar_t mp_enttools_enable = { "mp_enttools_enable", "0", FCVAR_SERVER };
cvar_t mp_enttools_maxfire = { "mp_enttools_maxfire", "5", FCVAR_SERVER };

void ENT_RegisterCVars( void )
{
	CVAR_REGISTER( &mp_enttools_enable );
	CVAR_REGISTER( &mp_enttools_maxfire );
	CVAR_REGISTER( &mp_enttools_lockmapentities );
	CVAR_REGISTER( &mp_enttools_checkowner );
	CVAR_REGISTER( &mp_enttools_players );
	g_engfuncs.pfnAddServerCommand( "mp_enttools_addblacklist", Ent_AddToBlacklist_f );
	g_engfuncs.pfnAddServerCommand( "mp_enttools_clearblacklist", Ent_ClearBlacklist_f );

}

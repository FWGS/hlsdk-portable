#ifndef ENTTOOLS_H
#define ENTTOOLS_H

extern cvar_t mp_enttools_maxfire;
extern cvar_t mp_enttools_enable;
bool Ent_ProcessClientCommand( edict_t *player );
void ENT_RegisterCVars( void );

struct EntoolsEntData {
	// 0 - not created by enttools
	// 1 - created by anonymous user
	// 2 - created by registered user
	char enttools;
	char ownerid[33];
};

#endif // ENTTOOLS_H


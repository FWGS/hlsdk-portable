#ifndef ENTTOOLS_H
#define ENTTOOLS_H

extern cvar_t mp_enttools_maxfire;
extern cvar_t mp_enttools_enable;
bool Ent_ProcessClientCommand( edict_t *player );
void ENT_RegisterCVars( void );

struct EntoolsEntData {
	bool enttools; // created by enttools
	char ownerid[32];
};

#endif // ENTTOOLS_H


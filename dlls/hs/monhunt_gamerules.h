//
// Monster_gamerules.h
//

class CMonsterplay : public CHalfLifeMultiplay
{
public:
	CMonsterplay();
	virtual void Think( void );
	virtual const char *GetGameDescription( void ) { return "Monster Hunt Mode"; }  // this is the game name that gets seen in the server browser
	virtual BOOL IsMonster( void );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );

	virtual BOOL FAllowMonsters( void );

	float fMonHuntNextSpawn; //Next Time to spawn monsters in spawn points if there is none. Set by a CVAR.
	int bMonsterMakers; //If there is no MonsterMaker ents
	int bMonsterCheck; //Check for MonsterMaker ents

	static int iKillforMonster( const char *classname );
	static const char *PrepareMonsterName( const char *monster_name );
};
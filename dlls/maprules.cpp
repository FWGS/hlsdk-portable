/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

//	-------------------------------------------
//
//	maprules.cpp
//
//	This module contains entities for implementing/changing game
//	rules dynamically within each map (.BSP)
//
//	-------------------------------------------

#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "gamerules.h"
//#include "maprules.h" //empty file
#include "cbase.h"
#include "player.h"
#include "shake.h"
#include "weapons.h"

class CRuleEntity : public CBaseEntity
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	SetMaster( int iszMaster ) { m_iszMaster = iszMaster; }

protected:
	BOOL	CanFireForActivator( CBaseEntity *pActivator );

private:
	string_t	m_iszMaster;
};

TYPEDESCRIPTION	CRuleEntity::m_SaveData[] = 
{
	DEFINE_FIELD( CRuleEntity, m_iszMaster, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE( CRuleEntity, CBaseEntity )

void CRuleEntity::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= EF_NODRAW;
}

void CRuleEntity::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "master" ) )
	{
		SetMaster( ALLOC_STRING( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

BOOL CRuleEntity::CanFireForActivator( CBaseEntity *pActivator )
{
	if( m_iszMaster )
	{
		if( UTIL_IsMasterTriggered( m_iszMaster, pActivator ) )
			return TRUE;
		else
			return FALSE;
	}
	
	return TRUE;
}

// 
// CRulePointEntity -- base class for all rule "point" entities (not brushes)
//
class CRulePointEntity : public CRuleEntity
{
public:
	void		Spawn( void );
};

void CRulePointEntity::Spawn( void )
{
	CRuleEntity::Spawn();
	pev->frame			= 0;
	pev->model			= 0;
}

// 
// CRuleBrushEntity -- base class for all rule "brush" entities (not brushes)
// Default behavior is to set up like a trigger, invisible, but keep the model for volume testing
//
class CRuleBrushEntity : public CRuleEntity
{
public:
	void		Spawn( void );
	int ObjectCaps() { return CRuleEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
};

void CRuleBrushEntity::Spawn( void )
{
	SET_MODEL( edict(), STRING(pev->model) );
	CRuleEntity::Spawn();
}

// CGameScore / game_score	-- award points to player / team 
//	Points +/- total
//	Flag: Allow negative scores					SF_SCORE_NEGATIVE
//	Flag: Award points to team in teamplay		SF_SCORE_TEAM

#define SF_SCORE_NEGATIVE			0x0001
#define SF_SCORE_TEAM				0x0002

class CGameScore : public CRulePointEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	inline	int		Points( void ) { return (int)pev->frags; }
	inline	BOOL	AllowNegativeScore( void ) { return pev->spawnflags & SF_SCORE_NEGATIVE; }
	inline	BOOL	AwardToTeam( void ) { return pev->spawnflags & SF_SCORE_TEAM; }

	inline	void	SetPoints( int points ) { pev->frags = points; }

private:
};

LINK_ENTITY_TO_CLASS( game_score, CGameScore )

void CGameScore::Spawn( void )
{
	CRulePointEntity::Spawn();
}

void CGameScore::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "points" ) )
	{
		SetPoints( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameScore::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	// Only players can use this
	if( pActivator && pActivator->IsPlayer() )
	{
		if( AwardToTeam() )
		{
			pActivator->AddPointsToTeam( Points(), AllowNegativeScore() );
		}
		else
		{
			pActivator->AddPoints( Points(), AllowNegativeScore() );
		}
	}
}

// CGameEnd / game_end	-- Ends the game in MP

class CGameEnd : public CRulePointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
private:
};

LINK_ENTITY_TO_CLASS( game_end, CGameEnd )

void CGameEnd::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
	if(pEntity)
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		if(pPlayer)
		{
			pPlayer->Clear_SayText();
			pPlayer->EnableControl(FALSE);
			pPlayer->m_trainning = 1;
			UTIL_ScreenFade( pPlayer, Vector(0,0,0), 4.0, 6.0, 255, FFADE_OUT );
			pPlayer->m_iClient_Gameover = -1;
			pPlayer->pev->health = 0;
			pPlayer->m_fGameOverTime = gpGlobals->time + 4;
		}
	}
	//g_pGameRules->EndMultiplayerGame();
}

//
// CGameText / game_text	-- NON-Localized HUD Message (use env_message to display a titles.txt message)
//	Flag: All players					SF_ENVTEXT_ALLPLAYERS
//

#define SF_ENVTEXT_ALLPLAYERS			0x0001

class CGameText : public CRulePointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	inline	BOOL	MessageToAll( void ) { return (pev->spawnflags & SF_ENVTEXT_ALLPLAYERS); }
	inline	void	MessageSet( const char *pMessage ) { pev->message = ALLOC_STRING(pMessage); }
	inline	const char *MessageGet( void )	{ return STRING(pev->message); }

private:
	int		m_iszSentence;		// string index for idle animation
	int		m_iszEntity;	// entity that is wanted for this sentence
	int		m_iszPlay;

	hudtextparms_t	m_textParms;
};

LINK_ENTITY_TO_CLASS( game_text, CGameText )
LINK_ENTITY_TO_CLASS( game_saytext, CGameText );

// Save parms as a block.  Will break save/restore if the structure changes, but this entity didn't ship with Half-Life, so
// it can't impact saved Half-Life games.
TYPEDESCRIPTION	CGameText::m_SaveData[] = 
{
	DEFINE_FIELD( CGameText, m_iszSentence, FIELD_STRING ),
	DEFINE_FIELD( CGameText, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CGameText, m_iszPlay, FIELD_STRING ),
	DEFINE_ARRAY( CGameText, m_textParms, FIELD_CHARACTER, sizeof(hudtextparms_t) ),
};

IMPLEMENT_SAVERESTORE( CGameText, CRulePointEntity )

void CGameText::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "channel" ) )
	{
		m_textParms.channel = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "x" ) )
	{
		m_textParms.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "y" ) )
	{
		m_textParms.y = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "effect" ) )
	{
		m_textParms.effect = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "color" ) )
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, pkvd->szValue );
		m_textParms.r1 = color[0];
		m_textParms.g1 = color[1];
		m_textParms.b1 = color[2];
		m_textParms.a1 = color[3];
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "color2" ) )
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, pkvd->szValue );
		m_textParms.r2 = color[0];
		m_textParms.g2 = color[1];
		m_textParms.b2 = color[2];
		m_textParms.a2 = color[3];
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "fadein" ) )
	{
		m_textParms.fadeinTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "fadeout" ) )
	{
		m_textParms.fadeoutTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		m_textParms.holdTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "fxtime" ) )
	{
		m_textParms.fxTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sentence"))
	{
		m_iszSentence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "entity"))
	{
		m_iszEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPlay"))
	{
		m_iszPlay = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameText::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if ( FClassnameIs( pev, "game_saytext" ) )
	{
		if(pev->frags == 1)
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )
			{
				CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, "legless_bar" );
				if ( pEntity2 ){
					if ( pEntity2->pev->deadflag == DEAD_NO )
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						if(pEnemyMonster)
						{
							pEnemyMonster->m_rpgms_inteam = 4;
						}
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
						pPlayer->m_skill_defguard = 2;
						pPlayer->TeamMate_Nagamatagi_Allclear(0);
						
						char text[256];
						sprintf( text, "- Kadoma Learning New Skill! (Defensive array)\n");
						UTIL_SayTextAll( text,this );
						sprintf( text, "- Aimed friendly units press T (Use / Cancel) to form a defensive formation.\n");
						UTIL_SayTextAll( text,this );
					}
				}
			}
		}
		else if(pev->frags == 2)
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
				pPlayer->m_music_save = 18;
				CLIENT_COMMAND(pPlayer->edict(), "cd loop 8\n");
				//SERVER_COMMAND("mp3 loop media/music23.mp3\n");
			}
		}
		else if(pev->frags == 3)
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_LARGEFUNNEL );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( g_sModelIndexFlareGlow );
				WRITE_SHORT( 1 );
				MESSAGE_END();

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/Flash3.wav", 1, 0);
				UTIL_ScreenFade( pPlayer, Vector(255,255,255), 1, 1, 255, FFADE_IN );

				pPlayer->pev->origin = pev->origin + Vector(0,0,36);
				pPlayer->pev->health = pPlayer->pev->max_health;
				pPlayer->TeamMate_Nagamatagi_RespawnStone(1);
				pPlayer->TeamMate_Nagamatagi_Teleport(4);

				UTIL_Remove( this );
			}
		}
		else if(pev->frags >= 4)
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
				if(m_textParms.holdTime > 0)
				{
					pPlayer->m_fNextClearTextTime = gpGlobals->time + m_textParms.holdTime;
				}
				else if(m_textParms.holdTime <= -1)
				{
					pPlayer->Clear_SayText();
					return;
				}
			}

			char text[256];
			if(pev->frags == 4)
			{
				sprintf( text, "- BOSS RUSH Mode\n");
				UTIL_SayTextAll( text,this );
				
				sprintf( text, "- Fully armed in Kadoma's strongest state, defeat nine bosses in a row.\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- (Saving and loading are prohibited)\n");
				UTIL_SayTextAll( text,this );

				return;
			}
			
			if(pev->frags == 5)
			{
				sprintf( text, "- HeadCrab Ball Mode\n");
				UTIL_SayTextAll( text,this );
		
				sprintf( text, "- Continuously knock the headcrab thrown by the enemy into the air.\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- (Saving and loading are prohibited)\n");
				UTIL_SayTextAll( text,this );
			
				return;
			}
		}
		else
		{
			CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, STRING(m_iszEntity) );
			if ( pEntity2 )
			{
				if ( pEntity2->pev->deadflag == DEAD_NO )
				{

					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
					if ( pEntity )
					{
					CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
						if(m_textParms.holdTime > 0)
						{
							pPlayer->m_fNextClearTextTime = gpGlobals->time + m_textParms.holdTime;
						}
						else if(m_textParms.holdTime <= -1)
						{
							pPlayer->Clear_SayText();
							return;
						}
					}
						
					char text[256];
					if ( FStrEq(STRING(pev->targetname), "legless_say1") )
					{
						sprintf( text, "Guard: Ugh, this is bad. I got shot, and lost a leg.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "legless_say2") )
					{
						sprintf( text, "Guard: Do you know who's behind this?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "legless_say3") )
					{
						sprintf( text, "Guard: Do you know how many of us have died?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "legless_say4") )
					{
						sprintf( text, "Guard: I can't go on; you must help me.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "legless_say7") )
					{
						sprintf( text, "Guard: I hope you do better than I did.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "scisaytext_1") )
					{
						sprintf( text, "Scientist: Hold on, I'll let you in.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "scisaytext_2") )
					{
						sprintf( text, "Scientist: Protect us, and we'll get you some valuable supplies.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "scisaytext_3") )
					{
						sprintf( text, "Scientist: We just want to get out alive.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "scisaytext_4") )
					{
						sprintf( text, "Scientist: We'll do anything!\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "scisaytext_5") )
					{
						sprintf( text, "Scientist: We'll stop our awful experiments.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "ecysci_saytext1") )
					{
						sprintf( text, "Scientist: Oh, I really do miss the 2D world.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "ecysci_saytext2") )
					{
						sprintf( text, "Scientist: You think the same?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "ecysci_saytext3") )
					{
						sprintf( text, "Scientist: Oh, come on.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "ecysci_saytext4") )
					{
						sprintf( text, "Scientist: I want to give my right brain a break.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "eat_shitfood") )
					{
						sprintf( text, "???: Mmmm yummy.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "medic_ass_sci_text1") )
					{
						sprintf( text, "Guard: What are you doing?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "medic_ass_sci_text2") )
					{
						sprintf( text, "Scientist: You look awful.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "medic_ass_sci_text3") )
					{
						sprintf( text, "Scientist: I think you need medical attention.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "medic_ass_sci_text4") )
					{
						sprintf( text, "Guard: Come on, old man!\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "medic_ass_sci_text5") )
					{
						sprintf( text, "Guard: Are you looking for trouble?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "medic_ass_sci_text6") )
					{
						sprintf( text, "Scientist: This might hurt a bit.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text1") )
					{
						sprintf( text, "Scientist: The antiproton spectrometer says 114514%.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text2") )
					{
						sprintf( text, "Scientist: This is the purest sample we've seen - this is once-in-a-lifetime.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text3") )
					{
						sprintf( text, "Scientist: Its power is greatly concentrated - very useful.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text4") )
					{
						sprintf( text, "Scientist: Try it!\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text5") )
					{
						sprintf( text, "Scientist: If anyone can end this, it will be what's left of the Lambda team on the other end.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text6") )
					{
						sprintf( text, "Scientist: It's hell over there. Lots of monsters.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text7") )
					{
						sprintf( text, "Scientist: It has a big gate, leading to another world.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text8") )
					{
						sprintf( text, "Scientist: You'll know it when you see it.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text9") )
					{
						sprintf( text, "Scientist: You must kill Gman and Doma before they get any stronger.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xen_power_sci_text10") )
					{
						sprintf( text, "Scientist: You can trust them. You can trust all of us. Good luck.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "bar_biomssay_text1") )
					{
						sprintf( text, "Guard: What is this all about?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "bar_biomssay_text2") )
					{
						sprintf( text, "Guard: You seen anything like this?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "eyelambdabar_saytext1") )
					{
						sprintf( text, "Guard: Hey, wait!\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "eyelambdabar_saytext2") )
					{
						sprintf( text, "Guard: If it sees you, you'll die.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "eyelambdabar_saytext3") )
					{
						sprintf( text, "Guard: Stay in the safe zone.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "lambda_sci_tripsay1") )
					{
						sprintf( text, "Scientist: Are you sure someone will rescue us?\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "lambda_sci_tripsay2") )
					{
						sprintf( text, "Scientist: I don't know.\n");
					}
					else if ( FStrEq(STRING(pev->targetname), "xenworld_telp_sbtn") )
					{
						sprintf( text, "Scientist: We have total confidence in you.\n");
					}

					if (text[0] != '\0')
					{
						UTIL_SayTextAll( text,this );
					}
					else if (!FStringNull(pev->message))
					{
						UTIL_SayTextAll( MessageGet(),this );
					}

					if (!FStringNull(m_iszPlay) || !FStringNull(m_iszSentence))
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity2->MyMonsterPointer();
						if(pEnemyMonster)
						{
							if (!FStringNull(m_iszPlay))
							{
								pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( STRING(m_iszPlay) );
								pEnemyMonster->ResetSequenceInfo( );
								pEnemyMonster->pev->frame = 0;
								pEnemyMonster->SetState( MONSTERSTATE_HUNT );
								pEnemyMonster->pev->angles = pev->angles;
							}
							EMIT_SOUND_DYN( pEnemyMonster->edict(), CHAN_VOICE, STRING(m_iszSentence), VOL_NORM, 0, 0, PITCH_NORM );
						}
					}
				}
			}
		}

		UTIL_Remove( this );
	}
	else
	{
		if( MessageToAll() )
		{
			UTIL_HudMessageAll( m_textParms, MessageGet() );
		}
		else
		{
			if( pActivator && pActivator->IsNetClient() )
			{
				UTIL_HudMessage( pActivator, m_textParms, MessageGet() );
			}
		}
	}
}

//
// CGameTeamMaster / game_team_master	-- "Masters" like multisource, but based on the team of the activator
// Only allows mastered entity to fire if the team matches my team
//
// team index (pulled from server team list "mp_teamlist"
// Flag: Remove on Fire
// Flag: Any team until set?		-- Any team can use this until the team is set (otherwise no teams can use it)
//

#define SF_TEAMMASTER_FIREONCE			0x0001
#define SF_TEAMMASTER_ANYTEAM			0x0002

class CGameTeamMaster : public CRulePointEntity
{
public:
	void		KeyValue( KeyValueData *pkvd );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int			ObjectCaps( void ) { return CRulePointEntity:: ObjectCaps() | FCAP_MASTER; }

	BOOL		IsTriggered( CBaseEntity *pActivator );
	const char	*TeamID( void );
	inline BOOL RemoveOnFire( void ) { return (pev->spawnflags & SF_TEAMMASTER_FIREONCE) ? TRUE : FALSE; }
	inline BOOL AnyTeam( void ) { return (pev->spawnflags & SF_TEAMMASTER_ANYTEAM) ? TRUE : FALSE; }

private:
	BOOL		TeamMatch( CBaseEntity *pActivator );

	int			m_teamIndex;
	USE_TYPE	triggerType;
};

LINK_ENTITY_TO_CLASS( game_team_master, CGameTeamMaster )

void CGameTeamMaster::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "teamindex" ) )
	{
		m_teamIndex = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue( pkvd );
}

void CGameTeamMaster::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( useType == USE_SET )
	{
		if( value < 0 )
		{
			m_teamIndex = -1;
		}
		else
		{
			m_teamIndex = g_pGameRules->GetTeamIndex( pActivator->TeamID() );
		}
		return;
	}

	if( TeamMatch( pActivator ) )
	{
		SUB_UseTargets( pActivator, triggerType, value );
		if( RemoveOnFire() )
			UTIL_Remove( this );
	}
}

BOOL CGameTeamMaster::IsTriggered( CBaseEntity *pActivator )
{
	return TeamMatch( pActivator );
}

const char *CGameTeamMaster::TeamID( void )
{
	if( m_teamIndex < 0 )		// Currently set to "no team"
		return "";

	return g_pGameRules->GetIndexedTeamName( m_teamIndex );		// UNDONE: Fill this in with the team from the "teamlist"
}

BOOL CGameTeamMaster::TeamMatch( CBaseEntity *pActivator )
{
	if( m_teamIndex < 0 && AnyTeam() )
		return TRUE;

	if( !pActivator )
		return FALSE;

	return UTIL_TeamsMatch( pActivator->TeamID(), TeamID() );
}

//
// CGameTeamSet / game_team_set	-- Changes the team of the entity it targets to the activator's team
// Flag: Fire once
// Flag: Clear team				-- Sets the team to "NONE" instead of activator

#define SF_TEAMSET_FIREONCE			0x0001
#define SF_TEAMSET_CLEARTEAM		0x0002

class CGameTeamSet : public CRulePointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return (pev->spawnflags & SF_TEAMSET_FIREONCE) ? TRUE : FALSE; }
	inline BOOL ShouldClearTeam( void ) { return (pev->spawnflags & SF_TEAMSET_CLEARTEAM) ? TRUE : FALSE; }

private:
};

LINK_ENTITY_TO_CLASS( game_team_set, CGameTeamSet )

void CGameTeamSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( ShouldClearTeam() )
	{
		SUB_UseTargets( pActivator, USE_SET, -1 );
	}
	else
	{
		SUB_UseTargets( pActivator, USE_SET, 0 );
	}

	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGamePlayerZone / game_player_zone -- players in the zone fire my target when I'm fired
//
// Needs master?
class CGamePlayerZone : public CRuleBrushEntity
{
public:
	void		KeyValue( KeyValueData *pkvd );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	string_t	m_iszInTarget;
	string_t	m_iszOutTarget;
	string_t	m_iszInCount;
	string_t	m_iszOutCount;
};

LINK_ENTITY_TO_CLASS( game_zone_player, CGamePlayerZone )
TYPEDESCRIPTION	CGamePlayerZone::m_SaveData[] =
{
	DEFINE_FIELD( CGamePlayerZone, m_iszInTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszInCount, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutCount, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CGamePlayerZone, CRuleBrushEntity )

void CGamePlayerZone::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "intarget" ) )
	{
		m_iszInTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "outtarget" ) )
	{
		m_iszOutTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "incount" ) )
	{
		m_iszInCount = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "outcount" ) )
	{
		m_iszOutCount = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CRuleBrushEntity::KeyValue( pkvd );
}

void CGamePlayerZone::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int playersInCount = 0;
	int playersOutCount = 0;

	if( !CanFireForActivator( pActivator ) )
		return;

	CBaseEntity *pPlayer = NULL;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			TraceResult trace;
			int			hullNumber;

			hullNumber = human_hull;
			if( pPlayer->pev->flags & FL_DUCKING )
			{
				hullNumber = head_hull;
			}

			UTIL_TraceModel( pPlayer->pev->origin, pPlayer->pev->origin, hullNumber, edict(), &trace );

			if( trace.fStartSolid )
			{
				playersInCount++;
				if( m_iszInTarget )
				{
					FireTargets( STRING( m_iszInTarget ), pPlayer, pActivator, useType, value );
				}
			}
			else
			{
				playersOutCount++;
				if( m_iszOutTarget )
				{
					FireTargets( STRING( m_iszOutTarget ), pPlayer, pActivator, useType, value );
				}
			}
		}
	}

	if( m_iszInCount )
	{
		FireTargets( STRING( m_iszInCount ), pActivator, this, USE_SET, playersInCount );
	}

	if( m_iszOutCount )
	{
		FireTargets( STRING( m_iszOutCount ), pActivator, this, USE_SET, playersOutCount );
	}
}

//
// CGamePlayerHurt / game_player_hurt	-- Damages the player who fires it
// Flag: Fire once

#define SF_PKILL_FIREONCE			0x0001

class CGamePlayerHurt : public CRulePointEntity
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return (pev->spawnflags & SF_PKILL_FIREONCE) ? TRUE : FALSE; }

private:
};

LINK_ENTITY_TO_CLASS( game_player_hurt, CGamePlayerHurt )

void CGamePlayerHurt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( pActivator->IsPlayer() )
	{
		if(pev->impulse == 1)
		{
			if(pev->origin.z < pActivator->pev->origin.z)
				return;
		}
		if(pev->impulse == 2)
		{
			pActivator->pev->origin.z = pActivator->pev->origin.z + 768;
			UTIL_Remove( this );
			return;
		}

		if( pev->dmg < 0 )
			pActivator->TakeHealth( -pev->dmg, DMG_GENERIC );
		else
			pActivator->TakeDamage( pev, pev, pev->dmg, DMG_GENERIC );
	}

	SUB_UseTargets( pActivator, useType, value );

	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGameCounter / game_counter	-- Counts events and fires target
// Flag: Fire once
// Flag: Reset on Fire

#define SF_GAMECOUNT_FIREONCE			0x0001
#define SF_GAMECOUNT_RESET				0x0002

class CGameCounter : public CRulePointEntity
{
public:
	void		Spawn( void );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return (pev->spawnflags & SF_GAMECOUNT_FIREONCE) ? TRUE : FALSE; }
	inline BOOL ResetOnFire( void ) { return (pev->spawnflags & SF_GAMECOUNT_RESET) ? TRUE : FALSE; }

	inline void CountUp( void ) { pev->frags++; }
	inline void CountDown( void ) { pev->frags--; }
	inline void ResetCount( void ) { pev->frags = pev->dmg; }
	inline int CountValue( void ) { return (int)pev->frags; }
	inline int LimitValue( void ) { return (int)pev->health; }

	inline BOOL HitLimit( void ) { return CountValue() == LimitValue(); }

private:

	inline void SetCountValue( int value ) { pev->frags = value; }
	inline void SetInitialValue( int value ) { pev->dmg = value; }
};

LINK_ENTITY_TO_CLASS( game_counter, CGameCounter )

void CGameCounter::Spawn( void )
{
	// Save off the initial count
	SetInitialValue( CountValue() );
	CRulePointEntity::Spawn();
}

void CGameCounter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	switch( useType )
	{
	case USE_ON:
	case USE_TOGGLE:
		CountUp();
		break;
	case USE_OFF:
		CountDown();
		break;
	case USE_SET:
		SetCountValue( (int)value );
		break;
	}

	if( HitLimit() )
	{
		SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
		if( RemoveOnFire() )
		{
			UTIL_Remove( this );
		}

		if( ResetOnFire() )
		{
			ResetCount();
		}
	}
}

//
// CGameCounterSet / game_counter_set	-- Sets the counter's value
// Flag: Fire once

#define SF_GAMECOUNTSET_FIREONCE			0x0001

class CGameCounterSet : public CRulePointEntity
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline BOOL RemoveOnFire( void ) { return (pev->spawnflags & SF_GAMECOUNTSET_FIREONCE) ? TRUE : FALSE; }

private:
};

LINK_ENTITY_TO_CLASS( game_counter_set, CGameCounterSet )

void CGameCounterSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	SUB_UseTargets( pActivator, USE_SET, pev->frags );

	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

//
// CGamePlayerEquip / game_playerequip	-- Sets the default player equipment
// Flag: USE Only

#define SF_PLAYEREQUIP_USEONLY			0x0001
#define MAX_EQUIP		32

class CGamePlayerEquip : public CRulePointEntity
{
public:
	void		KeyValue( KeyValueData *pkvd );
	void		Touch( CBaseEntity *pOther );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	inline BOOL	UseOnly( void ) { return (pev->spawnflags & SF_PLAYEREQUIP_USEONLY) ? TRUE : FALSE; }

private:
	void		EquipPlayer( CBaseEntity *pPlayer );

	string_t	m_weaponNames[MAX_EQUIP];
	int			m_weaponCount[MAX_EQUIP];
};

LINK_ENTITY_TO_CLASS( game_player_equip, CGamePlayerEquip )

void CGamePlayerEquip::KeyValue( KeyValueData *pkvd )
{
	CRulePointEntity::KeyValue( pkvd );

	if( !pkvd->fHandled )
	{
		for( int i = 0; i < MAX_EQUIP; i++ )
		{
			if( !m_weaponNames[i] )
			{
				char tmp[128];

				UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ));

				m_weaponNames[i] = ALLOC_STRING( tmp );
				m_weaponCount[i] = atoi( pkvd->szValue );
				m_weaponCount[i] = Q_max( 1, m_weaponCount[i] );
				pkvd->fHandled = TRUE;
				break;
			}
		}
	}
}

void CGamePlayerEquip::Touch( CBaseEntity *pOther )
{
	if( !CanFireForActivator( pOther ) )
		return;

	if( UseOnly() )
		return;

	EquipPlayer( pOther );
}

void CGamePlayerEquip::EquipPlayer( CBaseEntity *pEntity )
{
	CBasePlayer *pPlayer = NULL;

	if( pEntity->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pEntity;
	}

	if( !pPlayer )
		return;

	for( int i = 0; i < MAX_EQUIP; i++ )
	{
		if( !m_weaponNames[i] )
			break;

		if( g_pGameRules->IsBustingGame() && !strcmp( STRING( m_weaponNames[i] ), "weapon_egon" ))
			continue;

		for( int j = 0; j < m_weaponCount[i]; j++ )
		{
 			pPlayer->GiveNamedItem( STRING( m_weaponNames[i] ) );
		}
	}
}

void CGamePlayerEquip::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	EquipPlayer( pActivator );
}

//
// CGamePlayerTeam / game_player_team	-- Changes the team of the player who fired it
// Flag: Fire once
// Flag: Kill Player
// Flag: Gib Player

#define SF_PTEAM_FIREONCE			0x0001
#define SF_PTEAM_KILL    			0x0002
#define SF_PTEAM_GIB     			0x0004

class CGamePlayerTeam : public CRulePointEntity
{
public:
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
	inline BOOL RemoveOnFire( void ) { return (pev->spawnflags & SF_PTEAM_FIREONCE) ? TRUE : FALSE; }
	inline BOOL ShouldKillPlayer( void ) { return (pev->spawnflags & SF_PTEAM_KILL) ? TRUE : FALSE; }
	inline BOOL ShouldGibPlayer( void ) { return (pev->spawnflags & SF_PTEAM_GIB) ? TRUE : FALSE; }
	
	const char *TargetTeamName( const char *pszTargetName );
};

LINK_ENTITY_TO_CLASS( game_player_team, CGamePlayerTeam )

const char *CGamePlayerTeam::TargetTeamName( const char *pszTargetName )
{
	CBaseEntity *pTeamEntity = NULL;

	while( ( pTeamEntity = UTIL_FindEntityByTargetname( pTeamEntity, pszTargetName ) ) != NULL )
	{
		if( FClassnameIs( pTeamEntity->pev, "game_team_master" ) )
			return pTeamEntity->TeamID();
	}

	return NULL;
}

void CGamePlayerTeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !CanFireForActivator( pActivator ) )
		return;

	if( pActivator->IsPlayer() )
	{
		const char *pszTargetTeam = TargetTeamName( STRING(pev->target) );
		if ( pszTargetTeam )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			g_pGameRules->ChangePlayerTeam( pPlayer, pszTargetTeam, ShouldKillPlayer(), ShouldGibPlayer() );
		}
	}
	
	if( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}

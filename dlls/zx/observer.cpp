//=============================================================================
// observer.cpp
//
#include        "extdll.h"
#include        "util.h"
#include        "cbase.h"
#include        "player.h"
#include        "weapons.h"

extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgTeamInfo;
extern int gmsgSpectator;

// Spieler wurde zum Spectator, erstmal alles einrichten
// Dies wurde von der player.cpp hierher verschoben.
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
        // Alle Client-Side-Entities, die dem Spieler
        // zugewiesen wurden, abschalten
        MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
                WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
                WRITE_BYTE( (BYTE)entindex() );
        MESSAGE_END();

        // Alle Waffen wegstecken
        if (m_pActiveItem)
                m_pActiveItem->Holster( );

        if ( m_pTank != NULL )
        {
                m_pTank->Use( this, this, USE_OFF, 0 );
                m_pTank = NULL;
        }

        // Message Cache des Anzugs löschen,
        // damit wir nicht weiterlabern ;-)
        SetSuitUpdate(NULL, FALSE, 0);

        // Mitteilung an Ammo-HUD, dass der Spieler tot ist
        MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
                WRITE_BYTE(0);
                WRITE_BYTE(0XFF);
                WRITE_BYTE(0xFF);
        MESSAGE_END();

        // Zoom zurücksetzen
        m_iFOV = m_iClientFOV = 0;
        pev->fov = m_iFOV;
        MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
                WRITE_BYTE(0);
        MESSAGE_END();

        // Ein paar Flags setzen
        m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
        m_afPhysicsFlags |= PFLAG_OBSERVER;
        pev->effects = EF_NODRAW;
        pev->view_ofs = g_vecZero;
        pev->angles = pev->v_angle = vecViewAngle;
        pev->fixangle = TRUE;
        pev->solid = SOLID_NOT;
        pev->takedamage = DAMAGE_NO;
        pev->movetype = MOVETYPE_NONE;
        ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
        ClearBits( pev->flags, FL_DUCKING );
        pev->deadflag = DEAD_RESPAWNABLE;
        pev->health = 1;

        // Status-Bar ausschalten
        m_fInitHUD = TRUE;

        // Team Status updaten
        pev->team = 0;
        MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
                WRITE_BYTE( ENTINDEX(edict()) );
                WRITE_STRING( "" );
        MESSAGE_END();

        // Dem Spieler alles wegnehmen
        RemoveAllItems( FALSE );

        // Einen Spieler finden, den wir angucken
        m_flNextObserverInput = 0;
        Observer_SetMode(OBS_CHASE_LOCKED);

        // Allen Clients mitteilen, dass der Spieler jetzt Spectator ist
        MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
                WRITE_BYTE( ENTINDEX( edict() ) );
                WRITE_BYTE( 1 );
        MESSAGE_END();
}

// Observermodus verlassen
void CBasePlayer::StopObserver( void )
{
        // Spectator-Modus abschalten
        if ( pev->iuser1 || pev->iuser2 )
        {
                // Allen Clients mitteilen, dass der Spieler kein Spectator
                // mehr ist.
                MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
                        WRITE_BYTE( ENTINDEX( edict() ) );
                        WRITE_BYTE( 0 );
                MESSAGE_END();

                pev->iuser1 = pev->iuser2 = 0;
                m_iHideHUD &= ~(HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
                m_hObserverTarget = NULL;
        }
}

// Den nächsten Client finden, den der Spieler anschaut
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{
        // MOD AUTOREN: Hier die Logik ändern, wenn ihr den Observer nur
        //              bestimmte Spieler beobachten lassen wollt, beispielsweise
        //                nur Teammitglieder.

        int                iStart;
        if ( m_hObserverTarget )
                iStart = ENTINDEX( m_hObserverTarget->edict() );
        else
                iStart = ENTINDEX( edict() );
        int            iCurrent = iStart;
        m_hObserverTarget = NULL;
        int iDir = bReverse ? -1 : 1;

        do
        {
                iCurrent += iDir;

                // Durch alle Clients loopen
                if (iCurrent > gpGlobals->maxClients)
                        iCurrent = 1;
                if (iCurrent < 1)
                        iCurrent = gpGlobals->maxClients;

                CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
                if ( !pEnt )
                        continue;
                if ( pEnt == this )
                        continue;
                // Keine unsichtbaren Spieler oder andere Observer beobachten
                if ( ((CBasePlayer*)pEnt)->IsObserver() || (pEnt->pev->effects & EF_NODRAW) )
                        continue;

                // MOD AUTOREN: Hier die Überprüfungen einfügen

                m_hObserverTarget = pEnt;
                break;

        } while ( iCurrent != iStart );

        // Ziel gefunden?
        if ( m_hObserverTarget )
        {
                // Ziel in pev speichern damit die Bewegungs-DLL dran kommt
                pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

                ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
        }
        else
        {
                ALERT( at_console, "No observer targets.\n" );
        }
}

// Tastatur-Eingaben für den Observermodus...
void CBasePlayer::Observer_HandleButtons()
{
        // Mouse-Clicks verlangsamen
        if ( m_flNextObserverInput > gpGlobals->time )
                return;

        // Springen wechselt zwischen den Modi
        if ( m_afButtonPressed & IN_JUMP )
        {
                if ( pev->iuser1 == OBS_ROAMING )
                        Observer_SetMode( OBS_CHASE_LOCKED );
                else if ( pev->iuser1 == OBS_CHASE_LOCKED )
                        Observer_SetMode( OBS_CHASE_FREE );
                else
                        Observer_SetMode( OBS_ROAMING );

                m_flNextObserverInput = gpGlobals->time + 0.2;
        }

        // Attack wechselt zum nächsten Spieler
        if ( m_afButtonPressed & IN_ATTACK && pev->iuser1 != OBS_ROAMING )
        {
                Observer_FindNextPlayer( false );

                m_flNextObserverInput = gpGlobals->time + 0.2;
        }

        // Attack2 wechselt zum vorherigen Spieler
        if ( m_afButtonPressed & IN_ATTACK2 && pev->iuser1 != OBS_ROAMING )
        {
                Observer_FindNextPlayer( true );

                m_flNextObserverInput = gpGlobals->time + 0.2;
        }
}

// Versuche, Observer-Modus zu wechseln
void CBasePlayer::Observer_SetMode( int iMode )
{
        // Abbrechen, wenn wir bereits im gewünschten Modus sind
        if ( iMode == pev->iuser1 )
                return;

        // Wechsle zu Roaming?
        if ( iMode == OBS_ROAMING )
        {
                // MOD AUTOREN: Wenn ihr kein Roaming-Modus in eurem Mod wollt,
                //              dann brecht hier einfach ab
                pev->iuser1 = OBS_ROAMING;
                pev->iuser2 = 0;

                ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode3" );
                pev->maxspeed = 320;
                return;
        }

        // Wechsle zu ChaseLock?
        if ( iMode == OBS_CHASE_LOCKED )
        {
                // Sicherstellen, dass wir ein Ziel haben
                if ( m_hObserverTarget == NULL )
                        Observer_FindNextPlayer( false );

                if (m_hObserverTarget)
                {
                        pev->iuser1 = OBS_CHASE_LOCKED;
                        pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
                        pev->maxspeed = 0;
                }
                else
                {
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
                        Observer_SetMode(OBS_ROAMING);
                }

                return;
        }

        // Wechsle zu ChaseFree?
        if ( iMode == OBS_CHASE_FREE )
        {
                // Sicherstellen, dass wir ein Ziel haben
                if ( m_hObserverTarget == NULL )
                        Observer_FindNextPlayer( false );

                if (m_hObserverTarget)
                {
                        pev->iuser1 = OBS_CHASE_FREE;
                        pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode2" );
                        pev->maxspeed = 0;
                }
                else
                {
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
                        Observer_SetMode(OBS_ROAMING);
                }

                return;
        }
}
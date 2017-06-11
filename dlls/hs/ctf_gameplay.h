// Status of the flag
#define FLAG_STOLEN 1
#define FLAG_CAPTURE    2
#define FLAG_DROPPED    3

// Our flag class which holds all our functions and variables we're going to use

class CObjectFlag : public CBaseEntity
{
public:
    void Spawn( );
    void Precache( );
    void Touch(CBaseEntity *);
    bool m_fIsInPlay;
};

// The Flag entity that is dropped by a player when killed/disconnected/whatnot
class CDroppedFlag : public CBaseEntity
{
public:
    void Spawn( );
    void Precache( );
    void Touch(CBaseEntity *);
};

// Capture point for Team 1
class CCaptureTeam1 : public CBaseEntity
{
public:
   
       void Spawn( );
       void Precache( );
       void EXPORT Touch(CBaseEntity *);
       void EXPORT Think( );
       void KeyValue( KeyValueData* );
};


// Capture point for Team 2
class CCaptureTeam2 : public CBaseEntity
{
public:
   
       void Spawn( );
       void Precache( );
       void EXPORT Touch( CBaseEntity *);
       void EXPORT Think( );
       void KeyValue( KeyValueData* );
};
class CAmbientMP3 : public CPointEntity
{
public:
        static CAmbientMP3 *AmbientMP3Create( const char *pszSound );
        void Spawn( void );
        void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

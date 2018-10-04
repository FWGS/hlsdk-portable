#ifndef _INC_BUMPLIGHT_H
#define _INC_BUMPLIGHT_H

#define BUMPLIGHT_SPAWNFLAG_START_OFF (1<<0)

class CBumpLight : public CPointEntity
{
public:
	CBumpLight(void);

	virtual void	KeyValue( KeyValueData* pkvd ); 
	virtual void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void EXPORT CreateOnClient(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float m_fStrength, m_fRadius;
	Vector m_vColour;
	bool m_bEnabled;
	char m_szMovewithEnt[64];
	bool m_bMovewith;
	int m_iStyle;
};

#endif
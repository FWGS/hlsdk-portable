//-------------------------------------------------
//-												---
//-			radiomsg.h							---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- header de la radio du hud			-----------
//-------------------------------------------------


#ifndef RADIOMSG_H
#define RADIOMSG_H


#define	GLASSES		0
#define EINSTEIN	1
#define LUTHER		2
#define SLICK		3
#define BARNEY		4
#define GMAN		5
#define VOCAL		6
#define INFOCOMBI	7
#define INCONNU		8



class CRadiomsg : public CPointEntity
{
public:
	void	Spawn		( void );
	void	Precache	( void );

	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	int		m_iszSentence;
	int		m_iszMessage;
	int		m_iszText;
	int		m_iHead;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

};


int	GetRadiomsgText ( int iszMessage );



#endif	//RADIOMSG_H

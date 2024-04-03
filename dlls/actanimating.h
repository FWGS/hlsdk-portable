
class CActAnimating : public CBaseAnimating
{
public:
	void			SetActivity( Activity act );
	inline Activity	GetActivity( void ) { return m_Activity; }
	void			SetSequence( int sequence );
	inline int		GetSequence( void ) { return m_iSequence; }

	virtual int	ObjectCaps( void ) { return CBaseAnimating :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	Activity	m_Activity;
	int			m_iSequence;
};


#ifndef CLIMBINGMONSTER_H
#define CLIMBINGMONSTER_H

class CClimbingMonster : public CBaseMonster
{
public:
	float			ChangeYaw( int speed );
	virtual BOOL	FGetNodeRoute ( Vector vecDest );
	virtual BOOL	BuildRoute ( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget );
	BOOL			FRefreshRoute( void );
	BOOL			ShouldAdvanceRoute( float flWaypointDist );
	virtual int		CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );// check validity of a straight move through space
	void			Move( float flInterval = 0.1 );
	virtual void	MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void			Stop( void );
	void			Killed( entvars_t *pevAttacker, int iGib );

protected:
	BOOL		mbIsClimbing;

	int			mOldType;

	Vector		m_vecTravel;		// Current direction
	Vector		m_velocity;
	float		m_climbSpeed;		// Current climb speed
	float		m_stopTime;			// Last time we stopped (to avoid switching states too soon)
};


#endif



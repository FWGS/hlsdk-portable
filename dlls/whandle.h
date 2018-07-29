#ifndef WHANDLE_H
#define WHANDLE_H

#ifndef CLIENT_DLL

class CBasePlayerItem;

class EHBasePlayerItem : public EHANDLE
{
public:
	operator CBasePlayerItem *()
	{
		return (CBasePlayerItem *)GET_PRIVATE( Get() ); 
	}
	CBasePlayerItem *operator ->()
	{
		return (CBasePlayerItem *)GET_PRIVATE( Get() ); 
	}
	template <class T>
	operator T()
	{
		return (T)GET_PRIVATE( Get() ); 
	}
	template <class T>
	T *operator = ( T *pEntity )
	{
		edict_t *e = NULL;
		if( pEntity )
		e = pEntity->edict();
		return (T*)CBaseEntity::Instance( Set ( e ) );
	}

	// handle = NULL correctly
	int operator = ( int null1 )
	{
		//assert( !null1 );
		Set(0);
		return 0;
	}
	operator int ()
	{
		return Get() != NULL;
	}

	bool operator !=(EHBasePlayerItem &other)
	{
		return Get() != other.Get();
	}

	bool operator !=(my_nullptr_t &null)
	{
		return Get() != (edict_t*)0;
	}
};
#else
#define EHBasePlayerItem CBasePlayerItem*
#endif

#endif

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

	bool operator !=(EHBasePlayerItem &other)
	{
		return Get() != other.Get();
	}
#if 0
	bool operator !=(my_nullptr_t &null1)
	{
		return Get() != (edict_t*)0;
	}
#endif
	bool operator !=(int null1)
	{
		return Get() != (edict_t*)null1;
	}
};
#else
#define EHBasePlayerItem CBasePlayerItem*
#endif

#endif

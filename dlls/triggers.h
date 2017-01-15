
#ifndef TRIGGERS_H
#define TRIGGERS_H

//=====================================
// CBaseTrigger
//=====================================
class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch(CBaseEntity *pOther);
	void KeyValue(KeyValueData *pkvd);
	void EXPORT MultiTouch(CBaseEntity *pOther);
	void EXPORT HurtTouch(CBaseEntity *pOther);
	void EXPORT CDAudioTouch(CBaseEntity *pOther);
	void ActivateMultiTrigger(CBaseEntity *pActivator);
	void EXPORT MultiWaitOver(void);
	void EXPORT CounterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void InitTrigger(void);

	virtual int	ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn(void);
};

class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn(void);
};

#endif // TRIGGERS_H
#include "extdll.h" 
#include "util.h" 
#include "cbase.h" 
#include "monsters.h" 
#include "weapons.h" 
#include "nodes.h" 
#include "player.h"

LINK_ENTITY_TO_CLASS( weapon_example, CExampleGun );

void CExampleGun::Spawn( ) 
{ 
pev->classname = MAKE_STRING("weapon_example"); // здесь имя энтити которое надо будет затем вписать в фгд 
Precache( ); 
SET_MODEL(ENT(pev), "models/w_9mmAR.mdl"); // Тут должна быть модель вашего оружия 
m_iId = WEAPON_EXAMPLE;

m_iDefaultAmmo = 50;
FallInit(); 
}

void CExampleGun::Precache( void ) { // Тут вы прекешите звуки и модели для вашего оружия PRECACHE_MODEL("models/v_9mmAR.mdl"); PRECACHE_MODEL("models/w_9mmAR.mdl"); PRECACHE_MODEL("models/p_9mmAR.mdl");

PRECACHE_SOUND("items/9mmclip1.wav");

PRECACHE_SOUND("items/clipinsert1.wav"); PRECACHE_SOUND("items/cliprelease1.wav");

PRECACHE_SOUND ("weapons/hks1.wav"); PRECACHE_SOUND ("weapons/hks2.wav"); PRECACHE_SOUND ("weapons/hks3.wav");

// this is to hook your client-side event m_usExampleFire = PRECACHE_EVENT( 1, "events/example.sc" ); }

int CExampleGun::GetItemInfo(ItemInfo *p) { p->pszName = STRING(pev->classname); p->pszAmmo1 = "9mm"; p->iMaxAmmo1 = 200; p->iMaxClip = -1; p->iSlot = 0; // слот в худе ( помните также, что елси здесь вы написали например 2 то в игре будет занят 3 слот, т.к в коде исчесление начинаеться от нуля) p->iPosition = 1; // позиция в слоте ( то же самое что и в слоте - елси вам надо занять 4 позицию , то пишите 3 в коде) p->iFlags = 0; p->iId = m_iId = WEAPON_EXAMPLE; p->iWeight = MP5_WEIGHT; //p->pszAmmo2 = NULL; //p->iMaxAmmo2 = -1;

return 1; }

int CExampleGun::AddToPlayer( CBasePlayer *pPlayer ) { if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) ) { MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev ); WRITE_BYTE( m_iId ); MESSAGE_END(); return TRUE; } return FALSE; }

С
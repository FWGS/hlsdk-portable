/****************************************************************
*																*
*				nvg.cpp											*
*																*
*				par Julien										*
*																*
****************************************************************/

// code de la vision infrarouge


//inclusions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"



//------------------------------------
//
// rafraichissement de l'affichage


int CHudNVG :: Draw	( float flTime )
{
	float delta = flTime - lasttime;
	lasttime = flTime;

	frame += delta * 10;	// 10fps

	while ( (int)frame > 4 )
		frame -= 5;

	nombre = 0;

	wrect_t rec;
	rec.left = rec.top = 0;
	rec.right = rec.bottom = 256;

	int x = (int)(ScreenWidth / 256) + 1;
	int y = (int)(ScreenHeight / 256) + 1;

	SPR_Set( m_sprNVG, 175, 175, 175);

	for ( int i = 0; i < x; i ++ )
	{
		for ( int j = 0; j < y; j ++ )
			SPR_DrawAdditive( Frame(), 256 * i, 256 * j, &rec );	
	}

	// viseur

	rec = CreateWrect (0,0,256,256);

	x = (int)(ScreenWidth / 2) - 128;
	y = (int)(ScreenHeight / 2) - 128;

	SPR_Set( m_sprEnnemy, 0, 200, 0);
	SPR_DrawAdditive( 0, x, y, &rec );	


	return 1;
}


//------------------------------------
//
// num

int CHudNVG :: Frame( void )
{
/*	int returnframe = (int)frame + nombre;

	while ( returnframe > 4 )
		returnframe -= 5;

	nombre ++;

	return returnframe;
	*/
	return (int)frame;
}




//------------------------------------
//
// d
// gmsgSwitchNVG
// gmsgInfosNVG

DECLARE_MESSAGE(m_NVG, SwitchNVG );
DECLARE_MESSAGE(m_NVG, InfosNVG );


//------------------------------------
//
// initialisation au chargement de la dll

int CHudNVG :: Init( void )
{
	m_sprNVG	= SPR_Load("sprites/nvg.spr");
	m_sprEnnemy	= SPR_Load("sprites/irgunviseur.spr");	// pour pas recompiler le hud.h :)
	lasttime = frame = 0;

	ClearEnnemies ();

	HOOK_MESSAGE( SwitchNVG );
	HOOK_MESSAGE( InfosNVG );

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudNVG :: VidInit( void )
{
	m_sprNVG	= SPR_Load("sprites/nvg.spr");
	m_sprEnnemy	= SPR_Load("sprites/irgunviseur.spr");	// pour pas recompiler le hud.h :)
	lasttime = frame = 0;

	ClearEnnemies ();

	m_iFlags &= ~HUD_ACTIVE;
	return 1;
}


//------------------------------------
//
// gestion des messages serveur


int CHudNVG::MsgFunc_SwitchNVG( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iOnOff = READ_BYTE();

	if ( iOnOff == 1 )
		m_iFlags |= HUD_ACTIVE;

	if ( iOnOff == 0 )
	{
		m_iFlags &= ~HUD_ACTIVE;
		ClearEnnemies ();
	}

	return 1;
}

int CHudNVG::MsgFunc_InfosNVG ( const char *pszName, int iSize, void *pbuf )
{
	ClearEnnemies ();

	BEGIN_READ( pbuf, iSize );
	int nument = READ_BYTE();

	if ( nument == 0 )
		return 1;

	for ( int i = 0; i < nument; i++ )
	{
		// lecture

		int entindex = READ_BYTE();
		vec3_t color;
		color.x = READ_COORD() / 255;
		color.y = READ_COORD() / 255;
		color.z = READ_COORD() / 255;

		// enregistrement

		nvg_ennemy_t *p = NULL;
		p = new nvg_ennemy_t;
		p->index = entindex;
		p->color = color;

		if ( pEnnemy )
			p->pNext = pEnnemy;
		else
			p->pNext = NULL;

		pEnnemy = p;
	}

	return 1;
}


//------------------------------------
//
// destruction du registre des
// entit

void CHudNVG :: ClearEnnemies ( void )
{
	nvg_ennemy_t *p;

	while ( pEnnemy )
	{
		p = pEnnemy;
		pEnnemy = p->pNext;
		delete p;
	}
}

//------------------------------------
// cherche si l'entit

nvg_ennemy_t *CHudNVG :: IsEnnemy ( int index )
{
	if ( pEnnemy == NULL )
		return NULL;

	nvg_ennemy_t *p = pEnnemy;

	do
	{
		if ( p->index == index )
			return p;

		p = p->pNext;
	}
	while ( p != NULL );

	// sort de la boucle : aucune entit

	return NULL;
}

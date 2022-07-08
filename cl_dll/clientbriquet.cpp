//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					clientlbriquet.cpp					---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code serveur du briquet								---
//---------------------------------------------------------
//---------------------------------------------------------



//---------------------------------------------------------
//inclusions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "cl_entity.h"
#include "eventscripts.h"

extern vec3_t v_angles;			// angles de la vue du joueur


#define FLAMME_SPR				"sprites/briquet.spr"
#define	FLAMME_X_SCALE			1
#define	FLAMME_Y_SCALE			3
#define	SPR_FRAMES				4
#define	SPR_FRAMERATE			10



//------------------------------------
//
// d
// gmsgLFlammes

DECLARE_MESSAGE(m_Briquet, Briquet );


//------------------------------------
//
// gestion des messages serveur


int CHudBriquet::MsgFunc_Briquet( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iOnOff	= READ_BYTE();

	if ( iOnOff == 1 )
	{	// allume
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_iFlags &= ~HUD_ACTIVE;
	}

	return 1;
}




//------------------------------------
//
// rafraichissement de l'affichage


int CHudBriquet :: Draw	( float flTime )
{
	return 1;
}


void CHudBriquet :: DrawFlamme ( void )
{
	// hud 
	if ( ! (m_iFlags & HUD_ACTIVE))
		return;

	// ptr pour le viewmodel
	cl_entity_t *pViewmodel = GetViewEntity();
	if ( !pViewmodel )
		return;

	// vectors

	vec3_t v_right, v_up;
	AngleVectors ( v_angles, Vector(0,0,0), v_right, v_up );

	vec3_t vecPos = pViewmodel->attachment[0];
	vec3_t right = v_right * FLAMME_X_SCALE;
	vec3_t up = v_up * FLAMME_Y_SCALE;
	vec3_t vertex;

	// initialisation triapi

	int frame = (int)(gEngfuncs.GetClientTime()*SPR_FRAMERATE) % SPR_FRAMES;

	int modelindex;
	struct model_s *mod = gEngfuncs.CL_LoadModel( FLAMME_SPR , &modelindex );
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, frame );	

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling
	gEngfuncs.pTriAPI->Brightness( 0.8 );						
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//d

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );						//premier vertex
		vertex = vecPos - right + up;
		gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );						//deuxieme vertex
		vertex = vecPos + right + up;
		gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );						//troisieme vertex
		vertex = vecPos + right;
		gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );					//quatrieme vertex
		vertex = vecPos - right;
		gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

	gEngfuncs.pTriAPI->End();									//fin du trac
}



//------------------------------------
//
// initialisation au chargement de la dll

int CHudBriquet :: Init( void )
{
	HOOK_MESSAGE( Briquet );

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudBriquet :: VidInit( void )
{
	HOOK_MESSAGE( Briquet );

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


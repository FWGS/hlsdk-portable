//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					clientlflammes.cpp					---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code serveur du lance flammes						---
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

extern vec3_t v_angles, v_origin;			// angles de la vue du joueur


#define FLAMME_LIBRE			0
#define	FLAMME_ATTACHEE			1
#define	DETRUIT_FLAMME			2
#define	FLAMME_DECO				3
#define	FLAMME_DEAD				4

#define FLAMME_FLAMME			0
#define FLAMME_FUMEE			1
#define FLAMME_DYING			2

#define FLAMMES_SPRITE			"sprites/lflammes02.spr"
#define FRAMERATE				11

#define MAXFRAMES				33
#define MAXFRAMES_LOOP			21

#define SPRITE_START_WIDTH		4
#define SPRITEFXAMOUNT			180

#define MAX_ROTSPEED			100

#define SPRITE_FADE_TIME		2

#define FLAMME_DECO_WIDTH		(SPRITE_START_WIDTH * 28)/2	
#define SMOKERATE				0.1
#define FLAMME_DECO_LIFETIME	0.8
#define FLAMME_DECO_FRAMERATE	15

#define FUMEE_SPRITE			"sprites/tank_smokeball.spr"
#define FUMEE_MAXFRAMES			7

#define DEAD_LIFETIME			5
#define DEAD_SMOKERATE			0.2


//------------------------------------
//
// déclaration du message :
// gmsgLFlammes

DECLARE_MESSAGE(m_LFlammes, LFlammes );


//------------------------------------
//
// gestion des messages serveur


int CHudLFlammes::MsgFunc_LFlammes( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int mode	= READ_BYTE();

	if ( mode == FLAMME_LIBRE || mode == FLAMME_ATTACHEE )
	{
		int idx		= (int)READ_COORD();
		float time	= READ_COORD();
		
		// nouvelle flamme

		flammes_t *p = NULL;
		p = new flammes_t;

		// paramètres

		p->index = idx;
		p->flBirthTime = time;

		p->bXdir = gEngfuncs.pfnRandomLong(0,1) ? true : false;
		p->bYdir = gEngfuncs.pfnRandomLong(0,1) ? true : false;

		p->angle = gEngfuncs.pfnRandomFloat ( 0,360 );
		p->rotspeed = gEngfuncs.pfnRandomFloat ( -MAX_ROTSPEED, MAX_ROTSPEED );

		p->imode = mode;

		p->offset.x = READ_COORD();
		p->offset.y = READ_COORD();
		p->offset.z = READ_COORD();

		p->flag = FLAMME_FLAMME;

		// pointeur

		p->pNext = m_pFlammes;
		m_pFlammes = p;
	}

	else if ( mode  == DETRUIT_FLAMME )
	{
		int idx		= (int)READ_COORD();

		// destruction d'une flamme

		flammes_t *p = NULL;
		flammes_t *q = NULL;
		p = m_pFlammes;

		while ( p != NULL )
		{
			if ( p->index == idx && p->imode != FLAMME_DECO )
			{
				if ( q == NULL )
				{
					// première de la liste

					m_pFlammes = p->pNext,

					delete p;
					break;
				}
				else
				{
					q->pNext = p->pNext;
					delete p;

					break;
				}
			}

			q = p;
			p = p->pNext;
		}
	}

	else if ( mode  == FLAMME_DEAD )
	{
		int idx		= (int)READ_COORD();
		
		flammes_t *p = NULL;
		p = m_pFlammes;

		while ( p != NULL )
		{
			if (p->index == idx && p->imode == FLAMME_ATTACHEE )
			{
				p->flag = FLAMME_DYING;
				p->flBirthTime = gEngfuncs.GetClientTime();
				
				cl_entity_t *ent = gEngfuncs.GetEntityByIndex(gEngfuncs.GetEntityByIndex( p->index )->curstate.aiment);
				p->offset = ent->origin;
			}

			p = p->pNext;
		}
	}


	return 1;
}



//------------------------------------
//
// destruction du registre des flammes

void CHudLFlammes :: ClearFlammes ( void )
{
	while ( m_pFlammes != NULL )
	{
		flammes_t *p = NULL;
		p = m_pFlammes;

		m_pFlammes = m_pFlammes->pNext;
		delete p;
	}
}


//------------------------------------
//
// rafraichissement de l'affichage


int CHudLFlammes :: Draw	( float flTime )
{
	m_iFlags &= ~HUD_ACTIVE;
	return 1;
}


void CHudLFlammes :: DrawFlammes ( void )
{

	flammes_t *p = NULL;
	p = m_pFlammes;

	vec3_t v_forward, v_right, v_up;

	// initialisation triapi

	int modelindex;
	struct model_s *mod = gEngfuncs.CL_LoadModel( FUMEE_SPRITE , &modelindex );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );					//mode de transparence
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling


	// premier cycle pour la fumee
	while ( p != NULL )
	{	
		if ( p->flag != FLAMME_FUMEE )
		{
			p = p->pNext;
			continue;
		}


		float lifetime = gEngfuncs.GetClientTime()-p->flBirthTime; 
		vec3_t vertex, vecPos;
		
		// position de la flamme

		vecPos = p->offset;


		// taille du sprite

		float fldist;
		
		fldist = FLAMME_DECO_WIDTH;

		fldist *= 0.5;

		
		// vecteurs de base
		
		AngleVectors ( v_angles + Vector(0.0f, 0.0f, p->angle ), v_forward, v_right, v_up );

		vec3_t right = v_right * fldist * (p->bXdir==true?1:-1);
		vec3_t up = v_up * fldist * (p->bYdir==true?1:-1);

		p->angle += p->rotspeed * gHUD.m_flTimeDelta;


		// frame
		
		int frame;

		frame = (int)(lifetime*FLAMME_DECO_FRAMERATE) % FUMEE_MAXFRAMES;

		// transparence

		float flTrans;
		
		flTrans = 100 * (1- lifetime / FLAMME_DECO_LIFETIME) + 80;

		flTrans = flTrans / (float)255;


		// tracé

		gEngfuncs.pTriAPI->Brightness( flTrans );						
		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, flTrans );
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, frame );	
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//démarrage du tracé, en mode quads - carrés .

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );						//premier vertex
			vertex = vecPos - right + up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );						//deuxieme vertex
			vertex = vecPos + right + up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );						//troisieme vertex
			vertex = vecPos + right - up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );					//quatrieme vertex
			vertex = vecPos - right - up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

		gEngfuncs.pTriAPI->End();									//fin du tracé


		p = p->pNext;
	}



	// second cycle pour les flammes

	
	mod = gEngfuncs.CL_LoadModel( FLAMMES_SPRITE , &modelindex );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence


	p = m_pFlammes;

	while ( p != NULL )
	{		
		float lifetime = gEngfuncs.GetClientTime()-p->flBirthTime; 
		vec3_t vertex, vecPos;


		// destruction des flammes trop anciennes

		if ( (p->imode == FLAMME_DECO && lifetime > FLAMME_DECO_LIFETIME) /*|| (p->imode == FLAMME_ATTACHEE && p->flag == FLAMME_DYING && lifetime > DEAD_LIFETIME)*/ )
		{
			flammes_t *r = NULL;
			flammes_t *q = NULL;
			r = m_pFlammes;

			while ( r != NULL )
			{
				if ( r == p )
				{
					if ( q == NULL )
					{
						// première de la liste

						m_pFlammes = p->pNext,

						delete p;
						break;
					}
					else
					{
						q->pNext = p->pNext;
						delete p;

						break;
					}
				}

				q = r;
				r = r->pNext;
			}
		}

		// application des vecteurs vitesse

		if ( p->imode == FLAMME_DECO )
		{
			p->offset = p->offset + p->velocity * gHUD.m_flTimeDelta;
		}


		// position de la flamme

		AngleVectors ( v_angles + Vector(0.0f, 0.0f, p->angle ), Vector(0,0,0), v_right, v_up );

		if ( p->imode == FLAMME_LIBRE )
			vecPos = gEngfuncs.GetEntityByIndex( p->index )->origin;

		else if ( p->imode == FLAMME_ATTACHEE && p->flag == FLAMME_DYING )
		{
			vecPos = p->offset;
		}
		else if ( p->imode == FLAMME_ATTACHEE )
		{
			// movetype follow

			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(gEngfuncs.GetEntityByIndex( p->index )->curstate.aiment);
			vecPos = ent->origin + p->offset;
		}

		else if ( p->imode == FLAMME_DECO )
			vecPos = p->offset;




		// apparition des flammes deco

		if ( p->imode == FLAMME_ATTACHEE/* && p->flag != FLAMME_ATTACHDEAD*/ )
		{

			int t1 = lifetime / SMOKERATE;
			int t2 = (lifetime-gHUD.m_flTimeDelta) / SMOKERATE;

			if ( t2 < t1 )
			{

				for ( int i=0; i<2; i++ )
				{
					// génération d'une flamme

					flammes_t *Q = NULL;
					Q = new flammes_t;

					// paramètres

					Q->index = p->index;
					Q->flBirthTime = gEngfuncs.GetClientTime ();

					Q->bXdir = gEngfuncs.pfnRandomLong(0,1) ? true : false;
					Q->bYdir = gEngfuncs.pfnRandomLong(0,1) ? true : false;

					Q->angle = gEngfuncs.pfnRandomFloat ( 0,360 );
					Q->rotspeed = gEngfuncs.pfnRandomFloat ( -MAX_ROTSPEED, MAX_ROTSPEED );

					Q->imode	= FLAMME_DECO;

					if ( p->flag == FLAMME_DYING )
					{
						if ( gEngfuncs.pfnRandomLong( 1, 5 ) == 1 )
							Q->flag = FLAMME_FLAMME;
						else
							Q->flag = FLAMME_FUMEE;
					}
					else
					{
						if ( i == 0 )
							Q->flag = FLAMME_FLAMME;
						else
							Q->flag = FLAMME_FUMEE;
					}

					// physique

 					Q->offset = vecPos;
					Q->velocity = Vector ( gEngfuncs.pfnRandomFloat(-15,15), gEngfuncs.pfnRandomFloat(-15,15), 90.0f );

					// pointeur

				/*	if ( Q->offset == p->offset || Q->offset == vec3_t(0,0,0) || pent == NULL )
					{
						delete Q;
					}
					else*/
					{
						Q->pNext = m_pFlammes;
						m_pFlammes = Q;
					}
				}
			}
		}


		// stop pour la fumee
		if ( p->flag == FLAMME_FUMEE )
		{
			p = p->pNext;
			continue;
		}


		// taille du sprite

		float fldist;
		
		if ( p->imode == FLAMME_LIBRE )
		{
			// y = 0.0837 e (7.712 x )

			fldist = lifetime < 0.725 ?
				SPRITE_START_WIDTH * ( 1 + 0.0837 * exp( 7.712 * lifetime )) * 32/24 :
				SPRITE_START_WIDTH * 28;
		}
		else if ( p->imode == FLAMME_ATTACHEE )
		{
			fldist = SPRITE_START_WIDTH * 14;
		}
		else if ( p->imode == FLAMME_DECO )
		{
			fldist = FLAMME_DECO_WIDTH;
		}

		fldist *= 0.5;

		
		// vecteurs de base
		
		vec3_t right = v_right * fldist * (p->bXdir==true?1:-1);
		vec3_t up = v_up * fldist * (p->bYdir==true?1:-1);

		p->angle += p->rotspeed * gHUD.m_flTimeDelta;


		// frame
		
		int frame;

		if ( p->imode == FLAMME_LIBRE )
		{
			frame = (int)(lifetime*FRAMERATE);
			frame = frame >= MAXFRAMES ? MAXFRAMES-1 : frame;
		}

		else if ( p->imode == FLAMME_ATTACHEE )
			frame = (int)(lifetime*FRAMERATE) % MAXFRAMES_LOOP;

		else if ( p->imode == FLAMME_DECO )
			frame = (int)(lifetime*FLAMME_DECO_FRAMERATE) + (MAXFRAMES/FLAMME_DECO_FRAMERATE - FLAMME_DECO_LIFETIME)*FLAMME_DECO_FRAMERATE - 1;

		// transparence

		float flTrans;
		float flratio;

		if ( p->imode == FLAMME_ATTACHEE )
		{
			flTrans = 180;
		}

		else if ( p->imode == FLAMME_LIBRE )
		{
			if ( lifetime <= SPRITE_FADE_TIME )
				flTrans = 180;
			else
			{
				flratio = (float)(1-(lifetime-(float)SPRITE_FADE_TIME)/(MAXFRAMES/FRAMERATE-SPRITE_FADE_TIME));
				flTrans = 180.0;
				flTrans *= flratio;
			}
		}

		else if ( p->imode == FLAMME_DECO )
		{
			flTrans = 100 * (1- lifetime / FLAMME_DECO_LIFETIME) + 80;
		}

		flTrans = flTrans / (float)255;


		// tracé

		gEngfuncs.pTriAPI->Brightness( flTrans );						
		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, flTrans );
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, frame );	
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//démarrage du tracé, en mode quads - carrés .

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );						//premier vertex
			vertex = vecPos - right + up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );						//deuxieme vertex
			vertex = vecPos + right + up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );						//troisieme vertex
			vertex = vecPos + right - up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );					//quatrieme vertex
			vertex = vecPos - right - up;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

		gEngfuncs.pTriAPI->End();									//fin du tracé




		p = p->pNext;
	}

}



//------------------------------------
//
// initialisation au chargement de la dll

int CHudLFlammes :: Init( void )
{
	ClearFlammes ();

	HOOK_MESSAGE( LFlammes );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation après le chargement


int CHudLFlammes :: VidInit( void )
{
	ClearFlammes ();

	HOOK_MESSAGE( LFlammes );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


/****************************************************************
*																*
*				particules.cpp									*
*																*
*				par Julien										*
*																*
****************************************************************/

// code du syst
// et des decals



#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include <string.h>
#include "cdll_int.h"
#include "pmtrace.h"
#include "event_api.h"
#include "pm_defs.h"
#include "parsemsg.h"
#include "eventscripts.h"
#include "r_efx.h"

extern void EV_HLDM_EjectParticules ( Vector vecOrigin, Vector vecNormal, char chTextureType, int decal, int iParticules );
void VectorAngles( const float *forward, float *angles );

extern vec3_t v_angles;



#define PARTICULES_LARGEUR		2
#define DECALS_LARGEUR			4




//========================
// ajout de particule
//

void CHudParticules :: AddParticule ( vec3_t origin, vec3_t angles, vec3_t velocity, vec3_t avelocity, char model [64], int deathtime, int flags )
{

	int iIndex;

	for ( iIndex = 0 ; iIndex < MAX_PARTICULES ; iIndex ++ )
	{
		if ( m_pParticules [ iIndex ].libre == 1 )
			break;
	}


	// array rempli
	if ( m_pParticules [ iIndex ].libre != 1 )
	{
//		gEngfuncs.Con_Printf ( "client.dll : Particule overflow :  > %i\n", MAX_PARTICULES );	//alertatconsole
		return;
	}

	
	//remplissage du nouveau tableau de donnees

	m_pParticules [ iIndex ].origin			= origin;
	m_pParticules [ iIndex ].angles			= angles;
	m_pParticules [ iIndex ].velocity		= velocity;
	m_pParticules [ iIndex ].avelocity		= avelocity;
	m_pParticules [ iIndex ].deathtime		= deathtime;
	m_pParticules [ iIndex ].flags			= flags;
	m_pParticules [ iIndex ].libre			= 0;

	strcpy ( m_pParticules [ iIndex ].model, model );

}



//=========================
// ajout de decal
//

void	CHudParticules :: AddDecal ( vec3_t origin, vec3_t angles, char model [64], float deathtime, int flags )
{

	// recherche de place libre
	int iIndex;
	int iIndexNew = 0;

	for ( iIndex = 0 ; iIndex < MAX_DECALS ; iIndex ++ )
	{
		if ( m_pDecals [ iIndex ].libre == 1 )
			break;
	}

	// place libre
	if ( m_pDecals [ iIndex ].libre == 1 )
	{
		// demande acceptee
		iIndexNew = iIndex;
	}
	// tableau rempli
	else
	{

		float age = m_pDecals[0].deathtime;
		int oldindex = 0;

		// cherche le plus ancien
		for ( iIndex = 0; iIndex < MAX_DECALS ; iIndex ++ )
		{
			if ( m_pDecals[iIndex].deathtime < age )
			{
				age = m_pDecals[iIndex].deathtime;
				oldindex = iIndex;
			}
		}

		// detruit le plus ancient
		RemoveDecal ( oldindex );

		// prend la place nouvellement libre
		// demande acceptee
		iIndexNew = oldindex;


		// actualise l age minimum
		age = m_pDecals[0].deathtime;

		for ( iIndex = 0; iIndex < MAX_DECALS ; iIndex ++ )
		{
			if ( m_pDecals[iIndex].deathtime < age )
				age = m_pDecals[iIndex].deathtime;
		}

	}

	// enregistre 

	m_pDecals[iIndexNew].angles		= angles;
	m_pDecals[iIndexNew].deathtime	= deathtime;
	m_pDecals[iIndexNew].flags		= flags;
	m_pDecals[iIndexNew].libre		= 0;
	m_pDecals[iIndexNew].origin		= origin;

	strcpy ( m_pDecals [ iIndex ].model, model );

	// electro-rocket

	if ( flags & ( FLAG_DECAL_DISK | flags & FLAG_DECAL_SG | FLAG_DECAL_XENO ) )
	{
		m_pDecals[iIndexNew].iRndSensX	= m_pDecals[iIndexNew].iRndSensY = 1;
		return;
	}



	// fixe 

	Vector forward, right, up;
	AngleVectors ( m_pDecals[iIndexNew].angles, forward, right, up );
	pmtrace_t pmtrace;
	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( m_pDecals[iIndexNew].origin - forward.Normalize()*0.2, m_pDecals[iIndexNew].origin, PM_STUDIO_BOX | PM_STUDIO_IGNORE, -1, &pmtrace );					

	physent_s *pe = gEngfuncs.pEventAPI->EV_GetPhysent( pmtrace.ent );
	int entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( &pmtrace );

	if ( pmtrace.fraction == 0.0 && pe && pe->solid == SOLID_BSP && entity != 0 )
	{

		// contre un mur et contre une entit

		m_pDecals[iIndexNew].entity = entity;
		m_pDecals[iIndexNew].offset = m_pDecals[iIndexNew].origin - pe->origin;
		m_pDecals[iIndexNew].angoffset = pe->angles;
		m_pDecals[iIndexNew].oldangles = m_pDecals[iIndexNew].angles;

		cl_entity_s *pClEnt = GetEntity( entity );
	
		// pas de d
		if ( pe->rendermode != kRenderNormal && pClEnt->curstate.renderamt == 0 )
		{
			RemoveDecal ( iIndexNew );
			return;
		}

		if ( pe->rendermode == kRenderTransTexture )
		{
			//entit

			if ( pClEnt->curstate.renderamt == 0 && m_pDecals[iIndexNew].flags & FLAG_DECAL_GLASS )
			{
				RemoveDecal ( iIndexNew );
				return;
			}

			// entit

			else if ( m_pDecals[iIndexNew].flags & FLAG_DECAL_GLASS && gEngfuncs.pfnRandomLong (0,3) != 0 )
			{
				m_pDecals[iIndexNew].flags |= FLAG_DECAL_WIDEGLASS;
			}
		}

		// grille
		else if ( pe->rendermode == kRenderTransAlpha )
		{
			RemoveDecal ( iIndexNew );
			return;
		}


	}
	else
	{
		m_pDecals[iIndexNew].entity = 0;
		m_pDecals[iIndexNew].offset = m_pDecals[iIndexNew].angoffset = Vector(0,0,0);
	}


	// taille al

	float iRnd = gEngfuncs.pfnRandomFloat( 0.75, 1 );

	if ( m_pDecals[iIndexNew].flags & FLAG_DECAL_WIDEGLASS )
		iRnd *= 6;

	else if ( m_pDecals[iIndexNew].flags & FLAG_DECAL_GLASS )
		iRnd *= 4;

	m_pDecals[iIndexNew].iRndSensX	= m_pDecals[iIndexNew].iRndSensY = iRnd;

	iRnd = gEngfuncs.pfnRandomLong( 0, 1 );

	m_pDecals[iIndexNew].iRndSensX	= iRnd == 1 ? - m_pDecals[iIndexNew].iRndSensX : m_pDecals[iIndexNew].iRndSensX;
	m_pDecals[iIndexNew].iRndSensY	= iRnd == 1 ? - m_pDecals[iIndexNew].iRndSensY : m_pDecals[iIndexNew].iRndSensY;

	
	// sens du crowbar

	if ( m_pDecals[iIndexNew].flags & FLAG_DECAL_CROWBAR_INVERT )
		m_pDecals[iIndexNew].iRndSensX	= - m_pDecals[iIndexNew].iRndSensX;

	// A FAIRE : enlever les d
	
	// demande acceptee
	return;

}




//===========================================
// application des lois physiques

int CHudParticules :: Draw	( float flTime )
{

	static const Vector gravity (0,0,-400);

	float delta = flTime - m_flLastTime;
	m_flLastTime = flTime; 

	if ( delta == 0 )
		return 1;

	for ( int iIndex = 0; iIndex < MAX_PARTICULES ; iIndex ++ )
	{
		// fin de la liste
		if ( m_pParticules [ iIndex ].libre == 1 )
			continue;

		vec3_t lastpos = m_pParticules [ iIndex ].origin;

		if ( m_pParticules [ iIndex ].deathtime < flTime )
		{
			RemoveParticule ( iIndex );
			continue;
		}

		

		m_pParticules[iIndex].angles = m_pParticules[iIndex].angles + m_pParticules[iIndex].avelocity * delta;
		
		m_pParticules[iIndex].origin =  m_pParticules[iIndex].origin + m_pParticules[iIndex].velocity * delta;

		if ( !(m_pParticules[iIndex].flags & ( FLAG_PARTICULE_OUTRO1 | FLAG_PARTICULE_OUTRO2 | FLAG_PARTICULE_OUTRO3 | FLAG_PARTICULE_SMOKE )) )
		{
			m_pParticules[iIndex].velocity = m_pParticules[iIndex].velocity + gravity * delta;
		}

		if ( m_pParticules[iIndex].flags & FLAG_PARTICULE_OUTRO2  )
		{
			if ( m_pParticules [ iIndex ].deathtime - flTime < 4.5 )
			{
				m_pParticules[iIndex].velocity = m_pParticules[iIndex].velocity.Normalize() * Q_max( 0, m_pParticules[iIndex].velocity.Length() - 10 * delta );
			}
			else if ( m_pParticules [ iIndex ].deathtime - flTime < 6)
			{
				int i = ( m_pParticules[iIndex].angles.x - 1.2 ) / 0.3;
				m_pParticules[iIndex].velocity = m_pParticules[iIndex].velocity.Normalize() * 14 * ( 0.3 - i*0.03 ) * 2.5;
			}
		}

		if ( m_pParticules[iIndex].flags & ( FLAG_PARTICULE_OUTRO1 | FLAG_PARTICULE_OUTRO2 | FLAG_PARTICULE_OUTRO3 ) )
		{
			if ( m_pParticules [ iIndex ].deathtime - flTime < 2.5 )
			{
				m_pParticules [ iIndex ].angles.y -=  delta * 0.3 / 2.5;

				m_pParticules [ iIndex ].angles.y = Q_max ( 0, m_pParticules [ iIndex ].angles.y );
			}
		}


		
		pmtrace_t pmtrace;
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( lastpos, m_pParticules [ iIndex ].origin, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &pmtrace );					

		if ( pmtrace.fraction != 1 )
			m_pParticules [ iIndex ].deathtime = gEngfuncs.GetClientTime();
	}



	for ( int iDecal = 0; iDecal < MAX_DECALS ; iDecal ++ )
	{
		// innocupp
		if ( m_pDecals[iDecal].libre == 1 )
			continue;

		if ( m_pDecals[iDecal].deathtime < flTime )
		{
			RemoveDecal ( iDecal );
			return 1;
		}

		if ( m_pDecals[iDecal].flags & FLAG_DECAL_DISK )
		{
			if ( m_pDecals[iDecal].iRndSensX < ELECTRO_DISK_MAX )
			{
				m_pDecals[iDecal].iRndSensX	+= ELECTRO_DISK_SPEED * delta;
				m_pDecals[iDecal].iRndSensY = m_pDecals[iDecal].iRndSensX;
			}
			continue;
		}

		if ( m_pDecals[iDecal].flags & FLAG_DECAL_SG )
		{
			if ( m_pDecals[iDecal].iRndSensX < 8 )
			{
				m_pDecals[iDecal].iRndSensX	+= 16 * delta;
				m_pDecals[iDecal].iRndSensY = m_pDecals[iDecal].iRndSensX = Q_min ( 8, m_pDecals[iDecal].iRndSensX );
			}
			continue;
		}

		if ( m_pDecals[iDecal].flags & FLAG_DECAL_XENO )
		{
			if ( m_pDecals[iDecal].iRndSensX < 3 )
			{
				m_pDecals[iDecal].iRndSensX	+= 3 * delta;
				m_pDecals[iDecal].iRndSensY = m_pDecals[iDecal].iRndSensX = Q_min ( 3, m_pDecals[iDecal].iRndSensX );
			}
			continue;
		}


		Vector forward, right, up;
		AngleVectors ( m_pDecals[iDecal].angles, forward, right, up );

		pmtrace_t pmtrace;
		
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( m_pDecals[iDecal].origin - forward.Normalize()*0.35, m_pDecals[iDecal].origin, PM_STUDIO_BOX | PM_STUDIO_IGNORE, -1, &pmtrace );					

		physent_s *pe = gEngfuncs.pEventAPI->EV_GetPhysent( pmtrace.ent );
		int entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( &pmtrace );

		// sprite anim

		if ( m_pDecals[iDecal].flags & FLAG_DECAL_WIDEGLASS )
		{
			// 9.0 fps  - - 5 frames max

			m_pDecals[iDecal].flFrame += delta * 9;
			m_pDecals[iDecal].flFrame = m_pDecals[iDecal].flFrame > 4 ? 4 : m_pDecals[iDecal].flFrame;
		}




		if ( m_pDecals[iDecal].entity != 0  )
		{

			//suit le d

			cl_entity_t *pClEnt = GetEntity( m_pDecals[iDecal].entity );

			if ( pClEnt == 0 )
			{
				m_pDecals[iDecal].entity = 0;
			}

			else if ( m_pDecals[iDecal].angoffset != pClEnt->angles )
			{
				// entit

				float flLength = m_pDecals[iDecal].offset.Length();

				vec3_t angDiff, angNewAngles;
				VectorAngles( m_pDecals[iDecal].offset.Normalize(), angDiff );
				
				angNewAngles = pClEnt->angles + ( angDiff - m_pDecals[iDecal].angoffset );

				vec3_t forward, right, up;
				angNewAngles.x += 180;
				angNewAngles.y += 180;

				AngleVectors ( angNewAngles, forward, right, up );

				m_pDecals[iDecal].origin = pClEnt->origin + forward.Normalize() * flLength;

				m_pDecals[iDecal].angles = pClEnt->angles + ( m_pDecals[iDecal].oldangles - m_pDecals[iDecal].angoffset );

			}

			else if ( ( ( pmtrace.startsolid == 1 && m_pDecals[iDecal].entity != 0 ) ||
					    ( pmtrace.fraction == 0.0 && m_pDecals[iDecal].entity != 0 )  )
					&&  ( m_pDecals[iDecal].origin - pClEnt->origin != m_pDecals[iDecal].offset ) )
			{				
				// porte coulissante
				m_pDecals[iDecal].origin = pClEnt->origin + m_pDecals[iDecal].offset;
			}

			else if ( pmtrace.fraction != 0.0 )
			{
				// d
				RemoveDecal ( iDecal );
			}


		}
		else if ( pmtrace.fraction != 0.0 )
		{
			// d
			RemoveDecal ( iDecal );
		}




	}





	return 1;
}


//=================================
// suppression d une particule
 
void CHudParticules :: RemoveParticule ( int iIndex )
{
	m_pParticules[iIndex].libre = 1;
	m_pParticules[iIndex].angles = m_pParticules[iIndex].origin = m_pParticules[iIndex].avelocity = m_pParticules[iIndex].velocity = Vector ( 0,0,0 );
	m_pParticules[iIndex].deathtime = m_pParticules[iIndex].flags = 0;
	strcpy ( m_pParticules[iIndex].model, "" );
}

// suppression d'un d

void CHudParticules :: RemoveDecal ( int iIndex )
{
	m_pDecals[iIndex].libre = 1;
	m_pDecals[iIndex].angles = m_pDecals[iIndex].origin = m_pDecals[iIndex].offset = Vector ( 0,0,0 );
	m_pDecals[iIndex].deathtime = 0;
	m_pDecals[iIndex].flags = 0;
	m_pDecals[iIndex].entity = 0;
//	strcpy ( m_pDecals[iIndex].model, " " );
}




//=====================================
//
// trac


void CHudParticules :: DrawAll	( void )
{

	//particules

	int iIndex, iresult = 0;

	for ( iIndex = 0; iIndex < MAX_PARTICULES ; iIndex ++ )
	{
		// fin de la liste
		if ( m_pParticules [ iIndex ].libre == 1 )
			continue;

		iresult ++;
			

		// vecteurs de base
		
		vec3_t forward, right, up, vertex, ang;

		if ( m_pParticules[iIndex].flags & ( FLAG_PARTICULE_OUTRO1 | FLAG_PARTICULE_OUTRO2 | FLAG_PARTICULE_OUTRO3 ) )
		{
			ang = v_angles + Vector(0.0f, 0.0f, m_pParticules[iIndex].angles.z );
		}
		else if ( m_pParticules[iIndex].flags & FLAG_PARTICULE_SMOKE )
		{
			ang = v_angles;
		}
		else
		{
			ang = m_pParticules[iIndex].angles;
		}

		AngleVectors ( ang, forward, right, up );

		float largeurX = PARTICULES_LARGEUR / 2;
		float largeurY = PARTICULES_LARGEUR / 2;

		if ( m_pParticules[iIndex].flags & FLAG_PARTICULE_WOOD )
		{
			largeurY *= 0.5;
			largeurX *= 2;
		}

		if ( m_pParticules[iIndex].flags & ( FLAG_PARTICULE_OUTRO1 | FLAG_PARTICULE_OUTRO2 | FLAG_PARTICULE_OUTRO3 ) )
		{
			largeurX = largeurY = m_pParticules[iIndex].angles.x;
		}

		if ( m_pParticules[iIndex].flags & FLAG_PARTICULE_SMOKE )
		{
			largeurX = largeurY = 0.75 * 32 * 0.5;
		}

		int modelindex;
		struct model_s *mod = gEngfuncs.CL_LoadModel( m_pParticules[iIndex].model, &modelindex );


		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, 0 );	//chargement du sprite


		// mode de transparence

		if ( m_pParticules[iIndex].flags & ( FLAG_PARTICULE_OUTRO1 | FLAG_PARTICULE_OUTRO2 | FLAG_PARTICULE_OUTRO3 ) )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );	
			gEngfuncs.pTriAPI->Brightness( m_pParticules[iIndex].angles.y );						
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, m_pParticules[iIndex].angles.y );
		}
		else if ( m_pParticules[iIndex].flags & FLAG_PARTICULE_SMOKE )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );	
			gEngfuncs.pTriAPI->Brightness( 240/255 );						
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 240/255 );
		}
		else
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );	
			gEngfuncs.pTriAPI->Brightness( 1 );						
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
		}

		
		
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling
		

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//d

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );						//premier vertex
			vertex = m_pParticules[iIndex].origin - up*largeurY - right * largeurX;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );						//deuxieme vertex
			vertex = m_pParticules[iIndex].origin + right* largeurX - up*largeurY;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );						//troisieme vertex
			vertex = m_pParticules[iIndex].origin + right* largeurX + up* largeurY;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );					//quatrieme vertex
			vertex = m_pParticules[iIndex].origin + up* largeurX - right* largeurY;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

		gEngfuncs.pTriAPI->End();									//fin du trac
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );



	}



	int iDecalIndex, idecalresult = 0;

	for ( iDecalIndex = 0; iDecalIndex < MAX_DECALS ; iDecalIndex ++ )
	{
		// emplacement vide
		if ( m_pDecals [ iDecalIndex ].libre == 1 )
			continue;

		idecalresult ++;

		// frame

		int frame;

		if ( m_pDecals[iDecalIndex].flags & FLAG_DECAL_WIDEGLASS )
			frame = ( int ) m_pDecals[iDecalIndex].flFrame;
		else
			frame = 0;

			
		//trac
		vec3_t forward, right, up, vertex;
		AngleVectors ( m_pDecals[iDecalIndex].angles, forward, right, up );

		// sens al
		up		= m_pDecals[iDecalIndex].iRndSensX * up;
		right	= m_pDecals[iDecalIndex].iRndSensY * right;

		const float largeur = DECALS_LARGEUR / 2;

		int modelindex;
		struct model_s *mod = gEngfuncs.CL_LoadModel( m_pDecals[iDecalIndex].model, &modelindex );

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, frame );	//chargement du sprite

		if ( m_pDecals[iDecalIndex].flags & ( FLAG_DECAL_DISK | FLAG_DECAL_SG | FLAG_DECAL_XENO ) )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling

			float timeleft = m_pDecals[iDecalIndex].deathtime - gHUD.m_flTime;
			float alpha =  timeleft <= 0.5 ? timeleft * 2 : 1.0;
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
		}
		else if ( m_pDecals[iDecalIndex].flags & FLAG_DECAL_XENO )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence
			gEngfuncs.pTriAPI->CullFace( TRI_FRONT );						//(des)activer le face culling

			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 0.6 );
		}
		else
		{		
			gEngfuncs.pTriAPI->RenderMode( kRenderTransColor );					//mode de transparence

			if ( m_pDecals[iDecalIndex].flags & (FLAG_DECAL_WIDEGLASS|FLAG_DECAL_CROWBAR|FLAG_DECAL_CROWBAR_INVERT) )
				gEngfuncs.pTriAPI->CullFace( TRI_NONE );

			else
				gEngfuncs.pTriAPI->CullFace( TRI_FRONT );						//(des)activer le face culling

			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
		}
		

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//d

			gEngfuncs.pTriAPI->Brightness( 1 );						

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );						//premier vertex
			vertex = m_pDecals[iDecalIndex].origin - ( up + right ) * largeur;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );						//deuxieme vertex
			vertex = m_pDecals[iDecalIndex].origin + ( right - up ) * largeur;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );						//troisieme vertex
			vertex = m_pDecals[iDecalIndex].origin + ( right + up ) * largeur;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z ); 

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );					//quatrieme vertex
			vertex = m_pDecals[iDecalIndex].origin + ( up - right ) * largeur;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

		gEngfuncs.pTriAPI->End();									//fin du trac
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );


	}


//controle du nombre de quads dessin

	//	gEngfuncs.Con_Printf ( "client.dll : particules %i quads\n", iresult );
	//	gEngfuncs.Con_Printf ( "client.dll : decals : %i quads\n", idecalresult );

}


DECLARE_MESSAGE(m_Particules, ClientDecal);	//modif de Julien 7/7/01

//
//
// initialisation au chargement de la dll
//

int CHudParticules :: Init( void )
{
	for ( int i = 0; i < MAX_PARTICULES ; i ++ )
	{
		RemoveParticule ( i );
	}

	for ( int i = 0; i < MAX_DECALS ; i ++ )
	{	
		RemoveDecal ( i );
	}


	HOOK_MESSAGE(ClientDecal);	//modif de Julien npopopoy gvzakdsupp

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
}




//
//
// initialisation apr
//

int CHudParticules :: VidInit( void )
{

	for ( int i = 0; i < MAX_PARTICULES ; i ++ )
	{
		RemoveParticule ( i );
	}

	for ( int i = 0; i < MAX_DECALS ; i ++ )
	{	
		RemoveDecal ( i );
	}


	m_iFlags |= HUD_ACTIVE;

	return 1;
}




//
// activation du syst
// et de particules
// depuis le serveur
//


int CHudParticules::MsgFunc_ClientDecal( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	Vector vecSrc, vecNormal;
	char chTextureType;

	vecSrc.x = READ_COORD();
	vecSrc.y = READ_COORD();
	vecSrc.z = READ_COORD();

	vecNormal.x = READ_COORD();
	vecNormal.y = READ_COORD();
	vecNormal.z = READ_COORD();

	chTextureType = READ_CHAR();
	int decal = READ_BYTE();

	if ( decal == 4 )	// explo electro-roquette
	{
		AddDecal ( vecSrc, Vector ( 90,0,0 ), "sprites/rpg_disk.spr", gHUD.m_flTime + ELECTRO_DISK_MAX / ELECTRO_DISK_SPEED + 0.5, FLAG_DECAL_DISK );
		return 1;
	}
	if ( decal == 5 )	// explo supergun
	{
		Vector angDir;
		VectorAngles ( -vecNormal, angDir );
		angDir.y += 180;

		AddDecal ( vecSrc, angDir, "sprites/rpg_disk.spr", gHUD.m_flTime + 0.5, FLAG_DECAL_SG );
		return 1;
	}

	if ( decal == 6 )	// muzzle outro
	{
		Vector vecDir = vecNormal, velocity (0,0,0), avelocity(0,0,0), src = vecSrc;
		float largeur, brightness;

		//gros

		for ( int j=0; j<4; j++ )
		{
			avelocity	= Vector (0.0f, 0.0f, gEngfuncs.pfnRandomFloat(30,60)* ((j%2)==0?1:-1) );
			velocity	= Vector (vecDir - vecSrc) * 0/*( 0.08 - j*0.02 )*/;
			largeur = 5;
			brightness = 0.3;

			AddParticule ( src, Vector (largeur,brightness,0.0f), velocity, avelocity , "sprites/outro_muzzle.spr", gHUD.m_flTime + 7, FLAG_PARTICULE_OUTRO1 );
		}

		// petits

		for ( int i=0; i<10; i++ )
		{
			brightness = 0.3;
			avelocity	= Vector (0.0f, 0.0f, gEngfuncs.pfnRandomFloat(-300,300) );
		//	velocity	= Vector (vecDir - vecSrc) * ( 0.3 - i*0.03 ) * 2.5;
			velocity	= Vector (vecDir - vecSrc).Normalize() * 0.1;
			largeur		= 1.2 + i*0.3;
			src			= vecSrc + 0.1*(vecDir-vecSrc);

			AddParticule ( src, Vector (largeur,brightness,0.0f), velocity, avelocity, "sprites/outro_muzzle.spr", gHUD.m_flTime + 7, FLAG_PARTICULE_OUTRO2 );
		}

		return 1;
	}

	EV_HLDM_EjectParticules ( vecSrc, vecNormal, chTextureType, decal, 1 );

	return 1;
}

/*  
message du serveur

MESSAGE_BEGIN( MSG_ALL, gmsgClientDecal );

WRITE_COORD( vecSrc.x );					// xyz source
WRITE_COORD( vecSrc.y );
WRITE_COORD( vecSrc.z );
WRITE_COORD( pTrace->vecPlaneNormal.x );	// xyz norme
WRITE_COORD( pTrace->vecPlaneNormal.y );
WRITE_COORD( pTrace->vecPlaneNormal.z );
WRITE_CHAR ( chTextureType );				// type de texture

MESSAGE_END();
*/





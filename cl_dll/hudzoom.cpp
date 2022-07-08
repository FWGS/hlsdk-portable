/****************************************************************
*																*
*				hudzoom.cpp										*
*																*
*				par Julien										*
*																*
****************************************************************/

// code du viseur du fusil de snipe


#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"



int CHudSniper :: Draw	( float flTime )
{
	if ( gHUD.m_iFOV == 90 || gHUD.m_iFOV == 0 )
		return 1;

	static const float CenterX = ScreenWidth / 2; //modif de Roy, add float
	static const float CenterY = ScreenHeight / 2;


	if ( ScreenWidth >= 1024 )
	{
		// ligne du haut

		SPR_Set( m_sprHG, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 128 - 256, CenterY -128 - 256, &m_wrc1024);	

		SPR_Set( m_sprH, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 128, CenterY - 128 - 256, &m_wrc1024);

		SPR_Set( m_sprHD, 255, 255, 255);
		SPR_DrawHoles(0, CenterX + 128, CenterY -128 - 256, &m_wrc1024);

		// ligne du milieu

		SPR_Set( m_sprG, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 128 - 256, CenterY -128, &m_wrc1024);	

		SPR_Set( m_sprViseur, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 128, CenterY -128, &m_wrc1024);

		SPR_Set( m_sprD, 255, 255, 255);
		SPR_DrawHoles(0, CenterX + 128, CenterY -128, &m_wrc1024);

		// ligne du bas 

		SPR_Set( m_sprBG, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 128 - 256, CenterY + 128, &m_wrc1024);	

		SPR_Set( m_sprB, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 128, CenterY + 128, &m_wrc1024);

		SPR_Set( m_sprBD, 255, 255, 255);
		SPR_DrawHoles(0, CenterX + 128, CenterY + 128, &m_wrc1024);


		// noir 

		int i;

		for ( i = 0 ; i < ScreenHeight ; i += 256 )
		{
			m_wrcNoir.left = 0;
			m_wrcNoir.top = 0;
			m_wrcNoir.right = ScreenWidth / 2 - 128 - 256;
			m_wrcNoir.bottom = 256;

			SPR_Set( m_sprBlack, 255, 255, 255);
			SPR_DrawHoles(0, 0, i, &m_wrcNoir);

			SPR_DrawHoles( 0, ScreenWidth / 2 + 128 + 256, i, &m_wrcNoir);

		}

		if ( 256 * i + 256 < ScreenHeight )
		{
			m_wrcNoir.bottom = ScreenHeight - ( 256 * i + 256 ) ;

			SPR_Set( m_sprBlack, 255, 255, 255);
			SPR_DrawHoles(0, 0, i, &m_wrcNoir);

			SPR_DrawHoles( 0, ScreenWidth / 2 + 128 + 256, i, &m_wrcNoir);

		}

		// noir en haut et en bas

		if ( CenterY - 128 - 256 > 0 )
		{

			int i;

			for ( i = 0 ; i < 3 ; i ++ )
			{
				m_wrcNoir.left = 0;
				m_wrcNoir.top = 0;
				m_wrcNoir.right = 256;
				m_wrcNoir.bottom = CenterY - 128 - 256;

				SPR_Set( m_sprBlack, 255, 255, 255);
				SPR_DrawHoles(0, CenterX - 128 - 256 + i * 256 , 0, &m_wrcNoir);

				SPR_Set( m_sprBlack, 255, 255, 255);
				SPR_DrawHoles(0, CenterX - 128 - 256 + i * 256 , CenterY + 128 + 256, &m_wrcNoir);

			}
		}
	}


	else
	{

		// ligne du haut

		SPR_Set( m_sprHG, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 240, CenterY -240, &m_wrc640);	

		SPR_Set( m_sprHD, 255, 255, 255);
		SPR_DrawHoles(0, CenterX, CenterY -240, &m_wrc640);

		// ligne du bas 

		SPR_Set( m_sprBG, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 240, CenterY, &m_wrc640);	

		SPR_Set( m_sprBD, 255, 255, 255);
		SPR_DrawHoles(0, CenterX, CenterY, &m_wrc640);
		
		// viseur

		SPR_Set( m_sprViseur, 255, 255, 255);
		SPR_DrawHoles(0, CenterX - 16, CenterY - 16, &m_wrc640Viseur);


		// noir 

		if ( CenterX - 240 > 0 )
		{
			int i;

			for ( i = 0 ; i < ScreenHeight ; i += 256 )
			{
				m_wrcNoir.left = 0;
				m_wrcNoir.top = 0;
				m_wrcNoir.right = CenterX - 240;
				m_wrcNoir.bottom = 256;

				SPR_Set( m_sprBlack, 255, 255, 255);
				SPR_DrawHoles(0, 0, i, &m_wrcNoir);

				SPR_DrawHoles( 0, CenterX + 240, i, &m_wrcNoir);

			}

			if ( 256 * i + 256 < ScreenHeight )
			{
				m_wrcNoir.bottom = ScreenHeight - ( 256 * i + 256 ) ;

				SPR_Set( m_sprBlack, 255, 255, 255);
				SPR_DrawHoles(0, 0, i, &m_wrcNoir);

				SPR_DrawHoles( 0, CenterX + 240, i, &m_wrcNoir);

			}
		}

		// noir en haut et en bas

		if ( CenterY - 240 > 0 )
		{

			int i;

			for ( i = 0 ; i < 3 ; i ++ )
			{
				m_wrcNoir.left = 0;
				m_wrcNoir.top = 0;
				m_wrcNoir.right = 256;
				m_wrcNoir.bottom = CenterY - 240;

				SPR_Set( m_sprBlack, 255, 255, 255);
				SPR_DrawHoles(0, CenterX - 240 + i * 256 , 0, &m_wrcNoir);

				SPR_Set( m_sprBlack, 255, 255, 255);
				SPR_DrawHoles(0, CenterX - 240 + i * 256 , CenterY + 240, &m_wrcNoir);

			}
		}
	}


	return 1;
}




int CHudSniper :: Init( void )
{

	if ( ScreenWidth >= 1024 )
	{
		
		m_sprHG	= SPR_Load("sprites/fsniper/fsniper_1024_up_l.spr");
		if(!m_sprHG) m_sprHG	= SPR_Load("sprites/fsniper/fsniper_1024_up_l.SPR");
		m_sprH	= SPR_Load("sprites/fsniper/fsniper_1024_up.spr");
		if(!m_sprH) m_sprH	= SPR_Load("sprites/fsniper/fsniper_1024_up.SPR");
		m_sprHD = SPR_Load("sprites/fsniper/fsniper_1024_up_r.spr");
		if(!m_sprHD) m_sprHD = SPR_Load("sprites/fsniper/fsniper_1024_up_r.SPR");

		m_sprBD = SPR_Load("sprites/fsniper/fsniper_1024_down_r.spr");
		if(!m_sprBD) m_sprBD = SPR_Load("sprites/fsniper/fsniper_1024_down_r.SPR");
		m_sprB	= SPR_Load("sprites/fsniper/fsniper_1024_down.spr");
		if(!m_sprB) m_sprB	= SPR_Load("sprites/fsniper/fsniper_1024_down.SPR");
		m_sprBG = SPR_Load("sprites/fsniper/fsniper_1024_down_l.spr");
		if(!m_sprBG) m_sprBG = SPR_Load("sprites/fsniper/fsniper_1024_down_l.SPR");

		m_sprG	= SPR_Load("sprites/fsniper/fsniper_1024_left.spr");
		if(!m_sprG) m_sprG	= SPR_Load("sprites/fsniper/fsniper_1024_left.SPR");
		m_sprD	= SPR_Load("sprites/fsniper/fsniper_1024_right.spr");
		if(!m_sprD) m_sprD	= SPR_Load("sprites/fsniper/fsniper_1024_right.SPR");

		m_sprViseur	= SPR_Load("sprites/fsniper/fsniper_1024_cross.spr");
		if(!m_sprViseur) m_sprViseur	= SPR_Load("sprites/fsniper/fsniper_1024_cross.SPR");
		m_sprBlack	= SPR_Load("sprites/fsniper/fsniper_black.spr");
		if(!m_sprBlack) m_sprBlack	= SPR_Load("sprites/fsniper/fsniper_black.SPR");


		m_wrc1024.left = 0;
		m_wrc1024.top = 0;
		m_wrc1024.right = 256;
		m_wrc1024.bottom = 256;

	}

	else /* if ( ScreenWidth <= 640 )*/
	{

		m_sprHG	= SPR_Load("sprites/fsniper/fsniper_640_up_l.spr");
		if(!m_sprHG) m_sprHG = SPR_Load("sprites/fsniper/fsniper_640_up_l.SPR");
		m_sprHD = SPR_Load("sprites/fsniper/fsniper_640_up_r.spr");
		if(!m_sprHD) m_sprHD = SPR_Load("sprites/fsniper/fsniper_640_up_r.SPR");
		m_sprBD = SPR_Load("sprites/fsniper/fsniper_640_down_r.spr");
		if(!m_sprBD) m_sprBD = SPR_Load("sprites/fsniper/fsniper_640_down_r.SPR");
		m_sprBG = SPR_Load("sprites/fsniper/fsniper_640_down_l.spr");
		if(!m_sprBG) m_sprBG = SPR_Load("sprites/fsniper/fsniper_640_down_l.SPR");

		m_sprViseur	= SPR_Load("sprites/fsniper/fsniper_640_cross.spr");
		if(!m_sprViseur) m_sprViseur = SPR_Load("sprites/fsniper/fsniper_640_cross.SPR");
		m_sprBlack	= SPR_Load("sprites/fsniper/fsniper_black.spr");



		m_wrc640.left	= 0;
		m_wrc640.top	= 0;
		m_wrc640.right	= 240;
		m_wrc640.bottom = 240;

		m_wrc640Viseur.left		= 0;
		m_wrc640Viseur.top		= 0;
		m_wrc640Viseur.right	= 32;
		m_wrc640Viseur.bottom	= 32;
	}


	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
}



int CHudSniper :: VidInit( void )
{

	if ( ScreenWidth >= 1024 )
	{
		
		m_sprHG	= SPR_Load("sprites/fsniper/fsniper_1024_up_l.spr");
		if(!m_sprHG) m_sprHG	= SPR_Load("sprites/fsniper/fsniper_1024_up_l.SPR");
		m_sprH	= SPR_Load("sprites/fsniper/fsniper_1024_up.spr");
		if(!m_sprH) m_sprH	= SPR_Load("sprites/fsniper/fsniper_1024_up.SPR");
		m_sprHD = SPR_Load("sprites/fsniper/fsniper_1024_up_r.spr");
		if(!m_sprHD) m_sprHD = SPR_Load("sprites/fsniper/fsniper_1024_up_r.SPR");

		m_sprBD = SPR_Load("sprites/fsniper/fsniper_1024_down_r.spr");
		if(!m_sprBD) m_sprBD = SPR_Load("sprites/fsniper/fsniper_1024_down_r.SPR");
		m_sprB	= SPR_Load("sprites/fsniper/fsniper_1024_down.spr");
		if(!m_sprB) m_sprB	= SPR_Load("sprites/fsniper/fsniper_1024_down.SPR");
		m_sprBG = SPR_Load("sprites/fsniper/fsniper_1024_down_l.spr");
		if(!m_sprBG) m_sprBG = SPR_Load("sprites/fsniper/fsniper_1024_down_l.SPR");

		m_sprG	= SPR_Load("sprites/fsniper/fsniper_1024_left.spr");
		if(!m_sprG) m_sprG	= SPR_Load("sprites/fsniper/fsniper_1024_left.SPR");
		m_sprD	= SPR_Load("sprites/fsniper/fsniper_1024_right.spr");
		if(!m_sprD) m_sprD	= SPR_Load("sprites/fsniper/fsniper_1024_right.SPR");

		m_sprViseur	= SPR_Load("sprites/fsniper/fsniper_1024_cross.spr");
		if(!m_sprViseur) m_sprViseur	= SPR_Load("sprites/fsniper/fsniper_1024_cross.SPR");
		m_sprBlack	= SPR_Load("sprites/fsniper/fsniper_black.spr");
		if(!m_sprBlack) m_sprBlack	= SPR_Load("sprites/fsniper/fsniper_black.SPR");


		m_wrc1024.left = 0;
		m_wrc1024.top = 0;
		m_wrc1024.right = 256;
		m_wrc1024.bottom = 256;

	}

	else /*if ( ScreenWidth <= 640 )*/
	{

		m_sprHG	= SPR_Load("sprites/fsniper/fsniper_640_up_l.spr");
		if(!m_sprHG) m_sprHG = SPR_Load("sprites/fsniper/fsniper_640_up_l.SPR");
		m_sprHD = SPR_Load("sprites/fsniper/fsniper_640_up_r.spr");
		if(!m_sprHD) m_sprHD = SPR_Load("sprites/fsniper/fsniper_640_up_r.SPR");
		m_sprBD = SPR_Load("sprites/fsniper/fsniper_640_down_r.spr");
		if(!m_sprBD) m_sprBD = SPR_Load("sprites/fsniper/fsniper_640_down_r.SPR");
		m_sprBG = SPR_Load("sprites/fsniper/fsniper_640_down_l.spr");
		if(!m_sprBG) m_sprBG = SPR_Load("sprites/fsniper/fsniper_640_down_l.SPR");

		m_sprViseur	= SPR_Load("sprites/fsniper/fsniper_640_cross.spr");
		if(!m_sprViseur) m_sprViseur = SPR_Load("sprites/fsniper/fsniper_640_cross.SPR");
		m_sprBlack	= SPR_Load("sprites/fsniper/fsniper_black.spr");



		m_wrc640.left	= 0;
		m_wrc640.top	= 0;
		m_wrc640.right	= 240;
		m_wrc640.bottom = 240;

		m_wrc640Viseur.left		= 0;
		m_wrc640Viseur.top		= 0;
		m_wrc640Viseur.right	= 32;
		m_wrc640Viseur.bottom	= 32;
	}

	m_iFlags |= HUD_ACTIVE;

	return 1;
}

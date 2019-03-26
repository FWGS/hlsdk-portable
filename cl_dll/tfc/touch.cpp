#include "touch.h"
#include "cl_dll.h"
//#include "util.h"
#include "hud.h"
#include "cl_util.h"

CLabel::CLabel( int index, int x, int y )
{
	Position.X1 = ( (float)x * ( (float)ScreenWidth / 640.0f ) ) / (float)ScreenWidth;
	Position.Y1 = ( (float)y * ( (float)ScreenHeight / 480.0f ) ) / (float)ScreenHeight;
	Position.X2 = 1.0f;
	Position.Y2 = 1.0f;
	sprintf( Name, "_vgui_label%i", index );
	SetVisibility( false );
}

void CLabel::SetText( const char* str )
{
	sprintf(Text, "%s", str);
}

void CLabel::SetPosition( int x, int y )
{
	Position.X1 = ( (float)x * ( (float)ScreenWidth / 640.0f ) ) / (float)ScreenWidth;
	Position.Y1 = ( (float)y * ( (float)ScreenHeight / 480.0f ) ) / (float)ScreenHeight;
}

void CLabel::SetColor( int r, int g, int b, int a )
{
	Color.R = r;
	Color.G = g;
	Color.B = b;
	Color.A = a;
}

void CLabel::SetVisibility( bool visible )
{
	Visible = visible;
}

void CLabel::Draw( void )
{
	if(!Visible)
		return;

	char szCmd[512];

	snprintf( szCmd, 512, "touch_addbutton \"%s\" \"#%s\" \"", Name, Text );
	snprintf( szCmd + strlen(szCmd), 512, "\" %.6f %.6f %.6f %.6f %i %i %i %i 4\n", Position.X1, Position.Y1, Position.X2, Position.Y2, Color.R, Color.G, Color.B, Color.A);

	ClientCmd( szCmd );
}

CButton::CButton( int index, int x, int y, int width, int height )
{
	Position.X1 = ( (float)x * ( (float)ScreenWidth / 640.0f ) ) / (float)ScreenWidth;
	Position.Y1 = ( (float)y * ( (float)ScreenHeight / 480.0f ) ) / (float)ScreenHeight;
	Position.X2 = ( ( (float)x + (float)width ) * ( (float)ScreenWidth / 640.0f ) ) / (float)ScreenWidth;
	Position.Y2 = ( ( (float)y + (float)height ) * ( (float)ScreenHeight / 480.0f ) ) / (float)ScreenHeight;
	Size.Width = width;
	Size.Height = height;
	NumCmds = 0;
	sprintf( Name, "_vgui_button%i", index );

	Label = new CLabel( index, x + 2 , y + ( height / 2 ) );

	SetVisibility( false );
}

void CButton::AddCommand( const char* cmd )
{
	if(NumCmds == 8)
		return;

	sprintf( Commands[NumCmds], "%s;", cmd );
	NumCmds++;
}

void CButton::SetVisibility( bool visible )
{
	Visible = visible;
}

void CButton::SetPosition( int x, int y )
{
	Position.X1 = ( (float)x * ( (float)ScreenWidth / 640.0f ) ) / (float)ScreenWidth;
	Position.Y1 = ( (float)y * ( (float)ScreenHeight / 480.0f ) ) / (float)ScreenHeight;
	Position.X2 = ( ( (float)x + (float)Size.Width ) * ( (float)ScreenWidth / 640.0f ) ) / (float)ScreenWidth;
	Position.Y2 = ( ( (float)y + (float)Size.Height ) * ( (float)ScreenHeight / 480.0f ) ) / (float)ScreenHeight;

	Label->SetPosition( x + 2, y + ( Size.Height / 4 ) );
}

void CButton::SetText( const char* str )
{
	Label->SetText( str );
	Label->SetColor( 255, 174, 0, 255 );
	Label->SetVisibility( true );
}

void CButton::SetColor( int r, int g, int b, int a )
{
	Color.R = r;
	Color.G = g;
	Color.B = b;
	Color.A = a;
}

void CButton::Draw( void )
{
	if(!Visible)
		return;

	char szCmd[512];

	sprintf( szCmd, "touch_addbutton \"%s\" \"*white\" \"", Name );

	for( int i = 0; i < NumCmds; i++ )
		sprintf( szCmd + strlen(szCmd), "%s", Commands[i] );

	sprintf( szCmd + strlen(szCmd), "\" %.6f %.6f %.6f %.6f %i %i %i %i 260\n", Position.X1, Position.Y1, Position.X2, Position.Y2, Color.R, Color.G, Color.B, Color.A);

	Label->Draw();

	ClientCmd( szCmd );
}

void CTouchMenu::SetStroke( int width, int r, int g, int b, int a )
{
	char szCmd[64];

	sprintf( szCmd, "touch_set_stroke %i %i %i %i %i\n", width, r, g, b, a );

	ClientCmd( szCmd );
}

void CTouchMenu::Hide( void )
{
	char szCmd[64];

	sprintf( szCmd, CMD_EXIT );

	ClientCmd( szCmd );
}


int CTouchMenu::GetNewButtonIndex( void )
{
	return ++BtnIndex;
}

void CTeamTouchMenu::Init( void )
{
	if( Initialized )
		return;

	SetStroke( 1, 156, 77, 20, 200 );

	m_pTitle = new CLabel( GetNewButtonIndex(), 40, 32 );
	//m_pTitle->SetText( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Title_SelectYourTeam" ) );
	m_pTitle->SetText( "SELECT YOUR TEAM" );
	m_pTitle->SetColor( 255, 174, 0, 255 );
	m_pTitle->SetVisibility( true );

	//CLabel *pMapName = new CLabel(16, 16);
	//pMapName->SetColor(255, 174, 0, 255);

	//Map Briefing?

	for ( int i = 1; i <= 5; i++ )
	{
		char szCmd[32];

		int iYPos = 80 + ( (24 + 8) * i );

		sprintf(szCmd, "jointeam %d", i);

		if (i == 5)
		{
			m_pButtons[i] = new CButton( GetNewButtonIndex(), 40, iYPos, 124, 24 );
			m_pButtons[i]->AddCommand( szCmd );
			m_pButtons[i]->AddCommand( CMD_EXIT );
			//m_pButtons[i]->SetText( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Team_AutoAssign" ) );
			m_pButtons[i]->SetText( "AUTO-ASSIGN" );
			m_pButtons[i]->SetColor( 0, 0, 0, 50 );
		}

		m_pButtons[i] = new CButton( GetNewButtonIndex(), 40, iYPos, 124, 24 );
		m_pButtons[i]->AddCommand( szCmd );
		m_pButtons[i]->AddCommand( CMD_EXIT );
		m_pButtons[i]->SetColor( 0, 0, 0, 50 );

		//Team Info?
	}

	m_pCancelButton = new CButton( GetNewButtonIndex(), 40, 0, 124, 24 );
	//m_pCancelButton->SetText( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Menu_Cancel" ) );
	m_pCancelButton->SetText( "CANCEL" );
	m_pCancelButton->SetColor(0, 0, 0, 50);
	m_pCancelButton->AddCommand( CMD_EXIT );

	m_pSpectateButton = new CButton( GetNewButtonIndex(), 40, 0, 124, 24 );
	//m_pSpectateButton->SetText( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Menu_Spectate" ) );
	m_pSpectateButton->SetText( "SPECTATE" );
	m_pSpectateButton->SetColor( 0, 0, 0, 50 );
	m_pSpectateButton->AddCommand( "spectate" );

	Initialized = true;

	Update();
}

void CTeamTouchMenu::Update( void )
{
	if( !Initialized )
		return;

	int iYPos = 80;
	char *szTeams[] = {":P", "1 RED", "2 BLUE", "3 GREEN", "4 YELLOW"};

	for ( int i = 1; i <= 4; i++ )
	{
		if ( i <= gHUD.m_iNumberOfTeams )
		{
			m_pButtons[i]->SetPosition( 40, iYPos );
			m_pButtons[i]->SetVisibility( true );
			m_pButtons[i]->SetText( szTeams[i] );
			iYPos += 24 + 8;
		}

	}

	m_pButtons[5]->SetPosition( 40, iYPos );
	iYPos += 24 + 8;

	m_pSpectateButton->SetPosition( 40, iYPos );
	m_pSpectateButton->SetVisibility( true );
	iYPos += 24 + 8;

	if ( g_iTeamNumber )
	{
		m_pCancelButton->SetPosition( 40, iYPos );
		m_pCancelButton->SetVisibility( true );
	}

	Draw();
}

void CTeamTouchMenu::Draw( void )
{
	if( !Initialized )
		Init();

	ClientCmd( "touch_setclientonly 1\n" );

	m_pTitle->Draw();

	for ( int i = 1; i <= 4; i++ )
		m_pButtons[i]->Draw();

	m_pSpectateButton->Draw();
	m_pCancelButton->Draw();
}

void CClassTouchMenu::Init( void )
{
	if( Initialized )
		return;

	SetStroke( 1, 156, 77, 20, 200 );
	int iYPos = 80;

	m_pTitle = new CLabel( GetNewButtonIndex(), 40, 32 );
	m_pTitle->SetText( "SELECT YOUR CLASS" );
	m_pTitle->SetColor( 255, 174, 0, 255 );
	m_pTitle->SetVisibility( true );

	char *szCommands[] = {"scout", "sniper", "soldier", "demoman", "medic", "hwguy", "pyro", "spy", "engineer"};
	char *szLabels[] = {"1 SCOUT", "2 SNIPER", "3 SOLDIER", "4 DEMOMAN", "5 MEDIC", "6 HWGUY", "7 PYRO", "8 SPY", "9 ENGINEER"};

	for ( int i = 0; i <= 8; i++ )
	{
		m_pButtons[i] = new CButton( GetNewButtonIndex(), 40, iYPos, 124, 24 );
		m_pButtons[i]->AddCommand( szCommands[i] );
		m_pButtons[i]->AddCommand( CMD_EXIT );
		m_pButtons[i]->SetText( szLabels[i] );
		m_pButtons[i]->SetColor( 0, 0, 0, 50 );
		m_pButtons[i]->SetVisibility( true );
		iYPos += 24 + 8;
	}

	Initialized = true;
}

void CClassTouchMenu::Draw( void )
{
	if( !Initialized )
		Init();

	ClientCmd( "touch_setclientonly 1\n" );

	m_pTitle->Draw();

	for ( int i = 0; i <= 8; i++ )
		m_pButtons[i]->Draw();
}

#ifndef TOUCH_H
#define TOUCH_H

#define CMD_EXIT "touch_removebutton _vgui_*;touch_setclientonly 0"

typedef struct color_s {
	int R, G, B, A;
} color_t;

typedef struct position_s {
	float X1, Y1, X2, Y2;
} position_t;

typedef struct dimensions_s {
	int Width, Height;
} dimensions_t;

class CLabel
{
public:
	CLabel( int index, int x, int y );

	void SetVisibility( bool visible );
	void SetText( const char* str );
	void SetPosition( int x, int y );
	void SetColor( int r, int g, int b, int a );
	void Draw( void );

private:
	char Name[64];
	char Text[64];
	position_t Position;
	color_t Color;
	bool Visible;
};

class CButton
{
public:
	CButton( int index, int x, int y, int width, int height );

	void SetVisibility( bool visible );
	void SetText( const char* str );
	void SetPosition( int x, int y );
	void SetColor( int r, int g, int b, int a );
	void Draw( void );
	void AddCommand( const char* cmd );

private:
	char Name[64];
	char Commands[8][64];
	position_t Position;
	dimensions_t Size;
	color_t Color;
	bool Visible;
	int NumCmds;
	CLabel *Label;
};

class CTouchMenu
{
public:
	CTouchMenu() :  BtnIndex(0), Initialized(false) {}

	void SetStroke( int width, int r, int g, int b, int a );
	void Hide( void );
	int GetNewButtonIndex( void );
	int BtnIndex;
	bool Initialized;
};

class CTeamTouchMenu : public CTouchMenu
{
public:
	void Init( void );
	void Update ( void );
	void Draw( void );
	CButton	*m_pButtons[6];

private:
	CLabel *m_pTitle;
	CButton *m_pSpectateButton;
	CButton *m_pCancelButton;
};

class CClassTouchMenu : public CTouchMenu
{
public:
	void Init( void );
	void Draw( void );
	CButton	*m_pButtons[9];

private:
	CLabel *m_pTitle;
};

#endif

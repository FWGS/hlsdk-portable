
#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include<VGUI_Panel.h>
#include<VGUI_Frame.h>
#include<VGUI_Label.h>

using namespace vgui;

// Wrapper for an Image Label without a background
class CImageLabel : public Label
{
public:
	BitmapTGA	*m_pTGA;

public:
	CImageLabel( const char* pImageName,int x,int y );
	CImageLabel( const char* pImageName,int x,int y,int wide,int tall );
	~CImageLabel();

	virtual int getImageTall();
	virtual int getImageWide();

	virtual void paintBackground()
	{
		// Do nothing, so the background's left transparent.
	}
};

#endif


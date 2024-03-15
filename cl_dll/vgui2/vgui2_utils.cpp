#include <vgui2/vgui2_utils.h>

#include <tier1/strtools.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Controls.h>

vgui2::HFont font = vgui2::INVALID_FONT;

void VGUI2_Init()
{
	font = vgui2::surface()->CreateFont();
	vgui2::surface()->AddGlyphSetToFont(font, "Open Sans", 36, 600, 0, 0, vgui2::ISurface::FONTFLAG_ANTIALIAS, 0, 0);
}

int VGUI2_GetCharacterWidth(int ch)
{
	int a = 0;
	int b = 0;
	int c = 0;
	vgui2::surface()->GetCharABCwide(font, ch, a, b, c);
	return a + b + c;
}

int VGUI2_DrawCharacter(int x, int y, int ch, int r, int g, int b)
{
	vgui2::surface()->DrawSetTextFont(font);
	vgui2::surface()->DrawSetTextColor(r, g, b, 255);
	vgui2::surface()->DrawSetTextPos(x, y);
	vgui2::surface()->DrawUnicodeCharAdd(ch);
	return VGUI2_GetCharacterWidth(ch);
}

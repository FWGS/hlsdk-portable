/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Btns.h"
#include <string.h>

#define ART_BUTTONS_MAIN "gfx/shell/btns_main.bmp" // we support bmp only
#define BTN_GUARD_SIZE   2 // guard pixels between buttons and states in atlas

/*
=================
CBtnsManager::LoadBmpButtons
=================
*/
void CBtnsManager::LoadBmpButtons()
{
	if( uiStatic.lowmemory || uiStatic.renderPicbuttonText )
		return;

	CBMP *bmp = CBMP::LoadFile( ART_BUTTONS_MAIN );

	if( bmp == nullptr )
		return;

	const bmp_t *hdr = bmp->GetBitmapHdr();
	const int bpp = hdr->bitsPerPixel / 8;

	width = hdr->width;
	height = 26; // virtual button height for layout

	const int button_w = hdr->width;
	const int button_tex_h = height; // WON buttons are always 26px in height
	const int src_cell_h = button_tex_h * 3;
	const int pic_count = Q_min( hdr->height / button_tex_h / 3, PC_BUTTONCOUNT );

	if( bpp == 0 || button_w == 0 || pic_count <= 0 )
	{
		delete bmp;
		return;
	}

	const int src_stride = (( hdr->width * bpp ) + 3 ) & ~3;

	// atlas cell dimensions including guard pixels
	const int state_step = button_tex_h + BTN_GUARD_SIZE;
	const int cell_w = button_w + BTN_GUARD_SIZE;
	const int cell_h = state_step * 3;

	tex_h = button_tex_h;
	tex_stride = state_step;

	// find the smallest POT atlas that fits all buttons in a single page
	const int pot_sizes[] = { 256, 512, 1024 };
	int atlas_w = 0, atlas_h = 0;
	int cols = 0, rows = 0;
	int best_area = 0x7FFFFFFF;

	for( int wi = 0; wi < ARRAYSIZE( pot_sizes ); wi++ )
	{
		for( int hi = 0; hi < ARRAYSIZE( pot_sizes ); hi++ )
		{
			int w = pot_sizes[wi];
			int h = pot_sizes[hi];
			int c = w / cell_w;
			int r = h / cell_h;

			if( c > 0 && r > 0 && c * r >= pic_count && w * h < best_area )
			{
				atlas_w = w;
				atlas_h = h;
				cols = c;
				rows = r;
				best_area = w * h;
			}
		}
	}

	// if no single texture fits, use 1024x1024 pages
	if( atlas_w == 0 )
	{
		atlas_w = 1024;
		atlas_h = 1024;
		cols = atlas_w / cell_w;
		rows = atlas_h / cell_h;

		if( cols == 0 || rows == 0 )
		{
			delete bmp;
			return;
		}
	}

	const int per_page = cols * rows;
	const int num_pages = ( pic_count + per_page - 1 ) / per_page;
	const int dst_stride = (( atlas_w * bpp ) + 3 ) & ~3;
	const int atlas_img_sz = dst_stride * atlas_h;
	const int src_height = hdr->height;

	byte *src = bmp->GetTextureData();

	for( int page = 0; page < num_pages; page++ )
	{
		CBMP atlas_bmp( bmp->GetBitmapHdr(), atlas_img_sz );
		atlas_bmp.GetBitmapHdr()->width = atlas_w;
		atlas_bmp.GetBitmapHdr()->height = atlas_h;

		byte *dst = atlas_bmp.GetTextureData();
		memset( dst, 0, atlas_img_sz );

		for( int slot = 0; slot < per_page; slot++ )
		{
			int btn = page * per_page + slot;
			if( btn >= pic_count )
				break;

			int col = slot % cols;
			int row = slot / cols;

			x[btn] = col * cell_w;
			y[btn] = row * cell_h;

			// copy 3 button states into the atlas cell
			for( int state = 0; state < 3; state++ )
			{
				for( int line = 0; line < button_tex_h; line++ )
				{
					int src_visual_y = btn * src_cell_h + state * button_tex_h + line;
					int src_row = src_height - 1 - src_visual_y;

					int dst_visual_y = row * cell_h + state * state_step + line;
					int dst_row = atlas_h - 1 - dst_visual_y;

					byte *s = &src[src_row * src_stride];
					byte *d = &dst[dst_row * dst_stride + col * cell_w * bpp];

					memcpy( d, s, button_w * bpp );

					// fixup alpha for half-life infected mod
					if( bpp == 4 )
					{
						for( int px = 0; px < button_w; px++ )
							d[px * 4 + 3] = 255;
					}
				}

				// fix misaligned gearbox btns: clear bottom visual row of each state
				int dst_visual_y2 = row * cell_h + state * state_step + button_tex_h - 1;
				int dst_row2 = atlas_h - 1 - dst_visual_y2;
				byte *d2 = &dst[dst_row2 * dst_stride + col * cell_w * bpp];
				memset( d2, 0, button_w * bpp );
			}
		}

		CUtlString fname;
		fname.Format( "#btns_atlas_%d.bmp", page );

		HIMAGE hAtlas = atlas_bmp.Upload( fname.String() );

		for( int slot = 0; slot < per_page; slot++ )
		{
			int btn = page * per_page + slot;
			if( btn >= pic_count )
				break;

			pics[btn] = hAtlas;
		}
	}

	delete bmp;
}

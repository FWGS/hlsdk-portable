#include <cassert>

#include "hud.h"
#include "cl_util.h"

#include "triangleapi.h"

#include "hud_sprite.h"


extern cvar_t *hud_renderer;
extern cvar_t *hud_scale;

static void ScaledSetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b)
{
	ScaledRenderer::Instance().SetCrosshair(hspr, rc, r, g, b);
}

ScaledRenderer::ScaledRenderer() {
	origSpriteEngfuncs.pfnSetCrosshair = NULL;

	sprite = -1;
	sprite_model = NULL;
	sprite_color.r = 0;
	sprite_color.g = 0;
	sprite_color.b = 0;

	crosshair = -1;
	crosshair_model = NULL;
	crosshair_dimensions.left = 0;
	crosshair_dimensions.right = 0;
	crosshair_dimensions.top = 0;
	crosshair_dimensions.bottom = 0;

	crosshair_color.r = 0;
	crosshair_color.g = 0;
	crosshair_color.b = 0;

	hud_renderer_value = 0.0f;
}

void ScaledRenderer::EnableCustomRendering() {
	gEngfuncs.pfnSetCrosshair = ScaledSetCrosshair;
}

void ScaledRenderer::DisableCustomRendering() {
	gEngfuncs.pfnSetCrosshair = origSpriteEngfuncs.pfnSetCrosshair;
}

float ScaledRenderer::GetHUDScale() {
	if (gHUD.hasHudScaleInEngine)
		return 1.0f;
	if (gHUD.m_iHardwareMode == 0)
		return 1.0f;

	assert(hud_renderer != NULL && hud_scale != NULL);

	float scale = 1.0f;

	if (hud_renderer->value > 0.0f) {
		if (hud_scale->value > 0.0f)
			scale = hud_scale->value;
		else
			scale = hud_auto_scale_value;
	}

	return scale;
}

int ScaledRenderer::ScreenWidthScaled() {
	return static_cast<int>(gHUD.m_scrinfo.iWidth / GetHUDScale());
}

int ScaledRenderer::ScreenHeightScaled() {
	return static_cast<int>(gHUD.m_scrinfo.iHeight / GetHUDScale());
}

int ScaledRenderer::ScaleScreen(int value) {
	return static_cast<int>(value * GetHUDScale());
}

int ScaledRenderer::UnscaleScreen(int value) {
	return static_cast<int>(value / GetHUDScale());
}

void ScaledRenderer::SPR_DrawInternal(int frame, float x, float y, float width, float height, const wrect_t *dimensions, int mode) {
	if (!sprite_model) {
		return;
	}

	float s1 = 0.0f;
	float s2 = 0.0f;
	float t1 = 0.0f;
	float t2 = 0.0f;

	if (width == -1.0f && height == -1.0f) {
		width = SPR_Width(sprite, frame);
		height = SPR_Height(sprite, frame);
	}

	if (dimensions) {
		wrect_t rc = *dimensions;

		if (rc.left <= 0 || rc.left >= width) {
			rc.left = 0;
		}

		if (rc.top <= 0 || rc.top >= height) {
			rc.top = 0;
		}

		if (rc.right <= 0 || rc.right > width) {
			rc.right = width;
		}

		if (rc.bottom <= 0 || rc.bottom > height) {
			rc.bottom = height;
		}

		float offset = 0.0f;

		if (GetHUDScale() > 1.0f) {
			offset = 0.5f;
		}

		s1 = (rc.left + offset) / width;
		t1 = (rc.top + offset) / height;

		s2 = (rc.right - offset) / width;
		t2 = (rc.bottom - offset) / height;

		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	} else {
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	// TODO: Do scissor test here

	x = ScaleScreen(x);
	y = ScaleScreen(y);

	width = ScaleScreen(width);
	height = ScaleScreen(height);

	if (!gEngfuncs.pTriAPI->SpriteTexture(sprite_model, frame)) {
		return;
	}

	gEngfuncs.pTriAPI->Color4ub(sprite_color.r, sprite_color.g, sprite_color.b, 255);

	gEngfuncs.pTriAPI->RenderMode(mode);

	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		gEngfuncs.pTriAPI->TexCoord2f(s1, t1);
		gEngfuncs.pTriAPI->Vertex3f(x, y, 0.0f);
		gEngfuncs.pTriAPI->TexCoord2f(s2, t1);
		gEngfuncs.pTriAPI->Vertex3f(x + width, y, 0.0f);
		gEngfuncs.pTriAPI->TexCoord2f(s2, t2);
		gEngfuncs.pTriAPI->Vertex3f(x + width, y + height, 0.0f);
		gEngfuncs.pTriAPI->TexCoord2f(s1, t2);
		gEngfuncs.pTriAPI->Vertex3f(x, y + height, 0.0f);
	gEngfuncs.pTriAPI->End();

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

int ScaledRenderer::HUD_VidInit() {
	if (gHUD.hasHudScaleInEngine)
		return 1;
	if (gHUD.m_iHardwareMode == 0)
		DisableCustomRendering();
	else if (hud_renderer) {
		hud_renderer_value = hud_renderer->value;
		if (hud_renderer_value > 0.0f) {
			EnableCustomRendering();
		}
	}

	assert(hud_scale != NULL);

	float average_scale = 1.0f;

	float x_scale;
	float y_scale;

	const int iWidth = gHUD.m_scrinfo.iWidth;
	const int iHeight = gHUD.m_scrinfo.iHeight;

	if (iWidth > 1024) {
		if (iWidth >= 1280) {
			x_scale = iWidth / 960.0f;
		} else {
			x_scale = iWidth / 640.0f;
		}

		if (iHeight >= 1024 || iWidth > 1280) {
			y_scale = iHeight / 720.0f;
		} else {
			y_scale = iHeight / 480.0f;
		}

		if (x_scale > 0.0f && y_scale > 0.0f) {
			average_scale = sqrtf(0.5f * (x_scale * x_scale + y_scale * y_scale));

			float doubleScale = average_scale * 2.0f;
			float rounded = floor(doubleScale + 0.5f);
			average_scale = rounded / 2.0f;
		}
	}
	hud_auto_scale_value = average_scale;

	return 1;
}

void ScaledRenderer::HUD_Init() {
	origSpriteEngfuncs.pfnSetCrosshair = gEngfuncs.pfnSetCrosshair;
}

void ScaledRenderer::HUD_Frame(double time) {
	(void)time;
	if (hud_renderer && hud_renderer->value != hud_renderer_value) {
		hud_renderer_value = hud_renderer->value;

		if (hud_renderer->value > 0.0f) {
			EnableCustomRendering();
		} else {
			DisableCustomRendering();
		}
	}

	// TODO: Simple bounds checking on certain variables
}

void ScaledRenderer::SPR_Set(HSPRITE hPic, int r, int g, int b) {
	if (GetHUDScale() == 1.0f) {
		::SPR_Set(hPic, r, g, b);
		return;
	}
	SPR_SetInternal(hPic, r, g, b);
}

void ScaledRenderer::SPR_SetInternal(HSPRITE hPic, int r, int g, int b) {
	sprite = hPic;
	sprite_model = const_cast<model_t *>(gEngfuncs.GetSpritePointer(sprite));
	sprite_color.r = r;
	sprite_color.g = g;
	sprite_color.b = b;
}

void ScaledRenderer::SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc) {
	if (GetHUDScale() == 1.0f) {
		::SPR_DrawAdditive(frame, x, y, prc);
		return;
	}
	SPR_DrawInternal(frame, x, y, -1.0f, -1.0f, prc, kRenderTransAdd);
}

void ScaledRenderer::FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a) {
	if (GetHUDScale() == 1.0f) {
		::FillRGBA(x, y, width, height, r, g, b, a);
		return;
	}

	gEngfuncs.pfnFillRGBA(ScaleScreen(x), ScaleScreen(y), ScaleScreen(width), ScaleScreen(height), r, g, b, a);
}

void ScaledRenderer::SetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b) {
	crosshair = hspr;
	crosshair_model = const_cast<model_t *>(gEngfuncs.GetSpritePointer(crosshair));
	crosshair_dimensions = rc;
	crosshair_color.r = r;
	crosshair_color.g = g;
	crosshair_color.b = b;

	wrect_t crosshair_rect;
	crosshair_rect.left = 0;
	crosshair_rect.right = 0;
	crosshair_rect.top = 0;
	crosshair_rect.bottom = 0;

	origSpriteEngfuncs.pfnSetCrosshair(0, crosshair_rect, 0, 0, 0);
}

void ScaledRenderer::QueryCrosshairInfo(HSPRITE *sprite, model_t **sprite_model, wrect_t *sprite_dimensions, color24 *sprite_color) {
	assert(sprite);
	assert(sprite_model);
	assert(sprite_dimensions);
	assert(sprite_color);

	*sprite = crosshair;
	*sprite_model = crosshair_model;
	*sprite_dimensions = crosshair_dimensions;
	*sprite_color = crosshair_color;
}

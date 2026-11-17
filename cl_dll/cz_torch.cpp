// CZ blowtorch effect, ported from halflife-featureful (ev_hldm.cpp, blowtorch):
// a glow and a pilot flame made of particles at the view model attachment 0, plus a dynamic light.
#include <cmath>
#include <cstdio>

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "r_efx.h"
#include "particleman.h"

extern int g_czTool, g_czFiber;
extern vec3_t v_angles, v_origin;

// 0 = off, 1 = pilot (idle), 2 = burning (attack held)
static int g_torchState = 0;
static Vector g_vTorchAttachment;
static model_s *g_torchFlameModel = nullptr;
static model_s *g_torchGlowModel = nullptr;
static char g_torchSprName[64] = "";

static cvar_t *cz_torch_spr = nullptr;

// view the attachment was taken in (the view model is drawn after the effect is created, so the
// attachment is one frame old: it is re-expressed in the current view to avoid lagging behind when turning)
static Vector g_vTorchViewOrg, g_vTorchViewAng;

void CZ_TorchSetAttachment( cl_entity_t *pEntity )
{
	if( g_torchState && gEngfuncs.GetViewModel() == pEntity )
	{
		g_vTorchAttachment = pEntity->attachment[0];
		g_vTorchViewOrg = v_origin;
		g_vTorchViewAng = v_angles;
	}
}

static Vector CzTorchPos()
{
	Vector f0, r0, u0, f1, r1, u1;
	gEngfuncs.pfnAngleVectors( g_vTorchViewAng, f0, r0, u0 );
	gEngfuncs.pfnAngleVectors( v_angles, f1, r1, u1 );
	const Vector d = g_vTorchAttachment - g_vTorchViewOrg;
	return v_origin + f1 * DotProduct( d, f0 ) + r1 * DotProduct( d, r0 ) + u1 * DotProduct( d, u0 );
}

class CPartTorchGlow : public CBaseParticle
{
public:
	void Think( float flTime ) override
	{
		m_vOrigin = CzTorchPos();
		CBaseParticle::Think( flTime );
	}
};

class CPartTorchPilot : public CBaseParticle
{
public:
	CPartTorchPilot() : CBaseParticle(), m_flForward( 0.0f )
	{
		m_prevTime = gEngfuncs.GetClientTime();
	}

	void Think( float flTime ) override
	{
		Vector forward, right, up;
		gEngfuncs.pfnAngleVectors( v_angles, forward, right, up );

		if( g_torchState == 2 )
		{
			const float r1 = Com_RandomFloat( -0.065f, 0.065f );
			const float r2 = Com_RandomFloat( -0.08f, 0.08f );
			m_vOrigin = CzTorchPos() + forward * m_flForward + r2 * right + r1 * up;
		}
		else
		{
			const float r1 = Com_RandomFloat( -0.05f, 0.05f );
			const float r2 = Com_RandomFloat( -0.05f, 0.05f );
			m_vOrigin = CzTorchPos() + forward * m_flForward / 1.5f + r2 * right + r1 * up;
		}

		const float now = gEngfuncs.GetClientTime();
		m_flForward += 48.0f * fabsf( now - m_prevTime );
		m_prevTime = now;

		CBaseParticle::Think( flTime );

		if( g_torchState == 0 )
			m_flDieTime = flTime;
	}

	float m_flForward;
	float m_prevTime;
};

static void CzTorchLoadSprites()
{
	if( !cz_torch_spr )
		cz_torch_spr = gEngfuncs.pfnRegisterVariable( "cz_torch_spr", "sprites/flare3.spr", 0 );

	if( strcmp( g_torchSprName, cz_torch_spr->string ) || !g_torchFlameModel )
	{
		snprintf( g_torchSprName, sizeof g_torchSprName, "%s", cz_torch_spr->string );
		HSPRITE h = SPR_Load( g_torchSprName );
		g_torchFlameModel = h ? (model_s *)gEngfuncs.GetSpritePointer( h ) : nullptr;
		g_torchGlowModel = g_torchFlameModel;
	}
}

static void CreatePilotFlame( const Vector &origin )
{
	if( !g_torchFlameModel )
		return;

	CPartTorchPilot *p = new CPartTorchPilot();
	if( !p )
		return;

	const float now = gEngfuncs.GetClientTime();
	const bool firing = g_torchState == 2;
	p->InitializeSprite( origin, Vector(), g_torchFlameModel, firing ? 1.1f : 1.0f, 255 );
	p->SetLightFlag( LIGHT_NONE );
	p->SetRenderFlag( RENDER_FACEPLAYER );
	p->m_iRendermode = kRenderTransAdd;
	p->m_flFadeSpeed = 0.0f;

	if( firing )
	{
		p->m_flScaleSpeed = -0.1f;
		p->m_vColor = Vector( 255.0f, 175.0f, 175.0f );
		p->m_flDieTime = now + 0.12f;
	}
	else
	{
		p->m_flScaleSpeed = -0.4f;
		p->m_vColor = Vector( 150.0f, 150.0f, 255.0f );
		p->m_flDieTime = now + 0.06f;
	}
}

// called every frame from HUD_CreateEntities
void CZ_TorchUpdate()
{
	static float flOldTime = 0.0f;

	const bool have = g_czTool == 3 && !g_czFiber && gEngfuncs.GetLocalPlayer() && !gEngfuncs.IsSpectateOnly();
	if( !have )
	{
		g_torchState = 0;
		return;
	}
	g_torchState = ( gHUD.m_iKeyBits & IN_ATTACK ) ? 2 : 1;

	const float now = gEngfuncs.GetClientTime();
	if( fabsf( now - flOldTime ) <= 0.02f )
		return;

	CzTorchLoadSprites();

	const bool firing = g_torchState == 2;

	if( g_torchGlowModel )
	{
		CPartTorchGlow *glow = new CPartTorchGlow();
		if( glow )
		{
			glow->InitializeSprite( CzTorchPos(), Vector(), g_torchGlowModel,
				firing ? Com_RandomFloat( 11.0f, 12.0f ) : Com_RandomFloat( 4.0f, 5.0f ), 225 );
			glow->SetLightFlag( LIGHT_NONE );
			glow->SetRenderFlag( RENDER_FACEPLAYER );
			glow->m_iRendermode = kRenderTransAdd;
			glow->m_flFadeSpeed = 0.0f;
			glow->m_flScaleSpeed = 0.05f;
			glow->m_vColor = Vector( 50, 30, 30 );
			glow->m_flDieTime = now + 0.08f;
		}
	}

	dlight_t *light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	if( light )
	{
		VectorCopy( CzTorchPos(), light->origin );
		if( firing )
		{
			light->radius = 75.0f;
			light->decay = 300.0f;
			light->color.r = 255; light->color.g = 190; light->color.b = 40;
		}
		else
		{
			light->radius = 50.0f;
			light->decay = 25.0f;
			light->color.r = 225; light->color.g = 190; light->color.b = 40;
		}
		light->die = now + 0.08f;
	}

	CreatePilotFlame( CzTorchPos() );

	flOldTime = now;
}

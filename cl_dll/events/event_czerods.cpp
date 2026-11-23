// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#include "events.h"
#include "com_model.h"

void EV_FireM60( event_args_t *args )
{
	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector vecSrc, vecAiming;
	int    idx = args->entindex;
	Vector origin( args->origin );
	Vector angles(
		args->iparam1 / 100.0f + args->angles[0],
		args->iparam2 / 100.0f + args->angles[1],
		args->angles[2]
		);
	Vector velocity( args->velocity );
	Vector forward, right, up;

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(Com_RandomLong(1,2), 2);
		if( !gHUD.cl_righthand->value )
		{
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -10.0, -13.0, true);
		}
		else
		{
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -10.0, 13.0, 0);
		}
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -12.0, 4.0, 0);
	}


	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], g_iRShell, TE_BOUNCE_SHELL);

	PLAY_EVENT_SOUND( "weapons/m60-1.wav" );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	Vector vSpread;

	vSpread.x = args->fparam1;
	vSpread.y = args->fparam2;
	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, vecAiming,
		vSpread, 8192.0, BULLET_PLAYER_762MM,
		2 );
}

void EV_FireCamera( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 1, 2 );

	PLAY_EVENT_SOUND( "weapons/camera-1.wav" );
}
void EV_FireFiberOpticCamera( event_args_t *args )
{
}
// the shield with the pistol (v_shield_r.mdl): the server sends the punch angles * 10000000, the sounds are of the Five-seveN
void EV_FireShieldGun( event_args_t *args )
{
	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector vecSrc, vecAiming;

	int idx = args->entindex;
	Vector origin( args->origin );
	Vector angles(
		args->iparam1 / 10000000.0f + args->angles[0],
		args->iparam2 / 10000000.0f + args->angles[1],
		args->angles[2] );
	Vector velocity( args->velocity );
	Vector forward, right, up;

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		// SHIELDGUN_SHOOT1 = 1, SHIELDGUN_SHOOT2 = 2, SHIELDGUN_SHOOT_EMPTY = 3
		gEngfuncs.pEventAPI->EV_WeaponAnimation( args->bparam1 ? 3 : Com_RandomLong( 1, 2 ), 2 );
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36.0, -14.0, 14.0, 0 );
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -12.0, 4.0, 0 );
	}

	EV_EjectBrass( ShellOrigin, ShellVelocity, angles[YAW], g_iPShell, TE_BOUNCE_SHELL );

	PLAY_EVENT_SOUND( "weapons/fiveseven-1.wav" );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread( args->fparam1, args->fparam2, 0.0f );

	EV_HLDM_FireBullets( idx,
		forward, right, up,
		1, vecSrc, vecAiming,
		vSpread, 8192.0, BULLET_PLAYER_57MM,
		2 );
}

int g_iBlowTorchFiring = 0;
void EV_HolsterBlowtorch( event_args_t *args )
{
	g_iBlowTorchFiring = 0;
}
void EV_IdleBlowtorch( event_args_t *args )
{
	g_iBlowTorchFiring = 1;
}

void EV_FireBlowtorch( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;
	Vector angles(
		args->iparam1 / 10000000.0f + args->angles[0],
		args->iparam2 / 10000000.0f + args->angles[1],
		args->angles[2] );

	Vector forward, right, up;
	Vector vecSrc;

	AngleVectors( angles, forward, right, up );

	EV_GetGunPosition( args, vecSrc, origin );

	if ( EV_IsLocal(idx) )
	  gEngfuncs.pEventAPI->EV_WeaponAnimation(1, 2);

	g_iBlowTorchFiring = 2;

	Vector vSpread( args->fparam1, args->fparam2, 0.0f );

	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, forward,
		vSpread, 36.0, BULLET_PLAYER_BLOWTORCH,
		2 );

}
void EV_FireLaws( event_args_t *args )
{

}
void EV_FireBriefcase( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 3, 2 );

	PLAY_EVENT_SOUND( "weapons/briefcase_use.wav" );
}
void EV_FireMedkit( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, 2 );

	PLAY_EVENT_SOUND( "weapons/blowtorch-1.wav" );

}
void EV_FireSyringe( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, 2 );

	PLAY_EVENT_SOUND( "weapons/syringe_use.wav" );

}
void EV_FireRadio( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 1, 2 );

	PLAY_EVENT_SOUND( "weapons/radio_activate.wav" );
}
void EV_FireZipline( event_args_t *args )
{
	if ( EV_IsLocal(args->entindex) )
	  gEngfuncs.pEventAPI->EV_WeaponAnimation(3, 2);
}
void EV_CreateGlass( event_args_t *args )
{

}
// CZ:DS explosions. The original client draws them with particleman (fireball sprite, flares, dust and
// smoke rings, debris of the surface material); this is the same picture with the engine's temp
// entities: the fireball zerog-frag1 animated at 90 fps, smoke puffs gas_puff_01, a light and the sounds.
static TEMPENTITY *CZ_ExploSprite( const Vector &origin, const char *name, float size, int rendermode )
{
	HSPRITE hspr = SPR_Load( name );
	const struct model_s *mdl = hspr ? gEngfuncs.GetSpritePointer( hspr ) : NULL;

	if( !mdl )
	{
		gEngfuncs.Con_DPrintf( "Couldn't load Sprite: %s\n", name );
		return NULL;
	}

	TEMPENTITY *te = gEngfuncs.pEfxAPI->CL_TempEntAlloc( (float *)&origin, (struct model_s *)mdl );
	if( !te )
		return NULL;

	// the original sizes are the half width of the sprite quad in world units
	float width = mdl->maxs[1] - mdl->mins[1];
	if( width < 1.0f )
		width = 1.0f;

	te->entity.curstate.scale = size * 2.0f / width;
	te->entity.curstate.rendermode = rendermode;
	te->entity.curstate.renderamt = 255;
	te->entity.curstate.rendercolor.r = 255;
	te->entity.curstate.rendercolor.g = 255;
	te->entity.curstate.rendercolor.b = 255;
	te->entity.baseline.renderamt = 255;
	te->frameMax = mdl->numframes > 0 ? mdl->numframes - 1 : 0;
	te->flags &= ~FTENT_NOMODEL;

	return te;
}

static void CZ_ExploFireball( const Vector &origin )
{
	TEMPENTITY *te = CZ_ExploSprite( origin, "sprites/zerog-frag1.spr", 175.0f, kRenderTransAdd );
	if( !te )
		return;

	te->flags |= FTENT_SPRANIMATE;
	te->entity.curstate.framerate = 90;
	te->entity.curstate.rendercolor.g = 225;
	te->entity.curstate.rendercolor.b = 225;
	te->die = gEngfuncs.GetClientTime() + ( te->frameMax + 1 ) / 90.0f;
}

static void CZ_ExploSmoke( const Vector &origin, const Vector &velocity, float life, int color )
{
	TEMPENTITY *te = CZ_ExploSprite( origin, "sprites/gas_puff_01.spr", gEngfuncs.pfnRandomFloat( 140, 180 ) * 0.5f, kRenderTransAlpha );
	if( !te )
		return;

	te->flags |= FTENT_FADEOUT | FTENT_COLLIDEWORLD;
	te->entity.curstate.rendercolor.r = color;
	te->entity.curstate.rendercolor.g = color;
	te->entity.curstate.rendercolor.b = color;
	te->entity.curstate.frame = gEngfuncs.pfnRandomLong( 0, te->frameMax );
	te->entity.baseline.origin = velocity;
	te->fadeSpeed = 0.5f;
	te->die = gEngfuncs.GetClientTime() + life;
}

static void CZ_ExploSounds( const Vector &origin, bool law )
{
	static const char *sounds[] = { "weapons/explode3.wav", "weapons/explode4.wav", "weapons/explode5.wav" };

	// the original plays from the picked one to the last one, all at once
	for( int i = gEngfuncs.pfnRandomLong( 0, 2 ); i < 3; i++ )
	{
		gEngfuncs.pEventAPI->EV_PlaySound( -1, (float *)&origin, CHAN_AUTO,
			law ? "weapons/law_explode.wav" : sounds[i], 1.0f, ATTN_NORM, 0, PITCH_NORM );
	}
}

static void CZ_ExploLight( const Vector &origin )
{
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	if( !dl )
		return;

	dl->origin = origin;
	dl->radius = 300;
	dl->color.r = 255;
	dl->color.g = 190;
	dl->color.b = 40;
	dl->die = gEngfuncs.GetClientTime() + 1.0f;
	dl->decay = 275;
}

void EV_GrenadeExplosion( event_args_t *args )
{
	Vector origin( args->origin );
	Vector forward, right, up;

	CZ_ExploSounds( origin, args->bparam2 != 0 );
	CZ_ExploLight( origin );

	// the fireball on the ground under the blast
	pmtrace_t tr;
	Vector down = origin - Vector( 0, 0, 100 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, down, PM_NORMAL, -1, &tr );
	CZ_ExploFireball( tr.fraction < 1.0f ? Vector( tr.endpos ) : origin );

	// the smoke column and the smoke thrown aside
	for( int i = 0; i < 4; i++ )
	{
		Vector angles( 0.0f, i * 45.0f, 0.0f );
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		float height = gEngfuncs.pfnRandomFloat( 5, 25 );
		CZ_ExploSmoke( origin + up * height, up * ( 55.0f - height * 2.0f ) * 0.5f, 6.5f, 75 );
	}

	for( int i = 0; i < 8; i++ )
	{
		Vector angles( 0.0f, i * 45.0f, 0.0f );
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		float rise = gEngfuncs.pfnRandomFloat( 25, 40 ) * i;
		float speed = gEngfuncs.pfnRandomFloat( 300, 400 );
		int color = (int)gEngfuncs.pfnRandomFloat( 75, 100 );
		CZ_ExploSmoke( origin + up * ( 5.0f * i ), ( forward * speed + up * rise ) * 0.2f, 6.0f, color );
	}
}

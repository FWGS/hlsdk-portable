#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"
#include "game.h"
#include "player.h"
#include "shake.h"
#include "soundent.h"

extern DLL_GLOBAL int		g_restore_fix;

//=====================//
//Bullet Impact effects//
//=====================//
void FX_ImpBullet( Vector origin, Vector normal, Vector angles, int IsBsp, int type, float TexType )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgImpBullet );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_COORD( angles.x );
	WRITE_COORD( angles.y );
	WRITE_COORD( angles.z );
	WRITE_COORD( normal.x );
	WRITE_COORD( normal.y );
	WRITE_COORD( normal.z );
	WRITE_BYTE( IsBsp );
	WRITE_BYTE( type );
	WRITE_BYTE( TexType );
	MESSAGE_END();
	}
}

//=====================//
//Rocket Impact effects//
//=====================//
void FX_ImpRocket( Vector origin, Vector angles, int IsBsp, int type, float TexType )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgImpRocket );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_COORD( angles.x );
	WRITE_COORD( angles.y );
	WRITE_COORD( angles.z );
	WRITE_BYTE( IsBsp );
	WRITE_BYTE( type );
	WRITE_BYTE( TexType );
	MESSAGE_END();
	}
}

//===================//
//Beam Impact effects//
//===================//
void FX_ImpBeam( Vector origin, Vector angles, int IsBsp, int type )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgImpBeam );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_COORD( angles.x );
	WRITE_COORD( angles.y );
	WRITE_COORD( angles.z );
	WRITE_BYTE( IsBsp );
	WRITE_BYTE( type );
	MESSAGE_END();
	}
}

//=========================//
//Generic Explosion effects//
//=========================//
void FX_Explosion( Vector origin, int type )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgExplosion );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_BYTE( type );
	MESSAGE_END();
	}
}

//================//
//Beam gun Effects//
//================//
void FX_FireBeam( Vector origin, Vector angles, Vector normal, int Type )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgFireBeam );
	WRITE_COORD( origin.x );//start point
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_COORD( angles.x );//end point
	WRITE_COORD( angles.y );
	WRITE_COORD( angles.z );
	WRITE_COORD( normal.x );//normal
	WRITE_COORD( normal.y );
	WRITE_COORD( normal.z );
	WRITE_BYTE( Type );
	MESSAGE_END();
	}
}

//===================//
//weapon fire effects//
//===================//
void FX_FireGun(Vector angles, int EntIndex, int Animation, int Special, int Type )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgFireGun );
	WRITE_COORD( angles.x );
	WRITE_COORD( angles.y );
	WRITE_COORD( angles.z );
	WRITE_BYTE( EntIndex );
	WRITE_BYTE( Animation );
	WRITE_BYTE( Special );
	WRITE_BYTE( Type );
	MESSAGE_END();
	}
}

//===================//
//enpty clip ejecting//
//===================//
void FX_BrassClip( Vector origin, Vector angles, int EntIndex, int Type )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgBrassClip );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_COORD( angles.x );
	WRITE_COORD( angles.y );
	WRITE_COORD( angles.z );
	WRITE_BYTE( EntIndex );
	WRITE_BYTE( Type );
	MESSAGE_END();
}

//==================//
//Player gib effects//
//==================//
void FX_PlrGib( Vector origin, int Type )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgPlrGib );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_BYTE( Type );
	MESSAGE_END();
}

//===========================//
//Projectile trails & impacts//
//===========================//
void FX_Trail( Vector origin, int EntIndex, int Type )
{
	if(g_restore_fix == 0){
	MESSAGE_BEGIN( MSG_ALL, gmsgTrail );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_SHORT( EntIndex );
	WRITE_BYTE( Type );
	MESSAGE_END();
	}
}

//===================================
//func_breakable & func_pushable gibs
//===================================
void FX_BreakGib( Vector origin, int Velocity, int Scale, int Amount, int Type )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgBreakGib );
	WRITE_COORD( origin.x );
	WRITE_COORD( origin.y );
	WRITE_COORD( origin.z );
	WRITE_SHORT( Velocity );
	WRITE_BYTE( Scale );
	WRITE_BYTE( Amount );
	WRITE_BYTE( Type );
	MESSAGE_END();
}
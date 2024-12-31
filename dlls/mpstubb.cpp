/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"soundent.h"
#include	"nodes.h"
#include	"talkmonster.h"

float	CTalkMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once

/*********************************************************/

CGraph WorldGraph;
void CGraph::InitGraph( void ) { }
int CGraph::FLoadGraph( const char *szMapName ) { return FALSE; }
int CGraph::AllocNodes( void ) { return FALSE; }
int CGraph::CheckNODFile( const char *szMapName ) { return FALSE; }
int CGraph::FSetGraphPointers( void ) { return 0; }
void CGraph::ShowNodeConnections( int iNode ) { }
int CGraph::FindNearestNode( const Vector &vecOrigin, int afNodeTypes ) { return 0; }

/*********************************************************/

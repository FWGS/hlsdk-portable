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
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "animation.h"
// #define DUCKFIX

int game_savefucked_num;
int game_player_dead;
int game_boss_battle;

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL BOOL		g_fCantSave;
extern DLL_GLOBAL BOOL g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int g_iSkillLevel, gDisplayTitle;
extern DLL_GLOBAL int		g_restore_fix;
extern DLL_GLOBAL int		g_causality_add;
extern DLL_GLOBAL int		g_gibexp_max;
extern DLL_GLOBAL BOOL		g_StartDark;
extern DLL_GLOBAL int		g_fGameSkipCG;
extern DLL_GLOBAL int		g_fGameJumpCG;

extern DLL_GLOBAL BOOL		g_Spawnpreacheally;

BOOL gInitHUD = TRUE;

extern void CopyToBodyQue( entvars_t *pev);
extern void respawn( entvars_t *pev, BOOL fCopyCorpse );
extern Vector VecBModelOrigin( entvars_t *pevBModel );
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

extern DLL_GLOBAL int			g_Language;

// the world node graph
extern CGraph WorldGraph;

#define TRAIN_ACTIVE		0x80
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL		0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM		0x03
#define TRAIN_FAST		0x04
#define TRAIN_BACK		0x05

#define	FLASH_DRAIN_TIME	 1.5f //100 units/3 minutes
#define	FLASH_CHARGE_TIME	 0.4f // 100 units/20 seconds  (seconds per unit)

// Global Savedata for player
TYPEDESCRIPTION	CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_IntoWaterTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_RecoverTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_ClimbWallTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_MonsterCatchTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_air_oxyan_stop_time, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),

	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tSneaking, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_kadoma_exp, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_kadoma_level, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_kadoma_skill, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_skill_reload, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_defguard, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_longjump, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_punch, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_valvesword, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_respawn, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_darkhide, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_deathmatch, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_wrongdoor, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_miss, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_goddam, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_skill_locked, FIELD_INTEGER ),
	
	DEFINE_FIELD( CBasePlayer, m_skill_darkhide_on, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( CBasePlayer, m_level_up_switch, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, m_concussion_time, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_skill_respawn_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_fldarkhideTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_mode_int1, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_mode_int2, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_mode_int3, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_mode_float1, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_mode_float2, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_mode_origin, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( CBasePlayer, m_sword_aim_origin, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( CBasePlayer, m_iNVG, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fSecondWorld, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fPlayerHideMode, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, m_fDeadRespawn, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_old_Respawn_origin, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_fMoveItem, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_fContPoint, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_fContPoint_type, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_hasflashlight, FIELD_BOOLEAN ),//Item 1 - ����
	DEFINE_FIELD( CBasePlayer, m_fMask, FIELD_BOOLEAN ),//Item 2 - �����
	DEFINE_FIELD( CBasePlayer, m_fGlodenKey, FIELD_INTEGER ),//Item 3 - ���Կ�ף��ɶ���
	DEFINE_FIELD( CBasePlayer, m_fGenerenKey, FIELD_BOOLEAN ),//Item 4 - ͨ�Կ��
	DEFINE_FIELD( CBasePlayer, m_fSecurityKey, FIELD_BOOLEAN ),//Item 5 - ��ȫԿ��
	DEFINE_FIELD( CBasePlayer, m_fBloodlyKey, FIELD_BOOLEAN ),//Item 6 - ȾѪԿ��
	DEFINE_FIELD( CBasePlayer, m_fGreenCard, FIELD_BOOLEAN ),//Item 7 - �ɫ�ſ�
	DEFINE_FIELD( CBasePlayer, m_fSecurityCard, FIELD_BOOLEAN ),//Item 8 - ��ȫ�ſ�
	DEFINE_FIELD( CBasePlayer, m_fValve, FIELD_BOOLEAN ),//Item 9 - ���

	DEFINE_FIELD( CBasePlayer, m_fequip1, FIELD_BOOLEAN ),//װ��1 - ��ģ�����+20%��
	DEFINE_FIELD( CBasePlayer, m_fequip2, FIELD_BOOLEAN ),//װ��2 - ��������+10%��
//	DEFINE_FIELD( CBasePlayer, m_fequip3, FIELD_BOOLEAN ),//װ��3 - ���ñ���Ծ��+150%���ȹ�˺���ӣ�
	DEFINE_FIELD( CBasePlayer, m_fequip4, FIELD_BOOLEAN ),//װ��4 - ��£������+25%��
	DEFINE_FIELD( CBasePlayer, m_fequip5, FIELD_BOOLEAN ),//װ��5 - ��֮��磨�������������ڰ������
	DEFINE_FIELD( CBasePlayer, m_fequip6, FIELD_BOOLEAN ),//װ��6 - ����ˮ����������+100%��

	DEFINE_FIELD( CBasePlayer, m_fSelectMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fSelectNumber, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flNextSoundTime1, FIELD_FLOAT ), 

	DEFINE_FIELD( CBasePlayer, m_wdoor_mynpc, FIELD_EDICT ),//���NPC
// rain tutorial
/*
	DEFINE_FIELD( CBasePlayer, Rain_dripsPerSecond, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, Rain_windX, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_windY, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_randX, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_randY, FIELD_FLOAT ),

	DEFINE_FIELD( CBasePlayer, Rain_ideal_dripsPerSecond, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, Rain_ideal_windX, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_ideal_windY, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_ideal_randX, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_ideal_randY, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, Rain_endFade, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, Rain_nextFadeUpdate, FIELD_TIME ),
*/
	DEFINE_FIELD( CBasePlayer, m_fNextClearTextTime, FIELD_TIME ),
	//======================================
	DEFINE_FIELD( CBasePlayer, m_newcross_active, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_newcross_size, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_newcross_ontarget, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_stuck_origin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_old_teleprort_origin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_teleprort_in_xen, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_god_time, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flVelocityModifier, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flVelocityModifier2, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_needleheal, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_needleheal2, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_enemy_kills, FIELD_INTEGER ),//ɱ���
	DEFINE_FIELD( CBasePlayer, m_ending_frags, FIELD_INTEGER ),//�Ʒֵ
	DEFINE_FIELD( CBasePlayer, m_player_diamonds, FIELD_INTEGER ),//�ʯ�
	DEFINE_FIELD( CBasePlayer, m_game_rate, FIELD_INTEGER ),//�Ϸ����
	DEFINE_FIELD( CBasePlayer, m_player_time, FIELD_FLOAT ),//�Ϸʱ��

	DEFINE_FIELD( CBasePlayer, m_vecClimb, FIELD_VECTOR ),

	DEFINE_FIELD( CBasePlayer, m_player_died, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, m_title, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_trainning, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_music_save, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_skill_maxarmor, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_deadtakedmgkill, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_godposion, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_needlekilled_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_wrongdoor_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_wrongdoor_cover_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_needleuse_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_swordrecover_time, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_save_check, FIELD_INTEGER ),//�浵�ͷ��������������������������ǻ��ʧ
	DEFINE_FIELD( CBasePlayer, m_save_allow, FIELD_INTEGER ),//�����浵

	DEFINE_FIELD( CBasePlayer, m_air_oxyan, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_air_oxyan_max, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_air_show, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_Fast_RTP_Show, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_barnacle_draw_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_god_time, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_RTP, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_RTP_relase, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_RTP_bar, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_RTP_button, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_Level, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_barnacle_catchme, FIELD_EHANDLE ),

	DEFINE_FIELD( CBasePlayer, m_team_npc1, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc2, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc3, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc4, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc5, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc6, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc7, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc8, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc9, FIELD_EHANDLE ),//���
	DEFINE_FIELD( CBasePlayer, m_team_npc10, FIELD_EHANDLE ),//���0
	DEFINE_FIELD( CBasePlayer, m_team_npc11, FIELD_EHANDLE ),//���1
	DEFINE_FIELD( CBasePlayer, m_team_npc12, FIELD_EHANDLE ),//���2

	DEFINE_FIELD( CBasePlayer, m_team_prot, FIELD_EHANDLE ),//����
	DEFINE_FIELD( CBasePlayer, m_player_camera, FIELD_EHANDLE ),//��ͷ

	DEFINE_FIELD( CBasePlayer, m_rpg_menu_actor1, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_actor2, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_actor3, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_actor4, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_actor5, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item_e, FIELD_INTEGER ),//װ���ο��Ʒ
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item_t, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item1, FIELD_INTEGER ),//�Ʒ��
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item2, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item3, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item4, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item5, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item6, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item7, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item8, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item9, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item10, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item11, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_item12, FIELD_INTEGER ),
	/*
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill1, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill2, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill3, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill4, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill5, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill6, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill7, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill8, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill9, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_menu_skill_chater, FIELD_INTEGER ),
	*/

	DEFINE_FIELD( CBasePlayer, m_rpg_password_on, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_select, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light1, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light2, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light3, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light4, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light5, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light6, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light7, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light8, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_rpg_password_light9, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_boss_find, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_boss_pov_time, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_boss_on, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_boss_type, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flash_mode, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_guard_mynpc, FIELD_INTEGER ),

	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore
};	

int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgTeamNames = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0;

//============================
int gmsgDarkHoles = 0;
int gmsgHPbar = 0;
int gmsgAPbar = 0;
int gmsgNVG = 0;
int gmsgGunScope = 0;
int gmsgLifeLoad = 0;
int gmsgAirBar    = 0;
int gmsgRTPbar    = 0;
int gmsgGameOver  = 0;
int gmsgModeShow = 0;
//int gmsgMoney	 = 0;

int gmsgExplosion = 0;
int gmsgWorldExp = 0;
int gmsgImpBullet = 0;
int gmsgImpRocket = 0;
int gmsgImpBeam = 0;
int gmsgRain = 0;
int gmsgFireGun = 0;
int gmsgFireBeam = 0;
int gmsgBrassClip = 0;
int gmsgPlrGib = 0;
int gmsgTrail = 0;
int gmsgBreakGib = 0;

int gmsgVGUIMenu = 0; // VGUI
int gmsgTbutton = 0;

int gmsgRPGMenu = 0;//RPG״̬�˵�
int gmsgPWBord = 0;//����

void LinkUserMessages( void )
{
	// Already taken care of?
	if( gmsgSelAmmo )
	{
		return;
	}

	gmsgSelAmmo = REG_USER_MSG( "SelAmmo", sizeof(SelAmmo) );
	gmsgCurWeapon = REG_USER_MSG( "CurWeapon", 3 );
	gmsgGeigerRange = REG_USER_MSG( "Geiger", 1 );
	gmsgFlashlight = REG_USER_MSG( "Flashlight", 2 );
	gmsgFlashBattery = REG_USER_MSG( "FlashBat", 1 );
	gmsgHealth = REG_USER_MSG( "Health", 4 );
	gmsgDamage = REG_USER_MSG( "Damage", 12 );
	gmsgBattery = REG_USER_MSG( "Battery", 2);
	gmsgTrain = REG_USER_MSG( "Train", 1 );
	//gmsgHudText = REG_USER_MSG( "HudTextPro", -1 );
	gmsgHudText = REG_USER_MSG( "HudText", -1 ); // we don't use the message but 3rd party addons may!
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgWeaponList = REG_USER_MSG( "WeaponList", -1 );
	gmsgResetHUD = REG_USER_MSG( "ResetHUD", 1 );		// called every respawn
	gmsgInitHUD = REG_USER_MSG( "InitHUD", 0 );		// called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG( "GameTitle", 1 );
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 9 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 2 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 1 );
	gmsgItemPickup = REG_USER_MSG( "ItemPickup", -1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgShake = REG_USER_MSG( "ScreenShake", sizeof(ScreenShake) );
	gmsgFade = REG_USER_MSG( "ScreenFade", sizeof(ScreenFade) );
	gmsgAmmoX = REG_USER_MSG( "AmmoX", 3 );
	gmsgTeamNames = REG_USER_MSG( "TeamNames", -1 );

	gmsgTbutton	= REG_USER_MSG( "Tbutton", 2 );

	gmsgStatusText = REG_USER_MSG( "StatusText", -1 );
	gmsgStatusValue = REG_USER_MSG( "StatusValue", 3 );

	gmsgRPGMenu = REG_USER_MSG( "WRPGMenu", -1 );
	gmsgPWBord = REG_USER_MSG( "WPWBord", -1 );

   	gmsgHPbar = REG_USER_MSG("CheckHPbar", 5);
	gmsgAPbar = REG_USER_MSG("CheckAPbar", 5);
	gmsgDarkHoles = REG_USER_MSG( "FDarkHoles",3);

    gmsgNVG = REG_USER_MSG("NVGActivate", 2); 
	gmsgGunScope = REG_USER_MSG("FGunScope", 1); 
	gmsgLifeLoad = REG_USER_MSG("FLoadLife", 8); 
	gmsgAirBar    = REG_USER_MSG( "CheckAirbar",3);
	gmsgRTPbar    = REG_USER_MSG( "CheckRTPbar",3);
	gmsgGameOver  = REG_USER_MSG("FGameOver", 2); 
	gmsgModeShow = REG_USER_MSG("FModeShow", 2);

	gmsgRain = REG_USER_MSG("Rain", 15);
	gmsgImpBullet = REG_USER_MSG("ImpBullet", 21);
	gmsgImpRocket = REG_USER_MSG("ImpRocket", 15);
	gmsgImpBeam = REG_USER_MSG("ImpBeam", 14);
	gmsgExplosion = REG_USER_MSG("Explosion", 7);
	gmsgWorldExp = REG_USER_MSG("WorldExp", 13);
	gmsgFireBeam = REG_USER_MSG("FireBeam", 19);
	gmsgFireGun = REG_USER_MSG("FireGun", 10);
	gmsgBrassClip = REG_USER_MSG("BrassClip", 14);
	gmsgPlrGib = REG_USER_MSG("PlrGib", 7);
	gmsgTrail = REG_USER_MSG("Trail", 9);
	gmsgBreakGib = REG_USER_MSG("BreakGib", 11);

	gmsgVGUIMenu = REG_USER_MSG("VGUIMenu", 1);
}

LINK_ENTITY_TO_CLASS( player, CBasePlayer )

void CBasePlayer::Blind(float flUntilTime, float flHoldTime, float flFadeTime, int iAlpha)
{
	m_blindUntilTime = flUntilTime + gpGlobals->time;
	m_blindStartTime = gpGlobals->time;
	m_blindHoldTime = flHoldTime;
	m_blindFadeTime = flFadeTime;
	m_blindAlpha = iAlpha;
}


// Start 
void CBasePlayer::ShowVGUIMenu(int iMenuID)
{
	if(!m_fSelectMode && (iMenuID >= 31 && iMenuID <= 37) )
		return;

    MESSAGE_BEGIN(MSG_ONE, gmsgVGUIMenu, NULL, pev);
        WRITE_BYTE( iMenuID );
    MESSAGE_END();
}
// End 

/*
void CBasePlayer::Pain( void )
{
	float flRndSound;//sound randomizer

	flRndSound = RANDOM_FLOAT( 0.0f, 1.0f ); 

	if( flRndSound <= 0.33f )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
	else if( flRndSound <= 0.66f )	
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
	else
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
}*/

void CBasePlayer :: MenuItem_use( int iMenu_Item)
{
	int used_item = 0;
	int reset_item = 0;

	reset_use:
	if (iMenu_Item == 0)
	{
		used_item = m_rpg_menu_item1;
		if(reset_item == 1)
		{
			m_rpg_menu_item1 = 0;
		}
	}
	else if (iMenu_Item == 1)
	{
		used_item = m_rpg_menu_item2;
		if(reset_item == 1)
		{
			m_rpg_menu_item2 = 0;
		}
	}
	else if (iMenu_Item == 2)
	{
		used_item = m_rpg_menu_item3;
		if(reset_item == 1)
		{
			m_rpg_menu_item3 = 0;
		}
	}
	else if (iMenu_Item == 3)
	{
		used_item = m_rpg_menu_item4;
		if(reset_item == 1)
		{
			m_rpg_menu_item4 = 0;
		}
	}
	else if (iMenu_Item == 4)
	{
		used_item = m_rpg_menu_item5;
		if(reset_item == 1)
		{
			m_rpg_menu_item5 = 0;
		}
	}
	else if (iMenu_Item == 5)
	{
		used_item = m_rpg_menu_item6;
		if(reset_item == 1)
		{
			m_rpg_menu_item6 = 0;
		}
	}
	else if (iMenu_Item == 6)
	{
		used_item = m_rpg_menu_item7;
		if(reset_item == 1)
		{
			m_rpg_menu_item7 = 0;
		}
	}
	else if (iMenu_Item == 7)
	{
		used_item = m_rpg_menu_item8;
		if(reset_item == 1)
		{
			m_rpg_menu_item8 = 0;
		}
	}
	else if (iMenu_Item == 8)
	{
		used_item = m_rpg_menu_item9;
		if(reset_item == 1)
		{
			m_rpg_menu_item9 = 0;
		}
	}
	else if (iMenu_Item == 9)
	{
		used_item = m_rpg_menu_item10;
		if(reset_item == 1)
		{
			m_rpg_menu_item10 = 0;
		}
	}
	else if (iMenu_Item == 10)
	{
		used_item = m_rpg_menu_item11;
		if(reset_item == 1)
		{
			m_rpg_menu_item11 = 0;
		}
	}
	else if (iMenu_Item == 11)
	{
		used_item = m_rpg_menu_item12;
		if(reset_item == 1)
		{
			m_rpg_menu_item12 = 0;
		}
	}

	if(used_item != 0 && reset_item == 0)
	{
		if(used_item == 9 && m_skill_maxarmor < 300)
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

			m_skill_maxarmor = 100;
			pev->armorvalue = m_skill_maxarmor;
			reset_item = 1;
			m_rpg_menu_on = 0;
			goto reset_use;
		}
		else if(used_item == 10 && m_skill_maxarmor < 300)
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

			m_skill_maxarmor = 120;
			pev->armorvalue = m_skill_maxarmor;
			reset_item = 1;
			m_rpg_menu_on = 0;
			goto reset_use;
		}
		else if(used_item == 11 && m_skill_maxarmor < 300)
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

			m_skill_maxarmor = 150;
			pev->armorvalue = m_skill_maxarmor;
			reset_item = 1;
			m_rpg_menu_on = 0;
			goto reset_use;
		}
		else if(used_item == 12 && m_skill_maxarmor < 300)
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/ammopickup.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

			m_skill_maxarmor = 200;
			pev->armorvalue = m_skill_maxarmor;
			reset_item = 1;
			m_rpg_menu_on = 0;
			goto reset_use;
		}
		else if(used_item == 13)
		{
			FX_Explosion( Center(), 45);

			pev->health = pev->max_health;
			if (m_team_npc1 != NULL && m_team_npc1->pev->deadflag == DEAD_NO)
			{
				m_team_npc1->pev->health = m_team_npc1->pev->max_health;
				FX_Explosion( m_team_npc1->Center(), 45);
			}
			if (m_team_npc2 != NULL && m_team_npc2->pev->deadflag == DEAD_NO)
			{
				m_team_npc2->pev->health = m_team_npc2->pev->max_health;
				FX_Explosion( m_team_npc2->Center(), 45);
			}
			if (m_team_npc3 != NULL && m_team_npc3->pev->deadflag == DEAD_NO)
			{
				m_team_npc3->pev->health = m_team_npc3->pev->max_health;
				FX_Explosion( m_team_npc3->Center(), 45);
			}
			if (m_team_npc4 != NULL && m_team_npc4->pev->deadflag == DEAD_NO)
			{
				m_team_npc4->pev->health = m_team_npc4->pev->max_health;
				FX_Explosion( m_team_npc4->Center(), 45);
			}
			if (m_team_npc5 != NULL && m_team_npc5->pev->deadflag == DEAD_NO)
			{
				m_team_npc5->pev->health = m_team_npc5->pev->max_health;
			}
			if (m_team_npc6 != NULL && m_team_npc6->pev->deadflag == DEAD_NO)
			{
				m_team_npc6->pev->health = m_team_npc6->pev->max_health;
			}
			if (m_team_npc7 != NULL && m_team_npc7->pev->deadflag == DEAD_NO)
			{
				m_team_npc7->pev->health = m_team_npc7->pev->max_health;
			}
			if (m_team_npc8 != NULL && m_team_npc8->pev->deadflag == DEAD_NO)
			{
				m_team_npc8->pev->health = m_team_npc8->pev->max_health;
			}
			if (m_team_npc9 != NULL && m_team_npc9->pev->deadflag == DEAD_NO)
			{
				m_team_npc9->pev->health = m_team_npc9->pev->max_health;
			}
			if (m_team_npc10 != NULL && m_team_npc10->pev->deadflag == DEAD_NO)
			{
				m_team_npc10->pev->health = m_team_npc10->pev->max_health;
			}
			if (m_team_npc11 != NULL && m_team_npc11->pev->deadflag == DEAD_NO)
			{
				m_team_npc11->pev->health = m_team_npc11->pev->max_health;
			}
			if (m_team_npc12 != NULL && m_team_npc12->pev->deadflag == DEAD_NO)
			{
				m_team_npc12->pev->health = m_team_npc12->pev->max_health;
			}
					
			reset_item = 1;
					
			if(m_rpg_menu_on > 1)
			{
				m_rpg_menu_on = 1;
			}
			goto reset_use;
		}
		else if(used_item == 21)
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "newadd/shrinebuff.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			m_god_time = gpGlobals->time + 15.0;

			if(m_darkposion > 0)
			{
				m_skill_darkhide_on = FALSE;
				m_darkposion = 0;
				m_iClientHealth = -1;
				m_iClient_oxyan = -1;
				pev->flags &= ~FL_NOTARGET;
				m_fldarkhideTime = gpGlobals->time + 35;
			}

			m_godposion = 1;
			m_iClientHealth = -1;
			reset_item = 1;
			m_rpg_menu_on = 0;
			goto reset_use;
		}
		else if(used_item == 22)
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "info_player_coop" );
			if ( pEntity )
			{
				pev->origin = pEntity->pev->origin;
				UTIL_ScreenFade( this, Vector(32,255,32), 1, 1, 255, FFADE_IN );
				EMIT_SOUND(ENT(pev), CHAN_NETWORKVOICE_BASE, "debris/beamstart10.wav", 1, 0.7);
				reset_item = 1;
				m_rpg_menu_on = 0;
				goto reset_use;
			}
		}
	}
}

void CBasePlayer :: MenuItem_equip( int iMenu_Item)
{
	int equip_item = 0;
	if (iMenu_Item == 0)
	{
		equip_item = m_rpg_menu_item1;
	}
	else if (iMenu_Item == 1)
	{
		equip_item = m_rpg_menu_item2;
	}
	else if (iMenu_Item == 2)
	{
		equip_item = m_rpg_menu_item3;
	}
	else if (iMenu_Item == 3)
	{
		equip_item = m_rpg_menu_item4;
	}
	else if (iMenu_Item == 4)
	{
		equip_item = m_rpg_menu_item5;
	}
	else if (iMenu_Item == 5)
	{
		equip_item = m_rpg_menu_item6;
	}
	else if (iMenu_Item == 6)
	{
		equip_item = m_rpg_menu_item7;
	}
	else if (iMenu_Item == 7)
	{
		equip_item = m_rpg_menu_item8;
	}
	else if (iMenu_Item == 8)
	{
		equip_item = m_rpg_menu_item9;
	}
	else if (iMenu_Item == 9)
	{
		equip_item = m_rpg_menu_item10;
	}
	else if (iMenu_Item == 10)
	{
		equip_item = m_rpg_menu_item11;
	}
	else if (iMenu_Item == 11)
	{
		equip_item = m_rpg_menu_item12;
	}

	if(equip_item == 13 || equip_item == 21 || equip_item == 22 || equip_item == 12 || equip_item == 11 || equip_item == 9 || equip_item == 10)
	{
		if(m_rpg_menu_item_e == iMenu_Item && m_rpg_menu_item_t == equip_item)
		{
			m_rpg_menu_item_e = -1;
			m_rpg_menu_item_t = -1;
		}
		else
		{
			m_rpg_menu_item_e = iMenu_Item;
			m_rpg_menu_item_t = equip_item;
		}
		m_iClient_mynpc = -1;
	}
}

void CBasePlayer :: MenuItem_drop( int iMenu_Item)
{
	int drop_item = 0;
	int can_drop = 0;

	redrop:
	if (iMenu_Item == 0)
	{
		drop_item = m_rpg_menu_item1;
		if(can_drop == 1)
		{
			m_rpg_menu_item1 = 0;
		}
	}
	else if (iMenu_Item == 1)
	{
		drop_item = m_rpg_menu_item2;
		if(can_drop == 1)
		{
			m_rpg_menu_item2 = 0;
		}
	}
	else if (iMenu_Item == 2)
	{
		drop_item = m_rpg_menu_item3;
		if(can_drop == 1)
		{
			m_rpg_menu_item3 = 0;
		}
	}
	else if (iMenu_Item == 3)
	{
		drop_item = m_rpg_menu_item4;
		if(can_drop == 1)
		{
			m_rpg_menu_item4 = 0;
		}
	}
	else if (iMenu_Item == 4)
	{
		drop_item = m_rpg_menu_item5;
		if(can_drop == 1)
		{
			m_rpg_menu_item5 = 0;
		}
	}
	else if (iMenu_Item == 5)
	{
		drop_item = m_rpg_menu_item6;
		if(can_drop == 1)
		{
			m_rpg_menu_item6 = 0;
		}
	}
	else if (iMenu_Item == 6)
	{
		drop_item = m_rpg_menu_item7;
		if(can_drop == 1)
		{
			m_rpg_menu_item7 = 0;
		}
	}
	else if (iMenu_Item == 7)
	{
		drop_item = m_rpg_menu_item8;
		if(can_drop == 1)
		{
			m_rpg_menu_item8 = 0;
		}
	}
	else if (iMenu_Item == 8)
	{
		drop_item = m_rpg_menu_item9;
		if(can_drop == 1)
		{
			m_rpg_menu_item9 = 0;
		}
	}
	else if (iMenu_Item == 9)
	{
		drop_item = m_rpg_menu_item10;
		if(can_drop == 1)
		{
			m_rpg_menu_item10 = 0;
		}
	}
	else if (iMenu_Item == 10)
	{
		drop_item = m_rpg_menu_item11;
		if(can_drop == 1)
		{
			m_rpg_menu_item11 = 0;
		}
	}
	else if (iMenu_Item == 11)
	{
		drop_item = m_rpg_menu_item12;
		if(can_drop == 1)
		{
			m_rpg_menu_item12 = 0;
		}
	}

	if(drop_item != 0 && can_drop == 0)
	{
		if(drop_item >= 9 && drop_item <= 13 || drop_item == 21 || drop_item == 2)
		{
			can_drop = 1;

			if(m_rpg_menu_item_e == iMenu_Item)
			{
				m_rpg_menu_item_e = -1;
				m_rpg_menu_item_t = -1;
				m_iClient_mynpc   = -1;
			}

			CBaseEntity *pDropItem = Create("item_dropusekey", pev->origin, Vector(0,pev->angles.y,0) );
			if(drop_item == 13)
			{
				pDropItem->pev->frags = 1;
			}
			else if(drop_item == 21)
			{
				pDropItem->pev->frags = 2;
			}
			else if(drop_item == 12)
			{
				pDropItem->pev->frags = 3;
			}
			else if(drop_item == 11)
			{
				pDropItem->pev->frags = 4;
			}
			else if(drop_item == 9)
			{
				pDropItem->pev->frags = 5;
			}
			else if(drop_item == 10)
			{
				pDropItem->pev->frags = 6;
			}
			else if(drop_item == 2)
			{
				pDropItem->pev->frags = 7;
				m_fMask = FALSE;
			}
			
			goto redrop;
		}
	}
}

void CBasePlayer :: MenuItem_add( int iMenu_Item )
{
	if (m_rpg_menu_item1 == 0)
	{
		m_rpg_menu_item1 = iMenu_Item;
	}
	else if (m_rpg_menu_item2 == 0){
		m_rpg_menu_item2 = iMenu_Item;
	}
	else if (m_rpg_menu_item3 == 0)
	{
		m_rpg_menu_item3 = iMenu_Item;
	}
	else if (m_rpg_menu_item4 == 0)
	{
		m_rpg_menu_item4 = iMenu_Item;
	}
	else if (m_rpg_menu_item5 == 0)
	{
		m_rpg_menu_item5 = iMenu_Item;
	}
	else if (m_rpg_menu_item6 == 0)
	{
		m_rpg_menu_item6 = iMenu_Item;
	}
	else if (m_rpg_menu_item7 == 0)
	{
		m_rpg_menu_item7 = iMenu_Item;
	}
	else if (m_rpg_menu_item8 == 0)
	{
		m_rpg_menu_item8 = iMenu_Item;
	}
	else if (m_rpg_menu_item9 == 0)
	{
		m_rpg_menu_item9 = iMenu_Item;
	}
	else if (m_rpg_menu_item10 == 0)
	{
		m_rpg_menu_item10 = iMenu_Item;
	}
	else if (m_rpg_menu_item11 == 0)
	{
		m_rpg_menu_item11 = iMenu_Item;
	}
	else if (m_rpg_menu_item12 == 0)
	{
		m_rpg_menu_item12 = iMenu_Item;
	}
}

void CBasePlayer :: MenuItem_remove( int iMenu_Item_ID )
{
	if (m_rpg_menu_item1 == iMenu_Item_ID)
	{
		m_rpg_menu_item1 = 0;
	}
	else if (m_rpg_menu_item2 == iMenu_Item_ID)
	{
		m_rpg_menu_item2 = 0;
	}
	else if (m_rpg_menu_item3 == iMenu_Item_ID)
	{
		m_rpg_menu_item3 = 0;
	}
	else if (m_rpg_menu_item4 == iMenu_Item_ID)
	{
		m_rpg_menu_item4 = 0;
	}
	else if (m_rpg_menu_item5 == iMenu_Item_ID)
	{
		m_rpg_menu_item5 = 0;
	}
	else if (m_rpg_menu_item6 == iMenu_Item_ID)
	{
		m_rpg_menu_item6 = 0;
	}
	else if (m_rpg_menu_item7 == iMenu_Item_ID)
	{
		m_rpg_menu_item7 = 0;
	}
	else if (m_rpg_menu_item8 == iMenu_Item_ID)
	{
		m_rpg_menu_item8 = 0;
	}
	else if (m_rpg_menu_item9 == iMenu_Item_ID)
	{
		m_rpg_menu_item9 = 0;
	}
	else if (m_rpg_menu_item10 == iMenu_Item_ID)
	{
		m_rpg_menu_item10 = 0;
	}
	else if (m_rpg_menu_item11 == iMenu_Item_ID)
	{
		m_rpg_menu_item11 = 0;
	}
	else if (m_rpg_menu_item12 == iMenu_Item_ID)
	{
		m_rpg_menu_item12 = 0;
	}
}


void CBasePlayer :: TeamMate_expadd( CBaseMonster *pAllynpc , CBaseMonster *pKillnpc)
{
	if(!pKillnpc->m_killed_exp)
		return;

	int get_exp = 0;
	int level_fabs = pKillnpc->m_rpgms_level - m_kadoma_level;

	if(m_rpg_menu_actor1 == 1)
	{
		if(pKillnpc->m_flPlayerDamage_exp >= pKillnpc->pev->max_health * 0.5 || pKillnpc->m_is_the_boss || pAllynpc == NULL)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else if(pKillnpc->m_flPlayerDamage_exp >= pKillnpc->pev->max_health * 0.2 || !FNullEnt(m_wdoor_mynpc))
		{
			get_exp = pKillnpc->m_killed_exp * 0.5;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.3;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}

		if(get_exp >= 1)
		{
			m_kadoma_exp += get_exp;
			m_enemy_kills += 1;
		}
		//ALERT ( at_console, "Player Exp %d\n", get_exp );
	}
	else
	{
		get_exp = pKillnpc->m_killed_exp * 0.2;

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			m_kadoma_exp += get_exp;
			m_enemy_kills += 1;
		}
		//ALERT ( at_console, "Player Exp %d\n", get_exp );
	}

	if(pAllynpc != NULL)
	{
		level_fabs = pKillnpc->m_rpgms_level - pAllynpc->m_rpgms_level;
	}

	CBaseMonster *pEnemyMonster;
	if(m_team_npc1 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_flPlayerTeamMateDamage_exp1 >= pKillnpc->pev->max_health * 0.5  || pKillnpc->m_is_the_boss || pAllynpc == m_team_npc1)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else if(pKillnpc->m_flPlayerTeamMateDamage_exp1 >= pKillnpc->pev->max_health * 0.2)
		{
			get_exp = pKillnpc->m_killed_exp * 0.5;
		}
		else if(m_team_npc1->pev->deadflag == DEAD_NO)
		{
			get_exp = pKillnpc->m_killed_exp * 0.3;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}

		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
		//ALERT ( at_console, "TeamNPC1 Exp %d\n", get_exp );
	}
	if(m_team_npc2 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_flPlayerTeamMateDamage_exp2 >= pKillnpc->pev->max_health * 0.5 || pKillnpc->m_is_the_boss || pAllynpc == m_team_npc2)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else if(pKillnpc->m_flPlayerTeamMateDamage_exp2 >= pKillnpc->pev->max_health * 0.2)
		{
			get_exp = pKillnpc->m_killed_exp * 0.5;
		}
		else if(m_team_npc2->pev->deadflag == DEAD_NO)
		{
			get_exp = pKillnpc->m_killed_exp * 0.3;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
		//ALERT ( at_console, "TeamNPC2 Exp %d\n", get_exp );
	}
	if(m_team_npc3 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_flPlayerTeamMateDamage_exp3 >= pKillnpc->pev->max_health * 0.5 || pKillnpc->m_is_the_boss || pAllynpc == m_team_npc3)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else if(pKillnpc->m_flPlayerTeamMateDamage_exp3 >= pKillnpc->pev->max_health * 0.2)
		{
			get_exp = pKillnpc->m_killed_exp * 0.5;
		}
		else if(m_team_npc3->pev->deadflag == DEAD_NO)
		{
			get_exp = pKillnpc->m_killed_exp * 0.3;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
		//ALERT ( at_console, "TeamNPC3 Exp %d\n", get_exp );
	}
	if(m_team_npc4 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_flPlayerTeamMateDamage_exp4 >= pKillnpc->pev->max_health * 0.5 || pKillnpc->m_is_the_boss || pAllynpc == m_team_npc4)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else if(pKillnpc->m_flPlayerTeamMateDamage_exp4 >= pKillnpc->pev->max_health * 0.2)
		{
			get_exp = pKillnpc->m_killed_exp * 0.5;
		}
		else if(m_team_npc4->pev->deadflag == DEAD_NO)
		{
			get_exp = pKillnpc->m_killed_exp * 0.3;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
		//ALERT ( at_console, "TeamNPC4 Exp %d\n", get_exp );
	}
	if(m_team_npc5 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc5->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
		//ALERT ( at_console, "TeamNPC5-12 Exp %d\n", get_exp );
	}
	if(m_team_npc6 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc6->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
	if(m_team_npc7 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc7->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
	if(m_team_npc8 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc8->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
	if(m_team_npc9 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc9->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
	if(m_team_npc10 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc10->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
	if(m_team_npc11 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc11->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
	if(m_team_npc12 != NULL)
	{
		get_exp = 0;
		if(pKillnpc->m_is_the_boss)
		{
			get_exp = pKillnpc->m_killed_exp;
		}
		else
		{
			get_exp = pKillnpc->m_killed_exp * 0.2;
		}

		if(level_fabs >= 40)
		{
			get_exp *= 1.5;
		}
		else if(level_fabs > -20)
		{
			get_exp *= 1.0;
		}
		else if(level_fabs > -40)
		{
			get_exp *= 0.5;
		}
		else
		{
			get_exp *= 0.3;
		}


		if(get_exp >= 1)
		{
			pEnemyMonster = m_team_npc12->MyMonsterPointer();
			pEnemyMonster->m_rpgms_exp += get_exp;
			pEnemyMonster->m_rpgms_maxexp += get_exp;
		}
	}
}

void CBasePlayer :: TeamMate_add( CBaseMonster *pAllynpc )
{
	if(pAllynpc->m_lovehate <= 0 || pAllynpc->m_lovehate == 810)
		return;
	

	if (m_team_npc1 == NULL)
	{
		m_team_npc1 = pAllynpc;
		m_rpg_menu_actor2 = pAllynpc->m_rpgms_actor;
		pAllynpc->m_rpgms_inteam = 1;
	}
	else if (m_team_npc2 == NULL)
	{
		m_team_npc2 = pAllynpc;
		m_rpg_menu_actor3 = pAllynpc->m_rpgms_actor;
		pAllynpc->m_rpgms_inteam = 2;
	}
	else if (m_team_npc3 == NULL)
	{
		m_team_npc3 = pAllynpc;
		m_rpg_menu_actor4 = pAllynpc->m_rpgms_actor;
		pAllynpc->m_rpgms_inteam = 3;
	}
	else if (m_team_npc4 == NULL)
	{
		m_team_npc4 = pAllynpc;
		m_rpg_menu_actor5 = pAllynpc->m_rpgms_actor;
		pAllynpc->m_rpgms_inteam = 4;
	}
	else if (m_team_npc5 == NULL)
	{
		m_team_npc5 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc6 == NULL)
	{
		m_team_npc6 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc7 == NULL)
	{
		m_team_npc7 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc8 == NULL)
	{
		m_team_npc8 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc9 == NULL)
	{
		m_team_npc9 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc10 == NULL)
	{
		m_team_npc10 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc11 == NULL)
	{
		m_team_npc11 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}
	else if (m_team_npc12 == NULL)
	{
		m_team_npc12 = pAllynpc;
		pAllynpc->m_rpgms_inteam = 5;
		pAllynpc->Hunt_Stand_Set(0);
	}

	CBaseMonster *pMonster;
	if (m_team_npc1 != NULL)
	{
		pMonster = m_team_npc1->MyMonsterPointer();
		TeamMate_NPC_add(pMonster);
		if(pMonster->m_hPlayer == NULL)
		{
			pMonster->m_hPlayer = this;
		}
	}
	if (m_team_npc2 != NULL)
	{
		pMonster = m_team_npc2->MyMonsterPointer();
		TeamMate_NPC_add(pMonster);
		if(pMonster->m_hPlayer == NULL)
		{
			pMonster->m_hPlayer = this;
		}
	}
	if (m_team_npc3 != NULL)
	{
		pMonster = m_team_npc3->MyMonsterPointer();
		TeamMate_NPC_add(pMonster);
		if(pMonster->m_hPlayer == NULL)
		{
			pMonster->m_hPlayer = this;
		}
	}
	if (m_team_npc4 != NULL)
	{
		pMonster = m_team_npc4->MyMonsterPointer();
		TeamMate_NPC_add(pMonster);
		if(pMonster->m_hPlayer == NULL)
		{
			pMonster->m_hPlayer = this;
		}
	}
}

void CBasePlayer :: TeamMate_NPC_add( CBaseMonster *pAllynpc )
{
	if(m_team_npc1 != NULL && m_team_npc1 != pAllynpc)
	{
		pAllynpc->m_hTeamMate1 = m_team_npc1;
	}
	if(m_team_npc2 != NULL && m_team_npc2 != pAllynpc)
	{
		pAllynpc->m_hTeamMate2 = m_team_npc2;
	}
	if(m_team_npc3 != NULL && m_team_npc3 != pAllynpc)
	{
		pAllynpc->m_hTeamMate3 = m_team_npc3;
	}
	if(m_team_npc4 != NULL && m_team_npc4 != pAllynpc)
	{
		pAllynpc->m_hTeamMate4 = m_team_npc4;
	}
}

void CBasePlayer :: PassWordBordUse( int use )
{
	if(m_rpg_password_on == 20)
	{
		EnableControl(TRUE);
		m_rpg_password_on = 0;
		return;
	}

	if(m_rpg_password_on == 0 || m_rpg_password_on > 10)
		return;

	if(m_rpg_password_select == 0)
		m_rpg_password_select = 1;

	if(use == 1)
	{
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/ace_enter.wav", 0.4, ATTN_NORM);
		if(m_rpg_password_select == 1)
		{
			if(m_rpg_password_light1 == 1)
				m_rpg_password_light1 = 0;
			else
				m_rpg_password_light1 = 1;

			if(m_rpg_password_light2 == 1)
				m_rpg_password_light2 = 0;
			else
				m_rpg_password_light2 = 1;

			if(m_rpg_password_light4 == 1)
				m_rpg_password_light4 = 0;
			else
				m_rpg_password_light4 = 1;
		}
		else if(m_rpg_password_select == 2)
		{
			if(m_rpg_password_light1 == 1)
				m_rpg_password_light1 = 0;
			else
				m_rpg_password_light1 = 1;

			if(m_rpg_password_light2 == 1)
				m_rpg_password_light2 = 0;
			else
				m_rpg_password_light2 = 1;

			if(m_rpg_password_light3 == 1)
				m_rpg_password_light3 = 0;
			else
				m_rpg_password_light3 = 1;

			if(m_rpg_password_light5 == 1)
				m_rpg_password_light5 = 0;
			else
				m_rpg_password_light5 = 1;
		}
		else if(m_rpg_password_select == 3)
		{
			if(m_rpg_password_light3 == 1)
				m_rpg_password_light3 = 0;
			else
				m_rpg_password_light3 = 1;

			if(m_rpg_password_light2 == 1)
				m_rpg_password_light2 = 0;
			else
				m_rpg_password_light2 = 1;

			if(m_rpg_password_light6 == 1)
				m_rpg_password_light6 = 0;
			else
				m_rpg_password_light6 = 1;
		}
		else if(m_rpg_password_select == 4)
		{
			if(m_rpg_password_light1 == 1)
				m_rpg_password_light1 = 0;
			else
				m_rpg_password_light1 = 1;

			if(m_rpg_password_light4 == 1)
				m_rpg_password_light4 = 0;
			else
				m_rpg_password_light4 = 1;

			if(m_rpg_password_light5 == 1)
				m_rpg_password_light5 = 0;
			else
				m_rpg_password_light5 = 1;

			if(m_rpg_password_light7 == 1)
				m_rpg_password_light7 = 0;
			else
				m_rpg_password_light7 = 1;
		}
		else if(m_rpg_password_select == 5)
		{
			if(m_rpg_password_light2 == 1)
				m_rpg_password_light2 = 0;
			else
				m_rpg_password_light2 = 1;

			if(m_rpg_password_light4 == 1)
				m_rpg_password_light4 = 0;
			else
				m_rpg_password_light4 = 1;

			if(m_rpg_password_light5 == 1)
				m_rpg_password_light5 = 0;
			else
				m_rpg_password_light5 = 1;

			if(m_rpg_password_light6 == 1)
				m_rpg_password_light6 = 0;
			else
				m_rpg_password_light6 = 1;

			if(m_rpg_password_light8 == 1)
				m_rpg_password_light8 = 0;
			else
				m_rpg_password_light8 = 1;
		}
		else if(m_rpg_password_select == 6)
		{
			if(m_rpg_password_light3 == 1)
				m_rpg_password_light3 = 0;
			else
				m_rpg_password_light3 = 1;

			if(m_rpg_password_light5 == 1)
				m_rpg_password_light5 = 0;
			else
				m_rpg_password_light5 = 1;

			if(m_rpg_password_light6 == 1)
				m_rpg_password_light6 = 0;
			else
				m_rpg_password_light6 = 1;

			if(m_rpg_password_light9 == 1)
				m_rpg_password_light9 = 0;
			else
				m_rpg_password_light9 = 1;
		}
		else if(m_rpg_password_select == 7)
		{
			if(m_rpg_password_light4 == 1)
				m_rpg_password_light4 = 0;
			else
				m_rpg_password_light4 = 1;

			if(m_rpg_password_light7 == 1)
				m_rpg_password_light7 = 0;
			else
				m_rpg_password_light7 = 1;

			if(m_rpg_password_light8 == 1)
				m_rpg_password_light8 = 0;
			else
				m_rpg_password_light8 = 1;
		}
		else if(m_rpg_password_select == 8)
		{
			if(m_rpg_password_light5 == 1)
				m_rpg_password_light5 = 0;
			else
				m_rpg_password_light5 = 1;

			if(m_rpg_password_light7 == 1)
				m_rpg_password_light7 = 0;
			else
				m_rpg_password_light7 = 1;

			if(m_rpg_password_light8 == 1)
				m_rpg_password_light8 = 0;
			else
				m_rpg_password_light8 = 1;

			if(m_rpg_password_light9 == 1)
				m_rpg_password_light9 = 0;
			else
				m_rpg_password_light9 = 1;
		}
		else if(m_rpg_password_select == 9)
		{
			if(m_rpg_password_light6 == 1)
				m_rpg_password_light6 = 0;
			else
				m_rpg_password_light6 = 1;

			if(m_rpg_password_light8 == 1)
				m_rpg_password_light8 = 0;
			else
				m_rpg_password_light8 = 1;

			if(m_rpg_password_light9 == 1)
				m_rpg_password_light9 = 0;
			else
				m_rpg_password_light9 = 1;
		}

		if(m_rpg_password_light1 == 1 && m_rpg_password_light2 == 1
		&& m_rpg_password_light3 == 1 && m_rpg_password_light4 == 1
		&& m_rpg_password_light5 == 1 && m_rpg_password_light6 == 1
		&& m_rpg_password_light7 == 1 && m_rpg_password_light8 == 1
		&& m_rpg_password_light9 == 1)
		{
			m_rpg_password_on = 20;
			m_rpg_password_select = 0;
		}
	}
	if(use == 2)
	{
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/ace_select.wav", 0.4, ATTN_NORM);
		if(m_rpg_password_select == 1)
		{
			m_rpg_password_select = 7;
		}
		else if(m_rpg_password_select == 2)
		{
			m_rpg_password_select = 8;
		}
		else if(m_rpg_password_select == 3)
		{
			m_rpg_password_select = 9;
		}
		else
		{
			m_rpg_password_select -= 3;
		}
	}
	if(use == 3){
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/ace_select.wav", 0.4, ATTN_NORM);
		if(m_rpg_password_select == 7)
		{
			m_rpg_password_select = 1;
		}
		else if(m_rpg_password_select == 8)
		{
			m_rpg_password_select = 2;
		}
		else if(m_rpg_password_select == 9)
		{
			m_rpg_password_select = 3;
		}
		else
		{
			m_rpg_password_select += 3;
		}
	}
	if(use == 4)
	{
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/ace_select.wav", 0.4, ATTN_NORM);
		if(m_rpg_password_select == 1)
		{
			m_rpg_password_select = 3;
		}
		else if(m_rpg_password_select == 4)
		{
			m_rpg_password_select = 6;
		}
		else if(m_rpg_password_select == 7)
		{
			m_rpg_password_select = 9;
		}
		else
		{
			m_rpg_password_select -= 1;
		}
	}
	if(use == 5)
	{
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/ace_select.wav", 0.4, ATTN_NORM);
		if(m_rpg_password_select == 3)
		{
			m_rpg_password_select = 1;
		}
		else if(m_rpg_password_select == 6)
		{
			m_rpg_password_select = 4;
		}
		else if(m_rpg_password_select == 9)
		{
			m_rpg_password_select = 7;
		}
		else
		{
			m_rpg_password_select += 1;
		}
	}
}

void CBasePlayer :: TeamMate_GetSkill( CBaseMonster *pMonster )
{
	m_rpg_menu_skill_chater = pMonster->m_rpgms_actor;
	m_rpg_menu_skill1 = pMonster->m_rpgms_skill1_learn;
	m_rpg_menu_skill2 = pMonster->m_rpgms_skill2_learn;
	m_rpg_menu_skill3 = pMonster->m_rpgms_skill3_learn;
	m_rpg_menu_skill4 = pMonster->m_rpgms_skill4_learn;
	m_rpg_menu_skill5 = pMonster->m_rpgms_skill5_learn;
	m_rpg_menu_skill6 = pMonster->m_rpgms_skill6_learn;
	m_rpg_menu_skill7 = pMonster->m_rpgms_skill7_learn;
	m_rpg_menu_skill8 = pMonster->m_rpgms_skill8_learn;
	m_rpg_menu_skill9 = pMonster->m_rpgms_skill9_learn;
	m_rpg_menu_skill10 = pMonster->m_rpgms_skill10_learn;
	m_rpg_menu_skill11 = pMonster->m_rpgms_skill11_learn;
	m_rpg_menu_skill12 = pMonster->m_rpgms_skill12_learn;
}

void CBasePlayer :: GetGame_Playcvar( void )
{
	float gtime = m_player_time;
	float gtime_h = m_player_time / 3600;
	if(gtime_h >= 1)
	{
		gtime -= (int)gtime_h * 3600;
	}
	float gtime_m = gtime / 60;
	if(gtime_m >= 1)
	{
		gtime -= (int)gtime_m * 60;
	}
	float gtime_s = gtime;

	m_rpg_menu_skill1 = (int)gtime_h;
	m_rpg_menu_skill2 = (int)gtime_m;
	m_rpg_menu_skill3 = (int)gtime_s;
	m_rpg_menu_skill4 = g_iSkillLevel;
	m_rpg_menu_skill5 = m_game_rate;
	m_rpg_menu_skill6 = m_enemy_kills;
	m_rpg_menu_skill7 = m_player_diamonds;
	m_rpg_menu_skill8 = m_ending_frags;
}

void CBasePlayer :: TeamMate_Nagamatagi_Switch_Auto( CBaseMonster *pMonster )
{
	if(m_rpg_menu_actor1 != 1)
		return;

	int ally1 = 0;
	int ally2 = 0;
	if(pMonster->m_rpgms_inteam == 1)
	{
		ally1 = 1;
	}
	else if(pMonster->m_rpgms_inteam == 2)
	{
		ally1 = 2;
	}
	else if(pMonster->m_rpgms_inteam == 3)
	{
		ally1 = 3;
	}
	else if(pMonster->m_rpgms_inteam == 4)
	{
		ally1 = 4;
	}

	if(m_team_npc5 != NULL && m_team_npc5->pev->deadflag == DEAD_NO)
	{
		ally2 = 5;
	}
	else if(m_team_npc6 != NULL && m_team_npc6->pev->deadflag == DEAD_NO)
	{
		ally2 = 6;
	}
	else if(m_team_npc7 != NULL && m_team_npc7->pev->deadflag == DEAD_NO)
	{
		ally2 = 7;
	}
	else if(m_team_npc8 != NULL && m_team_npc8->pev->deadflag == DEAD_NO)
	{
		ally2 = 8;
	}
	else if(m_team_npc9 != NULL && m_team_npc9->pev->deadflag == DEAD_NO)
	{
		ally2 = 9;
	}
	else if(m_team_npc10 != NULL && m_team_npc10->pev->deadflag == DEAD_NO)
	{
		ally2 = 10;
	}
	else if(m_team_npc11 != NULL && m_team_npc11->pev->deadflag == DEAD_NO)
	{
		ally2 = 11;
	}
	else if(m_team_npc12 != NULL && m_team_npc12->pev->deadflag == DEAD_NO)
	{
		ally2 = 12;
	}

	if(ally1 > 0 && ally2 > 0)
	{
		TeamMate_Nagamatagi_Switch(ally1,ally2);
	}
}

void CBasePlayer :: TeamMate_Nagamatagi_Teleport( int mode )
{
	CBaseMonster *pEnemyMonster;
	if(mode == 1)
	{
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(0,96,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(0,192,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(0,-96,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-192,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
	}
	if(mode == 2)
	{
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
	}
	if(mode == 3)
	{
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(96,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(192,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-96,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(-192,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
	}
	if(mode == 4)
	{
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(64,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(128,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-64,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(-128,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
	}
	if(mode == 5)
	{
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(0,64,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(0,128,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(0,-64,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);//ս��
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-128,-36) );
			pEnemyMonster->Hunt_Stand_Set(2);
		}
	}
	if(mode == 6)
	{
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(-64,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(0);
			pEnemyMonster->pev->angles.y = 90;
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(-128,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(0);
			pEnemyMonster->pev->angles.y = 90;
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-192,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(0);
			pEnemyMonster->pev->angles.y = 90;
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(-256,0,-36) );
			pEnemyMonster->Hunt_Stand_Set(0);
			pEnemyMonster->pev->angles.y = 90;
		}
	}
	if(mode == 7)
	{
		if (m_team_npc1 != NULL)
		{
			if (m_team_npc1->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc1->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(0,96,-36) );
			}
		}
		if (m_team_npc2 != NULL)
		{
			if (m_team_npc2->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc2->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(0,192,-36) );
			}
		}
		if (m_team_npc3 != NULL)
		{
			if (m_team_npc3->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc3->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(0,-96,-36) );
			}
		}
		if (m_team_npc4 != NULL)
		{
			if (m_team_npc4->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc4->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-192,-36) );
			}
		}
	}
	if(mode == 8)
	{
		if (m_team_npc1 != NULL)
		{
			if (m_team_npc1->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc1->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(96,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(2);
			}
		}
		if (m_team_npc2 != NULL)
		{
			if (m_team_npc2->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc2->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(192,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(2);
			}
		}
		if (m_team_npc3 != NULL)
		{
			if (m_team_npc3->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc3->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-96,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(2);
			}
		}
		if (m_team_npc4 != NULL)
		{
			if (m_team_npc4->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc4->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(-192,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(2);
			}
		}
	}
	if(mode == 9)
	{
		if (m_team_npc1 != NULL)
		{
			if (m_team_npc1->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc1->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(96,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(0);
				pEnemyMonster->pev->angles.y = 270;
			}
		}
		if (m_team_npc2 != NULL)
		{
			if (m_team_npc2->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc2->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(192,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(0);
				pEnemyMonster->pev->angles.y = 270;
			}
		}
		if (m_team_npc3 != NULL)
		{
			if (m_team_npc3->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc3->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-96,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(0);
				pEnemyMonster->pev->angles.y = 270;
			}
		}
		if (m_team_npc4 != NULL)
		{
			if (m_team_npc4->pev->deadflag == DEAD_NO)
			{
				pEnemyMonster = m_team_npc4->MyMonsterPointer();
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(-192,0,-36) );
				pEnemyMonster->Hunt_Stand_Set(0);
				pEnemyMonster->pev->angles.y = 270;
			}
		}
	}
	if(mode == 10)
	{
		if (m_team_npc1 != NULL)
		{
			UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(80,0,-36) );
			m_team_npc1->pev->angles.y = 0;
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc2 != NULL)
		{
			UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(160,0,-36) );
			m_team_npc2->pev->angles.y = 90;
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc3 != NULL)
		{
			UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-80,0,-36) );
			m_team_npc3->pev->angles.y = 180;
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc4 != NULL)
		{
			UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(-160,0,-36) );
			m_team_npc4->pev->angles.y = 270;
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc5 != NULL)
		{
			UTIL_SetOrigin( m_team_npc5->pev, pev->origin + Vector(80,-80,-36) );
			m_team_npc5->pev->angles.y = 180;
			pEnemyMonster = m_team_npc5->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc6 != NULL)
		{
			UTIL_SetOrigin( m_team_npc6->pev, pev->origin + Vector(160,-80,-36) );
			m_team_npc6->pev->angles.y = 90;
			pEnemyMonster = m_team_npc6->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc7 != NULL)
		{
			UTIL_SetOrigin( m_team_npc7->pev, pev->origin + Vector(-80,-80,-36) );
			m_team_npc7->pev->angles.y = 0;
			pEnemyMonster = m_team_npc7->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc8 != NULL)
		{
			UTIL_SetOrigin( m_team_npc8->pev, pev->origin + Vector(-160,-80,-36) );
			m_team_npc8->pev->angles.y = 90;
			pEnemyMonster = m_team_npc8->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc9 != NULL)
		{
			UTIL_SetOrigin( m_team_npc9->pev, pev->origin + Vector(80,-160,-36) );
			m_team_npc9->pev->angles.y = 180;
			pEnemyMonster = m_team_npc9->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc10 != NULL)
		{
			UTIL_SetOrigin( m_team_npc10->pev, pev->origin + Vector(160,-160,-36) );
			m_team_npc10->pev->angles.y = 270;
			pEnemyMonster = m_team_npc10->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc11 != NULL)
		{
			UTIL_SetOrigin( m_team_npc11->pev, pev->origin + Vector(-80,-160,-36) );
			m_team_npc11->pev->angles.y = 45;
			pEnemyMonster = m_team_npc11->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
		if (m_team_npc12 != NULL)
		{
			UTIL_SetOrigin( m_team_npc12->pev, pev->origin + Vector(-160,-160,-36) );
			m_team_npc12->pev->angles.y = 135;
			pEnemyMonster = m_team_npc12->MyMonsterPointer();
			pEnemyMonster->SetActivity ( ACT_DIESIMPLE );
			pEnemyMonster->pev->velocity = g_vecZero;
			FX_Explosion( pEnemyMonster->Center(), 236 );
		}
	}
	if(mode == 11)
	{
		if (m_team_npc1 != NULL)
		{
			if (m_team_npc1->pev->deadflag == DEAD_NO)
			{
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(96,0,-36) );
			}
			else
			{
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(96,0,8192) );
			}
		}
		if (m_team_npc2 != NULL)
		{
			if (m_team_npc2->pev->deadflag == DEAD_NO)
			{
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(0,96,-36) );
			}
			else
			{
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(0,96,8192) );
			}
		}
		if (m_team_npc3 != NULL)
		{
			if (m_team_npc3->pev->deadflag == DEAD_NO)
			{
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-96,0,-36) );
			}
			else
			{
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(-96,0,8192) );
			}
		}
		if (m_team_npc4 != NULL){
			if (m_team_npc4->pev->deadflag == DEAD_NO)
			{
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-96,-36) );
			}
			else
			{
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-96,8192) );
			}
		}
	}

	if(mode == 1 || mode == 2 || mode == 3 || mode == 6)
	{
		if (m_team_npc5 != NULL)
		{
			pEnemyMonster = m_team_npc5->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc5->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc6 != NULL)
		{
			pEnemyMonster = m_team_npc6->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc6->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc7 != NULL)
		{
			pEnemyMonster = m_team_npc7->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc7->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc8 != NULL){
			pEnemyMonster = m_team_npc8->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc8->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc9 != NULL)
		{
			pEnemyMonster = m_team_npc9->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc9->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc10 != NULL)
		{
			pEnemyMonster = m_team_npc10->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc10->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc11 != NULL)
		{
			pEnemyMonster = m_team_npc11->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc11->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
		if (m_team_npc12 != NULL)
		{
			pEnemyMonster = m_team_npc12->MyMonsterPointer();
			UTIL_SetOrigin( m_team_npc12->pev, pev->origin + Vector(0,0,8192) );
			pEnemyMonster->Hunt_Stand_Set(0);
		}
	}
}

void CBasePlayer :: TeamMate_Nagamatagi_Allclear( int mode )
{
	if(mode == 10)
	{
		CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_saintna");
		CBaseMonster *pEnemyMonster = pEntity2->MyMonsterPointer();
		if ( pEntity2 )
		{
			TeamMate_add(pEnemyMonster);
			pEntity2->pev->frags = 2;
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_giant");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_blues");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_andylow");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_nobita");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_dragon");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_willam");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_wisebeast");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
			pEntity2->pev->frags = 2;
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_mario");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hime");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
		}
		pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
		if ( pEntity2 )
		{
			pEnemyMonster = pEntity2->MyMonsterPointer();
			TeamMate_add(pEnemyMonster);
			pEntity2->pev->frags = 2;
		}
		return;
	}
	else if(mode == 4)
	{
		if(m_team_npc1 != NULL)
		{
			if(m_team_npc1->pev->deadflag == DEAD_NO)
			{
				m_team_npc1->Killed( m_team_npc1->pev, GIB_NEVER );
			}
		}
		if(m_team_npc2 != NULL)
		{
			if(m_team_npc2->pev->deadflag == DEAD_NO)
			{
				m_team_npc2->Killed( m_team_npc2->pev, GIB_NEVER );
			}
		}
		if(m_team_npc3 != NULL)
		{
			if(m_team_npc3->pev->deadflag == DEAD_NO)
			{
				m_team_npc3->Killed( m_team_npc3->pev, GIB_NEVER );
			}
		}
		if(m_team_npc4 != NULL){

			if(m_team_npc4->pev->deadflag == DEAD_NO)
			{
				m_team_npc4->Killed( m_team_npc4->pev, GIB_NEVER );
			}
		}
		if(m_team_npc5 != NULL)
		{
			if(m_team_npc5->pev->deadflag == DEAD_NO)
			{
				m_team_npc5->Killed( m_team_npc5->pev, GIB_NEVER );
			}
		}
		if(m_team_npc6 != NULL)
		{
			if(m_team_npc6->pev->deadflag == DEAD_NO)
			{
				m_team_npc6->Killed( m_team_npc6->pev, GIB_NEVER );
			}
		}
		if(m_team_npc7 != NULL)
		{
			if(m_team_npc7->pev->deadflag == DEAD_NO)
			{
				m_team_npc7->Killed( m_team_npc7->pev, GIB_NEVER );
			}
		}
		if(m_team_npc8 != NULL)
		{
			if(m_team_npc8->pev->deadflag == DEAD_NO)
			{
				m_team_npc8->Killed( m_team_npc8->pev, GIB_NEVER );
			}
		}
		if(m_team_npc9 != NULL)
		{
			if(m_team_npc9->pev->deadflag == DEAD_NO)
			{
				m_team_npc9->Killed( m_team_npc9->pev, GIB_NEVER );
			}
		}
		if(m_team_npc10 != NULL)
		{
			if(m_team_npc10->pev->deadflag == DEAD_NO)
			{
				m_team_npc10->Killed( m_team_npc10->pev, GIB_NEVER );
			}
		}
		if(m_team_npc11 != NULL)
		{
			if(m_team_npc11->pev->deadflag == DEAD_NO)
			{
				m_team_npc11->Killed( m_team_npc11->pev, GIB_NEVER );
			}
		}
		if(m_team_npc12 != NULL)
		{
			if(m_team_npc12->pev->deadflag == DEAD_NO)
			{
				m_team_npc12->Killed( m_team_npc12->pev, GIB_NEVER );
			}
		}
		return;
	}
	else if(mode == 5)
	{
		if(m_team_npc1 != NULL)
		{
			m_team_npc1->SUB_StartFadeOut3();
		}
		if(m_team_npc2 != NULL)
		{
			m_team_npc2->SUB_StartFadeOut3();
		}
		if(m_team_npc3 != NULL)
		{
			m_team_npc3->SUB_StartFadeOut3();
		}
		if(m_team_npc4 != NULL)
		{
			m_team_npc4->SUB_StartFadeOut3();
		}
		if(m_team_npc5 != NULL)
		{
			m_team_npc5->SUB_StartFadeOut3();
		}
		if(m_team_npc6 != NULL)
		{
			m_team_npc6->SUB_StartFadeOut3();
		}
		if(m_team_npc7 != NULL)
		{
			m_team_npc7->SUB_StartFadeOut3();
		}
		if(m_team_npc8 != NULL)
		{
			m_team_npc8->SUB_StartFadeOut3();
		}
		if(m_team_npc9 != NULL)
		{
			m_team_npc9->SUB_StartFadeOut3();
		}
		if(m_team_npc10 != NULL)
		{
			m_team_npc10->SUB_StartFadeOut3();
		}
		if(m_team_npc11 != NULL)
		{
			m_team_npc11->SUB_StartFadeOut3();
		}
		if(m_team_npc12 != NULL)
		{
			m_team_npc12->SUB_StartFadeOut3();
		}
		return;
	}
	else if(mode == 99)
	{
		CBaseMonster *pEnemyMonster;
		if(m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc5 != NULL)
		{
			pEnemyMonster = m_team_npc5->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc6 != NULL)
		{
			pEnemyMonster = m_team_npc6->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc7 != NULL)
		{
			pEnemyMonster = m_team_npc7->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc8 != NULL)
		{
			pEnemyMonster = m_team_npc8->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc9 != NULL)
		{
			pEnemyMonster = m_team_npc9->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc10 != NULL)
		{
			pEnemyMonster = m_team_npc10->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc11 != NULL)
		{
			pEnemyMonster = m_team_npc11->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
		}
		if(m_team_npc12 != NULL)
		{
			pEnemyMonster = m_team_npc12->MyMonsterPointer();
			pEnemyMonster->m_rpgms_level = 99;
			}
		return;
	}

	if(m_guard_mynpc == 1 || !FNullEnt(m_wdoor_mynpc))
	{
		m_wdoor_mynpc = NULL;
		m_guard_mynpc = 0;
	}

	if(mode >= 1)
	{
		CBaseMonster *pEnemyMonster;
		if (m_team_npc1 != NULL)
		{
			pEnemyMonster = m_team_npc1->MyMonsterPointer();
			pEnemyMonster->m_rpgms_inteam = 0;
			pEnemyMonster->m_hPlayer = NULL;
			if(mode == 2)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Killed( pEnemyMonster->pev, GIB_NEVER );
				}
			}
			if(mode == 3)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Hunt_Stand_Set(0);
				}
			}
		}
		if (m_team_npc2 != NULL)
		{
			pEnemyMonster = m_team_npc2->MyMonsterPointer();
			pEnemyMonster->m_rpgms_inteam = 0;
			pEnemyMonster->m_hPlayer = NULL;
			if(mode == 2)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Killed( pEnemyMonster->pev, GIB_NEVER );
				}
			}
			if(mode == 3)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Hunt_Stand_Set(0);
				}
			}
		}
		if (m_team_npc3 != NULL)
		{
			pEnemyMonster = m_team_npc3->MyMonsterPointer();
			pEnemyMonster->m_rpgms_inteam = 0;
			pEnemyMonster->m_hPlayer = NULL;
			if(mode == 2)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Killed( pEnemyMonster->pev, GIB_NEVER );
				}
			}
			if(mode == 3)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Hunt_Stand_Set(0);
				}
			}
		}
		if (m_team_npc4 != NULL)
		{
			pEnemyMonster = m_team_npc4->MyMonsterPointer();
			pEnemyMonster->m_rpgms_inteam = 0;
			pEnemyMonster->m_hPlayer = NULL;
			if(mode == 2)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Killed( pEnemyMonster->pev, GIB_NEVER );
				}
			}
			if(mode == 3)
			{
				if(pEnemyMonster->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster->Hunt_Stand_Set(0);
				}
			}
		}
	}

	m_team_npc1 = NULL;
	m_rpg_menu_actor2 = 0;
	m_team_npc2 = NULL;
	m_rpg_menu_actor3 = 0;
	m_team_npc3 = NULL;
	m_rpg_menu_actor4 = 0;
	m_team_npc4 = NULL;
	m_rpg_menu_actor5 = 0;

	m_team_npc5 = NULL;
	m_team_npc6 = NULL;
	m_team_npc7 = NULL;
	m_team_npc8 = NULL;
	m_team_npc9 = NULL;
	m_team_npc10 = NULL;
	m_team_npc11 = NULL;
	m_team_npc12 = NULL;
}

void CBasePlayer :: TeamMate_Nagamatagi_RespawnStone( int mode )
{
	CBaseMonster *pEnemyMonster;
	if (m_team_npc1 != NULL)
	{
		pEnemyMonster = m_team_npc1->MyMonsterPointer();
		pEnemyMonster->Hunt_Stand_Set(mode);
	}
	if (m_team_npc2 != NULL)
	{
		pEnemyMonster = m_team_npc2->MyMonsterPointer();
		pEnemyMonster->Hunt_Stand_Set(mode);
	}
	if (m_team_npc3 != NULL)
	{
		pEnemyMonster = m_team_npc3->MyMonsterPointer();
		pEnemyMonster->Hunt_Stand_Set(mode);
	}
	if (m_team_npc4 != NULL)
	{
		pEnemyMonster = m_team_npc4->MyMonsterPointer();
		pEnemyMonster->Hunt_Stand_Set(mode);
	}

	if(mode != 2 && mode != 3)
	{
		if (m_team_npc5 != NULL)
		{
			pEnemyMonster = m_team_npc5->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc6 != NULL)
		{
			pEnemyMonster = m_team_npc6->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc7 != NULL)
		{
			pEnemyMonster = m_team_npc7->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc8 != NULL)
		{
			pEnemyMonster = m_team_npc8->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc9 != NULL)
		{
			pEnemyMonster = m_team_npc9->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc10 != NULL)
		{
			pEnemyMonster = m_team_npc10->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc11 != NULL)
		{
			pEnemyMonster = m_team_npc11->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
		if (m_team_npc12 != NULL)
		{
			pEnemyMonster = m_team_npc12->MyMonsterPointer();
			pEnemyMonster->Hunt_Stand_Set(mode);
		}
	}
}

void CBasePlayer :: TeamMate_Nagamatagi_Switch( int ally1,int ally2 )
{
	if(ally1 == ally2)
		return;
	
	EHANDLE	 combat_target;
	EHANDLE	 switch_target;
	CBaseMonster *pEnemyMonster_combat;
	CBaseMonster *pEnemyMonster_switch;
	Vector combat_origin;
	Vector switch_origin;
	combat_target = NULL;
    switch_target = NULL;
	int switch_redive = 0;

	nagama_switch_redive:
	
	if(ally1 == 1 && m_team_npc1 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc1 = switch_target;
		}
		else
		{
			combat_target = m_team_npc1;
			pEnemyMonster_combat = m_team_npc1->MyMonsterPointer();
			combat_origin = m_team_npc1->pev->origin;
		}
	}
	else if(ally1 == 2 && m_team_npc2 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc2 = switch_target;
		}
		else
		{
			combat_target = m_team_npc2;
			pEnemyMonster_combat = m_team_npc2->MyMonsterPointer();
			combat_origin = m_team_npc2->pev->origin;
		}
	}
	else if(ally1 == 3 && m_team_npc3 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc3 = switch_target;
		}
		else
		{
			combat_target = m_team_npc3;
			pEnemyMonster_combat = m_team_npc3->MyMonsterPointer();
			combat_origin = m_team_npc3->pev->origin;
		}
	}
	else if(ally1 == 4 && m_team_npc4 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc4 = switch_target;
		}
		else
		{
			combat_target = m_team_npc4;
			pEnemyMonster_combat = m_team_npc4->MyMonsterPointer();
			combat_origin = m_team_npc4->pev->origin;
		}
	}
	else if(ally1 == 5 && m_team_npc5 != NULL)
	{
		combat_target = m_team_npc5;
		pEnemyMonster_combat = m_team_npc5->MyMonsterPointer();
		combat_origin = m_team_npc5->pev->origin;
		if(switch_target != NULL)
		{
			m_team_npc5 = switch_target;
		}
	}
	else if(ally1 == 6 && m_team_npc6 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc6 = switch_target;
		}
		else
		{
			combat_target = m_team_npc6;
			pEnemyMonster_combat = m_team_npc6->MyMonsterPointer();
			combat_origin = m_team_npc6->pev->origin;
		}
	}
	else if(ally1 == 7 && m_team_npc7 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc7 = switch_target;
		}
		else
		{
			combat_target = m_team_npc7;
			pEnemyMonster_combat = m_team_npc7->MyMonsterPointer();
			combat_origin = m_team_npc7->pev->origin;
		}
	}
	else if(ally1 == 8 && m_team_npc8 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc8 = switch_target;
		}
		else
		{
			combat_target = m_team_npc8;
			pEnemyMonster_combat = m_team_npc8->MyMonsterPointer();
			combat_origin = m_team_npc8->pev->origin;
		}
	}
	else if(ally1 == 9 && m_team_npc9 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc9 = switch_target;
		}
		else
		{
			combat_target = m_team_npc9;
			pEnemyMonster_combat = m_team_npc9->MyMonsterPointer();
			combat_origin = m_team_npc9->pev->origin;
		}
	}
	else if(ally1 == 10 && m_team_npc10 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc10 = switch_target;
		}
		else
		{
			combat_target = m_team_npc10;
			pEnemyMonster_combat = m_team_npc10->MyMonsterPointer();
			combat_origin = m_team_npc10->pev->origin;
		}
	}
	else if(ally1 == 11 && m_team_npc11 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc11 = switch_target;
		}
		else
		{
			combat_target = m_team_npc11;
			pEnemyMonster_combat = m_team_npc11->MyMonsterPointer();
			combat_origin = m_team_npc11->pev->origin;
		}
	}
	else if(ally1 == 12 && m_team_npc12 != NULL)
	{
		if(switch_target != NULL)
		{
			m_team_npc12 = switch_target;
		}
		else
		{
			combat_target = m_team_npc12;
			pEnemyMonster_combat = m_team_npc12->MyMonsterPointer();
			combat_origin = m_team_npc12->pev->origin;
		}
	}

	if(combat_target == NULL)
		return;

	if(switch_redive == 0)
	{
		if(ally2 == 1 && m_team_npc1 != NULL)
		{
			if(combat_target->pev->deadflag != DEAD_NO && ally1 >= 5)
				return;
			
			switch_target = m_team_npc1;
			pEnemyMonster_switch = m_team_npc1->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_freezetime > 0)
				return;
			
			switch_origin = m_team_npc1->pev->origin;
			m_team_npc1 = combat_target;
			if(ally1 >= 5)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( combat_target->Center(), 102);
				pEnemyMonster_switch->m_rpgms_inteam = 5;
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(0);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO){

					pEnemyMonster_combat->Hunt_Stand_Set(2);
				}
			}
		}
		else if(ally2 == 2 && m_team_npc2 != NULL)
		{
			if(combat_target->pev->deadflag != DEAD_NO && ally1 >= 5)
				return;
			
			switch_target = m_team_npc2;
			pEnemyMonster_switch = m_team_npc2->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_freezetime > 0)
				return;
			
			switch_origin = m_team_npc2->pev->origin;
			m_team_npc2 = combat_target;
			if(ally1 >= 5)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( combat_target->Center(), 102);
				pEnemyMonster_switch->m_rpgms_inteam = 5;
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(0);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(2);
				}
			}
		}
		else if(ally2 == 3 && m_team_npc3 != NULL)
		{
			if(combat_target->pev->deadflag != DEAD_NO && ally1 >= 5)
				return;
			
			switch_target = m_team_npc3;
			pEnemyMonster_switch = m_team_npc3->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_freezetime > 0)
				return;
			
			switch_origin = m_team_npc3->pev->origin;
			m_team_npc3 = combat_target;
			if(ally1 >= 5)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( combat_target->Center(), 102);
				pEnemyMonster_switch->m_rpgms_inteam = 5;
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(0);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(2);
				}
			}
		}
		else if(ally2 == 4 && m_team_npc4 != NULL)
		{
			if(combat_target->pev->deadflag != DEAD_NO && ally1 >= 5)
				return;
			
			switch_target = m_team_npc4;
			pEnemyMonster_switch = m_team_npc4->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_freezetime > 0)
				return;
			
			switch_origin = m_team_npc4->pev->origin;
			m_team_npc4 = combat_target;
			if(ally1 >= 5)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( combat_target->Center(), 102);
				pEnemyMonster_switch->m_rpgms_inteam = 5;
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(0);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(2);
				}
			}
		}
		else if(ally2 == 5 && m_team_npc5 != NULL)
		{
			if(m_team_npc5->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			switch_target = m_team_npc5;
			pEnemyMonster_switch = m_team_npc5->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc5->pev->origin;
			m_team_npc5 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 6 && m_team_npc6 != NULL)
		{
			if(m_team_npc6->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;

			switch_target = m_team_npc6;
			pEnemyMonster_switch = m_team_npc6->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;

			switch_origin = m_team_npc6->pev->origin;
			m_team_npc6 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 7 && m_team_npc7 != NULL)
		{
			if(m_team_npc7->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			
			switch_target = m_team_npc7;
			pEnemyMonster_switch = m_team_npc7->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc7->pev->origin;
			m_team_npc7 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 8 && m_team_npc8 != NULL)
		{
			if(m_team_npc8->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			
			switch_target = m_team_npc8;
			pEnemyMonster_switch = m_team_npc8->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc8->pev->origin;
			m_team_npc8 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 9 && m_team_npc9 != NULL)
		{
			if(m_team_npc9->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			
			switch_target = m_team_npc9;
			pEnemyMonster_switch = m_team_npc9->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc9->pev->origin;
			m_team_npc9 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
					pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 10 && m_team_npc10 != NULL)
		{
			if(m_team_npc10->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			
			switch_target = m_team_npc10;
			pEnemyMonster_switch = m_team_npc10->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc10->pev->origin;
			m_team_npc10 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 11 && m_team_npc11 != NULL)
		{
			if(m_team_npc11->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			
			switch_target = m_team_npc11;
			pEnemyMonster_switch = m_team_npc11->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc11->pev->origin;
			m_team_npc11 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}
		else if(ally2 == 12 && m_team_npc12 != NULL)
		{
			if(m_team_npc12->pev->deadflag != DEAD_NO && ally1 <= 4)
				return;
			
			switch_target = m_team_npc12;
			pEnemyMonster_switch = m_team_npc12->MyMonsterPointer();
			if ( pEnemyMonster_switch->m_MonsterState == MONSTERSTATE_PRONE || pEnemyMonster_switch->m_IdealMonsterState == MONSTERSTATE_PRONE )
				return;
			
			switch_origin = m_team_npc12->pev->origin;
			m_team_npc12 = combat_target;
			if(ally1 <= 4)
			{
				UTIL_SetOrigin (combat_target->pev, switch_origin);
				UTIL_SetOrigin (switch_target->pev, combat_origin);
				FX_Explosion( switch_target->Center(), 102);
				if(pEnemyMonster_switch->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_switch->Hunt_Stand_Set(2);
				}
				if(pEnemyMonster_combat->pev->deadflag == DEAD_NO)
				{
					pEnemyMonster_combat->Hunt_Stand_Set(0);
				}
				pEnemyMonster_combat->m_rpgms_inteam = 5;
			}
		}

		if(switch_target == NULL)
			return;
		
		switch_redive = 1;
		goto nagama_switch_redive;
	}

	if (m_team_npc1 != NULL)
	{
		pEnemyMonster_combat = m_team_npc1->MyMonsterPointer();
		pEnemyMonster_combat->m_rpgms_inteam = 1;
		TeamMate_NPC_add(pEnemyMonster_combat);
		m_rpg_menu_actor2 = pEnemyMonster_combat->m_rpgms_actor;
	}
	if (m_team_npc2 != NULL)
	{
		pEnemyMonster_combat = m_team_npc2->MyMonsterPointer();
		pEnemyMonster_combat->m_rpgms_inteam = 2;
		TeamMate_NPC_add(pEnemyMonster_combat);
		m_rpg_menu_actor3 = pEnemyMonster_combat->m_rpgms_actor;
	}
	if (m_team_npc3 != NULL)
	{
		pEnemyMonster_combat = m_team_npc3->MyMonsterPointer();
		pEnemyMonster_combat->m_rpgms_inteam = 3;
		TeamMate_NPC_add(pEnemyMonster_combat);
		m_rpg_menu_actor4 = pEnemyMonster_combat->m_rpgms_actor;
	}
	if (m_team_npc4 != NULL)
	{
		pEnemyMonster_combat = m_team_npc4->MyMonsterPointer();
		pEnemyMonster_combat->m_rpgms_inteam = 4;
		TeamMate_NPC_add(pEnemyMonster_combat);
		m_rpg_menu_actor5 = pEnemyMonster_combat->m_rpgms_actor;
	}
}

void CBasePlayer :: TeamMate_GetNagamatagi( void )
{
	CBaseMonster *pEnemyMonster;
	m_rpg_menu_skill1 = 0;
	if (m_team_npc1 != NULL)
	{
		pEnemyMonster = m_team_npc1->MyMonsterPointer();
		m_rpg_menu_skill1 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc1->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill1 += 100;
		}
		m_rpg_menu_actor2 = pEnemyMonster->m_rpgms_actor;
	}
	m_rpg_menu_skill2 = 0;
	if (m_team_npc2 != NULL)
	{
		pEnemyMonster = m_team_npc2->MyMonsterPointer();
		m_rpg_menu_skill2 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc2->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill2 += 100;
		}
		m_rpg_menu_actor3 = pEnemyMonster->m_rpgms_actor;
	}
	m_rpg_menu_skill3 = 0;
	if (m_team_npc3 != NULL)
	{
		pEnemyMonster = m_team_npc3->MyMonsterPointer();
		m_rpg_menu_skill3 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc3->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill3 += 100;
		}
		m_rpg_menu_actor4 = pEnemyMonster->m_rpgms_actor;
	}
	m_rpg_menu_skill4 = 0;
	if (m_team_npc4 != NULL)
	{
		pEnemyMonster = m_team_npc4->MyMonsterPointer();
		m_rpg_menu_skill4 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc4->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill4 += 100;
		}
		m_rpg_menu_actor5 = pEnemyMonster->m_rpgms_actor;
	}
	m_rpg_menu_skill5 = 0;
	if (m_team_npc5 != NULL)
	{
		pEnemyMonster = m_team_npc5->MyMonsterPointer();
		m_rpg_menu_skill5 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc5->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill5 += 100;
		}
	}
	m_rpg_menu_skill6 = 0;
	if (m_team_npc6 != NULL)
	{
		pEnemyMonster = m_team_npc6->MyMonsterPointer();
		m_rpg_menu_skill6 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc6->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill6 += 100;
		}
	}
	m_rpg_menu_skill7 = 0;
	if (m_team_npc7 != NULL)
	{
		pEnemyMonster = m_team_npc7->MyMonsterPointer();
		m_rpg_menu_skill7 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc7->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill7 += 100;
		}
	}
	m_rpg_menu_skill8 = 0;
	if (m_team_npc8 != NULL)
	{
		pEnemyMonster = m_team_npc8->MyMonsterPointer();
		m_rpg_menu_skill8 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc8->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill8 += 100;
		}
	}
	m_rpg_menu_skill9 = 0;
	if (m_team_npc9 != NULL)
	{
		pEnemyMonster = m_team_npc9->MyMonsterPointer();
		m_rpg_menu_skill9 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc9->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill9 += 100;
		}
	}
	m_rpg_menu_skill10 = 0;
	if (m_team_npc10 != NULL)
	{
		pEnemyMonster = m_team_npc10->MyMonsterPointer();
		m_rpg_menu_skill10 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc10->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill10 += 100;
		}
	}
	m_rpg_menu_skill11 = 0;
	if (m_team_npc11 != NULL)
	{
		pEnemyMonster = m_team_npc11->MyMonsterPointer();
		m_rpg_menu_skill11 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc11->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill11 += 100;
		}
	}
	m_rpg_menu_skill12 = 0;
	if (m_team_npc12 != NULL)
	{
		pEnemyMonster = m_team_npc12->MyMonsterPointer();
		m_rpg_menu_skill12 = pEnemyMonster->m_rpgms_actor;
		if(m_team_npc12->pev->deadflag == DEAD_DEAD)
		{
			m_rpg_menu_skill12 += 100;
		}
	}
}

void CBasePlayer :: TeamMate_remove( CBaseMonster *pAllynpc )
{
	if (m_team_npc1 == pAllynpc)
	{
		m_team_npc1 = NULL;
		m_rpg_menu_actor2 = 0;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc2 == pAllynpc)
	{
		m_team_npc2 = NULL;
		m_rpg_menu_actor3 = 0;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc3 == pAllynpc)
	{
		m_team_npc3 = NULL;
		m_rpg_menu_actor4 = 0;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc4 == pAllynpc)
	{
		m_team_npc4 = NULL;
		m_rpg_menu_actor5 = 0;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc5 == pAllynpc)
	{
		m_team_npc5 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc6 == pAllynpc)
	{
		m_team_npc6 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc7 == pAllynpc)
	{
		m_team_npc7 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc8 == pAllynpc)
	{
		m_team_npc8 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc9 == pAllynpc)
	{
		m_team_npc9 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc10 == pAllynpc)
	{
		m_team_npc10 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc11 == pAllynpc)
	{
		m_team_npc11 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
	else if (m_team_npc12 == pAllynpc)
	{
		m_team_npc12 = NULL;
		pAllynpc->m_rpgms_inteam = 0;
	}
}


BOOL CBasePlayer::HasTeamMate_CanAdd( CBaseMonster *pAllynpc )
{
	if(m_team_npc1 != NULL && m_team_npc2 != NULL 
	&& m_team_npc3 != NULL && m_team_npc4 != NULL
	&& m_team_npc5 != NULL && m_team_npc6 != NULL
	&& m_team_npc7 != NULL && m_team_npc8 != NULL
	&& m_team_npc9 != NULL && m_team_npc10 != NULL
	&& m_team_npc11 != NULL && m_team_npc12 != NULL)
	{
		return FALSE;
	}

	if(m_team_npc1 == pAllynpc || m_team_npc2 == pAllynpc
	|| m_team_npc3 == pAllynpc || m_team_npc4 == pAllynpc
	|| m_team_npc5 == pAllynpc || m_team_npc6 == pAllynpc
	|| m_team_npc7 == pAllynpc || m_team_npc8 == pAllynpc
	|| m_team_npc9 == pAllynpc || m_team_npc10 == pAllynpc
	|| m_team_npc11 == pAllynpc || m_team_npc12 == pAllynpc)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CBasePlayer::HasMenuItem_Full( void )
{
	if(m_rpg_menu_item1 != 0 && m_rpg_menu_item2 != 0 
	&& m_rpg_menu_item3 != 0 && m_rpg_menu_item4 != 0
	&& m_rpg_menu_item5 != 0 && m_rpg_menu_item6 != 0
	&& m_rpg_menu_item7 != 0 && m_rpg_menu_item8 != 0
	&& m_rpg_menu_item9 != 0 && m_rpg_menu_item10 != 0
	&& m_rpg_menu_item11 != 0 && m_rpg_menu_item12 != 0)
	{
		return TRUE;
	}

	return FALSE;
}

void CBasePlayer :: BOSS_Find( void )
{
	CBaseEntity *pEntity = NULL;
	CBaseMonster *pEnemyMonster;
						
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8192 )) != NULL)
	{
		if ( (pEntity->pev->flags & FL_MONSTER) && pEntity->IsAlive() )
		{
			pEnemyMonster = pEntity->MyMonsterPointer();
			if(pEnemyMonster)
			{
				if(pEnemyMonster->m_is_the_boss)
				{
					m_boss_find = pEntity;
					m_boss_on = 1;
					break;
				}
			}
		}
	}

}

void CBasePlayer :: Clear_SayText( void )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_STRING( NULL );
		WRITE_BYTE( 1 );
	MESSAGE_END();

	m_fNextClearTextTime = -1;
}

Vector VecVelocityForDamage( float flDamage )
{
	Vector vec( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );

	if( flDamage > -50 )
		vec = vec * 0.7f;
	else if( flDamage > -200 )
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

#if 0 
static void ThrowGib( entvars_t *pev, char *szGibModel, float flDamage )
{
	edict_t *pentNew = CREATE_ENTITY();
	entvars_t *pevNew = VARS( pentNew );

	pevNew->origin = pev->origin;
	SET_MODEL( ENT( pevNew ), szGibModel );
	UTIL_SetSize( pevNew, g_vecZero, g_vecZero );

	pevNew->velocity = VecVelocityForDamage( flDamage );
	pevNew->movetype = MOVETYPE_BOUNCE;
	pevNew->solid = SOLID_NOT;
	pevNew->avelocity.x = RANDOM_FLOAT( 0, 600 );
	pevNew->avelocity.y = RANDOM_FLOAT( 0, 600 );
	pevNew->avelocity.z = RANDOM_FLOAT( 0, 600 );
	CHANGE_METHOD( ENT( pevNew ), em_think, SUB_Remove );
	pevNew->ltime = gpGlobals->time;
	pevNew->nextthink = gpGlobals->time + RANDOM_FLOAT( 10, 20 );
	pevNew->frame = 0;
	pevNew->flags = 0;
}

static void ThrowHead( entvars_t *pev, char *szGibModel, floatflDamage )
{
	SET_MODEL( ENT( pev ), szGibModel );
	pev->frame = 0;
	pev->nextthink = -1;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	pev->view_ofs = Vector( 0, 0, 8 );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 56 ) );
	pev->velocity = VecVelocityForDamage( flDamage );
	pev->avelocity = RANDOM_FLOAT( -1, 1 ) * Vector( 0, 600, 0 );
	pev->origin.z -= 24;
	ClearBits( pev->flags, FL_ONGROUND );
}
#endif

int TrainSpeed( int iSpeed, int iMax )
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if( iSpeed < 0 )
		iRet = TRAIN_BACK;
	else if( iSpeed == 0.0f )
		iRet = TRAIN_NEUTRAL;
	else if( fSpeed < 0.33f )
		iRet = TRAIN_SLOW;
	else if( fSpeed < 0.66f )
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound( void )
{
	// water death sounds
	/*
	if( pev->waterlevel == 3 )
	{
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE );
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	/*switch( RANDOM_LONG( 1, 5 ) )
	{
	case 1: 
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
		break;
	case 2: 
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
		break;
	case 3: 
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT( ENT( pev ), "HEV_DEAD" );*/
}

// override takehealth
// bitsDamageType indicates type of damage healed. 
int CBasePlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	return CBaseMonster::TakeHealth( flHealth, bitsDamageType );
}

Vector CBasePlayer::GetGunPosition()
{
	//UTIL_MakeVectors( pev->v_angle );
	//m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if((pev->flags & FL_FROZEN) || m_trainning == 1 || m_god_time >= gpGlobals->time || m_rpg_menu_actor1 != 1 || (pev->flags & FL_GODMODE) )
		return;

	if( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		if (pevAttacker)
		{
			CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
			if(pEntity){
				if (  (pEntity->pev->flags & FL_MONSTER) ){
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					if(pEnemyMonster->m_rpgms_inteam > 0)
						return;
				}
			}
		}

		if( bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB) )
		{

			switch( ptr->iHitgroup )
			{
			case HITGROUP_GENERIC:
				break;
			case HITGROUP_HEAD:
				{
					if(pev->armorvalue <= 0 && flDamage >= 5)
					{
						UTIL_BloodStream( ptr->vecEndPos, gpGlobals->v_forward * -5 + gpGlobals->v_up * 2, (unsigned short)73, 60 );	
					}
					else
					{
						MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
						WRITE_BYTE(TE_STREAK_SPLASH);
						WRITE_COORD(ptr->vecEndPos.x);
						WRITE_COORD(ptr->vecEndPos.y);
						WRITE_COORD(ptr->vecEndPos.z);
						WRITE_COORD(ptr->vecPlaneNormal.x);
						WRITE_COORD(ptr->vecPlaneNormal.y);
						WRITE_COORD(ptr->vecPlaneNormal.z);
						WRITE_BYTE(5);
						WRITE_SHORT(22);
						WRITE_SHORT(25);
						WRITE_SHORT(65);
						MESSAGE_END();
					}

					flDamage *= 1.5;
					}
				break;
			case HITGROUP_CHEST:
				flDamage *= 1;
				break;
			case HITGROUP_STOMACH:
				flDamage *= 1;
				break;
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
				flDamage *= 1;
				break;
			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
				flDamage *= 0.75;
				break;
			default:
				break;
			}
		}
		else
		{
			if( bitsDamageType & (DMG_SHOCK | DMG_SLASH | DMG_BURN | DMG_SONIC) )
			{
				if(ptr->iHitgroup == 1)
				{
					flDamage *= 1.25;
					if(pev->armorvalue <= 0)
					{
						UTIL_BloodStream( ptr->vecEndPos, gpGlobals->v_forward * -5 + gpGlobals->v_up * 2, (unsigned short)73, 60 );	
					}
					else
					{
						MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
						WRITE_BYTE(TE_STREAK_SPLASH);
						WRITE_COORD(ptr->vecEndPos.x);
						WRITE_COORD(ptr->vecEndPos.y);
						WRITE_COORD(ptr->vecEndPos.z);
						WRITE_COORD(ptr->vecPlaneNormal.x);
						WRITE_COORD(ptr->vecPlaneNormal.y);
						WRITE_COORD(ptr->vecPlaneNormal.z);
						WRITE_BYTE(5);
						WRITE_SHORT(22);
						WRITE_SHORT(25);
						WRITE_SHORT(65);
						MESSAGE_END();
					}
				}
			}
		}


		if(m_skill_maxarmor < 200 || pev->armorvalue <= 0)
		{
			if(flDamage >= 3 && bitsDamageType != DMG_BLOOD)
			{
				SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );// a little surface blood.
				TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
				
				if(flDamage >= 40)
				{
					FX_Explosion( ptr->vecEndPos, 236 );
				}
					else if(flDamage >= 20)
				{
				FX_Explosion( ptr->vecEndPos, 234 );
				}
				else if(flDamage >= 10)
				{
					FX_Explosion( ptr->vecEndPos, 232 );
				}
			}
		}
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO	0.2	// Armor Takes 80% of the damage
#define ARMOR_BONUS	0.5	// Each Point of Armor is work 1/x points of health

int CBasePlayer::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if((pev->flags & FL_FROZEN) || m_trainning == 1 || m_god_time >= gpGlobals->time || m_rpg_menu_actor1 != 1 || (pev->flags & FL_GODMODE) )
		return 0;
	
	if (pevAttacker)
	{
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
		if(pEntity)
		{
			if (  (pEntity->pev->flags & FL_MONSTER) )
			{
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				if(pEnemyMonster->m_rpgms_inteam > 0)
					return 0;
			}
		}
	}

	if(m_trainning == 1 || m_god_time >= gpGlobals->time)
		return 0;

	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flArmor_origin = pev->armorvalue;
	float flHealthPrev = pev->health;

	if(m_skill_maxarmor == 300)
	{
		flRatio = 0.0;
		flBonus = 0.3;
	}
	else if(m_skill_maxarmor == 200)
	{
		flRatio = 0.0;
		flBonus = 0.4;
	}
	else if(m_skill_maxarmor == 150)
	{
		flRatio = 0.2;
		flBonus = 0.5;
	}
	else if(m_skill_maxarmor == 120)
	{
		flRatio = 0.4;
		flBonus = 0.8;
	}
	else if(m_skill_maxarmor == 100)
	{
		flRatio = 0.5;
		flBonus = 1.0;
	}

	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	if(m_guard_mynpc == 1)
	{
		if(!FNullEnt(m_wdoor_mynpc))
		{
			if ( FClassnameIs( m_wdoor_mynpc, "monster_willam" ) )
			{
				flDamage *= 0.2;
			}
		}
	}
	if(FBitSet ( pev->flags, FL_ONGROUND ) && FBitSet(pev->flags,FL_DUCKING))
	{
		if(pev->velocity.Length() >= 750)
		{
			flDamage *= 0.3;
		}
	}
	if(m_hPortecter != NULL && flDamage > 0)
	{
		if(m_hPortecter->pev->deadflag == DEAD_NO && m_hPortecter->pev->weapons > 0)
		{
			pev->armorvalue = flArmor_origin;
			m_hPortecter->TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType);
			return 0;
		}
		else
		{
			m_hPortecter = NULL;
		}
	}

	if(m_wrongdoor_time >= 1)
	{
		flDamage *= 0.5;
	}

	if(g_causality_add > 0)
	{
		flDamage *= 1.0 - (0.1 * g_causality_add);
	}

	if( (bitsDamageType & DMG_BLAST)  )
	{
		flDamage *= 1.25;
	}

	if ( m_fMask )
	{
		if( bitsDamageType & DMG_NERVEGAS )
		return 0;
	}

	if( (bitsDamageType & DMG_DARK) )
		return 0;

	if(bitsDamage & DMG_UNKNOWBLAST)
	{
		if(m_wrongdoor_time == 0)
		{
			if(m_concussion_time == 0)
			{
				pev->viewmodel = 0;
				pev->punchangle.x += RANDOM_FLOAT(-60, 60);
				pev->punchangle.y += RANDOM_FLOAT(-60, 60);
				pev->punchangle.z += RANDOM_FLOAT(-60, 60);
				if (m_pActiveItem)
				{
					m_pActiveItem->Holster();
				}
			}
			m_concussion_time = gpGlobals->time + 4.0;
		}
		else
		{
			if(m_concussion_time == 0)
			{
				pev->viewmodel = 0;
				pev->punchangle.x += RANDOM_FLOAT(-30, 30);
				pev->punchangle.y += RANDOM_FLOAT(-30, 30);
				pev->punchangle.z += RANDOM_FLOAT(-30, 30);
				if (m_pActiveItem)
				{
					m_pActiveItem->Holster();
				}
			}
			m_concussion_time = gpGlobals->time + 2.0;
		}
	}
	else if (bitsDamage & DMG_CONCUSSION)
	{
		if(m_wrongdoor_time == 0)
		{
			if(m_concussion_time == 0)
			{
				pev->viewmodel = 0;
				pev->punchangle.x += RANDOM_FLOAT(-30, 30);
				pev->punchangle.y += RANDOM_FLOAT(-30, 30);
				pev->punchangle.z += RANDOM_FLOAT(-30, 30);
				if (m_pActiveItem)
				{
					m_pActiveItem->Holster();
				}
			}
			m_concussion_time = gpGlobals->time + 2.0;
		}
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = (int)flDamage;

	// Armor. 
	if( !( pev->flags & FL_GODMODE ) && pev->armorvalue && !( bitsDamageType & ( DMG_AIR | DMG_FALL | DMG_DROWN | DMG_NERVEGAS ) ) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor;

		flArmor = ( flDamage - flNew ) * flBonus;

		// Does this use more armor than we have?
		if( flArmor > pev->armorvalue )
		{
			flArmor = pev->armorvalue;
			flArmor *= ( 1 / flBonus );
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
			pev->armorvalue -= flArmor;

		flDamage = flNew;
	}

	if(flDamage >= pev->health && m_skill_goddam && pev->deadflag == DEAD_NO)
	{
		if(pev->health > pev->max_health * 0.2)
		{
			pev->health = 1;
			m_god_time = gpGlobals->time + 2;
			UTIL_ScreenFade( this, Vector(255,0,0), 1.5, 0.5, 100, FFADE_IN );
			return 0;
		}
	}

	m_deadtakedmgkill = bitsDamageType;

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage >= 0.0f ? floor(flDamage) : ceil(flDamage), bitsDamageType );

	if ( !IsAlive() )
	{
		return fTookDamage;
	}

	// reset damage time countdown for each type of time based damage player just sustained
	{
		for( int i = 0; i < CDMG_TIMEBASED; i++ )
			if( bitsDamageType & ( DMG_PARALYZE << i ) )
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );	// command length in bytes
		WRITE_BYTE( DRC_CMD_EVENT );	// take damage event
		WRITE_SHORT( ENTINDEX( this->edict() ) );	// index number of primary entity
		WRITE_SHORT( ENTINDEX( ENT( pevInflictor ) ) );	// index number of secondary entity
		WRITE_LONG( 5 );   // eventflags (priority and flags)
	MESSAGE_END();

	// how bad is it, doc?
	ftrivial = ( pev->health > 75 || m_lastDamageAmount < 5 );
	fmajor = ( m_lastDamageAmount > 25 );
	fcritical = ( pev->health < 30 );

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while( fTookDamage && ( !ftrivial || ( bitsDamage & DMG_TIMEBASED ) ) && ffound && bitsDamage )
	{
		ffound = FALSE;

		if( bitsDamage & DMG_CLUB )
		{
			/*if( fmajor )
				SetSuitUpdate( "!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC );	// minor fracture*/
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if( bitsDamage & ( DMG_FALL | DMG_CRUSH ) )
		{
			/*if( fmajor )
				SetSuitUpdate( "!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC );	// major fracture
			else
				SetSuitUpdate( "!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC );	// minor fracture*/

			bitsDamage &= ~( DMG_FALL | DMG_CRUSH );
			ffound = TRUE;
		}

		if( bitsDamage & DMG_BULLET )
		{
			/*if( m_lastDamageAmount > 5 )
				SetSuitUpdate( "!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC );	// blood loss detected
			//else
			//	SetSuitUpdate( "!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC );	// minor laceration*/

			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_SLASH )
		{
			/*if( fmajor )
				SetSuitUpdate( "!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC );	// major laceration
			else
				SetSuitUpdate( "!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC );	// minor laceration*/

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_SONIC )
		{
			/*if( fmajor )
				SetSuitUpdate( "!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN );	// internal bleeding*/
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if( bitsDamage & ( DMG_POISON | DMG_PARALYZE ) )
		{
			//SetSuitUpdate( "!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN );	// blood toxins detected
			bitsDamage &= ~( DMG_POISON | DMG_PARALYZE );
			ffound = TRUE;
		}

		if( bitsDamage & DMG_ACID )
		{
			//SetSuitUpdate( "!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN );	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_NERVEGAS )
		{
			//SetSuitUpdate( "!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN );	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if( bitsDamage & DMG_RADIATION )
		{
			//SetSuitUpdate( "!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN );	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if( bitsDamage & DMG_SHOCK )
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_CONCUSSION)
		{
			bitsDamage &= ~DMG_CONCUSSION;
			ffound = TRUE;
		}
	}

	/*pev->punchangle.x = -2;

	if( fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75 )
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate( "!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN );	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate( "!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN );	// morphine shot
	}

	if( fTookDamage && !ftrivial && fcritical && flHealthPrev < 75 )
	{
		// already took major damage, now it's critical...
		if( pev->health < 6 )
			SetSuitUpdate( "!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN );	// near death
		else if( pev->health < 20 )
			SetSuitUpdate( "!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN );	// health critical

		// give critical health warnings
		if( !RANDOM_LONG( 0, 3 ) && flHealthPrev < 50 )
			SetSuitUpdate( "!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN ); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if( fTookDamage && ( bitsDamageType & DMG_TIMEBASED ) && flHealthPrev < 75 )
	{
		if( flHealthPrev < 50 )
		{
			if( !RANDOM_LONG( 0, 3 ) )
				SetSuitUpdate( "!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN ); //seek medical attention
		}
		else
			SetSuitUpdate( "!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN );	// health dropping
	}*/

	if(flDamage >= 48 && pev->punchangle.x > -4)
	{
		pev->punchangle.x = -4;
	}
	else if(flDamage >= 24 && pev->punchangle.x > -3)
	{
		pev->punchangle.x = -3;
	}
	else if(flDamage >= 12 && pev->punchangle.x > -2)
	{
		pev->punchangle.x = -2;
	}
	else if(flDamage > 0 && pev->punchangle.x > -1)
	{
		pev->punchangle.x = -1;
	}

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	/*int iWeaponRules;
	int iAmmoRules;
	int i, j;
	CBasePlayerWeapon *rgpPackWeapons[MAX_WEAPONS] = {0,};
	int iPackAmmo[MAX_AMMO_SLOTS];
	int iPW = 0;// index into packweapons array
	int iPA = 0;// index into packammo array

	memset( iPackAmmo, -1, sizeof(iPackAmmo) );

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( TRUE );
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for( i = 0; i < MAX_ITEM_TYPES && iPW < MAX_WEAPONS; i++ )
	{
		if( m_rgpPlayerItems[i] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

			while( pPlayerItem && iPW < MAX_WEAPONS )
			{
				switch( iWeaponRules )
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if( m_pActiveItem && pPlayerItem == m_pActiveItem )
					{
						// this is the active item. Pack it.
						rgpPackWeapons[iPW] = (CBasePlayerWeapon *)pPlayerItem;
					}
					break;
				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[iPW] = (CBasePlayerWeapon *)pPlayerItem;
					break;
				default:
					break;
				}

				if( rgpPackWeapons[iPW] )
				{
					// complete the reload.
					j = Q_min( rgpPackWeapons[iPW]->iMaxClip() - rgpPackWeapons[iPW]->m_iClip, m_rgAmmo[rgpPackWeapons[iPW]->m_iPrimaryAmmoType] );

					// Add them to the clip
					rgpPackWeapons[iPW]->m_iClip += j;
					m_rgAmmo[rgpPackWeapons[iPW]->m_iPrimaryAmmoType] -= j;
					iPW++;
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	// now go through ammo and make a list of which types to pack.
	if( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for( i = 0; i < MAX_AMMO_SLOTS; i++ )
		{
			if( m_rgAmmo[i] > 0 )
			{
				// player has some ammo of this type.
				switch( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;
				case GR_PLR_DROP_AMMO_ACTIVE:
					if( m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex() ) 
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if( m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex() ) 
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;
				default:
					break;
				}
			}
		}
	}
	// create a box to pack the stuff into.
	CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );

	pWeaponBox->pev->angles.x = 0;// don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink( &CWeaponBox::Kill );
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	if( g_pGameRules->IsBustingGame())
	{
		while( rgpPackWeapons[iPW] )
		{
			// weapon unhooked from the player. Pack it into der box.
			if( FClassnameIs( rgpPackWeapons[iPW]->pev, "weapon_egon" ))
			{
				pWeaponBox->PackWeapon( rgpPackWeapons[iPW] );
				SET_MODEL( pWeaponBox->edict(), "models/w_egon.mdl" );
				pWeaponBox->pev->velocity = g_vecZero;
				pWeaponBox->pev->renderfx = kRenderFxGlowShell;
				pWeaponBox->pev->renderamt = 25;
				pWeaponBox->pev->rendercolor = Vector( 0, 75, 250 );
				break;
			}
			iPW++;
		}
	}
	else
	{
		bool bPackItems = true;
		if ( iAmmoRules == GR_PLR_DROP_AMMO_ACTIVE && iWeaponRules == GR_PLR_DROP_GUN_ACTIVE )
		{
			if ( rgpPackWeapons[0] == NULL
				|| ( FClassnameIs( rgpPackWeapons[0]->pev, "weapon_satchel" ) && ( iPackAmmo[0] == -1 || ( m_rgAmmo[iPackAmmo[0]] == 0 ) ) ) )
			{
				bPackItems = false;
			}
		}

		if ( bPackItems )
		{
			// pack the ammo
			while( iPackAmmo[iPA] != -1 )
			{
				pWeaponBox->PackAmmo( MAKE_STRING( CBasePlayerItem::AmmoInfoArray[iPackAmmo[iPA]].pszName ), m_rgAmmo[iPackAmmo[iPA]] );
				iPA++;
			}

			// now pack all of the items in the lists
			while( rgpPackWeapons[iPW] )
			{
				// weapon unhooked from the player. Pack it into der box.
				pWeaponBox->PackWeapon( rgpPackWeapons[iPW] );

				iPW++;
			}
		}

		pWeaponBox->pev->velocity = pev->velocity * 1.2f;// weaponbox has player's velocity, then some.
	}*/
	RemoveAllItems( TRUE );// now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems( BOOL removeSuit )
{
	m_rpg_menu_item1 = 0;
	m_rpg_menu_item2 = 0;
	m_rpg_menu_item3 = 0;
	m_rpg_menu_item4 = 0;
	m_rpg_menu_item5 = 0;
	m_rpg_menu_item6 = 0;
	m_rpg_menu_item7 = 0;
	m_rpg_menu_item8 = 0;
	m_rpg_menu_item9 = 0;
	m_rpg_menu_item10 = 0;
	m_rpg_menu_item11 = 0;
	m_rpg_menu_item12 = 0;

	m_rpg_menu_item_e = -1;
	m_rpg_menu_item_t = -1;
	m_iClient_mynpc = -1;

	int i;
	CBasePlayerItem *pPendingItem;

	if( m_pActiveItem )
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	m_iTrain = TRAIN_NEW; // turn off train

	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		m_rgpPlayerItems[i] = NULL;
		while( m_pActiveItem )
		{
			pPendingItem = m_pActiveItem->m_pNext; 
			m_pActiveItem->Drop();
			m_pActiveItem = pPendingItem;
		}
	}
	m_pActiveItem = NULL;

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	if( removeSuit )
		pev->weapons = 0;
	else
		pev->weapons &= ~WEAPON_ALLWEAPONS;

	// Turn off flashlight
	if (removeSuit)
		ClearBits( pev->effects, EF_DIMLIGHT );

	for( i = 0; i < MAX_AMMO_SLOTS; i++ )
		m_rgAmmo[i] = 0;

	if( satchelfix.value )
		DeactivateSatchels( this );

	UpdateClientData();

	// send Selected Weapon Message to our client
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
				// Better solution:  Add as parameter to all Killed() functions.

void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	if(pev->deadflag != DEAD_NO || m_player_died == TRUE)
	{
		if ( pev->solid != SOLID_NOT && ( pev->health < -pev->max_health * 1.5 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
		{
			m_fDeadRespawn = 0;
			m_guard_mynpc = 0;
			m_skill_respawn = 0;

			MESSAGE_BEGIN( MSG_ONE, gmsgTbutton, NULL, pev );
			WRITE_SHORT( 0 );
			MESSAGE_END();

			UTIL_ScreenFade( this, Vector(0,0,0), 4, 12, 255, FFADE_OUT | FFADE_MODULATE );
			m_flDeadTime = gpGlobals->time + 4.0;

			pev->solid			= SOLID_NOT;
			pev->origin.z		-= 114514;
			EMIT_SOUND(ENT(pev), CHAN_BODY, "common/bodysplat.wav", 1, ATTN_NORM);		
			return;
		}

		return;
	}

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "dead_bugfix", "1" );
	m_player_died = TRUE;

	CSound *pSound;

	// Holster weapon immediately, to allow it to cleanup
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	//g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );
	g_pGameRules->DeathNotice( this, pevAttacker, g_pevLastInflictor, m_deadtakedmgkill );

	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if( pSound )
		{
			pSound->Reset();
		}
	}

	pev->punchangle.z += -60.0;

	SetAnimation( PLAYER_DIE );

	m_flRespawnTimer = 0;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

	pev->viewmodel = 0;

	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_BOUNCE;
	ClearBits( pev->flags, FL_ONGROUND );
	if( pev->velocity.z < 10 )
		pev->velocity.z += RANDOM_FLOAT( 0, 300 );

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate( NULL, FALSE, 0 );

	// send "health" update message to zero
	m_iClientHealth = 0;
	m_iClient_mynpc = 0;

	// reset FOV
	pev->fov = 0;
	m_iClientFOV = 0;
	m_iFOV = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
	WRITE_BYTE(0);
	MESSAGE_END();

	game_player_dead = 1;

	pev->button = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgGunScope, NULL, pev );
	WRITE_BYTE( 0 );
	MESSAGE_END();

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	m_barnacle_RTP = 0;
	if ( m_barnacle_catchme != NULL )
	{
		if(FClassnameIs( m_barnacle_catchme->pev, "monster_headcrab") 
		|| FClassnameIs( m_barnacle_catchme->pev, "monster_headcrab_throw") )
		{
			m_barnacle_catchme->pev->owner = NULL;
			m_barnacle_catchme->pev->movetype = MOVETYPE_STEP;
		}
	}

	m_rpg_menu_on = 0;

	CLIENT_COMMAND(edict(), "=cammousemove\n");

	if(pev->armorvalue < 1)
	{
		if(m_skill_maxarmor < 300)
		{
			m_skill_maxarmor = 0;
		}
		m_iClientBattery = -1;
	}

	if(m_skill_reload && g_causality_add < 5)
	{
		g_causality_add++;
	}

	if ( pev->solid != SOLID_NOT && ( pev->health < -pev->max_health * 1.5 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
	{
		m_fDeadRespawn = 0;
		m_guard_mynpc = 0;
		m_skill_respawn = 0;
		m_rpg_menu_item_e = -1;
		m_rpg_menu_item_t = -1;

		MESSAGE_BEGIN( MSG_ONE, gmsgModeShow, NULL, pev );
		WRITE_BYTE( m_guard_mynpc );
		WRITE_BYTE( m_rpg_menu_item_t );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ONE, gmsgTbutton, NULL, pev );
		WRITE_SHORT( 0 );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_STRING( NULL );
		WRITE_BYTE( 1 );
		MESSAGE_END();

		UTIL_ScreenFade( this, Vector(0,0,0), 5, 15, 255, FFADE_OUT | FFADE_MODULATE );

		if(m_trainning == 3)
		{
			char text[256];
			sprintf( text, "- Total: %d  Hits: %d  Miss: %d\n", (m_enemy_kills+m_game_rate),m_enemy_kills,m_game_rate);
			UTIL_SayTextAll( text,this );
			m_flDeadTime = gpGlobals->time + 12.0;
		}
		else
		{
			m_flDeadTime = gpGlobals->time + 4.0;
		}

		pev->solid = SOLID_NOT;
		pev->origin.z -= 114514;
		EMIT_SOUND(ENT(pev), CHAN_BODY, "common/bodysplat.wav", 1, ATTN_NORM);		
		return;
	}

	if(m_skill_respawn == 0 || m_skill_respawn_time > gpGlobals->time)
	{
		// Tell Ammo Hud that the player is dead
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
			WRITE_BYTE(0);
			WRITE_BYTE(0XFF);
			WRITE_BYTE(0xFF);
		MESSAGE_END();

		m_guard_mynpc = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgModeShow, NULL, pev );
		WRITE_BYTE( m_guard_mynpc );
		WRITE_BYTE( m_rpg_menu_item_t );
		MESSAGE_END();

		if(m_trainning == 3)
		{
			char text[256];
			sprintf( text, "- Total: %d  Hits: %d  Miss: %d\n", (m_enemy_kills+m_game_rate),m_enemy_kills,m_game_rate);
			UTIL_SayTextAll( text,this );
			UTIL_ScreenFade( this, Vector(0,0,0), 8, 12, 255, FFADE_OUT | FFADE_MODULATE );
			m_flDeadTime = gpGlobals->time + 12.0;
		}
		else
		{
			UTIL_ScreenFade( this, Vector(0,0,0), 4, 12, 255, FFADE_OUT | FFADE_MODULATE );
			m_flDeadTime = gpGlobals->time + 4.0;
		}

		DeathSound();
	}
	else
	{
		m_flDeadTime = gpGlobals->time + 2.0;
	}

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink( &CBasePlayer::PlayerDeathThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if( pev->flags & FL_FROZEN )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch( playerAnim )
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;
	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;
	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		//m_IdealActivity = GetDeathActivity();
		break;
	case PLAYER_ATTACK1:
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if( !FBitSet( pev->flags, FL_ONGROUND ) && ( m_Activity == ACT_HOP || m_Activity == ACT_LEAP ) )	// Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if( pev->waterlevel > 1 )
		{
			if( speed == 0 )
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch( m_IdealActivity )
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if( m_Activity == m_IdealActivity )
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity( m_Activity );

		// Already using the desired animation?
		if( pev->sequence == animDesired )
			return;

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;
	case ACT_RANGE_ATTACK1:
		if( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
		strcat( szAnim, m_szAnimExtention );
		animDesired = LookupSequence( szAnim );
		if( animDesired == -1 )
			animDesired = 0;

		if( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if( !m_fSequenceLoops )
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;
	case ACT_WALK:
		if( m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished )
		{
			if( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
				strcpy( szAnim, "crouch_aim_" );
			else
				strcpy( szAnim, "ref_aim_" );
			strcat( szAnim, m_szAnimExtention );
			animDesired = LookupSequence( szAnim );
			if( animDesired == -1 )
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if( FBitSet( pev->flags, FL_DUCKING ) )
	{
		if( speed == 0 )
		{
			pev->gaitsequence = LookupActivity( ACT_CROUCHIDLE );
			// pev->gaitsequence = LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence = LookupActivity( ACT_CROUCH );
		}
	}
	else if( speed > 220 )
	{
		pev->gaitsequence = LookupActivity( ACT_RUN );
	}
	else if( speed > 0 )
	{
		pev->gaitsequence = LookupActivity( ACT_WALK );
	}
	else
	{
		// pev->gaitsequence = LookupActivity( ACT_WALK );
		pev->gaitsequence = LookupSequence( "deep_idle" );
	}

	// Already using the desired animation?
	if( pev->sequence == animDesired )
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_9mm = AmmoInventory( GetAmmoIndex( "9mm" ) );
	ammo_357 = AmmoInventory( GetAmmoIndex( "357" ) );
	ammo_argrens = AmmoInventory( GetAmmoIndex( "ARgrenades" ) );
	ammo_bolts = AmmoInventory( GetAmmoIndex( "bolts" ) );
	ammo_buckshot = AmmoInventory( GetAmmoIndex( "buckshot" ) );
	ammo_rockets = AmmoInventory( GetAmmoIndex( "rockets" ) );
	ammo_uranium = AmmoInventory( GetAmmoIndex( "uranium" ) );
	ammo_hornets = AmmoInventory( GetAmmoIndex( "Hornets" ) );
	ammo_762nato = AmmoInventory( GetAmmoIndex( "762nato" ) );
	ammo_45acp = AmmoInventory( GetAmmoIndex( "45acp" ) );
	ammo_556nato = AmmoInventory( GetAmmoIndex( "556nato" ) );
	ammo_338mag = AmmoInventory( GetAmmoIndex( "338mag" ) );
	ammo_762natobox = AmmoInventory( GetAmmoIndex( "762natobox" ) );
}

/*
===========
WaterMove
============
*/
#define AIRTIME	12		// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if( pev->movetype == MOVETYPE_NOCLIP )
		return;

	if( pev->health < 0 )
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if( pev->waterlevel != 3 ) 
	{
		// not underwater

		// play 'up for air' sound
		if( pev->air_finished < gpGlobals->time )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM );
		else if( pev->air_finished < gpGlobals->time + 9 )
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM );

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 0;

		// if we took drowning damage, give it back slowly
		if( m_idrowndmg > m_idrownrestored )
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if( pev->air_finished < gpGlobals->time )		// drown!
		{
			if( pev->pain_finished < gpGlobals->time )
			{
				// take drowning damage
				pev->dmg += 1;
				if( pev->dmg > 5 )
					pev->dmg = 5;
				//TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), pev->dmg, DMG_DROWN );
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += (int)pev->dmg;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if( !pev->waterlevel )
	{
		if( FBitSet( pev->flags, FL_INWATER ) )
		{
			ClearBits( pev->flags, FL_INWATER );
		}
		return;
	}

	// make bubbles
	if( pev->waterlevel == 3 )
	{
		air = (int)( pev->air_finished - gpGlobals->time );
		if( !RANDOM_LONG( 0, 0x1f ) && RANDOM_LONG( 0, AIRTIME - 1 ) >= air )
		{
			switch( RANDOM_LONG( 0, 3 ) )
			{
				case 0:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM );
					break;
				case 2:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM );
					break;
				case 3:
					EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM );
					break;
			}
		}
	}

	if( pev->watertype == CONTENT_LAVA )		// do damage
	{
		TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 20, DMG_BURN );
	}
	else if( pev->watertype == CONTENT_SLIME )		// do damage
	{
		TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), 10, DMG_ACID );
	}
	else if (m_flash_mode == 2)		// do damage
	{
		m_deadtakedmgkill = 0;
		m_flash_mode = 3;
		pev->health = 0;
		Killed( pev, GIB_ALWAYS );
	}

	if( !FBitSet( pev->flags, FL_INWATER ) )
	{
		SetBits( pev->flags, FL_INWATER );
		pev->dmgtime = 0;
	}
}

// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{ 
	return ( pev->movetype == MOVETYPE_FLY );
}

void CBasePlayer::PlayerDeathThink( void )
{
	float flForward;

	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		flForward = pev->velocity.Length() - 20;
		if( flForward <= 0 )
			pev->velocity = g_vecZero;
		else    
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	/*if( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}*/

	if(pev->viewmodel != 0)
		pev->viewmodel = 0;

	g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 1);

	if( pev->modelindex && ( !m_fSequenceFinished ) && ( pev->deadflag == DEAD_DYING ))
	{
		StudioFrameAdvance();

		m_flRespawnTimer = gpGlobals->frametime + m_flRespawnTimer;	// Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if( m_flRespawnTimer < 4.0f )   // Animations should be no longer than this
			return;
	}

	if( pev->deadflag == DEAD_DYING )
	{
		if( g_pGameRules->IsMultiplayer() && m_fSequenceFinished && pev->movetype == MOVETYPE_NONE )
		{
			CopyToBodyQue( pev );
			pev->modelindex = 0;
		}
		pev->deadflag = DEAD_DEAD;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if( pev->movetype != MOVETYPE_NONE && FBitSet( pev->flags, FL_ONGROUND ) )
		pev->movetype = MOVETYPE_NONE;

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	BOOL fAnyButtonDown = ( pev->button & ~IN_SCORE );

	pev->button = 0;

	//ALERT(at_console, "Respawn\n");

	if(m_flDeadTime <= gpGlobals->time && m_flDeadTime >= 0)
	{
		if(m_skill_respawn && m_fDeadRespawn == 0)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTbutton, NULL, pev );
			WRITE_SHORT( 4 );
			MESSAGE_END();
			m_flDeadTime = gpGlobals->time + 4.0;
			m_fDeadRespawn = 1;
		}
		else if(m_fDeadRespawn == 1)
		{
			if ( !(pev->flags & FL_FROZEN) )
			{
				CLIENT_COMMAND(edict(), "-cammousemove\n");
			}
			m_needleheal = 1;
			m_needleuse_time = gpGlobals->time + 5.0;
			m_air_oxyan = 1;
			m_barnacle_god_time = gpGlobals->time + 6.0;
			m_barnacle_RTP_relase = 0;

			if (m_pActiveItem)
			{
				m_pActiveItem->Deploy();
			}
			pev->deadflag		= DEAD_NO;
			pev->takedamage		= DAMAGE_AIM;
			pev->solid			= SOLID_SLIDEBOX;
			pev->movetype		= MOVETYPE_WALK;
			//pev->health			= 1;
			//m_needleheal2	   += pev->max_health;
			pev->health			= pev->max_health;
			pev->view_ofs = VEC_VIEW;
			pev->velocity = g_vecZero;
			m_flFallVelocity = 0;
			m_god_time = gpGlobals->time + 6.0;
			m_godposion = 1;
			FX_Explosion( pev->origin, EXPLOSION_BIOMASS);

			m_fDeadRespawn = 0;
			m_flDeadTime = -1;

			m_skill_respawn_time = gpGlobals->time + 120.0;

			if ( !(pev->flags & FL_FROZEN) ){
			UTIL_ScreenFade( this, Vector(255,255,255), 0.3, 0.5, 255, FFADE_IN );
			}

			m_flVelocityModifier = 0;

			pev->dmg_take		= 0;
			pev->dmg_save		= 0;
			pev->friction		= 1.0;
			pev->gravity		= 1.0;
			m_bitsHUDDamage		= -1;
			m_bitsDamageType	= 0;
			m_afPhysicsFlags	= 0;

			pev->fov = 0;
			m_iFOV = 0;
			m_iClientFOV		= -1; // make sure fov reset is sent

			m_iClient_mynpc     = -1;

			pev->sequence		= LookupActivity( ACT_IDLE );

			if(g_causality_add > 0)
			{
				if(m_skill_reload)
				{
					MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
					if(g_causality_add == 1)
					{
						WRITE_STRING( "c_lv_1" );
					}
					else if(g_causality_add == 2)
					{
						WRITE_STRING( "c_lv_2" );
					}
					else if(g_causality_add == 3)
					{
						WRITE_STRING( "c_lv_3" );
					}
					else if(g_causality_add == 4)
					{
						WRITE_STRING( "c_lv_4" );
					}
					else if(g_causality_add == 5)
					{
						WRITE_STRING( "c_lv_5" );
						pev->armorvalue = m_skill_maxarmor;
					}
					MESSAGE_END();
				}
				else
				{
					g_causality_add = 0;
				}
			}

			//	m_pLastItem = NULL;
			//	m_iClientHideHUD = -1;  // force this to be recalculated
			//	m_fWeapon = FALSE;
			//	m_fKnownItem = FALSE;
			//	m_iClientBattery = -1;
			//	m_fInitHUD = TRUE;

			m_blindUntilTime = 0;
			m_blindStartTime = 0;
			m_blindHoldTime = 0;
			m_blindFadeTime = 0;
			m_blindAlpha = 0;
		}
		else
		{
			SERVER_COMMAND("reload\n");
		}
	}
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission" );

	if( !FNullEnt( pSpot ) )
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG( 0, 3 );

		while( iRand > 0 )
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME( pSpot, "info_intermission" );

			if( pNewSpot )
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue( pev );

		UTIL_SetOrigin( pev, pSpot->v.origin );
		pev->angles = pev->v_angle = pSpot->v.v_angle;
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue( pev );
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &tr );

		UTIL_SetOrigin( pev, tr.vecEndPos );
		pev->angles = pev->v_angle = UTIL_VecToAngles( tr.vecEndPos - pev->origin );
	}

	// start death cam
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
}

void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	if( m_pTank != 0 )
		m_pTank->Use( this, this, USE_OFF, 0 );

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate( NULL, FALSE, 0 );

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0XFF );
		WRITE_BYTE( 0xFF );
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = ( HIDEHUD_HEALTH | HIDEHUD_FLASHLIGHT | HIDEHUD_WEAPONS );
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = TRUE;

	m_szTeamName[0] = '\0';
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_STRING( "" );
	MESSAGE_END();

	// Remove all the player's stuff
	RemoveAllItems( FALSE );

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode( m_iObserverLastMode );
}

//
// PlayerUse - handles USE keypress
//
#define	PLAYER_SEARCH_RADIUS	(float)64

void CBasePlayer::PlayerUse( void )
{
	if( IsObserver() )
		return;

	// Was use pressed or released?
	if( !( ( pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE ) )
		return;

	// Hit Use on a train?
	if( m_afButtonPressed & IN_USE )
	{
		if( m_pTank != 0 )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			return;
		}
		else
		{
			if( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;

				CBaseEntity *pTrain = Instance( pev->groundentity );
				if( pTrain && pTrain->Classify() == CLASS_VEHICLE )
				{
					( (CFuncVehicle *)pTrain )->m_pDriver = NULL;
				}
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if( pTrain && !( pev->button & IN_JUMP ) && FBitSet( pev->flags, FL_ONGROUND ) && ( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) && pTrain->OnControls( pev ) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed( (int)pTrain->pev->speed, pTrain->pev->impulse );
					m_iTrain |= TRAIN_NEW;

					if( pTrain->Classify() == CLASS_VEHICLE )
					{
						EMIT_SOUND( ENT( pev ), CHAN_ITEM, "vehicle/vehicle_start1.wav", 0.8, ATTN_NORM );
						( (CFuncVehicle *)pTrain )->m_pDriver = this;
					}
					else
						EMIT_SOUND( ENT( pev ), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM );
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors( pev->v_angle );// so we know which way we are facing

	while( ( pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_SEARCH_RADIUS ) ) != NULL )
	{
		if( pObject->ObjectCaps() & ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE ) )
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = ( VecBModelOrigin( pObject->pev ) - ( pev->origin + pev->view_ofs ) );

			// This essentially moves the origin of the target to the corner nearest the player to test to see 
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );

			flDot = DotProduct( vecLOS , gpGlobals->v_forward );
			if( flDot > flMaxDot )
			{
				// only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if( pObject )
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls			
		int caps = pObject->ObjectCaps();

		if( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM );

		if( ( ( pev->button & IN_USE ) && ( caps & FCAP_CONTINUOUS_USE ) ) ||
			 ( ( m_afButtonPressed & IN_USE ) && ( caps & ( FCAP_IMPULSE_USE | FCAP_ONOFF_USE ) ) ) )
		{
			if( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use( this, this, USE_SET, 1 );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if( ( m_afButtonReleased & IN_USE ) && ( pObject->ObjectCaps() & FCAP_ONOFF_USE ) )	// BUGBUG This is an "off" use
		{
			pObject->Use( this, this, USE_SET, 0 );
		}
	}
	else
	{
		if( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM );
	}
}

void CBasePlayer::Jump()
{
	Vector vecWallCheckDir;// direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;

	if( FBitSet( pev->flags, FL_WATERJUMP ) )
		return;

	if( pev->waterlevel >= 2 )
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if( !FBitSet( m_afButtonPressed, IN_JUMP ) )
		return;         // don't pogo stick

	if( !( pev->flags & FL_ONGROUND ) || !pev->groundentity )
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors( pev->angles );

	// ClearBits( pev->flags, FL_ONGROUND );		// don't stairwalk

	SetAnimation( PLAYER_JUMP );

	if( m_fLongJump &&
		( pev->button & IN_DUCK ) &&
		( pev->flDuckTime > 0 ) &&
		pev->velocity.Length() > 50 )
	{
		SetAnimation( PLAYER_SUPERJUMP );
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS( pev->groundentity );
	if( pevGround )
	{
		if( pevGround->flags & FL_CONVEYOR )
		{
			pev->velocity = pev->velocity + pev->basevelocity;
		}

		if( FClassnameIs( pevGround, "func_tracktrain" ) || FClassnameIs( pevGround, "func_train" ) || FClassnameIs( pevGround, "func_door" ) || FClassnameIs( pevGround, "func_door_breaker" ) || FClassnameIs( pevGround, "func_vehicle" ) )
		{
			pev->velocity = pevGround->velocity + pev->velocity;
		}
	}
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( edict_t *pPlayer )
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace );
		if( trace.fStartSolid )
			pPlayer->v.origin.z++;
		else
			break;
	}
}

void CBasePlayer::Duck()
{
	if( pev->button & IN_DUCK )
	{
		if( m_IdealActivity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

//
// ID's player as such.
//
int CBasePlayer::Classify( void )
{
	return CLASS_PLAYER;
}

void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore )
{
	// Positive score always adds
	if( score < 0 )
	{
		if( !bAllowNegativeScore )
		{
			if( pev->frags < 0 )		// Can't go more negative
				return;

			if( -score > pev->frags )	// Will this go negative?
			{
				score = (int)( -pev->frags );		// Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_SHORT( (int)pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( m_szTeamName ) + 1 );
	MESSAGE_END();
}

void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore )
{
	int index = entindex();

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if( pPlayer && i != index )
		{
			if( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

int CBasePlayer :: Game_Load_SecondData ()
{
    char    szFilename[MAX_PATH];
    int     length;
    byte    *aMemFile;
    byte    *pMemFile;

    snprintf( szFilename, sizeof(szFilename), "data/game_clear.%s", STRING(pev->netname) );

    pMemFile = aMemFile = LOAD_FILE_FOR_ME(szFilename, &length);

    if ( !aMemFile )
    {
    //  UTIL_SayTextAll( szFilename,this );
    //  UTIL_CenterPrintAll( "Fuck You" );
    //  fopen ( szFilename, "w+" );
        return FALSE;
    }
    else
    {
        m_fSecondWorld = TRUE;
        FREE_FILE(aMemFile);
        return TRUE;
    }

    return FALSE;
}

//=========================================================
// CGraph - FSaveGraph - It's not rocket science.
// this WILL overwrite existing files.
//=========================================================
int CBasePlayer::Game_Save_SecondData()
{
    FILE    *file;
    char    szNrpFilename [MAX_PATH];// text node report filename

    GET_GAME_DIR( szNrpFilename );
    strcat( szNrpFilename, "/data/game_clear." );
    strcat( szNrpFilename, STRING(pev->netname) );
    file = fopen ( szNrpFilename, "w+" );

    if ( file )
    {
        fprintf( file, "Game Clear Data:\n");
        fprintf( file, "Kills: %d\n",m_enemy_kills);
        fprintf( file, "Diamonds: %d\n",m_player_diamonds);
        fprintf( file, "Friendly: %d\n",m_ending_frags);

        int total_sec = (int)m_player_time;
        int gtime_h = total_sec / 3600;
        int gtime_m = (total_sec % 3600) / 60;
        int gtime_s = total_sec % 60;

        fprintf( file, "Play Time: %02d:%02d:%02d\n", gtime_h, gtime_m, gtime_s);
        /*
        if(g_fGameJumpCG == 11){
        fprintf( file, "Ending: S");
        }
        else if(g_fGameJumpCG == 1){
        fprintf( file, "Ending: A");
        }
        else if(g_fGameJumpCG == 2 || g_fGameJumpCG == 12){
        fprintf( file, "Ending: B");
        }
        else if(g_fGameJumpCG == 3 || g_fGameJumpCG == 13){
        fprintf( file, "Ending: C");
        }
        else if(g_fGameJumpCG == 4 || g_fGameJumpCG == 14){
        fprintf( file, "Ending: D");
        }
        else if(g_fGameJumpCG == 5 || g_fGameJumpCG == 15){
        fprintf( file, "Ending: E");
        }
        */
        fclose ( file );
        return TRUE;
    }

    return FALSE;
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0; 
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END] = {0};
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[ SBAR_STRING_SIZE ];

	strcpy( sbuf0, m_SbarString0 );
	strcpy( sbuf1, m_SbarString1 );

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors( pev->v_angle + pev->punchangle );
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + ( gpGlobals->v_forward * MAX_ID_RANGE );
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr );

	if( tr.flFraction != 1.0f )
	{
		if( !FNullEnt( tr.pHit ) )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			if( pEntity->pev->flags & (FL_MONSTER) && pEntity->pev->deadflag == DEAD_NO && !(pEntity->pev->effects & EF_NODRAW ) )
			{
				if (pEntity->Classify() == CLASS_PLAYER_ALLY)
				{
					CBaseMonster *pEnemyMonster;
					pEnemyMonster = pEntity->MyMonsterPointer();
					if(pEnemyMonster)
					{
						if(pEnemyMonster->m_rpgms_inteam > 0)
						{
							newSBarState[ SBAR_ID_TARGETHEALTH ] = (pEntity->pev->health / pEntity->pev->max_health) * 100;
							//newSBarState[ SBAR_ID_TARGETARMOR ] = pEnemyMonster->m_lovehate;
							strcpy( sbuf1, "1 %p1\n2[HP: %i2%%]" );
							//strcpy( sbuf1, "1 %p1\n2[HP: %i2%%\n3 LP: %i3%%]" );
							m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
						}
					}
				}
			}
		}
		else if( m_flStatusBarDisappearDelay > gpGlobals->time )
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	BOOL bForceResend = FALSE;

	if( strcmp( sbuf0, m_SbarString0 ) || g_restore_fix > 0 )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( sbuf0 );
		MESSAGE_END();

		strcpy( m_SbarString0, sbuf0 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if( strcmp( sbuf1, m_SbarString1 ) || g_restore_fix > 0 )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_STRING( sbuf1 );
		MESSAGE_END();

		strcpy( m_SbarString1, sbuf1 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for( int i = 1; i < SBAR_END; i++ )
	{
		if( newSBarState[i] != m_izSBarState[i] || bForceResend )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( newSBarState[i] );
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

#define CLIMB_SHAKE_FREQUENCY		22	// how many frames in between screen shakes when climbing
#define	MAX_CLIMB_SPEED			200	// fastest vertical climbing speed possible
#define	CLIMB_SPEED_DEC			15	// climbing deceleration rate
#define	CLIMB_PUNCH_X			-7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z			7	// how far to 'punch' client Z axis when climbing

void CBasePlayer::PreThink( void )
{
	int buttonsChanged = ( m_afButtonLast ^ pev->button );	// These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & ( ~pev->button );	// The ones not down are "released"

	g_pGameRules->PlayerThink( this );

	if( g_fGameOver )
		return;         // intermission or finale

	UTIL_MakeVectors( pev->v_angle );             // is this still used?

	if(m_fNextClearTextTime > 0 && m_fNextClearTextTime < gpGlobals->time)
	{
		Clear_SayText();
	}

	if(m_player_time_now < gpGlobals->time)
	{
		m_player_time_now = gpGlobals->time + 1.0;
		m_player_time += 1;
	}

	if(m_pActiveItem != NULL)
	{
		if(m_grenadeboomidle >= 0 && m_pActiveItem->m_iId != WEAPON_HANDGRENADE && m_grenadeboomtime != 0)
		{
			float time = m_grenadeboomtime - gpGlobals->time + 3.0;
			if (time < 0){
			SelectItem("weapon_handgrenade");
			}
		}
		else if(m_grenadeboomidle2 >= 0 && m_pActiveItem->m_iId != WEAPON_DARKGRENADE && m_grenadeboomtime2 != 0)
		{
			float time = m_grenadeboomtime2 - gpGlobals->time + 3.0;
			if (time < 0){
			SelectItem("weapon_darkgrenade");
			}
		}
	}

	ItemPreFrame();
	WaterMove();

	if(g_gibexp_max > 0)g_gibexp_max--;

	if (!FNullEnt(m_boss_find))
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgLifeLoad, NULL, pev );
		WRITE_BYTE( m_boss_on );
		if(m_boss_find->pev->health > 0)
		{
			float hp = 520 * (m_boss_find->pev->health / m_boss_find->pev->max_health);
			if(hp < 1){hp = 1;}
			WRITE_SHORT( (int)hp  );
			WRITE_BYTE( m_boss_type );
			WRITE_LONG((int)m_boss_find->pev->health );
		}
		else
		{
			m_boss_find = NULL;
			m_boss_type = 0;
			m_boss_on = 0;
			WRITE_SHORT( 1  );
			WRITE_BYTE( 0 );
			WRITE_LONG(0);
		}
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgLifeLoad, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_SHORT( 1  );
		WRITE_BYTE( 0 );
		WRITE_LONG(0);
		MESSAGE_END();
	}
		

	if(m_new_spawner > 0)
	{
		m_new_spawner -= 1;
	}
	if(m_new_spawner == 1)
	{
		CLIENT_COMMAND(edict(), "-cammousemove\n");
	}
	
	if ( FlashlightIsOn() )
	{
		int flash_dist = 2048;
		int radius = 18;

		if(m_flash_mode >= 2)
		{
			flash_dist = 136;
			radius = 24;
		}

		TraceResult trf;
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs,pev->origin + pev->view_ofs + gpGlobals->v_forward * flash_dist,dont_ignore_monsters, edict(), &trf );

		 MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			  WRITE_BYTE( TE_DLIGHT );
			  WRITE_COORD( trf.vecEndPos.x ); // origin
			  WRITE_COORD( trf.vecEndPos.y );
			  WRITE_COORD( trf.vecEndPos.z );
			  WRITE_BYTE( radius );     // radius
			  WRITE_BYTE( 240 );     // R
			  WRITE_BYTE( 240 );     // G
			  WRITE_BYTE( 240 );     // B
			  WRITE_BYTE( 0 );     // life * 10
			  WRITE_BYTE( 0 ); // decay
		 MESSAGE_END();
	}

	if(m_barnacle_RTP == 0 && m_concussion_time <= gpGlobals->time)
	{
		if(pev->movetype == MOVETYPE_NOCLIP || m_skill_darkhide_on || m_trainning == 2)
		{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 400.0);
		}
		else if(m_wrongdoor_time >= 1)
		{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 200.0);
		}
		else if(m_fequip2 == TRUE)
		{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 330.0);
		}
		else{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 300.0);
		}
	}
	else
	{
		if(m_barnacle_Level == 1 && m_barnacle_catchme != NULL)
		{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 200.0);
		}
		else if(m_barnacle_RTP == 0 && m_concussion_time > 0)
		{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 100.0);
		}
		else
		{
			g_engfuncs.pfnSetClientMaxspeed(ENT(pev), 1);
		}
	}

	if(pev->health > 0 && pev->health < 1)
	{
		pev->health = 1;
	}

	if(m_fMoveItem != NULL)
	{
		if(pev->viewmodel != 0)
		{
			pev->viewmodel = 0;
			if (m_pActiveItem)
			{
				m_pActiveItem->Holster();
			}
		} 

		if(m_fMoveItem->pev->armorvalue < 50)
		{
			m_fMoveItem->pev->armorvalue += 1;
		}

		if(m_barnacle_RTP != 0)
		{
			m_fMoveItem->pev->movetype = MOVETYPE_BOUNCE;

			UTIL_MakeVectors(pev->v_angle);
			Vector vecSrc = pev->origin + gpGlobals->v_forward * 16;
			Vector vecThrow = gpGlobals->v_forward * 500 + pev->velocity;

			m_fMoveItem->pev->velocity = vecThrow;

			m_fMoveItem->pev->avelocity.x = pev->velocity.Length();
			m_fMoveItem->pev->avelocity.y = RANDOM_FLOAT( -pev->velocity.Length(), pev->velocity.Length() );

			m_fMoveItem->pev->armorvalue = 2;
			m_fMoveItem->pev->owner = NULL;
			m_fMoveItem = NULL;

			if (m_pActiveItem)
			{
				m_pActiveItem->Deploy();
			}
		}
		else
		{
			Vector moveorigin;
			UTIL_MakeVectors(pev->angles);

			m_fMoveItem->pev->avelocity = g_vecZero;
			Vector vecsrc;

			if(m_fMoveItem->pev->body == 0)
			{
				m_fMoveItem->pev->angles.x = 90;
				m_fMoveItem->pev->angles.y = pev->angles.y;
				m_fMoveItem->pev->angles.z = 0;
				vecsrc = pev->origin + Vector(0,0,8) + gpGlobals->v_right * 8;
			}
			else
			{
				vecsrc = pev->origin - Vector(0,0,8) + gpGlobals->v_right * 8;
			}
			
			TraceResult tr;
			UTIL_TraceLine(vecsrc, vecsrc + gpGlobals->v_forward * 24, dont_ignore_monsters, ENT(pev), &tr);
			if ( tr.flFraction < 1.0 )
			{
				moveorigin = tr.vecEndPos + (tr.vecPlaneNormal * 8);
			}
			else
			{
				moveorigin = vecsrc + gpGlobals->v_forward * 24;
			}
			UTIL_SetOrigin(m_fMoveItem->pev,moveorigin);
		}
	}

	if(m_trainning == 0 && m_barnacle_RTP == 0 && m_barnacle_god_time <= gpGlobals->time && pev->movetype == MOVETYPE_WALK)
	{
		if(pev->iuser4 == 0)
		{
			m_stuck_inter = 0;
			m_stuck_origin = pev->origin;
		}
		if(pev->iuser4 == 1 && pev->health > 0)
		{
			TraceResult trace;
			Vector checkorigin;
			int hull = human_hull;
			if(m_stuck_inter == 0)
			{
				checkorigin = pev->origin + Vector(0,0,1);
			}
			else
			{
				checkorigin = pev->origin + Vector(RANDOM_LONG(-m_stuck_inter,m_stuck_inter),RANDOM_LONG(-m_stuck_inter,m_stuck_inter),RANDOM_LONG(-m_stuck_inter,m_stuck_inter));
			}
			if ( FBitSet( pev->flags, FL_DUCKING ) )
			{
				hull = head_hull;
			}
			UTIL_TraceHull(checkorigin, checkorigin, dont_ignore_monsters, hull, ENT(pev),&trace);
			if (trace.fStartSolid == 0)
			{
				pev->origin = checkorigin;
			}
			if(m_stuck_inter > 512)
			{
				pev->origin = m_stuck_origin;
				m_stuck_inter = 0;
			}

			m_stuck_inter++;
			pev->iuser4 = 2;
		}
	}
	else
	{
		//m_stuck_origin = pev->origin;
		m_stuck_inter = 0;
		pev->iuser4 = 0;
	}

	if ( pev->flags & FL_ONGROUND ) 
	{	
		if(g_StartDark)
		{
			m_flVelocityModifier -= 5;
			g_StartDark = FALSE;
		}

		if (m_flVelocityModifier < 1)
		{
			if(m_flVelocityModifier < 1)
			{
				if(m_flVelocityModifier < -4)
				{
					m_flVelocityModifier = -4;
				}
				m_flVelocityModifier += 0.01;
				if (m_flVelocityModifier < 0)
				{
					pev->velocity = pev->velocity * 0.01;
				}
				else{
					pev->velocity = pev->velocity * m_flVelocityModifier;
				}
			}
		}

		if (m_flVelocityModifier > 1)
		{
			m_flVelocityModifier = 1;
		}

		if (m_flVelocityModifier2 != 1)
		{
			m_flVelocityModifier2 = 1;
		}

		if ( pev->flags & FL_DUCKING ) 
		{
			if( m_iWeaponFlash >=256)
			{
				m_newcross_size = 3;
			}
			else if ( m_iWeaponFlash >=128)
			{
				m_newcross_size = 2;
			}
			else
			{
				m_newcross_size = 1;
			}
		}
		else // si no es porq esta parado
		{
			if ( (pev->velocity.Length2D() >= 100 && pev->velocity.Length2D() < 220 ) )//walk 220 //120-300
			{
				if( m_iWeaponFlash >=256)
				{
					m_newcross_size = 5;
				}
				else if ( m_iWeaponFlash >=128)
				{
					m_newcross_size = 4;
				}
				else
				{
					m_newcross_size = 3;
				}
			}
			else if (pev->velocity.Length2D() >= 220 ) //RUN
			{			
				if ( m_iWeaponFlash >=128)
				{
					m_newcross_size = 5;
				}
				else
				{
					m_newcross_size = 4;
				}
			}
			else//solo esta parado
			{
				if( m_iWeaponFlash >=256){
					m_newcross_size = 4;
				}
				else if ( m_iWeaponFlash >=128)
				{
					m_newcross_size = 3;
				}
				else
				{
					m_newcross_size = 2;
				}
			}
		}
	}
	else // si no esta en el suelo es porque esta en el aire
	{
		m_newcross_size = 5;
		if (m_flVelocityModifier2 < 1)
		{
			if(m_climbspark >= 3)
			{
				UTIL_Sparks( m_vecClimb );
				m_climbspark = 0;
			}
			else
			{
				m_climbspark += 1;
			}

			if(m_pActiveItem->m_iId != WEAPON_FIST)
			{
				m_flVelocityModifier2 += 0.2;
			}

			if(pev->origin.z - 36 > m_vecClimb.z)
			{
				m_flVelocityModifier2 += 0.1;
			}

			m_flVelocityModifier2 += 0.01;
			if (m_flVelocityModifier2 < 0)
			{
				pev->velocity = pev->velocity * 0;
			}
			else
			{
				pev->velocity = pev->velocity * m_flVelocityModifier2;
			}

			if(gpGlobals->time >= m_ClimbWallTime && m_air_oxyan > 1)
			{
				m_air_oxyan -= 80;
				m_ClimbWallTime = gpGlobals->time + 0.1;
			}
			else if(m_air_oxyan <= 1)
			{
				m_air_oxyan = 1;
				m_flVelocityModifier2 += 0.1;
			}

			pev->velocity.z += 36 - (16 * m_flVelocityModifier2);

			if(m_vecClimb.y > pev->origin.y)
				pev->velocity.y += 24 - (8 * m_flVelocityModifier2);
			else
				pev->velocity.y -= 24 - (8 * m_flVelocityModifier2);

			if(m_vecClimb.x > pev->origin.x)
				pev->velocity.x += 24 - (8 * m_flVelocityModifier2);
			else
				pev->velocity.x -= 24 - (8 * m_flVelocityModifier2);

		}
		else if(m_ClimbWallTime != 0)
		{
			m_ClimbWallTime = 0;
		}

		if (m_flVelocityModifier2 > 1){
			m_flVelocityModifier2 = 1;
		}
	}

	if(pev->health > 0 && pev->deadflag == DEAD_NO)
	{
		if(m_barnacle_RTP > 0 || m_air_oxyan_stop_time > gpGlobals->time 
		|| pev->waterlevel == 3 || m_MonsterCatchTime != 0 ||
		(m_ClimbWallTime != 0 && !FBitSet( pev->flags, FL_ONGROUND ))
		|| m_concussion_time > gpGlobals->time )
		{
			m_RecoverTime = 0;
		}

		if(m_barnacle_RTP_relase == 1)
		{
			m_barnacle_god_time = gpGlobals->time + 3.0;
			m_barnacle_RTP_relase = 0;
			if (m_pActiveItem)
			{
				m_pActiveItem->Deploy();
			}
		}

		if(m_concussion_time < gpGlobals->time && m_concussion_time != 0)
		{
			if (m_pActiveItem)
			{
				m_pActiveItem->Deploy();
			}
			m_concussion_time = 0;
		}

		if(m_barnacle_RTP > 0)
		{
			if(pev->viewmodel != 0)
			{
				pev->viewmodel = 0;
				if (m_pActiveItem)
				{
					m_pActiveItem->Holster();
				}
			} 
			if(m_MonsterCatchTime == 0)
			{
				m_MonsterCatchTime = gpGlobals->time;
			}
			else if(gpGlobals->time >= m_MonsterCatchTime + 0.1)
			{
				if(m_barnacle_Level > 0)
				{
					m_air_oxyan -= (gpGlobals->time - m_MonsterCatchTime) * m_barnacle_Level * 500;
				}

				if(m_barnacle_RTP_bar > 0)
				{
					m_barnacle_RTP_bar -= (gpGlobals->time - m_MonsterCatchTime) * 20;
				}

				m_MonsterCatchTime = gpGlobals->time;
			}

			if ( m_barnacle_catchme != NULL )
			{
				if(FClassnameIs( m_barnacle_catchme->pev, "monster_headcrab") || FClassnameIs( m_barnacle_catchme->pev, "monster_headcrab_throw") )
				{
					if(m_barnacle_catchme->pev->health > 0)
					{
						if(m_barnacle_catchme->pev->owner != ENT(pev))
						{
							m_barnacle_catchme->pev->owner = ENT(pev);
							m_barnacle_catchme->pev->movetype = MOVETYPE_FLY;
						}
						else if(pev->health <= 0)
						{
							m_barnacle_catchme->pev->owner = NULL;
							m_barnacle_catchme->pev->movetype = MOVETYPE_STEP;
						}
						UTIL_MakeVectors(pev->v_angle);
						m_barnacle_catchme->pev->sequence = 20;
						UTIL_SetOrigin(m_barnacle_catchme->pev,pev->origin + Vector(0,0,16) + gpGlobals->v_forward * 16 + gpGlobals->v_up * 4);
					}
					else
					{
						m_barnacle_RTP_relase = 1;
						m_barnacle_draw_time = gpGlobals->time + 0.5;
						m_barnacle_RTP = 0;
						m_barnacle_RTP_bar = 0;
						m_barnacle_Level = 0;
						m_barnacle_catchme = NULL;
					}
									
					if(m_barnacle_RTP_bar >= 255)
					{
						UTIL_SetOrigin(m_barnacle_catchme->pev,pev->origin + Vector(0,0,16) + gpGlobals->v_forward * 32 + gpGlobals->v_up * 4);
						m_barnacle_catchme->TakeDamage ( pev, pev, 60, DMG_NEVERGIB );
						m_barnacle_RTP_relase = 1;
						m_barnacle_draw_time = gpGlobals->time + 0.5;
						m_barnacle_RTP = 0;
						m_barnacle_RTP_bar = 0;
						m_barnacle_Level = 0;
						m_barnacle_catchme = NULL;
					}
				}
				else if(FClassnameIs( m_barnacle_catchme->pev, "sciheadclaw") )
				{
					if(m_barnacle_RTP_bar >= 255 || m_barnacle_catchme->pev->health <= 2)
					{
						m_barnacle_catchme->pev->health = 0;
						m_barnacle_catchme->pev->frags = -1;
						m_barnacle_RTP_relase = 1;
						m_barnacle_draw_time = gpGlobals->time + 0.5;
						m_barnacle_RTP = 0;
						m_barnacle_RTP_bar = 0;
						m_barnacle_Level = 0;
						m_barnacle_catchme = NULL;
					}
					else
					{
						pev->origin = m_barnacle_catchme->pev->origin + Vector(0,0,36);
					}
				}

				if(m_air_oxyan <= 0)
				{
					if(FClassnameIs( m_barnacle_catchme->pev, "monster_bloodsucker") || FClassnameIs( m_barnacle_catchme->pev, "monster_vanlve"))
					{
					}
					else if(FClassnameIs( m_barnacle_catchme->pev, "monster_barnacle") || FClassnameIs( m_barnacle_catchme->pev, "monster_barnacle_fantasy") || FClassnameIs( m_barnacle_catchme->pev, "monster_barnacle_fantasy_r"))
					{
						if(m_barnacle_Level == 3)
						{
							TakeDamage(pev, pev, 2, DMG_SLASH);
						}
					}
					else
					{
						TakeDamage(pev, pev, m_barnacle_Level, DMG_AIR);
					}
					m_air_oxyan = 1;
				}
			}
			else
			{
				m_barnacle_RTP_relase = 1;
				m_barnacle_draw_time = gpGlobals->time + 0.5;
				m_barnacle_RTP = 0;
				m_barnacle_RTP_bar = 0;
				m_barnacle_Level = 0;
				m_barnacle_catchme = NULL;
			}

		}
		
		if(m_MonsterCatchTime != 0 && m_barnacle_RTP == 0)
			m_MonsterCatchTime = 0;

		if(m_barnacle_RTP == 1)
		{
			m_Fast_RTP_Show = 1;
		}
		else 
		{
			m_Fast_RTP_Show = 0;
		}
				
		if(m_skill_darkhide_on)
		{
			if(m_RecoverTime == 0)
			{
				m_RecoverTime = gpGlobals->time;
			}
			else if(gpGlobals->time >= m_RecoverTime + 0.1)
			{
				m_darkposion -= (gpGlobals->time - m_RecoverTime) * 400;
				m_RecoverTime = gpGlobals->time;
			}
			if(m_barnacle_RTP != 0 || m_iWeaponFlash >= 128)
			{
				m_darkposion = 0;
			}
			if(m_darkposion <= 0)
			{
				m_skill_darkhide_on = FALSE;
				m_darkposion = 0;
				m_iClientHealth = -1;
				m_iClient_oxyan = -1;
				pev->flags &= ~FL_NOTARGET;
				m_fldarkhideTime = gpGlobals->time + 35;
			}
		}		

		if(pev->waterlevel < 3)
		{
			if(m_IntoWaterTime > 0)
			{
				m_IntoWaterTime = 0;
			}
			if(m_barnacle_RTP == 0 && !m_skill_darkhide_on)
			{
				if(m_air_oxyan_stop_time <= gpGlobals->time && m_air_oxyan < m_air_oxyan_max)
				{
					if(m_RecoverTime == 0)
					{
						m_RecoverTime = gpGlobals->time;
					}
					else if(gpGlobals->time >= m_RecoverTime + 0.1)
					{
						if(m_air_oxyan_max <= 2000)
						{
							if(pev->velocity.Length() < 100)
							{
								m_air_oxyan += (gpGlobals->time - m_RecoverTime) * 330;
							}
							else
							{
								m_air_oxyan += (gpGlobals->time - m_RecoverTime) * 250;
							}
						}
						else
						{
							if(pev->velocity.Length() < 100)
							{
								m_air_oxyan += (gpGlobals->time - m_RecoverTime) * 400;
							}
							else
							{
								m_air_oxyan += (gpGlobals->time - m_RecoverTime) * 300;
							}
						}
						m_RecoverTime = gpGlobals->time;
					}
				}
				
				if(pev->fuser4 == 2)
				{
					m_air_oxyan_stop_time = gpGlobals->time + 3.5;
					if(m_air_oxyan >= 900)
					{
						pev->fuser4 = 0;
						m_air_oxyan -= 900;
						EMIT_SOUND(ENT(pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
						//FX_Explosion( Center(), EXPLOSION_SPARKSHOWER );
					}
					else
					{
						pev->fuser4 = 1;
					}
				}
				else
				{
					if(m_concussion_time > 0 || m_air_oxyan < 900 || m_darkposion > 0 || m_wrongdoor_time > 0)
					{
						pev->fuser4 = 1;
					}
					else
					{
						pev->fuser4 = 0;
					}
				}
			}
		}
		else
		{
			if(pev->waterlevel == 3)
			{
				if(m_IntoWaterTime == 0)
				{
					m_IntoWaterTime = gpGlobals->time;
				}
				else if(gpGlobals->time >= m_IntoWaterTime + 0.1)
				{
					m_air_oxyan -= (gpGlobals->time - m_IntoWaterTime) * 60;
					m_IntoWaterTime = gpGlobals->time;
				}

				if(m_darkposion > 0)
				{
					m_skill_darkhide_on = FALSE;
					m_darkposion = 0;
					m_iClientHealth = -1;
					m_iClient_oxyan = -1;
					pev->flags &= ~FL_NOTARGET;
					m_fldarkhideTime = gpGlobals->time + 35;
				}

				if(m_air_oxyan <= 0)
				{
					m_air_oxyan = 1;
					TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 2, DMG_DROWN);
				}
			}
		}
				
		if(m_air_oxyan < m_air_oxyan_max)
		{
			m_air_show = 1;
		}
		else
		{
			if(m_air_oxyan > m_air_oxyan_max)
			{
				m_air_oxyan = m_air_oxyan_max;
			}
			m_air_show = 0;
		}
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
	WRITE_BYTE( 1 );
	if(pev->viewmodel == 0)
	{
		WRITE_BYTE( 0 );
	}
	else
	{
		WRITE_BYTE( m_newcross_active );
	}
	WRITE_BYTE( m_newcross_size );
	WRITE_BYTE( m_newcross_ontarget );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgRTPbar, NULL, pev );
	WRITE_BYTE( m_Fast_RTP_Show );
	if(m_barnacle_RTP_bar == 0)
	{
		WRITE_BYTE( 1 );
	}
	else
	{
		WRITE_BYTE( m_barnacle_RTP_bar );
	}
	WRITE_BYTE( 255 );
	MESSAGE_END();

	if(m_hasflashlight)
	{
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	}
	else
	{
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;
	}

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	if (pev->deadflag != DEAD_NO)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 0 );
		MESSAGE_END();

		if(m_flDeadTime <= gpGlobals->time + 3.0 && (!m_skill_respawn || m_skill_respawn_time > gpGlobals->time) )
		{
			m_skill_respawn_time = gpGlobals->time + 120.0;
			m_skill_respawn = 0;

			m_gameoveralpha += 1;
			if(m_gameoveralpha > 255)
			{
				m_gameoveralpha = 255;
			}

			if(m_trainning == 3)
			{
				MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
				WRITE_BYTE( ENTINDEX(edict()) );
				WRITE_STRING( NULL );
				WRITE_BYTE( 1 );
				MESSAGE_END();
			}

			MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
			WRITE_BYTE( m_gameoveralpha );//Alpha
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}

		PlayerDeathThink();
		return;
	}
	else
	{
		if(m_iClient_Gameover == 3)
		{
			if(m_fGameOverTime <= gpGlobals->time)
			{
				SERVER_COMMAND("disconnect\n");
			}
		}
		else if(m_iClient_Gameover == 0)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			MESSAGE_END();

			m_iClient_Gameover = 1;
		}
		else if(m_iClient_Gameover <= -1)
		{
			if(m_fGameOverTime <= gpGlobals->time + 4.0)
			{
				m_gameoveralpha += 1;
				if(m_gameoveralpha > 255)
				{
					m_gameoveralpha = 255;
				}

				MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
				WRITE_BYTE( m_gameoveralpha );
				WRITE_BYTE( 1 );
				MESSAGE_END();
			}
			if(m_fGameOverTime <= gpGlobals->time)
			{
				SERVER_COMMAND("reload\n");
			}
		}
		else if(m_iClient_Gameover == 2)
		{
			if(m_fGameOverTime <= gpGlobals->time)
			{
				m_gameoveralpha -= 2;
				if(m_gameoveralpha <= 1)
				{
					m_iClient_Gameover = 0;
				}
			}
			else if(m_fGameOverTime <= gpGlobals->time + 4.0)
			{
				m_gameoveralpha += 2;
				if(m_gameoveralpha > 255)
				{
					m_gameoveralpha = 255;
				}	
			}

			MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
			WRITE_BYTE( m_gameoveralpha );
			WRITE_BYTE( 2 );
			MESSAGE_END();
		}
			
		if ( m_iNVG == 1 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 2 );
			MESSAGE_END();
		}
		else if (m_iNVG == 2)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			MESSAGE_END();
		}
	}

	if( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	if (pev->deadflag != DEAD_NO)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 0 );
		MESSAGE_END();

		if(m_flDeadTime <= gpGlobals->time + 3.0 && (!m_skill_respawn || m_skill_respawn_time > gpGlobals->time) )
		{
			m_skill_respawn_time = gpGlobals->time + 120.0;
			m_skill_respawn = 0;

			m_gameoveralpha += 1;
			if(m_gameoveralpha > 255){
			m_gameoveralpha = 255;
			}

			if(m_trainning == 3){//Bug Fix 3.0 ͷзͶ����ʽ
			MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( ENTINDEX(edict()) );
			WRITE_STRING( NULL );
			WRITE_BYTE( 1 );
			MESSAGE_END();
			}

			MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
			WRITE_BYTE( m_gameoveralpha );//Alpha
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}

		PlayerDeathThink();
		return;
	}
	else
	{
		if(m_iClient_Gameover == 3)
		{
			if(m_fGameOverTime <= gpGlobals->time)
			{
				SERVER_COMMAND("disconnect\n");
			}
		}
		else if(m_iClient_Gameover == 0)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			MESSAGE_END();

			m_iClient_Gameover = 1;
		}
		else if(m_iClient_Gameover <= -1)
		{
			if(m_fGameOverTime <= gpGlobals->time + 4.0)
			{
				m_gameoveralpha += 1;
				if(m_gameoveralpha > 255)
				{
					m_gameoveralpha = 255;
				}

				MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
				WRITE_BYTE( m_gameoveralpha );
				WRITE_BYTE( 1 );
				MESSAGE_END();
			}
			if(m_fGameOverTime <= gpGlobals->time)
			{
				SERVER_COMMAND("reload\n");
			}
		}
		else if(m_iClient_Gameover == 2)
		{
			if(m_fGameOverTime <= gpGlobals->time)
			{
				m_gameoveralpha -= 2;
				if(m_gameoveralpha <= 1)
				{
					m_iClient_Gameover = 0;
				}
			}
			else if(m_fGameOverTime <= gpGlobals->time + 4.0){
				m_gameoveralpha += 2;
				if(m_gameoveralpha > 255)
				{
					m_gameoveralpha = 255;
				}	
			}

			MESSAGE_BEGIN( MSG_ONE, gmsgGameOver, NULL, pev );
			WRITE_BYTE( m_gameoveralpha );
			WRITE_BYTE( 2 );
			MESSAGE_END();
		}

			
		if ( m_iNVG == 1 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 2 );
			MESSAGE_END();
		}
		else if (m_iNVG == 2)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgNVG, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			MESSAGE_END();
		}
	}

	// Observer Button Handling
	/*if( IsObserver() )
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		pev->impulse = 0;
		return;
	}

	if( pev->deadflag >= DEAD_DYING )
	{
		PlayerDeathThink();
		return;
	}*/

	// So the correct flags get sent to client asap.
	//
	if( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;
		int iGearId;	// Vit_amiN: keeps the train control HUD in sync

		if( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -38 ), ignore_monsters, ENT( pev ), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if( trainTrace.flFraction != 1.0f && trainTrace.pHit )
			pTrain = CBaseEntity::Instance( trainTrace.pHit );

			if( !pTrain || !( pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE ) || !pTrain->OnControls( pev ) )
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				/*if( pTrain )
					( (CFuncVehicle *)pTrain )->m_pDriver = NULL;*/
				return;
			}
		}
		else if( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			//( (CFuncVehicle *)pTrain )->m_pDriver = NULL;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;

		if( pTrain->Classify() == CLASS_VEHICLE )
		{
			if( pev->button & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, vel );
			}

			if( pev->button & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, vel );
			}

			if( pev->button & IN_MOVELEFT )
			{
				vel = 20;
				pTrain->Use( this, this, USE_SET, vel );
			}
			if( pev->button & IN_MOVERIGHT )
			{
				vel = 30;
				pTrain->Use( this, this, USE_SET, vel );
			}
		}
		else
		{
			if( m_afButtonPressed & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, vel );
			}
			else if( m_afButtonPressed & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, vel );
			}
		}
		iGearId = TrainSpeed( pTrain->pev->speed, pTrain->pev->impulse );

		if( iGearId != ( m_iTrain & 0x0F ) )	// Vit_amiN: speed changed
		{
			m_iTrain = iGearId;
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if( m_iTrain & TRAIN_ACTIVE )
		m_iTrain = TRAIN_NEW; // turn off train

	if( pev->button & IN_JUMP )
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if( ( pev->button & IN_DUCK ) || FBitSet( pev->flags,FL_DUCKING ) || ( m_afPhysicsFlags & PFLAG_DUCKING ) )
		Duck();

	if (m_skill_miss && m_air_oxyan >= 1500)
	{
		if((pev->button & IN_DUCK) && (pev->button & IN_SCORE) && FBitSet ( pev->flags, FL_ONGROUND ) && pev->fuser4 == 0 && !FBitSet(pev->flags,FL_DUCKING))
		{
			m_air_oxyan -= 1500;
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
			m_air_oxyan_stop_time = gpGlobals->time + 3.5;
			pev->fuser4 = 3;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * 4500 + pev->velocity;
			pev->velocity.z = 0;

			TraceResult tr;
			UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + gpGlobals->v_forward * 256, dont_ignore_monsters, edict(), &tr);
			if(tr.flFraction != 1)
			{
				// What the hell are you doing?
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if ( pEntity )
				{
					ClearMultiDamage( );
					pEntity->TraceAttack(pev, 100, gpGlobals->v_forward, &tr, DMG_FALL); 
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "newadd/fist_hitbod3.wav", 1, ATTN_NORM); 
					MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
					WRITE_BYTE(TE_STREAK_SPLASH);
					WRITE_COORD(tr.vecEndPos.x);
					WRITE_COORD(tr.vecEndPos.y);
					WRITE_COORD(tr.vecEndPos.z);
					WRITE_COORD(tr.vecPlaneNormal.x);
					WRITE_COORD(tr.vecPlaneNormal.y);
					WRITE_COORD(tr.vecPlaneNormal.z);
					WRITE_BYTE(10);
					WRITE_SHORT(30);
					WRITE_SHORT(50);
					WRITE_SHORT(300);
					MESSAGE_END();
					UTIL_Ricochet( tr.vecEndPos, 1 );
					int tex = (int)TEXTURETYPE_Trace(&tr, pev->origin, tr.vecEndPos);
					int surface = (int)SURFACETYPE_Trace(&tr, pev->origin, tr.vecEndPos,Classify(),0);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, pev->origin, surface, BULLET_CROWBAR, (float)tex );

					ApplyMultiDamage( pev, pev );
				}
			}
		}
	}


	if( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance();//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}
}
/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON		(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID		(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION		30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION		16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE			2.0

//#define RADIATION_DURATION		50
//#define RADIATION_DAMAGE		1.0

//#define ACID_DURATION			10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION		2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION		1.0
//#define SLOWFREEZE_DAMAGE		3.0

void CBasePlayer::CheckTimeBasedDamage() 
{
	if (m_needleheal2 > 0)
	{
		m_bitsDamageType &= ~DMG_POISON;
		m_rgbTimeBasedDamage[itbd_Poison] = 0;
		
		m_bitsDamageType &= ~DMG_NERVEGAS;
		m_rgbTimeBasedDamage[itbd_NerveGas] = 0;
	}

	int i;
	BYTE bDuration = 0;

	//static float gtbdPrev = 0.0;

	if( !( m_bitsDamageType & DMG_TIMEBASED ) )
		return;

	// only check for time based damage approx. every 2 seconds
	if( fabs( gpGlobals->time - m_tbdPrev ) < 2.0f )
		return;

	m_tbdPrev = gpGlobals->time;

	for( i = 0; i < CDMG_TIMEBASED; i++ )
	{
		// make sure bit is set for damage type
		if( m_bitsDamageType & ( DMG_PARALYZE << i ) )
		{
			switch( i )
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				if ( !m_fMask )
				{
					TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_AIR);	
				}
				//TakeDamage( pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC );
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage( pev, pev, POISON_DAMAGE, DMG_AIR );
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//TakeDamage( pev, pev, RADIATION_DAMAGE, DMG_GENERIC );
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if( m_idrowndmg > m_idrownrestored )
				{
					int idif = Q_min( m_idrowndmg - m_idrownrestored, 10 );

					//TakeHealth( idif, DMG_GENERIC );
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//TakeDamage( pev, pev, ACID_DAMAGE, DMG_GENERIC );
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//TakeDamage( pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC );
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//TakeDamage( pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC );
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if( m_rgbTimeBasedDamage[i] )
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage					
				/*if( ( ( i == itbd_NerveGas ) && ( m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION ) ) ||
					( ( i == itbd_Poison ) && ( m_rgbTimeBasedDamage[i] < POISON_DURATION ) ) )
				{
					if( m_rgItems[ITEM_ANTIDOTE] )
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate( "!HEV_HEAL4", FALSE, SUIT_REPEAT_OK );
					}
				}*/

				// decrement damage duration, detect when done.
				if( !m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0 )
				{
					m_rgbTimeBasedDamage[i] = 0;

					// if we're done, clear damage bits
					m_bitsDamageType &= ~( DMG_PARALYZE << i );	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25f

void CBasePlayer::UpdateGeigerCounter( void )
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if( gpGlobals->time < m_flgeigerDelay )
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client
	range = (BYTE)( m_flgeigerRange / 4 );

	if( range != m_igeigerRangePrev )
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( range );
		MESSAGE_END();
	}

	// reset counter and semaphore
	if( !RANDOM_LONG( 0, 3 ) )
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME		3.5f
#define SUITFIRSTUPDATETIME	0.1f

void CBasePlayer::CheckSuitUpdate()
{
	/*int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if( !( pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if( g_pGameRules->IsMultiplayer() )
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if( gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0 )
	{
		// play a sentence off of the end of the queue
		for( i = 0; i < CSUITPLAYLIST; i++ )
		{
			if( ( isentence = m_rgSuitPlayList[isearch] ) )
				break;

			if( ++isearch == CSUITPLAYLIST )
				isearch = 0;
		}

		if( isentence )
		{
			m_rgSuitPlayList[isearch] = 0;
			if( isentence > 0 )
			{
				// play sentence number
				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy( sentence, "!" );
				strcat( sentence, gszallsentencenames[isentence] );
				EMIT_SOUND_SUIT( ENT( pev ), sentence );
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT( ENT( pev ), -isentence );
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check 
			m_flSuitUpdate = 0;
	}*/

	return;
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate( const char *name, int fgroup, int iNoRepeatTime )
{
	/*int i;
	int isentence;
	int iempty = -1;

	// Ignore suit updates if no suit
	if( !( pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return;

	if( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue
	if( !name )
	{
		for( i = 0; i < CSUITPLAYLIST; i++ )
			m_rgSuitPlayList[i] = 0;
		return;
	}

	// get sentence or group number
	if( !fgroup )
	{
		isentence = SENTENCEG_Lookup( name, NULL );
		if( isentence < 0 )
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex( name );

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.
	for( i = 0; i < CSUITNOREPEAT; i++ )
	{
		if( isentence == m_rgiSuitNoRepeat[i] )
		{
			// this sentence or group is already in 
			// the norepeat list
			if( m_rgflSuitNoRepeatTime[i] < gpGlobals->time )
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if( !m_rgiSuitNoRepeat[i] )
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given
	if( iNoRepeatTime )
	{
		if( iempty < 0 )
			iempty = RANDOM_LONG( 0, CSUITNOREPEAT - 1 ); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if( m_iSuitPlayNext == CSUITPLAYLIST )
		m_iSuitPlayNext = 0;

	if( m_flSuitUpdate <= gpGlobals->time )
	{
		if( m_flSuitUpdate == 0 )
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME; 
	}*/

	return;
}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
static void CheckPowerups( entvars_t *pev )
{
	if( pev->health <= 0 )
		return;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes
}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );

	if( !pSound )
	{
		ALERT( at_console, "Client lost reserved sound!\n" );
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.
	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		iBodyVolume = (int)pev->velocity.Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		if( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if( pev->button & IN_JUMP )
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	if( m_iWeaponVolume > iBodyVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= (int)( 250 * gpGlobals->frametime );
	if( m_iWeaponVolume < 0 )
	{
		m_iWeaponVolume = 0;
	}

	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if( iVolume > m_iTargetVolume )
	{
		iVolume -= (int)( 250 * gpGlobals->frametime );

		if( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if( m_fNoPlayerSound || m_skill_darkhide_on )
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if( gpGlobals->time > m_flStopExtraSoundTime )
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two 
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if( pSound )
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= ( bits_SOUND_PLAYER | m_iExtraSoundTypes );
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= (int)( 256 * gpGlobals->frametime );
	if( m_iWeaponFlash < 0 )
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the 
	// player is making. 
	// UTIL_ParticleEffect( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}

void CBasePlayer::PostThink()
{
	if( g_fGameOver )
		goto pt_end;	// intermission or finale

	if( g_restore_fix > 0)
	{
		g_restore_fix--;
		if(g_restore_fix == 99)
		{
			if(m_trainning != 1)
			{
				CLIENT_COMMAND(edict(), "-cammousemove\n");
			}
			if(FNullEnt(m_wdoor_mynpc) && m_guard_mynpc != 0)
			{
				m_flNPCguardTime = gpGlobals->time + 2.0;
				m_wdoor_mynpc = NULL;
				m_guard_mynpc = 0;
			}
			if(m_pActiveItem != NULL && pev->deadflag == DEAD_NO)
			{
				m_pActiveItem->Deploy();
			}
			pev->fov = 0;
			m_iFOV = 0;
			m_iClientFOV		= -1; // make sure fov reset is sent
			m_iClient_mynpc     = -1;

			if(m_player_camera != NULL)
			{
				SET_VIEW( edict(), m_player_camera->edict() );
			}
			if((pev->flags & FL_FROZEN))
			{
				CLIENT_COMMAND(edict(), "=cammousemove\n");
			}
		}
		if(g_restore_fix == 44 && game_player_dead == 1)
		{
			game_player_dead = 0;
			g_engfuncs.pfnSetPhysicsKeyValue( edict(), "dead_bugfix", "0" );//Xash 3D Debug Fix!
		}

		if(g_restore_fix == 25 && m_save_allow == 1)
		{
			if(m_load_check == 0)
			{
				//m_music_save = 0;
				//pev->health = 0;
				//Killed( pev, GIB_NEVER );
				//�����ˣ���ؽ����ؿ�!
				SERVER_COMMAND( "map wdoor_bonus_level\n" );
			}
		}

		if(g_restore_fix == 20)
		{
			if(m_music_save == 1)
			{
				CLIENT_COMMAND(edict(), "cd loop 4\n");
				//SERVER_COMMAND("mp3 loop media/music8.mp3\n");
			}
			else if(m_music_save == 2)
			{
				SERVER_COMMAND("mp3 loop media/music12.mp3\n");
			}
			else if(m_music_save == 3)
			{
				CLIENT_COMMAND(edict(), "cd loop 11\n");
				//SERVER_COMMAND("mp3 loop media/music14.mp3\n");
			}
			else if(m_music_save == 4)
			{
				CLIENT_COMMAND(edict(), "cd loop 20\n");
				//SERVER_COMMAND("mp3 loop media/boss4.mp3\n");
			}
			else if(m_music_save == 5)
			{
				CLIENT_COMMAND(edict(), "cd loop 10\n");
				//SERVER_COMMAND("mp3 loop media/boss1.mp3\n");
			}
			else if(m_music_save == 6)
			{
				CLIENT_COMMAND(edict(), "cd loop 17\n");
				//SERVER_COMMAND("mp3 loop media/boss2.mp3\n");
			}
			else if(m_music_save == 7)
			{
				CLIENT_COMMAND(edict(), "cd loop 12\n");
				//SERVER_COMMAND("mp3 loop media/boss3.mp3\n");
			}
			else if(m_music_save == 8)
			{
				CLIENT_COMMAND(edict(), "cd loop 14\n");
				//SERVER_COMMAND("mp3 loop media/music17.mp3\n");
			}
			else if(m_music_save == 9)
			{
				CLIENT_COMMAND(edict(), "cd loop 23\n");
				//SERVER_COMMAND("mp3 loop media/boss5.mp3\n");
			}
			else if(m_music_save == 10)
			{
				CLIENT_COMMAND(edict(), "cd loop 13\n");
				//SERVER_COMMAND("mp3 loop media/music13.mp3\n");
			}
			else if(m_music_save == 11)
			{
				SERVER_COMMAND("mp3 loop media/music19.mp3\n");
			}
			else if(m_music_save == 12)
			{
				CLIENT_COMMAND(edict(), "cd loop 3\n");
				//SERVER_COMMAND("mp3 loop media/music20.mp3\n");
			}
			else if(m_music_save == 13)
			{
				CLIENT_COMMAND(edict(), "cd loop 19\n");
				//SERVER_COMMAND("mp3 loop media/music21.mp3\n");
			}
			else if(m_music_save == 14){
				CLIENT_COMMAND(edict(), "cd loop 5\n");
				//SERVER_COMMAND("mp3 loop media/boss6.mp3\n");
			}
			else if(m_music_save == 15)
			{
				SERVER_COMMAND("mp3 loop media/music22.mp3\n");
			}
			else if(m_music_save == 16)
			{
				CLIENT_COMMAND(edict(), "cd loop 18\n");
				//SERVER_COMMAND("mp3 loop media/boss7.mp3\n");
			}
			else if(m_music_save == 17)
			{
				CLIENT_COMMAND(edict(), "cd loop 9\n");
				//SERVER_COMMAND("mp3 loop media/boss8.mp3\n");
			}
			else if(m_music_save == 18)
			{
				CLIENT_COMMAND(edict(), "cd loop 8\n");
				//SERVER_COMMAND("mp3 loop media/music23.mp3\n");
			}
			else if(m_music_save == 19)
			{
				CLIENT_COMMAND(edict(), "cd loop 24\n");
				//SERVER_COMMAND("mp3 loop media/music24.mp3\n");
			}
			else if(m_music_save == 20)
			{
				CLIENT_COMMAND(edict(), "cd loop 21\n");
				//SERVER_COMMAND("mp3 loop media/music25.mp3\n");
			}
			else if(m_music_save == 21)
			{
				CLIENT_COMMAND(edict(), "cd loop 6\n");
				//SERVER_COMMAND("mp3 loop media/boss9.mp3\n");
			}
			else if(m_music_save == 22)
			{
				CLIENT_COMMAND(edict(), "cd loop 22\n");
				//SERVER_COMMAND("mp3 loop media/boss10.mp3\n");
			}
		}
		if(g_restore_fix == 10)
		{
			if(g_causality_add > 0)
			{
				if(m_skill_reload)
				{
					MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
					if(g_causality_add == 1)
					{
						WRITE_STRING( "c_lv_1" );
					}
					else if(g_causality_add == 2)
					{
						WRITE_STRING( "c_lv_2" );
					}
					else if(g_causality_add == 3)
					{
						WRITE_STRING( "c_lv_3" );
					}
					else if(g_causality_add == 4)
					{
						WRITE_STRING( "c_lv_4" );
					}
					else if(g_causality_add == 5)
					{
						WRITE_STRING( "c_lv_5" );
					}
					MESSAGE_END();
				}
				else
				{
					g_causality_add = 0;
				}
			}
		}
	}

	if(CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999)
	{
		if (FBitSet( pev->flags, FL_NOTARGET ) && !m_fPlayerHideMode && !m_skill_darkhide_on && m_rpg_menu_actor1 == 1)
			pev->flags &= ~FL_NOTARGET;

		if (FBitSet( pev->flags, FL_GODMODE ))
			pev->flags &= ~FL_GODMODE;

		if (pev->movetype == MOVETYPE_NOCLIP && m_rpg_menu_actor1 == 1)
			pev->movetype = MOVETYPE_WALK;
	}

	if(CVAR_GET_FLOAT("sv_friction") != 6)
	{
		CVAR_SET_FLOAT("sv_friction", 6);
	}

	if(CVAR_GET_FLOAT("sv_stepsize") != 20)
	{
		CVAR_SET_FLOAT("sv_stepsize", 20);
	}

	if(CVAR_GET_FLOAT("sv_cheats") != 1)
	{
		CVAR_SET_FLOAT("sv_cheats", 1);
	}

	if(CVAR_GET_FLOAT("sv_maxspeed") != 400)
	{
		CVAR_SET_FLOAT("sv_maxspeed", 400);
	}

	if(CVAR_GET_FLOAT("sv_maxvelocity") != 9000)
	{
		CVAR_SET_FLOAT("sv_maxvelocity", 9000);
	}

	if(m_teleprort_in_xen == 0)
	{
		if(CVAR_GET_FLOAT("sv_gravity") != 800)
		{
			CVAR_SET_FLOAT("sv_gravity", 800);
		}
	}
	else
	{
		//Xen 
		if(CVAR_GET_FLOAT("sv_gravity") != 400)
		{
			CVAR_SET_FLOAT("sv_gravity", 400);
		}
	}

	if (!FBitSet( pev->flags, FL_NOTARGET ) && (m_fPlayerHideMode || m_skill_darkhide_on) )
		pev->flags |= FL_NOTARGET;

	if( IsAlive() && m_player_died == TRUE)
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "dead_bugfix", "0" );
		m_player_died = FALSE;
	}

	if( !IsAlive() )
		goto pt_end;

	// do weapon stuff
	ItemPostFrame( );

	if(m_godposion > 0)
	{
		if(m_god_time < gpGlobals->time)
		{
			m_godposion = 0;
			m_iClientHealth = -1;
		}
	}

	if(m_wrongdoor_time >= 1 && m_wrongdoor_time < gpGlobals->time)
	{
		if (m_pActiveItem->m_iId != WEAPON_FIST)
		{
			m_wrongdoor_time = 0;
		}
		else
		{
			m_wrongdoor_time = 0;
			m_wrongdoor_cover_time = gpGlobals->time + 60.0;
			m_pActiveItem->Deploy();

			m_newcross_active = 0;
			m_newcross_ontarget = 0;

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/gluongun_fire.wav", 1, ATTN_NORM);

			TraceResult tr;
			UTIL_MakeVectors(pev->v_angle);
			
			UTIL_TraceLine(GetGunPosition(), GetGunPosition() + gpGlobals->v_forward * 2048, dont_ignore_monsters, edict(), &tr);
			FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 15), 107 );
			FireBeam(GetGunPosition(), tr.vecEndPos + (tr.vecPlaneNormal * 15), 24, 623, pev);

			::RadiusDamage_limit( tr.vecEndPos + (tr.vecPlaneNormal * 15), pev, pev, 2000, 500, CLASS_PLAYER, DMG_MORTAR | DMG_CONCUSSION);
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			if(pEntity)
			{
				if(pEntity->pev->deadflag == DEAD_NO && pEntity->pev->takedamage)
				{
					if ( pEntity->pev->flags & FL_MONSTER )
					{
						CBaseMonster *pEnemyMonster;
						pEnemyMonster = pEntity->MyMonsterPointer();
						if(pEnemyMonster)
						{
							pEnemyMonster->Freeze_Monster(40);
							if(pEntity->pev->gravity <= 1.5)
							{
								pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 2500 + pev->velocity;
								pEntity->pev->velocity.z = 0;
							}
						}
					}
				}
			}

			m_air_oxyan = 1;
			m_air_oxyan_stop_time = gpGlobals->time + 5.0;
			pev->velocity = gpGlobals->v_forward * -1500 + pev->velocity;
			pev->velocity.z *= 0.6;
			pev->punchangle.x += -8;
			FX_Explosion( Center(), EXPLOSION_SPARKSHOWER );
		}
	}

	if(m_needlekilled_time >= 1 && m_needlekilled_time < gpGlobals->time)
	{
		if (m_pActiveItem->m_iId != WEAPON_FIREAXE)
		{
			m_needlekilled_time = 0;
		}
		else
		{
			//DropPlayerItem("weapon_valvesword");
			m_pActiveItem->Deploy();
			m_needlekilled_time = 0;
			m_air_oxyan = 1;
			m_air_oxyan_stop_time = gpGlobals->time + 5.0;
			m_swordrecover_time = gpGlobals->time + 45.0;
			FX_Explosion( Center(), EXPLOSION_HEVCHARGER);

			m_fPlayerUseHolySword = TRUE;

			CBaseEntity *pEnt = CBaseEntity::Create( "holy_valve_sword", pev->origin, g_vecZero, edict() );
			CBaseEntity *pTarget = CBaseEntity::Create( "sword_aim_target", pev->origin, g_vecZero, edict() );
		}
	}
	else if(m_swordrecover_time >= 1 && m_swordrecover_time < gpGlobals->time)
	{
		//GiveNamedItem( "weapon_valvesword" );
		m_swordrecover_time = 0;
	}

	if(m_needleheal >= 1)
	{
		if(m_needleuse_time < gpGlobals->time)
		{
			m_needleuse_time = 0;
			m_needleheal = 0;
			m_iClientHealth = -1;
		}
		if(m_air_oxyan < m_air_oxyan_max)
		{
			m_air_oxyan += 10;
		}
	}

	if(m_needleheal2 >= 1)
	{
		TakeHealth(1, DMG_GENERIC);
		m_needleheal2 -= 1;
	}
	else if(m_needleheal2 < 0)
	{
		if(pev->health > 1)
		{
			pev->health -= 1;
		}
		m_needleheal2 += 1;
	}

	if(m_fPlayerUseHolySword == TRUE || m_guard_mynpc == 1)
	{
		if(pev->viewmodel != 0)
		{
			pev->viewmodel = 0;
			if (m_pActiveItem)
			{
				m_pActiveItem->Holster();
			}
		} 
	}

	if( !m_level_up_switch && m_kadoma_level < int(m_kadoma_exp * 0.001) && m_kadoma_level < 99)
	{
		m_kadoma_level += 1;

		char text[256];
		UTIL_CenterPrintAll( "Level Up!" );

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
		WRITE_STRING( "lv_up" );
		MESSAGE_END();

		m_fNextClearTextTime = gpGlobals->time + 6.0;

		sprintf( text, "- Kadoma Level:%d\n", m_kadoma_level);
		UTIL_SayTextAll( text,this );

		if(m_kadoma_level == 99)
		{
			pev->max_health = 500;
			if(m_fequip1 == TRUE)
			{
				pev->max_health = 600;
			}

			if(m_skill_reload && m_skill_maxarmor != 300)
			{
				pev->max_health = 900;
				m_skill_maxarmor = 300;
				pev->armorvalue = m_skill_maxarmor;
			}

			sprintf( text, "- LEVEL MAX! MAX HP +200!!\n");
			UTIL_SayTextAll( text,this );

			TakeHealth(pev->max_health, DMG_GENERIC);
		}
		else
		{
			if(pev->max_health >= 300)
			{
				pev->max_health = 300;
			}
			else
			{
			
				pev->max_health = 100 + m_kadoma_level * 10;
			
				sprintf( text, "- MAX HP +10\n");
				UTIL_SayTextAll( text,this );
			}
			if(m_fequip1 == TRUE)
			{
				pev->max_health = 400;
			}
			TakeHealth(10, DMG_GENERIC);
		}

		if(m_kadoma_level == 16 && !m_skill_longjump)
		{
			m_skill_locked = 41;
			
			sprintf( text, "- New Skill: Cloud Push\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Fist special attack to push enemy dizzy!\n");
			UTIL_SayTextAll( text,this );
			
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 4.0;
		}
		else if(m_kadoma_level == 32 && !m_skill_longjump)
		{
			m_skill_longjump = 3;
			g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
	
			sprintf( text, "- New Skill: Long jump\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Forward move and duck then jump or press c.\n");
			UTIL_SayTextAll( text,this );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 4.0;
		}
		else if(m_kadoma_level == 40 && !m_skill_miss)
		{
			m_skill_miss = 81;
			
			sprintf( text, "- New Skill: Slide shovel\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Long press duck key + TAB to use slide shove acceleratel\n");
			UTIL_SayTextAll( text,this );
			
			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 4.0;
		}
		else if(m_kadoma_level == 48 && !m_skill_goddam)
		{
			m_skill_goddam = 40;
			
			sprintf( text, "- New Skill: Last bit\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- When health > 20%, death attack left 1 last health point\n");
			UTIL_SayTextAll( text,this );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 4.0;
		}
		else if(m_kadoma_level == 56 && !m_skill_respawn)
		{
			m_skill_respawn = 6;
	
			sprintf( text, "- New Skill: Respawn\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- When dead 6 second auto respawn\n");
			UTIL_SayTextAll( text,this );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 4.0;
		}
		else if(m_kadoma_level == 64 && !m_skill_deathmatch)
		{
			m_skill_deathmatch = 8;
		
			sprintf( text, "- New Skill: Armor repair\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Equip Egon's Book, Special attack can add armor point\n");
			UTIL_SayTextAll( text,this );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 4.0;
		}
		else if(m_kadoma_level == 80 && !m_skill_wrongdoor)
		{
			m_skill_wrongdoor = 9;
		
			sprintf( text, "- New Skill: Heavens Blow\n");
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Use fist press R, Aim to blast shock\n");
			UTIL_SayTextAll( text,this );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pev );
			WRITE_STRING( "n_skill" );//New Skill!
			MESSAGE_END();

			m_fNextClearTextTime += 6.0;
		}
	}

	// Handle Tank controlling
	if( m_pTank != 0 )
	{
		// if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if( m_pTank->OnControls( pev ) && !pev->weaponmodel )
		{  
			m_pTank->Use( this, this, USE_SET, 2 );	// try fire the gun
		}
		else
		{
			// they've moved off the platform
			m_pTank->Use( this, this, USE_OFF, 0 );
		}
	}

	if( (pev->flags & FL_FROZEN) || m_rpg_menu_actor1 != 1 || m_concussion_time > gpGlobals->time || m_barnacle_RTP != 0)
	{
		pev->viewmodel = 0;
	}
	else if(pev->deadflag == DEAD_NO)
	{
		if(pev->viewmodel == 0 && m_barnacle_RTP == 0 && m_fMoveItem == NULL && m_fPlayerUseHolySword == FALSE && m_guard_mynpc == 0 && m_concussion_time == 0)
		{
			if(m_pActiveItem != NULL)
			{
				m_pActiveItem->Deploy();
			}
		}
	}

	if(pev->waterlevel == 1)
	{
		if(pev->velocity.Length() >= 200 && m_waterstepTime < gpGlobals->time)
		{
			m_waterstepTime = gpGlobals->time + 0.32;
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin - Vector(0,0,36), ignore_monsters, ENT(pev), &tr);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, pev->origin, 0, 114, 0 );
		}
	}

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a 
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ( (FBitSet(pev->flags, FL_ONGROUND)) )
	{	
		//Mario Jump
		entvars_t *pevGround = VARS(pev->groundentity);
		if ( pevGround && pevGround->takedamage != DAMAGE_NO )
		{

			if(m_fequip2 == TRUE)
			{
				m_flFallVelocity += 100;
			}

			if( !(pevGround->flags & FL_MONSTER) )
			{
				m_flFallVelocity -= 100;
			}

			if(m_flFallVelocity >= 250)
			{
						
				TraceResult tr;
				Vector vecSrc	= pev->origin + Vector(0,0,32);
				Vector vecEnd = vecSrc - Vector(0,0,96);
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				pev->velocity = (pev->origin - pEntity->pev->origin).Normalize() * m_flFallVelocity * 0.5;

				if(m_flFallVelocity < 500)
				{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "newadd/fist_hitbod1.wav", 1, ATTN_NORM); 
					UTIL_Sparks(tr.vecEndPos);
				}
				else
				{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "newadd/fist_hitbod3.wav", 1, ATTN_NORM); 
					MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
					WRITE_BYTE(TE_STREAK_SPLASH);
					WRITE_COORD(tr.vecEndPos.x);
					WRITE_COORD(tr.vecEndPos.y);
					WRITE_COORD(tr.vecEndPos.z);
					WRITE_COORD(tr.vecPlaneNormal.x);
					WRITE_COORD(tr.vecPlaneNormal.y);
					WRITE_COORD(tr.vecPlaneNormal.z);
					WRITE_BYTE(10);
					WRITE_SHORT(30);
					WRITE_SHORT(50);
					WRITE_SHORT(300);
					MESSAGE_END();
					UTIL_Ricochet( tr.vecEndPos, 1 );
				}
						
				CBasePlayer *pfuck;
				pfuck = (CBasePlayer *)GET_PRIVATE(pev->groundentity);
				if(pfuck->pev->takedamage)
				{
					ClearMultiDamage( );

					float falldmg = m_flFallVelocity - 250;
					float hitdmg = falldmg * 0.15;

					if(m_fequip2 == TRUE)
					{
						hitdmg = falldmg * 0.25;
					}

					if(hitdmg < 10)
					{
						hitdmg = 10;
					}

					if(falldmg < 300)
					{
						pfuck->TraceAttack(pev, hitdmg, gpGlobals->v_forward, &tr, DMG_FALL | DMG_NEVERGIB ); 
					}
					else
					{
						hitdmg *= 1.5;
						pfuck->TraceAttack(pev, hitdmg, gpGlobals->v_forward, &tr, DMG_FALL ); 
					}

					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ApplyMultiDamage( pev, pev );
				}

				if((pevGround->flags & FL_MONSTER))
				{
					m_flFallVelocity -= 200;
					if(m_fequip2 == TRUE)
					{
						m_flFallVelocity -= 200;
					}
				}
			}
		}

		if(m_fequip2 == TRUE)
		{
			m_flFallVelocity -= 200;
		}
	}

	if( ( FBitSet( pev->flags, FL_ONGROUND ) ) && ( pev->health > 0 ) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
	{
		// ALERT( at_console, "%f\n", m_flFallVelocity );
		if( pev->watertype == CONTENT_WATER )
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when 
			// BUG - water has current force
			// if( !pev->groundentity || VARS(pev->groundentity )->velocity.z == 0 )
				// EMIT_SOUND( ENT( pev ), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM );
		}
		else if( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
		{
			// after this point, we start doing damage
			float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

			if( flFallDamage > pev->health && pev->deadflag == DEAD_NO )
			{
				//splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND( ENT( pev ), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM );
			}

			if( flFallDamage > 0 )
			{
				m_flVelocityModifier -= flFallDamage * 0.02;
				if(m_flVelocityModifier < -4)
					m_flVelocityModifier = -4;
				TakeDamage( VARS( eoNullEntity ), VARS( eoNullEntity ), flFallDamage, DMG_FALL ); 
				pev->punchangle.x = 0;
			}
		}

		if( IsAlive() )
		{
			SetAnimation( PLAYER_WALK );
		}
	}

	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		if( m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer() )
		{
			CSoundEnt::InsertSound( bits_SOUND_PLAYER, pev->origin, (int)m_flFallVelocity, 0.2 );
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character	
	/*if( IsAlive() )
	{*/
		if( !pev->velocity.x && !pev->velocity.y )
			SetAnimation( PLAYER_IDLE );
		else if( ( pev->velocity.x || pev->velocity.y ) && ( FBitSet( pev->flags, FL_ONGROUND ) ) )
			SetAnimation( PLAYER_WALK );
		else if( pev->waterlevel > 1 )
			SetAnimation( PLAYER_WALK );
	//}

	StudioFrameAdvance();
	CheckPowerups( pev );

	UpdatePlayerSound();
pt_end:
	if( pev->deadflag == DEAD_NO )
		m_vecLastViewAngles = pev->angles;
	else
		pev->angles = m_vecLastViewAngles;

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

#if CLIENT_WEAPONS
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if( m_rgpPlayerItems[i] )
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

			while( pPlayerItem )
			{
				CBasePlayerWeapon *gun;

				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();

				if( gun && gun->UseDecrement() )
				{
					gun->m_flNextPrimaryAttack = Q_max( gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0f );
					gun->m_flNextSecondaryAttack = Q_max( gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001f );

					if( gun->m_flTimeWeaponIdle != 1000.0f )
					{
						gun->m_flTimeWeaponIdle = Q_max( gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001f );
					}

					if( gun->pev->fuser1 != 1000.0f )
					{
						gun->pev->fuser1 = Q_max( gun->pev->fuser1 - gpGlobals->frametime, -0.001f );
					}

					// Only decrement if not flagged as NO_DECREMENT
					/*if( gun->m_flPumpTime != 1000.0f )
					{
						gun->m_flPumpTime = Q_max( gun->m_flPumpTime - gpGlobals->frametime, -0.001f );
					}*/
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if( m_flNextAttack < -0.001f )
		m_flNextAttack = -0.001f;
	
	if( m_flNextAmmoBurn != 1000.0f )
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if( m_flNextAmmoBurn < -0.001f )
			m_flNextAmmoBurn = -0.001f;
	}

	if( m_flAmmoStartCharge != 1000.0f )
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if( m_flAmmoStartCharge < -0.001f )
			m_flAmmoStartCharge = -0.001f;
	}
#else
	return;
#endif
}

// checks if the spot is clear of players
BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	if( !pSpot->IsTriggered( pPlayer ) )
	{
		return FALSE;
	}

	while( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 ) ) != NULL )
	{
		// if ent is a client, don't spawn on 'em
		if( ent->IsPlayer() && ent != pPlayer )
			return FALSE;
	}

	return TRUE;
}

DLL_GLOBAL CBaseEntity	*g_pLastSpawn;
inline int FNullEnt( CBaseEntity *ent ) { return ( ent == NULL ) || FNullEnt( ent->edict() ); }

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer )
{
	CBaseEntity *pSpot;
	edict_t *player;

	int nNumRandomSpawnsToTry = 10;

	player = pPlayer->edict();

	// choose a info_player_deathmatch point
	/*if( g_pGameRules->IsCoOp() )
	{
		pSpot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_coop" );
		if( !FNullEnt( pSpot ) )
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_start" );
		if( !FNullEnt(pSpot) ) 
			goto ReturnSpot;
	}
	else if( g_pGameRules->IsDeathmatch() )
	{
		if( !g_pLastSpawn )
		{
			nNumRandomSpawnsToTry = 0;
			CBaseEntity* pEnt = 0;

			while( ( pEnt = UTIL_FindEntityByClassname( pEnt, "info_player_deathmatch" )))
				nNumRandomSpawnsToTry++;
		}

		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for( int i = RANDOM_LONG( 1, nNumRandomSpawnsToTry - 1 ); i > 0; i-- )
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if( FNullEnt( pSpot ) )  // skip over the null point
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if( pSpot )
			{
				// check if pSpot is valid
				if( IsSpawnPointValid( pPlayer, pSpot ) )
				{
					if( pSpot->pev->origin == Vector( 0, 0, 0 ) )
					{
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if( !FNullEnt( pSpot ) )
		{
			CBaseEntity *ent = NULL;
			while( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 ) ) != NULL )
			{
				// if ent is a client, kill em (unless they are ourselves)
				if( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( VARS( INDEXENT( 0 ) ), VARS( INDEXENT( 0 ) ), 300, DMG_GENERIC );
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if( FStringNull( gpGlobals->startspot ) || (STRING( gpGlobals->startspot ) )[0] == '\0')
	{
		pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
		if( !FNullEnt( pSpot ) )
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname( NULL, STRING( gpGlobals->startspot ) );
		if( !FNullEnt( pSpot ) )
			goto ReturnSpot;
	}*/

	pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
	if ( !FNullEnt( pSpot ) )
		goto ReturnSpot;

ReturnSpot:
	if( FNullEnt( pSpot ) )
	{
		ALERT( at_error, "PutClientInServer: no info_player_start on level\n" );
		return INDEXENT( 0 );
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}

void CBasePlayer::Spawn( void )
{
	m_flStartCharge = gpGlobals->time;
	pev->classname = MAKE_STRING( "player" );
	pev->health = 100;
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags &= FL_PROXY;	// keep proxy flag sey by engine
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;				// initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0f;
	pev->gravity = 1.0f;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = FALSE;// no longjump module. 
	m_skill_maxarmor = 0;

	m_fMask	= FALSE;

	m_teleprort_in_xen  = 0;

	m_fPlayerUseHolySword= FALSE;

	m_player_died = FALSE;
	m_skill_darkhide_on = FALSE;
	m_level_up_switch = FALSE;

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "dead_bugfix", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "mario", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "fr", "1" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "bj", bhopcap.value ? "0" : "1" );

	pev->fov = m_iFOV = 0;// init field of view.
	m_iClientFOV = -1; // make sure fov reset is sent

	m_flNextDecalTime = 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0f;	// wait a few seconds until user-defined message registrations
							// are recieved by all clients

	m_newcross_active = 0;
	m_godposion = 0;

	m_iNVG = 0;

	m_god_time = gpGlobals->time + 3.0;
	m_flNextSoundTime1 = gpGlobals->time;

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5f;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	m_air_oxyan_max = 2000;
	m_air_oxyan = m_air_oxyan_max;

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam( this );
	g_pGameRules->GetPlayerSpawnSpot( this );

	SET_MODEL( ENT( pev ), "models/player.mdl" );
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity( ACT_IDLE );

	if( FBitSet( pev->flags, FL_DUCKING ) ) 
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector( 0, 32, 0 );

	if( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	m_blindUntilTime = 0;
	m_blindStartTime = 0;
	m_blindHoldTime = 0;
	m_blindFadeTime = 0;
	m_blindAlpha = 0;

	m_rpg_menu_actor1 = 1;
	m_rpg_menu_on = 0;
	m_rpg_menu_item_e = -1;
	m_rpg_menu_item_t = -1;

	m_team_npc1 = NULL;
	m_team_npc2 = NULL;
	m_team_npc3 = NULL;
	m_team_npc4 = NULL;
	m_team_npc5 = NULL;
	m_team_npc6 = NULL;
	m_team_npc7 = NULL;
	m_team_npc8 = NULL;
	m_team_npc9 = NULL;
	m_team_npc10 = NULL;
	m_team_npc11 = NULL;
	m_team_npc12 = NULL;

	m_flash_mode = 1;
	m_ending_frags = 50;
	m_game_rate = 0;
	m_skill_punch = 4;
	m_save_allow = 0;

	SET_VIEW( edict(), edict() );
	m_player_camera = NULL;

	// reset all ammo values to 0
	for( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	m_flNextChatTime = gpGlobals->time;
	m_new_spawner = 100;

	m_flVelocityModifier2 = 1;
	m_fNextClearTextTime = -1;

	m_concussion_time = 0;

	SET_VIEW(edict(), edict());

	g_pGameRules->PlayerSpawn( this );
}

void CBasePlayer::Precache( void )
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.

	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if( !WorldGraph.FSetGraphPointers() )
		{
			ALERT( at_console, "**Graph pointers were not set!\n" );
		}
		else
		{
			ALERT( at_console, "**Graph Pointers Set!\n" );
		}
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_flFlashLightTime = 1;

	m_iTrain |= TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if( gInitHUD )
		m_fInitHUD = TRUE;

	pev->fov = m_iFOV;	// Vit_amiN: restore the FOV on level change or map/saved game load
}

int CBasePlayer::Save( CSave &save )
{
	if( !CBaseMonster::Save( save ) )
		return 0;

	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );
}

//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems( void )
{

}

int CBasePlayer::Restore( CRestore &restore )
{
	if( !CBaseMonster::Restore( restore ) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );

	g_restore_fix = 100;
	m_rpg_menu_on = 0;
	g_fGameSkipCG = 0;
	m_flFlashLightTime = gpGlobals->time + 0.2f;

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if( !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t *pentSpawnSpot = EntSelectSpawnPoint( this );
		pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
		pev->angles = VARS( pentSpawnSpot )->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if( FBitSet( pev->flags, FL_DUCKING ) )
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	}
	else
	{
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	}

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	if( m_skill_longjump )
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	}

	if(m_fequip2 == TRUE)
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "mario", "1" );
	}
	else{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "mario", "0" );
	}

	RenewItems();

#if CLIENT_WEAPONS
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
#endif
	if( m_flFlashLightTime == 0.0f )
		m_flFlashLightTime = 1.0f;

	m_nCustomSprayFrames = -1;

	return status;
}

void CBasePlayer::SelectNextItem( int iItem )
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[iItem];

	if( !pItem )
		return;

	if( pItem == m_pActiveItem )
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext; 
		if( !pItem )
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while( pLast->m_pNext )
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[iItem] = pItem;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if( m_pActiveItem )
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pItem;

	if( m_pActiveItem )
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectItem( const char *pstr )
{
	if( !pstr )
		return;

	CBasePlayerItem *pItem = NULL;

	for( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if( m_rgpPlayerItems[i] )
		{
			pItem = m_rgpPlayerItems[i];

			while( pItem )
			{
				if( FClassnameIs( pItem->pev, pstr ) )
					break;
				pItem = pItem->m_pNext;
			}
		}

		if( pItem )
			break;
	}

	if( !pItem )
		return;

	if( pItem == m_pActiveItem )
		return;

	if( !pItem->CanDeploy())
		return;

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if( m_pActiveItem )
	{
		m_pActiveItem->pev->oldbuttons = 1;
		m_pActiveItem->Deploy();
		m_pActiveItem->pev->oldbuttons = 0;
		m_pActiveItem->UpdateItemInfo();
	}
}

void CBasePlayer::SelectLastItem( void )
{
	if( !m_pLastItem )
	{
		return;
	}

	if( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if( !m_pLastItem->CanDeploy())
		return;

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if( m_pActiveItem )
		m_pActiveItem->Holster();

	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;

	m_pActiveItem->pev->oldbuttons = 1;
	m_pActiveItem->Deploy();
	m_pActiveItem->pev->oldbuttons = 0;

	m_pActiveItem->UpdateItemInfo();
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons( void )
{
	int i;

	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if( m_rgpPlayerItems[i] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem( int iItem )
{
}

const char *CBasePlayer::TeamID( void )
{
	if( pev == NULL )		// Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}

//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn( entvars_t *pevOwner );
	void Think( void );

	virtual int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector( 0, 0, 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT( pevOwner );
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1f;
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM );
}

void CSprayCan::Think( void )
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;

	pPlayer = (CBasePlayer *)GET_PRIVATE( pev->owner );

	if( pPlayer )
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX( pev->owner );

	// ALERT( at_console, "Spray by player %i, %i of %i\n", playernum, (int)( pev->frame + 1 ), nFrames );

	UTIL_MakeVectors( pev->angles );
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr );

	// No customization present.
	if( nFrames == -1 )
	{
		UTIL_DecalTrace( &tr, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &tr, playernum, (int)pev->frame, TRUE );
		// Just painted last custom frame.
		if( pev->frame++ >= ( nFrames - 1 ) )
			UTIL_Remove( this );
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn( entvars_t *pevOwner );
	void Spray( void );
};

void CBloodSplat::Spawn( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector( 0, 0, 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT( pevOwner );

	SetThink( &CBloodSplat::Spray );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CBloodSplat::Spray( void )
{
	TraceResult tr;	

	UTIL_MakeVectors( pev->angles );
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr );

	UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

//==============================================
void CBasePlayer::GiveNamedItem( const char *pszName )
{
	edict_t	*pent;

	int istr = MAKE_STRING( pszName );

	pent = CREATE_NAMED_ENTITY( istr );
	if( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in GiveNamedItem!\n" );
		return;
	}
	VARS( pent )->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn( pent );
	DispatchTouch( pent, ENT( pev ) );
}

CBaseEntity *FindEntityForward( CBaseEntity *pMe )
{
	TraceResult tr;

	UTIL_MakeVectors( pMe->pev->v_angle );
	UTIL_TraceLine( pMe->pev->origin + pMe->pev->view_ofs,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192,dont_ignore_monsters, pMe->edict(), &tr );
	if( tr.flFraction != 1.0f && !FNullEnt( tr.pHit ) )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}


BOOL CBasePlayer :: NightViewIsOn( void )
{
	return FBitSet(pev->effects, EF_BRIGHTLIGHT);
}

void CBasePlayer :: NightViewTurnOn( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, SOUND_NIGHTVIEW_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
	SetBits(pev->effects, EF_BRIGHTLIGHT);
}

void CBasePlayer :: NightViewTurnOff( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, SOUND_NIGHTVIEW_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
	ClearBits(pev->effects, EF_BRIGHTLIGHT);
}

BOOL CBasePlayer::FlashlightIsOn( void )
{
	return FBitSet( pev->effects, EF_INVLIGHT );
}

void CBasePlayer::FlashlightTurnOn( void )
{
	if( m_iFlashBattery <= 20 || !m_hasflashlight )
	{
		return;
	}

	if( (pev->weapons & ( 1 << WEAPON_SUIT ) ) )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits( pev->effects, EF_INVLIGHT );//EF_DIMLIGHT
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
	}
}

void CBasePlayer::FlashlightTurnOff( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
	ClearBits( pev->effects, EF_INVLIGHT );//EF_DIMLIGHT
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( m_iFlashBattery );
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate( void )
{
	m_iClientHealth = -1;
	m_iClient_mynpc  = -1;
	m_iClientBattery = -1;
	m_iClientHideHUD = -1;	// Vit_amiN: forcing to update
	m_iClientFOV = -1;	// Vit_amiN: force client weapons to be sent
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;          // Force weapon send
	m_fKnownItem = FALSE;    // Force weaponinit messages.
	m_fInitHUD = TRUE;		// Force HUD gmsgResetHUD message
	memset( m_rgAmmoLast, 0, sizeof( m_rgAmmoLast )); // a1ba: Force update AmmoX



	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
extern cvar_t *g_enable_cheats;

void CBasePlayer::ImpulseCommands()
{
	TraceResult tr;// UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch( iImpulse )
	{
	/*case 99:
		int iOn;

		if( !gmsgLogo )
		{
			iOn = 1;
			gmsgLogo = REG_USER_MSG( "Logo", 1 );
		} 
		else 
		{
			iOn = 0;
		}

		ASSERT( gmsgLogo > 0 );

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
			WRITE_BYTE( iOn );
		MESSAGE_END();

		if(!iOn)
			gmsgLogo = 0;
		break;*/
	case 100:
        // temporary flashlight for level designers
		if(m_darkposion == 0)
		{
			if ( FlashlightIsOn() )
			{
				FlashlightTurnOff();
			}
			else
			{
				FlashlightTurnOn();
			}
		}
		break;
	case 114:
		{
			if(m_skill_darkhide_on == FALSE && m_needleheal == 0 && m_concussion_time == 0
			&& m_rpg_menu_actor1 == 1 && m_fequip5 && m_godposion == 0 && m_guard_mynpc == 0
			&& pev->waterlevel != 3
			&& m_needlekilled_time == 0 && m_wrongdoor_time == 0 && m_fPlayerUseHolySword == FALSE)
			{
				if(m_fldarkhideTime > gpGlobals->time)
				{
					char text[256];
					sprintf( text, "CD:%1.0fs\n",m_fldarkhideTime - gpGlobals->time);
					UTIL_CenterPrintAll( text );
				}
				else
				{
					if ( FlashlightIsOn() )
					{
						FlashlightTurnOff();
					}

					UTIL_ScreenFade( this, Vector(0,0,0), 0.3, 0.1, 50, FFADE_IN );

					EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/hide1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
					m_iWeaponFlash = 0;
					m_skill_darkhide_on = TRUE;
					m_darkposion = 2500;
					m_iClientHealth	= -1;
					m_iClient_oxyan = -1;
					pev->fuser4 = 1;
					m_RecoverTime = gpGlobals->time;
					m_air_oxyan_stop_time = gpGlobals->time;
					m_fldarkhideTime = gpGlobals->time + 35;
				}
			}
		}
		break;
	case 201:
		// paint decal
		if( gpGlobals->time < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction != 1.0f )
		{
			// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan *pCan = GetClassPtr( (CSprayCan *)NULL );
			pCan->Spawn( pev );
		}
		break;
	case 116:
		{
			if ((pev->flags & FL_FROZEN) || m_flNPCguardTime > gpGlobals->time || m_fPlayerUseHolySword
			|| m_barnacle_RTP || m_fMoveItem != NULL || !m_skill_defguard
			|| !FBitSet( pev->flags, FL_ONGROUND ) || m_rpg_menu_actor1 != 1)
				break;

			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, dont_ignore_monsters, edict(), &tr);

			if(m_guard_mynpc == 0 && tr.flFraction != 1 && m_air_oxyan > 400)
			{
				m_flNPCguardTime = gpGlobals->time + 1.0;
				// What the hell are you doing?
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if ( pEntity )
				{
					CBaseMonster *pMonster = pEntity->MyMonsterPointer();
					if ( pMonster )
					{
						if(pMonster->IsAlive())
						{
							if ( pMonster->m_MonsterState != MONSTERSTATE_SCRIPT 
							&& pMonster->m_IdealMonsterState != MONSTERSTATE_SCRIPT 
							&& pMonster->m_MonsterState != MONSTERSTATE_PRONE
							&& pMonster->m_IdealMonsterState != MONSTERSTATE_PRONE
							&& pMonster->m_lovehate > 0 && pMonster->m_hPlayer != NULL
							&& pMonster->m_rpgms_inteam >= 1 && pMonster->m_rpgms_inteam <= 4)
							{
								m_wdoor_mynpc = pMonster->edict();
								m_guard_mynpc = 1;
								m_air_oxyan -= 400;
								m_air_oxyan_stop_time = gpGlobals->time + 2.0;
								m_flVelocityModifier -= 1;
								FX_Explosion( pMonster->Center(), 102);

								pMonster->m_playerguardian_mode = 1;
								pMonster->pev->flags |= FL_NOTARGET;
								
								pMonster->m_hEnemy = NULL;
								pMonster->m_hOldEnemy[0] = NULL;
								pMonster->m_hOldEnemy[1] = NULL;
								pMonster->m_hOldEnemy[2] = NULL;
								pMonster->m_hOldEnemy[3] = NULL;
								pMonster->ClearSchedule();
								pMonster->SetYawSpeed();
								pMonster->RouteClear();
								pMonster->SetActivity( ACT_IDLE );
								pMonster->pev->angles.y = pev->angles.y;
								pMonster->pev->effects |= EF_NODRAW;
								pMonster->pev->velocity = g_vecZero;
								pMonster->pev->movetype = MOVETYPE_NONE;
								pMonster->pev->owner = ENT(pev);
								pMonster->m_no_cover_mode = 1;

								if (m_pActiveItem)
								{
									m_pActiveItem->Holster();
								}

								if(m_darkposion > 0)
								{
									m_darkposion = 0;
								}

								UTIL_MakeVectors( pev->angles );
								UTIL_SetOrigin( pMonster->pev, pev->origin + Vector(0,0,48) + gpGlobals->v_forward * -16 );
							}
						}
					}
				}
			}
			else if(m_guard_mynpc != 0)
			{
				if(!FNullEnt(m_wdoor_mynpc))
				{
					m_flNPCguardTime = gpGlobals->time + 1.0;

					if (m_pActiveItem)
					{
						m_pActiveItem->Deploy();
					}

					entvars_t *Myslave;
					Myslave = VARS( m_wdoor_mynpc );
					CBaseEntity *pNPC = GetClassPtr((CBaseEntity *)Myslave);
					CBaseMonster *pMonster = pNPC->MyMonsterPointer();
					pMonster->m_playerguardian_mode = 20;
					//pMonster->pev->effects |= EF_NODRAW;
					pMonster->pev->angles.y = pev->angles.y;
					UTIL_MakeVectors( pev->v_angle );
					UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 96, dont_ignore_monsters, edict(), &tr);
					UTIL_SetOrigin( pMonster->pev, tr.vecEndPos + (tr.vecPlaneNormal * 16) );
				}
				else
				{
					m_flNPCguardTime = gpGlobals->time + 1.0;
					if (m_pActiveItem)
					{
						m_pActiveItem->Deploy();
					}
					m_wdoor_mynpc = NULL;
					m_guard_mynpc = 0;
				}
			}
			break;
		}
	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !HLDEMO_BUILD
	if(CVAR_GET_FLOAT( "cshl623_debug_mode" ) != 1999)
		return;

	CBaseEntity *pEntity;
	TraceResult tr;

	switch( iImpulse )
	{
	case 65:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_barney", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			break;
		}
	case 11:
		{
			if(m_save_allow == 0)
			{
				UTIL_CenterPrintAll( "Save Check On" );
				m_save_allow = 1;
				m_load_check = 1;
			}
			else
			{
				UTIL_CenterPrintAll( "Save Check Off" );
				m_save_allow = 0;
				m_load_check = 0;
			}
		}
		break;
	case 99:
		{
			if (m_rpg_menu_actor1 == 1)
			{
				m_mode_origin = g_vecZero;
				m_godposion = 0;
				m_rpg_menu_actor1 = 0;
				m_iClientHealth  = -1;
				m_iClient_mynpc  = -1;
				m_iClientBattery = -1;
				pev->movetype = MOVETYPE_NOCLIP;
				m_iHideHUD |= HIDEHUD_WEAPONS;
				m_hasflashlight = FALSE;
				pev->flags |= FL_NOTARGET;
				pev->solid = SOLID_NOT;
			} 
			else 
			{
				m_godposion = 0;
				m_rpg_menu_actor1 = 1;
				m_iClientHealth  = -1;
				m_iClient_mynpc  = -1;
				m_iClientBattery = -1;
				m_iHideHUD &= ~HIDEHUD_WEAPONS;
				m_hasflashlight = TRUE;
				pev->flags &= ~FL_NOTARGET;
				pev->solid	= SOLID_SLIDEBOX;
				pev->movetype = MOVETYPE_WALK;
			}	
		}
		break;
	case 66:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_lelite", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 67:
		{
			CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "monster_saintna");
			if ( pSpot2 )
			{
				UTIL_Remove(pSpot2);
				UTIL_MakeVectors( pev->v_angle );
				Create("monster_saintna", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			}
			else
			{
				UTIL_MakeVectors( pev->v_angle );
				Create("monster_saintna", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			}
		}
		break;
	case 68:
		{
			UTIL_MakeVectors( pev->v_angle );
			CBaseEntity *pEntity2 = Create("monster_scientist", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			pEntity2->pev->weapons = -1;
		}
		break;
	case 69:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_human_grunt_ally", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 22:
		{
			CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "monster_dengor");
			if ( pSpot2 )
			{
				UTIL_Remove(pSpot2);
				UTIL_MakeVectors( pev->v_angle );
				Create("monster_dengor", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			}
			else
			{
				UTIL_MakeVectors( pev->v_angle );
				Create("monster_dengor", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			}
		}
		break;
	case 75:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_human_fassn", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 77:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_nurse", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 76:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 74:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_gargantua", pev->origin + gpGlobals->v_forward * 256, Vector(0,pev->angles.y,0) );
		}
		break;
	case 78:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_zombie", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 79:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_otis", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 73:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_houndeye", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 72:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_alien_grunt", pev->origin + gpGlobals->v_forward * 200, Vector(0,pev->angles.y,0) );
		}
		break;
	case 71:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_alien_slave", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 70:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_police", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 80:
		{
			GiveNamedItem( "item_godwater" );
		}
		break;
	case 81:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_human_assault", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 82:
		{
			GiveNamedItem( "item_respawn" );
		}
		break;
	case 83:
		break;
	case 84:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_gonome", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 85:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_barney_shield", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 86:
		break;
	case 87:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_headcrab", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
		}
		break;
	case 88:
		{
			Game_Save_SecondData();
		}
		break;
	case 89:
		{
			Game_Load_SecondData();
		}
		break;
	case 95:
		{
			char text[256];
			sprintf( text, "- Kills:%d Diamonds:%d End Frags:%d Game Time:%1.0f\n", m_enemy_kills, m_player_diamonds, m_ending_frags, m_player_time / 60 );
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Coordinates:%1.0f,%1.0f,%1.0f, Angle:%1.0f\n", pev->origin.x, pev->origin.y, pev->origin.z, pev->angles.y );
			UTIL_SayTextAll( text,this );
			sprintf( text, "- Game Time:%1.0f\n", gpGlobals->time );
			UTIL_SayTextAll( text,this );
			sprintf( text, "- EXP:%d  LEVEL:%d\n", m_kadoma_exp, m_kadoma_level );
			UTIL_SayTextAll( text,this );

			if(m_fSecondWorld)
			{
				sprintf( text, "- Currently in New Game Plus mode\n" );
				UTIL_SayTextAll( text,this );
			}

			m_fNextClearTextTime = gpGlobals->time + 6.0;
		}
		break;
	case 94:
		{
			GiveNamedItem( "item_armor4" );
		}
		break;
	case 93:
		{
			GiveNamedItem( "item_armor3" );
		}
		break;
	case 92:
		{
			GiveNamedItem( "item_armor2" );
		}
		break;
	case 91:
		{
			GiveNamedItem( "item_armor1" );
		}
		break;
	case 90:
		{
			MenuItem_add(15);
			m_fequip1 = TRUE;
			MenuItem_add(16);
			m_fequip2 = TRUE;
			g_engfuncs.pfnSetPhysicsKeyValue( edict(), "mario", "1" );
			MenuItem_add(18);
			m_fequip4 = TRUE;
			m_air_oxyan_max = 2500;
			m_air_oxyan = m_air_oxyan_max;
			MenuItem_add(19);
			m_fequip5 = TRUE;
			MenuItem_add(20);
			m_fequip6 = TRUE;

			m_skill_reload = 1;
			m_skill_defguard = 2;
			m_skill_longjump = 3;
			m_skill_punch = 4;
			m_skill_valvesword = 5;
			m_skill_respawn = 6;
			m_skill_darkhide = 7;
			m_skill_deathmatch = 8;
			m_skill_wrongdoor = 9;
			m_skill_miss = 81;
			m_skill_goddam = 40;
			m_skill_locked = 41;
			g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
			m_kadoma_exp = 100000;//LVMAX

			g_causality_add = 5;
		}
		break;

	case 13:
		{
			if(g_Spawnpreacheally == TRUE)
			{
				g_Spawnpreacheally = FALSE;
			}
			else
			{
				g_Spawnpreacheally = TRUE;
			}
		}
		break;
	case 14:
		{
			m_ending_frags -= 50;
		}
		break;
	case 15:
		{
			m_ending_frags += 50;
		}
		break;
	case 16:
		{
			UTIL_MakeVectors( pev->v_angle );
			Create("monster_misaliya", pev->origin + Vector(64,0,0), Vector(0,pev->angles.y,0) );
			Create("monster_saintna", pev->origin + Vector(-64,0,0), Vector(0,pev->angles.y,0) );
			Create("monster_hime", pev->origin + Vector(0,64,0), Vector(0,pev->angles.y,0) );
			Create("monster_dragon", pev->origin + Vector(0,-64,0), Vector(0,pev->angles.y,0) );
			Create("monster_dengor", pev->origin + Vector(128,0,0), Vector(0,pev->angles.y,0) );
			Create("monster_nobita", pev->origin + Vector(-128,0,0), Vector(0,pev->angles.y,0) );
			Create("monster_giant", pev->origin + Vector(0,128,0), Vector(0,pev->angles.y,0) );
			Create("monster_willam", pev->origin + Vector(0,-128,0), Vector(0,pev->angles.y,0) );
			Create("monster_blues", pev->origin + Vector(128,64,0), Vector(0,pev->angles.y,0) );
			Create("monster_mario", pev->origin + Vector(-128,64,0), Vector(0,pev->angles.y,0) );
			Create("monster_andylow", pev->origin + Vector(128,-64,0), Vector(0,pev->angles.y,0) );
			Create("monster_wisebeast", pev->origin + Vector(-128,-64,0), Vector(0,pev->angles.y,0) );
			//Create("monster_lelite", pev->origin + Vector(0,0,96), Vector(0,pev->angles.y,0) );
		}
		break;
	case 17:
		{
			TeamMate_Nagamatagi_Allclear(10);
		}
		break;
	case 18:
		{
			TeamMate_Nagamatagi_Allclear(4);
		}
		break;
	case 19:
		{
			TeamMate_Nagamatagi_Allclear(5);
		}
		break;
	case 20:
		{
			TeamMate_Nagamatagi_Allclear(99);
		}
		break;
	case 97:
		{
			CBaseEntity *pSpot2 = UTIL_FindEntityByClassname( NULL, "monster_misaliya");
			if ( pSpot2 )
			{
				UTIL_Remove(pSpot2);
				UTIL_MakeVectors( pev->v_angle );
				CBaseEntity *pMisa = Create("monster_misaliya", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			}
			else
			{
				UTIL_MakeVectors( pev->v_angle );
				CBaseEntity *pMisa = Create("monster_misaliya", pev->origin + gpGlobals->v_forward * 128, Vector(0,pev->angles.y,0) );
			}
		}
		break;
	case 98:
		{
			BOSS_Find();
		}
		break;
	case 245:
		{
			if(m_fNextClearTextTime < gpGlobals->time)
			{
				char text[256];
				sprintf( text, "====================\n");
				UTIL_SayTextAll( text,this );
				sprintf( text, "- Level:%d  EXP:%d\n", m_kadoma_level, m_kadoma_exp );
				UTIL_SayTextAll( text,this );
				sprintf( text, "- Kills:%d  End Frags:%d\n", m_enemy_kills, m_ending_frags );
				UTIL_SayTextAll( text,this );
				if (g_iSkillLevel == SKILL_EASY)
				{
					sprintf( text, "- Difficulty: Easy\n" );
				}
				else if (g_iSkillLevel == SKILL_HARD){
					sprintf( text, "- Difficulty: Hard\n" );
				}
				else
				{
					sprintf( text, "- Difficulty: Medium\n" );
				}
				UTIL_SayTextAll( text,this );
				sprintf( text, "- Game Time:%1.0f min\n", m_player_time / 60 );
				UTIL_SayTextAll( text,this );
				sprintf( text, "====================\n");
				UTIL_SayTextAll( text,this );
				m_fNextClearTextTime = gpGlobals->time + 6.0;
			}
		}
		break;
	case 125:
		{
			m_kadoma_exp += 2000;
		}
		break;
	case 101:
		gEvilImpulse101 = TRUE;
		GiveNamedItem( "item_suit" );
		GiveNamedItem( "item_flashlight" );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_9mmhandgun" );
		GiveNamedItem( "ammo_9mmclip" );
		GiveNamedItem( "weapon_shotgun" );
		GiveNamedItem( "ammo_buckshot" );
		GiveNamedItem( "weapon_9mmAR" );
		GiveNamedItem( "ammo_9mmAR" );
		GiveNamedItem( "ammo_ARgrenades" );
		GiveNamedItem( "weapon_handgrenade" );
		GiveNamedItem( "weapon_tripmine" );
		GiveNamedItem( "ammo_m16clip" );
		GiveNamedItem( "weapon_smg" );
		GiveNamedItem( "ammo_smgclip" );
		GiveNamedItem( "weapon_dueluzi" );
		GiveNamedItem( "ammo_uzi" );
#if !OEM_BUILD
		GiveNamedItem( "weapon_357" );
		GiveNamedItem( "ammo_357" );
		GiveNamedItem( "weapon_crossbow" );
		GiveNamedItem( "ammo_crossbow" );
		GiveNamedItem( "weapon_egon" );
		GiveNamedItem( "weapon_gauss" );
		GiveNamedItem( "ammo_gaussclip" );
		GiveNamedItem( "weapon_rpg" );
		GiveNamedItem( "ammo_rpgclip" );
		GiveNamedItem( "weapon_satchel" );
		GiveNamedItem( "weapon_snark" );
		GiveNamedItem( "weapon_hornetgun" );
		GiveNamedItem( "weapon_ak47" );
		GiveNamedItem( "weapon_valvesword" );
		GiveNamedItem( "weapon_fist" );
		GiveNamedItem( "weapon_deagle" );
		GiveNamedItem( "weapon_medkit" );
		GiveNamedItem( "weapon_hammer" );
		GiveNamedItem( "weapon_displacer" );
		GiveNamedItem( "weapon_redeemer" );
		GiveNamedItem( "weapon_sniperrifle" );
		GiveNamedItem( "weapon_darkgrenade" );
		GiveNamedItem( "ammo_762" );
		GiveNamedItem( "weapon_sg550" );
		GiveNamedItem( "ammo_sg550" );
		GiveNamedItem( "weapon_airgun" );
		GiveNamedItem( "weapon_minigun" );
		GiveNamedItem( "ammo_m134box" );
		GiveNamedItem( "weapon_dualdbarrel" );
#endif
		gEvilImpulse101 = FALSE;
		break;
	case 102:
		gEvilImpulse101 = TRUE;
		GiveNamedItem( "item_suit" );
		GiveNamedItem( "item_flashlight" );
		GiveNamedItem( "weapon_fist" );
		gEvilImpulse101 = FALSE;
		break;
	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if( pEntity )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if( pMonster )
				pMonster->ReportAIState();
		}
		break;
	case 110:
		{
			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 2048, dont_ignore_monsters, ENT(pev), & tr);

			if ( tr.flFraction != 1.0 )
			{	
				// line hit something, so paint a decal
				CBaseEntity *pEntity2 = CBaseEntity::Instance(tr.pHit);
					
				if(pEntity2)
				{
					CBaseMonster *pMonster = pEntity2->MyMonsterPointer();
					if ( pMonster )
					{
						if(pMonster->pev->deadflag == DEAD_NO)
						{
							pMonster->Killed( pev, GIB_NEVER );
							FX_Explosion( pMonster->Center(), 107 );
							pMonster->pev->health = 0;
						}
					}
				}
			}

		}
		break;
	case 111:
		{
			m_kadoma_exp += 1000;
		}
		break;
	case 112:
		{
			FX_Explosion( pev->origin, 254 );
		}
		break;
	case 113:
		{
			m_fSelectMode = TRUE;
			ShowVGUIMenu(31);
		}
		break;
	case 211:
		{
			if (m_boss_find)
			{
				if(m_boss_find->pev->deadflag == DEAD_NO)
				{
					m_boss_find->Killed( pev, GIB_NEVER );
					m_boss_find->pev->health = 0;
					FX_Explosion( m_boss_find->Center(), 107 );
					UTIL_ScreenFade( this, Vector(255,255,255), 0.3, 0.1, 64, FFADE_IN );
				}
			}
		}
		break;
	case 212:
		{
			if (m_team_npc1 != NULL)
			{
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(96,0,-36) );
			}
			if (m_team_npc2 != NULL)
			{
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(-96,0,-36) );
			}
			if (m_team_npc3 != NULL)
			{
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(0,96,-36) );
			}
			if (m_team_npc4 != NULL)
			{
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-96,-36) );
			}
		}
		break;
	case 213:
		{
			if (m_team_npc1 != NULL)
			{
				UTIL_SetOrigin( m_team_npc1->pev, pev->origin + Vector(64,0,-36) );
			}
			if (m_team_npc2 != NULL)
			{
				UTIL_SetOrigin( m_team_npc2->pev, pev->origin + Vector(-64,0,-36) );
			}
			if (m_team_npc3 != NULL)
			{
				UTIL_SetOrigin( m_team_npc3->pev, pev->origin + Vector(0,64,-36) );
			}
			if (m_team_npc4 != NULL)
			{
				UTIL_SetOrigin( m_team_npc4->pev, pev->origin + Vector(0,-64,-36) );
			}
			if (m_team_npc5 != NULL)
			{
				UTIL_SetOrigin( m_team_npc5->pev, pev->origin + Vector(0,-128,-36) );
			}
			if (m_team_npc6 != NULL)
			{
				UTIL_SetOrigin( m_team_npc6->pev, pev->origin + Vector(0,128,-36) );
			}
			if (m_team_npc7 != NULL)
			{
				UTIL_SetOrigin( m_team_npc7->pev, pev->origin + Vector(-128,0,-36) );
			}
			if (m_team_npc8 != NULL)
			{
				UTIL_SetOrigin( m_team_npc8->pev, pev->origin + Vector(128,0,-36) );
			}
			if (m_team_npc9 != NULL)
			{
				UTIL_SetOrigin( m_team_npc9->pev, pev->origin + Vector(64,64,-36) );
			}
			if (m_team_npc10 != NULL)
			{
				UTIL_SetOrigin( m_team_npc10->pev, pev->origin + Vector(64,-64,-36) );
			}
			if (m_team_npc11 != NULL)
			{
				UTIL_SetOrigin( m_team_npc11->pev, pev->origin + Vector(-64,-64,-36) );
			}
			if (m_team_npc12 != NULL)
			{
				UTIL_SetOrigin( m_team_npc12->pev, pev->origin + Vector(-64,64,-36) );
			}
		}
		break;
	case 115:
		BOSS_Find();
		break;
	case 120:
		{
			UTIL_ScreenFade( this, Vector(255,255,255), 1, 1, 255, FFADE_IN );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/Flash3.wav", 1, 0);

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_LARGEFUNNEL );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFlareGlow );
			WRITE_SHORT( 1 );
			MESSAGE_END();

			pev->health = pev->max_health;
			TeamMate_Nagamatagi_RespawnStone(1);
		}
		break;
	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;
	case 105:// player makes no sound for monsters to hear.
		if( m_fNoPlayerSound )
		{
			ALERT( at_console, "Player is audible\n" );
			m_fNoPlayerSound = FALSE;
		}
		else
		{
			ALERT( at_console, "Player is silent\n" );
			m_fNoPlayerSound = TRUE;
		}
		break;
	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if( pEntity )
		{
			ALERT( at_console, "Classname: %s", STRING( pEntity->pev->classname ) );

			if( !FStringNull( pEntity->pev->targetname ) )
			{
				ALERT( at_console, " - Targetname: %s\n", STRING( pEntity->pev->targetname ) );
			}
			else
			{
				ALERT( at_console, " - TargetName: No Targetname\n" );
			}

			ALERT ( at_console, "Origin: - X:%1.0f Y:%1.0f Z:%1.0f\n", pEntity->pev->origin.x,pEntity->pev->origin.y,pEntity->pev->origin.z );
			ALERT ( at_console, "Angles.Y: - %1.0f\n", pEntity->pev->angles.y);
			ALERT( at_console, "Model: %s\n", STRING( pEntity->pev->model ) );
			ALERT ( at_console, "Health: %1.0f\n", pEntity->pev->health);
			if( pEntity->pev->globalname )
				ALERT( at_console, "Globalname: %s\n", STRING( pEntity->pev->globalname ) );
		}
		break;
	case 107:
		{
			//TraceResult tr;

			edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;
			UTIL_TraceLine( start, end, ignore_monsters, edict(), &tr );
			if( tr.pHit )
				pWorld = tr.pHit;
			const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
			if( pTextureName )
				ALERT( at_console, "Texture: %s\n", pTextureName );
		}
		break;
	case 108:
		{
			// Give me the classname and targetname of this entity.
			pEntity = FindEntityForward( this );
			if ( pEntity )
			{
				ALERT ( at_console, "Origin: - X:%1.0f Y:%1.0f Z:%1.0f\n", pEntity->pev->origin.x,pEntity->pev->origin.y,pEntity->pev->origin.z );
				Vector old = pEntity->pev->origin;
				pEntity->pev->angles.y += 90;
				pEntity->pev->origin.z += 1;	// Pick up off of the floor
				UTIL_SetSize (pEntity->pev, pEntity->pev->mins, pEntity->pev->maxs);
				UTIL_SetOrigin( pEntity->pev, old - (pEntity->pev->mins + pEntity->pev->maxs)* 0.5 );
				ALERT ( at_console, "Origin: - X:%1.0f Y:%1.0f Z:%1.0f\n", pEntity->pev->origin.x,pEntity->pev->origin.y,pEntity->pev->origin.z );
			}
		}
		break;
	case 195:
		// show shortest paths for entire level to nearest node
		{
			Create( "node_viewer_fly", pev->origin, pev->angles );
		}
		break;
	case 196:
		// show shortest paths for entire level to nearest node
		{
			Create( "node_viewer_large", pev->origin, pev->angles );
		}
		break;
	case 197:
		// show shortest paths for entire level to nearest node
		{
			Create( "node_viewer_human", pev->origin, pev->angles );
		}
		break;
	case 199:
		// show nearest node and all connections
		{
			ALERT( at_console, "%d\n", WorldGraph.FindNearestNode( pev->origin, bits_NODE_GROUP_REALM ) );
			WorldGraph.ShowNodeConnections( WorldGraph.FindNearestNode( pev->origin, bits_NODE_GROUP_REALM ) );
		}
		break;
	case 202:
		// Random blood splatter
		UTIL_MakeVectors( pev->v_angle );
		UTIL_TraceLine( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT( pev ), &tr );

		if( tr.flFraction != 1.0f )
		{
			// line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr( (CBloodSplat *)NULL );
			pBlood->Spawn( pev );
		}
		break;
	case 203:
		// remove creature.
		pEntity = FindEntityForward( this );
		if( pEntity )
		{
			UTIL_Remove( pEntity );
			/*if( pEntity->pev->takedamage )
				pEntity->SetThink( &CBaseEntity::SUB_Remove );*/
		}
		break;
	}
#endif	// HLDEMO_BUILD
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem( CBasePlayerItem *pItem )
{
	CBasePlayerItem *pInsert;

	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while( pInsert )
	{
		if( FClassnameIs( pInsert->pev, STRING( pItem->pev->classname ) ) )
		{
			if( pItem->AddDuplicate( pInsert ) )
			{
				g_pGameRules->PlayerGotWeapon( this, pItem );
				//pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo();
				if( m_pActiveItem )
					m_pActiveItem->UpdateItemInfo();

				pItem->Kill();
			}
			else if( gEvilImpulse101 )
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill();
			}
			return FALSE;
		}
		pInsert = pInsert->m_pNext;
	}

	if( pItem->AddToPlayer( this ) )
	{
		g_pGameRules->PlayerGotWeapon( this, pItem );
		//pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		// should we switch to this item?
		if( g_pGameRules->FShouldSwitchWeapon( this, pItem ) )
		{
			SwitchWeapon( pItem );
		}

		return TRUE;
	}
	else if( gEvilImpulse101 )
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill();
	}
	return FALSE;
}

int CBasePlayer::RemovePlayerItem( CBasePlayerItem *pItem, bool bCallHolster )
{
	pItem->pev->nextthink = 0;// crowbar may be trying to swing again, etc.
	pItem->SetThink( NULL );

	if( m_pActiveItem == pItem )
	{
		ResetAutoaim();
		if( bCallHolster )
			pItem->Holster();
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}

	// In some cases an item can be both the active and last item, like for instance dropping all weapons and only having an exhaustible weapon left. - Solokiller
	if( m_pLastItem == pItem )
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if( pPrev == pItem )
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return TRUE;
	}
	else
	{
		while( pPrev && pPrev->m_pNext != pItem )
		{
			pPrev = pPrev->m_pNext;
		}
		if( pPrev )
		{
			pPrev->m_pNext = pItem->m_pNext;
			return TRUE;
		}
	}
	return FALSE;
}

//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer::GiveAmmo( int iCount, const char *szName, int iMax )
{
	if( !szName )
	{
		// no ammo.
		return -1;
	}

	if( !g_pGameRules->CanHaveAmmo( this, szName, iMax ) )
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex( szName );

	if( i < 0 || i >= MAX_AMMO_SLOTS )
		return -1;

	int iAdd = Q_min( iCount, iMax - m_rgAmmo[i] );
	if( iAdd < 1 )
		return i;

	m_rgAmmo[i] += iAdd;

	if( gmsgAmmoPickup )  // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
			WRITE_BYTE( GetAmmoIndex( szName ) );		// ammo ID
			WRITE_BYTE( iAdd );		// amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}

/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
	if(pev->deadflag != DEAD_NO)
		return;

	static int fInSelect = FALSE;

	ImpulseCommands();

	if(m_barnacle_RTP == 1)
	{
		if ( (pev->button & IN_ATTACK2) || (pev->button & IN_ATTACK) )
		{
			if(m_barnacle_RTP_button == 0)
			{
				m_barnacle_RTP_button = 1;
				if(m_barnacle_Level == 1)
				{
					m_barnacle_RTP_bar += 45;
				}
				else if(m_barnacle_Level == 2)
				{
					m_barnacle_RTP_bar += 35;
				}
				else
				{
					m_barnacle_RTP_bar += 30;
				}

				if(m_guard_mynpc >= 1)
				{
					m_barnacle_RTP_bar += 5;
				}

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "newadd/struggle_hit.wav", 1, ATTN_NORM);
			}
		}
		else if ( !(pev->button & (IN_ATTACK|IN_ATTACK2) ) )
		{
				m_barnacle_RTP_button = 0;
		}
	}

	if(m_rpg_menu_actor1 != 1)
	{
		if(m_mode_origin != g_vecZero)
		{
			if(pev->origin.x > m_mode_origin.x + 1024)
			{
				pev->origin.x = m_mode_origin.x + 1024;
			}
			else if(pev->origin.x < m_mode_origin.x - 1024)
			{
				pev->origin.x = m_mode_origin.x - 1024;
			}
			if(pev->origin.y > m_mode_origin.y + 1024)
			{
				pev->origin.y = m_mode_origin.y + 1024;
			}
			else if(pev->origin.y < m_mode_origin.y - 1024)
			{
				pev->origin.y = m_mode_origin.y - 1024;
			}
			if(pev->origin.z > m_mode_origin.z + 640)
			{
				pev->origin.z = m_mode_origin.z + 640;
			}
			else if(pev->origin.z < m_mode_origin.z)
			{
				pev->origin.z = m_mode_origin.z;
			}
		}	
	}

	if(m_fMoveItem != NULL || m_guard_mynpc == 1 || m_rpg_menu_actor1 != 1 || m_concussion_time > gpGlobals->time)
		return;

#if CLIENT_WEAPONS
	if( m_flNextAttack > 0 )
#else
	if( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if( !m_pActiveItem )
		return;

	m_pActiveItem->ItemPreFrame();
}

/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	//static int fInSelect = FALSE;

	// check if the player is using a tank
	if( m_pTank != 0 )
		return;

#if CLIENT_WEAPONS
	if( m_flNextAttack > 0 )
#else
	if( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	ImpulseCommands();

	if( !m_pActiveItem )
		return;

	m_pActiveItem->ItemPostFrame();
}

int CBasePlayer::AmmoInventory( int iAmmoIndex )
{
	if( iAmmoIndex == -1 )
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex( const char *psz )
{
	int i;

	if( !psz )
		return -1;

	for( i = 1; i < MAX_AMMO_SLOTS; i++ )
	{
		if( !CBasePlayerItem::AmmoInfoArray[i].pszName )
			continue;

		if( stricmp( psz, CBasePlayerItem::AmmoInfoArray[i].pszName ) == 0 )
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate( void )
{
	for( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		if( m_rgAmmo[i] != m_rgAmmoLast[i] )
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			ASSERT( m_rgAmmo[i] >= 0 );
			ASSERT( m_rgAmmo[i] < 255 );

			// send "Ammo" update message
			MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( Q_max( Q_min( m_rgAmmo[i], 999 ), 0 ) );
				//WRITE_BYTE( Q_max( Q_min( m_rgAmmo[i], 254 ), 0 ) );  // clamp the value to one byte
			MESSAGE_END();
		}
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData( void )
{
	if( m_fInitHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgTbutton, NULL, pev );
			WRITE_SHORT( 0 );
		MESSAGE_END();

		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if( !m_fGameHUDInitialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;

			m_iObserverLastMode = OBS_ROAMING;

			if( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
			}
		}

		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		// Send flashlight status
		/*MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
			WRITE_BYTE( FlashlightIsOn() ? 1 : 0 );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();*/

		// Vit_amiN: the geiger state could run out of sync, too
		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		InitStatusBar();
	}

	//==================Wdoor RPG Menu====================
	//if(m_rpg_menu_on >= 1){
	//	if(m_rpg_menu_origin < 50){
	//	m_rpg_menu_origin++;
	//	}
	//}

	if(m_rpg_menu_on >= 0)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgRPGMenu, NULL, pev );
		WRITE_BYTE( m_rpg_menu_on );
		WRITE_BYTE( m_rpg_menu_select );

		if(m_rpg_menu_on == 0)
		{
			m_rpg_menu_on = -1;
			m_rpg_menu_select_alpha = 0;
		}

		WRITE_BYTE( m_rpg_menu_select_alpha );
		//WRITE_BYTE( m_rpg_menu_origin );

		if(m_rpg_menu_on == 1 || m_rpg_menu_on == 4 || m_rpg_menu_on == 6)
		{
			WRITE_BYTE( m_rpg_menu_actor1 );
			if(pev->deadflag != DEAD_NO)
			{
				WRITE_BYTE( 0 );
			}
			else
			{
				WRITE_BYTE( (int)200 * (pev->health / pev->max_health) );
			}
			WRITE_LONG((int)pev->max_health );
			WRITE_LONG((int)pev->health );
			WRITE_BYTE( m_kadoma_level );
			int exp = (m_kadoma_exp - (1000 * m_kadoma_level)) * 0.2;
			if(m_kadoma_level == 99)
			{
				exp = 200;
			}
			WRITE_BYTE( exp );
			if(m_kadoma_exp > 99999)
			{
				m_kadoma_exp = 99999;
			}
			WRITE_LONG( m_kadoma_exp );
			WRITE_BYTE( 0 );
		}
		else
		{
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			WRITE_LONG( 0 );
			WRITE_LONG( 0 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			WRITE_LONG( 0 );
			WRITE_BYTE( 0 );
		}

		if(m_team_npc1 != NULL && (m_rpg_menu_on == 1 || m_rpg_menu_on == 4) )
		{
			CBaseMonster *pMonster = m_team_npc1->MyMonsterPointer();
			if ( pMonster )
			{
				m_rpg_menu_hp2 = pMonster->pev->health;
				m_rpg_menu_maxhp2 = pMonster->pev->max_health;
				m_rpg_menu_level2 = pMonster->m_rpgms_level;
				m_rpg_menu_exp2 = pMonster->m_rpgms_exp;
				m_rpg_menu_maxexp2 = pMonster->m_rpgms_maxexp;
			}
		}
		if(m_team_npc1 == NULL || m_team_npc1->pev->deadflag != DEAD_NO)
		{
			if(m_team_npc1 != NULL)
			{
				WRITE_BYTE( m_rpg_menu_actor2 );
				m_rpg_menu_hp2 = 0;
			}
			else
			{
				WRITE_BYTE( 0 );
			}
			WRITE_BYTE( 0 );
			WRITE_LONG((int)m_rpg_menu_maxhp2 );
			WRITE_LONG( 0 );
		}
		else
		{
			WRITE_BYTE( m_rpg_menu_actor2 );
			WRITE_BYTE( (int)200 * (m_rpg_menu_hp2 / m_rpg_menu_maxhp2) );
			WRITE_LONG((int)m_rpg_menu_maxhp2 );
			WRITE_LONG((int)m_rpg_menu_hp2 );
		}

		WRITE_BYTE( m_rpg_menu_level2 );
		WRITE_BYTE( int(m_rpg_menu_exp2 * 0.2) );
		WRITE_LONG( m_rpg_menu_maxexp2 );
		WRITE_BYTE( 0 );

		if(m_team_npc2 != NULL && (m_rpg_menu_on == 1 || m_rpg_menu_on == 4) )
		{
			CBaseMonster *pMonster = m_team_npc2->MyMonsterPointer();
			if ( pMonster )
			{
				m_rpg_menu_hp3 = pMonster->pev->health;
				m_rpg_menu_maxhp3 = pMonster->pev->max_health;
				m_rpg_menu_level3 = pMonster->m_rpgms_level;
				m_rpg_menu_exp3 = pMonster->m_rpgms_exp;
				m_rpg_menu_maxexp3 = pMonster->m_rpgms_maxexp;
			}
		}
		if(m_team_npc2 == NULL || m_team_npc2->pev->deadflag != DEAD_NO)
		{
			if(m_team_npc2 != NULL)
			{
				WRITE_BYTE( m_rpg_menu_actor3 );
			}
			else
			{
				WRITE_BYTE( 0 );
			}
			WRITE_BYTE( 0 );
			WRITE_LONG((int)m_rpg_menu_maxhp3 );
			WRITE_LONG( 0 );
		}
		else
		{
			WRITE_BYTE( m_rpg_menu_actor3 );
			WRITE_BYTE( (int)200 * (m_rpg_menu_hp3 / m_rpg_menu_maxhp3) );
			WRITE_LONG((int)m_rpg_menu_maxhp3 );
			WRITE_LONG((int)m_rpg_menu_hp3 );
		}
			
		WRITE_BYTE( m_rpg_menu_level3 );
		WRITE_BYTE( int(m_rpg_menu_exp3 * 0.2) );
		WRITE_LONG( m_rpg_menu_maxexp3 );
		WRITE_BYTE( 0 );

		if(m_team_npc3 != NULL && (m_rpg_menu_on == 1 || m_rpg_menu_on == 4) )
		{
			CBaseMonster *pMonster = m_team_npc3->MyMonsterPointer();
			if ( pMonster )
			{
				m_rpg_menu_hp4 = pMonster->pev->health;
				m_rpg_menu_maxhp4 = pMonster->pev->max_health;
				m_rpg_menu_level4 = pMonster->m_rpgms_level;
				m_rpg_menu_exp4 = pMonster->m_rpgms_exp;
				m_rpg_menu_maxexp4 = pMonster->m_rpgms_maxexp;
			}
		}
		if(m_team_npc3 == NULL || m_team_npc3->pev->deadflag != DEAD_NO)
		{
			if(m_team_npc3 != NULL)
			{
				WRITE_BYTE( m_rpg_menu_actor4 );
			}
			else
			{
				WRITE_BYTE( 0 );
			}
			WRITE_BYTE( 0 );
			WRITE_LONG((int)m_rpg_menu_maxhp4 );
			WRITE_LONG( 0 );
		}
		else
		{
			WRITE_BYTE( m_rpg_menu_actor4 );
			WRITE_BYTE( (int)200 * (m_rpg_menu_hp4 / m_rpg_menu_maxhp4) );
			WRITE_LONG((int)m_rpg_menu_maxhp4 );
			WRITE_LONG((int)m_rpg_menu_hp4 );
		}

		WRITE_BYTE( m_rpg_menu_level4 );
		WRITE_BYTE( int(m_rpg_menu_exp4 * 0.2) );
		WRITE_LONG( m_rpg_menu_maxexp4 );
		WRITE_BYTE( 0 );

		if(m_team_npc4 != NULL && (m_rpg_menu_on == 1 || m_rpg_menu_on == 4) )
		{
			CBaseMonster *pMonster = m_team_npc4->MyMonsterPointer();
			if ( pMonster )
			{
				m_rpg_menu_hp5 = pMonster->pev->health;
				m_rpg_menu_maxhp5 = pMonster->pev->max_health;
				m_rpg_menu_level5 = pMonster->m_rpgms_level;
				m_rpg_menu_exp5 = pMonster->m_rpgms_exp;
				m_rpg_menu_maxexp5 = pMonster->m_rpgms_maxexp;
			}
		}
		if(m_team_npc4 == NULL || m_team_npc4->pev->deadflag != DEAD_NO)
		{
			if(m_team_npc4 != NULL)
			{
				WRITE_BYTE( m_rpg_menu_actor5 );
			}
			else
			{
				WRITE_BYTE( 0 );
			}
			WRITE_BYTE( 0 );
			WRITE_LONG((int)m_rpg_menu_maxhp5 );
			WRITE_LONG( 0 );
		}
		else
		{
			WRITE_BYTE( m_rpg_menu_actor5 );
			WRITE_BYTE( (int)200 * (m_rpg_menu_hp5 / m_rpg_menu_maxhp5) );
			WRITE_LONG((int)m_rpg_menu_maxhp5 );
			WRITE_LONG((int)m_rpg_menu_hp5 );
		}
			
		WRITE_BYTE( m_rpg_menu_level5 );
		WRITE_BYTE( int(m_rpg_menu_exp5 * 0.2) );
		WRITE_LONG( m_rpg_menu_maxexp5 );
		WRITE_BYTE( 0 );

		WRITE_BYTE( m_rpg_menu_item1 );
		WRITE_BYTE( m_rpg_menu_item2 );
		WRITE_BYTE( m_rpg_menu_item3 );
		WRITE_BYTE( m_rpg_menu_item4 );
		WRITE_BYTE( m_rpg_menu_item5 );
		WRITE_BYTE( m_rpg_menu_item6 );
		WRITE_BYTE( m_rpg_menu_item7 );
		WRITE_BYTE( m_rpg_menu_item8 );
		WRITE_BYTE( m_rpg_menu_item9 );
		WRITE_BYTE( m_rpg_menu_item10 );
		WRITE_BYTE( m_rpg_menu_item11 );
		WRITE_BYTE( m_rpg_menu_item12 );
		WRITE_BYTE( m_rpg_menu_item_s );
		WRITE_BYTE( m_rpg_menu_item_e );
		WRITE_BYTE( m_rpg_menu_skill_chater );
		
		if(m_rpg_menu_on == 8)
		{
			GetGame_Playcvar();
		}
		
		WRITE_BYTE( m_rpg_menu_skill1 );
		WRITE_BYTE( m_rpg_menu_skill2 );
		WRITE_BYTE( m_rpg_menu_skill3 );
		WRITE_BYTE( m_rpg_menu_skill4 );
		WRITE_BYTE( m_rpg_menu_skill5 );
		WRITE_LONG( m_rpg_menu_skill6 );
		WRITE_BYTE( m_rpg_menu_skill7 );
		WRITE_BYTE( m_rpg_menu_skill8 );
		WRITE_BYTE( m_rpg_menu_skill9 );
		WRITE_BYTE( m_rpg_menu_skill10 );
		WRITE_BYTE( m_rpg_menu_skill11 );
		WRITE_BYTE( m_rpg_menu_skill12 );
		MESSAGE_END();
	}
	//===============================

	//=========================PassWord
	if(m_rpg_password_on >= 0)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgPWBord, NULL, pev );
		WRITE_BYTE( m_rpg_password_on );
		WRITE_BYTE( m_rpg_password_select );
		WRITE_BYTE( m_rpg_password_light1 );
		WRITE_BYTE( m_rpg_password_light2 );
		WRITE_BYTE( m_rpg_password_light3 );
		WRITE_BYTE( m_rpg_password_light4 );
		WRITE_BYTE( m_rpg_password_light5 );
		WRITE_BYTE( m_rpg_password_light6 );
		WRITE_BYTE( m_rpg_password_light7 );
		WRITE_BYTE( m_rpg_password_light8 );
		WRITE_BYTE( m_rpg_password_light9 );
		MESSAGE_END();
	}

	if( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if( m_iFOV != m_iClientFOV )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		if(m_iFOV >= 10 && m_iFOV <= 20)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgGunScope, NULL, pev );
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgGunScope, NULL, pev );
			WRITE_BYTE( 0 );
			MESSAGE_END();
		}

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if( gDisplayTitle )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if(m_guard_mynpc != m_iClient_mynpc)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgModeShow, NULL, pev );
		if ( (pev->flags & FL_FROZEN) || pev->deadflag != DEAD_NO)
		{
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
		}
		else
		{
			WRITE_BYTE( m_guard_mynpc );
			WRITE_BYTE( m_rpg_menu_item_t );
		}
		
		MESSAGE_END();
		m_iClient_mynpc = m_guard_mynpc;
	}

	if(m_air_oxyan != m_iClient_oxyan || m_skill_darkhide_on)
	{
		m_iClient_oxyan = m_air_oxyan;

		if(m_air_oxyan_max == 2000 && m_fequip4 == TRUE)
		{
			m_air_oxyan_max = 2500;
		}
		if(m_air_oxyan_max == 2500 && m_fequip4 == FALSE)
		{
			m_air_oxyan_max = 2000;
		}

		if(m_skill_darkhide_on)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgAirBar, NULL, pev );
			WRITE_BYTE( 2 );
			WRITE_BYTE( m_darkposion / 10 );
			WRITE_BYTE( 250 );
			MESSAGE_END();
		}
		else if(m_fequip4 == TRUE)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgAirBar, NULL, pev );
			WRITE_BYTE( m_air_show );
			WRITE_BYTE( m_air_oxyan / 10 );
			WRITE_BYTE( 250 );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgAirBar, NULL, pev );
			WRITE_BYTE( m_air_show );
			WRITE_BYTE( m_air_oxyan / 10 );
			WRITE_BYTE( 200 );
			MESSAGE_END();
		}
	}

	if( pev->health != m_iClientHealth )
	{
		if (m_darkposion > 0 || m_skill_darkhide_on)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 2 );
			MESSAGE_END();
		}
		else if (pev->health > 0 && m_godposion == 1 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 4 );
			MESSAGE_END();
		}
		else if (pev->health > 0 && m_greenpoison == 1)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 3 );
			MESSAGE_END();
		}
		else if(pev->health <= pev->max_health * 0.2)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 1 );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 0 );
			MESSAGE_END();
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgHPbar, NULL, pev );
		WRITE_BYTE( m_rpg_menu_actor1 );
		if(pev->max_health <= 300)
		{
			WRITE_BYTE( (int)pev->max_health * (pev->health / pev->max_health) * 0.8 );
			WRITE_BYTE( (int)pev->max_health * 0.8 );
		}
		else
		{
			WRITE_BYTE( (int)240 * (pev->health / pev->max_health));
			WRITE_BYTE( 240 );
		}
		WRITE_SHORT( (int)pev->health);
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}

	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		if(pev->health == m_iClientHealth)
		{
			if (m_darkposion > 0 || m_skill_darkhide_on)
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 2 );
				MESSAGE_END();
			}
			else if (pev->health > 0 && m_godposion == 1 )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 4 );
				MESSAGE_END();
			}
			else if (pev->health > 0 && m_greenpoison == 1)
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 3 );
				MESSAGE_END();
			}
			else if(pev->health <= pev->max_health * 0.2)
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 1 );
				WRITE_BYTE( 1 );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgDarkHoles, NULL, pev );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 0 );
				MESSAGE_END();
			}
		}

		if(m_skill_maxarmor >= 1)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgAPbar, NULL, pev );
			WRITE_BYTE( m_rpg_menu_actor1 );
			if(m_skill_maxarmor <= 200)
			{
				WRITE_BYTE( (int)m_skill_maxarmor * (pev->armorvalue / m_skill_maxarmor) * 1.2);
				WRITE_BYTE( (int)m_skill_maxarmor * 1.2 );
			}
			else
			{
				WRITE_BYTE( (int)240 * (pev->armorvalue / m_skill_maxarmor) );
				WRITE_BYTE( 240 );
			}
			WRITE_SHORT( (int)pev->armorvalue);
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgAPbar, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( (int)240 * (pev->armorvalue / m_skill_maxarmor) );
			WRITE_BYTE( 240 );
			WRITE_SHORT( (int)pev->armorvalue);
			MESSAGE_END();
		}
	}

	if( pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType )
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if( other )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( other );
			if( pEntity )
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( (int)pev->dmg_save );
			WRITE_BYTE( (int)pev->dmg_take );
			WRITE_LONG( visibleDamageBits );
			WRITE_COORD( damageOrigin.x );
			WRITE_COORD( damageOrigin.y );
			WRITE_COORD( damageOrigin.z );
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if( ( m_flFlashLightTime ) && ( m_flFlashLightTime <= gpGlobals->time ) )
	{
		if( FlashlightIsOn() )
		{
			if( m_iFlashBattery )
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;

				if( m_iFlashBattery <= 15 )
				{
					FlashlightTurnOff();
					m_iFlashBattery = 5;
				}
			}
		}
		else
		{
			if( m_iFlashBattery < 100 )
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();
	}

	if( m_iTrain & TRAIN_NEW )
	{
		ASSERT( gmsgTrain > 0 );

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgTrain, NULL, pev );
			WRITE_BYTE( m_iTrain & 0xF );
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if( !m_fKnownItem )
	{
		m_fKnownItem = TRUE;

	// WeaponInit Message
	// byte  = # of weapons
	//
	// for each weapon:
	// byte		name str length (not including null)
	// bytes... name
	// byte		Ammo Type
	// byte		Ammo2 Type
	// byte		bucket
	// byte		bucket pos
	// byte		flags
	// ????		Icons

		// Send ALL the weapon info now
		int i;

		for( i = 0; i < MAX_WEAPONS; i++ )
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if( !II.iId )
				continue;

			const char *pszName;
			if( !II.pszName )
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );  
				WRITE_STRING( pszName );			// string	weapon name
				WRITE_BYTE( GetAmmoIndex( II.pszAmmo1 ) );	// byte		Ammo Type
				WRITE_BYTE( II.iMaxAmmo1 );				// byte     Max Ammo 1
				WRITE_BYTE( GetAmmoIndex( II.pszAmmo2 ) );	// byte		Ammo2 Type
				WRITE_BYTE( II.iMaxAmmo2 );				// byte     Max Ammo 2
				WRITE_BYTE( II.iSlot );					// byte		bucket
				WRITE_BYTE( II.iPosition );				// byte		bucket pos
				WRITE_BYTE( II.iId );						// byte		id (bit index into pev->weapons)
				WRITE_BYTE( II.iFlags );					// byte		Flags
			MESSAGE_END();
		}
	}

	SendAmmoUpdate();

	// Update all the items
	for( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if( m_rgpPlayerItems[i] )  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if( m_flNextSBarUpdateTime < gpGlobals->time )
	{

		if(!(pev->flags & FL_FROZEN) && CVAR_GET_FLOAT( "hud_centerid" ) != 0)
			UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2f;
	}
}

//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer::FBecomeProne( void )
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer::BarnacleVictimBitten( entvars_t *pevBarnacle )
{
	if(!FClassnameIs(pevBarnacle,"monster_barnacle_holy"))
		m_air_oxyan -= 20;
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns. 
//=========================================================
void CBasePlayer::BarnacleVictimReleased( void )
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

//=========================================================
// Illumination 
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer::Illumination( void )
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if( iIllum > 255 )
		return 255;
	return iIllum;
}

void CBasePlayer::SetPrefsFromUserinfo( char *infobuffer )
{
	const char *pszKeyVal;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_autowepswitch" );

	if( pszKeyVal[0] != '\0' )
		m_iAutoWepSwitch = atoi( pszKeyVal );
	else
		m_iAutoWepSwitch = 1;
}

void CBasePlayer::EnableControl( BOOL fControl )
{
	m_iFOV = 0;
	pev->fov = 0;
	pev->gravity = 1.0;

	if (!fControl)
	{
		m_rpg_menu_on = 0;
		CLIENT_COMMAND(edict(), "=cammousemove\n");
		pev->flags |= FL_FROZEN;

		if (m_pActiveItem)
		{
			m_pActiveItem->Holster();
		}

		if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}

		if ( NightViewIsOn() )
		{
			NightViewTurnOff();
		}
	}
	else
	{
		CLIENT_COMMAND(edict(), "-cammousemove\n");
		pev->flags &= ~FL_FROZEN;

		if (FBitSet( pev->flags, FL_NOTARGET ))
			pev->flags &= ~FL_NOTARGET;

		if (FBitSet( pev->flags, FL_GODMODE ))
			pev->flags &= ~FL_GODMODE;

		if (pev->movetype == MOVETYPE_NOCLIP)
			pev->movetype = MOVETYPE_WALK;

		m_god_time = 0;
		m_rpg_menu_actor1 = 1;

		m_player_camera = NULL;
		SET_VIEW( edict(), edict() );
	}

	m_iClient_mynpc     = -1;
}

#define DOT_1DEGREE   0.9998476951564
#define DOT_2DEGREE   0.9993908270191
#define DOT_3DEGREE   0.9986295347546
#define DOT_4DEGREE   0.9975640502598
#define DOT_5DEGREE   0.9961946980917
#define DOT_6DEGREE   0.9945218953683
#define DOT_7DEGREE   0.9925461516413
#define DOT_8DEGREE   0.9902680687416
#define DOT_9DEGREE   0.9876883405951
#define DOT_10DEGREE  0.9848077530122
#define DOT_15DEGREE  0.9659258262891
#define DOT_20DEGREE  0.9396926207859
#define DOT_25DEGREE  0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAutoaimVector( float flDelta )
{
	Vector vecSrc = GetGunPosition();
	float flDist = 8192.0f;

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta );

	// update ontarget if changed
	if( m_fOldTargeting != m_fOnTarget )
	{
		m_pActiveItem->UpdateItemInfo();
	}

	// Don't send across network if sv_aim is 0
	if(m_fOnTarget == 1)
	{
		m_newcross_ontarget = 1;
	}
	else
	{
		m_newcross_ontarget = 0;
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	return gpGlobals->v_forward;
}

Vector CBasePlayer::AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta )
{
	edict_t *pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity *pEntity;
	float bestdot;
	Vector bestdir;
	edict_t *bestent;
	TraceResult tr;

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = FALSE;

	if(m_flash_mode >= 2)
		return Vector( 0, 0, 0 );

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr );

	if( tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO )
	{
		// don't look through water
		if( !( ( pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3 ) || ( pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0 ) ) )
		{
			if( tr.pHit->v.takedamage == DAMAGE_AIM && !FBitSet( tr.pHit->v.flags, FL_NOTARGET ) )
				m_fOnTarget = TRUE;

			return m_vecAutoAim;
		}
	}

	for( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		Vector center;
		Vector dir;
		float dot;

		if( pEdict->free )	// Not in use
			continue;

		if( pEdict->v.takedamage != DAMAGE_AIM )
			continue;
		if( pEdict == edict() )
			continue;
		//if( pev->team > 0 && pEdict->v.team == pev->team )
		//	continue;	// don't aim at teammate
		if( !g_pGameRules->ShouldAutoAim( this, pEdict ) )
			continue;

		pEntity = Instance( pEdict );
		if( pEntity == NULL )
			continue;

		if( !pEntity->IsAlive() )
			continue;

		// don't look through water
		if( ( pev->waterlevel != 3 && pEntity->pev->waterlevel == 3 ) || ( pev->waterlevel == 3 && pEntity->pev->waterlevel == 0 ) )
			continue;

		center = pEntity->BodyTarget( vecSrc );

		dir = ( center - vecSrc ).Normalize();

		// make sure it's in front of the player
		if( DotProduct( dir, gpGlobals->v_forward ) < 0 )
			continue;

		dot = fabs( DotProduct( dir, gpGlobals->v_right ) ) + fabs( DotProduct( dir, gpGlobals->v_up ) ) * 0.5f;

		// tweek for distance
		dot *= 1.0f + 0.2f * ( ( center - vecSrc ).Length() / flDist );

		if( dot > bestdot )
			continue;	// to far to turn

		UTIL_TraceLine( vecSrc, center, dont_ignore_monsters, edict(), &tr );
		if( tr.flFraction != 1.0f && tr.pHit != pEdict )
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if( IRelationship( pEntity ) < 0 )
		{
			if( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch() )
				// ALERT( at_console, "friend\n" );
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if( bestent )
	{
		bestdir = UTIL_VecToAngles( bestdir );
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if( bestent->v.takedamage == DAMAGE_AIM )
			m_fOnTarget = TRUE;

		return bestdir;
	}

	return Vector( 0, 0, 0 );
}

void CBasePlayer::ResetAutoaim()
{
	if( m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0 )
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		SET_CROSSHAIRANGLE( edict(), 0, 0 );
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer::SetCustomDecalFrames( int nFrames )
{
	if( nFrames > 0 && nFrames < 8 )
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames( void )
{
	return m_nCustomSprayFrames;
}

//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item. 
//=========================================================
void CBasePlayer::DropPlayerItem( char *pszItemName )
{
	/*if( !g_pGameRules->IsMultiplayer() || ( weaponstay.value > 0 ) )
	{
		// no dropping in single player.
		return;
	}*/

	if( pszItemName[0] == '\0' )
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	} 

	CBasePlayerItem *pWeapon;
	int i;

	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pWeapon = m_rgpPlayerItems[i];

		while( pWeapon )
		{
			if( pszItemName )
			{
				// try to match by name. 
				if( !strcmp( pszItemName, STRING( pWeapon->pev->classname ) ) )
				{
					// match! 
					break;
				}
			}
			else
			{
				// trying to drop active item
				if( pWeapon == m_pActiveItem )
				{
					// active item!
					break;
				}
			}

			pWeapon = pWeapon->m_pNext; 
		}

		// if we land here with a valid pWeapon pointer, that's because we found the 
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if( pWeapon )
		{
			//if( !g_pGameRules->GetNextBestWeapon( this, pWeapon ) )
				//return; // can't drop the item they asked for, may be our last item or something we can't holster

			g_pGameRules->GetNextBestWeapon( this, pWeapon );
			UTIL_MakeVectors( pev->angles ); 

			pev->weapons &= ~( 1 << pWeapon->m_iId );// take item off hud

			if(pWeapon->m_iId != WEAPON_FIREAXE){
				CWeaponBox *pWeaponBox = (CWeaponBox *)Create("weaponbox_drop", pev->origin + Vector(0,0,8) + gpGlobals->v_forward * 10, pev->angles, edict());
				pWeaponBox->pev->angles.x = 0;
				pWeaponBox->pev->angles.z = 0;
				pWeaponBox->SetThink(&CWeaponBox::Kill);
				
				pWeaponBox->pev->nextthink = gpGlobals->time + 10;

				pWeaponBox->PackWeapon(pWeapon);

				Vector angThrow = pev->v_angle + pev->punchangle;

				if (angThrow.x < 0)
					angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
				else
					angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

				float flVel = (90 - angThrow.x) * 6;

				if (flVel > 500)
					flVel = 500;

				UTIL_MakeVectors(angThrow);
				Vector vecSrc = pev->origin + pev->view_ofs + gpGlobals->v_forward * 16;
				Vector vecThrow = gpGlobals->v_forward * flVel + pev->velocity;

				pWeaponBox->pev->velocity = vecThrow;

				pWeaponBox->pev->avelocity.x = pev->velocity.Length();
				pWeaponBox->pev->avelocity.y = RANDOM_FLOAT( -pev->velocity.Length(), pev->velocity.Length() );

				
				// drop half of the ammo for this weapon.
				int	iAmmoIndex;

				iAmmoIndex = GetAmmoIndex ( pWeapon->pszAmmo1() ); // ???
				
				if ( iAmmoIndex != -1 )
				{
					// this weapon weapon uses ammo, so pack an appropriate amount.
					if ( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE )
					{
						// pack up all the ammo, this weapon is its own ammo type
						pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[ iAmmoIndex ] );
						m_rgAmmo[ iAmmoIndex ] = 0; 

					}
					else
					{
						// pack half of the ammo
						pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[ iAmmoIndex ] / 2 );
						m_rgAmmo[ iAmmoIndex ] /= 2; 
					}

				}
			}
			else
			{
				CWeaponBox *pWeaponBox = (CWeaponBox *)Create("weaponbox_drop", pev->origin + Vector(0,0,16384), pev->angles, edict());
				pWeaponBox->pev->angles.x = 0;
				pWeaponBox->pev->angles.z = 0;
				pWeaponBox->SetThink(&CWeaponBox::Kill);
				
				pWeaponBox->pev->nextthink = gpGlobals->time + 1;

				pWeaponBox->PackWeapon(pWeapon);
			}

			return;// we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while( pItem )
	{
		if( FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname ) ) )
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	CBasePlayerItem *pItem;
	int i;

	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pItem = m_rgpPlayerItems[i];

		while( pItem )
		{
			if( !strcmp( pszItemName, STRING( pItem->pev->classname ) ) )
			{
				return TRUE;
			}
			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
// HasPlayerItemFromID
//=========================================================
BOOL CBasePlayer::HasPlayerItemFromID( int nID )
{
	CBasePlayerItem *pItem;
	int i;

	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pItem = m_rgpPlayerItems[i];

		while( pItem )
		{
			if( nID == pItem->m_iId )
			{
				return TRUE;
			}
			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
// 
//=========================================================
BOOL CBasePlayer::SwitchWeapon( CBasePlayerItem *pWeapon ) 
{
	if( !pWeapon->CanDeploy() )
	{
		return FALSE;
	}
	
	ResetAutoaim();

	if( m_pActiveItem )
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pWeapon;

	pWeapon->pev->oldbuttons = 1;
	pWeapon->Deploy();
	pWeapon->pev->oldbuttons = 0;

	return TRUE;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn( void );
	int Classify( void )
	{
		return CLASS_HUMAN_MILITARY;
	}

	void KeyValue( KeyValueData *pkvd );

	int m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[4];
};

const char *CDeadHEV::m_szPoses[] =
{
	"deadback",
	"deadsitting",
	"deadstomach",
	"deadtable"
};

void CDeadHEV::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV )

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV::Spawn( void )
{
	PRECACHE_MODEL( "models/deadhaz.mdl" );
	SET_MODEL( ENT( pev ), "models/deadhaz.mdl" );

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead hevsuit with bad pose\n" );
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 20;

	MonsterInitDead();
}

class CStripWeapons : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons )

void CStripWeapons::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = NULL;

	if( pActivator && pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	if( pPlayer )
		pPlayer->RemoveAllItems( FALSE );
}

class CRevertSaved : public CPointEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MessageThink( void );
	void EXPORT LoadThink( void );
	void KeyValue( KeyValueData *pkvd );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration( void ) { return pev->dmg_take; }
	inline float HoldTime( void ) { return pev->dmg_save; }
	inline float MessageTime( void ) { return m_messageTime; }
	inline float LoadTime( void ) { return m_loadTime; }

	inline void SetDuration( float duration ) { pev->dmg_take = duration; }
	inline void SetHoldTime( float hold ) { pev->dmg_save = hold; }
	inline void SetMessageTime( float time ) { m_messageTime = time; }
	inline void SetLoadTime( float time ) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved )

TYPEDESCRIPTION	CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD( CRevertSaved, m_messageTime, FIELD_FLOAT ),	// These are not actual times, but durations, so save as floats
	DEFINE_FIELD( CRevertSaved, m_loadTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRevertSaved, CPointEntity )

void CRevertSaved::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		SetHoldTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "messagetime" ) )
	{
		SetMessageTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "loadtime" ) )
	{
		SetLoadTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CRevertSaved::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, FFADE_OUT );
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink( &CRevertSaved::MessageThink );
}

void CRevertSaved::MessageThink( void )
{
	UTIL_ShowMessageAll( STRING( pev->message ) );
	float nextThink = LoadTime() - MessageTime();
	if( nextThink > 0 ) 
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink( &CRevertSaved::LoadThink );
	}
	else
		LoadThink();
}

void CRevertSaved::LoadThink( void )
{
	if( !gpGlobals->deathmatch )
	{
		SERVER_COMMAND( "reload\n" );
	}
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission:public CPointEntity
{
	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2.0f;// let targets spawn!
}

void CInfoIntermission::Think( void )
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );

	if( !FNullEnt( pTarget ) )
	{
		pev->v_angle = UTIL_VecToAngles( ( pTarget->v.origin - pev->origin ).Normalize() );
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission )

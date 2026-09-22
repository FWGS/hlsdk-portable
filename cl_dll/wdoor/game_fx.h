#ifndef CL_FX_H
#define CL_FX_H
#include "r_efx.h"

void HookFXMessages(void);

void EV_EjectBrass( float *origin, float rotation, int soundtype, int body, int iLife = gHUD.TempEntLifeCvar->value );
void EV_EjectBrassCallback( struct tempent_s *ent, float frametime, float currenttime );
void EV_ClipTouch ( struct tempent_s *ent, struct pmtrace_s *ptr );
void EV_DynLight( float *origin, float MinRadius, float MaxRadius, float r, float g, float b, float life );
void EV_BlastModel( float *origin, int body, float StartScale, float scaleADD, float renderamtADD );
void EV_BlastModelCallback ( struct tempent_s *ent, float frametime, float currenttime );

qboolean EV_IsLocal( int idx );

int __MsgFunc_SetBody(const char *pszName, int iSize, void *pbuf);
int __MsgFunc_SetSkin(const char *pszName, int iSize, void *pbuf);
int __MsgFunc_Concuss(const char *pszName, int iSize, void *pbuf);

inline struct dlight_s *DynamicLight(const Vector &vecPos, float radius, byte r, byte g, byte b, float life, float decay)
{
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);\
	if (!dl) return NULL;\
	VectorCopy(vecPos, dl->origin);\
	dl->radius = radius;\
	dl->color.r = r;\
	dl->color.g = g;\
	dl->color.b = b;\
	dl->decay = decay;\
	dl->die = gEngfuncs.GetClientTime() + life;\
	return dl;\
}


//===========================================
// Bullet types. This used to differ 
// bullet-impact effects ONLY! (no DMG, etc.)
//===========================================
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_57mm, // g11
	BULLET_9mmP, // 9 mm Parabellum
	BULLET_8mm, // uzi
	BULLET_12G, // shieldgun
	BULLET_762Nato, //M16 
	BULLET_556Nato, //M249 
	BULLET_556, //u2 
	BULLET_762, //AK74 
	BULLET_762x54, //svd 
	BULLET_338Magnum, //AWP 
	BULLET_86mm, //minigun 
	BULLET_32mm, //machinegun 
	BULLET_MP5, // mp5
	BULLET_357, // python
	BULLET_BUCKSHOT, // shotgun
	BULLET_10MMBUCKSHOT, // automatic shotgun
	BULLET_BFG, // BFG
	BULLET_50AE, // Desert Eagle
	BULLET_45ACP, // usp
	BULLET_30mm, // 30mmsg
	BULLET_9MM,
	BULLET_12MM,
	BULLET_14MM,
	BULLET_CROWBAR,
	BULLET_KNIFE,
	BULLET_CROWBARQUAD,
	BULLET_KNIFEQUAD,
	BULLET_BANDSAW,
	BULLET_NAILGUN,
	BULLET_2MM,
	BULLET_2MM_QUAD,
	BULLET_127MM,
	BULLET_BOLT,
	BULLET_SMALEXP,
	BULLET_NORMEXP,
	BULLET_HIGHEXP,
	BULLET_MEGAEXP
} Bullet;

//func_breakable gibs
#define	SHARD_GLASS		0
#define	SHARD_WOOD		1
#define	SHARD_METALL		2
#define	SHARD_FLESH		3
#define	SHARD_CONCRETE_BLOCK	4
#define	SHARD_CEILING_TILE	5
#define	SHARD_COMPUTER		6
#define	SHARD_UNBR_GLASS	7
#define	SHARD_ROCK		8
#define	SHARD_GRATE		9
#define	SHARD_VENT		10	
#define	SHARD_BRICK		11		
#define	SHARD_CONCRETE		12		
#define	SHARD_ICE		13		
#define	SHARD_SANDWALL		14		


//Projectile trail and explosion effects
#define PROJ_REMOVE			0
#define PROJ_FLAME			1
#define PROJ_FLAME_DETONATE 		2
#define PROJ_FLAME_DETONATE_WATER	3
#define PROJ_ICE			4
#define PROJ_ICE_DETONATE 		5
#define PROJ_ICE_DETONATE_WATER		6
#define PROJ_NERVEGREN			7
#define PROJ_NERVEGREN_DETONATE 	8
#define PROJ_INCENDIARY2		9
#define PROJ_30MMGREN			10
#define PROJ_30MMGREN_DETONATE	 	11
#define PROJ_30MMGREN_DETONATE_WATER	12
#define PROJ_AK74			13
#define PROJ_AK74_DETONATE 		14
#define PROJ_AK74_DETONATE_WATER	15
#define PROJ_M203			16
#define PROJ_M203_DETONATE 		17
#define PROJ_M203_DETONATE_WATER	18
#define PROJ_MMISSILE			19
#define PROJ_MMISSILE_DETONATE 		20
#define PROJ_MMISSILE_DETONATE_WATER	21
#define PROJ_WARHEAD			22
#define PROJ_WARHEAD_DETONATE 		23
#define PROJ_WARHEAD_DETONATE_WATER	24
#define PROJ_DUMBFIRE			25
#define PROJ_DUMBFIRE_DETONATE 		26
#define PROJ_DUMBFIRE_DETONATE_WATER	27
#define PROJ_NUKE			28
#define PROJ_NUKE_DETONATE	 	29
#define PROJ_NUKE_DETONATE_WATER	30
#define PROJ_U2				31
#define PROJ_U2_DETONATE 		32
#define PROJ_U2_DETONATE_WATER		33
#define PROJ_U2_DETONATE_SHARDS		34
#define PROJ_U2_SHARD			35
#define PROJ_U2_SHARD_DETONATE 		36
#define PROJ_U2_SHARD_DETONATE_WATER	37
#define PROJ_FLAKBOMB			38
#define PROJ_FLAKBOMB_DETONATE 		39
#define PROJ_FLAKBOMB_DETONATE_WATER	40
#define PROJ_RPGROCKET			41
#define PROJ_RPGROCKET_DETONATE 	42
#define PROJ_RPGROCKET_DETONATE_WATER	43
#define PROJ_INCENDIARY			44
#define PROJ_INCENDIARY_DETONATE 	45
#define PROJ_INCENDIARY_DETONATE_WATER	46
#define PROJ_TESLAGREN			47
#define PROJ_TESLAGREN_DETONATE 	48
#define PROJ_TESLAGREN_DETONATE_WATER	49
#define PROJ_CLUSTERBOMB		50
#define PROJ_CLUSTERBOMB_DETONATE 	51
#define PROJ_CLUSTERBOMB_DETONATE_WATER	52
#define PROJ_CLUSTERBABY_DETONATE 	53
#define PROJ_CLUSTERBABY_DETONATE_WATER	54
#define PROJ_GLUON			55
#define PROJ_GLUON_DETONATE 		56
#define PROJ_GLUON_DETONATE_WATER	57
#define PROJ_GLUON2			58
#define PROJ_GLUON2_DETONATE 		59
#define PROJ_PLASMA	 		60
#define PROJ_DISPLACER			61
#define PROJ_DISPLACER_DETONATE 	62
#define PROJ_DISPLACER_DETONATE_WATER	63
#define PROJ_SHOCK			64
#define PROJ_SHOCK_DETONATE 		65
#define PROJ_SHOCK_DETONATE_WATER	66
#define PROJ_SUNOFGOD			67
#define PROJ_SUNOFGOD_DETONATE		68
#define PROJ_SUNOFGOD_DETONATE_WATER	69
#define PROJ_BLACKHOLE			70
#define PROJ_BLACKHOLE_DETONATE		71
#define PROJ_TELEENTER			72
#define PROJ_TELEENTER_DETONATE 	73
#define PROJ_TELEENTER_DETONATE_WATER	74
#define PROJ_DISPPOWER_DETONATE 	75
#define PROJ_SHOCKPOWER_DETONATE 	76
#define PROJ_GUTS			77
#define PROJ_GUTS_DETONATE		78
#define PROJ_ENERGYCHARGE_DETONATE 	79
#define PROJ_ENERGYCHARGE_DETONATE_WATER	80
#define EFFECT_BURN			81
#define EFFECT_FREEZE			82
#define PROJ_SHOCK2				83
#define PROJ_BLACKBALL			88
#define PROJ_SUNOFGOD2			134
#define PROJ_NERVEBLUE			135
#define PROJ_RPGROCKET2			136
#define PROJ_STONEBEAM			137
#define PROJ_STONESHOOT			138
#define PROJ_BLACKHOLE_DETONATE2		139
#define PROJ_HYDRA_SHOCKEFF				140
#define PROJ_DOMABEAM					141
#define PROJ_DOMABEAM_FIRE				142
#define PROJ_GMANBEAM					143
#define PROJ_GMANBEAM_FIRE				144
#define PROJ_GMAN_BURNFIRE				145
#define PROJ_DOMA_CLAW_HIT				146
#define PROJ_DOMA_FIRE_BALL				147
#define PROJ_DOMABEAM2					148
#define PROJ_DOMABEAM2_FIRE				149
#define PROJ_DOMABEAM2_HIT				150
#define PROJ_KADOMA_FISTHIT				151
#define PROJ_GOD_SWORD1					152
#define PROJ_GOD_SWORD2					153
#define PROJ_GOD_BALL1					154
#define PROJ_GOD_BALL2					155
#define PROJ_KADOMA_FLY					156

// Different bodyes for gun clips
#define CLIP_GLOCK		0
#define CLIP_USP		1
#define CLIP_DEAGLE		2
#define CLIP_UZI		3
#define CLIP_SHIELDGUN		4
#define CLIP_MP5		5
#define CLIP_M16		6
#define CLIP_AKIMBOGUN_SG552	7
#define CLIP_AKIMBOGUN_AUG	8
#define CLIP_AK74		9
#define CLIP_CROSSBOW		10
#define CLIP_G11		11
#define CLIP_U2			12
#define CLIP_SVD		13
#define CLIP_AWP		14
#define CLIP_BARETT		15
#define CLIP_M249		16
#define CLIP_MINIGUN		17
#define CLIP_NAILGUN		18
#define CLIP_FLAMETHROWER	19
#define CLIP_FROSTER		20
#define CLIP_FLAKCANNON		21
#define CLIP_BFG		22
#define CLIP_INCENDIARY		23
#define CLIP_TESLAGUN		24
#define CLIP_EGON		25
#define CLIP_PLASMARIFLE	26
#define CLIP_PHOTONGUN		27
#define CLIP_GAUSS		28
#define CLIP_TAUCANNON		29
#define CLIP_GLUONGUN		30
#define CLIP_DISPLACER		31
#define CLIP_BIORIFLE		32
#define CLIP_PULSERIFLE		33
#define CLIP_M72		34
#define CLIP_CHRONOSCEPTOR	35
#define CLIP_MACHINEGUN		36

#define CLIP_GLOCK_DUAL		37
#define CLIP_UZI_RIGHT		38
#define CLIP_UZI_LEFT		39
#define CLIP_MACHINEGUN_LEFT	40
#define CLIP_NAILGUN_LEFT	41
#define CLIP_EGON_LEFT		42
#define CLIP_EGON_MIDDLE	43
#define CLIP_CHRONOSCEPTOR_LEFT 44
#define CLIP_GLUONGUN_LEFT	45

//Weapon fire effects
#define FIREGUN_REMOVE		0
#define FIREGUN_GLOCK		1
#define FIREGUN_GLOCKAKIMBO	2
#define FIREGUN_USP		3
#define FIREGUN_DEAGLE		4
#define FIREGUN_PYTHON		5
#define FIREGUN_UZI		6
#define FIREGUN_UZIAKIMBO	7
#define FIREGUN_SHIELDGUN	8
#define FIREGUN_SHOTGUN		9
#define FIREGUN_SHOTGUN_BR_SHELL 10
#define FIREGUN_AUTOSHOTGUN	11
#define FIREGUN_30MMSG		12
#define FIREGUN_MP5		13
#define FIREGUN_M16		14
#define FIREGUN_AKIMBOGUN_LEFT	15
#define FIREGUN_AKIMBOGUN_RIGHT	16
#define FIREGUN_AKIMBOGUN_BOTH	17
#define FIREGUN_AK74		18
#define FIREGUN_G11		19
#define FIREGUN_U2		20
#define FIREGUN_SVD		21
#define FIREGUN_AWP		22
#define FIREGUN_TORCH		23
#define FIREGUN_M249		24
#define FIREGUN_MINIGUN		25
#define FIREGUN_BFG		26
#define FIREGUN_BARETT		27
#define FIREGUN_MACHINEGUN	28
#define FIREGUN_FLAKCANNON	29
#define FIREGUN_PLASMARIFLE	30
#define FIREGUN_FROSTER		31
#define FIREGUN_EPISTOL		32
#define FIREGUN_DEVASTATOR	33
#define FIREGUN_RPG		34
#define FIREGUN_GLUONGUN	35
#define FIREGUN_DISPLACER	36
#define FIREGUN_SMARTGUN	37
#define FIREGUN_INCENDIARY	38
#define FIREGUN_IONTURRET	39
#define FIREGUN_M72		40
#define FIREGUN_BANDSAW		41
#define FIREGUN_CROSSBOW	42
#define FIREGUN_NAILGUN		43
#define FIREGUN_TESLAGUN	44
#define FIREGUN_BLASTER		45
#define FIREGUN_GAUSS		46
#define FIREGUN_PHOTONGUN	47
#define FIREGUN_PHOTONGUN_EXP	48
#define FIREGUN_PULSERIFLE	49
#define FIREGUN_TAUCANNON	50
#define FIREGUN_TURRETKIT	51
#define FIREGUN_CHRONOSCEPTOR	52
#define FIREGUN_LIGHTSABER	53
#define FIREGUN_FTHROWER	54
#define FIREGUN_BFG_SEC		55
#define FIREGUN_EGON		56
#define FIREGUN_AIRGUN		57

//Impact surfaces
#define SURFACE_NONE		0
#define SURFACE_WORLDBRUSH	1
#define SURFACE_BREAKABLE	2
#define SURFACE_ARMOR		3
#define SURFACE_ENERGYARMOR	4
#define SURFACE_FLESH		5
#define SURFACE_FLESH_YELLOW		6
#define SURFACE_FLESH_LEVEL2			7
#define SURFACE_FLESH_YELLOW_LEVEL2		8
#define SURFACE_FLESH_LEVEL3			9
#define SURFACE_FLESH_YELLOW_LEVEL3		10

//Beam impact effects
#define IMPBEAM_BLASTER		1
#define IMPBEAM_GAUSS		2
#define IMPBEAM_GAUSSCHARGED	3
#define IMPBEAM_PHOTONGUN	4
#define IMPBEAM_TAUCANNON	5
#define IMPBEAM_IONTURRET	6
#define IMPBEAM_BLASTERBEAM	7
#define IMPBEAM_EGON		8
#define IMPBEAM_PLASMABALL	9
#define IMPBEAM_PBOLT		10
#define IMPBEAM_EGON2		11
#define IMPBEAM_TAUCANNON2	12
#define IMPBEAM_HELLVORTB	13

//Weapon beam effects
#define BEAM_BLASTER		1
#define BEAM_BLASTER_EXIT	2
#define BEAM_GAUSS		3
#define BEAM_GAUSS_EXIT		4
#define BEAM_GAUSSCHARGED	5
#define BEAM_GAUSSCHARGED_EXIT	6
#define BEAM_PHOTONGUN		7
#define BEAM_PHOTONGUN_EXIT	8
#define BEAM_TAUCANNON		9
#define BEAM_TAUCANNON_EXIT	10
#define BEAM_IONTURRET		11
#define BEAM_IONTURRET_EXIT	12
#define BEAM_PHOTONGUN_EXP	13
#define BEAM_TESLAGUN		14
#define BEAM_PULSERIFLE		15
#define BEAM_PSP		16
#define BEAM_LGTNGBALL		17
#define BEAM_PLASMABALL		18
#define BEAM_BLOODSTREAM	19
#define BEAM_STONEBEAM		20
#define BEAM_NOBITABEAM		21
#define BEAM_TAUCANNON2		22
#define BEAM_ZDEADEYE		23
#define BEAM_DOMABOSS		25
#define BEAM_BLOODSTREAM233	 233 
                        
// Projectile explosion effects
#define EXPLOSION_C4		1
#define EXPLOSION_BIOMASS	2 
#define EXPLOSION_TURRET	3
#define EXPLOSION_PSHIELD	4 
#define EXPLOSION_TRIPMINE	5 
#define EXPLOSION_GRENADE	6 
#define EXPLOSION_FLASHBANG	7 
#define EXPLOSION_CHRONOCLIP	8 
#define EXPLOSION_BOLT		9
#define EXPLOSION_SATCHEL	10
#define EXPLOSION_TANKPROJ	11
#define EXPLOSION_SPARKSHOWER	12
#define EXPLOSION_SHIELDIMPACT	13
#define EXPLOSION_ARMORIMPACT	14
#define EXPLOSION_BUBBLES	15
#define EXPLOSION_WHITESMOKE	16
#define EXPLOSION_BIOMASSIMPACT	17 
#define EXPLOSION_TANK		18
#define EXPLOSION_BLOODDRIPS	19 
#define EXPLOSION_SATELLITE	20
#define EXPLOSION_MORTAR	21
#define EXPLOSION_TORCH		22
#define EXPLOSION_PTELEPORT	23
#define EXPLOSION_DISPTELEPORT	24
#define EXPLOSION_PLASMABALL2	25
#define EXPLOSION_PLASMABALL2_SPARKS	26
#define EXPLOSION_ENERGY_INWATER_S	27
#define EXPLOSION_ENERGY_INWATER_M	28
#define EXPLOSION_ENERGY_INWATER_L	29
#define EXPLOSION_PBOLT			30
#define EXPLOSION_WHL_SHARD		31
#define EXPLOSION_SPARKSHOWER_SND	32
#define EXPLOSION_LIGHTSABER		33
#define EXPLOSION_LAVA			34
#define EXPLOSION_LAVA_FLAME		35
#define EXPLOSION_CLUSTERBABY		36
#define EXPLOSION_MEDKIT		37
#define EXPLOSION_HEVCHARGER		38
#define EXPLOSION_BEAM_FLESHIMPACT	39
#define EXPLOSION_TURRET_SPAWN		40
#define EXPLOSION_MACHINEGUN		41
#define EXPLOSION_YELOOW			42
#define EXPLOSION_WIND_EXP			43
#define EXPLOSION_HOLY_STAR			44
#define EXPLOSION_HEAL_HOLY			45
#define EXPLOSION_WILLAM_SLASH		46
#define EXPLOSION_DRAGON_SLASH		47
#define EXPLOSION_SLEEP_Z			48
#define EXPLOSION_LIGHTARROW_EXP	49
#define EXPLOSION_SNAKER_EXP		50
#define EXPLOSION_DRAGON_SLASH2		51
#define EXPLOSION_DEFINE_SHIELD		52
#define EXPLOSION_BLOODDRIPS233	    233

//BODY GIBBAGE
#define GIBBED_BODY	1
#define GIBBED_HEAD	2
#define GIBBED_FROZEN	3
#define GIBBED_IGNITE	4
#define GIBBED_ELECTRO	5

//====================//
//CLIENT SIDED ONLY!!!//
//====================//

// Different bodyes for explosion effects, Client-Sided ONLY
#define BLAST_MDL_MUSHROOM	1
#define BLAST_MDL_SPHERE	2
#define BLAST_MDL_HALFSPHERE	3
#define BLAST_MDL_CONE		4

// Different bodyes for gun shells, Client-Sided ONLY
#define SHELL_PISTOL_9MM	0
#define SHELL_PISTOL_8MM	1
#define SHELL_PISTOL_45ACP	2
#define SHELL_PISTOL_50AE	3
#define SHELL_PISTOL_357	4
#define SHELL_RIFLE_556NATO	5
#define SHELL_RIFLE_762NATO	6
#define SHELL_RIFLE_87MM	7
#define SHELL_RIFLE_762MAGNUM	8
#define SHELL_RIFLE_127MM	9
#define SHELL_SHOTGUN_10MM	10
#define SHELL_SHOTGUN_RED	11
#define SHELL_BFG		12
#define SHELL_RIFLE_338MAGNUM	13
#define SHELL_RIFLE_556		14
#define SHELL_RIFLE_762		15
#define SHELL_PISTOL_12G	16
#define SHELL_RIFLE_9MM		17
#define SHELL_RIFLE_762x54	18
#define SHELL_RIFLE_32MM	19

// Different skins for gibs, Client-Sided ONLY
#define GIB_BRICK	0
#define GIB_ROCK	1
#define GIB_ASPHALT	2
#define GIB_FLESH	3
#define GIB_CONCRETE	4
#define GIB_TILE	5
#define GIB_SNOW	6
#define GIB_GRASS	7
#define GIB_DIRT	8
#define GIB_SAND	9
#define GIB_SANDWALL	10
#define GIB_SNOWROCK	11
#define GIB_LEAVES	12
#define GIB_GLASS	13//Not a skin, just submodels
#define GIB_WOOD	14
#define GIB_METALL	15
#define GIB_GRATE	16

//Gun smoke effects, Client-sided ONLY
#define GUNSMOKE_WHITE_SMALLEST	1
#define GUNSMOKE_WHITE_SMALL	2
#define GUNSMOKE_WHITE_MEDIUM	3
#define GUNSMOKE_WHITE_LARGE	4
#define GUNSMOKE_BLACK_SMALLEST	5
#define GUNSMOKE_BLACK_SMALL	6
#define GUNSMOKE_BLACK_MEDIUM	7
#define GUNSMOKE_BLACK_LARGE	8

//Spark shower effects, Client-sided ONLY
#define SPARKSHOWER_SPARKS	0
#define SPARKSHOWER_SPARKS2	1
#define SPARKSHOWER_EXP		2
#define SPARKSHOWER_SPARKSMOKE	3
#define SPARKSHOWER_STREAKS	4
#define SPARKSHOWER_FLICKER	5
#define SPARKSHOWER_SMOKE	6
#define SPARKSHOWER_FIRESMOKE	7
#define SPARKSHOWER_FIREEXP	8
#define SPARKSHOWER_ENERGY	9
#define SPARKSHOWER_BLOODDRIPS	10
#define SPARKSHOWER_LAVA_FLAME	11

//=====================================//
//particle sprite-frames settings start//
//=====================================//
//rings_all.spr, 12 frames 
#define BLAST_SKIN_DISPLACER	0
#define BLAST_SKIN_C4		1
#define BLAST_SKIN_TELEENTER	2
#define BLAST_SKIN_ENERGYBOLT	3
#define BLAST_SKIN_GLUON	4
#define BLAST_SKIN_FROSTGRENADE	5
#define BLAST_SKIN_LIGHTNING	6
#define BLAST_SKIN_ENERGYBEAM	7
#define BLAST_SKIN_TAUBEAM	8
#define BLAST_SKIN_FIREBEAM	9
#define BLAST_SKIN_PSPBEAM	10
#define BLAST_SKIN_GAUSSBEAM	11
#define BLAST_SKIN_SHOCKWAVE	12
#define BLAST_SKIN_PLASMA	13
#define BLAST_SKIN_STEAM	14
#define BLAST_SKIN_PULSE	15
#define BLAST_SKIN_WASTEDBEAM	16

//particles_white.spr, 16 frames
#define PARTICLE_WHITE_0	0
#define PARTICLE_WHITE_1	1
#define PARTICLE_WHITE_2	2
#define PARTICLE_WHITE_3	3
#define PARTICLE_WHITE_4	4
#define PARTICLE_WHITE_5	5
#define PARTICLE_WHITE_6	6
#define PARTICLE_WHITE_7	7
#define PARTICLE_WHITE_8	8
#define PARTICLE_WHITE_9	9
#define PARTICLE_WHITE_10	10//rain drop
#define PARTICLE_WHITE_11	11//bubble
#define PARTICLE_WHITE_12	12//water ring
#define PARTICLE_WHITE_13	13//water ring 2
#define PARTICLE_WHITE_14	14//shock ring

//particles_black.spr, 14 frames
#define PARTICLE_BLACK_0	0
#define PARTICLE_BLACK_1	1
#define PARTICLE_BLACK_2	2
#define PARTICLE_BLACK_3	3
#define PARTICLE_BLACK_4	4
#define PARTICLE_BLACK_5	5//snowflake
#define PARTICLE_BLACK_6	6
#define PARTICLE_BLACK_7	7
#define PARTICLE_BLACK_8	8
#define PARTICLE_BLACK_9	9//blackhole
#define PARTICLE_BLACK_10	10//water ring
#define PARTICLE_BLACK_11	11//water ring2
#define PARTICLE_BLACK_12	12//shock ring

//particles_red.spr, 12 frames
#define PARTICLE_RED_0		0//blaster muzzleflash
#define PARTICLE_RED_1		1//lasgun muzzleflash
#define PARTICLE_RED_2		2//gauss sec. muzzleflash
#define PARTICLE_RED_3		3//epistol muzzleflash
#define PARTICLE_RED_4		4//m72 pri. muzzleflash
#define PARTICLE_RED_5		5//gauss pri. muzzleflash
#define PARTICLE_RED_6		6//pulserifle muzzleflash
#define PARTICLE_RED_7		7
#define PARTICLE_RED_8		8
#define PARTICLE_RED_9		9
#define PARTICLE_RED_10		10
#define PARTICLE_RED_11		11
#define PARTICLE_RED_12		12//hotglow
#define PARTICLE_RED_13		13//3-x laserdot
#define PARTICLE_RED_14		14//laserdot
#define PARTICLE_RED_15		15//double laserdot
//new
#define PARTICLE_RED_16		16
#define PARTICLE_RED_17		17
#define PARTICLE_RED_18		18
#define PARTICLE_RED_19		19
#define PARTICLE_RED_20		20
#define PARTICLE_RED_21		21
#define PARTICLE_RED_22		22
#define PARTICLE_RED_23		23
#define PARTICLE_RED_24		24
#define PARTICLE_RED_25		25
#define PARTICLE_RED_26		26
#define PARTICLE_RED_27		27
#define PARTICLE_RED_28		28
#define PARTICLE_RED_29		29
#define PARTICLE_RED_30		30
#define PARTICLE_RED_31		31
#define PARTICLE_RED_32		32
#define PARTICLE_RED_33		33
#define PARTICLE_RED_34		34
#define PARTICLE_RED_35		35
#define PARTICLE_RED_36		36
#define PARTICLE_RED_37		37
#define PARTICLE_RED_38		38
#define PARTICLE_RED_39		39
#define PARTICLE_RED_40		40

//particles_green.spr, 9 frames
#define PARTICLE_GREEN_0	0//plasmarifle pri. muzzleflash
#define PARTICLE_GREEN_1	1
#define PARTICLE_GREEN_2	2 
#define PARTICLE_GREEN_3	3//m72 sec. muzzleflash
#define PARTICLE_GREEN_4	4//plasmarifle sec. muzzleflash 
#define PARTICLE_GREEN_5	5//m72 quad trail
#define PARTICLE_GREEN_6	6//plasma impact
#define PARTICLE_GREEN_7	7
#define PARTICLE_GREEN_8	8

//particles_blue.spr, 5 frames
#define PARTICLE_BLUE_0		0//froster muzzleflash
#define PARTICLE_BLUE_1		1//smartgun muzzleflash
#define PARTICLE_BLUE_2		2//photongun muzzleflash 
#define PARTICLE_BLUE_3		3//egon muzzleflash
#define PARTICLE_BLUE_4		4//gluongun muzzleflash
#define PARTICLE_BLUE_5		5
#define PARTICLE_BLUE_6		6

//particles_violet.spr, 5 frames
#define PARTICLE_VIOLET_0		0//ion turret
#define PARTICLE_VIOLET_1		1//taucannon muzzleflash
#define PARTICLE_VIOLET_2		2
#define PARTICLE_VIOLET_3		3//teslagun muzzleflash
#define PARTICLE_VIOLET_4		4

//particles_blood.spr, 13 frames
#define PARTICLE_BLOOD_0		0
#define PARTICLE_BLOOD_1		1
#define PARTICLE_BLOOD_2		2
#define PARTICLE_BLOOD_3		3
#define PARTICLE_BLOOD_4		4
#define PARTICLE_BLOOD_5		5
#define PARTICLE_BLOOD_6		6
#define PARTICLE_BLOOD_7		7
#define PARTICLE_BLOOD_8		8
#define PARTICLE_BLOOD_9		9
#define PARTICLE_BLOOD_10		10
#define PARTICLE_BLOOD_11		11
#define PARTICLE_BLOOD_12		12

//particles_gibs.spr, 40 frames
#define PARTICLE_CONCRETE_0	0
#define PARTICLE_CONCRETE_1	1
#define PARTICLE_CONCRETE_2	2
#define PARTICLE_CONCRETE_3	3
#define PARTICLE_CONCRETE_4	4
#define PARTICLE_CONCRETE_5	5
#define PARTICLE_BRICK_0	6
#define PARTICLE_BRICK_1	7
#define PARTICLE_BRICK_2	8
#define PARTICLE_BRICK_3	9
#define PARTICLE_BRICK_4	10
#define PARTICLE_BRICK_5	11
#define PARTICLE_WOOD_0		12
#define PARTICLE_WOOD_1		13
#define PARTICLE_WOOD_2		14
#define PARTICLE_WOOD_3		15
#define PARTICLE_WOOD_4		16
#define PARTICLE_WOOD_5		17
#define PARTICLE_LEAVES_0	18
#define PARTICLE_LEAVES_1	19
#define PARTICLE_LEAVES_2	20
#define PARTICLE_LEAVES_3	21
#define PARTICLE_LEAVES_4	22
#define PARTICLE_LEAVES_5	23
#define PARTICLE_LEAVES_6	24
#define PARTICLE_LEAVES_7	25
#define PARTICLE_GRASS_0	26
#define PARTICLE_GRASS_1	27
#define PARTICLE_GRASS_2	28
#define PARTICLE_GRASS_3	29
#define PARTICLE_ASPHALT_0	30
#define PARTICLE_ASPHALT_1	31
#define PARTICLE_ASPHALT_2	32
#define PARTICLE_ASPHALT_3	33
#define PARTICLE_ASPHALT_4	34
#define PARTICLE_ASPHALT_5	35
#define PARTICLE_SAND_0		36
#define PARTICLE_SAND_1		37
#define PARTICLE_SAND_2		38
#define PARTICLE_SAND_3		39
#define PARTICLE_SAND_4		40
#define PARTICLE_SAND_5		41
#define PARTICLE_GLASS_0	42
#define PARTICLE_GLASS_1	43
#define PARTICLE_GLASS_2	44
#define PARTICLE_GLASS_3	45
#define PARTICLE_GLASS_4	46
#define PARTICLE_GLASS_5	47
#define PARTICLE_ICE_0		48
#define PARTICLE_ICE_1		49
#define PARTICLE_ICE_2		50
#define PARTICLE_ICE_3		51
#define PARTICLE_ICE_4		52
#define PARTICLE_ICE_5		53
#define PARTICLE_TILE_0		54
#define PARTICLE_TILE_1		55
#define PARTICLE_TILE_2		56
#define PARTICLE_TILE_3		57
#define PARTICLE_TILE_4		58
#define PARTICLE_TILE_5		59

#endif // CL_FX_H
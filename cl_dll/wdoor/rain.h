#ifndef __RAIN_H__
#define __RAIN_H__

#define DRIPSPEED	900		// скорость падения капель (пикс в сек)
#define SNOWSPEED	200		// скорость падения снежинок
#define SNOWFADEDIST	80

#define MAXDRIPS	80000	// лимит капель (можно увеличить при необходимости)
#define MAXFX		20000	// лимит дополнительных частиц (круги по воде и т.п.)

#define DRIP_SPRITE_HALFHEIGHT	64
#define DRIP_SPRITE_HALFWIDTH	64
#define SNOW_SPRITE_HALFSIZE	12

// "радиус" круга на воде, до которого он разрастается за секунду
#define MAXRINGHALFSIZE		25

typedef struct
{
	int		dripsPerSecond;
	float		distFromPlayer;
	float		windX, windY;
	float		randX, randY;
	int		weatherMode;	// 0 - snow, 1 - rain
	float		globalHeight;
} rain_properties;

typedef struct cl_drip
{
	float		birthTime;
	float		minHeight;
	vec3_t		origin;
	float		alpha;

	float		xDelta;		// side speed
	float		yDelta;
	int			landInWater;
	
	cl_drip*		p_Next;		// next drip in chain
	cl_drip*		p_Prev;		// previous drip in chain
} cl_drip_t;

typedef struct cl_rainfx
{
	float		birthTime;
	float		life;
	vec3_t		origin;
	float		alpha;
	
	cl_rainfx*		p_Next;		// next fx in chain
	cl_rainfx*		p_Prev;		// previous fx in chain
} cl_rainfx_t;

void ProcessRain( void );
void ProcessFXObjects( void );
void ResetRain( void );
void InitRain( void );

#endif


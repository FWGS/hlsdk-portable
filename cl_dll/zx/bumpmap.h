//
// Bump Mapping in Half-Life
//
// Original code copyright (C) Francis "DeathWish" Woodhouse, 2004.
//
// You are free to use this code in your own projects, so long as
//   (a) The original author of this code is credited and
//   (b) This header remains intact on both of the main source files
//
// I'd be interested to know if you do decide to use this in something, so I'd
// appreciate it if you drop me a line at deathwish@valve-erc.com if you do.
//

#ifndef _INC_BUMPMAP_H
#define _INC_BUMPMAP_H

#pragma warning(disable : 4786) // identifier truncated to 255 characters in browser information

#include <cg/cg.h>
#include <vector>
#include <map>

// We define a huge base number to which we add our texture IDs, since HL doesn't bother with glGenTextures so this stops
// us getting mangled up with HL's textures.
#define GL_TEXTURE_NUM_BASE (1<<25)

// This is the width and height of each side of the normalisation cube map. Total memory taken up by the NCM is
// 6*3*BUMP_NORM_CUBE_MAP_SIZE*BUMP_NORM_CUBE_MAP_SIZE bytes (which in this case = 1.1 MB - ouch!)
#define BUMP_NORM_CUBE_MAP_SIZE 256

// The width, height and depth of the 3D texture used to look up attenuation. Total memory taken up by this is
// BUMP_ATTENUATION_MAP_SIZE*BUMP_ATTENUATION_MAP_SIZE*BUMP_ATTENUATION_MAP_SIZE bytes
// (in this case, = 2 MB - bigger ouch! thank goodness it's only greyscale 8bpp, not RGB 24bpp)
#define BUMP_ATTENUATION_MAP_SIZE 128

struct bumplight_t
{
	bool enabled; // does exactly what it says on the tin
	char targetname[64]; // targetname of the light, used to reference it if need be
	Vector pos; // position in world space
	Vector colour; // colour - should be in range [0,1], but you can try higher if you want weird per-channel intensity modulation
	float strength; // final colour = (L dot N) * colour * strength (colour is component-wise)
	float radius; // distance divided by radius to give look-up in attenuation texture
	int style; // index into the array of styles in bumpmap.cpp

	// MOVEWITH STUFF
	Vector origPos; // the original position of this light
	cl_entity_t* moveWithEnt; // entity we're moving with
	Vector entOrigPos; // entity's original position
	Vector entOrigAngles; // entity's original angles
};

typedef std::vector<bumplight_t> BumpSceneList;
typedef BumpSceneList::iterator BumpSceneList_It;

typedef std::map<unsigned int, unsigned int> TexNormalMaps;
typedef TexNormalMaps::iterator TexNormalMaps_It;

class CBumpmapMgr
{
public:
	// DATA COLLECTIONS
	BumpSceneList m_aBumpedScenes;
	TexNormalMaps m_aNormalMaps;

	// OPENGL TEXTURE HANDLES
	unsigned int m_uiDiffuseScene;
	unsigned int m_uiLightmapScene;
	unsigned int m_uiBumpScene;
	unsigned int m_uiNormCubeMap;
	unsigned int m_uiAttenuationMap;
	unsigned int m_uiFlatSurfaceNorm;

	// CG VARIABLES
	CGcontext m_cg_Context;
	CGprofile m_cg_vertProfile;
	CGprofile m_cg_fragProfile;

	// CG FRAGMENT PROGRAMS
	CGprogram m_fp_Diffuse;
	CGprogram m_fp_Lightmaps;
	CGprogram m_fp_Bump;
	CGprogram m_fp_ModelPass0;
	CGprogram m_fp_ModelPass1;

	// CG FRAGMENT PROGRAM PARAMETERS
	CGparameter m_parm_BumpLightStrength;
	CGparameter m_parm_BumpLightColour;

	// MISC
	bool m_bInitialised, m_bFailedInit;
	int m_iLargestVisFrame;
	unsigned int m_uiNextTexIdx;
	int m_iNumFrameLights;
	int m_iCurPass;

public:
	CBumpmapMgr(void)
	{
		m_bInitialised = false;
		m_bFailedInit = false;
		m_uiNextTexIdx = GL_TEXTURE_NUM_BASE;
		m_iNumFrameLights = 0;
	}

	~CBumpmapMgr(void)
	{
		if (m_bInitialised)
			Shutdown();
	}

	bool Initialise(void); // (Re-)initialise everything.
	void Render(int pass); // Do the bumpmapping. Called from view.cpp and cdll_int.cpp.
	void Shutdown(void); // De-initialise everything.
	void Reset(void); // Reset the system, since we've changed map

	void AddLight(const char* targetname, Vector pos, Vector colour, float strength, float radius, bool enabled = true, int style = 0, int moveWithEntID = -1, bool moveWithExtraInfo = false, Vector moveWithEntPos = Vector(0,0,0), Vector moveWithEntAngles = Vector(0,0,0)); // add a bumpmapping light
	void EnableLight(const char* targetname, bool enable);

	void RenderStudioModel(bool bPreRender); // called before and after the studio model renderer renders a model

protected:
	bool IsHardwareCapable(void); // Can the user's hardware do the bumpmapping?
	bool InitialiseOGLExtensions(void); // Grab all the function pointers for the OGL extensions used
	bool InitialiseCg(void); // Init Cg
	bool LoadFragmentPrograms(void); // Load up all fragment programs and parameters used
	bool LoadBumpTextures(void); // Load the normal maps for textures specified in the map's bump file
	bool CreateSceneTextures(void); // Create all miscellaneous textures needed

	bool LoadCgProgram(CGprogram* pDest, CGprofile eProfile, const char* szFile); // load a Cg program
	bool LoadTexture(const char* szFile, unsigned int* pTexIdx); // load a 2D texture from a bitmap file
	void MakeNormCubeMap(unsigned int* pTexIdx); // make a normalisation cube map
	void MakeAttenuationMap(unsigned int* pTexIdx); // make a 3D attenuation texture
	void MakeFlatNormalMap(unsigned int* pTexIdx); // make a 1x1 flat normal map

	void RenderLight(int light); // render one light
	void CombineScenes(void); // combine everything together

	void RenderScreenQuad(float texU, float texV, int iNumTexUnits); // render a quad across the screen
	void DoOrthoProjection(bool activate); // activate/deactivate an orthogonal projection
};

extern CBumpmapMgr g_BumpmapMgr;

#endif
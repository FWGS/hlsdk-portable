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

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glaux.h>
#include <gl/glext.h>
#include <cg/cg.h>
#include <cg/cgGL.h>
#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "r_studioint.h"
#include "com_model.h"
#include "bumpmap.h"

extern globalvars_t* gpGlobals;

//
// CVARs AVAILABLE
//
// bm_enable 1/0 - enable/disable bumpmapping altogether
// bm_allflat 1/0 - don't use the supplied bumpmaps; render everything as if the surface is flat
//

#define MAX_STYLE_LEN 100
#define NUM_STYLES 12

// every 10th of a second. 'z' is max light, 'a' is darkness
char g_LightStyles[NUM_STYLES][MAX_STYLE_LEN+1] = 
{
	"z",
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	"mamamamamama",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomno",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba"
};

// we have to do this to stop the linker complaining, since we're using the evil GLaux
extern "C" long _ftol( double ); //defined by VC6 C libs
extern "C" long _ftol2( double dblSource ) { return _ftol( dblSource ); }

#define M_PI 3.141592653589793238

#define DEG2RAD(d) ((d) * (M_PI / 180))

inline float Clamp(float f, float low, float high)
{
	if (f < low)
		f = low;
	if (f > high)
		f = high;
	return f;
}

inline Vector RotateX(Vector V, float angle) // angle in degrees
{
	Vector ret;

	angle = DEG2RAD(angle);

	ret.x = V.x;
	ret.y = V.y*cos(angle) + V.z*sin(angle);
	ret.z = V.z*cos(angle) - V.y*sin(angle);

	return ret;
}

inline Vector RotateY(Vector V, float angle) // angle in degrees
{
	Vector ret;

	angle = DEG2RAD(angle);

	ret.x = V.x*cos(angle) - V.z*sin(angle);
	ret.y = V.y;
	ret.z = V.z*cos(angle) + V.x*sin(angle);

	return ret;
}

inline Vector RotateZ(Vector V, float angle) // angle in degrees
{
	Vector ret;

	angle = DEG2RAD(angle);

	ret.x = V.x*cos(angle) + V.y*sin(angle);
	ret.y = V.y*cos(angle) - V.x*sin(angle);
	ret.z = V.z;

	return ret;
}

inline Vector vecNCMVector(int i, int x, int y)
{
	float s = ((float)x + 0.5) / BUMP_NORM_CUBE_MAP_SIZE;
	float t = ((float)y + 0.5) / BUMP_NORM_CUBE_MAP_SIZE;
	float sc = s*2.0 - 1.0;
	float tc = t*2.0 - 1.0;
	Vector V;

	switch (i) 
	{
	case 0:
		V.x = 1.0;
		V.y = -tc;
		V.z = -sc;
		break;
	case 1:
		V.x = -1.0;
		V.y = -tc;
		V.z = sc;
		break;
	case 2:
		V.x = sc;
		V.y = 1.0;
		V.z = tc;
		break;
	case 3:
		V.x = sc;
		V.y = -1.0;
		V.z = -tc;
		break;
	case 4:
		V.x = sc;
		V.y = -tc;
		V.z = 1.0;
		break;
	case 5:
		V.x = -sc;
		V.y = -tc;
		V.z = -1.0;
	break;
	}

	V = V.Normalize();

	return V;
}

// Global instance of the bumpmap manager
CBumpmapMgr g_BumpmapMgr;

// OpenGL multitexture functions
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;

// OpenGL 3D texture functions
PFNGLTEXIMAGE3DEXTPROC glTexImage3DEXT;

bool CBumpmapMgr::Initialise(void)
{
	if (m_bInitialised)
		Shutdown();

	gEngfuncs.pfnRegisterVariable("bm_enable", "0", 0);
	gEngfuncs.pfnRegisterVariable("bm_allflat", "0", 0);
	
	m_bFailedInit = true;

	if (!IsHardwareCapable())
		return false;

	if (!InitialiseOGLExtensions())
		return false;

	if (!CreateSceneTextures())
		return false;

	if (!LoadBumpTextures())
		return false;

	if (!InitialiseCg())
		return false;

	if (!LoadFragmentPrograms())
		return false;

	m_bInitialised = true;
	m_bFailedInit = false;

	return true;
}

bool CBumpmapMgr::IsHardwareCapable(void)
{
	// Minimum hardware is a GeForce 3 or a Radeon 9500. I don't see the point in making bumpmapping run
	// on GeForce 2 or lower - the cards don't have a fast enough fillrate and much texture memory anyway.

	// grab the string saying which OpenGL extensions are available on this graphics card
	std::string sExtensions;
	sExtensions.assign((const char*)glGetString(GL_EXTENSIONS), strlen((const char*)glGetString(GL_EXTENSIONS)));

	// check for GL_ARB_multitexture (why the hell are they trying to run this on a Voodoo 2, anyway?)
	if (sExtensions.find("GL_ARB_multitexture") == -1)
	{
		MessageBox(NULL, "Bumpmapping error: Your hardware does not support GL_ARB_multitexture.", "Fatal Error", NULL);
		return false;
	}

	// check for GL_ARB_texture_cube_map
	if (sExtensions.find("GL_ARB_texture_cube_map") == -1)
	{
		MessageBox(NULL, "Bumpmapping error: Your hardware does not support GL_ARB_texture_cube_map.", "Fatal Error", NULL);
		return false;
	}

	// check for GL_NV_texture_rectangle or GL_EXT_texture_rectangle
	if (sExtensions.find("GL_NV_texture_rectangle") == -1 && sExtensions.find("GL_EXT_texture_rectangle") == -1)
	{
		MessageBox(NULL, "Bumpmapping error: Your hardware does not support GL_NV_texture_rectangle or GL_EXT_texture_rectangle.", "Fatal Error", NULL);
		return false;
	}

	// check for GL_EXT_texture3D
	if (sExtensions.find("GL_EXT_texture3D") == -1)
	{
		MessageBox(NULL, "Bumpmapping error: Your hardware does not support GL_EXT_texture3D.", "Fatal Error", NULL);
		return false;
	}

	// see if we have the requisite number of texture units
	int iTexUnits;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &iTexUnits);
	if (iTexUnits < 4)
	{
		MessageBox(NULL, "Bumpmapping error: Your hardware does not have four texture units.", "Fatal Error", NULL);
		return false;
	}

	// Check if fragment programs can be run (this'll cull the largest proportion of video cards)
	// FP20 = GL_NV_register_combiners and GL_NV_texture_shader
	// FP30 = GL_NV_fragment_program
	// ARBFP1 = GL_ARB_fragment_program
	// There's no GL_ATI_fragment_shader support because Cg doesn't include support for compiling to that target.
	// You could write a straight shader in shader ASM, but it isn't really worth it for the few extra cards it'd add
	// support for.
	if (!cgGLIsProfileSupported(CG_PROFILE_FP20) && !cgGLIsProfileSupported(CG_PROFILE_FP30) && !cgGLIsProfileSupported(CG_PROFILE_ARBFP1))
	{
		MessageBox(NULL, "Bumpmapping error: Your hardware does not support GL_NV_register_combiners and GL_NV_texture_shader, GL_NV_fragment_program or GL_ARB_fragment_program.", "Fatal Error", NULL);
		return false;
	}

	return true;
}

bool CBumpmapMgr::CreateSceneTextures(void)
{
	// create the DIFFUSE scene texture
	m_uiDiffuseScene = m_uiNextTexIdx++;

	// create the LIGHTMAPPED scene texture
	m_uiLightmapScene = m_uiNextTexIdx++;

	// create the BUMPMAPPED scene texture
	m_uiBumpScene = m_uiNextTexIdx++;

	// create the NORMALISATION CUBE MAP
	MakeNormCubeMap(&m_uiNormCubeMap);

	// create the ATTENUATION MAP
	MakeAttenuationMap(&m_uiAttenuationMap);

	// create the NORMAL MAP for FLAT SURFACES
	MakeFlatNormalMap(&m_uiFlatSurfaceNorm);
	
	return true;
}

bool CBumpmapMgr::LoadBumpTextures(void)
{
	char szFile[256];
	char szTex[256];
	char* pBumpFile;

	std::string sMap(gEngfuncs.pfnGetLevelName());

	sprintf(szFile, "%s/%s_bump.txt", gEngfuncs.pfnGetGameDirectory(), sMap.substr(0, sMap.rfind('.')).c_str());

	FILE* pFile = fopen(szFile, "r");
	if (!pFile)
	{
		// If we can't find the file, just use the flat bumpmap for everything in this map.
		return true;
	}

	model_t* pWorld = gEngfuncs.GetEntityByIndex(0)->model; // worldspawn model
	unsigned int uiTex;
	char szTexPath[256];

	memset(szTex, 0, 256);

	// go through each line of the file
	while (fgets(szTex, 256, pFile))
	{
		if (szTex[strlen(szTex)-1] == '\n')
			szTex[strlen(szTex)-1] = '\0';

		// set pBumpFile to point to the string after the space, and set the space to a null so that szTex only
		// references the string before the space
		pBumpFile = strchr(szTex, ' ');
		*pBumpFile = '\0';
		pBumpFile++;

		// go through each texinfo in the map
		for (int i = 0; i < pWorld->numtexinfo; i++)
		{
			// we found the texture
			if (!_stricmp(pWorld->texinfo[i].texture->name, szTex))
			{
				sprintf(szTexPath, "%s/detail/%s", gEngfuncs.pfnGetGameDirectory(), pBumpFile);

				// load up the specified normal map
				if (LoadTexture(szTexPath, &uiTex))
				{
					// shove it into the map of diffuse texture ID to normal map texture ID
					m_aNormalMaps.insert(std::make_pair(pWorld->texinfo[i].texture->gl_texturenum, uiTex));
					break;
				}
			}
		}

		memset(szTex, 0, 256);
	}

	fclose(pFile);

	return true;
}

bool CBumpmapMgr::InitialiseOGLExtensions(void)
{
	// MULTITEXTURE EXTENSIONS
	// Used for... well, multitexturing.
	
	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
	if (!glActiveTextureARB)
	{
		MessageBox(NULL, "Bumpmapping error: Could not acquire pointer to glActiveTextureARB.", "Fatal Error", NULL);
		return false;
	}

	glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
	if (!glMultiTexCoord2fARB)
	{
		MessageBox(NULL, "Bumpmapping error: Could not acquire pointer to glMultiTexCoord2fARB.", "Fatal Error", NULL);
		return false;
	}

	glMultiTexCoord3fvARB = (PFNGLMULTITEXCOORD3FVARBPROC)wglGetProcAddress("glMultiTexCoord3fvARB");
	if (!glMultiTexCoord3fvARB)
	{
		MessageBox(NULL, "Bumpmapping error: Could not acquire pointer to glMultiTexCoord3fvARB.", "Fatal Error", NULL);
		return false;
	}

	// 3D TEXTURE EXTENSIONS
	// A 3D texture is used for the attenuation map.

	glTexImage3DEXT = (PFNGLTEXIMAGE3DEXTPROC)wglGetProcAddress("glTexImage3DEXT");
	if (!glTexImage3DEXT)
	{
		MessageBox(NULL, "Bumpmapping error: Could not acquire pointer to glTexImage3DEXT.", "Fatal Error", NULL);
		return false;
	}

	return true;
}

bool CBumpmapMgr::InitialiseCg(void)
{
	// create a Cg context
	m_cg_Context = cgCreateContext();
	if (!m_cg_Context)
	{
		MessageBox(NULL, "Bumpmapping error: Could not create Cg context!", "Fatal Error", NULL);
		return false;
	}

	// get the best vertex and fragment program profiles
	m_cg_vertProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	m_cg_fragProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

	return true;
}

bool CBumpmapMgr::LoadFragmentPrograms(void)
{
	// fullbright map
	if (!LoadCgProgram(&m_fp_Diffuse, m_cg_fragProfile, "render/fp_diffuse.cg"))
	{
		MessageBox(NULL, "Bumpmapping error: Could not load fp_diffuse.cg!", "Fatal Error", NULL);
		return false;
	}

	// map with only lightmaps
	if (!LoadCgProgram(&m_fp_Lightmaps, m_cg_fragProfile, "render/fp_lightmaps.cg"))
	{
		MessageBox(NULL, "Bumpmapping error: Could not load fp_lightmaps.cg!", "Fatal Error", NULL);
		return false;
	}

	// bumpmapping
	if (!LoadCgProgram(&m_fp_Bump, m_cg_fragProfile, "render/fp_bump.cg"))
	{
		MessageBox(NULL, "Bumpmapping error: Could not load fp_bump.cg!", "Fatal Error", NULL);
		return false;
	}

	// fullbright model
	if (!LoadCgProgram(&m_fp_ModelPass0, m_cg_fragProfile, "render/fp_model_p0.cg"))
	{
		MessageBox(NULL, "Bumpmapping error: Could not load fp_model_p0.cg!", "Fatal Error", NULL);
		return false;
	}

	// model with diffuse colour
	if (!LoadCgProgram(&m_fp_ModelPass1, m_cg_fragProfile, "render/fp_model_p1.cg"))
	{
		MessageBox(NULL, "Bumpmapping error: Could not load fp_model_p1.cg!", "Fatal Error", NULL);
		return false;
	}

	m_parm_BumpLightColour = cgGetNamedParameter(m_fp_Bump, "LightColour");
	m_parm_BumpLightStrength = cgGetNamedParameter(m_fp_Bump, "LightStrength");

	return true;
}

bool CBumpmapMgr::LoadCgProgram(CGprogram* pDest, CGprofile eProfile, const char* szFile)
{
	// map the path
	char file[512];
	sprintf(file, "%s/%s", gEngfuncs.pfnGetGameDirectory(), szFile);

	// load the program
	*pDest = cgCreateProgramFromFile(m_cg_Context, CG_SOURCE, file, eProfile, "main", NULL);
	if (!(*pDest))
		return false;

	cgGLLoadProgram(*pDest);

	return true;
}

bool CBumpmapMgr::LoadTexture(const char* szFile, unsigned int* pTexIdx)
{
	// We're using GLaux for this, because I can't be bothered to code a proper bitmap loading function.
	// It works, so no complaining.

	// load the image
	AUX_RGBImageRec* pImg = auxDIBImageLoad(szFile);
	if (!pImg)
		return false;

	// assign a texture ID and set the various parameters
	*pTexIdx = m_uiNextTexIdx++;
	glBindTexture(GL_TEXTURE_2D, *pTexIdx);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// load the texture, generating mipmaps for it at the same time
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, pImg->sizeX, pImg->sizeY, GL_RGB, GL_UNSIGNED_BYTE, pImg->data);

	free(pImg->data);
	free(pImg);

	return true;
}

void CBumpmapMgr::Shutdown(void)
{
	cgDestroyProgram(m_fp_Diffuse);
	cgDestroyProgram(m_fp_Lightmaps);
	cgDestroyProgram(m_fp_Bump);
	cgDestroyProgram(m_fp_ModelPass0);
	cgDestroyProgram(m_fp_ModelPass1);

	cgDestroyContext(m_cg_Context);

	Reset();
}

void CBumpmapMgr::Reset(void)
{
	m_iNumFrameLights = 0;
	m_bInitialised = false;
	m_bFailedInit = false;
	m_iLargestVisFrame = 0;

	m_aBumpedScenes.clear();
	m_aNormalMaps.clear();
}

void CBumpmapMgr::Render(int pass)
{
	if (!m_bInitialised && !m_bFailedInit)
		Initialise();
	
	if (CVAR_GET_FLOAT("bm_enable") == 0 || g_BumpmapMgr.m_bFailedInit)
		return;

	int i;

	// We must clear to black to enable non-bumpmapped items to be re-combined with the scene properly,
	// and also to enable us to just render the bump mapping with additive blending for every light.
	glClearColor(0,0,0,1);

	m_iCurPass = pass;

	switch (pass)
	{
	case 0:
		// Render the scene with only the DIFFUSE COLOUR (fullbright)
		cgGLEnableProfile(m_cg_fragProfile);
		cgGLBindProgram(m_fp_Diffuse);

		break;

	case 1:
		// In pass 0, we rendered the scene with just the diffuse texture. Grab this to a texture.
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, m_uiDiffuseScene);
		glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth, ScreenHeight, 0);

		// Here is the best time to render the bump-mapped scenes. We make use of the existing z-buffer data
		// to (a) stop us having to update the z-buffer and (b) to create black "holes" where items
		// such as models and brush entities go.

		glClear(GL_COLOR_BUFFER_BIT); // clear the colour buffer, but NOT the depth buffer

		glDepthMask(GL_FALSE); // disable modification of the depth buffer
		glDepthFunc(GL_LEQUAL);

		// Each light adds its contribution to the rest
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		for (i = 0; i < m_aBumpedScenes.size(); i++)
		{
			RenderLight(i);
		}

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE); // re-enable modification of the depth buffer
		glDepthFunc(GL_LESS);

		glEnable(GL_TEXTURE_RECTANGLE_NV);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, m_uiBumpScene);
		glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth, ScreenHeight, 0);
		glDisable(GL_TEXTURE_RECTANGLE_NV);

		glClear(GL_COLOR_BUFFER_BIT);

		// We now render the scene with just the LIGHTMAPS.
		cgGLBindProgram(m_fp_Lightmaps);

		break;

	case 2:
		// In pass 1, we rendered the scene with just the lightmaps. Grab this to a texture.
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, m_uiLightmapScene);
		glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth, ScreenHeight, 0);

		// Fragment programs aren't needed any more.
		cgGLDisableProfile(m_cg_fragProfile);

		// Switch to an orthogonal projection to render full-screen quads and combine all the scenes
		// together to give the final result.
		DoOrthoProjection(true);
		CombineScenes();
		DoOrthoProjection(false);

		break;

	default:
		gEngfuncs.Con_Printf("BUMPMAPPING: Unknown pass number %i passed to Render\n", pass);
	}
}

void CBumpmapMgr::RenderLight(int light)
{
	// get a ptr to the light in question
	bumplight_t* pLight = &m_aBumpedScenes[light];

	if (!pLight->enabled)
		return;

	// calculate the movewith
	if (pLight->moveWithEnt)
	{
		// first, adjust the light's position by any change that the movewith entity has undergone
		pLight->pos = pLight->origPos + (pLight->moveWithEnt->origin - pLight->entOrigPos);

		// rotate the light around the movewith entity by any change in its angles
		Vector anglesChange = pLight->moveWithEnt->angles - pLight->entOrigAngles;
		Vector newPos = pLight->pos - pLight->moveWithEnt->origin;
		newPos = RotateZ(newPos, -anglesChange.y);
		newPos = RotateY(newPos, -anglesChange.x);
		newPos = RotateX(newPos, -anglesChange.z);
		newPos = newPos + pLight->moveWithEnt->origin;

		pLight->pos = newPos;
	}

	float styleStrength = (float)(g_LightStyles[pLight->style][(int)(gpGlobals->time * 10) % strlen(g_LightStyles[pLight->style])] - 'a') / ('z' - 'a');

	if (styleStrength == 0)
		return;

	// bind the right fragment program and set the parameters
	cgGLBindProgram(m_fp_Bump);

	cgGLSetParameter1f(m_parm_BumpLightStrength, pLight->strength * styleStrength);
	cgGLSetParameter3fv(m_parm_BumpLightColour, pLight->colour);
	
	// bind the normalisation cube map to tex unit 1
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_RECTANGLE_NV);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_uiNormCubeMap);

	// bind the attenuation map to tex unit 2
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_3D_EXT);
	glBindTexture(GL_TEXTURE_3D_EXT, m_uiAttenuationMap);

	// normal maps will be bound to tex unit 0
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_RECTANGLE_NV);
	glEnable(GL_TEXTURE_2D);

	// create a bunch of variables
	model_t* pWorld = gEngfuncs.GetEntityByIndex(0)->model; // worldspawn model
	msurface_t** pSurf = pWorld->marksurfaces; // ptr to array of surfaces
	glpoly_t* pPoly;
	Vector N, T, B, L, L2, V, V1, V2, V3, E1, E2, AttenCoords;
	TexNormalMaps_It texID;

	glShadeModel(GL_SMOOTH);

	// go through each surface
	for (int i = 0; i < pWorld->nummarksurfaces; i++)
	{
		// This is a semi-hack to use the pre-calculated visibilty info. We don't have access
		// to what the current visframe is, but we can assume that the surfaces with the highest
		// visframe are those which are currently being rendered. If the visframe changes, then
		// there might be more overdraw for one frame, but this isn't noticeable.
		//
		// To stop surfaces being rendered twice (which is disastrous, since we are using additive
		// blending) we set the visframe to the negative value of itself once we've rendered the surface.
		// If we then find a surface that has a negative visframe, we know that we've already
		// rendered that one.

		if (pSurf[i]->visframe > 0 && pSurf[i]->visframe >= m_iLargestVisFrame)
		{
			m_iLargestVisFrame = pSurf[i]->visframe;

			pPoly = pSurf[i]->polys;
			float* vert = pPoly->verts[0];

			// find the normal map corresponding to this surface's texture (use the flat map if there isn't one)
			texID = m_aNormalMaps.find(pSurf[i]->texinfo->texture->gl_texturenum);
			if (CVAR_GET_FLOAT("bm_allflat") == 1 || texID == m_aNormalMaps.end())
				glBindTexture(GL_TEXTURE_2D, m_uiFlatSurfaceNorm);
			else
				glBindTexture(GL_TEXTURE_2D, texID->second);

			// tangent and binormal simply come from the UV vectors
			T = pSurf[i]->texinfo->vecs[0];
			B = pSurf[i]->texinfo->vecs[1];
			T = T.Normalize();
			B = B.Normalize();

			// We must now calculate the normal of the surface. We can't cross-product the tangent and binormal,
			// as we can't guarantee which way the normal will be facing (textures can be mirrored both ways).
			// So, we must calculate it by doing the cross product of two edge vectors. However, we can't just
			// choose any old edge vectors, since some turn out to be colinear, which gives bogus normals.
			// The solution is to go through each vertex until we find two edge vectors that aren't colinear.

			V1 = Vector(vert[0], vert[1], vert[2]);
			V2 = Vector(vert[VERTEXSIZE], vert[VERTEXSIZE+1], vert[VERTEXSIZE+2]);

			E1 = (V1 - V2).Normalize();

			bool bFound = false;

			for (int j = 2; j < pPoly->numverts; j++)
			{
				V3 = Vector(vert[VERTEXSIZE*j], vert[VERTEXSIZE*j + 1], vert[VERTEXSIZE*j + 2]);

				E2 = (V1 - V3).Normalize();

				if (fabs(fabs(DotProduct(E1, E2))-1) > 0.01f) // vectors aren't colinear (dp 1 = equal, dp -1 = opposite) - check for > epsilon to account for FP error
				{
					N = -1 * CrossProduct(E1, E2).Normalize(); // we can use these edge vectors to calculate the normal
					bFound = true;
					break;
				}
			}

			if (!bFound)
				N = -1 * CrossProduct(E1, (V1 - Vector(vert + VERTEXSIZE*2)).Normalize()).Normalize();
			
			// render this surface

			glBegin(GL_POLYGON);

			for (int v = 0; v < pPoly->numverts; v++, vert+=VERTEXSIZE)
			{
				// Since we can't use a vertex program, we have to calculate the light vector here. We first
				// calculate the light vector in world space, and then transform it into tangent space
				// by doing the equivalent of multiplying it by the TBN matrix, and finally scale it into the range [0,1].
				V = Vector(vert);
				L = pLight->pos - V;
				L2.x = DotProduct(T, L)*0.5 + 0.5;
				L2.y = -DotProduct(B, L)*0.5 + 0.5; // for some reason, the y component of the light vector must be flipped - probably a by-product of the normal maps being flipped on loading
				L2.z = DotProduct(N, L)*0.5 + 0.5;

				// We also have to calculate the coordinates in the attenuation texture
				AttenCoords = (L / (pLight->radius * 2)) + Vector(0.5,0.5,0.5);

				// send the data to OGL
				glTexCoord2f(vert[3], 1 - vert[4]); // the normal maps get flipped on loading, for some reason - reverse that here
				glMultiTexCoord3fvARB(GL_TEXTURE1_ARB, L2);
				glMultiTexCoord3fvARB(GL_TEXTURE2_ARB, AttenCoords);
				glVertex3fv(vert);
			}

			glEnd();
		}

		// set the negative visframe to say we've rendered this surface
		if (pSurf[i]->visframe > 0)
			pSurf[i]->visframe = -pSurf[i]->visframe;
	}

	// go back and make all visframes positive again, so that HL can render the map properly
	for (i = 0; i < pWorld->nummarksurfaces; i++)
	{
		if (pSurf[i]->visframe < 0)
			pSurf[i]->visframe = -pSurf[i]->visframe;
	}

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_3D_EXT);

	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void CBumpmapMgr::CombineScenes(void)
{
	// find out how many texture units we've got to mess with
	int iTexUnits;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &iTexUnits);


	// Add together the lightmap and the bump mapping, and modulate this by the textures

	for (int i = 0; i < iTexUnits; i++)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB + i);
		glDisable(GL_TEXTURE_RECTANGLE_NV);
		glDisable(GL_TEXTURE_2D);
	}

	glColor4f(1,1,1,1);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, m_uiLightmapScene);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, m_uiBumpScene);

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, m_uiDiffuseScene);

	RenderScreenQuad((float)ScreenWidth, (float)ScreenHeight, iTexUnits);


	// Clean up - de-activate texturing on all units (except the first one, on which
	// we enable regular 2D texturing, since HL doesn't do this...)

	for (i = 0; i < iTexUnits; i++)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB + i);
		glDisable(GL_TEXTURE_RECTANGLE_NV);
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);

	glDisable(GL_BLEND);
}

void CBumpmapMgr::RenderScreenQuad(float texU, float texV, int iNumTexUnits)
{
	int i;

	glBegin(GL_QUADS);

	for (i = 0; i < iNumTexUnits; i++) glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, 0, texV);
	glVertex2f(0,0);
	
	for (i = 0; i < iNumTexUnits; i++) glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, texU, texV);
	glVertex2f(1,0);
	
	for (i = 0; i < iNumTexUnits; i++) glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, texU, 0);
	glVertex2f(1,1);
	
	for (i = 0; i < iNumTexUnits; i++) glMultiTexCoord2fARB(GL_TEXTURE0_ARB + i, 0, 0);
	glVertex2f(0,1);

	glEnd();
}

void CBumpmapMgr::DoOrthoProjection(bool activate)
{
	if (activate)
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 1, 1, 0, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
}

void CBumpmapMgr::AddLight(const char* targetname, Vector pos, Vector colour, float strength, float radius,
						   bool enabled /*= true*/, int style /*= 0*/, int moveWithEntID /*= -1*/, bool moveWithExtraInfo /*= false*/,
						   Vector moveWithEntPos, Vector moveWithEntAngles)
{
	bumplight_t light;

	light.enabled = enabled;
	light.pos = pos;
	light.origPos = pos;
	light.colour = colour;
	light.strength = strength;
	light.radius = radius;
	light.style = (style >= NUM_STYLES ? 0 : style);

	memset(light.targetname, 0, 64);
	memcpy(light.targetname, targetname, strlen(targetname));

	if (moveWithEntID == -1)
		light.moveWithEnt = NULL;
	else
	{
		light.moveWithEnt = gEngfuncs.GetEntityByIndex(moveWithEntID);

		if (light.moveWithEnt != NULL)
		{
			if (moveWithExtraInfo)
			{
				light.entOrigPos = moveWithEntPos;
				light.entOrigAngles = moveWithEntAngles;
			}
			else
			{
				light.entOrigPos = light.moveWithEnt->origin;
				light.entOrigAngles = light.moveWithEnt->angles;
			}
		}
	}

	m_aBumpedScenes.push_back(light);

	m_iNumFrameLights++;
}

void CBumpmapMgr::EnableLight(const char* targetname, bool enable)
{
	for (BumpSceneList_It it = m_aBumpedScenes.begin(); it != m_aBumpedScenes.end(); ++it)
	{
		if (!_stricmp(targetname, it->targetname))
			it->enabled = enable;
	}
}

void CBumpmapMgr::RenderStudioModel(bool bPreRender)
{	
	if (CVAR_GET_FLOAT("bm_enable") == 0 || g_BumpmapMgr.m_bFailedInit)
		return;

	// bPreRender is true before rendering a studio model, false afterwards
	if (bPreRender)
	{
		if (m_iCurPass == 0)
			cgGLBindProgram(m_fp_ModelPass0);
		else
			cgGLBindProgram(m_fp_ModelPass1);
	}
	else
	{
		if (m_iCurPass == 0)
			cgGLBindProgram(m_fp_Diffuse);
		else
			cgGLBindProgram(m_fp_Lightmaps);
	}
}

void CBumpmapMgr::MakeNormCubeMap(unsigned int* pTexIdx)
{
	unsigned char* pTex = new unsigned char[BUMP_NORM_CUBE_MAP_SIZE*BUMP_NORM_CUBE_MAP_SIZE*3];

	*pTexIdx = m_uiNextTexIdx++;

	// set the texture's parameters
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, *pTexIdx);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);

	Vector V;

	// For each face of the cube map, generate a flat texture that gives a normalised version of the
	// vector used to access that texel in the cube texture.
	for (int i = 0; i < 6; i++)
	{
		for (int y = 0; y < BUMP_NORM_CUBE_MAP_SIZE; y++)
		{
			for (int x = 0; x < BUMP_NORM_CUBE_MAP_SIZE; x++)
			{
				V = vecNCMVector(i, x, y);
				pTex[(x + y*BUMP_NORM_CUBE_MAP_SIZE)*3] = (unsigned char)((V.x * 0.5 + 0.5)*255);
				pTex[(x + y*BUMP_NORM_CUBE_MAP_SIZE)*3+1] = (unsigned char)((V.y * 0.5 + 0.5)*255);
				pTex[(x + y*BUMP_NORM_CUBE_MAP_SIZE)*3+2] = (unsigned char)((V.z * 0.5 + 0.5)*255);
			}
		}

		// store this face's image
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, 3, BUMP_NORM_CUBE_MAP_SIZE, BUMP_NORM_CUBE_MAP_SIZE,
			0, GL_RGB, GL_UNSIGNED_BYTE, pTex);
	}

	delete[] pTex;
}

void CBumpmapMgr::MakeAttenuationMap(unsigned int* pTexIdx)
{
	unsigned char* pTex = new unsigned char[BUMP_ATTENUATION_MAP_SIZE*BUMP_ATTENUATION_MAP_SIZE*BUMP_ATTENUATION_MAP_SIZE];

	*pTexIdx = m_uiNextTexIdx++;

	glBindTexture(GL_TEXTURE_3D_EXT, *pTexIdx);
	glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);

	Vector V;

	// For each texel in the attenuation map, calculate x^2 + y^2 + z^2 and store it, clamping to [0,1].
	for (int z = 0; z < BUMP_ATTENUATION_MAP_SIZE; z++)
	{
		for (int y = 0; y < BUMP_ATTENUATION_MAP_SIZE; y++)
		{
			for (int x = 0; x < BUMP_ATTENUATION_MAP_SIZE; x++)
			{
				V = Vector(((float)x / BUMP_ATTENUATION_MAP_SIZE) * 2 - 1,
						   ((float)y / BUMP_ATTENUATION_MAP_SIZE) * 2 - 1,
						   ((float)z / BUMP_ATTENUATION_MAP_SIZE) * 2 - 1) * 1.05;

				pTex[x + y*BUMP_ATTENUATION_MAP_SIZE + z*BUMP_ATTENUATION_MAP_SIZE*BUMP_ATTENUATION_MAP_SIZE] = (unsigned char)Clamp((V.x*V.x + V.y*V.y + V.z*V.z) * 255, 0, 255);
			}
		}
	}

	// load the texture data
	glTexImage3DEXT(GL_TEXTURE_3D_EXT, 0, GL_RGB, BUMP_ATTENUATION_MAP_SIZE, BUMP_ATTENUATION_MAP_SIZE, BUMP_ATTENUATION_MAP_SIZE,
		0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pTex);

	delete[] pTex;
}

void CBumpmapMgr::MakeFlatNormalMap(unsigned int* pTexIdx)
{
	*pTexIdx = m_uiNextTexIdx++;

	glBindTexture(GL_TEXTURE_2D, *pTexIdx);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	unsigned char pTex[1*1*3]; // it doesn't need any clarity, seeing as it's all one colour - just make it 1x1

	pTex[0] = 128; // unpacked, this gives 0
	pTex[1] = 128; // unpacked, this gives 0
	pTex[2] = 255; // unpacked, this gives 1

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pTex);
}
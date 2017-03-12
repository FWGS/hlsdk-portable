//++ BulliT
//Most parts written by David "Nighthawk" Flor (dflor@mach3.com) for the mod Opera (http://www.halflife.net/opera)
//Parts of code from Valve Software mdlviewer (CalcBonePosition).

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"
#include "com_model.h"
#include "studio.h"
#include "com_weapons.h"
#include "AgModel.h"

#ifdef AG_USE_CHEATPROTECTION

void CalcBonePosition(int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos);
extern int g_iPure;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgModel::AgModel()
{
  m_vMinBounds = Vector(0,0,0);
	m_vMaxBounds = Vector(0,0,0);
	m_iVertexCount = 0;

	m_vMinBone = Vector(0,0,0);
	m_vMaxBone = Vector(0,0,0);
	m_iBoneCount = 0;
  m_bCorrupt = false;
  m_bFoundAndChecked = false;
}

AgModel::~AgModel()
{

}


void AgModel::AddVertex( const Vector &vPoint )
{
	if (m_iVertexCount == 0)
	{
		m_vMinBounds = m_vMaxBounds = vPoint;
	}
	else
	{
		m_vMinBounds.x = min( m_vMinBounds.x, vPoint.x );
		m_vMinBounds.y = min( m_vMinBounds.y, vPoint.y );
		m_vMinBounds.z = min( m_vMinBounds.z, vPoint.z );

		m_vMaxBounds.x = max( m_vMaxBounds.x, vPoint.x );
		m_vMaxBounds.y = max( m_vMaxBounds.y, vPoint.y );
		m_vMaxBounds.z = max( m_vMaxBounds.z, vPoint.z );
	}
	m_iVertexCount++;
}

void AgModel::AddBone( const Vector &vPoint )
{
	if (m_iBoneCount == 0)
	{
		m_vMinBone = m_vMaxBone = vPoint;
	}
	else
	{
		m_vMinBone.x = min( m_vMinBone.x, vPoint.x );
		m_vMinBone.y = min( m_vMinBone.y, vPoint.y );
		m_vMinBone.z = min( m_vMinBone.z, vPoint.z );

		m_vMaxBone.x = max( m_vMaxBone.x, vPoint.x );
		m_vMaxBone.y = max( m_vMaxBone.y, vPoint.y );
		m_vMaxBone.z = max( m_vMaxBone.z, vPoint.z );
	}
	m_iBoneCount++;
}

void AgModel::AddBonesToVertices( void )
{
	Vector vAdjust;

	if (m_iBoneCount > 0)
	{
		vAdjust = (m_vMaxBone - m_vMinBone);

		AddVertex( vAdjust / 2 );
		AddVertex( -(vAdjust / 2) );

		m_vMinBone = m_vMaxBone = Vector(0,0,0);
		m_iBoneCount = 0;
	}
}


void AgModel::ReadModel(const char *szModelName)
{
	char		*pBuffer;
	char		*pTempBuffer;
	char		szFullName[ _MAX_PATH ];
	float		flBone[3];

  strcpy( szFullName, szModelName );
  pBuffer = (char*)gEngfuncs.COM_LoadFile( szFullName, 5, NULL );
	if (pBuffer)
	{
		studiohdr_t			*pHeader;
		mstudiobodyparts_t	*pBodyParts;
		mstudiomodel_t		*pModel;
		long				iCnt;
		long				iModelCnt, iModels;
		long				iVert, iVertCnt;
		vec3_t				*pVert;
		mstudiobone_t		*pBone;
		long				iBoneCnt, iBones;
		mstudioseqdesc_t	*pSequence;
		long				iSequenceCnt, iSequences;
		mstudioanim_t		*pAnim;
		long				iFrameCnt, iFrames;

		pHeader = (studiohdr_t *)pBuffer;
    if (10 == pHeader->version)
    {
		  pTempBuffer = (pBuffer + pHeader->bodypartindex);
		  pBodyParts = (mstudiobodyparts_t *)pTempBuffer;
		  iModels = 0;
		  for (iCnt = 0; iCnt < pHeader->numbodyparts; iCnt++)
			  iModels += pBodyParts[iCnt].nummodels;
		  pTempBuffer += (pHeader->numbodyparts * sizeof(mstudiobodyparts_t));

		  pModel = (mstudiomodel_t *)pTempBuffer;
		  for (iModelCnt = 0; iModelCnt < iModels; iModelCnt++)
		  {
			  iVert = pModel[iModelCnt].numverts;
			  pVert = (vec3_t *)(pBuffer + pModel[iModelCnt].vertindex);
			  for (iVertCnt = 0; iVertCnt < iVert; iVertCnt++)
			  {
				  AddVertex( pVert[iVertCnt] );
			  }
		  }

		  pBone = (mstudiobone_t *)(pBuffer + pHeader->boneindex);
		  iBones = pHeader->numbones;

		  pSequence = (mstudioseqdesc_t *)(pBuffer + pHeader->seqindex);
		  iSequences = pHeader->numseq;
		  for (iSequenceCnt = 0; iSequenceCnt < iSequences; iSequenceCnt++)
		  {
			  iFrames = pSequence[iSequenceCnt].numframes;

			  pTempBuffer = (pBuffer + pSequence[iSequenceCnt].animindex);
			  pAnim = (mstudioanim_t *)pTempBuffer;
			  for (iBoneCnt = 0; iBoneCnt < iBones; iBoneCnt++)
			  {
				  for (iFrameCnt = 0; iFrameCnt < iFrames; iFrameCnt++)
				  {
					  CalcBonePosition( iFrameCnt, pBone + iBoneCnt,
						  pAnim, flBone );

					  AddBone( flBone );
				  }
			  }

			  AddBonesToVertices();
		  }
      m_bFoundAndChecked = true;
    }
    else
    {
      m_bCorrupt = true;
    }
    gEngfuncs.COM_FreeFile( pBuffer );
	}
}

void CalcBonePosition( int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos )
{
	float				s = 0;	
	int					j, k;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			
			k = frame;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0 - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
	}
}

bool AgModel::CheckModel(const char* szModelName)
{
  try 
  {
    ReadModel(szModelName);
  }
  catch(...)
  {
    m_bCorrupt = true;
  }

  if (m_bCorrupt)
  {
    char szMessage[256];
    sprintf(szMessage,"Server enforces model check and %s seems to be corrupt.\n",szModelName);
    AgLog(szMessage);
    ConsolePrint(szMessage);
#ifdef _DEBUG
		return true;
#else
    return false;
#endif
  }

	Vector	vMaxBounds = Vector(0,0,0);
  Vector  vBounds = m_vMaxBounds - m_vMinBounds;
  if ( !strnicmp( szModelName, "/models/player", 14) )
  {
    if (0 < g_iPure)
   	  vMaxBounds = Vector( 78, 30, 98 );
    else
  	  vMaxBounds = Vector( 105, 105, 105 ); //Big fucking models allowed..
  }
	else if ( !strnicmp( szModelName, "/models/p_", 9 ) )
		vMaxBounds = Vector( 42, 21, 60 );
	else if ( !strnicmp( szModelName, "/models/w_", 9 ) )
		vMaxBounds = Vector( 82, 69, 76);
	else if ( !strnicmp( szModelName, "/models/v_", 9 ) )
		vMaxBounds = Vector( 46, 55, 120 );
	else
		vMaxBounds = Vector( 100, 100, 100 );

	if (vBounds.x > vMaxBounds.x || vBounds.y > vMaxBounds.y || vBounds.z > vMaxBounds.z)
  {
    char szMessage[256];
    sprintf(szMessage,"Server enforces model check and %s is not valid. Your model got these ranges: %.4f,%.4f,%.4f\n",szModelName,vBounds.x, vBounds.y, vBounds.z);
    AgLog(szMessage);
    sprintf(szMessage,"Server enforces model check and %s is not valid.\n",szModelName);
    ConsolePrint(szMessage);
#ifdef _DEBUG
		return true;
#else
    return false;
#endif
  }

  return true;
}

bool AgModel::IsChecked()
{
  return m_bFoundAndChecked;
}

#endif //AG_USE_CHEATPROTECTION

//-- Martin Webrant

#include "BFW.h"
#include "BFW_Totoro.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_Physics.h"
#include "BFW_Object.h"
#include "Oni_Weapon.h"
#include "Oni.h"
#include "ONI_GameStatePrivate.h"
#include "Oni_FX.h"

UUtBool			FXgLaser_UseAlpha	= UUcFalse;
M3tTextureMap	*FXgLaser_Texture	= NULL;

float			FXgLaser_Width		= 0.2f;

#define CONE_SEG_COUNT	16


UUtError FXrEffects_Initialize(void)
{
#if CONSOLE_DEBUGGING_COMMANDS
	SLrGlobalVariable_Register_Float("fx_laser_width",	"",	&FXgLaser_Width);
#endif

	return UUcError_None;
}

UUtError FXrEffects_LevelBegin(void)
{
	if(FXgLaser_UseAlpha)
	{
		TMrInstance_GetDataPtr( M3cTemplate_TextureMap, "contrail4444", (void **) &FXgLaser_Texture );
		FXgLaser_Texture->flags			&= ~M3cTextureFlags_Blend_Additive;
	}
	else
	{
		TMrInstance_GetDataPtr( M3cTemplate_TextureMap, "laser_contrail", (void **) &FXgLaser_Texture );
		FXgLaser_Texture->flags			|= M3cTextureFlags_Blend_Additive;
	}

	return UUcError_None;
}

void FXrDrawLaserDot(M3tPoint3D *inPoint)
{
	UUtError error;
	M3tTextureMap *dotTexture;
	float scale = .003f;
	float cam_sphere_dist;
	M3tPoint3D cameraLocation;

	M3rCamera_GetViewData(ONgActiveCamera, &cameraLocation, NULL, NULL);

	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "dot", (void **) &dotTexture);
	UUmAssert(UUcError_None == error);

	if (error) 
		return;

	cam_sphere_dist = MUmVector_GetDistance( cameraLocation, (*inPoint));

	if (cam_sphere_dist > .001f) 
	{
		scale *= cam_sphere_dist;
		M3rSimpleSprite_Draw(dotTexture, inPoint, scale, scale, IMcShade_Red, M3cMaxAlpha);
	}
}

void FXrDrawLaser( M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor )
{
	M3tVector3D				laser_from;
	M3tVector3D				laser_to;
	M3tVector3D				cameraPos, cameraDir;
	M3tVector3D				contrailDir;
	M3tVector3D				contrailWidth;
	M3tContrailData			p1;
	M3tContrailData			p2;
	M3tGeomCamera			*activeCamera;
	float					width;
	float					scale;
	UUtUns32				color;	
	UUtUns16				alpha;	
	M3tMatrix4x3*			stack_matrix;
	M3tMatrix4x3			matrix;
	
	M3rGeom_State_Push();
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);

	laser_from		= *inFrom;
	laser_to		= *inTo;

	M3rMatrixStack_Get(&stack_matrix);
	MUrMatrix_Inverse(stack_matrix,&matrix);

	M3rCamera_GetActive(&activeCamera);
	M3rCamera_GetViewData(activeCamera, &cameraPos, NULL, NULL);

	MUrMatrix_MultiplyPoint(&cameraPos, &matrix, &cameraPos);

	MUmVector_Subtract(cameraDir,	laser_from, cameraPos);	
	MUmVector_Subtract(contrailDir, laser_to,	laser_from);

	// the contrail is still oriented from one point towards the other, but rotates to face the camera (i.e. perpendicular to cameraDir)
	MUrVector_CrossProduct(&contrailDir, &cameraDir, &contrailWidth);

	scale			= FXgLaser_Width;
	width			= MUmVector_GetLength(contrailWidth);
	width			= UUmMax( width, 0.001f );

	MUmVector_Scale(contrailWidth, scale / width);

	color			= inColor & 0xFFFFFF;
	alpha			= (UUtUns8) ( inColor >> 24 );

	p1.position		= laser_from;
	p1.width		= contrailWidth;
	p1.tint			= color;
	p1.alpha		= alpha;

	p2.position		= laser_to;
	p2.width		= contrailWidth;
	p2.tint			= color;
	p2.alpha		= alpha;

	M3rContrail_Draw( FXgLaser_Texture, 0, 1, &p1, &p2 );

	M3rGeom_State_Pop();
}

void FXrDrawSingleCursor(M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor, M3tTextureMap *inTexture)
{
	float scale = 5.f;

	M3rGeom_State_Push();
	{
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Flat);
		M3rGeom_State_Commit();

		M3rSimpleSprite_Draw(inTexture, inTo, scale, scale, IMcShade_White, M3cMaxAlpha);
	}
	M3rGeom_State_Pop();

	return;
}

void FXrDrawCursorTunnel(M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor, M3tTextureMap *inTexture)
{
	float					dist;
	float					scale = .003f;
	M3tVector3D				sight_vector;
	M3tVector3D				position;
	M3tVector3D				up, right;
	M3tMatrix3x3			matrix;
	UUtUns32				alpha;
	float					max_distance;
	float					next_distance;

	MUrMatrix3x3_Identity(&matrix);

	MUmVector_Subtract( sight_vector, *inTo, *inFrom );
	
	max_distance = MUrPoint_Length( &sight_vector );

	MUmVector_Normalize( sight_vector );	

	up.x	= 0;
	up.y	= 1;
	up.z	= 0;

	MUrVector_CrossProduct( &sight_vector, &up,		&right );
	MUrVector_CrossProduct( &sight_vector, &right,	&up );

	MUmVector_Normalize(right);	
	MUmVector_Normalize(up);	

	matrix.m[0][0]	= right.x;
	matrix.m[0][1]	= right.y;
	matrix.m[0][2]	= right.z;
	matrix.m[2][0]	= up.x;
	matrix.m[2][1]	= up.y;
	matrix.m[2][2]	= up.z;

	dist			= UUmFeetToUnits( 2 );
	alpha			= M3cMaxAlpha;
	next_distance	= dist;

	M3rGeom_State_Push();
	{
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Flat);
		M3rGeom_State_Commit();

		scale			= 5.0f;
		while(1)
		{
			float	temp;
			next_distance	+= UUmFeetToUnits( 12 );

			position.x		= inFrom->x + sight_vector.x * dist;		
			position.y		= inFrom->y + sight_vector.y * dist;		
			position.z		= inFrom->z + sight_vector.z * dist;

			if (next_distance >= max_distance) {
				break;
			}
			
			M3rSimpleSprite_Draw(inTexture, &position, scale, scale, IMcShade_Red, (UUtUns16) alpha);
			alpha = MUrUnsignedSmallFloat_To_Uns_Round(alpha * 0.85f);

			if (alpha < (M3cMaxAlpha * 0.1f)) {
				break;
			}
			
			temp			= dist;
			dist			= next_distance;
			next_distance	= temp;
		};

		M3rSimpleSprite_Draw(inTexture, inTo, scale, scale, IMcShade_White, M3cMaxAlpha);
		scale *= 0.75f;

		M3rSimpleSprite_Draw(inTexture, inTo, scale, scale, IMcShade_White, M3cMaxAlpha);

	}

	M3rGeom_State_Pop();
}



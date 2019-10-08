/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef R_LOCAL_H
#define R_LOCAL_H

// Enable render features
#define FEATURE_SKY_CUBEMAP 0
#define FEATURE_WATER_WARP 0
#define FEATURE_PARTICLES 0
#define FEATURE_SCREENSHOT 0
#define FEATURE_BEAM 0
#define FEATURE_LIGHT 0
#define FEATURE_ALIAS_MODEL 1
#define FEATURE_BRUSH_MODEL 1
#define FEATURE_SPRITE_MODEL 1
#define FEATURE_NULL 1
#define FEATURE_FADE_SCREEN 1
#define FEATURE_CINEMATIC 1
#define FEATURE_DRAW_FILL 1
#define FEATURE_DRAW_TEXT 1
#define FEATURE_DRAW_PICTURES 1

#define DX11_IMPL 0


#include <windows.h>
#include <stdio.h>
#include <math.h>

#include <d3d11.h>
#include <d3d12.h>

#define RENDERER
#include "../DirectQII/ref.h"

// we load shaders from embedded resources in the executable, but we can't embed resources in a static lib so we must embed them in the engine project instead
// this is OK because the only reason we're using a static lib is to ensure we enforce code separation
#include "../DirectQII/resource.h"

#include "matrix.h"

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------

// different viewport
void R_Set2D (void);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// world transforms
extern QMATRIX	r_view_matrix;
extern QMATRIX	r_proj_matrix;
extern QMATRIX	r_gun_matrix;
extern QMATRIX	r_mvp_matrix;
extern QMATRIX	r_local_matrix[MAX_ENTITIES];

void R_PrepareAliasModel (entity_t *e, QMATRIX *localmatrix);
void R_PrepareBrushModel (entity_t *e, QMATRIX *localmatrix);
void R_PrepareSpriteModel (entity_t *e, QMATRIX *localmatrix);
void R_PrepareBeam (entity_t *e, QMATRIX *localmatrix);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// module registration and object lifecycle
#ifdef __cplusplus
extern "C" {
#endif
void R_InitMain (void);
//void R_InitShaders (void);
//void R_ShutdownShaders (void);
void R_InitSurfaces (void);
void R_ShutdownSurfaces (void);
void R_InitParticles (void);
void R_InitSprites (void);
void R_ShutdownSprites (void);
void R_InitLight (void);
void R_ShutdownLight (void);
void R_InitWarp (void);
void R_InitSky (void);
void R_ShutdownWarp (void);
void R_ShutdownSky (void);
void R_InitMesh (void);
void R_ShutdownMesh (void);
void R_InitBeam (void);
void R_ShutdownBeam (void);
void R_InitNull (void);
#ifdef __cplusplus
}
#endif
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// helper macros
#define VDECL(name, index, fmt, slot, steprate) { \
	name, \
	index, \
	fmt, \
	slot, \
	D3D11_APPEND_ALIGNED_ELEMENT, \
	((steprate > 0) ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA), \
	steprate \
}

#define DEFINE_LAYOUT(lo) lo, sizeof (lo) / sizeof (lo[0])
#define SAFE_RELEASE(obj) {if (obj) {obj->lpVtbl->Release (obj); obj = NULL;}}



#define	REF_VERSION	"D3D 11"


extern	viddef_t	vid;


/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

  */

typedef enum _imagetype_t {
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_charset
} imagetype_t;

typedef struct image_s {
	char	name[MAX_QPATH];			// game path, including extension
	int		flags;						// loading flags
	int		texinfoflags;				// flags from texinfo object for wall textures
	int		width, height;				// source image
	int		registration_sequence;		// 0 = free

	// color that was used in radiosity calcs
	float	color[3];

	// chained surfaces for drawing
	// the same image_t may be used by multiple texinfo, so by storing the chain here we can get larger batches == fewer draw calls
	struct msurface_s	*texturechain;
#if DX11_IMPL
	// D3D texture object
	ID3D11Texture2D *Texture;
	ID3D11ShaderResourceView *SRV;
#else // DX12
    int textureId;
#endif // DX11_IMPL
} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	1024


//===================================================================

typedef enum _rserr_t
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "r_model.h"

//void R_SetDefaultState (void);

#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01


//====================================================


extern	image_t		*r_notexture;
extern	image_t		*r_blacktexture;
extern	image_t		*r_greytexture;
extern	image_t		*r_whitetexture;

extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;


//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;

//
// screen size info
//
extern	refdef_t	r_newrefdef;

extern	cvar_t	*scr_viewsize;
extern	cvar_t	*r_testnullmodels;
extern	cvar_t	*r_lightmap;
extern	cvar_t	*r_testnotexture;
extern	cvar_t	*r_lightmodel;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_beamdetail;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_novis;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern	cvar_t	*r_desaturatelighting;
extern	cvar_t	*vid_mode;

extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_polyblend;
extern  cvar_t  *gl_lockpvs;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;
extern	cvar_t	*vid_brightness;

extern	cvar_t	*vid_width;
extern	cvar_t	*vid_height;
extern	cvar_t	*vid_vsync;

extern	cvar_t	*intensity;


void R_RegeneratePVS (void);

void R_TranslatePlayerSkin (int playernum);
void GL_TexEnv (int value);

void R_LightPoint (vec3_t p, vec3_t color);
void R_DynamicLightPoint (vec3_t p, vec3_t color);

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table_solid[256];
extern	unsigned	d_8to24table_alpha[256];
extern	unsigned	d_8to24table_trans33[256];
extern	unsigned	d_8to24table_trans66[256];

extern	int		r_registration_sequence;


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int R_Init (void *hinstance, void *wndproc);
void R_Shutdown (void);

void R_DrawAliasModel (entity_t *e, QMATRIX *localmatrix);
void R_DrawBrushModel (entity_t *e, QMATRIX *localmatrix);
void R_DrawSpriteModel (entity_t *e, QMATRIX *localmatrix);
void R_DrawBeam (entity_t *e, QMATRIX *localmatrix);
void R_DrawWorld (void);
void R_DrawAlphaSurfaces (void);
void R_CreateSpecialTextures (void);
void Draw_InitLocal (void);

qboolean R_CullBox (vec3_t mins, vec3_t maxs);
qboolean R_CullBoxClipflags (vec3_t mins, vec3_t maxs, int clipflags);
qboolean R_CullSphere (float *center, float radius);
qboolean R_CullForEntity (vec3_t mins, vec3_t maxs, QMATRIX *localmatrix);

void R_MarkLeaves (void);

void COM_StripExtension (char *in, char *out);

qboolean Draw_GetPicSize (int *w, int *h, char *name);
void Draw_StretchPic (int x, int y, int w, int h, char *pic);
void Draw_Pic (int x, int y, char *name);
void Draw_ConsoleBackground (int x, int y, int w, int h, char *pic, int alpha);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void Draw_StretchRaw (int cols, int rows, byte *data, int frame, const unsigned char *palette);
void Draw_ShutdownRawImage (void);

void Draw_Char (int x, int y, int num);
void Draw_Field (int x, int y, int color, int width, int value);

int Draw_GetPalette (void);

image_t *R_RegisterSkin (char *name);
image_t *GL_FindImage (char *name, imagetype_t type);
image_t *GL_LoadWal (char *name, int flags);

void R_InitImages (void);
void R_ShutdownImages (void);

void R_FreeUnusedImages (void);


/*
===============
GL config stuff
===============
*/

typedef struct glconfig_s
{
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;

	qboolean	allow_cds;
} glconfig_t;

typedef struct glstate_s
{
	int     prev_mode;
} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void GLimp_BeginFrame (viddef_t *vd, int scrflags);
void GLimp_EndFrame (int scrflags);
//int GLimp_Init (void *hinstance, void *wndproc);
//void GLimp_Shutdown (void);
int GLimp_SetMode (int *pwidth, int *pheight, int mode, qboolean fullscreen);
void GLimp_AppActivate (qboolean active);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// draw
void Draw_UpdateConstants (int scrflags);
void Draw_Flush (void);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// shaders
//int D_CreateShaderBundle (int resourceID, const char *vsentry, const char *gsentry, const char *psentry, D3D11_INPUT_ELEMENT_DESC *layout, int numlayout);

//void D_BindShaderBundle (int sb);
//void D_RegisterConstantBuffer (ID3D11Buffer *cBuffer, int slot);
//void D_BindConstantBuffers (void);
#if !DX11_IMPL
void R_UpdateEntityShader(int stateId, int rflags);
#endif
void R_PrepareEntityForRendering (QMATRIX *localMatrix, float *color, float alpha, int rflags);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// states
typedef enum BSEnum
{
    BSNone,
    BSAlphaBlend,
    BSAlphaReverse,
    BSAlphaPreMult,
    BSAdditive,
    BSRevSubtract,

    BS_COUNT
} BlendState;

typedef enum DSEnum
{
    DSFullDepth,
    DSDepthNoWrite,
    DSNoDepth,
    DSEqualDepthNoWrite,

    DS_COUNT
} DepthStencilState;

typedef enum RSEnum
{
    RSFullCull,
    RSReverseCull,
    RSNoCull,

    RS_COUNT
} RasterizerState;

typedef enum ParameterIdxEnum
{
    SRV_TABLE_IDX = 0,

    SAMPLERS_TABLE_IDX,

    CB0_IDX,
    CB1_IDX,
    CB2_IDX,
    CB3_IDX,
    CB4_IDX,
    CB5_IDX,
    CB6_IDX,
    CB7_IDX,
    CB8_IDX,
    CB9_IDX_MAX,

    ROOT_PARAMS_COUNT
} ParameterIdx;

typedef enum ShaderTypeEnum
{
    SHADER_UNDEFINED = 0,

    SHADER_DRAW_POLYBLEND_VS,
    SHADER_DRAW_POLYBLEND_PS,

    SHADER_DRAW_TEXTURED_VS,
    SHADER_DRAW_TEXTURED_PS,

    SHADER_DRAW_COLOURED_VS,
    SHADER_DRAW_COLOURED_PS,

    SHADER_DRAW_TEXT_ARRAY_VS,
    SHADER_DRAW_TEXT_ARRAY_PS,

    SHADER_DRAW_CINEMATIC_VS,
    SHADER_DRAW_CINEMATIC_PS,

    SHADER_FADE_SCREEN_VS,
    SHADER_FADE_SCREEN_PS,

    SHADER_MODEL_SPRITE_VS,
    SHADER_MODEL_SPRITE_PS,

    SHADER_NULL_VS,
    SHADER_NULL_GS,
    SHADER_NULL_PS,

    SHADER_MODEL_SURFACE_BASIC_VS,
    SHADER_MODEL_SURFACE_BASIC_PS,

    SHADER_MODEL_SURFACE_ALPHA_VS,
    SHADER_MODEL_SURFACE_ALPHA_PS,

    SHADER_MODEL_SURFACE_LIGHTMAP_VS,
    SHADER_MODEL_SURFACE_LIGHTMAP_PS,

    SHADER_MODEL_SURFACE_DRAWTURB_VS,
    SHADER_MODEL_SURFACE_DRAWTURB_PS,

    SHADER_MODEL_SURFACE_DYNAMIC_VS,
    SHADER_MODEL_SURFACE_DYNAMIC_GS,
    SHADER_MODEL_GENERIC_DYNAMIC_PS,

    SHADER_MODEL_MESH_LIGHTMAP_VS,
    SHADER_MODEL_MESH_FULLBRIGHT_PS,

    SHADER_TYPE_COUNT
} ShaderType;

typedef enum SamplerStateEnum
{
    SAMPLER_MAIN,
    SAMPLER_L_MAP,
    SAMPLER_WARP,
    SAMPLER_DRAW,
    SAMPLER_CINE,

    SAMPLER_COUNT
} SamplerState;

typedef enum InputLayoutEnum
{
    INPUT_LAYOUT_BEAM = 0,
    INPUT_LAYOUT_STANDART,
    INPUT_LAYOUT_STANDART_COPY,
    INPUT_LAYOUT_TEXARRAY,
    INPUT_LAYOUT_MESH,
    INPUT_LAYOUT_PARTICLES,
    INPUT_LAYOUT_SKY,
    INPUT_LAYOUT_SPRITES,
    INPUT_LAYOUT_SURFACES,

    INPUT_LAYOUT_COUNT
} InputLayout;

typedef struct State
{
    InputLayout inputLayout;

    ShaderType VS;
    ShaderType GS;
    ShaderType PS;

    BlendState BS;
    DepthStencilState DS;
    RasterizerState RS;

    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology;
} State;

#ifndef __cplusplus
#define SELECT_RASTERIZER( FLAGS ) ((((FLAGS) & RF_WEAPONMODEL) && r_lefthand->value) ? RSReverseCull : RSFullCull )
#endif // __cplusplus

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// textures
// a lot of these flags are legacy from Q1/H2 code
#define TEX_RGBA8			(0 << 0) // all textures are created as RGBA format unless explicitly specified otherwise
#define TEX_MIPMAP			(1 << 0) // creates a full mipmap chain
#define TEX_ALPHA			(1 << 1) // standard alphs channel with alpha edge fix
#define TEX_TRANSPARENT		(1 << 3) // special Hexen 2 transparency type
#define TEX_HOLEY			(1 << 4) // special Hexen 2 transparency type
#define TEX_SPECIAL_TRANS	(1 << 5) // special Hexen 2 transparency type
#define TEX_CUBEMAP			(1 << 7) // creates a cubemap (not used by this Hexen 2 engine)
#define TEX_STAGING			(1 << 8) // creates the texture in the staging pool
#define TEX_R32F			(1 << 9) // single-channel 32-bit floating-point texture
#define TEX_R16G16			(1 << 10) // 2-channel 16-bit per channel signed-normalized format for use with noise
#define TEX_PLAYERTEXTURE	(1 << 11) // used for player colour translations
#define TEX_UPSCALE			(1 << 12) // texture was upscaled
#define TEX_CHARSET			(1 << 13) // charset uses a 16x16 texture array
#define TEX_MUTABLE			(1 << 14) // textures are immutable unless this flag is specified
#define TEX_TRANS33			(1 << 15) // 0.333 alpha encoded into the texture
#define TEX_TRANS66			(1 << 16) // 0.666 alpha encoded into the texture

// if any of these are set the texture may be processed for having an alpha channel
#define TEX_ANYALPHA		(TEX_ALPHA | TEX_TRANSPARENT | TEX_HOLEY | TEX_SPECIAL_TRANS | TEX_TRANS33 | TEX_TRANS66)

// textures marked disposable may be flushed on map changes
#define TEX_DISPOSABLE		(1 << 30)

#if DX11_IMPL
void R_BindTexture (ID3D11ShaderResourceView *SRV);
void R_BindTexArray (ID3D11ShaderResourceView *SRV);
#else // DX12
void R_BindTexture(int resId);
void R_BindTexArray(int resId);
#endif // DX11_IMPL
image_t *R_LoadTexArray (char *base);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// images

#pragma pack(push, 1)
typedef struct _TargaHeader
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;
#pragma pack(pop)

byte *Image_LoadTGA (char *name, int *width, int *height);
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);

unsigned *Image_ResampleToSize (unsigned *in, int inwidth, int inheight, int outwidth, int outheight);
unsigned *Image_MipReduceLinearFilter (unsigned *in, int inwidth, int inheight);
unsigned *Image_MipReduceBoxFilter (unsigned *data, int width, int height);

void R_FloodFillSkin (byte *skin, int skinwidth, int skinheight);
unsigned *GL_Image8To32 (byte *data, int width, int height, unsigned *palette);

void Image_QuakePalFromPCXPal (unsigned *qpal, const byte *pcxpal, int flags);

byte *Image_Upscale8 (byte *in, int inwidth, int inheight);
unsigned *Image_Upscale32 (unsigned *in, int inwidth, int inheight);

// helpers/etc
void Image_CollapseRowPitch (unsigned *data, int width, int height, int pitch);
void Image_Compress32To24 (byte *data, int width, int height);
void Image_Compress32To24RGBtoBGR (byte *data, int width, int height);
void Image_WriteDataToTGA (char *name, void *data, int width, int height, int bpp);

// gamma
byte Image_GammaVal8to8 (byte val, float gamma);
unsigned short Image_GammaVal8to16 (byte val, float gamma);
byte Image_GammaVal16to8 (unsigned short val, float gamma);
unsigned short Image_GammaVal16to16 (unsigned short val, float gamma);
void Image_ApplyTranslationRGB (byte *rgb, int size, byte *table);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// lights
void R_BindLightmaps (void);
void D_SetupDynamicLight (dlight_t *dl, float *transformedorigin, int rflags);
void R_DrawDlightChains (entity_t *e, model_t *mod, QMATRIX *localmatrix);
void R_PushDlights (mnode_t *headnode, entity_t *e, model_t *mod, QMATRIX *localmatrix, int visframe);

extern int	r_dlightframecount;


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// surfaces
void R_AddSurfaceToBatch (const msurface_t *surf);
void R_EndSurfaceBatch (void);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// underwater warp
qboolean D_BeginWaterWarp (void);
void D_DoWaterWarp (void);
void R_Clear (ID3D11RenderTargetView *RTV, ID3D11DepthStencilView *DSV);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// mesh
void R_FreeUnusedAliasBuffers (void);
void D_MakeAliasBuffers (model_t *mod, dmdl_t *src);
void D_RegisterAliasBuffers (model_t *mod);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// sprites
void D_MakeSpriteBuffers (model_t *mod);
void R_FreeUnusedSpriteBuffers (void);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// memory
extern HANDLE hRefHeap;


// null model
void R_DrawNullModel (entity_t *e, QMATRIX *localmatrix);
void R_PrepareNullModel (entity_t *e, QMATRIX *localmatrix);


// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// more texture stuff
typedef struct rendertarget_s {
	ID3D11Texture2D *Texture;
	ID3D11ShaderResourceView *SRV;
	ID3D11RenderTargetView *RTV;
	D3D11_TEXTURE2D_DESC Desc;
} rendertarget_t;

void R_CreateRenderTarget (rendertarget_t *rt);
void R_ReleaseRenderTarget (rendertarget_t *rt);

#if DX11_IMPL
typedef struct texture_s {
	ID3D11Texture2D *Texture;
	ID3D11ShaderResourceView *SRV;
	D3D11_TEXTURE2D_DESC Desc;
} texture_t;

void R_CreateTexture (texture_t *t, D3D11_SUBRESOURCE_DATA *srd, int width, int height, int arraysize, int flags);
void R_ReleaseTexture (texture_t *t);
#else // DX12
typedef struct texture_s {
    int Id;
    D3D12_RESOURCE_DESC Desc;
} texture_t;

void R_CreateTexture(texture_t* t, D3D12_SUBRESOURCE_DATA* srd, int width, int height, int arraysize, int flags);
void R_ReleaseTexture(texture_t* t);
#endif // DX11_IMPL


typedef struct tbuffer_s {
	ID3D11Buffer *Buffer;
	ID3D11ShaderResourceView *SRV;
} tbuffer_t;

void R_CreateTBuffer (tbuffer_t *tb, void *data, int NumElements, int ElementSize, DXGI_FORMAT Format, D3D11_USAGE Usage);
void R_ReleaseTBuffer (tbuffer_t *t);

void R_CopyScreen (rendertarget_t *dst);

#endif // R_LOCAL_H

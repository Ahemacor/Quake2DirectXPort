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

// draw.c

#include "r_local.h"
#if DX11_IMPL
#include "CppWrapper.h"
#else // DX12
#include "TestDirectX12.h"
#endif // DX11_IMPL


typedef struct drawpolyvert_s {
	float position[2];
	float texcoord[2];
	union {
		DWORD color;
		byte rgba[4];
		float slice;
	};
} drawpolyvert_t;


// these should be sized such that MAX_DRAW_INDEXES = (MAX_DRAW_VERTS / 4) * 6
#define MAX_DRAW_VERTS		0x400
#define MAX_DRAW_INDEXES	0x600

#if (MAX_DRAW_VERTS / 4) * 6 != MAX_DRAW_INDEXES
#error (MAX_DRAW_VERTS / 4) * 6 != MAX_DRAW_INDEXES
#endif

#if DX11_IMPL
static drawpolyvert_t* d_drawverts = NULL;
static ID3D11Buffer *d3d_DrawVertexes = NULL;
static ID3D11Buffer *d3d_DrawIndexes = NULL;
#else // DX12
static drawpolyvert_t vertex_buffer[MAX_DRAW_VERTS];
static drawpolyvert_t* d_drawverts = NULL;
static int d3d_DrawVertexes = -1;

static int index_buffer[MAX_DRAW_INDEXES];
static int d3d_DrawIndexes = -1;
#endif // DX11_IMPL

#if FEATURE_DRAW_PICTURES
static int d3d_DrawTexturedShader = -1;
#endif // #if FEATURE_DRAW_PICTURES

static int d_firstdrawvert = 0;
static int d_numdrawverts = 0;

#define STAT_MINUS		10	// num frame for '-' stats digit

static image_t	*draw_chars;
static image_t	*sb_nums[2];

#if FEATURE_CINEMATIC
static int d3d_DrawCinematicShader;
#endif // FEATURE_CINEMATIC

#if FEATURE_DRAW_FILL
static int d3d_DrawColouredShader;
#endif // FEATURE_DRAW_FILL

#if FEATURE_DRAW_TEXT
static int d3d_DrawTexArrayShader;
#endif // #if FEATURE_DRAW_TEXT

#if FEATURE_FADE_SCREEN
static int d3d_DrawFadescreenShader;
#endif // FEATURE_FADE_SCREEN

#if DX11_IMPL
static ID3D11Buffer *d3d_DrawConstants = NULL;
#else // DX12
static int d3d_DrawConstants = -1;
#endif // DX11_IMPL

__declspec(align(16)) typedef struct drawconstants_s {
	QMATRIX OrthoMatrix;
	float gamma;
	float brightness;
	float ConScale[2]; // conw/w, conh/h
} drawconstants_t;


/*
// TEST -----------------------------------
typedef struct VertexStruct
{
    float position[4];
    float texcoord[2];
    float color[4];
} Vertex;

static const Vertex vertices[4] = {
    // Upper Left
    { { -1.0f, 1.0f, 0, 1 }, { 0, 0 }, {1, 0, 0, 1} },
    // Upper Right
    { { 1.0f, 1.0f, 0, 1 }, { 1, 0 }, {0, 1, 0, 1} },
    // Bottom right
    { { 1.0f, -1.0f, 0, 1 }, { 1, 1 }, {0, 0, 1, 1} },
    // Bottom left
    { { -1.0f, -1.0f, 0, 1 }, { 0, 1 }, {1, 1, 0, 1} }
};

//static const int indices[6] = { 0, 1, 2, 2, 3, 0 };
static const int indices[6] = { 0, 1, 2, 3, 4, 5 };

int testVertBuffer = -1;
int testIndxBuffer = -1;
int testRenderState = -1;

// TEST -----------------------------------
*/

void Draw_CreateBuffers (void)
{
#if DX11_IMPL
	D3D11_BUFFER_DESC vbDesc = {
		sizeof (drawpolyvert_t) * MAX_DRAW_VERTS,
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};

	D3D11_BUFFER_DESC ibDesc = {
		sizeof (unsigned short) * MAX_DRAW_INDEXES,
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_INDEX_BUFFER,
		0,
		0,
		0
	};

	int i;
	unsigned short* ndx = ri.Load_AllocMemory (sizeof (unsigned short) * MAX_DRAW_INDEXES);
    unsigned short* ndxCurrent = ndx;

	for (i = 0; i < MAX_DRAW_VERTS; i += 4, ndxCurrent += 6)
	{
        ndxCurrent[0] = i + 0;
        ndxCurrent[1] = i + 1;
        ndxCurrent[2] = i + 2;

        ndxCurrent[3] = i + 0;
        ndxCurrent[4] = i + 2;
        ndxCurrent[5] = i + 3;
	}

    RWCreateBuffer(&vbDesc, NULL, &d3d_DrawVertexes);
    RWCreateBuffer(&ibDesc, ndx, &d3d_DrawIndexes);
    ri.Load_FreeMemory();
#else // DX12
    unsigned int* ndxCurrent = index_buffer;

    for (int i = 0; i < MAX_DRAW_VERTS; i += 4, ndxCurrent += 6)
    {
        ndxCurrent[0] = i + 0;
        ndxCurrent[1] = i + 1;
        ndxCurrent[2] = i + 2;

        ndxCurrent[3] = i + 0;
        ndxCurrent[4] = i + 2;
        ndxCurrent[5] = i + 3;
    }

    d3d_DrawVertexes = DX12_CreateVertexBuffer(MAX_DRAW_VERTS, sizeof(drawpolyvert_t), NULL);
    d3d_DrawIndexes = DX12_CreateIndexBuffer(MAX_DRAW_INDEXES, index_buffer, sizeof(int));
/*
// TEST -----------------------------------
    testVertBuffer = DX12_CreateVertexBuffer(sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), vertices);
    testIndxBuffer = DX12_CreateIndexBuffer(sizeof(indices) / sizeof(int), indices, sizeof(int));

    State testState;
    testState.inputLayout = INPUT_LAYOUT_STANDART;
    testState.VS = SHADER_DRAW_TEXTURED_VS;
    testState.PS = SHADER_DRAW_TEXTURED_PS;
    testState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    testState.BS = BSAlphaBlend;
    testState.DS = DSDepthNoWrite;
    testState.RS = RSNoCull;

    testRenderState = DX12_CreateRenderState(&testState);
// TEST -----------------------------------
*/
#endif // DX11_IMPL
}


/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{

	D3D11_BUFFER_DESC cbDrawDesc = {
		sizeof (drawconstants_t),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	D3D11_BUFFER_DESC cbCineDesc = {
		sizeof (QMATRIX),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_CONSTANT_BUFFER,
		0,
		0,
		0
	};

	D3D11_INPUT_ELEMENT_DESC layout_standard[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0),
		VDECL ("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0),
		VDECL ("COLOUR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0)
	};

	D3D11_INPUT_ELEMENT_DESC layout_texarray[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0),
		VDECL ("TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0)
	};

#if DX11_IMPL
	// cbuffers
    RWCreateBuffer(&cbDrawDesc, NULL, &d3d_DrawConstants);
    SLRegisterConstantBuffer(d3d_DrawConstants, 0);
#else // DX12
    d3d_DrawConstants = DX12_CreateConstantBuffer(NULL, sizeof(drawconstants_t));
    DX12_BindConstantBuffer(d3d_DrawConstants, 0);
#endif // DX11_IMPL

	// shaders
#if FEATURE_DRAW_PICTURES
#if DX11_IMPL
	d3d_DrawTexturedShader = SLCreateShaderBundle(IDR_DRAWSHADER, "DrawTexturedVS", NULL, "DrawTexturedPS", DEFINE_LAYOUT (layout_standard));
#else // DX12
    State TexturedState;
    TexturedState.inputLayout = INPUT_LAYOUT_STANDART;
    TexturedState.VS = SHADER_DRAW_TEXTURED_VS;
    TexturedState.PS = SHADER_DRAW_TEXTURED_PS;
    TexturedState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    TexturedState.BS = BSAlphaPreMult;
    TexturedState.DS = DSNoDepth;
    TexturedState.RS = RSNoCull;
    d3d_DrawTexturedShader = DX12_CreateRenderState(&TexturedState);
#endif // DX11_IMPL
#endif // #if FEATURE_DRAW_PICTURES

#if FEATURE_DRAW_FILL
	d3d_DrawColouredShader = SLCreateShaderBundle(IDR_DRAWSHADER, "DrawColouredVS", NULL, "DrawColouredPS", DEFINE_LAYOUT (layout_standard));
#endif // #if FEATURE_DRAW_FILL

#if FEATURE_DRAW_TEXT
	d3d_DrawTexArrayShader = SLCreateShaderBundle(IDR_DRAWSHADER, "DrawTexArrayVS", NULL, "DrawTexArrayPS", DEFINE_LAYOUT (layout_texarray));
#endif // #if FEATURE_DRAW_TEXT

#if FEATURE_CINEMATIC
	d3d_DrawCinematicShader = SLCreateShaderBundle(IDR_DRAWSHADER, "DrawCinematicVS", NULL, "DrawCinematicPS", DEFINE_LAYOUT (layout_standard));
#endif // FEATURE_CINEMATIC

	// shaders for use without buffers
#if FEATURE_FADE_SCREEN
	d3d_DrawFadescreenShader = SLCreateShaderBundle(IDR_DRAWSHADER, "DrawFadescreenVS", NULL, "DrawFadescreenPS", NULL, 0);
#endif // FEATURE_FADE_SCREEN

	// vertex and index buffers
	Draw_CreateBuffers ();

	// load console characters
	draw_chars = GL_FindImage ("pics/conchars.pcx", it_charset);
	sb_nums[0] = R_LoadTexArray ("num");
	sb_nums[1] = R_LoadTexArray ("anum");
}


void Draw_UpdateConstants (int scrflags)
{
	__declspec(align(16)) drawconstants_t consts;

	R_MatrixIdentity (&consts.OrthoMatrix);
	R_MatrixOrtho (&consts.OrthoMatrix, 0, vid.conwidth, vid.conheight, 0, -1, 1);

	if (scrflags & SCR_NO_GAMMA)
		consts.gamma = 1.0f;
	else consts.gamma = vid_gamma->value;

	if (scrflags & SCR_NO_BRIGHTNESS)
		consts.brightness = 1.0f;
	else consts.brightness = vid_brightness->value;

	consts.ConScale[0] = (float) vid.conwidth / (float) vid.width;
	consts.ConScale[1] = (float) vid.conheight / (float) vid.height;

#if DX11_IMPL
    RWGetDeviceContext()->lpVtbl->UpdateSubresource(RWGetDeviceContext(), (ID3D11Resource*)d3d_DrawConstants, 0, NULL, &consts, 0, 0);
#else // DX12
    DX12_UpdateConstantBuffer(d3d_DrawConstants, &consts, sizeof(drawconstants_t));
#endif // DX11_IMPL
}


void Draw_Flush (void)
{
#if DX11_IMPL
	if (d_drawverts)
	{
        RWGetDeviceContext()->lpVtbl->Unmap(RWGetDeviceContext(), (ID3D11Resource*)d3d_DrawVertexes, 0);
		d_drawverts = NULL;
	}

	SMBindVertexBuffer (0, d3d_DrawVertexes, sizeof (drawpolyvert_t), 0);

	if (d_numdrawverts == 3)
	{
        RWGetDeviceContext()->lpVtbl->Draw(RWGetDeviceContext(), d_numdrawverts, d_firstdrawvert);
	}
	else if (d_numdrawverts > 3)
	{
		SMBindIndexBuffer (d3d_DrawIndexes, DXGI_FORMAT_R16_UINT);
        RWGetDeviceContext()->lpVtbl->DrawIndexed(RWGetDeviceContext(), (d_numdrawverts >> 2) * 6, 0, d_firstdrawvert);
	}
#else // DX12
    if (d_drawverts)
    {
        DX12_UpdateVertexBuffer(d3d_DrawVertexes, vertex_buffer, MAX_DRAW_VERTS, sizeof(drawpolyvert_t));
        d_drawverts = NULL;
    }

    if (d_numdrawverts == 3)
    {
        DX12_Draw(d_numdrawverts, d_firstdrawvert);
    }
    else if (d_numdrawverts > 3)
    {
        DX12_BindIndexBuffer(d3d_DrawIndexes);
        DX12_DrawIndexed((d_numdrawverts >> 2) * 6, 0, d_firstdrawvert);
    }
#endif // DX11_IMPL

	d_firstdrawvert += d_numdrawverts;
	d_numdrawverts = 0;
}


qboolean Draw_EnsureBufferSpace (void)
{
	if (d_firstdrawvert + d_numdrawverts + 4 >= MAX_DRAW_VERTS)
	{
		// if we run out of buffer space for the next quad we flush the batch and begin a new one
		Draw_Flush ();
		d_firstdrawvert = 0;
	}

#if DX11_IMPL
	if (!d_drawverts)
	{
		// first index is only reset to 0 if the buffer must wrap so this is valid to do
		D3D11_MAP mode = (d_firstdrawvert > 0) ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
		D3D11_MAPPED_SUBRESOURCE msr;

        if (FAILED(RWGetDeviceContext()->lpVtbl->Map(RWGetDeviceContext(), (ID3D11Resource*)d3d_DrawVertexes, 0, mode, 0, &msr)))
			return false;
		else d_drawverts = (drawpolyvert_t *) msr.pData + d_firstdrawvert;
	}
#else // DX12
    if (!d_drawverts)
    {
        d_drawverts = (drawpolyvert_t*)vertex_buffer + d_firstdrawvert;
    }
#endif // DX11_IMPL

	// all OK!
	return true;
}


void Draw_TexturedVertex (drawpolyvert_t *vert, float x, float y, DWORD color, float s, float t)
{
	vert->position[0] = x;
	vert->position[1] = y;

	vert->texcoord[0] = s;
	vert->texcoord[1] = t;

	vert->color = color;
}


void Draw_ColouredVertex (drawpolyvert_t *vert, float x, float y, DWORD color)
{
	vert->position[0] = x;
	vert->position[1] = y;

	vert->color = color;
}


void Draw_CharacterVertex (drawpolyvert_t *vert, float x, float y, float s, float t, float slice)
{
	vert->position[0] = x;
	vert->position[1] = y;

	vert->texcoord[0] = s;
	vert->texcoord[1] = t;

	vert->slice = slice;
}


void Draw_TexturedQuad (image_t *image, int x, int y, int w, int h, unsigned color)
{
#if FEATURE_DRAW_PICTURES
#if DX11_IMPL
    R_BindTexture(image->SRV);
    SLBindShaderBundle(d3d_DrawTexturedShader);
	SMSetRenderStates(BSAlphaPreMult, DSNoDepth, RSNoCull);
#else // DX12
    DX12_BindTexture(0, image->textureId);
    Dx12_SetRenderState(d3d_DrawTexturedShader);
#endif // DX11_IMPL

	if (Draw_EnsureBufferSpace ())
	{
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x, y, color, 0, 0);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x + w, y, color, 1, 0);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x + w, y + h, color, 1, 1);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x, y + h, color, 0, 1);

		Draw_Flush ();
	}

    //Dx12_SetRenderState(testRenderState);
    //DX12_BindVertexBuffer(0, testVertBuffer);
    //DX12_BindIndexBuffer(testIndxBuffer);
    //DX12_DrawIndexed(6, 0, 0);
    //DX12_Draw(6, 0);
#endif // #if FEATURE_DRAW_PICTURES
}


void Draw_CharacterQuad (int x, int y, int w, int h, int slice)
{
	// check for overflow
	if (Draw_EnsureBufferSpace ())
	{
		// and draw it
		Draw_CharacterVertex (&d_drawverts[d_numdrawverts++], x, y, 0, 0, slice);
		Draw_CharacterVertex (&d_drawverts[d_numdrawverts++], x + w, y, 1, 0, slice);
		Draw_CharacterVertex (&d_drawverts[d_numdrawverts++], x + w, y + h, 1, 1, slice);
		Draw_CharacterVertex (&d_drawverts[d_numdrawverts++], x, y + h, 0, 1, slice);
	}
}


void Draw_Field (int x, int y, int color, int width, int value)
{
#if FEATURE_DRAW_TEXT
	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	Com_sprintf (num, sizeof (num), "%i", value);
	l = strlen (num);

	if (l > width)
		l = width;

	x += 2 + sb_nums[color]->width * (width - l);
	ptr = num;

	R_BindTexArray (sb_nums[color]->SRV);

    SLBindShaderBundle(d3d_DrawTexArrayShader);
    SMSetRenderStates(BSAlphaPreMult, DSNoDepth, RSNoCull);

	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else frame = *ptr - '0';

		Draw_CharacterQuad (x, y, sb_nums[color]->width, sb_nums[color]->height, frame);

		x += sb_nums[color]->width;
		ptr++;
		l--;
	}

	Draw_Flush ();
#endif // #if FEATURE_DRAW_TEXT
}


/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
#if FEATURE_DRAW_TEXT
	// totally off screen
	if (y <= -8) return;

	// space
	if ((num & 127) == 32) return;

	// these are done for each char but they only trigger state changes for the first
	R_BindTexArray (draw_chars->SRV);

    SLBindShaderBundle(d3d_DrawTexArrayShader);
    SMSetRenderStates(BSAlphaPreMult, DSNoDepth, RSNoCull);

	Draw_CharacterQuad (x, y, 8, 8, num & 255);
#endif // #if FEATURE_DRAW_TEXT
}


/*
=============
Draw_FindPic
=============
*/
image_t	*Draw_FindPic (char *name)
{
	if (name[0] != '/' && name[0] != '\\')
		return GL_FindImage (va ("pics/%s.pcx", name), it_pic);
	else return GL_FindImage (name + 1, it_pic);
}

/*
=============
Draw_GetPicSize

modded to also allow the return value to be tested
=============
*/
qboolean Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl = Draw_FindPic (pic);

	if (!gl)
	{
		*w = *h = -1;
		return false;
	}

	*w = gl->width;
	*h = gl->height;

	return true;
}


/*
=============
Draw_Pic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *gl = Draw_FindPic (pic);

	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	Draw_TexturedQuad (gl, x, y, w, h, 0xffffffff);
}


void Draw_Pic (int x, int y, char *pic)
{
	image_t *gl = Draw_FindPic (pic);

	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	Draw_TexturedQuad (gl, x, y, gl->width, gl->height, 0xffffffff);
}


void Draw_ConsoleBackground (int x, int y, int w, int h, char *pic, int alpha)
{
    image_t* gl = Draw_FindPic (pic);

	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if (alpha >= 255)
		Draw_TexturedQuad (gl, x, y, w, h, 0xffffffff);
	else if (alpha > 0)
		Draw_TexturedQuad (gl, x, y, w, h, (alpha << 24) | 0xffffff);
}


void Draw_Fill (int x, int y, int w, int h, int c)
{
#if FEATURE_DRAW_FILL
	// this is a quad filled with a single solid colour so it doesn't need to blend
    SLBindShaderBundle(d3d_DrawColouredShader);
    SMSetRenderStates(BSNone, DSNoDepth, RSNoCull);

	if (Draw_EnsureBufferSpace ())
	{
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x, y, d_8to24table_solid[c]);
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x + w, y, d_8to24table_solid[c]);
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x + w, y + h, d_8to24table_solid[c]);
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x, y + h, d_8to24table_solid[c]);

		Draw_Flush ();
	}
#endif // #if FEATURE_DRAW_FILL
}


//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
#if FEATURE_FADE_SCREEN
    SMSetRenderStates(BSAlphaPreMult, DSDepthNoWrite, RSNoCull);
    SLBindShaderBundle(d3d_DrawFadescreenShader);

	// full-screen triangle
    RWGetDeviceContext()->lpVtbl->Draw(RWGetDeviceContext(), 3, 0);
#endif // FEATURE_FADE_SCREEN
}


//====================================================================


/*
=============
Draw_StretchRaw
=============
*/
#if FEATURE_CINEMATIC
texture_t r_CinematicPic;
#endif // #if FEATURE_CINEMATIC


void Draw_ShutdownRawImage (void)
{
#if FEATURE_CINEMATIC
	R_ReleaseTexture (&r_CinematicPic);
#endif // #if FEATURE_CINEMATIC
}


void R_TexSubImage32 (ID3D11Texture2D *tex, int level, int x, int y, int w, int h, unsigned *data);
void R_TexSubImage8 (ID3D11Texture2D *tex, int level, int x, int y, int w, int h, byte *data, unsigned *palette);

void Draw_StretchRaw (int cols, int rows, byte *data, int frame, const unsigned char *palette)
{
#if FEATURE_CINEMATIC
	// we only need to refresh the texture if the frame changes
	static int r_rawframe = -1;

	// if the dimensions change the texture needs to be recreated
	if (r_CinematicPic.Desc.Width != cols || r_CinematicPic.Desc.Height != rows)
		Draw_ShutdownRawImage ();

	if (!r_CinematicPic.SRV)
	{
		// ensure in case we got a partial creation
		Draw_ShutdownRawImage ();

		// and create it
		R_CreateTexture (&r_CinematicPic, NULL, cols, rows, 1, TEX_RGBA8 | TEX_MUTABLE);

		// load the image
		r_rawframe = -1;
	}

	// only reload the texture if the frame changes
	// in *theory* the original code allowed the palette to be changed independently of the texture, in practice the .cin format doesn't support this
	if (r_rawframe != frame)
	{
		if (palette)
		{
			unsigned r_rawpalette[256];
			Image_QuakePalFromPCXPal (r_rawpalette, palette, TEX_RGBA8);
			R_TexSubImage8 (r_CinematicPic.Texture, 0, 0, 0, cols, rows, data, r_rawpalette);
		}
		else R_TexSubImage8 (r_CinematicPic.Texture, 0, 0, 0, cols, rows, data, d_8to24table_solid);

		r_rawframe = frame;
	}

	// free any memory we may have used for loading it
	ri.Load_FreeMemory ();

	R_BindTexture (r_CinematicPic.SRV);

    SLBindShaderBundle(d3d_DrawCinematicShader);
    SMSetRenderStates(BSNone, DSNoDepth, RSNoCull);

	if (Draw_EnsureBufferSpace ())
	{
		// matrix transform for positioning the cinematic correctly
		// sampler state should be set to clamp-to-border with a border color of black
		float strans = 0.5f, ttrans = -0.5f;

		// derive the texture matrix for the cinematic pic
		if (vid.conwidth > vid.conheight)
			strans *= ((float) rows / (float) cols) * ((float) vid.conwidth / (float) vid.conheight);
		else
			ttrans *= ((float) cols / (float) rows) * ((float) vid.conheight / (float) vid.conwidth);

		// drawn without projection, full-screen triangle coords
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], -1, -1, 0xffffffff, strans, ttrans);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++],  3, -1, 0xffffffff, strans, ttrans);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], -1,  3, 0xffffffff, strans, ttrans);

		// always flush
		Draw_Flush ();
	}
#endif // #if FEATURE_CINEMATIC
}

void R_Set2D (void)
{
	// switch to our 2d viewport
	D3D11_VIEWPORT vp = {0, 0, vid.width, vid.height, 0, 0};
#if DX11_IMPL
    RWGetDeviceContext()->lpVtbl->RSSetViewports(RWGetDeviceContext(), 1, &vp);
#else // DX12
    DX12_SetViewport(&vp);
#endif // DX11_IMPL
}


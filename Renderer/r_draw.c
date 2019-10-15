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
#include "TestDirectX12.h"


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

static drawpolyvert_t vertex_buffer[MAX_DRAW_VERTS];
static drawpolyvert_t* d_drawverts = NULL;
static int d3d_DrawVertexes;

static int index_buffer[MAX_DRAW_INDEXES];
static int d3d_DrawIndexes;

static int d_firstdrawvert = 0;
static int d_numdrawverts = 0;

#define STAT_MINUS		10	// num frame for '-' stats digit

static image_t	*draw_chars;
static image_t	*sb_nums[2];

static int d3d_DrawTexturedShader;
static int d3d_DrawCinematicShader;
static int d3d_DrawColouredShader;
static int d3d_DrawTexArrayShader;
static int d3d_DrawFadescreenShader;
static int d3d_DrawConstants;

__declspec(align(16)) typedef struct drawconstants_s {
	QMATRIX OrthoMatrix;
	float gamma;
	float brightness;
	float ConScale[2]; // conw/w, conh/h
} drawconstants_t;

void Draw_CreateBuffers (void)
{
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
}


/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
    d3d_DrawConstants = DX12_CreateConstantBuffer(NULL, sizeof(drawconstants_t));
    DX12_BindConstantBuffer(d3d_DrawConstants, CB_SLOT_DRAW_PER_FRAME);

    State TexturedState;
    TexturedState.inputLayout = INPUT_LAYOUT_STANDART;
    TexturedState.VS = SHADER_DRAW_TEXTURED_VS;
    TexturedState.GS = SHADER_UNDEFINED;
    TexturedState.PS = SHADER_DRAW_TEXTURED_PS;
    TexturedState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    TexturedState.BS = BSAlphaPreMult;
    TexturedState.DS = DSNoDepth;
    TexturedState.RS = RSNoCull;
    d3d_DrawTexturedShader = DX12_CreateRenderState(&TexturedState);

    State ColouredState;
    ColouredState.inputLayout = INPUT_LAYOUT_STANDART;
    ColouredState.VS = SHADER_DRAW_COLOURED_VS;
    ColouredState.GS = SHADER_UNDEFINED;
    ColouredState.PS = SHADER_DRAW_COLOURED_PS;
    ColouredState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ColouredState.BS = BSNone;
    ColouredState.DS = DSNoDepth;
    ColouredState.RS = RSNoCull;
    d3d_DrawColouredShader = DX12_CreateRenderState(&ColouredState);

    State TextState;
    TextState.inputLayout = INPUT_LAYOUT_TEXARRAY;
    TextState.VS = SHADER_DRAW_TEXT_ARRAY_VS;
    TextState.GS = SHADER_UNDEFINED;
    TextState.PS = SHADER_DRAW_TEXT_ARRAY_PS;
    TextState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    TextState.BS = BSAlphaPreMult;
    TextState.DS = DSNoDepth;
    TextState.RS = RSNoCull;
    d3d_DrawTexArrayShader = DX12_CreateRenderState(&TextState);

    State CinematicState;
    CinematicState.inputLayout = INPUT_LAYOUT_STANDART;
    CinematicState.VS = SHADER_DRAW_CINEMATIC_VS;
    CinematicState.GS = SHADER_UNDEFINED;
    CinematicState.PS = SHADER_DRAW_CINEMATIC_PS;
    CinematicState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    CinematicState.BS = BSNone;
    CinematicState.DS = DSNoDepth;
    CinematicState.RS = RSNoCull;
    d3d_DrawCinematicShader = DX12_CreateRenderState(&CinematicState);

    State FadeScreenState;
    FadeScreenState.inputLayout = INPUT_LAYOUT_STANDART;
    FadeScreenState.VS = SHADER_FADE_SCREEN_VS;
    FadeScreenState.GS = SHADER_UNDEFINED;
    FadeScreenState.PS = SHADER_FADE_SCREEN_PS;
    FadeScreenState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    FadeScreenState.BS = BSAlphaPreMult;
    FadeScreenState.DS = DSDepthNoWrite;
    FadeScreenState.RS = RSNoCull;
    d3d_DrawFadescreenShader = DX12_CreateRenderState(&FadeScreenState);

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

    DX12_UpdateConstantBuffer(d3d_DrawConstants, &consts, sizeof(drawconstants_t));
}


void Draw_Flush (void)
{
    if (d_drawverts != NULL && d_numdrawverts > 0)
    {
        DX12_UpdateVertexBuffer(d3d_DrawVertexes, vertex_buffer, d_numdrawverts, d_firstdrawvert, sizeof(drawpolyvert_t));
        d_drawverts = NULL;
    }

    DX12_BindVertexBuffer(0, d3d_DrawVertexes, 0);

    if (d_numdrawverts == 3)
    {
        DX12_Draw(d_numdrawverts, d_firstdrawvert);
    }
    else if (d_numdrawverts > 3)
    {
        DX12_BindIndexBuffer(d3d_DrawIndexes);
        DX12_DrawIndexed((d_numdrawverts >> 2) * 6, 0, d_firstdrawvert);
    }

    if (d_numdrawverts > 0)
    {
        DX12_Execute();
        d_firstdrawvert += d_numdrawverts;
        d_numdrawverts = 0;
    }
}


qboolean Draw_EnsureBufferSpace (void)
{
	if (d_firstdrawvert + d_numdrawverts + 4 >= MAX_DRAW_VERTS)
	{
		// if we run out of buffer space for the next quad we flush the batch and begin a new one
		Draw_Flush ();
		d_firstdrawvert = 0;
	}

    if (!d_drawverts)
    {
        d_drawverts = (drawpolyvert_t*)vertex_buffer + d_firstdrawvert;
    }

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
    DX12_BindTexture(0, image->textureId);
    DX12_SetRenderState(d3d_DrawTexturedShader);

	if (Draw_EnsureBufferSpace ())
	{
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x, y, color, 0, 0);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x + w, y, color, 1, 0);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x + w, y + h, color, 1, 1);
		Draw_TexturedVertex (&d_drawverts[d_numdrawverts++], x, y + h, color, 0, 1);

		Draw_Flush ();
	}
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
    char	num[16], * ptr;
    int		l;
    int		frame;

    if (width < 1)
        return;

    // draw number string
    if (width > 5)
        width = 5;

    Com_sprintf(num, sizeof(num), "%i", value);
    l = strlen(num);

    if (l > width)
        l = width;

    x += 2 + sb_nums[color]->width * (width - l);
    ptr = num;

    R_BindTexArray(sb_nums[color]->textureId);
    DX12_SetRenderState(d3d_DrawTexArrayShader);

    while (*ptr && l)
    {
        if (*ptr == '-')
            frame = STAT_MINUS;
        else frame = *ptr - '0';

        Draw_CharacterQuad(x, y, sb_nums[color]->width, sb_nums[color]->height, frame);

        x += sb_nums[color]->width;
        ptr++;
        l--;
    }

    Draw_Flush();
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
	// totally off screen
	if (y <= -8) return;

	// space
	if ((num & 127) == 32) return;

    R_BindTexArray(draw_chars->textureId);
    DX12_SetRenderState(d3d_DrawTexArrayShader);

	Draw_CharacterQuad (x, y, 8, 8, num & 255);
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
	// this is a quad filled with a single solid colour so it doesn't need to blend
    DX12_SetRenderState(d3d_DrawColouredShader);

	if (Draw_EnsureBufferSpace ())
	{
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x, y, d_8to24table_solid[c]);
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x + w, y, d_8to24table_solid[c]);
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x + w, y + h, d_8to24table_solid[c]);
		Draw_ColouredVertex (&d_drawverts[d_numdrawverts++], x, y + h, d_8to24table_solid[c]);

		Draw_Flush ();
	}
}


//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
    // full-screen triangle
    DX12_SetRenderState(d3d_DrawFadescreenShader);
    DX12_Draw(3, 0);
}


//====================================================================


/*
=============
Draw_StretchRaw
=============
*/
texture_t r_CinematicPic;


void Draw_ShutdownRawImage (void)
{
	R_ReleaseTexture (&r_CinematicPic);
}

void Draw_StretchRaw (int cols, int rows, byte *data, int frame, const unsigned char *palette)
{
    // we only need to refresh the texture if the frame changes
    static int r_rawframe = -1;

    // if the dimensions change the texture needs to be recreated
    if (r_CinematicPic.Desc.Width != cols || r_CinematicPic.Desc.Height != rows)
        Draw_ShutdownRawImage();

    if (!r_CinematicPic.Id)
    {
        // ensure in case we got a partial creation
        Draw_ShutdownRawImage();

        // and create it
        R_CreateTexture(&r_CinematicPic, NULL, cols, rows, 1, TEX_RGBA8 | TEX_MUTABLE);

        // load the image
        r_rawframe = -1;
    }

    // only reload the texture if the frame changes
    // in *theory* the original code allowed the palette to be changed independently of the texture, in practice the .cin format doesn't support this
    if (r_rawframe != frame)
    {
        unsigned r_rawpalette[256];
        Image_QuakePalFromPCXPal(r_rawpalette, palette, TEX_RGBA8);

        unsigned* trans = GL_Image8To32(data, cols, rows, (palette) ? r_rawpalette : d_8to24table_solid);

        D3D12_SUBRESOURCE_DATA srd;
        srd.pData = trans;
        srd.RowPitch = cols << 2;
        srd.SlicePitch = 0;
        DX12_UpdateTexture(r_CinematicPic.Id, &srd);

        r_rawframe = frame;
    }

    // free any memory we may have used for loading it
    ri.Load_FreeMemory();

    //R_BindTexture(r_CinematicPic.Id);
    static int OldRes;
    if (OldRes != r_CinematicPic.Id)
    {
        DX12_BindTexture(10, r_CinematicPic.Id);
        OldRes = r_CinematicPic.Id;
    }

    //SLBindShaderBundle(d3d_DrawCinematicShader);
    DX12_SetRenderState(d3d_DrawCinematicShader);
    //SMSetRenderStates(BSNone, DSNoDepth, RSNoCull);

    if (Draw_EnsureBufferSpace())
    {
        // matrix transform for positioning the cinematic correctly
        // sampler state should be set to clamp-to-border with a border color of black
        float strans = 0.5f, ttrans = -0.5f;

        // derive the texture matrix for the cinematic pic
        if (vid.conwidth > vid.conheight)
            strans *= ((float)rows / (float)cols) * ((float)vid.conwidth / (float)vid.conheight);
        else
            ttrans *= ((float)cols / (float)rows) * ((float)vid.conheight / (float)vid.conwidth);

        // drawn without projection, full-screen triangle coords
        Draw_TexturedVertex(&d_drawverts[d_numdrawverts++], -1, -1, 0xffffffff, strans, ttrans);
        Draw_TexturedVertex(&d_drawverts[d_numdrawverts++], 3, -1, 0xffffffff, strans, ttrans);
        Draw_TexturedVertex(&d_drawverts[d_numdrawverts++], -1, 3, 0xffffffff, strans, ttrans);

        // always flush
        Draw_Flush();
    }
}

void R_Set2D (void)
{
	// switch to our 2d viewport
	D3D12_VIEWPORT vp = {0, 0, vid.width, vid.height, 0, 0};
    DX12_SetViewport(&vp);
}


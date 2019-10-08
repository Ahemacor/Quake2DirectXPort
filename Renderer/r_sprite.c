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

#include "r_local.h"

#if FEATURE_SPRITE_MODEL
#if DX11_IMPL
#include "CppWrapper.h"
#else // DX12
#include "TestDirectX12.h"
#endif // DX11_IMPL

typedef struct spritepolyvert_s {
	float XYOffset[2];
} spritepolyvert_t;

#if DX11_IMPL
typedef struct spritebuffers_s {
	ID3D11Buffer *PolyVerts;
	char Name[256];
	int registration_sequence;
} spritebuffers_t;
#else // DX12
typedef struct spritebuffers_s {
    int PolyVertsId;
    char Name[256];
    int registration_sequence;
} spritebuffers_t;
#endif // DX11_IMPL

static spritebuffers_t d3d_SpriteBuffers[MAX_MOD_KNOWN];


void R_FreeUnusedSpriteBuffers (void)
{
	int i;

	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		spritebuffers_t *set = &d3d_SpriteBuffers[i];

		if (set->registration_sequence != r_registration_sequence)
		{
#if DX11_IMPL
			SAFE_RELEASE (set->PolyVerts);
#endif // DX11_IMPL
			memset (set, 0, sizeof (spritebuffers_t));
		}
	}
}


static void D_CreateSpriteBufferSet (model_t *mod, dsprite_t *psprite)
{
#if DX11_IMPL
	int i;
	spritebuffers_t *set = &d3d_SpriteBuffers[mod->bufferset];
	spritepolyvert_t *verts = (spritepolyvert_t *) ri.Load_AllocMemory (sizeof (spritepolyvert_t) * 4 * psprite->numframes);

	D3D11_BUFFER_DESC vbDesc = {
		sizeof (spritepolyvert_t) * 4 * psprite->numframes,
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	// alloc a buffer to write the verts to and create the VB from
    void* pData = verts;

	// fill in the verts
	for (i = 0; i < psprite->numframes; i++, verts += 4)
	{
		dsprframe_t *frame = &psprite->frames[i];

		verts[0].XYOffset[0] = -frame->origin_y;
		verts[0].XYOffset[1] = -frame->origin_x;

		verts[1].XYOffset[0] = frame->height - frame->origin_y;
		verts[1].XYOffset[1] = -frame->origin_x;

		verts[2].XYOffset[0] = frame->height - frame->origin_y;
		verts[2].XYOffset[1] = frame->width - frame->origin_x;

		verts[3].XYOffset[0] = -frame->origin_y;
		verts[3].XYOffset[1] = frame->width - frame->origin_x;
	}

	// create the new vertex buffer
    RWCreateBuffer(&vbDesc, pData, &set->PolyVerts);
	ri.Load_FreeMemory ();
#else // DX12
    const int numVerts = 4;
    spritebuffers_t* set = &d3d_SpriteBuffers[mod->bufferset];
    spritepolyvert_t* verts = (spritepolyvert_t*)malloc(sizeof(spritepolyvert_t) * numVerts * psprite->numframes);

    // alloc a buffer to write the verts to and create the VB from
    void* pData = verts;

    // fill in the verts
    for (int i = 0; i < psprite->numframes; i++, verts += numVerts)
    {
        dsprframe_t* frame = &psprite->frames[i];

        verts[0].XYOffset[0] = -frame->origin_y;
        verts[0].XYOffset[1] = -frame->origin_x;

        verts[1].XYOffset[0] = frame->height - frame->origin_y;
        verts[1].XYOffset[1] = -frame->origin_x;

        verts[2].XYOffset[0] = frame->height - frame->origin_y;
        verts[2].XYOffset[1] = frame->width - frame->origin_x;

        verts[3].XYOffset[0] = -frame->origin_y;
        verts[3].XYOffset[1] = frame->width - frame->origin_x;
    }

    //RWCreateBuffer(&vbDesc, pData, &set->PolyVerts);
    set->PolyVertsId = DX12_CreateVertexBuffer(numVerts * psprite->numframes, sizeof(spritepolyvert_t), pData);
    free(pData);
#endif // DX11_IMPL
}


int D_FindSpriteBuffers (model_t *mod)
{
	int i;

	// see do we already have it
	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		spritebuffers_t *set = &d3d_SpriteBuffers[i];
#if DX11_IMPL
		if (!set->PolyVerts) continue;
#else // DX12
        if (!set->PolyVertsId) continue;
#endif // DX11_IMPL
		if (strcmp (set->Name, mod->name)) continue;

		// use this set and mark it as active
		mod->bufferset = i;
		set->registration_sequence = r_registration_sequence;

		return mod->bufferset;
	}

	return -1;
}


void D_MakeSpriteBuffers (model_t *mod)
{
	int i;

	// see do we already have it
	if ((mod->bufferset = D_FindSpriteBuffers (mod)) != -1) return;

	// find the first free buffer
	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		spritebuffers_t *set = &d3d_SpriteBuffers[i];

		// already allocated
#if DX11_IMPL
		if (set->PolyVerts) continue;
#else // DX12
        if (set->PolyVertsId) continue;
#endif // DX11_IMPL

		// cache the name so that we'll find it next time too
		strcpy (set->Name, mod->name);

		// use this set and mark it as active
		mod->bufferset = i;
		set->registration_sequence = r_registration_sequence;

		// now build everything from the model data
		D_CreateSpriteBufferSet (mod, mod->sprheader);

		// and done
		return;
	}

	ri.Sys_Error (ERR_DROP, "D_MakeSpriteBuffers : not enough free buffers!");
}


int d3d_SpriteShader;

#if DX11_IMPL
ID3D11Buffer *d3d_SpriteIndexes;
#else // DX12
int d3d_SpriteIndexes;
#endif // DX11_IMPL

void R_InitSprites (void)
{
#if DX11_IMPL
	// it's kinda slightly sucky that we need to create a new index buffer just for this.....
	unsigned short indexes[6] = {0, 1, 2, 0, 2, 3};
	D3D11_BUFFER_DESC ibDesc = {
		sizeof (indexes),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_INDEX_BUFFER,
		0,
		0,
		0
	};

	D3D11_INPUT_ELEMENT_DESC layout[] = {VDECL ("XYOFFSET", 0, DXGI_FORMAT_R32G32_FLOAT, 5, 0)};
	d3d_SpriteShader = SLCreateShaderBundle(IDR_SPRITESHADER, "SpriteVS", NULL, "SpritePS", DEFINE_LAYOUT (layout));
    RWCreateBuffer(&ibDesc, indexes, &d3d_SpriteIndexes);
#else // DX12
    UINT indexes[6] = { 0, 1, 2, 0, 2, 3 };

    State SpriteState;
    SpriteState.inputLayout = INPUT_LAYOUT_SPRITES;
    SpriteState.VS = SHADER_MODEL_SPRITE_VS;
    SpriteState.GS = SHADER_UNDEFINED;
    SpriteState.PS = SHADER_MODEL_SPRITE_PS;
    SpriteState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    SpriteState.BS = BSNone;
    SpriteState.DS = DSNoDepth;
    SpriteState.RS = RSNoCull;
    d3d_SpriteShader = DX12_CreateRenderState(&SpriteState);

    d3d_SpriteIndexes = DX12_CreateIndexBuffer(sizeof(indexes) / sizeof(UINT), indexes, sizeof(UINT));
#endif // DX11_IMPL
}


void R_ShutdownSprites (void)
{
	int i;

	for (i = 0; i < MAX_MOD_KNOWN; i++)
	{
		spritebuffers_t *set = &d3d_SpriteBuffers[i];
#if DX11_IMPL
		SAFE_RELEASE (set->PolyVerts);
#endif // DX11_IMPL
		memset (set, 0, sizeof (spritebuffers_t));
	}
}


void R_DrawSpriteModel (entity_t *e, QMATRIX *localmatrix)
{
	/*model_t *mod = e->model;

	// don't even bother culling, because it's just a single polygon without a surface cache
	// (note - with hardware it might make sense to cull)
	dsprite_t *psprite = mod->sprheader;
	int framenum = e->currframe % psprite->numframes;
	dsprframe_t *frame = &psprite->frames[framenum];

	// because there's buffer, shader and texture changes, as well as cbuffer updates, it's worth spending some CPU
	// time on cullboxing sprites
	float mins[3] = {
		e->currorigin[0] - frame->origin_x,
		e->currorigin[1] - frame->origin_y,
		e->currorigin[2] - 1 // so that the box isn't a 2d plane...
	};

	float maxs[3] = {
		e->currorigin[0] + frame->width - frame->origin_x,
		e->currorigin[1] + frame->height - frame->origin_y,
		e->currorigin[2] + 1 // so that the box isn't a 2d plane...
	};

	if (R_CullBox (mins, maxs)) return;

#if DX11_IMPL
	R_PrepareEntityForRendering (localmatrix, NULL, e->alpha, e->flags);
    SLBindShaderBundle(d3d_SpriteShader);
	R_BindTexture (mod->skins[framenum]->SRV);

	SMBindVertexBuffer (5, d3d_SpriteBuffers[mod->bufferset].PolyVerts, sizeof (spritepolyvert_t), 0);
	SMBindIndexBuffer (d3d_SpriteIndexes, DXGI_FORMAT_R16_UINT);

    RWGetDeviceContext()->lpVtbl->DrawIndexed(RWGetDeviceContext(), 6, 0, framenum * 4);
#else // DX12
    DX12_SetRenderState(d3d_SpriteShader);
    R_BindTexture(mod->skins[framenum]->textureId);
    R_PrepareEntityForRendering(localmatrix, NULL, e->alpha, e->flags);

    DX12_BindVertexBuffer(5, d3d_SpriteBuffers[mod->bufferset].PolyVertsId);
    DX12_BindIndexBuffer(d3d_SpriteIndexes);
    DX12_DrawIndexed(6, 0, framenum * 4);
#endif // DX11_IMPL*/
}


void R_PrepareSpriteModel (entity_t *e, QMATRIX *localmatrix)
{
	// get the transform in local space so that we can correctly handle dlights
	R_MatrixIdentity (localmatrix);
	R_MatrixTranslate (localmatrix, e->currorigin[0], e->currorigin[1], e->currorigin[2]);
}

#else 
void R_FreeUnusedSpriteBuffers(void) {}
void R_PrepareSpriteModel(entity_t* e, QMATRIX* localmatrix) {}
void R_DrawSpriteModel(entity_t* e, QMATRIX* localmatrix) {}
void R_InitSprites(void) {}
void R_ShutdownSprites(void) {}
#endif // FEATURE_SPRITE_MODEL


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
// GL_RSURF.C: surface-related refresh code
#include <assert.h>

#include "r_local.h"

#if FEATURE_BRUSH_MODEL
#if DX11_IMPL
#include "CppWrapper.h"
#else // DX12
#include "TestDirectX12.h"
#endif // DX11_IMPL

#define MAX_SURF_INDEXES	0x100000

static vec3_t	modelorg;		// relative to viewpoint

msurface_t	*r_sky_surfaces;
msurface_t	*r_alpha_surfaces;

void R_DrawSkyChain (msurface_t *surf);
void R_SetupLightmapTexCoords (msurface_t *surf, float *vec, unsigned short *lm);

#if DX11_IMPL
ID3D11Buffer *d3d_SurfVertexes = NULL;
ID3D11Buffer *d3d_SurfIndexes = NULL;
#else // DX12
int d3d_SurfVertexes = 0;
int d3d_SurfIndexes = 0;
#endif // DX11_IMPL

static int d3d_SurfBasicShader;
static int d3d_SurfAlphaShader;
static int d3d_SurfLightmapShader;
static int d3d_SurfDynamicShader;
static int d3d_SurfDrawTurbShader;

#pragma pack(push, 1)
// compressing the vertex struct down to 32 bytes
typedef struct brushpolyvert_s {
	float xyz[3];
	float st[2];
	unsigned short lm[2];
	byte styles[4];
	unsigned int mapnum;
	float scroll;
} brushpolyvert_t;
#pragma pack(pop)

void R_InitSurfaces (void)
{
#if DX11_IMPL
	// compressing the vertex struct down to 32 bytes
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0),
		VDECL ("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 4, 0),
		VDECL ("LIGHTMAP", 0, DXGI_FORMAT_R16G16_UNORM, 4, 0),
		VDECL ("STYLES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 4, 0),
		VDECL ("MAPNUM", 0, DXGI_FORMAT_R16_UINT, 4, 0),
		VDECL ("SCROLL", 0, DXGI_FORMAT_R16_UNORM, 4, 0)
	};

	D3D11_BUFFER_DESC ibDesc = {
		MAX_SURF_INDEXES * sizeof (unsigned int),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_INDEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};

    RWCreateBuffer(&ibDesc, NULL, &d3d_SurfIndexes);

	d3d_SurfBasicShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfBasicVS", NULL, "SurfBasicPS", DEFINE_LAYOUT (layout));
	d3d_SurfAlphaShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfAlphaVS", NULL, "SurfAlphaPS", DEFINE_LAYOUT (layout));
	d3d_SurfLightmapShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfLightmapVS", NULL, "SurfLightmapPS", DEFINE_LAYOUT (layout));
	d3d_SurfDrawTurbShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfDrawTurbVS", NULL, "SurfDrawTurbPS", DEFINE_LAYOUT (layout));
	d3d_SurfDynamicShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfDynamicVS", "SurfDynamicGS", "GenericDynamicPS", DEFINE_LAYOUT (layout));
#else // DX12
    d3d_SurfIndexes = DX12_CreateIndexBuffer(MAX_SURF_INDEXES, NULL, sizeof(unsigned int));

    State surfState;
    surfState.inputLayout = INPUT_LAYOUT_SURFACES;
    surfState.VS = SHADER_MODEL_SURFACE_BASIC_VS;
    surfState.GS = SHADER_UNDEFINED;
    surfState.PS = SHADER_MODEL_SURFACE_BASIC_PS;
    surfState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    surfState.BS = BSAlphaPreMult;
    surfState.DS = DSNoDepth;
    surfState.RS = RSNoCull;
    d3d_SurfBasicShader = DX12_CreateRenderState(&surfState);

    /*
    surfState.VS = SHADER_MODEL_SURFACE_ALPHA_VS;
    surfState.PS = SHADER_MODEL_SURFACE_ALPHA_PS;
    d3d_SurfAlphaShader = DX12_CreateRenderState(&surfState);

    surfState.VS = SHADER_MODEL_SURFACE_LIGHTMAP_VS;
    surfState.PS = SHADER_MODEL_SURFACE_LIGHTMAP_PS;
    d3d_SurfLightmapShader = DX12_CreateRenderState(&surfState);

    surfState.VS = SHADER_MODEL_SURFACE_DRAWTURB_VS;
    surfState.PS = SHADER_MODEL_SURFACE_DRAWTURB_PS;
    d3d_SurfDrawTurbShader = DX12_CreateRenderState(&surfState);

    surfState.VS = SHADER_MODEL_SURFACE_DYNAMIC_VS;
    surfState.GS = SHADER_MODEL_SURFACE_DYNAMIC_GS;
    surfState.PS = SHADER_MODEL_SURFACE_DYNAMIC_PS;
    d3d_SurfDynamicShader = DX12_CreateRenderState(&surfState);
    */
#endif // DX11_IMPL
}


void R_ShutdownSurfaces (void)
{
#if DX11_IMPL
	SAFE_RELEASE (d3d_SurfVertexes);
	SAFE_RELEASE (d3d_SurfIndexes);
#endif // DX11_IMPL
}


/*
=============================================================

BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation (mtexinfo_t *tex, int frame)
{
	if (tex->next)
	{
		int c = frame % tex->numframes;

		while (c)
		{
			tex = tex->next;
			c--;
		}
	}

	return tex->image;
}


/*
================
DrawTextureChains
================
*/
#if !DX11_IMPL
static unsigned int surf_index_buffer[MAX_SURF_INDEXES];
#endif // DX12

static unsigned int *r_SurfIndexes = NULL;
static int r_NumSurfIndexes = 0;
static int r_FirstSurfIndex = 0;
static int r_NumSurfVertexes = 0;


void R_AddSurfaceToBatch (const msurface_t *surf)
{
	if (r_FirstSurfIndex + r_NumSurfIndexes + surf->numindexes >= MAX_SURF_INDEXES)
	{
		R_EndSurfaceBatch ();
		r_FirstSurfIndex = 0; // buffer wrapped
	}

	if (!r_SurfIndexes)
	{
#if DX11_IMPL
		// first index is only reset to 0 if the buffer must wrap so this is valid to do
		D3D11_MAP mode = (r_FirstSurfIndex > 0) ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
		D3D11_MAPPED_SUBRESOURCE msr;

		//if (FAILED (d3d_Context->lpVtbl->Map (d3d_Context, (ID3D11Resource *) d3d_SurfIndexes, 0, mode, 0, &msr)))
        if (FAILED(RWGetDeviceContext()->lpVtbl->Map(RWGetDeviceContext(), (ID3D11Resource*)d3d_SurfIndexes, 0, mode, 0, &msr)))
			return;
		else r_SurfIndexes = (unsigned int *) msr.pData + r_FirstSurfIndex;
#else // DX12
        r_SurfIndexes = (unsigned int*)surf_index_buffer + r_FirstSurfIndex;
#endif // DX11_IMPL
	}

	// these should always be valid coming in here....
	if (r_SurfIndexes && surf->numedges && surf->numindexes)
	{
		// write out indexes
		int v;
		unsigned int *ndx = &r_SurfIndexes[r_NumSurfIndexes];

		// write in the indexes
		for (v = 2; v < surf->numedges; v++, ndx += 3)
		{
			ndx[0] = surf->firstvertex;
			ndx[1] = surf->firstvertex + (v - 1);
			ndx[2] = surf->firstvertex + v;
		}

		// accumulate indexes count
		r_NumSurfIndexes += surf->numindexes;
	}
}


void R_EndSurfaceBatch (void)
{
#if DX11_IMPL
	if (r_SurfIndexes)
	{
        RWGetDeviceContext()->lpVtbl->Unmap(RWGetDeviceContext(), (ID3D11Resource*)d3d_SurfIndexes, 0);
		r_SurfIndexes = NULL;
	}

	if (r_NumSurfIndexes)
	{
		//d3d_Context->lpVtbl->DrawIndexed (d3d_Context, r_NumSurfIndexes, r_FirstSurfIndex, 0);
        RWGetDeviceContext()->lpVtbl->DrawIndexed(RWGetDeviceContext(), r_NumSurfIndexes, r_FirstSurfIndex, 0);

		r_FirstSurfIndex += r_NumSurfIndexes;
		r_NumSurfIndexes = 0;
	}
#else // DX12
    if (r_SurfIndexes)
    {
        DX12_UpdateIndexBuffer(d3d_SurfIndexes, surf_index_buffer, MAX_SURF_INDEXES, sizeof(unsigned int));
        r_SurfIndexes = NULL;
    }

    if (r_NumSurfIndexes)
    {
        DX12_DrawIndexed(r_NumSurfIndexes, r_FirstSurfIndex, 0);

        r_FirstSurfIndex += r_NumSurfIndexes;
        r_NumSurfIndexes = 0;
    }
#endif // DX11_IMPL
}


image_t *R_SelectSurfaceTexture (mtexinfo_t *ti, int frame)
{
	if (r_testnotexture->value)
		return r_notexture;
	else if (r_lightmap->value)
	{
		if (ti->flags & SURF_WARP)
			return r_whitetexture;
		else return r_greytexture;
	}
	else
	{
		if (ti->flags & SURF_WARP)
			return ti->image;
		else return R_TextureAnimation (ti, frame);
	}
}


void R_SelectSurfaceShader (const mtexinfo_t *ti, qboolean alpha)
{
#if FEATURE_LIGHT
	if (ti->flags & SURF_WARP)
        SLBindShaderBundle(d3d_SurfDrawTurbShader);
	else if (alpha)
        SLBindShaderBundle(d3d_SurfAlphaShader);
	else
	{
		if (!r_worldmodel->lightdata || r_fullbright->value)
            SLBindShaderBundle(d3d_SurfBasicShader);
		else SLBindShaderBundle(d3d_SurfLightmapShader);
	}
#else
#if DX11_IMPL
    SLBindShaderBundle(d3d_SurfBasicShader);
#else // DX12
    DX12_SetRenderState(d3d_SurfBasicShader);
#endif // DX11_IMPL
#endif // #if FEATURE_LIGHT
}


void R_SetupSurfaceState (QMATRIX *localmatrix, float alphaval, int flags)
{
	R_PrepareEntityForRendering (localmatrix, NULL, alphaval, flags);
#if DX11_IMPL
	SMBindVertexBuffer (4, d3d_SurfVertexes, sizeof (brushpolyvert_t), 0);
	SMBindIndexBuffer (d3d_SurfIndexes, DXGI_FORMAT_R32_UINT);
#else // DX12
    DX12_BindVertexBuffer(4, d3d_SurfVertexes);
    DX12_BindIndexBuffer(d3d_SurfIndexes);
#endif // DX11_IMPL
}


void R_DrawTextureChains (entity_t *e, model_t *mod, QMATRIX *localmatrix, float alphaval)
{
    int	i;
	msurface_t	*surf;
    
	// and now set it up
	R_SetupSurfaceState (localmatrix, alphaval, e->flags);

	for (i = 0; i < mod->numtexinfo; i++)
	{
		mtexinfo_t *ti = &mod->texinfo[i];

		// no surfaces
		if ((surf = ti->image->texturechain) == NULL) continue;

		// select the correct shader
		R_SelectSurfaceShader (ti, e->flags & RF_TRANSLUCENT);
		// select the correct texture
#if DX11_IMPL
		R_BindTexture (R_SelectSurfaceTexture (ti, e->currframe)->SRV);
#else // DX12
        R_UpdateEntityShader(DX12_GetCurrentRenderStateId(), e->flags);
        R_BindTexture(R_SelectSurfaceTexture(ti, e->currframe)->textureId);
#endif // DX11_IMPL
        
		// and draw the texture chain
		for (; surf; surf = surf->texturechain)
		{
			R_AddSurfaceToBatch (surf);
			surf->dlightframe = r_dlightframecount;
		}

		// and done
		R_EndSurfaceBatch ();
		ti->image->texturechain = NULL;
	}
    
    if (r_sky_surfaces)
	{
		R_DrawSkyChain (r_sky_surfaces);
		r_sky_surfaces = NULL;
	}

	// no dlights in fullbright mode
	if (!r_worldmodel->lightdata || r_fullbright->value) return;

	// no dlights on translucent entities
	if (e->flags & RF_TRANSLUCENT) return;

	// add dynamic lighting to the entity
	if (mod == r_worldmodel)
		R_PushDlights (mod->nodes, e, mod, localmatrix, r_visframecount);
	else R_PushDlights (mod->nodes + mod->firstnode, e, mod, localmatrix, 0);
}


void R_DrawDlightChains (entity_t *e, model_t *mod, QMATRIX *localmatrix)
{
	int	i;
	msurface_t	*surf;

	for (i = 0; i < mod->numtexinfo; i++)
	{
		mtexinfo_t *ti = &mod->texinfo[i];

		// no surfaces
		if ((surf = ti->image->texturechain) == NULL) continue;

#if DX11_IMPL
        SLBindShaderBundle(d3d_SurfDynamicShader);

		// select the correct texture
		R_BindTexture (R_SelectSurfaceTexture (ti, e->currframe)->SRV);
#else // DX12
        DX12_GetRenderState(d3d_SurfDynamicShader);
        R_BindTexture(R_SelectSurfaceTexture(ti, e->currframe)->textureId);
#endif // DX11_IMPL

		for (; surf; surf = surf->texturechain)
			R_AddSurfaceToBatch (surf);

		R_EndSurfaceBatch ();
		ti->image->texturechain = NULL;
	}
}


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces (void)
{
	msurface_t	*s;
	QMATRIX	localMatrix;
	mtexinfo_t *lasttexinfo = NULL;

	if (!r_alpha_surfaces) return;

	// go back to the world matrix
	R_MatrixIdentity (&localMatrix);

	// and now set it up (translucency is written directly into the texture at Image_QuakePalFromPCXPal so that we don't change state if alpha changes
	// this doesn't work so well with r_lightmap...
	R_SetupSurfaceState (&localMatrix, 1.0f, RF_TRANSLUCENT);

	// we can't sort these by texture because they need to be drawn in back-to-front order, so we go through them in BSP order
	// (which is back to front) and snoop for texture changes manually
	for (s = r_alpha_surfaces; s; s = s->texturechain)
	{
		if (s->texinfo != lasttexinfo)
		{
			// end the previous batch
			R_EndSurfaceBatch ();

			// switch texture and shader
#if DX11_IMPL
			R_BindTexture (R_SelectSurfaceTexture (s->texinfo, 0)->SRV);
			R_SelectSurfaceShader (s->texinfo, true);
#else // DX12
            R_BindTexArray(R_SelectSurfaceTexture(s->texinfo, 0)->textureId);
            R_SelectSurfaceShader(s->texinfo, true);
            R_UpdateEntityShader(DX12_GetCurrentRenderStateId(), RF_TRANSLUCENT);
#endif // DX11_IMPL

			// cache back
			lasttexinfo = s->texinfo;
		}

		R_AddSurfaceToBatch (s);
	}

	R_EndSurfaceBatch ();
	r_alpha_surfaces = NULL;
}


void R_ChainSurface (msurface_t *surf)
{
	if (surf->texinfo->flags & SURF_SKY)
	{
		// add to the sky chain
		surf->texturechain = r_sky_surfaces;
		r_sky_surfaces = surf;
	}
	else if (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
	{
		// add to the translucent chain
		surf->texturechain = r_alpha_surfaces;
		r_alpha_surfaces = surf;
	}
	else
	{
		// normal texture chain
		surf->texturechain = surf->texinfo->image->texturechain;
		surf->texinfo->image->texturechain = surf;
	}
}


/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e, QMATRIX *localmatrix)
{
	int			i;
	int			numsurfaces = 0;
	float		mins[3], maxs[3];
	model_t		*mod = e->model;
	msurface_t	*psurf = &mod->surfaces[mod->firstmodelsurface];

	// R_CullForEntity is incorrect for rotated bmodels so revert to the original cull
	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		for (i = 0; i < 3; i++)
		{
			mins[i] = e->currorigin[i] - mod->radius;
			maxs[i] = e->currorigin[i] + mod->radius;
		}
	}
	else
	{
		Vector3Add (mins, e->currorigin, mod->mins);
		Vector3Add (maxs, e->currorigin, mod->maxs);
	}

	// and do the cull
	if (R_CullBox (mins, maxs)) return;

	R_VectorInverseTransform (localmatrix, modelorg, r_newrefdef.vieworg);

	// draw texture
	for (i = 0; i < mod->nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		float dot = Mod_PlaneDist (psurf->plane, modelorg);

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			R_ChainSurface (psurf);
			numsurfaces++;
		}
	}

	if (numsurfaces)
	{
		if (e->flags & RF_TRANSLUCENT)
			R_DrawTextureChains (e, mod, localmatrix, 0.25f);
		else R_DrawTextureChains (e, mod, localmatrix, 1.0f);
	}
}


void R_PrepareBrushModel (entity_t *e, QMATRIX *localmatrix)
{
	// get the transform in local space so that we can correctly handle dlights
	R_MatrixIdentity (localmatrix);
	R_MatrixTranslate (localmatrix, e->currorigin[0], e->currorigin[1], e->currorigin[2]);
	R_MatrixRotate (localmatrix, e->angles[0], e->angles[1], e->angles[2]);
}


/*
=============================================================

WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c, side, sidebit;
	msurface_t	*surf, **mark;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (clipflags)
	{
		for (c = 0; c < 4; c++)
		{
			// don't need to clip against it
			if (!(clipflags & (1 << c))) continue;

			// the frustum planes are *never* axial so there's no point in doing the "fast" pre-test
			side = BoxOnPlaneSide (node->mins, node->maxs, &frustum[c]);

			if (side == 1) clipflags &= ~(1 << c);	// node is entirely on screen on this side
			if (side == 2) return;	// node is entirely off screen
		}
	}

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		mleaf_t *pleaf = (mleaf_t *) node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (!(r_newrefdef.areabits[pleaf->area >> 3] & (1 << (pleaf->area & 7))))
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		return;
	}

	// node is just a decision point, so go down the apropriate sides
	// find which side of the node we are on
	if (Mod_PlaneDist (node->plane, modelorg) >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side], clipflags);

	// draw stuff
	for (c = node->numsurfaces, surf = node->surfaces; c; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;		// wrong side

		if (R_CullBoxClipflags (surf->mins, surf->maxs, clipflags))
			continue;

		R_ChainSurface (surf);
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side], clipflags);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t ent;
	QMATRIX	localMatrix;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	Vector3Copy (modelorg, r_newrefdef.vieworg);

	// auto cycle the world frame for texture animation
	memset (&ent, 0, sizeof (ent));
	ent.currframe = (int) (r_newrefdef.time * 2);
	ent.model = r_worldmodel;

	R_MatrixIdentity (&localMatrix);

	R_RecursiveWorldNode (r_worldmodel->nodes, 15);
	R_DrawTextureChains (&ent, r_worldmodel, &localMatrix, 1.0f);
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_RegeneratePVS (void)
{
	// this is sufficient to force the PVS to regenerate
	r_oldviewleaf = NULL;
}


void R_MarkLeaves (void)
{
	byte	*vis = NULL;

	// don't bother if there is no world
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) return;

	// these tests are only valid if both leafs are non-NULL
	if (r_oldviewleaf && r_viewleaf)
	{
		// if the leafs are the same the PVS doesn't need to be regenned
		if (r_oldviewleaf == r_viewleaf) return;

		// if the same cluster then the PVS doesn't need to be regenned
		if (r_oldviewleaf->cluster == r_viewleaf->cluster) return;
	}

	// development aid to let you run around and see exactly where the pvs ends
	if (gl_lockpvs->value) return;

	//ri.Con_Printf (PRINT_ALL, "Regenning PVS on frame %i\n", r_framecount);

	// go to a new visframe
	r_visframecount++;

	// r_viewleaf is allowed be NULL coming in here
	if (r_novis->value || !r_worldmodel->vis || !r_viewleaf)
	{
		// mark everything
		vis = Mod_ClusterPVS (-1, r_worldmodel);
		Mod_AddLeafsToPVS (r_worldmodel, vis);
	}
	else
	{
		vis = Mod_ClusterPVS (r_viewleaf->cluster, r_worldmodel);
		Mod_AddLeafsToPVS (r_worldmodel, vis);

		// may have to combine two clusters because of solid water boundaries
		if (r_oldviewleaf && r_viewleaf->contents != r_oldviewleaf->contents)
		{
			vis = Mod_ClusterPVS (r_oldviewleaf->cluster, r_worldmodel);
			Mod_AddLeafsToPVS (r_worldmodel, vis);
		}
	}

	// store back
	r_oldviewleaf = r_viewleaf;
}



/*
================
R_BuildPolygonFromSurface
================
*/
void R_BuildPolygonFromSurface (msurface_t *surf, model_t *mod, brushpolyvert_t *verts, dbsp_t *bsp)
{
	int	i, maps;
	byte styles[4] = {0, 0, 0, 0};

	// set up styles
	for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		styles[maps] = surf->styles[maps];

	// reconstruct the polygon
	for (i = 0; i < surf->numedges; i++, verts++)
	{
		int lindex = bsp->surfedges[surf->firstedge + i];

		if (lindex > 0)
			Vector3Copy (verts->xyz, bsp->vertexes[bsp->edges[lindex].v[0]].point);
		else Vector3Copy (verts->xyz, bsp->vertexes[bsp->edges[-lindex].v[1]].point);

		if (surf->texinfo->flags & SURF_WARP)
		{
			verts->st[0] = Vector3Dot (verts->xyz, surf->texinfo->vecs[0]) * 0.015625f;
			verts->st[1] = Vector3Dot (verts->xyz, surf->texinfo->vecs[1]) * 0.015625f;
		}
		else if (!(surf->texinfo->flags & SURF_SKY))
		{
			verts->st[0] = (Vector3Dot (verts->xyz, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]) / surf->texinfo->image->width;
			verts->st[1] = (Vector3Dot (verts->xyz, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]) / surf->texinfo->image->height;

			// lightmap texture coordinates
			R_SetupLightmapTexCoords (surf, verts->xyz, verts->lm);

			// copy over styles
			*((unsigned *) verts->styles) = *((unsigned *) styles);

			// lightmap texture num is texture array slice to use
			verts->mapnum = surf->lightmaptexturenum;
		}

		// copy over scroll factor
		// this is DXGI_FORMAT_R16_UNORM so 65535 is equivalent to a scroll factor of 1.0f
		if (surf->texinfo->flags & SURF_FLOWING)
			verts->scroll = 65535;
		else verts->scroll = 0;
	}
}


void R_BeginBuildingSurfaces (model_t *mod)
{
#if DX11_IMPL
	SAFE_RELEASE (d3d_SurfVertexes);
#else // DX12
    d3d_SurfVertexes = 0;
#endif // DX11_IMPL
	r_NumSurfVertexes = 0;
}


void R_RegisterSurface (msurface_t *surf)
{
	surf->firstvertex = r_NumSurfVertexes;
	surf->numindexes = (surf->numedges - 2) * 3;
	r_NumSurfVertexes += surf->numedges;
}


void R_EndBuildingSurfaces (model_t *mod, dbsp_t *bsp)
{
#if DX11_IMPL
	int i;

	// create the vertex buffer sized as appropriate for all surface vertexes that will be needed
	D3D11_BUFFER_DESC vbDesc = {
		sizeof (brushpolyvert_t) * r_NumSurfVertexes,
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	// alloc a buffer to write the verts to and create the VB from
	brushpolyvert_t *verts = (brushpolyvert_t *) ri.Load_AllocMemory (sizeof (brushpolyvert_t) * r_NumSurfVertexes);

	// fill in the verts
	for (i = 0; i < mod->numsurfaces; i++)
	{
		msurface_t *surf = &mod->surfaces[i];
		R_BuildPolygonFromSurface (surf, mod, &verts[surf->firstvertex], bsp);
	}

	// create the new vertex buffer
    RWCreateBuffer(&vbDesc, verts, &d3d_SurfVertexes);

	// for the next map
	ri.Load_FreeMemory ();

	r_NumSurfVertexes = 0;
	r_FirstSurfIndex = 0; // force a buffer discard on the first draw call to flush all indexes from the previous map
#else // DX12
    brushpolyvert_t* verts = (brushpolyvert_t*)malloc(sizeof(brushpolyvert_t) * r_NumSurfVertexes);

    // fill in the verts
    for (int i = 0; i < mod->numsurfaces; i++)
    {
        msurface_t* surf = &mod->surfaces[i];
        R_BuildPolygonFromSurface(surf, mod, &verts[surf->firstvertex], bsp);
    }

    d3d_SurfVertexes = DX12_CreateVertexBuffer(r_NumSurfVertexes, sizeof(brushpolyvert_t), verts);

    free(verts);

    r_NumSurfVertexes = 0;
    r_FirstSurfIndex = 0; // force a buffer discard on the first draw call to flush all indexes from the previous map
#endif // DX11_IMPL
}
#else
void R_RegeneratePVS(void) { r_oldviewleaf = NULL; }
void R_PrepareBrushModel(entity_t* e, QMATRIX* localmatrix) {}
void R_DrawBrushModel(entity_t* e, QMATRIX* localmatrix) {}
void R_DrawWorld(void) {}
void R_DrawAlphaSurfaces(void) {}
void R_MarkLeaves(void){}
void R_InitSurfaces(void) {}
void R_ShutdownSurfaces(void) {}
void R_DrawDlightChains(entity_t* e, model_t* mod, QMATRIX* localmatrix) {}
#endif // FEATURE_BRUSH_MODEL
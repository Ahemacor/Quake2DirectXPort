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

#if FEATURE_SKY_CUBEMAP
#if DX11_IMPL
#include "CppWrapper.h"
#else // DX12
#include "TestDirectX12.h"
#endif // DX11_IMPL

char	skyname[MAX_QPATH];
float	skyrotate;
vec3_t	skyaxis;

texture_t r_SkyCubemap;

static int d3d_SurfDrawSkyShader;
static int d3d_SkyNoSkyShader;


void R_InitSky (void)
{
#if DX11_IMPL
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		VDECL ("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0)
	};

	d3d_SurfDrawSkyShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfDrawSkyVS", NULL, "SurfDrawSkyPS", DEFINE_LAYOUT (layout));
	d3d_SkyNoSkyShader = SLCreateShaderBundle(IDR_SURFSHADER, "SurfDrawSkyVS", NULL, "SkyNoSkyPS", DEFINE_LAYOUT (layout));
#else // DX12
    State skyState;
    skyState.inputLayout = INPUT_LAYOUT_SKY;
    skyState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    skyState.BS = BSNone;
    skyState.DS = DSDepthNoWrite;
    skyState.RS = RSFullCull;
    skyState.VS = SHADER_SKY_VS;
    skyState.GS = SHADER_UNDEFINED;

    skyState.PS = SHADER_SKY_PS;
    d3d_SurfDrawSkyShader = DX12_CreateRenderState(&skyState);

    skyState.PS = SHADER_NO_SKY_PS;
    d3d_SkyNoSkyShader = DX12_CreateRenderState(&skyState);
#endif // DX11_IMPL
}


void R_ShutdownSky (void)
{
	R_ReleaseTexture (&r_SkyCubemap);
}


void R_SetupSky (QMATRIX *SkyMatrix)
{
#if DX11_IMPL
	if (r_SkyCubemap.SRV)
	{
		// this is z-going-up with rotate and flip to orient the sky correctly
		R_MatrixLoadf (SkyMatrix, 0, 0, 1, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

		// rotate and position the sky
		R_MatrixRotateAxis (SkyMatrix, r_newrefdef.time * skyrotate, skyaxis[0], skyaxis[2], skyaxis[1]);
		R_MatrixTranslate (SkyMatrix, -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

		// sky goes to slot 4
		//d3d_Context->lpVtbl->PSSetShaderResources (d3d_Context, 4, 1, &r_SkyCubemap.SRV);
        RWGetDeviceContext()->lpVtbl->PSSetShaderResources(RWGetDeviceContext(), 4, 1, &r_SkyCubemap.SRV);
	}
	else R_MatrixIdentity (SkyMatrix);
#else // DX12
    if (r_SkyCubemap.Id)
    {
        // this is z-going-up with rotate and flip to orient the sky correctly
        R_MatrixLoadf(SkyMatrix, 0, 0, 1, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

        // rotate and position the sky
        R_MatrixRotateAxis(SkyMatrix, r_newrefdef.time * skyrotate, skyaxis[0], skyaxis[2], skyaxis[1]);
        R_MatrixTranslate(SkyMatrix, -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

        // sky goes to slot 4
        DX12_BindTexture(4, r_SkyCubemap.Id);
    }
    else R_MatrixIdentity(SkyMatrix);
#endif // DX11_IMPL
}


void R_DrawSkyChain (msurface_t *surf)
{
#if DX11_IMPL
	if (r_lightmap->value)
        SLBindShaderBundle(d3d_SkyNoSkyShader);
	else SLBindShaderBundle(d3d_SurfDrawSkyShader);
#else // DX12
    if (r_lightmap->value)
        DX12_SetRenderState(d3d_SkyNoSkyShader);
    else DX12_SetRenderState(d3d_SurfDrawSkyShader);
#endif // DX11_IMPL

	for (; surf; surf = surf->texturechain)
		R_AddSurfaceToBatch (surf);

	R_EndSurfaceBatch ();
}


void R_SetSky (char *name, float rotate, vec3_t axis)
{
	int		i;

	byte	*sky_pic[6];
	int		sky_width[6];
	int		sky_height[6];

	// 3dstudio environment map names
	char	*suf[6] = {"ft", "bk", "up", "dn", "rt", "lf"};

	// maximum dimension of skyface images
	int		max_size = -1;

	// shutdown the old sky
	R_ShutdownSky ();

	// begin a new sky
	strncpy (skyname, name, sizeof (skyname) - 1);
	skyrotate = rotate;
	Vector3Copy (skyaxis, axis);

	for (i = 0; i < 6; i++)
	{
		// clear it down
		sky_pic[i] = NULL;
		sky_width[i] = sky_height[i] = -1;

		// attempt to load it
		if ((sky_pic[i] = Image_LoadTGA (va ("env/%s%s.tga", skyname, suf[i]), &sky_width[i], &sky_height[i])) == NULL) continue;

		// figure the max size to create the cubemap at
		if (sky_width[i] > max_size) max_size = sky_width[i];
		if (sky_height[i] > max_size) max_size = sky_height[i];
	}

	// only proceed if we got something
	if (max_size < 1) return;

	// now set up the skybox faces for the cubemap
#if DX11_IMPL
    D3D11_SUBRESOURCE_DATA srd[6];
	for (i = 0; i < 6; i++)
	{
		if (sky_width[i] != max_size || sky_height[i] != max_size)
		{
			// if the size is different (e.g. a smaller bottom face may be provided due to some misguided attempt to "save memory")
			// we need to resample it up to the correct size because of cubemap creation constraints
			if (sky_pic[i])
				sky_pic[i] = (byte *) Image_ResampleToSize ((unsigned *) sky_pic[i], sky_width[i], sky_height[i], max_size, max_size);
			else
			{
				// case where a sky face may be omitted due to some misguided attempt to "save memory"
				sky_pic[i] = (byte *) ri.Load_AllocMemory (max_size * max_size * 4);
				memset (sky_pic[i], 0, max_size * max_size * 4);
			}

			// and set the new data
			sky_width[i] = max_size;
			sky_height[i] = max_size;
		}

		// and copy them off to the SRD
		srd[i].pSysMem = sky_pic[i];
		srd[i].SysMemPitch = max_size << 2;
		srd[i].SysMemSlicePitch = 0;
	}
#else // DX12
    D3D12_SUBRESOURCE_DATA srd[6];
    for (i = 0; i < 6; i++)
    {
        if (sky_width[i] != max_size || sky_height[i] != max_size)
        {
            // if the size is different (e.g. a smaller bottom face may be provided due to some misguided attempt to "save memory")
            // we need to resample it up to the correct size because of cubemap creation constraints
            if (sky_pic[i])
                sky_pic[i] = (byte*)Image_ResampleToSize((unsigned*)sky_pic[i], sky_width[i], sky_height[i], max_size, max_size);
            else
            {
                // case where a sky face may be omitted due to some misguided attempt to "save memory"
                sky_pic[i] = (byte*)ri.Load_AllocMemory(max_size * max_size * 4);
                memset(sky_pic[i], 0, max_size * max_size * 4);
            }

            // and set the new data
            sky_width[i] = max_size;
            sky_height[i] = max_size;
        }

        // and copy them off to the SRD
        srd[i].pData = sky_pic[i];
        srd[i].RowPitch = max_size << 2;
        srd[i].SlicePitch = 0;
    }
#endif // DX11_IMPL

	// and create it
	R_CreateTexture (&r_SkyCubemap, srd, max_size, max_size, 1, TEX_RGBA8 | TEX_CUBEMAP);

	// throw away memory used for loading
	ri.Load_FreeMemory ();
}
#else
void R_SetSky(char* name, float rotate, vec3_t axis) {}
void R_DrawSkyChain(msurface_t* surf) {}
void R_SetupSky(QMATRIX* SkyMatrix) {}
void R_InitSky(void) {}
void R_ShutdownSky(void) {}
#endif // FEATURE_SKY_CUBEMAP
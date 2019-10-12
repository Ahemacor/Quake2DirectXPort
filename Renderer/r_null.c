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

#if DX11_IMPL
#include "CppWrapper.h"

int d3d_NullShader = 0;

void R_InitNull (void)
{
	d3d_NullShader = SLCreateShaderBundle(IDR_NULLSHADER, "NullVS", "NullGS", "NullPS", NULL, 0);
}

void R_DrawNullModel (entity_t *e, QMATRIX *localmatrix)
{
	float shadelight[3];

	float mins[3] = {-16, -16, -16};
	float maxs[3] = {16, 16, 16};

	if (R_CullForEntity (mins, maxs, localmatrix))
		return;

	if (e->flags & RF_FULLBRIGHT)
		Vector3Set (shadelight, 1, 1, 1);
	else
	{
		R_LightPoint (e->currorigin, shadelight);
		R_DynamicLightPoint (e->currorigin, shadelight);
	}

	R_PrepareEntityForRendering (localmatrix, shadelight, e->alpha, e->flags);
    SLBindShaderBundle(d3d_NullShader);

    RWGetDeviceContext()->lpVtbl->Draw(RWGetDeviceContext(), 24, 0);
}


void R_PrepareNullModel (entity_t *e, QMATRIX *localmatrix)
{
	R_MatrixIdentity (localmatrix);
	R_MatrixTranslate (localmatrix, e->currorigin[0], e->currorigin[1], e->currorigin[2]);
	R_MatrixRotate (localmatrix, -e->angles[0], e->angles[1], -e->angles[2]);
}
#else // DX12
#include "TestDirectX12.h"

int d3d_NullShader = 0;

void R_InitNull(void)
{
    State state = DX12_GetCurrentRenderState();
    state.VS = SHADER_NULL_VS;
    state.GS = SHADER_NULL_GS;
    state.PS = SHADER_NULL_PS;
    d3d_NullShader = DX12_CreateRenderState(&state);
}

void R_DrawNullModel(entity_t* e, QMATRIX* localmatrix)
{
    float shadelight[3];

    float mins[3] = { -16, -16, -16 };
    float maxs[3] = { 16, 16, 16 };

    if (R_CullForEntity(mins, maxs, localmatrix))
        return;

    if (e->flags & RF_FULLBRIGHT)
        Vector3Set(shadelight, 1, 1, 1);
    else
    {
        R_LightPoint(e->currorigin, shadelight);
        R_DynamicLightPoint(e->currorigin, shadelight);
    }

    R_PrepareEntityForRendering(localmatrix, shadelight, e->alpha, e->flags);
    DX12_SetRenderState(d3d_NullShader);

    DX12_Draw(24, 0);
}

void R_PrepareNullModel(entity_t* e, QMATRIX* localmatrix)
{
    R_MatrixIdentity(localmatrix);
    R_MatrixTranslate(localmatrix, e->currorigin[0], e->currorigin[1], e->currorigin[2]);
    R_MatrixRotate(localmatrix, -e->angles[0], e->angles[1], -e->angles[2]);
}
#endif // DX11_IMPL
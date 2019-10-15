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
#include "TestDirectX12.h"

static int d3d_ParticleCircleShader;
static int d3d_ParticleSquareShader;
static int d3d_ParticleShader = 0;

#define MAX_GPU_PARTICLES (MAX_PARTICLES * 10)
static int r_FirstParticle = 0;
particle_t particle_buffer[MAX_GPU_PARTICLES];
static int d3d_ParticleVertexes;

void R_InitParticles (void)
{
    d3d_ParticleVertexes = DX12_CreateVertexBuffer(MAX_GPU_PARTICLES, sizeof(particle_t), NULL);

    State particleState;
    particleState.inputLayout = INPUT_LAYOUT_PARTICLES;
    particleState.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;//D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    particleState.BS = BSAlphaBlend;
    particleState.DS = DSDepthNoWrite;
    particleState.RS = RSFullCull;

    particleState.VS = SHADER_PARTICLE_VS;
    particleState.GS = SHADER_PARTICLE_CIRCLE_GS; // SHADER_UNDEFINED;
    particleState.PS = SHADER_PARTICLE_CIRCLE_PS;
    d3d_ParticleCircleShader = DX12_CreateRenderState(&particleState);

    particleState.GS = SHADER_PARTICLE_SQUARE_GS;
    particleState.PS = SHADER_PARTICLE_SQUARE_PS;
    d3d_ParticleSquareShader = DX12_CreateRenderState(&particleState);

    d3d_ParticleShader = d3d_ParticleCircleShader;
}


void R_DrawParticles (void)
{
    if (!r_newrefdef.num_particles)
        return;

    DX12_UpdateVertexBuffer(d3d_ParticleVertexes, r_newrefdef.particles, r_newrefdef.num_particles, 0, sizeof(particle_t));
    DX12_SetRenderState(d3d_ParticleShader);
    DX12_BindVertexBuffer(6, d3d_ParticleVertexes, 0);
    DX12_Draw(r_newrefdef.num_particles, 0);
    //DX12_Execute();
}

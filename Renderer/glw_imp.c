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

/*
=========================================================================================================
GLW_IMP.C

This file contains ALL Win32 specific stuff having to do with the
OpenGL refresh.  When a port is being made the following functions
must be implemented by the port:

GLimp_EndFrame
GLimp_Init
GLimp_Shutdown
=========================================================================================================
*/

#include <assert.h>
#include <windows.h>
#include "r_local.h"
#include "CppWrapper.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")

// this just needs to be included anywhere....
#pragma comment (lib, "dxguid.lib")

HANDLE hRefHeap;

// version for the menus
vidmenu_t vid_modedata;


float M_VideoGetRefreshRate (DXGI_MODE_DESC *mode)
{
	return (float) mode->RefreshRate.Numerator / (float) mode->RefreshRate.Denominator;
}


char *M_VideoGetScanlineOrdering (DXGI_MODE_DESC *mode)
{
	switch (mode->ScanlineOrdering)
	{
	case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED: return "Unspecified";
	case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE: return "Progressive";
	case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST: return "UFF";
	case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST: return "LFF";
	}

	return "Unspecified";
}


char *M_VideoGetScaling (DXGI_MODE_DESC *mode)
{
	switch (mode->Scaling)
	{
	case DXGI_MODE_SCALING_UNSPECIFIED: return "Unspecified";
	case DXGI_MODE_SCALING_CENTERED: return "Centered";
	case DXGI_MODE_SCALING_STRETCHED: return "Stretched";
	}

	return "Unspecified";
}


void D_EnumerateVideoModes (void)
{
    if (GetModesNumber() == 0)
		ri.Sys_Error (ERR_FATAL, "Failed to enumerate any usable video modes!");

	// init the video mode data - add 1 to windowed modes for the current values of vid_width and vid_height if required (they may not be)
	vid_modedata.widths = (int *) HeapAlloc (hRefHeap, HEAP_ZERO_MEMORY, sizeof (int) * (GetModesNumber() + 1));
	vid_modedata.heights = (int *) HeapAlloc (hRefHeap, HEAP_ZERO_MEMORY, sizeof (int) * (GetModesNumber() + 1));

	// add 1 to fullscreen modes to NULL-terminate the list
	vid_modedata.fsmodes = (char **) HeapAlloc (hRefHeap, HEAP_ZERO_MEMORY, sizeof (char *) * (GetModesNumber() + 1));

	vid_modedata.numwidths = 0;
	vid_modedata.numheights = 0;
	vid_modedata.numfsmodes = 0;

	// set up widths
	int biggest = 0;

	for (int i = 0; i < GetModesNumber(); i++)
	{
		if (GetMode(i).Width > biggest)
		{
			vid_modedata.widths[vid_modedata.numwidths] = GetMode(i).Width;
			vid_modedata.numwidths++;
			biggest = GetMode(i).Width;
		}
	}

	// set up heights
	biggest = 0;

	for (int i = 0; i < GetModesNumber(); i++)
	{
		if (GetMode(i).Height > biggest)
		{
			vid_modedata.heights[vid_modedata.numheights] = GetMode(i).Height;
			vid_modedata.numheights++;
			biggest = GetMode(i).Height;
		}
	}

	// set up fullscreen modes
	for (int i = 0; i < GetModesNumber(); i++)
	{
        DXGI_MODE_DESC mode = GetMode(i);
		char *modedesc = va (
			"[%i %i] %0.1fHz\n%s/%s",
            mode.Width,
            mode.Height,
			M_VideoGetRefreshRate(&mode),
			M_VideoGetScanlineOrdering(&mode),
			M_VideoGetScaling(&mode)
		);

		vid_modedata.fsmodes[i] = (char *) HeapAlloc (hRefHeap, HEAP_ZERO_MEMORY, strlen (modedesc) + 1);
		strcpy(vid_modedata.fsmodes[i], modedesc);
		vid_modedata.fsmodes[i + 1] = NULL;
	}

	ri.Load_FreeMemory ();
}


/*
=========================================================================================================================================================================

OBJECT LIFETIME MANAGEMENT

=========================================================================================================================================================================
*/

// object cache lets us store out objects on creation so that indivdual routines don't need to destroy them
#define MAX_OBJECT_CACHE	1024

typedef struct d3dobject_s {
	ID3D11DeviceChild *Object;
	char *name;
} d3dobject_t;

d3dobject_t ObjectCache[MAX_OBJECT_CACHE];
int NumObjectCache = 0;

void D_CacheObject (ID3D11DeviceChild *Object, const char *name)
{
	if (!Object)
		return;
	else if (NumObjectCache < MAX_OBJECT_CACHE)
	{
		ObjectCache[NumObjectCache].Object = Object;
		ObjectCache[NumObjectCache].name = (char *) HeapAlloc (hRefHeap, HEAP_ZERO_MEMORY, strlen (name) + 1);
		strcpy(ObjectCache[NumObjectCache].name, name);

		NumObjectCache++;
	}
	else Sys_Error ("R_CacheObject : object cache overflow!");
}


void D_ReleaseObjectCache (void)
{
	int i;

	for (i = 0; i < MAX_OBJECT_CACHE; i++)
		SAFE_RELEASE (ObjectCache[i].Object);

	memset (ObjectCache, 0, sizeof (ObjectCache));
}


/*
=========================================================================================================================================================================

STOCK CODE CONTINUES

=========================================================================================================================================================================
*/

extern cvar_t *vid_fullscreen;

static qboolean VerifyDriver (void)
{
	return true;
}

/*
===============
GLimp_SetMode
===============
*/
rserr_t GLimp_SetMode (int *pwidth, int *pheight, int mode, qboolean fullscreen)
{
	//int width, height;
	const char *win_fs[] = {"W", "FS"};

	ri.Con_Printf (PRINT_ALL, "Initializing display\n");
	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode);

    DXGI_MODE_DESC modeDesc = GetMode(mode);
	ri.Con_Printf (PRINT_ALL, " %d %d %s\n", modeDesc.Width, modeDesc.Height, win_fs[fullscreen]);

	// destroy the existing window
	if (GetWindowHandle() != NULL)
	{
        CloseRenderWindow();
	}

	// do a CDS if needed
	if (fullscreen)
	{
		ri.Con_Printf (PRINT_ALL, "...setting fullscreen mode\n");

		// if we fail to create a fullscreen mode call recursively to create a windowed mode
		//if (!VID_CreateWindow (modeDesc.Width, modeDesc.Height, true))
        if (!RWInitWindow(modeDesc.Width, modeDesc.Height, mode, true))
			return GLimp_SetMode (pwidth, pheight, mode, false);
	}
	else
	{
		ri.Con_Printf (PRINT_ALL, "...setting windowed mode\n");

        cvar_t* vid_width = NULL;
        cvar_t* vid_height = NULL;
        vid_width = ri.Cvar_Get("vid_width", "640", 0, NULL);
        vid_height = ri.Cvar_Get("vid_height", "480", 0, NULL);
        modeDesc.Width = vid_width->value;
        modeDesc.Height = vid_height->value;

		// if we fail to create a windowed mode it's an error
		//if (!VID_CreateWindow (modeDesc.Width, modeDesc.Height, false))
        if (!RWInitWindow(modeDesc.Width, modeDesc.Height, mode, false))
			return rserr_invalid_mode;
	}

    ri.Vid_NewWindow();
	return rserr_ok;
}

static void GLimp_GetGUIScale (void)
{
	// viewsize is a percentage scaleup from 640x480 (which will be aspect-adjusted) to the full current resolution
	// and which is applied to the ortho matrix for 2D GUI views
	int scale = (int) scr_viewsize->value;

	if (scale < 0) scale = 0;
	if (scale > 100) scale = 100;

	if (vid.width > 640 && vid.height > 480)
	{
		if (vid.width > vid.height)
		{
			int virtual_height = (((vid.height - 480) * scale) / 100) + 480;

			if (vid.height > virtual_height)
			{
				vid.conwidth = (virtual_height * vid.width) / vid.height;
				vid.conheight = virtual_height;
				return;
			}
		}
		else
		{
			int virtual_width = (((vid.width - 640) * scale) / 100) + 640;

			if (vid.width > virtual_width)
			{
				vid.conwidth = virtual_width;
				vid.conheight = (virtual_width * vid.height) / vid.width;
				return;
			}
		}
	}

	// default scale
	vid.conwidth = vid.width;
	vid.conheight = vid.height;
}


/*
===============
GLimp_BeginFrame
===============
*/
void GLimp_BeginFrame (viddef_t *vd, int scrflags)
{
	// get client dimensions
	RECT cr;
	GetClientRect (GetWindowHandle(), &cr);

	// setup dimensions for the refresh
	vid.width = cr.right - cr.left;
	vid.height = cr.bottom - cr.top;

	// apply scaling
	GLimp_GetGUIScale ();

	// copy them over to the main engine
	vd->width = vid.width;
	vd->height = vid.height;
	vd->conwidth = vid.conwidth;
	vd->conheight = vid.conheight;

	// set up the 2D ortho view, brightness and contrast
	Draw_UpdateConstants (scrflags);

	// everything in all draws is drawn as an indexed triangle list, even if it's ultimately a strip or a single tri, so this can be set-and-forget once per frame
	//d3d_Context->lpVtbl->IASetPrimitiveTopology (d3d_Context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    RWGetDeviceContext()->lpVtbl->IASetPrimitiveTopology(RWGetDeviceContext(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// bind buffers and samplers that will remain bound for the duration of the frame
	D_BindSamplers ();
	D_BindConstantBuffers ();
}


/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers and possibly for other stuff
as yet to be determined.  Probably better not to make this a GLimp
function and instead do a call to GLimp_SwapBuffers.
===============
*/
void GLimp_EndFrame (int scrflags)
{
	// free any loading memory that may have been used during the frame
	ri.Load_FreeMemory ();

	if (scrflags & SCR_SYNC_PIPELINE)
		R_SyncPipeline ();

	// perform the buffer swap with or without vsync as appropriate
	if (scrflags & SCR_NO_PRESENT)
		return;
	else if (scrflags & SCR_NO_VSYNC)
		//d3d_SwapChain->lpVtbl->Present (d3d_SwapChain, 0, 0);
        RWGetSwapchain()->lpVtbl->Present(RWGetSwapchain(), 0, 0);
	else if (vid_vsync->value)
		//d3d_SwapChain->lpVtbl->Present (d3d_SwapChain, 1, 0);
        RWGetSwapchain()->lpVtbl->Present(RWGetSwapchain(), 1, 0);
	else //d3d_SwapChain->lpVtbl->Present (d3d_SwapChain, 0, 0);
        RWGetSwapchain()->lpVtbl->Present(RWGetSwapchain(), 0, 0);
}


/*
===============
GLimp_AppActivate
===============
*/
void GLimp_AppActivate (qboolean active)
{
	// does DXGI not do this for us????
	if (active)
	{
		SetForegroundWindow (GetWindowHandle());
		ShowWindow (GetWindowHandle(), SW_RESTORE);
	}
	else
	{
		if (vid_fullscreen->value)
			ShowWindow (GetWindowHandle(), SW_MINIMIZE);
	}
}



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
#include "CppWrapper.h"

extern vidmenu_t vid_modedata;

void R_Register (void)
{
	scr_viewsize = ri.Cvar_Get ("viewsize", "100", CVAR_ARCHIVE, NULL);
	r_testnullmodels = ri.Cvar_Get ("r_testnullmodels", "0", CVAR_CHEAT, NULL);
	r_lightmap = ri.Cvar_Get ("r_lightmap", "0", CVAR_CHEAT, NULL);
	r_testnotexture = ri.Cvar_Get ("r_testnotexture", "0", CVAR_CHEAT, NULL);
	r_lightmodel = ri.Cvar_Get ("r_lightmodel", "1", 0, NULL);
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", CVAR_CHEAT, NULL);
	r_beamdetail = ri.Cvar_Get ("r_beamdetail", "24", CVAR_ARCHIVE, NULL);
	r_lefthand = ri.Cvar_Get ("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE, NULL);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", 0, NULL);
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", CVAR_CHEAT, NULL);
	r_novis = ri.Cvar_Get ("r_novis", "0", 0, R_RegeneratePVS);

	r_lightlevel = ri.Cvar_Get ("r_lightlevel", "0", 0, NULL);
	r_desaturatelighting = ri.Cvar_Get ("r_desaturatelighting", "1", CVAR_ARCHIVE, NULL);

	vid_mode = ri.Cvar_Get ("vid_mode", "-1", CVAR_ARCHIVE | CVAR_VIDEO, NULL);
	gl_finish = ri.Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE, NULL);
	gl_clear = ri.Cvar_Get ("gl_clear", "0", 0, NULL);
	gl_polyblend = ri.Cvar_Get ("gl_polyblend", "1", 0, NULL);
	gl_lockpvs = ri.Cvar_Get ("gl_lockpvs", "0", 0, R_RegeneratePVS);

	vid_fullscreen = ri.Cvar_Get ("vid_fullscreen", "0", CVAR_ARCHIVE | CVAR_VIDEO, NULL);
	vid_gamma = ri.Cvar_Get ("vid_gamma", "1.0", CVAR_ARCHIVE, NULL);
	vid_brightness = ri.Cvar_Get ("vid_brightness", "1.0", CVAR_ARCHIVE, NULL);

	vid_width = ri.Cvar_Get ("vid_width", "640", CVAR_ARCHIVE | CVAR_VIDEO, NULL);
	vid_height = ri.Cvar_Get ("vid_height", "480", CVAR_ARCHIVE | CVAR_VIDEO, NULL);
	vid_vsync = ri.Cvar_Get ("vid_vsync", "0", CVAR_ARCHIVE, NULL);
}


/*
==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	if (vid_fullscreen->modified && !gl_config.allow_cds)
	{
		ri.Con_Printf (PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n");
		ri.Cvar_SetValue ("vid_fullscreen", !vid_fullscreen->value);
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	vid_mode->modified = false;

	if ((err = GLimp_SetMode (&vid.width, &vid.height, vid_mode->value, fullscreen)) == rserr_ok)
	{
		gl_state.prev_mode = vid_mode->value;
	}
	else
	{
		if (err == rserr_invalid_fullscreen)
		{
			ri.Cvar_SetValue ("vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf (PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n");
			if ((err = GLimp_SetMode (&vid.width, &vid.height, vid_mode->value, false)) == rserr_ok)
				goto done;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue ("vid_mode", gl_state.prev_mode);
			vid_mode->modified = false;
			ri.Con_Printf (PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode (&vid.width, &vid.height, gl_state.prev_mode, false)) != rserr_ok)
		{
			ri.Con_Printf (PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}

done:;
	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow ();

	return true;
}

/*
===============
R_Init
===============
*/
int R_Init (void *hinstance, void *wndproc)
{
    RWSetAppProps(hinstance, wndproc);

	ri.Con_Printf (PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	Draw_GetPalette ();

	R_Register ();

	gl_config.allow_cds = true;

	// set our "safe" modes
	gl_state.prev_mode = -1;

	// create the window and set up the context
	if (!R_SetMode ())
	{
		ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n");
		return -1;
	}

	ri.Vid_PrepVideoMenu (&vid_modedata);
	ri.Vid_MenuInit ();

	// this sets up state objects and NULLs-out cached state
    SMInitDefaultStates();

	// initialize all objects, textures, shaders, etc
	R_InitImages ();
	Mod_Init ();
	R_CreateSpecialTextures ();
	Draw_InitLocal ();

	// success
	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	Mod_FreeAll ();

	R_ShutdownImages ();

	// shut down OS specific OpenGL stuff like contexts, etc.
    RWClose();
}


//===================================================================


void R_BeginRegistration (char *map);
struct model_s	*R_RegisterModel (char *name);
struct image_s	*R_RegisterSkin (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void R_EndRegistration (void);

void R_RenderFrame (refdef_t *fd);

struct image_s	*Draw_FindPic (char *name);

void Draw_StretchPic (int x, int y, int w, int h, char *pic);
void Draw_Pic (int x, int y, char *name);
void Draw_Char (int x, int y, int c);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void D_EnumerateVideoModes (void);
void D_CaptureScreenshot (char *checkname);
void R_ClearToBlack (void);

/*
===============
GetRefAPI

===============
*/
refexport_t GetRefAPI (refimport_t rimp)
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration; // init model_t *r_worldmodel
	re.RegisterModel = R_RegisterModel; // Load Mod_ForName and load info by type mod_sprite mod_alias mod_brush
	re.RegisterSkin = R_RegisterSkin; // Finds or loads the given image
	re.RegisterPic = Draw_FindPic; // Finds or loads the given image
	re.SetSky = R_SetSky; // Create sky cubemap texture
	re.EndRegistration = R_EndRegistration; // Free unused models

	re.RenderFrame = R_RenderFrame; // Update resources and R_RenderScene

	re.DrawConsoleBackground = Draw_ConsoleBackground; // Draw console background
	re.DrawGetPicSize = Draw_GetPicSize; // Find picture and return it's H/W size
	re.DrawStretchPic = Draw_StretchPic; // draw it at the correct scaled size for the HUD
	re.DrawPic = Draw_Pic; // Draw_TexturedQuad
	re.DrawFill = Draw_Fill; // Some drawing for menu
	re.DrawFadeScreen = Draw_FadeScreen; // Draw darkened menu background
	re.Clear = R_ClearToBlack; // Seems not used

	re.DrawChar = Draw_Char; // Draw_CharacterQuad
	re.DrawString = Draw_Flush; // (sprite staff) D_BindVertexBuffer and draw or draw indexed
	re.DrawField = Draw_Field; // Draw HUD strings

	re.DrawStretchRaw = Draw_StretchRaw; // For SCR_DrawCinematic

	re.Init = R_Init; // Init window and graphics
	re.Shutdown = R_Shutdown; // free models, images, and graphics

	re.BeginFrame = GLimp_BeginFrame;
	re.Set2D = R_Set2D; // HZ switch to our 2d viewport
	re.EndFrame = GLimp_EndFrame;

	re.AppActivate = GLimp_AppActivate; // minimize/restore
	re.EnumerateVideoModes = D_EnumerateVideoModes;
	re.CaptureScreenshot = D_CaptureScreenshot;

	Swap_Init ();

	// set up the refresh heap for allocations that live as long as the refresh;
	// first of all, if one existed from a previous refresh, destroy it
	if (hRefHeap)
	{
		HeapDestroy (hRefHeap);
		hRefHeap = NULL;
	}

	// now create the new one we're going to use
	hRefHeap = HeapCreate (0, 0, 0);

    CPPRease();
    CPPInit();

	// and done
	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	ri.Sys_Error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	ri.Con_Printf (PRINT_ALL, "%s", text);
}

#endif

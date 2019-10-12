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

image_t		gltextures[MAX_GLTEXTURES];

unsigned	d_8to24table_solid[256];
unsigned	d_8to24table_alpha[256];
unsigned	d_8to24table_trans33[256];
unsigned	d_8to24table_trans66[256];

void R_DescribeTexture(D3D12_RESOURCE_DESC* Desc, int width, int height, int arraysize, int flags)
{
    // basic info
    Desc->Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    Desc->Alignment = 0;

    Desc->Width = width;
    Desc->Height = height;

    Desc->MipLevels = 1;
    Desc->Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    // no multisampling
    Desc->SampleDesc.Count = 1;
    Desc->SampleDesc.Quality = 0;

    Desc->Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    Desc->Flags = D3D12_RESOURCE_FLAG_NONE;

    // select if creating a cubemap (allow creation of cubemap arrays)
    if (flags & TEX_CUBEMAP)
    {
        Desc->DepthOrArraySize = 6 * arraysize;
    }
    else
    {
        Desc->DepthOrArraySize = arraysize;
    }
}

void R_CreateTexture32 (image_t *image, unsigned *data)
{
    D3D12_RESOURCE_DESC  Desc;
    if (image->flags & TEX_CHARSET)
    {
        D3D12_SUBRESOURCE_DATA srd[256];
        for (int i = 0; i < 256; i++)
        {
            int row = (i >> 4);
            int col = (i & 15);

            srd[i].pData = &data[((row * (image->width >> 4))* image->width) + col * (image->width >> 4)];
            srd[i].RowPitch = image->width << 2;
            srd[i].SlicePitch = 0;
        }

        R_DescribeTexture(&Desc, image->width >> 4, image->height >> 4, 256, image->flags);
        image->textureId = DX12_CreateTexture(&Desc, &srd);
    }
    else
    {
        // this is good for a 4-billion X 4-billion texture; we assume it will never be needed that large
        D3D12_SUBRESOURCE_DATA srd[32];

        // copy these off so that they can be changed during miplevel reduction
        int width = image->width;
        int height = image->height;

        // the first one just has the data
        srd[0].pData = data;
        srd[0].RowPitch = width << 2;
        srd[0].SlicePitch = 0;

        // create further miplevels for the texture type
        if (image->flags & TEX_MIPMAP)
        {
            int mipnum;

            for (mipnum = 1; width > 1 || height > 1; mipnum++)
            {
                // choose the appropriate filter
                if ((width & 1) || (height & 1))
                    data = Image_MipReduceLinearFilter(data, width, height);
                else data = Image_MipReduceBoxFilter(data, width, height);

                if ((width = width >> 1) < 1) width = 1;
                if ((height = height >> 1) < 1) height = 1;

                srd[mipnum].pData = data;
                srd[mipnum].RowPitch = width << 2;
                srd[mipnum].SlicePitch = 0;
            }
        }

        R_DescribeTexture(&Desc, image->width, image->height, 1, image->flags);
        image->textureId = DX12_CreateTexture(&Desc, &srd);
    }
}


void R_CreateTexture8 (image_t *image, byte *data, unsigned *palette)
{
	unsigned *trans = GL_Image8To32 (data, image->width, image->height, palette);
	R_CreateTexture32 (image, trans);
}

void R_BindTexArray(int resId)
{
    static int* OldResId;
    if (OldResId != resId)
    {
        DX12_BindTexture(6, resId);
        OldResId = resId;
    }
}

void R_BindTexture(int resId)
{
    static int OldRes;
    if (OldRes != resId)
    {
        DX12_BindTexture(0, resId);
        OldRes = resId;
    }
}

image_t *GL_FindFreeImage (char *name, int width, int height, imagetype_t type)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i = 0, image = gltextures; i < MAX_GLTEXTURES; i++, image++)
	{
        if (image->textureId) continue;

		break;
	}

	if (i == MAX_GLTEXTURES)
		ri.Sys_Error (ERR_DROP, "MAX_GLTEXTURES");

	image = &gltextures[i];

	if (strlen (name) >= sizeof (image->name))
		ri.Sys_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);

	strcpy (image->name, name);
	image->registration_sequence = r_registration_sequence;

	image->width = width;
	image->height = height;

	// basic flags
	image->flags = TEX_RGBA8;

	// additional flags - these image types are mipmapped
	if (type != it_pic && type != it_charset) image->flags |= TEX_MIPMAP;

	// these image types may be thrown away
	if (type != it_pic && type != it_charset) image->flags |= TEX_DISPOSABLE;

	// drawn as a 256-slice texture array
	if (type == it_charset) image->flags |= TEX_CHARSET;

	return image;
}


/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits, unsigned *palette)
{
    image_t* image = GL_FindFreeImage(name, width, height, type);

    // floodfill 8-bit alias skins (32-bit are assumed to be already filled)
    if (type == it_skin && bits == 8)
        R_FloodFillSkin(pic, width, height);

    // problem - if we use linear filtering, we lose all of the fine pixel art detail in the original 8-bit textures.
    // if we use nearest filtering we can't do anisotropic and we get noise at minification levels.
    // so what we do is upscale the texture by a simple 2x nearest-neighbour upscale, which gives us magnification-nearest
    // quality but not with the same degree of discontinuous noise, but let's us minify and anisotropically filter them properly.
    if ((type == it_wall || type == it_skin) && bits == 8)
    {
        pic = Image_Upscale8(pic, image->width, image->height);
        image->width <<= 1;
        image->height <<= 1;
        image->flags |= TEX_UPSCALE;
    }

    // it's 2018 and we have non-power-of-two textures nowadays so don't bother with scraps
    if (bits == 8)
        R_CreateTexture8(image, pic, palette);
    else
        R_CreateTexture32(image, (unsigned*)pic);

    // if the image was upscaled, bring it back down again so that texcoord calculation will work as expected
    if (image->flags & TEX_UPSCALE)
    {
        image->width >>= 1;
        image->height >>= 1;
    }

    // free memory used for loading the image
    ri.Load_FreeMemory();

    return image;
}


image_t *GL_HaveImage (char *name, int flags)
{
	int		i;
	image_t	*image;

	// look for it
	for (i = 0, image = gltextures; i < MAX_GLTEXTURES; i++, image++)
	{
        if (!image->textureId) continue;

		// only brush models send texinfo flags and they must match because we're encoding alpha into the textures
		if (image->texinfoflags != flags) continue;

		if (!strcmp (name, image->name))
		{
			image->registration_sequence = r_registration_sequence;
			return image;
		}
	}

	// don't have it
	return NULL;
}


float ColorNormalize (vec3_t out, vec3_t in)
{
	float max = in[0];

	if (in[1] > max) max = in[1];
	if (in[2] > max) max = in[2];

	if (max == 0)
		return 0;

	Vector3Scalef (out, in, 1.0f / max);

	return max;
}


/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal (char *name, int flags)
{
	miptex_t	*mt;
	int			i, width, height;
	image_t		*image;
	byte		*texels;
	float		scale;

	// look for it
	if ((image = GL_HaveImage (name, flags)) != NULL)
		return image;

	// load the pic from disk
	ri.FS_LoadFile (name, (void **) &mt);

	if (!mt)
	{
		ri.Con_Printf (PRINT_ALL, "GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	texels = (byte *) mt + LittleLong (mt->offsets[0]);

	// choose the correct palette to use (note: using texinfo flags here)
	if (flags & SURF_TRANS33)
		image = GL_LoadPic (name, texels, width, height, it_wall, 8, d_8to24table_trans33);
	else if (flags & SURF_TRANS66)
		image = GL_LoadPic (name, texels, width, height, it_wall, 8, d_8to24table_trans66);
	else image = GL_LoadPic (name, texels, width, height, it_wall, 8, d_8to24table_solid);

	// calculate the colour that was used to generate radiosity for this texture
	// https://github.com/id-Software/Quake-2-Tools/blob/master/bsp/qrad3/patches.c#L88
	// this is used for R_LightPoint tracing that hits sky and may also be used for contents colours in the future
	Vector3Set (image->color, 0, 0, 0);

	// accumulate the colours
	for (i = 0; i < width * height; i++)
	{
		image->color[0] += ((byte *) &d_8to24table_solid[texels[i]])[0];
		image->color[1] += ((byte *) &d_8to24table_solid[texels[i]])[1];
		image->color[2] += ((byte *) &d_8to24table_solid[texels[i]])[2];
	}

	// average them out and bring to 0..1 scale
	image->color[0] = image->color[0] / (width * height) / 255.0f;
	image->color[1] = image->color[1] / (width * height) / 255.0f;
	image->color[2] = image->color[2] / (width * height) / 255.0f;

	// scale the reflectivity up, because the textures are so dim
	scale = ColorNormalize (image->color, image->color);

	// ??? can this even happen ???
	if (scale < 0.5)
		Vector3Scalef (image->color, image->color, scale * 2);

	// free any memory used for loading
	ri.FS_FreeFile ((void *) mt);
	ri.Load_FreeMemory ();

	// store out the flags used for matching
	image->texinfoflags = flags;

	return image;
}


/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t *GL_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int		len;
	byte	*pic, *palette;
	int		width, height;

	// validate the name
	if (!name) return NULL;
	if ((len = strlen (name)) < 5) return NULL;

	// look for it
	if ((image = GL_HaveImage (name, 0)) != NULL)
		return image;

	// load the pic from disk
	pic = NULL;
	palette = NULL;

	// PCX/TGA types only; WAL is sent directly through GL_LoadWal
	if (!strcmp (name + len - 4, ".pcx"))
	{
		unsigned table[256];

		LoadPCX (name, &pic, &palette, &width, &height);

		if (!pic)
			return NULL;

		// skins use the solid palette; everything else has alpha
		if (type == it_skin)
			Image_QuakePalFromPCXPal (table, palette, TEX_RGBA8);
		else Image_QuakePalFromPCXPal (table, palette, TEX_ALPHA);

		image = GL_LoadPic (name, pic, width, height, type, 8, table);
	}
	else if (!strcmp (name + len - 4, ".tga"))
	{
		if ((pic = Image_LoadTGA (name, &width, &height)) == NULL)
			return NULL;
		else image = GL_LoadPic (name, pic, width, height, type, 32, NULL);
	}
	else
	{
		ri.Sys_Error (ERR_DROP, "GL_FindImage : %s is unsupported file type\n", name);
		return NULL;
	}

	// free any memory used for loading
	ri.Load_FreeMemory ();

	// store out the flags used for matching
	image->texinfoflags = 0;

	return image;
}


/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin);
}


/*
================
R_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void R_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free special textures
	r_notexture->registration_sequence = r_registration_sequence;
	r_blacktexture->registration_sequence = r_registration_sequence;
	r_greytexture->registration_sequence = r_registration_sequence;
	r_whitetexture->registration_sequence = r_registration_sequence;

	for (i = 0, image = gltextures; i < MAX_GLTEXTURES; i++, image++)
	{
		// used this sequence
		if (image->registration_sequence == r_registration_sequence) continue;

		// disposable type
		if (image->flags & TEX_DISPOSABLE)
		{
			memset (image, 0, sizeof (*image));
		}
	}
}


/*
===============
R_InitImages
===============
*/
void R_InitImages (void)
{
	r_registration_sequence = 1;
	Draw_GetPalette ();
}


/*
===============
R_ShutdownImages
===============
*/
void R_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i = 0, image = gltextures; i < MAX_GLTEXTURES; i++, image++)
	{
		memset (image, 0, sizeof (*image));
	}

	Draw_ShutdownRawImage ();
}


image_t *R_LoadTexArray (char *base)
{
    int i;
    image_t* image = NULL;
    char* sb_nums[11] = { "_0", "_1", "_2", "_3", "_4", "_5", "_6", "_7", "_8", "_9", "_minus" };

    byte* sb_pic[11];
    byte* sb_palette[11];
    int		sb_width[11];
    int		sb_height[11];

    D3D12_SUBRESOURCE_DATA srd[11];
    D3D12_RESOURCE_DESC Desc;

    for (i = 0; i < 11; i++)
    {
        LoadPCX(va("pics/%s%s.pcx", base, sb_nums[i]), &sb_pic[i], &sb_palette[i], &sb_width[i], &sb_height[i]);

        if (!sb_pic[i]) ri.Sys_Error(ERR_FATAL, "malformed sb number set");
        if (sb_width[i] != sb_width[0]) ri.Sys_Error(ERR_FATAL, "malformed sb number set");
        if (sb_height[i] != sb_height[0]) ri.Sys_Error(ERR_FATAL, "malformed sb number set");

        srd[i].pData = GL_Image8To32(sb_pic[i], sb_width[i], sb_height[i], d_8to24table_alpha);
        srd[i].RowPitch = sb_width[i] << 2;
        srd[i].SlicePitch = 0;
    }

    image = GL_FindFreeImage(va("sb_%ss_texarray", base), sb_width[0], sb_height[0], it_pic);
    R_DescribeTexture(&Desc, sb_width[0], sb_height[0], 11, image->flags);
    image->textureId = DX12_CreateTexture(&Desc, &srd);

    return image;
}

void R_CreateTexture(texture_t* t, D3D12_SUBRESOURCE_DATA* srd, int width, int height, int arraysize, int flags)
{
    if (!srd) flags |= TEX_MUTABLE;

    R_DescribeTexture(&t->Desc, width, height, arraysize, flags);
    t->Id = DX12_CreateTexture(&t->Desc, srd);
}

void R_ReleaseTexture (texture_t *t)
{
    memset(t, 0, sizeof(texture_t));
}

int R_CreateTBuffer(void* data, int NumElements, int ElementSize)
{
    return DX12_CreateTextureBuffer(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NumElements, ElementSize, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// special texture loading
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
void R_CreateSpecialTextures (void)
{
	unsigned blacktexturedata = 0xff000000;
	unsigned greytexturedata = 0xff7f7f7f;
	unsigned whitetexturedata = 0xffffffff;
	byte notexturedata[16] = {0x00, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, 0x00};

	r_blacktexture = GL_LoadPic ("***r_blacktexture***", (byte *) &blacktexturedata, 1, 1, it_wall, 32, NULL);
	r_greytexture = GL_LoadPic ("***r_greytexture***", (byte *) &greytexturedata, 1, 1, it_wall, 32, NULL);
	r_whitetexture = GL_LoadPic ("***r_whitetexture***", (byte *) &whitetexturedata, 1, 1, it_wall, 32, NULL);
	r_notexture = GL_LoadPic ("***r_notexture***", notexturedata, 4, 4, it_wall, 8, d_8to24table_solid);
}


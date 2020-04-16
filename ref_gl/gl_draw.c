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

#include "gl_local.h"

image_t		*draw_chars;

extern	qboolean	scrap_dirty;
void Scrap_Upload (void);


/*
===============
Draw_InitLocal
===============
*/
void RefreshFont (void)
{
	con_font->modified = false;

	draw_chars = GL_FindImage (va("fonts/%s.pcx", con_font->string), it_pic);
	if (!draw_chars)
		draw_chars = GL_FindImage ("fonts/default.pcx", it_pic);
	if (!draw_chars) // fall back on old Q2 conchars
		draw_chars = GL_FindImage ("pics/conchars.pcx", it_pic);
	if (!draw_chars) // Knightmare- prevent crash caused by missing font
		ri.Sys_Error (ERR_FATAL, "RefreshFont: couldn't load fonts/default");

	GL_Bind( draw_chars->texnum );
}

void Draw_InitLocal (void)
{
	// load console characters (don't bilerp characters)
	//draw_chars = GL_FindImage ("pics/conchars.pcx", it_pic);
	//GL_Bind( draw_chars->texnum );
	image_t	*Draw_FindPic (char *name);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	RefreshFont();
}



/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character(float x, float y, float frow, float fcol, float size, float scale,
                    int red, int green, int blue, int alpha, qboolean italic)
{
	float italicAdd = 0;

	if (italic)
		italicAdd = scale*0.25;

	qglColor4ub( (byte)red, (byte)green, (byte)blue, (byte)alpha);

	qglTexCoord2f (fcol, frow);
	qglVertex2f (x+italicAdd, y);

	qglTexCoord2f (fcol + size, frow);
	qglVertex2f (x+scale+italicAdd, y);

	qglTexCoord2f (fcol + size, frow + size);
	qglVertex2f (x+scale-italicAdd, y+scale);

	qglTexCoord2f (fcol, frow + size);
	qglVertex2f (x-italicAdd, y+scale);
}

void Draw_ScaledChar(float x, float y, int num, float scale, 
	             int red, int green, int blue, int alpha, qboolean italic)
{
	int	row, col;
	float	frow, fcol, size, cscale;

	num &= 255;


	if (alpha >= 255)
		alpha = 255;
	else if (alpha <= 1)
		alpha = 1;

	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;
	cscale = scale * DEFAULT_FONT_SIZE;

	{
		qglDisable (GL_ALPHA_TEST);
		GL_TexEnv( GL_MODULATE );
		qglEnable(GL_BLEND);
		qglDepthMask   (false);
	}

	GL_Bind(draw_chars->texnum);


	qglBegin (GL_QUADS);
	{
		Draw_Character(x, y, frow, fcol, size, cscale,red, green, blue, alpha, italic);
	}
	qglEnd ();

	{
		qglDepthMask (true);
		GL_TexEnv( GL_REPLACE );
		qglDisable ( GL_BLEND );
		qglColor4f   (1,1,1,1);
		qglEnable (GL_ALPHA_TEST);
	}
}

void Draw_Char (int x, int y, int num, int alpha)
{
	int		row, col;
	float		frow, fcol, size, scale;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;
	scale = DEFAULT_FONT_SIZE;
	
	{
		qglDisable (GL_ALPHA_TEST);
		GL_TexEnv( GL_MODULATE );
		qglColor4ub( 255, 255, 255, (byte)alpha);
		qglEnable( GL_BLEND );
		qglDepthMask   (false);
	}

	GL_Bind (draw_chars->texnum);

	qglBegin (GL_QUADS);

	qglBegin (GL_QUADS);
        Draw_Character(x, y, frow, fcol, size, scale,
		       255, 255, 255, alpha, false);
	qglEnd ();

	{
		qglDepthMask (true);
		GL_TexEnv( GL_REPLACE );
		qglDisable ( GL_BLEND );
		qglColor4f   (1,1,1,1);

		qglEnable (GL_ALPHA_TEST);
	}
}

/*
=============
Draw_FindPic
=============
*/
image_t	*Draw_FindPic (char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
		gl = GL_FindImage (name+1, it_pic);

	return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *pic, float alpha)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty)
		Scrap_Upload ();

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglDisable (GL_ALPHA_TEST);

	// add alpha support
	if (gl->has_alpha || alpha < 1) {
		qglDisable(GL_ALPHA_TEST);

		GL_Bind(gl->texnum);

		GL_TexEnv(GL_MODULATE);
		qglColor4f(1, 1, 1, alpha);
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	} else
		GL_Bind (gl->texnum);
	qglBegin (GL_QUADS);
	qglTexCoord2f (gl->sl, gl->tl);
	qglVertex2f (x, y);
	qglTexCoord2f (gl->sh, gl->tl);
	qglVertex2f (x+w, y);
	qglTexCoord2f (gl->sh, gl->th);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f (gl->sl, gl->th);
	qglVertex2f (x, y+h);
	qglEnd ();
	
	// add alpha support
	if (gl->has_alpha || alpha < 1) {
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);

		qglEnable(GL_ALPHA_TEST);
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglEnable (GL_ALPHA_TEST);
}

void Draw_StretchPic2 (int x, int y, int w, int h, char *pic, float red, float green, float blue, float alpha)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty)
		Scrap_Upload ();

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglDisable (GL_ALPHA_TEST);

	// add alpha support
	if (gl->has_alpha || alpha < 1) {
		qglDisable(GL_ALPHA_TEST);

		GL_Bind(gl->texnum);

		GL_TexEnv(GL_MODULATE);
		qglColor4f(red, green, blue, alpha);
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	} else
		GL_Bind (gl->texnum);
	qglBegin (GL_QUADS);
	qglTexCoord2f (gl->sl, gl->tl);
	qglVertex2f (x, y);
	qglTexCoord2f (gl->sh, gl->tl);
	qglVertex2f (x+w, y);
	qglTexCoord2f (gl->sh, gl->th);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f (gl->sl, gl->th);
	qglVertex2f (x, y+h);
	qglEnd ();
	
	// add alpha support
	if (gl->has_alpha || alpha < 1) {
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);

		qglEnable(GL_ALPHA_TEST);
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglEnable (GL_ALPHA_TEST);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *pic, float alpha)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Can't find pic: %s\n", pic);
		return;
	}
	if (scrap_dirty)
		Scrap_Upload ();

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglDisable (GL_ALPHA_TEST);

	/* add alpha support */
	{
		qglDisable(GL_ALPHA_TEST);
		qglBindTexture(GL_TEXTURE_2D, gl->texnum);
		GL_TexEnv(GL_MODULATE);
		qglColor4f(1, 1, 1, 0.999);	/* need <1 for trans to work */
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	}

	GL_Bind (gl->texnum);
	qglBegin (GL_QUADS);
	qglTexCoord2f (gl->sl, gl->tl);
	qglVertex2f (x, y);
	qglTexCoord2f (gl->sh, gl->tl);
	qglVertex2f (x+gl->width, y);
	qglTexCoord2f (gl->sh, gl->th);
	qglVertex2f (x+gl->width, y+gl->height);
	qglTexCoord2f (gl->sl, gl->th);
	qglVertex2f (x, y+gl->height);
	qglEnd ();

	/* add alpha support */
	{
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);
		qglEnable(GL_ALPHA_TEST);
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !gl->has_alpha)
		qglEnable (GL_ALPHA_TEST);
}
/*
 * ============= Draw_ScaledPic =============
 */

void
Draw_ScaledPic(int x, int y, float scale, float alpha, char *pic, float red, float green, float blue, qboolean fixcoords, qboolean repscale)
{
	float		xoff, yoff;
	image_t        *gl;

	gl = Draw_FindPic(pic);
	if (!gl) {
		ri.Con_Printf(PRINT_DEVELOPER, "Can't find pic: %s\n", pic);
		return;
	}
	if (scrap_dirty)
		Scrap_Upload();

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !gl->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	/* add alpha support */
	{
		qglDisable(GL_ALPHA_TEST);

		qglBindTexture(GL_TEXTURE_2D, gl->texnum);

		GL_TexEnv(GL_MODULATE);
		qglColor4f(red, green, blue, alpha);
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	}

	/* NOTE: replace this with shaders as soon as they are supported */
	if (repscale)
		scale *= gl->replace_scale;	/* scale down if replacing a pcx image */

	if (fixcoords) {	/* Knightmare- whether to adjust coordinates for scaling */

		xoff = (gl->width * scale - gl->width) / 2;
		yoff = (gl->height * scale - gl->height) / 2;

		GL_Bind(gl->texnum);
		qglBegin(GL_QUADS);
		qglTexCoord2f(gl->sl, gl->tl);
		qglVertex2f(x - xoff, y - yoff);
		qglTexCoord2f(gl->sh, gl->tl);
		qglVertex2f(x + gl->width + xoff, y - yoff);
		qglTexCoord2f(gl->sh, gl->th);
		qglVertex2f(x + gl->width + xoff, y + gl->height + yoff);
		qglTexCoord2f(gl->sl, gl->th);
		qglVertex2f(x - xoff, y + gl->height + yoff);
		qglEnd();

	} else {
		xoff = gl->width * scale - gl->width;
		yoff = gl->height * scale - gl->height;
		
		GL_Bind(gl->texnum);
		qglBegin(GL_QUADS);
		qglTexCoord2f(gl->sl, gl->tl);
		qglVertex2f(x, y);
		qglTexCoord2f(gl->sh, gl->tl);
		qglVertex2f(x + gl->width + xoff, y);
		qglTexCoord2f(gl->sh, gl->th);
		qglVertex2f(x + gl->width + xoff, y + gl->height + yoff);
		qglTexCoord2f(gl->sl, gl->th);
		qglVertex2f(x, y + gl->height + yoff);
		qglEnd();
	}

	/* add alpha support */
	{
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);
		qglEnable(GL_ALPHA_TEST);
	}

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) &&
	      !gl->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *pic)
{
	image_t	*image;

	image = Draw_FindPic (pic);
	if (!image)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Can't find pic: %s\n", pic);
		return;
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !image->has_alpha)
		qglDisable (GL_ALPHA_TEST);

	GL_Bind (image->texnum);
	qglBegin (GL_QUADS);
	qglTexCoord2f (x/64.0, y/64.0);
	qglVertex2f (x, y);
	qglTexCoord2f ( (x+w)/64.0, y/64.0);
	qglVertex2f (x+w, y);
	qglTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f ( x/64.0, (y+h)/64.0 );
	qglVertex2f (x, y+h);
	qglEnd ();

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !image->has_alpha)
		qglEnable (GL_ALPHA_TEST);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if ( (unsigned)c > 255)
		ri.Sys_Error (ERR_FATAL, "Draw_Fill: bad color");

	qglDisable (GL_TEXTURE_2D);

	color.c = d_8to24table[c];
	qglColor3f (color.v[0]/255.0,
		color.v[1]/255.0,
		color.v[2]/255.0);

	qglBegin (GL_QUADS);

	qglVertex2f (x,y);
	qglVertex2f (x+w, y);
	qglVertex2f (x+w, y+h);
	qglVertex2f (x, y+h);

	qglEnd ();
	qglColor3f (1,1,1);
	qglEnable (GL_TEXTURE_2D);
}

// Knightmare added
/*
======================
R_DrawFill2

Fills a box of pixels with a
24-bit color w/ alpha
===========================
*/
void Draw_Fill2 (int x, int y, int w, int h, int red, int green, int blue, int alpha)
{
	red = min(red, 255);
	green = min(green, 255);
	blue = min(blue, 255);

	if (alpha >= 255)
		alpha = 255;
	else if (alpha <= 1)
		alpha = 1;

	qglDisable (GL_TEXTURE_2D);
	qglDisable(GL_ALPHA_TEST);
	GL_TexEnv( GL_MODULATE );
	qglEnable(GL_BLEND);
	qglDepthMask   (false);

	qglColor4ub ((byte)red, (byte)green, (byte)blue, (byte)alpha);
	
	qglBegin (GL_QUADS);

	qglVertex2f (x,y);
	qglVertex2f (x+w, y);
	qglVertex2f (x+w, y+h);
	qglVertex2f (x, y+h);

	qglEnd ();

	qglDepthMask (true);
	GL_TexEnv( GL_REPLACE );
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
	qglEnable(GL_ALPHA_TEST);
	qglEnable (GL_TEXTURE_2D);
}


//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	qglEnable (GL_BLEND);
	qglDisable (GL_TEXTURE_2D);
	qglColor4f (0, 0, 0, 0.8);
	qglBegin (GL_QUADS);

	qglVertex2f (0,0);
	qglVertex2f (vid.width, 0);
	qglVertex2f (vid.width, vid.height);
	qglVertex2f (0, vid.height);

	qglEnd ();
	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);
	qglDisable (GL_BLEND);
}

//====================================================================


/*
=============
Draw_StretchRaw
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	unsigned	image32[256*256];
	unsigned char image8[256*256];
	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	GL_Bind (0);

	if (rows<=256) {
		hscale = 1;
		trows = rows;
	}
	else {
		hscale = rows/256.0;
		trows = 256;
	}
	
	t = rows*hscale / 256;
	fracstep = cols*0x10000/256;

	if ( !qglColorTableEXT ) {
		unsigned *dest = image32;

		memset ( image32, 0, sizeof(unsigned)*256*256 );

		for (i=0 ; i<trows ; i++, dest+=256) {
			row = (int)(i*hscale);
			if (row > rows)
				break;
			source = data + cols*row;
			frac = fracstep >> 1;
			for (j=0 ; j<256 ; j+=4) {
				dest[j] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
				dest[j+1] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
				dest[j+2] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
				dest[j+3] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
			}
		}

		qglTexImage2D (GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	}
	else {
		unsigned char *dest = image8;

		memset ( image8, 0, sizeof(unsigned char)*256*256 );

		for (i=0 ; i<trows ; i++, dest+=256) {
			row = (int)(i*hscale);
			if (row > rows)
				break;
			source = data + cols*row;
			frac = fracstep >> 1;
			for (j=0 ; j<256 ; j+=4) {
				dest[j] = source[frac>>16];
				frac += fracstep;
				dest[j+1] = source[frac>>16];
				frac += fracstep;
				dest[j+2] = source[frac>>16];
				frac += fracstep;
				dest[j+3] = source[frac>>16];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, 
			      256, 256, 0, GL_COLOR_INDEX, 
			      GL_UNSIGNED_BYTE, image8 );
	}
	
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ((gl_config.renderer == GL_RENDERER_MCD ) ||
	    (gl_config.renderer & GL_RENDERER_RENDITION)) 
		qglDisable (GL_ALPHA_TEST);

	qglBegin (GL_QUADS);
	qglTexCoord2f (0, 0);
	qglVertex2f (x, y);
	qglTexCoord2f (1, 0);
	qglVertex2f (x+w, y);
	qglTexCoord2f (1, t);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f (0, t);
	qglVertex2f (x, y+h);
	qglEnd ();

	if ((gl_config.renderer == GL_RENDERER_MCD) ||
	    (gl_config.renderer & GL_RENDERER_RENDITION)) 
		qglEnable (GL_ALPHA_TEST);
}


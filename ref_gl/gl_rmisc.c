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
// r_misc.c

#include "gl_local.h"

#if defined(__unix__)
#include <jpeglib.h>
#include <png.h>
#endif

// MH - detail textures begin
// blatantly plagiarized darkplaces code
// no bound in Q2.
// this would be better as a #define but it's not runtime so it doesn't matter
// (dukey) fixed some small issues
// it now multitextures the texture if your card has enough texture units (3)
// otherwise it will revert to the old school method of render everything again
// and blending it with the original texture + lightmap which is slow as fuck frankly
// but the option is there for slow cards to go even slower ;) enjoy
// my radeon 9600se has 8 texture units :)

int bound (int smallest, int val, int biggest)
{
	if (val < smallest) return smallest;
	if (val > biggest)	return biggest;

	return val;
}


void fractalnoise(byte *noise, int size, int startgrid)
{
	int x, y, g, g2, amplitude, min, max, size1 = size - 1, sizepower, gridpower;
	int *noisebuf;

	#define n(x,y) noisebuf[((y)&size1)*size+((x)&size1)]

	for (sizepower = 0;(1 << sizepower) < size;sizepower++);
	if (size != (1 << sizepower))
		Sys_Error("fractalnoise: size must be power of 2\n");

	for (gridpower = 0;(1 << gridpower) < startgrid;gridpower++);
	if (startgrid != (1 << gridpower))
		Sys_Error("fractalnoise: grid must be power of 2\n");

	startgrid = bound (0, startgrid, size);

	amplitude = 0xFFFF; // this gets halved before use
	noisebuf = Q_malloc(size*size*sizeof(int));
	memset(noisebuf, 0, size*size*sizeof(int));

	for (g2 = startgrid;g2;g2 >>= 1)
	{
		// brownian motion (at every smaller level there is random behavior)
		amplitude >>= 1;
		for (y = 0;y < size;y += g2)
			for (x = 0;x < size;x += g2)
				n(x,y) += (rand()&amplitude);

		g = g2 >> 1;
		if (g)
		{
			// subdivide, diamond-square algorithm (really this has little to do with squares)
			// diamond
			for (y = 0;y < size;y += g2)
				for (x = 0;x < size;x += g2)
					n(x+g,y+g) = (n(x,y) + n(x+g2,y) + n(x,y+g2) + n(x+g2,y+g2)) >> 2;
			// square
			for (y = 0;y < size;y += g2)
				for (x = 0;x < size;x += g2)
				{
					n(x+g,y) = (n(x,y) + n(x+g2,y) + n(x+g,y-g) + n(x+g,y+g)) >> 2;
					n(x,y+g) = (n(x,y) + n(x,y+g2) + n(x-g,y+g) + n(x+g,y+g)) >> 2;
				}
		}
	}
	// find range of noise values
	min = max = 0;
	for (y = 0;y < size;y++)
		for (x = 0;x < size;x++)
		{
			if (n(x,y) < min) min = n(x,y);
			if (n(x,y) > max) max = n(x,y);
		}
	max -= min;
	max++;
	// normalize noise and copy to output
	for (y = 0;y < size;y++)
		for (x = 0;x < size;x++)
			*noise++ = (byte) (((n(x,y) - min) * 256) / max);
	Q_free(noisebuf);

	#undef n
}


void R_BuildDetailTexture (void)
{
	int x, y, light;
	float vc[3], vx[3], vy[3], vn[3], lightdir[3];

	// increase this if you want
	#define DETAILRESOLUTION 256

	byte data[DETAILRESOLUTION][DETAILRESOLUTION][4], noise[DETAILRESOLUTION][DETAILRESOLUTION];

	// this looks odd, but it's necessary cos Q2's uploader will lightscale the texture, which
	// will cause all manner of unholy havoc.  So we need to run it through GL_LoadPic before
	// we even fill in the data just to fill in the rest of the image_t struct, then we'll
	// build the texture for OpenGL manually later on.
	r_detailtexture = GL_LoadPic ("***detail***", (byte *) data, DETAILRESOLUTION, DETAILRESOLUTION, it_wall, 32);

	lightdir[0] = 0.5;
	lightdir[1] = 1;
	lightdir[2] = -0.25;
	VectorNormalize(lightdir);

	fractalnoise(&noise[0][0], DETAILRESOLUTION, DETAILRESOLUTION >> 4);
	for (y = 0;y < DETAILRESOLUTION;y++)
	{
		for (x = 0;x < DETAILRESOLUTION;x++)
		{
			vc[0] = x;
			vc[1] = y;
			vc[2] = noise[y][x] * (1.0f / 32.0f);
			vx[0] = x + 1;
			vx[1] = y;
			vx[2] = noise[y][(x + 1) % DETAILRESOLUTION] * (1.0f / 32.0f);
			vy[0] = x;
			vy[1] = y + 1;
			vy[2] = noise[(y + 1) % DETAILRESOLUTION][x] * (1.0f / 32.0f);
			VectorSubtract(vx, vc, vx);
			VectorSubtract(vy, vc, vy);
			CrossProduct(vx, vy, vn);
			VectorNormalize(vn);
			light = 128 - DotProduct(vn, lightdir) * 128;
			light = bound(0, light, 255);
			data[y][x][0] = data[y][x][1] = data[y][x][2] = light;
			data[y][x][3] = 255;
		}
	}

	// now we build the texture manually.  you can reuse this code for auto mipmap generation
	// in other contexts if you wish.  defines are in qgl.h
	// first, bind the texture.  probably not necessary, but it seems like good practice
	GL_Bind (r_detailtexture->texnum);
	
#if defined(USE_GLU)
	// upload the correct texture data without any lightscaling interference
	gluBuild2DMipmaps (GL_TEXTURE_2D, GL_RGBA, DETAILRESOLUTION, DETAILRESOLUTION, GL_RGBA, GL_UNSIGNED_BYTE, (byte *) data);
#else
	// set the hint for mipmap generation
	qglHint (GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	// now switch mipmap generation on.
	// this has been part of the core OpenGL specification since version 1.4, so there's 
	// no real need to check for extension support and other tiresomely silly blather,
	// unless you have a really weird OpenGL implementation.  it does, however, even
	// work on ATI cards, so you should have no trouble with it.
	qglTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	// upload the correct texture data without any lightscaling interference
	qglTexImage2D (GL_TEXTURE_2D, 
				   0, 
				   GL_RGBA, 
				   DETAILRESOLUTION, 
				   DETAILRESOLUTION, 
				   0, 
				   GL_RGBA, 
				   GL_UNSIGNED_BYTE, 
				   (byte *) data);
#endif
	// set some quick and ugly filtering modes.  detail texturing works by scrunching a
	// large image into a small space, so there's no need for sexy filtering (change this,
	// turn off the blend in R_DrawDetailSurfaces, and compare if you don't believe me).
	// this also helps to take some of the speed hit from detail texturing away.
	// fucks up for some reason so using different filtering.
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

}
// MH - detail textures end


static float	Diamond8x[8][8] = {
	{0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0},
	{0.0, 0.0, 0.2, 0.3, 0.3, 0.2, 0.0, 0.0},
	{0.0, 0.2, 0.4, 0.6, 0.6, 0.4, 0.2, 0.0},
	{0.1, 0.3, 0.6, 0.9, 0.9, 0.6, 0.3, 0.1},
	{0.1, 0.3, 0.6, 0.9, 0.9, 0.6, 0.3, 0.1},
	{0.0, 0.2, 0.4, 0.6, 0.6, 0.4, 0.2, 0.0},
	{0.0, 0.0, 0.2, 0.3, 0.3, 0.2, 0.0, 0.0},
	{0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0}
};

static float	Diamond6x[6][6] = {
	{0.0, 0.0, 0.1, 0.1, 0.0, 0.0},
	{0.0, 0.3, 0.5, 0.5, 0.3, 0.0},
	{0.1, 0.5, 0.9, 0.9, 0.5, 0.1},
	{0.1, 0.5, 0.9, 0.9, 0.5, 0.1},
	{0.0, 0.3, 0.5, 0.5, 0.3, 0.0},
	{0.0, 0.0, 0.1, 0.1, 0.0, 0.0}
};

static float	Diamond4x[4][4] = {
	{0.3, 0.4, 0.4, 0.3},
	{0.4, 0.9, 0.9, 0.4},
	{0.4, 0.9, 0.9, 0.4},
	{0.3, 0.4, 0.4, 0.3}
};

int BLOOM_SIZE;

int r_screendownsamplingtexture_size,
    screen_texture_width, screen_texture_height,
    r_screenbackuptexture_size;

//current refdef size:
int curView_x,
    curView_y,
    curView_width,
    curView_height;

//texture coordinates of screen data inside screentexture
float screenText_tcw,
      screenText_tch,
      sampleText_tcw,
      sampleText_tch;

int sample_width,
    sample_height;
/*
=================
R_Bloom_InitBackUpTexture
=================
*/
void R_Bloom_InitBackUpTexture( int width, int height )
{
	byte	*data;
	
	data = Q_malloc( width * height * 4 );
	memset( data, 0, width * height * 4 );

	r_screenbackuptexture_size = width;

	r_bloombackuptexture = GL_LoadPic( "***r_bloombackuptexture***", (byte *)data, width, height, it_pic, 3 );
	
	Q_free ( data );
}
/*
=================
R_Bloom_InitEffectTexture
=================
*/
void R_Bloom_InitEffectTexture( void )
{
	byte	*data;
	float	bloomsizecheck;
	
	if( (int)gl_bloom_sample_size->value < 32 )
		ri.Cvar_SetValue ("gl_bloom_sample_size", 32);

	//make sure bloom size is a power of 2
	BLOOM_SIZE = (int)gl_bloom_sample_size->value;
	bloomsizecheck = (float)BLOOM_SIZE;
	while(bloomsizecheck > 1.0f) bloomsizecheck /= 2.0f;
	if( bloomsizecheck != 1.0f )
	{
		BLOOM_SIZE = 32;
		while( BLOOM_SIZE < (int)gl_bloom_sample_size->value )
			BLOOM_SIZE *= 2;
	}

	//make sure bloom size doesn't have stupid values
	if( BLOOM_SIZE > screen_texture_width ||
		BLOOM_SIZE > screen_texture_height )
		BLOOM_SIZE = min( screen_texture_width, screen_texture_height );

	if( BLOOM_SIZE != (int)gl_bloom_sample_size->value )
		ri.Cvar_SetValue ("gl_bloom_sample_size", BLOOM_SIZE);

	data = Q_malloc( BLOOM_SIZE * BLOOM_SIZE * 4 );
	memset( data, 0, BLOOM_SIZE * BLOOM_SIZE * 4 );

	r_bloomeffecttexture = GL_LoadPic( "***r_bloomeffecttexture***", (byte *)data, BLOOM_SIZE, BLOOM_SIZE, it_pic, 3 );
	
	Q_free ( data );
}
/*
=================
R_Bloom_InitTextures
=================
*/
void R_Bloom_InitTextures(void)
{
	byte	*data;
	int	size;

	gl_bloom = ri.Cvar_Get("gl_bloom", "0", CVAR_ARCHIVE);
	gl_bloom_alpha = ri.Cvar_Get("gl_bloom_alpha", "0.5", CVAR_ARCHIVE);
	gl_bloom_diamond_size = ri.Cvar_Get("gl_bloom_diamond_size", "8", CVAR_ARCHIVE);
	gl_bloom_intensity = ri.Cvar_Get("gl_bloom_intensity", "0.6", CVAR_ARCHIVE);
	gl_bloom_darken = ri.Cvar_Get("gl_bloom_darken", "4", CVAR_ARCHIVE);
	gl_bloom_sample_size = ri.Cvar_Get("gl_bloom_sample_size", "128", CVAR_ARCHIVE);
	gl_bloom_fast_sample = ri.Cvar_Get("gl_bloom_fast_sample", "0", CVAR_ARCHIVE);

	//find closer power of 2 to screen size 
	for (screen_texture_width = 1; screen_texture_width < vid.width; screen_texture_width *= 2);
	for (screen_texture_height = 1; screen_texture_height < vid.height; screen_texture_height *= 2);

	//init the screen texture
	size = screen_texture_width * screen_texture_height * 4;
	data = Q_malloc( size );
	memset( data, 255, size );
	r_bloomscreentexture = GL_LoadPic("***r_bloomscreentexture***", (byte *)data, 
	                                  screen_texture_width, screen_texture_height, it_pic, 3);
	Q_free ( data );

	//validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture ();

	//if screensize is more than 2x the bloom effect texture, set up for stepped downsampling
	r_bloomdownsamplingtexture = NULL;
	r_screendownsamplingtexture_size = 0;
	if( vid.width > (BLOOM_SIZE * 2) && !gl_bloom_fast_sample->value )
	{
		r_screendownsamplingtexture_size = (int)(BLOOM_SIZE * 2);
		data = Q_malloc( r_screendownsamplingtexture_size * r_screendownsamplingtexture_size * 4 );
		memset( data, 0, r_screendownsamplingtexture_size * r_screendownsamplingtexture_size * 4 );
		r_bloomdownsamplingtexture = GL_LoadPic("***r_bloomdownsamplingtexture***", (byte *)data,
		                                        r_screendownsamplingtexture_size,
							r_screendownsamplingtexture_size, it_pic, 3);
		Q_free ( data );
	}

	//Init the screen backup texture
	if( r_screendownsamplingtexture_size )
		R_Bloom_InitBackUpTexture( r_screendownsamplingtexture_size, r_screendownsamplingtexture_size );
	else
		R_Bloom_InitBackUpTexture( BLOOM_SIZE, BLOOM_SIZE );
	
}
/*
=================
R_Bloom_DrawEffect
=================
*/
void R_Bloom_DrawEffect( void )
{
	GL_Bind(r_bloomeffecttexture->texnum);  
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);
	qglColor4f(gl_bloom_alpha->value, gl_bloom_alpha->value, gl_bloom_alpha->value, 1.0f);
	GL_TexEnv(GL_MODULATE);
	qglBegin(GL_QUADS);
	qglTexCoord2f(0, sampleText_tch);
	qglVertex2f(curView_x, curView_y);
	qglTexCoord2f(0, 0);
	qglVertex2f(curView_x, curView_y + curView_height);	
	qglTexCoord2f(sampleText_tcw,0);
	qglVertex2f(curView_x + curView_width,curView_y + curView_height);	
	qglTexCoord2f(sampleText_tcw, sampleText_tch);	
	qglVertex2f(curView_x + curView_width, curView_y);
	qglEnd();
	qglDisable(GL_BLEND);
}

void R_Bloom_GeneratexDiamonds( void )
{
	int	i, j;
	static float intensity;

	//set up sample size workspace
	qglViewport(0, 0, sample_width, sample_height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho(0, sample_width, sample_height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();

	//copy small scene into r_bloomeffecttexture
	GL_Bind(r_bloomeffecttexture->texnum);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);

	//start modifying the small scene corner
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	qglEnable(GL_BLEND);

	//darkening passes
	if(gl_bloom_darken->value ) {
		qglBlendFunc(GL_DST_COLOR, GL_ZERO);
		GL_TexEnv(GL_MODULATE);
		
		for(i=0; i<gl_bloom_darken->value ;i++) {
			R_Bloom_SamplePass( 0, 0 );
		}
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);
	}

	//bluring passes
	//qglBlendFunc(GL_ONE, GL_ONE);
	qglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	
	if(gl_bloom_diamond_size->value > 7 || gl_bloom_diamond_size->value <= 3)
	{
		if( (int)gl_bloom_diamond_size->value != 8 ) ri.Cvar_SetValue( "gl_bloom_diamond_size", 8 );

		for(i=0; i<gl_bloom_diamond_size->value; i++) {
			for(j=0; j<gl_bloom_diamond_size->value; j++) {
				intensity = gl_bloom_intensity->value * 0.3 * Diamond8x[i][j];
				if( intensity < 0.01f ) continue;
				qglColor4f( intensity, intensity, intensity, 1.0);
				R_Bloom_SamplePass( i-4, j-4 );
			}
		}
	} else if(gl_bloom_diamond_size->value > 5 ) {
		
		if(gl_bloom_diamond_size->value != 6 ) ri.Cvar_SetValue( "gl_bloom_diamond_size", 6 );

		for(i=0; i<gl_bloom_diamond_size->value; i++) {
			for(j=0; j<gl_bloom_diamond_size->value; j++) {
				intensity = gl_bloom_intensity->value * 0.5 * Diamond6x[i][j];
				if( intensity < 0.01f ) continue;
				qglColor4f( intensity, intensity, intensity, 1.0);
				R_Bloom_SamplePass( i-3, j-3 );
			}
		}
	} else if(gl_bloom_diamond_size->value > 3 ) {

		if( (int)gl_bloom_diamond_size->value != 4 ) ri.Cvar_SetValue( "gl_bloom_diamond_size", 4 );

		for(i=0; i<gl_bloom_diamond_size->value; i++) {
			for(j=0; j<gl_bloom_diamond_size->value; j++) {
				intensity = gl_bloom_intensity->value * 0.8f * Diamond4x[i][j];
				if( intensity < 0.01f ) continue;
				qglColor4f( intensity, intensity, intensity, 1.0);
				R_Bloom_SamplePass( i-2, j-2 );
			}
		}
	}
	
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);

	//restore full screen workspace
	qglViewport( 0, 0, vid.width, vid.height );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, vid.width, vid.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
}

void R_Bloom_DownsampleView( void )
{
	qglDisable( GL_BLEND );
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	//stepped downsample
	if( r_screendownsamplingtexture_size )
	{
		int	midsample_width = r_screendownsamplingtexture_size * sampleText_tcw;
		int	midsample_height = r_screendownsamplingtexture_size * sampleText_tch;
		
		//copy the screen and draw resized
		GL_Bind(r_bloomscreentexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curView_x, vid.height - (curView_y + curView_height),
		                     curView_width, curView_height);
		R_Bloom_Quad(0, vid.height-midsample_height, midsample_width, midsample_height,
		             screenText_tcw, screenText_tch  );
		
		//now copy into Downsampling (mid-sized) texture
		GL_Bind(r_bloomdownsamplingtexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, midsample_width, midsample_height);

		//now draw again in bloom size
		qglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		R_Bloom_Quad(0, vid.height-sample_height, sample_width, sample_height,
		             sampleText_tcw, sampleText_tch );
		
		//now blend the big screen texture into the bloom generation space (hoping it adds some blur)
		qglEnable( GL_BLEND );
		qglBlendFunc(GL_ONE, GL_ONE);
		qglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		GL_Bind(r_bloomscreentexture->texnum);
		R_Bloom_Quad(0, vid.height-sample_height, sample_width, sample_height,
		             screenText_tcw, screenText_tch );
		qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		qglDisable( GL_BLEND );

	} else {	//downsample simple

		GL_Bind(r_bloomscreentexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curView_x, vid.height - (curView_y + curView_height),
		                     curView_width, curView_height);
		R_Bloom_Quad(0, vid.height-sample_height, sample_width, sample_height,
		             screenText_tcw, screenText_tch );
	}
}

void R_BloomBlend ( refdef_t *fd )
{
	if( !(fd->rdflags & RDF_BLOOM) || !gl_bloom->value )
		return;

	
	if(screen_texture_width < BLOOM_SIZE ||
	   screen_texture_height < BLOOM_SIZE )
		return;
	
	//set up full screen workspace
	qglViewport( 0, 0, vid.width, vid.height );
	qglDisable( GL_DEPTH_TEST );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, vid.width, vid.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
	qglDisable(GL_CULL_FACE);
	qglDepthMask(false);
	qglDisable( GL_BLEND );
	qglEnable( GL_TEXTURE_2D );

	qglColor4f( 1, 1, 1, 1 );

	//set up current sizes
	curView_x = fd->x;
	curView_y = fd->y;
	curView_width = fd->width;
	curView_height = fd->height;
	screenText_tcw = ((float)fd->width / (float)screen_texture_width);
	screenText_tch = ((float)fd->height / (float)screen_texture_height);
	if( fd->height > fd->width ) {
		sampleText_tcw = ((float)fd->width / (float)fd->height);
		sampleText_tch = 1.0f;
	} else {
		sampleText_tcw = 1.0f;
		sampleText_tch = ((float)fd->height / (float)fd->width);
	}
	sample_width = BLOOM_SIZE * sampleText_tcw;
	sample_height = BLOOM_SIZE * sampleText_tch;
	
	//copy the screen space we'll use to work into the backup texture
	GL_Bind(r_bloombackuptexture->texnum);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
	                     r_screenbackuptexture_size * sampleText_tcw,
			     r_screenbackuptexture_size * sampleText_tch);

	//create the bloom image
	R_Bloom_DownsampleView();
	R_Bloom_GeneratexDiamonds();

	//restore the screen-backup to the screen
	qglDisable(GL_BLEND);
	GL_Bind(r_bloombackuptexture->texnum);
	qglColor4f( 1, 1, 1, 1 );
	R_Bloom_Quad(0, vid.height - (r_screenbackuptexture_size * sampleText_tch),
		     r_screenbackuptexture_size * sampleText_tcw,
		     r_screenbackuptexture_size * sampleText_tch,
		     sampleText_tcw,
		     sampleText_tch );

	R_Bloom_DrawEffect();
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglEnable (GL_TEXTURE_2D);
	qglEnable( GL_DEPTH_TEST );
	qglColor4f(1,1,1,1);
	qglDepthMask(true);
}

/*
==================
R_InitParticleTexture
==================
*/
#if defined(PARTICLESYSTEM)
void SetParticlePicture (int num, char *name)
{
	r_particletexture[num] = GL_FindImage(name, it_part);
	if (!r_particletexture[num])
		r_particletexture[num] = r_notexture;
}
byte	dottexture[8][8] =
{
	{075,075,075,075,255,255,255,255},
	{075,075,075,075,255,255,255,255},
	{075,075,075,075,255,255,255,255},
	{075,075,075,075,255,255,255,255},
	{255,255,255,255,175,175,175,175},
	{255,255,255,255,175,175,175,175},
	{255,255,255,255,175,175,175,175},
	{255,255,255,255,175,175,175,175},
};
#else
byte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};
#endif
void R_InitParticleTexture (void)
{
	int	i, x, y;
	byte	data[8][8][4];
#if !defined(PARTICLESYSTEM)
	byte	data1[16][16][4];
	int	dx2, dy, d;
#endif
	char flares[MAX_QPATH];

	//
	// particle texture
	//
#if !defined(PARTICLESYSTEM)
	for (x = 0; x < 16; x++) {
		dx2 = x - 8;
		dx2 *= dx2;
		for (y = 0; y < 16; y++) {
			dy = y - 8;
			d = 255 - 4 * (dx2 + (dy * dy));
			if (d <= 0) {
				d = 0;
				data1[y][x][0] = 0;
				data1[y][x][1] = 0;
				data1[y][x][2] = 0;
			} else {
				data1[y][x][0] = 255;
				data1[y][x][1] = 255;
				data1[y][x][2] = 255;
			}

			data1[y][x][3] = (byte) d;
		}
	}
	r_particletexture = GL_LoadPic ("***particle***", (byte *)data1, 16, 16, 0, 32);
#endif

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			/*data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;*/
			data[y][x][0] = dottexture[y][x];
			data[y][x][1] = dottexture[y][x];
			data[y][x][2] = dottexture[y][x];
			data[y][x][3] = 255;
		}
	}
	
	r_notexture = GL_LoadPic ("***r_notexture***", (byte *)data, 8, 8, it_wall, 32);
	
#if defined(PARTICLESYSTEM)
	r_particlebeam = GL_FindImage("particles/beam.png", it_part);
	if (!r_particlebeam)
		r_particlebeam = r_notexture;

	for (x=0 ; x<PARTICLE_TYPES ; x++)
		r_particletexture[x] = NULL;
#endif
	r_caustictexture = Draw_FindPic(gl_water_caustics_image->string);
	if(!r_caustictexture)
		r_caustictexture = r_notexture;
	
	r_shelltexture = Draw_FindPic(gl_shell_image->string);
	if (!r_shelltexture) {
        	r_shelltexture = r_notexture;
	}
		
	r_radarmap = GL_FindImage("gfx/radarmap.pcx", it_skin);
	if (!r_radarmap) {
		r_radarmap = r_notexture;
	}
	
	r_around = GL_FindImage("gfx/around.pcx", it_skin);
	if (!r_around) {
		r_around = r_notexture;
	}
	
	for (i=0; i<MAX_FLARE; i++) {
		Com_sprintf (flares, sizeof(flares), "gfx/flare%i.pcx", i);
		r_flare[i] = GL_FindImage(flares, it_pic);
		if(!r_flare[i])
			r_flare[i] = r_notexture;
	}
	
#if !defined(PARTICLESYSTEM)
	{       
                /* Decals */
		r_decaltexture[DECAL_BLOOD1]	= GL_FindImage("gfx/decal_blood1.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD2]	= GL_FindImage("gfx/decal_blood2.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD3]	= GL_FindImage("gfx/decal_blood3.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD4]	= GL_FindImage("gfx/decal_blood4.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD5]	= GL_FindImage("gfx/decal_blood5.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD6]	= GL_FindImage("gfx/decal_blood6.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD7]	= GL_FindImage("gfx/decal_blood7.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD8]	= GL_FindImage("gfx/decal_blood8.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD9]	= GL_FindImage("gfx/decal_blood9.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD10]	= GL_FindImage("gfx/decal_blood10.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD11]	= GL_FindImage("gfx/decal_blood11.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD12]	= GL_FindImage("gfx/decal_blood12.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD13]	= GL_FindImage("gfx/decal_blood13.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD14]	= GL_FindImage("gfx/decal_blood14.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD15]	= GL_FindImage("gfx/decal_blood15.pcx",it_pic);
		r_decaltexture[DECAL_BLOOD16]	= GL_FindImage("gfx/decal_blood16.pcx",it_pic);
		r_decaltexture[DECAL_BULLET]	= GL_FindImage("gfx/decal_bullet.pcx",it_pic);
		r_decaltexture[DECAL_BLASTER]	= GL_FindImage("gfx/decal_blaster.pcx",it_pic);
		r_decaltexture[DECAL_EXPLODE]	= GL_FindImage("gfx/decal_explode.pcx",it_pic);
		r_decaltexture[DECAL_RAIL]	= GL_FindImage("gfx/decal_rail.pcx",it_pic);
		r_decaltexture[DECAL_BFG]	= GL_FindImage("gfx/decal_bfg.pcx",it_pic);

		for (x = 0; x < DECAL_MAX; x++) 
			if (!r_decaltexture[x]) r_decaltexture[x] = r_notexture;
                        
	}
#endif
	
	// MH - detail textures begin
	// builds and uploads detailed texture
	R_BuildDetailTexture ();
	// MH - detail textures end
	R_Bloom_InitTextures();
#if defined(PARTICLESYSTEM)
	ri.SetParticleImages();
#endif
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

/* jitgamma -- apply video gammaramp to screenshot */
void GammaShots(byte *rgbdata, int w, int h)
{
	register int i,j,k;
	extern unsigned short gamma_ramp[3][256];

	if (!gl_state.hwgamma)
		return; /* don't apply gamma if it's not turned on! */
	else
	{
		k = w*h;

		for(i=0; i<k; i++)
			for(j=0; j<3; j++)
				rgbdata[i*3+j] = ((gamma_ramp[j][rgbdata[i*3+j]]) >> 8);
	}
}

/*
 * ==================
 * GL_ScreenShot_JPG By Robert 'Heffo' Heffernan
 * ==================
 */
void GL_ScreenShot_JPG(void)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte           *rgbdata;
	JSAMPROW	s[1];
	FILE           *file;
	char		picname[80], checkname[MAX_OSPATH];
	int		i, offset;

	/* Create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	/* Knightmare- changed screenshot filenames, up to 100 screenies */
	for (i = 0; i <= 999; i++) {
		int	one, ten, hundred;

		hundred = i * 0.01;
		ten = (i - hundred * 100) * 0.1;
		one = i - hundred * 100 - ten * 10;
		
		Com_sprintf(picname, sizeof(picname), "q2p_%i%i%i.jpg", hundred, ten, one);
		Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		file = fopen(checkname, "rb");
		if (!file)
			break;	/* file doesn't exist */
		fclose(file);
	}
	if (i == 1000) {
		ri.Con_Printf(PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n");
		return;
	}
	/* Open the file for Binary Output */
	file = fopen(checkname, "wb");
	if (!file) {
		ri.Con_Printf(PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n");
		return;
	}
	/* Allocate room for a copy of the framebuffer */
	rgbdata = Q_malloc(vid.width * vid.height * 3);
	if (!rgbdata) {
		fclose(file);
		return;
	}
	/* Read the framebuffer into our storage */
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
	
	/* jitgamma -- apply video gammaramp to screenshot */
	GammaShots(rgbdata, vid.width, vid.height);

	/* Initialise the JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	/* Setup JPEG Parameters */
	cinfo.image_width = vid.width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	if ((gl_screenshot_jpeg_quality->value >= 101) || (gl_screenshot_jpeg_quality->value <= 0))
		ri.Cvar_Set("gl_screenshot_jpeg_quality", "95");
	jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality->value, TRUE);

	/* Start Compression */
	jpeg_start_compress(&cinfo, true);

	/* Feed Scanline data */
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while (cinfo.next_scanline < cinfo.image_height) {
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	/* Finish Compression */
	jpeg_finish_compress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_compress(&cinfo);

	/* Close File */
	fclose(file);

	/* Free Temp Framebuffer */
	Q_free(rgbdata);

	/* Done! */
	ri.Con_Printf(PRINT_ALL, "Wrote %s\n", picname);
}

void GL_ScreenShot_JPG_Levelshots(void)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte           *rgbdata;
	JSAMPROW	s[1];
	FILE           *file;
	char		picname[80], checkname[MAX_OSPATH];
	int		i, offset;

	/* Create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/levelshots", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	/* Knightmare- changed screenshot filenames, up to 100 screenies */
	for (i = 0; i <= 999; i++) {
		Com_sprintf(picname, sizeof(picname), "%s.jpg", ri.FS_Mapname());
		Com_sprintf(checkname, sizeof(checkname), "%s/levelshots/%s", ri.FS_Gamedir(), picname);
		file = fopen(checkname, "rb");
		if (!file)
			break;	/* file doesn't exist */
		fclose(file);
	}
	if (i == 1000) {
		ri.Con_Printf(PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n");
		return;
	}
	/* Open the file for Binary Output */
	file = fopen(checkname, "wb");
	if (!file) {
		ri.Con_Printf(PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n");
		return;
	}
	/* Allocate room for a copy of the framebuffer */
	rgbdata = Q_malloc(vid.width * vid.height * 3);
	if (!rgbdata) {
		fclose(file);
		return;
	}
	/* Read the framebuffer into our storage */
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
	
	/* jitgamma -- apply video gammaramp to screenshot */
	GammaShots(rgbdata, vid.width, vid.height);

	/* Initialise the JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	/* Setup JPEG Parameters */
	cinfo.image_width = vid.width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	if ((gl_screenshot_jpeg_quality->value >= 101) || (gl_screenshot_jpeg_quality->value <= 0))
		ri.Cvar_Set("gl_screenshot_jpeg_quality", "95");
	jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality->value, TRUE);

	/* Start Compression */
	jpeg_start_compress(&cinfo, true);

	/* Feed Scanline data */
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while (cinfo.next_scanline < cinfo.image_height) {
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	/* Finish Compression */
	jpeg_finish_compress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_compress(&cinfo);

	/* Close File */
	fclose(file);

	/* Free Temp Framebuffer */
	Q_free(rgbdata);

	/* Done! */
	ri.Con_Printf(PRINT_ALL, "Wrote levelshot %s\n", picname);
}


typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/* 
================== 
GL_ScreenShot_f
================== 
*/  
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;
	extern cvar_t	*levelshots;

	if (levelshots->value) {
		GL_ScreenShot_JPG_Levelshots();
		return;
	}
	
	/* Heffo - JPEG Screenshots */
	if (gl_screenshot_jpeg->value) {
		GL_ScreenShot_JPG();
		return;
	}
	
	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	Q_strncpyz(picname, "q2p00.tga", sizeof(picname));

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		ri.Con_Printf (PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n"); 
		return;
 	}


	buffer = Q_malloc(vid.width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	qglReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 
	
	/* jitgamma -- apply video gammaramp to screenshot */
	GammaShots(buffer + 18, vid.width, vid.height);

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	Q_free (buffer);
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	ri.Con_Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	ri.Con_Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	ri.Con_Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
	ri.Con_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearColor (1,0, 0.5 , 0.5);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);
	
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);

	qglColor4f (1,1,1,1);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

#if !defined(PARTICLESYSTEM)
	if ( qglPointParameterfEXT ) {
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable( GL_POINT_SMOOTH );
		qglPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		qglPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		qglPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}
	if (qglColorTableEXT && gl_ext_palettedtexture->value) {
		qglEnable( GL_SHARED_TEXTURE_PALETTE_EXT );
		GL_SetTexturePalette( d_8to24table );
	}
#endif
	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled ) 
		{
#ifdef _WIN32
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}

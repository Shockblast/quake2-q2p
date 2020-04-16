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

#ifndef _GL_LOCAL_H_
#define _GL_LOCAL_H_

#ifdef _WIN32
#include <windows.h>
#pragma warning (disable: 4127)
#endif

#include <stdio.h>
#include <GL/gl.h>

#ifdef _WIN32
#define USE_GLU
#include "../win32/include/glext.h"
#include "../win32/include/jpeglib.h"
#include "../win32/include/png.h"
#else
#include <GL/glx.h>
#include <GL/glext.h>
#endif

#if defined(USE_GLU)
#include <GL/glu.h>
#endif

#include <math.h>

#ifndef __unix__
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif
#endif

#include "../client/ref.h"

#include "qgl.h"

#define	REF_VERSION	"GL 0.1-2"

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#ifndef __VIDDEF_T
#define __VIDDEF_T
typedef struct
{
	unsigned		width, height;			// coordinates from main game
} viddef_t;
#endif

extern	viddef_t	vid;


/*
  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped.
  model skin
  sprite frame
  wall texture
  pic
*/
typedef enum 
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
#if defined(PARTICLESYSTEM)
	,
	it_part
#endif
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	imagetype_t	type;
	int		width, height;				// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;	// for sort-by-texture world drawing
	GLuint		texnum;						// gl texture binding
	float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	qboolean	scrap;
	qboolean	has_alpha;

	qboolean 	paletted;
	float		replace_scale;
} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		1152
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	1024

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "gl_model.h"
#define CONTENTS_NODE -1

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;


#define	MAX_LBM_HEIGHT		480

#define ON_EPSILON		0.1
#define BACKFACE_EPSILON	0.01

#define MAX_FLARE 8

//====================================================

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int		numgltextures;


#if defined(PARTICLESYSTEM)
#define PARTICLE_TYPES 1024
extern	image_t		*r_particletexture[PARTICLE_TYPES];
extern	image_t		*r_particlebeam;
#else
extern	image_t		*r_particletexture;
extern  image_t 	*r_decaltexture[DECAL_MAX];
#endif
extern	image_t		*r_notexture;
extern	image_t		*r_detailtexture;
extern  image_t		*r_caustictexture;
extern  image_t         *r_shelltexture;
extern  image_t 	*r_radarmap;
extern  image_t 	*r_around;
extern  image_t		*r_flare[MAX_FLARE];
extern	image_t         *r_bloomscreentexture;
extern	image_t         *r_bloomeffecttexture;
extern	image_t         *r_bloombackuptexture;
extern	image_t         *r_bloomdownsamplingtexture;

extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int		r_visframecount;
extern	int		r_framecount;
extern	cplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;
extern  int		c_flares;


extern	int		gl_filter_min, gl_filter_max;
//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

//extern cvar_t	*gl_vertex_arrays;

extern cvar_t	*gl_ext_swapinterval;
extern cvar_t	*gl_ext_palettedtexture;
extern cvar_t	*gl_ext_multitexture;
extern cvar_t	*gl_ext_pointparameters;
extern cvar_t	*gl_ext_compiled_vertex_array;

#if defined(PARTICLESYSTEM)
extern cvar_t  *gl_transrendersort;
extern cvar_t  *gl_particle_lighting;
extern cvar_t  *gl_particle_min;
extern cvar_t  *gl_particle_max;
#else
extern cvar_t	*gl_particle_min_size;
extern cvar_t	*gl_particle_max_size;
extern cvar_t	*gl_particle_size;
extern cvar_t	*gl_particle_att_a;
extern cvar_t	*gl_particle_att_b;
extern cvar_t	*gl_particle_att_c;
#endif
extern	cvar_t	*gl_nosubimage;
extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_log;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_dynamic;
extern  cvar_t  *gl_monolightmap;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_round_down;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_texsort;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_lightmaptype;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_playermip;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_3dlabs_broken;
extern  cvar_t  *gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern  cvar_t  *gl_saturatelighting;
extern  cvar_t  *gl_lockpvs;

extern  cvar_t  *hw_gl_strings;
extern  cvar_t  *gl_overbrightbits;
extern  cvar_t  *gl_ext_mtexcombine;
extern  cvar_t  *gl_ext_texture_compression;
extern  cvar_t  *gl_motionblur;
extern  cvar_t  *gl_motionblur_intensity;
extern  cvar_t  *gl_skydistance;
extern	cvar_t	*gl_water_waves;
extern  cvar_t  *gl_water_caustics;
extern  cvar_t  *gl_water_caustics_image;
extern	cvar_t	*gl_detailtextures;
extern	cvar_t	*gl_reflection;
extern	cvar_t	*gl_reflection_debug;
extern	cvar_t	*gl_reflection_max;
extern	cvar_t	*gl_reflection_shader; 
extern	cvar_t	*gl_reflection_shader_image; 
extern	cvar_t	*gl_reflection_fragment_program;
extern	cvar_t	*gl_reflection_water_surf;
extern  cvar_t  *gl_fogenable;
extern  cvar_t  *gl_fogred;
extern  cvar_t  *gl_foggreen;
extern  cvar_t  *gl_fogblue;
extern  cvar_t  *gl_fogstart;
extern  cvar_t  *gl_fogend;
extern  cvar_t  *gl_fogdensity;
extern  cvar_t  *gl_fogunderwater;
extern  cvar_t  *gl_screenshot_jpeg;
extern  cvar_t  *gl_screenshot_jpeg_quality;
extern  cvar_t  *r_decals;
extern  cvar_t  *r_decals_time;
extern  cvar_t  *gl_lightmap_saturation;
extern  cvar_t  *gl_lightmap_texture_saturation;
extern  cvar_t  *gl_anisotropy;
extern  cvar_t  *gl_shell_image;
extern  cvar_t  *gl_minimap;
extern  cvar_t  *gl_minimap_size;
extern  cvar_t  *gl_minimap_zoom;
extern  cvar_t  *gl_minimap_style;
extern  cvar_t  *gl_minimap_x;
extern  cvar_t  *gl_minimap_y; 
extern  cvar_t  *gl_lensflare;
extern  cvar_t  *gl_lensflare_intens;
extern  cvar_t  *gl_cellshade;
extern  cvar_t  *gl_cellshade_width;
extern  cvar_t  *gl_bloom;
extern  cvar_t  *gl_bloom_alpha;
extern  cvar_t  *gl_bloom_diamond_size;
extern  cvar_t  *gl_bloom_intensity;
extern  cvar_t  *gl_bloom_darken;
extern  cvar_t  *gl_bloom_sample_size;
extern  cvar_t  *gl_bloom_fast_sample;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;

extern	cvar_t  *cl_3dcam;
extern	cvar_t  *con_font;
extern	cvar_t  *con_font_size;
extern	cvar_t  *font_size;
extern	cvar_t	*alt_text_color;

extern	cvar_t	*intensity;

extern	int	gl_lightmap_format;
extern	int	gl_solid_format;
extern	int	gl_alpha_format;
extern	int	gl_tex_solid_format;
extern	int	gl_tex_alpha_format;

extern	int	c_visible_lightmaps;
extern	int	c_visible_textures;

extern	float	r_world_matrix[16];

void R_TranslatePlayerSkin (int playernum);
void GL_Bind (int texnum);
void GL_Bind_2 (int tmu, image_t *tex);
void GL_MBind( GLenum target, int texnum );
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( qboolean enable );
void GL_Enable3dTextureUnit( qboolean enable );
void GL_SelectTexture( GLenum );

void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];
#if !defined PARTICLESYSTEM
extern	float	d_8to24tablef[256][3];
#endif

extern	int		registration_sequence;

#define SURF_WATER      0x10000000
#define SURF_LAVA       0x20000000
#define SURF_SLIME      0x40000000

extern qboolean inlava;
extern qboolean inslime;
extern qboolean inwater;

void V_AddBlend (float r, float g, float b, float a, float *v_blend);


int 	R_Init( void *hinstance, void *hWnd );
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void GL_ScreenShot_JPG(void);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);
void R_DrawBeam( entity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void GL_RenderLightmappedPoly(msurface_t * surf);
void R_RenderBrushPoly (msurface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (entity_t *e);
void R_MarkLeaves (void);

glpoly_t *WaterWarpPolyVerts (glpoly_t *p);
void EmitWaterPolys (msurface_t *fa);
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

/** MPO **/
void MYgluPerspective (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
double calc_wave(GLfloat x, GLfloat y);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name, float alpha);
void	Draw_StretchPic (int x, int y, int w, int h, char *name, float alpha);
void	Draw_StretchPic2 (int x, int y, int w, int h, char *name,
                          float red, float green, float blue, float alpha);
void    Draw_ScaledPic(int x, int y, float scale, float alpha, char *pic, float red, float green, float blue, 
                       qboolean fixcoords, qboolean repscale);
void	Draw_Char (int x, int y, int num, int alpha);
void    Draw_ScaledChar (float x, float y, int num, float scale, 
	                 int red, int green, int blue, int alpha, qboolean italic);
void	Draw_TileClear (int x, int y, int w, int h, char *pic);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_Fill2 (int x, int y, int w, int h, int red, int green, int blue, int alpha);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
void	R_SetPalette ( const unsigned char *palette);

int		Draw_GetPalette (void);

void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

struct image_s *R_RegisterSkin (char *name);

void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
void LoadTGA (char *filename, byte **pic, int *width, int *height);
void LoadJPG (char *filename, byte **pic, int *width, int *height);
void LoadPNG (char *filename, byte **pic, int *width, int *height);

image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t	*GL_FindImage (char *name, imagetype_t type);
image_t	*Draw_FindPic (char *name);	//mpo need this so we can call this method in gl_refl.c
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);

void	GL_SetTexturePalette( unsigned palette[256] );

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void    GL_TextureAlphaMode( char *string );
void    GL_TextureSolidMode( char *string );

void    GL_MipMap (byte *in, int width, int height);

#if defined(PARTICLESYSTEM)
#define DEPTHHACK_RANGE_SHORT	0.999f
#define DEPTHHACK_RANGE_MID		0.99f
#define DEPTHHACK_RANGE_LONG	0.975f
void SetParticlePicture (int num, char *name);
void R_RenderBeam(vec3_t start, vec3_t end, float size, float red, float green, float blue, float alpha);
void SetVertexOverbrights (qboolean toggle);
void SetParticleOverbright(qboolean toggle);
void renderParticle (particle_t *p);
void renderDecal (particle_t *p);
float *worldVert (int i, msurface_t *surf);
void Mod_SplashFragTexCoords (vec3_t axis[3], const vec3_t origin, float radius, markFragment_t *fr);
void Mod_SetTexCoords (const vec3_t origin, vec3_t axis[3], float radius);
int R_AddDecal(const vec3_t origin,  vec3_t axis[3], float radius, int maxPoints, 
               vec3_t *points, vec2_t *tcoords, int maxFragments, markFragment_t *fragments);
#else
void GL_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );
void    GL_ClearDecals(void);
void    R_AddDecals(void);
void	R_AddDecal(vec3_t origin, vec3_t dir, float red, float green, float blue, 
                   float alpha, float endRed, float endGreen, float endBlue, 
	           float endAlpha, float size, int type, int flags, float angle);
#endif
void GL_DrawRadar(void);

int R_DrawRSpeeds(char *S);
void R_PolyBlend (void);
void R_SetupFrame (void);
void R_SetFrustum (void);
void R_SetupGL (void);
void R_BloomBlend (refdef_t *fd);
void R_RenderFlares (void);

#define MAX_ARRAY MAX_PARTICLES*4
#define VA_SetElem2(v,a,b)	((v)[0]=(a),(v)[1]=(b))
#define VA_SetElem3(v,a,b,c)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))
#define VA_SetElem4(v,a,b,c,d)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))
float	tex_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];
float	col_array[MAX_ARRAY][4];

/*
** GL extension emulation functions
*/

#define GLSTATE_DISABLE_ALPHATEST if (gl_state.alpha_test) { qglDisable(GL_ALPHA_TEST); gl_state.alpha_test=false; }
#define GLSTATE_ENABLE_ALPHATEST if (!gl_state.alpha_test) { qglEnable(GL_ALPHA_TEST); gl_state.alpha_test=true; }

#define GLSTATE_DISABLE_BLEND if (gl_state.blend) { qglDisable(GL_BLEND); gl_state.blend=false; }
#define GLSTATE_ENABLE_BLEND if (!gl_state.blend) { qglEnable(GL_BLEND); gl_state.blend=true; }

#define GLSTATE_DISABLE_TEXGEN if (gl_state.texgen) { qglDisable(GL_TEXTURE_GEN_S); qglDisable(GL_TEXTURE_GEN_T); qglDisable(GL_TEXTURE_GEN_R); gl_state.texgen=false; }
#define GLSTATE_ENABLE_TEXGEN if (!gl_state.texgen) { qglEnable(GL_TEXTURE_GEN_S); qglEnable(GL_TEXTURE_GEN_T); qglEnable(GL_TEXTURE_GEN_R); gl_state.texgen=true; }

/*
** GL config stuff
*/
#define GL_RENDERER_VOODOO		0x00000001
#define GL_RENDERER_VOODOO2   	0x00000002
#define GL_RENDERER_VOODOO_RUSH	0x00000004
#define GL_RENDERER_BANSHEE		0x00000008
#define		GL_RENDERER_3DFX		0x0000000F

#define GL_RENDERER_PCX1		0x00000010
#define GL_RENDERER_PCX2		0x00000020
#define GL_RENDERER_PMX			0x00000040
#define		GL_RENDERER_POWERVR		0x00000070

#define GL_RENDERER_PERMEDIA2	0x00000100
#define GL_RENDERER_GLINT_MX	0x00000200
#define GL_RENDERER_GLINT_TX	0x00000400
#define GL_RENDERER_3DLABS_MISC	0x00000800
#define		GL_RENDERER_3DLABS	0x00000F00

#define GL_RENDERER_REALIZM		0x00001000
#define GL_RENDERER_REALIZM2	0x00002000
#define		GL_RENDERER_INTERGRAPH	0x00003000

#define GL_RENDERER_3DPRO		0x00004000
#define GL_RENDERER_REAL3D		0x00008000
#define GL_RENDERER_RIVA128		0x00010000
#define GL_RENDERER_DYPIC		0x00020000

#define GL_RENDERER_V1000		0x00040000
#define GL_RENDERER_V2100		0x00080000
#define GL_RENDERER_V2200		0x00100000
#define		GL_RENDERER_RENDITION	0x001C0000

#define GL_RENDERER_O2          0x00100000
#define GL_RENDERER_IMPACT      0x00200000
#define GL_RENDERER_RE			0x00400000
#define GL_RENDERER_IR			0x00800000
#define		GL_RENDERER_SGI			0x00F00000

#define GL_RENDERER_MCD			0x01000000
#define GL_RENDERER_OTHER		0x80000000

typedef struct
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;

	qboolean	allow_cds;
	qboolean	mtexcombine;
} glconfig_t;

typedef struct
{
	float inverse_intensity;
	qboolean fullscreen;

	int     prev_mode;

	unsigned char *d_16to8table;

	int lightmap_textures;

	int	currenttextures[3];
	int currenttmu;

	float camera_separation;
	qboolean stereo_enabled;

	unsigned char originalRedGammaTable[256];
	unsigned char originalGreenGammaTable[256];
	unsigned char originalBlueGammaTable[256];

	qboolean	alpha_test;
	qboolean	blend;
	qboolean	texgen;
	
	qboolean	hwgamma;
	qboolean	fragment_program; // mpo does gfx support fragment programs
	qboolean	sgis_mipmap;
	qboolean	reg_combiners;
	qboolean	tex_rectangle;
	qboolean	texture_compression;
	float           max_anisotropy; // jitanisotropy
	
	int		width, height;
} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
int 		GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );
int     	GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void		GLimp_AppActivate( qboolean active );
void		GLimp_EnableLogging( qboolean enable );
void		GLimp_LogNewFrame( void );
void		UpdateHardwareGamma(void);
void		RefreshFont(void);

void		GL_Stencil(qboolean enable);
void		NormalToLatLong(const vec3_t normal, byte latlong[2]);
void		LatLongToNormal(byte latlong[2], vec3_t normal);

void GL_BlendFunction(GLenum sfactor, GLenum dfactor);
void GL_ShadeModel(GLenum mode);

/* sul */
#define MAX_RADAR_ENTS 128
typedef struct RadarEnt_s
{
	float	color[4];
	vec3_t	org;
} RadarEnt_t;

int		numRadarEnts;
RadarEnt_t	RadarEnts[MAX_RADAR_ENTS];


/* this macro is in sample size workspace coordinates */
#define R_Bloom_SamplePass( xpos, ypos ) \
	qglBegin(GL_QUADS); \
	qglTexCoord2f( 0, sampleText_tch); \
	qglVertex2f( xpos, ypos); \
	qglTexCoord2f( 0, 0); \
	qglVertex2f( xpos, ypos+sample_height); \
	qglTexCoord2f( sampleText_tcw, 0); \
	qglVertex2f( xpos+sample_width, ypos+sample_height); \
	qglTexCoord2f( sampleText_tcw, sampleText_tch); \
	qglVertex2f( xpos+sample_width, ypos); \
	qglEnd();

#define R_Bloom_Quad( x, y, width, height, textwidth, textheight ) \
	qglBegin(GL_QUADS); \
	qglTexCoord2f( 0, textheight); \
	qglVertex2f( x, y); \
	qglTexCoord2f( 0, 0); \
	qglVertex2f( x, y+height); \
	qglTexCoord2f( textwidth, 0); \
	qglVertex2f( x+width, y+height); \
	qglTexCoord2f( textwidth, textheight); \
	qglVertex2f( x+width, y); \
	qglEnd();


#endif /* _GL_LOCAL_H_ */




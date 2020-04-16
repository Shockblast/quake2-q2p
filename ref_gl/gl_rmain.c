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
// r_main.c
#include <ctype.h>
#include "gl_local.h"
#include "gl_refl.h" /** MPO **/

/***************************************************************************** */
/* nVidia extensions */
/***************************************************************************** */
PFNGLCOMBINERPARAMETERFVNVPROC qglCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERFNVPROC qglCombinerParameterfNV;
PFNGLCOMBINERPARAMETERIVNVPROC qglCombinerParameterivNV;
PFNGLCOMBINERPARAMETERINVPROC qglCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC qglCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC qglCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC qglFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC qglGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC qglGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC qglGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC qglGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC qglGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC qglGetFinalCombinerInputParameterivNV;

void R_Clear (void);

viddef_t	vid;

refimport_t	ri;

int GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2;

int maxTextureUnits;	// MH max number of texture units avaliable

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t  gl_state;

image_t		*r_notexture;		// use for bad textures
#if defined(PARTICLESYSTEM)
image_t		*r_particletexture[PARTICLE_TYPES]; 	//list for particles
image_t		*r_particlebeam;			//used for beam ents
#else
image_t		*r_particletexture;	// little dot for particles
image_t         *r_decaltexture[DECAL_MAX];
#endif
image_t         *r_caustictexture;	// Water caustic texture
image_t		*r_detailtexture;
image_t		*r_flare[MAX_FLARE];
image_t         *r_shelltexture;        // c14 added shell texture
image_t         *r_radarmap;		// wall texture for radar texgen
image_t         *r_around;
image_t         *r_bloomscreentexture;
image_t         *r_bloomeffecttexture;
image_t         *r_bloombackuptexture;
image_t         *r_bloomdownsamplingtexture;

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int		r_visframecount;	// bumped when going to a new PVS
int		r_framecount;		// used for dlight push checking

int		c_brush_polys, c_alias_polys;
int		c_flares;

float		v_blend[4];			// final blending color

void GL_Strings_f( void );

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

//cvar_t	*gl_vertex_arrays;

#if defined(PARTICLESYSTEM)
cvar_t  *gl_transrendersort;
cvar_t  *gl_particle_lighting;
cvar_t  *gl_particle_min;
cvar_t  *gl_particle_max;
#else
cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;
cvar_t	*gl_particles;
#endif
cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_palettedtexture;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_log;
cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t  *gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t  *gl_monolightmap;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

cvar_t	*gl_3dlabs_broken;

cvar_t  *hw_gl_strings;
cvar_t  *gl_overbrightbits;
cvar_t  *gl_ext_mtexcombine;
cvar_t  *gl_ext_texture_compression;
cvar_t  *gl_motionblur;
cvar_t  *gl_motionblur_intensity;
cvar_t	*gl_skydistance;
cvar_t	*gl_detailtextures;
cvar_t	*gl_water_waves;
cvar_t  *gl_water_caustics;
cvar_t  *gl_water_caustics_image;
cvar_t	*gl_reflection;
cvar_t	*gl_reflection_debug;
cvar_t	*gl_reflection_max;
cvar_t	*gl_reflection_shader;
cvar_t	*gl_reflection_shader_image;
cvar_t	*gl_reflection_fragment_program;
cvar_t	*gl_reflection_water_surf;
cvar_t  *gl_fogenable;
cvar_t  *gl_fogred;
cvar_t  *gl_foggreen;
cvar_t  *gl_fogblue;
cvar_t  *gl_fogstart;
cvar_t  *gl_fogend;
cvar_t  *gl_fogdensity;
cvar_t  *gl_fogunderwater;
cvar_t  *gl_screenshot_jpeg;
cvar_t  *gl_screenshot_jpeg_quality;
cvar_t  *r_decals;
cvar_t  *r_decals_time;
cvar_t  *gl_lightmap_saturation;
cvar_t  *gl_lightmap_texture_saturation;
cvar_t  *gl_anisotropy;
cvar_t  *gl_shell_image;
cvar_t  *gl_minimap;
cvar_t  *gl_minimap_size;
cvar_t  *gl_minimap_zoom;
cvar_t  *gl_minimap_style;
cvar_t  *gl_minimap_x;
cvar_t  *gl_minimap_y;
cvar_t  *gl_lensflare;
cvar_t  *gl_lensflare_intens;
cvar_t  *gl_cellshade;
cvar_t  *gl_cellshade_width;
cvar_t  *gl_bloom;
cvar_t  *gl_bloom_alpha;
cvar_t  *gl_bloom_diamond_size;
cvar_t  *gl_bloom_intensity;
cvar_t  *gl_bloom_darken;
cvar_t  *gl_bloom_sample_size;
cvar_t  *gl_bloom_fast_sample;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;

cvar_t	*cl_3dcam;
cvar_t  *con_font;
cvar_t  *con_font_size;
cvar_t  *font_size;
cvar_t	*alt_text_color;
cvar_t	*levelshots;

/** MPO **/
void ( APIENTRY * qglGenProgramsARB) (GLint n, GLuint *programs);
void ( APIENTRY * qglDeleteProgramsARB) (GLint n, const GLuint *programs);
void ( APIENTRY * qglBindProgramARB) (GLenum target, GLuint program);
void ( APIENTRY * qglProgramStringARB) (GLenum target, GLenum format, GLint len, const void *string); 
void ( APIENTRY * qglProgramEnvParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY * qglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);

/*
 * ================= GL_Stencil
 *
 * setting stencil buffer =================
 */
extern qboolean	have_stencil;
void
GL_Stencil(qboolean enable)
{
	if (!have_stencil)
		return;

	if (enable) {
		qglEnable(GL_STENCIL_TEST);
		qglStencilFunc(GL_EQUAL, 1, 2);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	} else {
		qglDisable(GL_STENCIL_TEST);
	}
}

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;
	cplane_t 	*p;

	if (r_nocull->value)
		return false;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		switch (p->signbits)
		{
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		default:
			return false;
		}
	}
	return false;
}

void R_RotateForEntity (entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[1],  0, 0, 1);
    qglRotatef (-e->angles[0],  0, 1, 0);
    qglRotatef (-e->angles[2],  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0F )
		qglEnable( GL_BLEND );

	qglColor4f( 1, 1, 1, alpha );

	GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv( GL_MODULATE );
#if 0
	if ((currententity->flags&RF_TRANS_ADDITIVE) && (alpha != 1.0F))
	{ 	
		qglEnable (GL_BLEND);
		GL_TexEnv( GL_MODULATE );

		qglDisable( GL_ALPHA_TEST );
		GL_BlendFunction (GL_SRC_ALPHA, GL_ONE);

		qglColor4ub(255, 255, 255, alpha*254);		
	}
	else
	{
		if ( alpha != 1.0F )
			qglEnable (GL_BLEND);

		GL_TexEnv( GL_MODULATE );
		
		if ( alpha == 1.0 )
			qglEnable (GL_ALPHA_TEST);
		else
			qglDisable( GL_ALPHA_TEST );
		
		qglColor4f( 1, 1, 1, alpha );
	}
#else
	if ( alpha == 1.0 )
		qglEnable (GL_ALPHA_TEST);
	else
		qglDisable( GL_ALPHA_TEST );
#endif
	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (0, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	qglVertex3fv (point);
	
	qglEnd ();

	qglDisable (GL_ALPHA_TEST);
	GL_TexEnv( GL_REPLACE );

	if ( alpha != 1.0F )
		qglDisable( GL_BLEND );

	qglColor4f( 1, 1, 1, 1 );
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight, true);

	qglPushMatrix ();
	R_RotateForEntity (currententity);

	qglDisable (GL_TEXTURE_2D);
	qglColor3fv (shadelight);

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	qglEnable (GL_TEXTURE_2D);
}

GLenum bFunc1 = -1,
       bFunc2 = -1,
       shadeModelMode = -1;
       
void GL_BlendFunction(GLenum sfactor, GLenum dfactor)
{
	if (sfactor!=bFunc1 || dfactor!=bFunc2) {
		bFunc1 = sfactor;
		bFunc2 = dfactor;

		qglBlendFunc(bFunc1, bFunc2);
	}
}

void GL_ShadeModel(GLenum mode)
{
	if (mode!=shadeModelMode) {
		shadeModelMode = mode;
		qglShadeModel(mode);
	}
}

void R_Fog(void)
{
	vec3_t colors;
	
	if (!gl_fogenable->value)
		return;
		
	qglDisable(GL_FOG);
	
	if (gl_fogenable->value && !(r_newrefdef.rdflags & RDF_UNDERWATER)) {
		qglEnable(GL_FOG);
		qglFogi(GL_FOG_MODE, GL_BLEND);
		
		colors[0] = gl_fogred->value;
		colors[1] = gl_foggreen->value;
		colors[2] = gl_fogblue->value;
		
		qglFogf(GL_FOG_DENSITY, gl_fogdensity->value);
		qglFogfv(GL_FOG_COLOR, colors);
		qglFogf(GL_FOG_START, gl_fogstart->value);
		qglFogf(GL_FOG_END, gl_fogend->value);
	} else if (gl_fogunderwater->value && (r_newrefdef.rdflags & RDF_UNDERWATER)) {
		qglEnable(GL_FOG);
		qglFogi(GL_FOG_MODE, GL_BLEND);
		
		colors[0] = gl_fogred->value;
		colors[1] = gl_foggreen->value;
		colors[2] = gl_fogblue->value;
        	
		qglFogf(GL_FOG_START, 0.0);
	    	qglFogf(GL_FOG_END, 2048);
		qglFogf(GL_FOG_DENSITY, 0.0);
	
		if (inlava)  colors[0] = 0.7;
		if (inslime) colors[1] = 0.7;
		if (inwater) colors[2] = 0.6;

		qglFogf(GL_FOG_DENSITY, 0.001);
		qglFogf(GL_FOG_START, 0.0);
		qglFogfv(GL_FOG_COLOR, colors);
		qglFogf(GL_FOG_END, 450);
	} else
		qglDisable(GL_FOG);
}


/*
=============
R_DrawEntitiesOnList
=============
*/
/*
** GL_DrawParticles
**
*/
#if defined(PARTICLESYSTEM)
typedef struct sortedelement_s 
{
        void *data,
             *left,
             *right,
             *next;
        vec_t len;
        vec3_t org;
} sortedelement_t;
sortedelement_t theents[MAX_ENTITIES],
                *ents_viewweaps,
                *ents_viewweaps_trans,
                *ents_prerender,
                *ents_last,
                *listswap[2],
                theparts[MAX_PARTICLES],
                *parts_prerender,
                *parts_decals,
                *parts_last;

typedef struct sortedpart_s
{
        particle_t *p;
        vec_t len;
} sortedpart_t;
sortedpart_t theoldparts[MAX_PARTICLES];

particle_t *currentparticle;

vec3_t ParticleVec[4],
       particle_coord[4],
       shadelight;

int entstosort,
    partstosort;
       
qboolean ParticleOverbright;
       
void vectoanglerolled(vec3_t value1, float angleyaw, vec3_t angles)
{
	float	forward, yaw, pitch;

	yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
	forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
	pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);

	if (pitch < 0)
		pitch += 360;

	angles[PITCH] = -pitch;
	angles[YAW] =  yaw;
	angles[ROLL] = - angleyaw;
}

float AngleFind(float input)
{
	return 180.0/input;
}
       
void SetVertexOverbrights(qboolean toggle)
{
	if (!gl_overbrightbits->value || !gl_config.mtexcombine)
		return;

	if (toggle){ //turn on
	
		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, gl_overbrightbits->value);
		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		
		GL_TexEnv( GL_COMBINE_EXT );
	}
	else { //turn off
		GL_TexEnv( GL_MODULATE );
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
	}
}

void resetSortList(void)
{
	entstosort = 0;
	ents_last = NULL;
	ents_prerender = NULL;
	ents_viewweaps = NULL;
	ents_viewweaps_trans = NULL;
}

sortedelement_t *NewSortEnt(entity_t *ent)
{
	vec3_t distance;
	sortedelement_t *element;

	element = &theents[entstosort];

	VectorSubtract(ent->origin, r_origin, distance);
	VectorCopy(ent->origin, element->org);

	element->data = (entity_t *)ent;
	element->len = (vec_t)VectorLength(distance);
	element->left = NULL;
	element->right = NULL;
	element->next = NULL;

	return element;
}

void ElementAddNode(sortedelement_t *base, sortedelement_t *thisElement)
{
	if (thisElement->len > base->len) {
		if (base->left)
			ElementAddNode(base->left, thisElement);
		else
			base->left = thisElement;
	}
	else {
		if (base->right)
			ElementAddNode(base->right, thisElement);
		else
			base->right = thisElement;
	}
}

void AddEntViewWeapTree(entity_t *ent, qboolean trans)
{
	sortedelement_t *thisEnt;


	thisEnt = NewSortEnt(ent);

	if (!thisEnt)
		return;

	if (!trans) {
		if (ents_viewweaps)
			ElementAddNode(ents_viewweaps, thisEnt);
		else
			ents_viewweaps = thisEnt;
	}
	else {		
		if (ents_viewweaps_trans)
			ElementAddNode(ents_viewweaps_trans, thisEnt);
		else
			ents_viewweaps_trans = thisEnt;	
	}

	entstosort++;
}

void AddEntTransTree(entity_t *ent)
{
	sortedelement_t *thisEnt;


	thisEnt = NewSortEnt(ent);

	if (!thisEnt)
		return;

	if (ents_prerender)
		ElementAddNode(ents_prerender, thisEnt);
	else
		ents_prerender = thisEnt;

	ents_last = thisEnt;

	entstosort++;
}

void ParseRenderEntity(entity_t *ent)
{
	currententity = ent;

	if ( currententity->flags & RF_BEAM ) 
		R_DrawBeam( currententity );
	else {
		currentmodel = currententity->model;
		if (!currentmodel) {
			R_DrawNullModel ();
			return;
		}

		switch (currentmodel->type)
		{
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;
		case mod_brush:
			R_DrawBrushModel (currententity);
			break;
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;
		default:
			ri.Sys_Error (ERR_DROP, "Bad modeltype");
			break;
		}
	}
}

qboolean transBrushModel(entity_t *ent)
{
	int i;
	msurface_t *surf;

	if (ent && ent->model && ent->model->type==mod_brush) {
		surf = &ent->model->surfaces[ent->model->firstmodelsurface];
		for (i=0 ; i<ent->model->nummodelsurfaces ; i++, surf++)
			if (surf && surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
				return true;
	}

	return false;
}

void RenderEntTree(sortedelement_t *element)
{
	if (!element)
		return;

	RenderEntTree(element->left);

	if (element->data)
		ParseRenderEntity(element->data);


	RenderEntTree(element->right);
}

void R_DrawAllEntities(qboolean addViewWeaps)
{
	qboolean alpha;
	int i;
	
	if (!r_drawentities->value)
		return;

	resetSortList();

	for (i=0;i<r_newrefdef.num_entities; i++) {
		currententity = &r_newrefdef.entities[i];

		alpha = false;
		if (currententity->flags&RF_TRANSLUCENT)
			alpha = true;

		if (alpha)
			continue;

		if (currententity->flags & RF_WEAPONMODEL)
			if (!addViewWeaps)
				continue;

		ParseRenderEntity(currententity);
	}

	qglDepthMask (0);
	for (i=0;i<r_newrefdef.num_entities; i++) {
		currententity = &r_newrefdef.entities[i];

		alpha = false;
		if (currententity->flags&RF_TRANSLUCENT)
			alpha = true;

		if (currententity->flags & RF_WEAPONMODEL)
			if (!addViewWeaps)
				continue;

		if (!alpha)
			continue;

		ParseRenderEntity(currententity);
	}
	qglDepthMask (1);
}

void R_DrawSolidEntities(void)
{
	qboolean alpha;
	int		i;

	if (!r_drawentities->value)
		return;

	resetSortList();

	for (i=0;i<r_newrefdef.num_entities; i++) {
		currententity = &r_newrefdef.entities[i];
		alpha = false;
		
		if (currententity->flags&RF_TRANSLUCENT)
			alpha = true;

		if (currententity->flags & RF_WEAPONMODEL) {
			AddEntViewWeapTree(currententity, alpha);
			continue;
		}

		if (alpha) {
			AddEntTransTree(currententity);
			continue;
		}

		ParseRenderEntity(currententity);
	}
}

void R_DrawEntitiesOnList(void *list)
{
	if (!r_drawentities->value)
		return;

	RenderEntTree(list);
}

sortedelement_t *NewSortPart(particle_t *p)
{
	vec3_t distance;
	sortedelement_t *element;

	element = &theparts[partstosort];

	VectorSubtract(p->origin, r_origin, distance);
	VectorCopy(p->origin, element->org);

	element->data	= p;
	element->len	= VectorLength(distance);
	element->left	= NULL;
	element->right	= NULL;

	return element;
}

void resetPartSortList(void)
{
	partstosort = 0;
	parts_prerender = NULL;
	parts_decals = NULL;
	parts_last = NULL;
}

qboolean particleClip(float len)
{
	if (gl_particle_min->value>0) {
		if (len < gl_particle_min->value)
			return true;
	}
	if (gl_particle_max->value>0) {
		if (len > gl_particle_max->value)
			return true;
	}

	return false;
}

void DecalElementAddNode(sortedelement_t *base, sortedelement_t *thisElement)
{
	particle_t *pBase = thisElement->data,
	           *pThis = base->data;

	if (pBase->flags & PART_DECAL_SUB) {
		if (pThis->flags & PART_DECAL_SUB) {
			if (base->right)
				ElementAddNode(base->right, thisElement);
			else
				base->right = thisElement;
		}
		else {
			if (base->left)
				ElementAddNode(base->left, thisElement);
			else
				base->left = thisElement;
		}
		return;
	}
	else if (pThis->flags & PART_DECAL_ADD) {
		if (pThis->flags & PART_DECAL_SUB) {
			if (base->left)
				ElementAddNode(base->left, thisElement);
			else
				base->left = thisElement;
		}
		else {
			if (base->right)
				ElementAddNode(base->right, thisElement);
			else
				base->right = thisElement;
		}
		return;
	}

	if (thisElement->len > base->len) {
		if (base->left)
			ElementAddNode(base->left, thisElement);
		else
			base->left = thisElement;
	}
	else {
		if (base->right)
			ElementAddNode(base->right, thisElement);
		else
			base->right = thisElement;
	}
}

void AddPartTransTree(particle_t *p)
{
	sortedelement_t *thisPart;

	thisPart = NewSortPart(p);

	//decals are sorted by render type, then depth
	if (p->flags&PART_DECAL) {
		if (parts_decals)
			DecalElementAddNode(parts_decals, thisPart);		
		else
			parts_decals = thisPart;	
	}
	else {
		if (particleClip(thisPart->len))
			return;

		if (parts_prerender)
			ElementAddNode(parts_prerender, thisPart);	
		else
			parts_prerender = thisPart;

		parts_last = thisPart;
	}

	partstosort++;
}

void GL_BuildParticleList(void)
{
	int		i;
	resetPartSortList();

	for ( i=0 ; i < r_newrefdef.num_particles ; i++) {
		currentparticle = &r_newrefdef.particles[i];
		AddPartTransTree(currentparticle);
	}
}

void RenderParticleTree(sortedelement_t *element)
{
	if (!element)
		return;

	RenderParticleTree(element->left);

	if (element->data)
		renderParticle((particle_t *)element->data);

	RenderParticleTree(element->right);
}

void RenderDecalTree(sortedelement_t *element)
{
	if (!element)
		return;

	RenderDecalTree(element->left);

	if (element->data)
		renderDecal((particle_t *)element->data);

	RenderDecalTree(element->right);
}

void R_DrawDecals(void)
{
	vec3_t	up	= {vup[0]    * 0.75f, vup[1]    * 0.75f, vup[2]    * 0.75f};
	vec3_t	right	= {vright[0] * 0.75f, vright[1] * 0.75f, vright[2] * 0.75f};

	VectorAdd      (up, right, particle_coord[0]);
	VectorSubtract (right, up, particle_coord[1]);
	VectorNegate   (particle_coord[0], particle_coord[2]);
	VectorNegate   (particle_coord[1], particle_coord[3]);
	
	qglDisable(GL_FOG);
	
	qglEnable (GL_TEXTURE_2D);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask   (false);
	qglEnable(GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);
	qglDisable(GL_ALPHA_TEST);

	RenderDecalTree(parts_decals);

	qglDepthRange (gldepthmin, gldepthmax);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask (true);
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
	
	//qglEnable(GL_FOG);
}

void R_DrawAllSubDecals(void)
{	
	vec3_t	up	= {vup[0]    * 0.75f, vup[1]    * 0.75f, vup[2]    * 0.75f};
	vec3_t	right	= {vright[0] * 0.75f, vright[1] * 0.75f, vright[2] * 0.75f};
	int	i;

	VectorAdd      (up, right, particle_coord[0]);
	VectorSubtract (right, up, particle_coord[1]);
	VectorNegate   (particle_coord[0], particle_coord[2]);
	VectorNegate   (particle_coord[1], particle_coord[3]);
	
	qglEnable (GL_TEXTURE_2D);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask   (false);
	qglEnable(GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);	
	qglDisable(GL_ALPHA_TEST);

	for ( i=0 ; i < r_newrefdef.num_particles ; i++)
		if (r_newrefdef.particles[i].flags&PART_DECAL && r_newrefdef.particles[i].flags&PART_DECAL_SUB)
			renderDecal(&r_newrefdef.particles[i]);

	qglDepthRange (gldepthmin, gldepthmax);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask (true);
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
}

void R_DrawAllDecals(void)
{	
	vec3_t	up	= {vup[0]    * 0.75f, vup[1]    * 0.75f, vup[2]    * 0.75f};
	vec3_t	right	= {vright[0] * 0.75f, vright[1] * 0.75f, vright[2] * 0.75f};
	int	i;

	VectorAdd      (up, right, particle_coord[0]);
	VectorSubtract (right, up, particle_coord[1]);
	VectorNegate   (particle_coord[0], particle_coord[2]);
	VectorNegate   (particle_coord[1], particle_coord[3]);
	
	qglEnable (GL_TEXTURE_2D);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask   (false);
	qglEnable(GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);	
	qglDisable(GL_ALPHA_TEST);

	for ( i=0 ; i < r_newrefdef.num_particles ; i++)
		if (r_newrefdef.particles[i].flags&PART_DECAL)
			renderDecal(&r_newrefdef.particles[i]);

	qglDepthRange (gldepthmin, gldepthmax);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask (true);
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
}

void SetParticleOverbright(qboolean toggle)
{
	if ( (toggle && !ParticleOverbright) || (!toggle && ParticleOverbright) ) {
		SetVertexOverbrights(toggle);
		ParticleOverbright = toggle;
	}
}

void R_DrawParticles(void *list)
{	
	vec3_t	up	= {vup[0]    * 0.75f, vup[1]    * 0.75f, vup[2]    * 0.75f};
	vec3_t	right	= {vright[0] * 0.75f, vright[1] * 0.75f, vright[2] * 0.75f};

	VectorAdd      (up, right, particle_coord[0]);
	VectorSubtract (right, up, particle_coord[1]);
	VectorNegate   (particle_coord[0], particle_coord[2]);
	VectorNegate   (particle_coord[1], particle_coord[3]);
	
	qglDisable(GL_FOG);
	
	qglEnable (GL_TEXTURE_2D);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask   (false);
	qglEnable(GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);
	qglDisable(GL_ALPHA_TEST);
	ParticleOverbright = false;

	RenderParticleTree(list);

	qglDepthRange (gldepthmin, gldepthmax);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask (true);
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
	
	//qglEnable(GL_FOG);
}

void R_DrawAllParticles(void)
{	
	vec3_t	up	= {vup[0]    * 0.75f, vup[1]    * 0.75f, vup[2]    * 0.75f};
	vec3_t	right	= {vright[0] * 0.75f, vright[1] * 0.75f, vright[2] * 0.75f};
	int		i;

	VectorAdd      (up, right, particle_coord[0]);
	VectorSubtract (right, up, particle_coord[1]);
	VectorNegate   (particle_coord[0], particle_coord[2]);
	VectorNegate   (particle_coord[1], particle_coord[3]);
	
	qglEnable (GL_TEXTURE_2D);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask   (false);
	qglEnable(GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);
	qglDisable(GL_ALPHA_TEST);
	ParticleOverbright = false;

	for ( i=0 ; i < r_newrefdef.num_particles ; i++)
		if (!(r_newrefdef.particles[i].flags&PART_DECAL))
			renderParticle(&r_newrefdef.particles[i]);

	qglDepthRange (gldepthmin, gldepthmax);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask (true);
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
}

int texParticle(int type)
{
	image_t		*part_img;

	part_img = r_particletexture[type];

	return part_img->texnum;
}

void setBeamAngles(vec3_t start, vec3_t end, vec3_t up, vec3_t right)
{
	vec3_t move, delta;

	VectorSubtract(end, start, move);
	VectorNormalize(move);

	VectorCopy(move, up);
	VectorSubtract(r_newrefdef.vieworg, start, delta);
	CrossProduct(up, delta, right);
	VectorNormalize(right);
}

void getParticleLight(particle_t *p, vec3_t pos, float lighting, vec3_t shadelight)
{
	int j;
	float lightest = 0;

	if (!lighting) {
		VectorSet(shadelight, p->red, p->green, p->blue);
		return;
	}

	R_LightPoint (pos, shadelight, true);

	shadelight[0]= (lighting*shadelight[0]+(1-lighting)) * p->red;
	shadelight[1]= (lighting*shadelight[1]+(1-lighting)) * p->green;
	shadelight[2]= (lighting*shadelight[2]+(1-lighting)) * p->blue;

	//this cleans up the lighting
	{
		for (j=0;j<3;j++)
			if (shadelight[j]>lightest)
				lightest= shadelight[j];
		if (lightest>255)
			for (j=0;j<3;j++) {
				shadelight[j]*= 255/lightest;
				if (shadelight[j]>255)
					shadelight[j] = 255;
			}

		for (j=0;j<3;j++) {
			if (shadelight[j]<0)
				shadelight[j] = 0;
		}	
	}
}

void renderParticleShader(particle_t *p, vec3_t origin, float size, qboolean translate)
{
	qglPushMatrix();
	
	{
		if (translate) {
			qglTranslatef( origin[0], origin[1], origin[2] );
			qglScalef(size, size, size );
		}
		if (p->decal) {
			
			qglEnable(GL_POLYGON_OFFSET_FILL); 
			qglPolygonOffset(-2, -1); 
			qglBegin (GL_TRIANGLE_FAN);
			
			{
				int i;
				for (i = 0; i < p->decal->numpolys; i++) { 
					qglTexCoord2f (p->decal->coords[i][0], p->decal->coords[i][1]);
					qglVertex3fv  (p->decal->polys[i]);
				}
			}
			
			qglEnd ();
			qglDisable(GL_POLYGON_OFFSET_FILL);
		}
		else {
			qglBegin (GL_QUADS);
			
			{
				qglTexCoord2f (0, 1);
				qglVertex3fv  (ParticleVec[0]);
				qglTexCoord2f (0, 0);
				qglVertex3fv  (ParticleVec[1]);
				qglTexCoord2f (1, 0); 
				qglVertex3fv  (ParticleVec[2]);
				qglTexCoord2f (1, 1);
				qglVertex3fv  (ParticleVec[3]);
			}
			
			qglEnd ();
		}
		
	}
	
	qglPopMatrix ();
}

void renderDecal(particle_t *p)
{
	float size, alpha;
	vec3_t angl_coord[4];
	vec3_t ang_up, ang_right, ang_forward, color;

	size = (p->size>0.1) ? p->size : 0.1;
	alpha = p->alpha;


	if (p->flags&PART_SHADED) {
		getParticleLight (p, p->origin, gl_particle_lighting->value, shadelight);
		VectorSet(color, shadelight[0]/255.0, shadelight[1]/255.0, shadelight[2]/255.0);
	}
	else {
		VectorSet(shadelight, p->red, p->green, p->blue);
		VectorSet(color, p->red/255.0, p->green/255.0, p->blue/255.0);
	}

	{
		GL_BlendFunction (p->blendfunc_src, p->blendfunc_dst);
		GL_Bind(texParticle(p->image));

		if (p->flags&PART_ALPHACOLOR)
			qglColor4f(color[0]*alpha,color[1]*alpha,color[2]*alpha, alpha);
		else
			qglColor4f(color[0],color[1],color[2], alpha);
	}

	if (!p->decal) {
		AngleVectors(p->angle, ang_forward, ang_right, ang_up); 

		VectorScale (ang_right, 0.75f, ang_right);
		VectorScale (ang_up, 0.75f, ang_up);

		VectorAdd      (ang_up, ang_right, angl_coord[0]);
		VectorSubtract (ang_right, ang_up, angl_coord[1]);
		VectorNegate   (angl_coord[0], angl_coord[2]);
		VectorNegate   (angl_coord[1], angl_coord[3]);
		
		VectorMA(p->origin, size, angl_coord[0], ParticleVec[0]);
		VectorMA(p->origin, size, angl_coord[1], ParticleVec[1]);
		VectorMA(p->origin, size, angl_coord[2], ParticleVec[2]);
		VectorMA(p->origin, size, angl_coord[3], ParticleVec[3]);
	}
	renderParticleShader(p, NULL, 0, false);

}

void renderParticle(particle_t *p)
{
	float		size, lighting = gl_particle_lighting->value;
	vec3_t		shadelight;
	float		alpha;
	vec3_t		coord[4], color;

	VectorCopy(particle_coord[0], coord[0]);
	VectorCopy(particle_coord[1], coord[1]);
	VectorCopy(particle_coord[2], coord[2]);
	VectorCopy(particle_coord[3], coord[3]);

	size = (p->size>0.1) ? p->size : 0.1;
	alpha = p->alpha;

	if (p->flags&PART_DEPTHHACK_SHORT) //nice little poly-peeking - psychospaz
		qglDepthRange (gldepthmin, gldepthmin + DEPTHHACK_RANGE_SHORT*(gldepthmax-gldepthmin));
	if (p->flags&PART_DEPTHHACK_MID)
		qglDepthRange (gldepthmin, gldepthmin + DEPTHHACK_RANGE_MID*(gldepthmax-gldepthmin));
	if (p->flags&PART_DEPTHHACK_LONG)
		qglDepthRange (gldepthmin, gldepthmin + DEPTHHACK_RANGE_LONG*(gldepthmax-gldepthmin));

	if (p->flags&PART_OVERBRIGHT)
		SetParticleOverbright(true);

	if (p->flags&PART_SHADED) {
		getParticleLight (p, p->origin, lighting, shadelight);
		VectorSet(color, shadelight[0]/255.0, shadelight[1]/255.0, shadelight[2]/255.0);
	}
	else {
		VectorSet(shadelight, p->red, p->green, p->blue);
		VectorSet(color, p->red/255.0, p->green/255.0, p->blue/255.0);
	}

	{
		GL_BlendFunction (p->blendfunc_src, p->blendfunc_dst);

		GL_Bind(texParticle(p->image));

		if (p->flags&PART_ALPHACOLOR)
			qglColor4f(color[0]*alpha,color[1]*alpha,color[2]*alpha, alpha);
		else
			qglColor4f(color[0],color[1],color[2], alpha);
	}
	
	if (p->flags&PART_BEAM) { //given start and end positions, will have fun here :)
		
		//p->angle = start
		//p->origin= end
		vec3_t	ang_up, ang_right;

		setBeamAngles(p->angle, p->origin, ang_up, ang_right);
		VectorScale(ang_right, size, ang_right);

		VectorAdd(p->origin, ang_right, ParticleVec[0]);
		VectorAdd(p->angle, ang_right, ParticleVec[1]);
		VectorSubtract(p->angle, ang_right, ParticleVec[2]);
		VectorSubtract(p->origin, ang_right, ParticleVec[3]);
		renderParticleShader(p, NULL, 0, false);
	}
	else if (p->flags&PART_LIGHTNING) { //given start and end positions, will have fun here :)
		
		//p->angle = start
		//p->origin= end
		int k = 0;
		float	len, dec, angdot;
		vec3_t	move, lastvec, thisvec, tempvec;
		vec3_t	old_coord[2];
		vec3_t	ang_up, ang_right, last_right, abs_up, abs_right;
		float	width, scale_up, scale_right;

		scale_up = scale_right = 0;
		dec = size*5.0;
		width = size;

		VectorSubtract(p->origin, p->angle, move);
		len = VectorNormalize(move);
	
		setBeamAngles(p->angle, p->origin, abs_up, abs_right);
		VectorScale(move, dec, move);
		VectorCopy(p->angle, thisvec);
		VectorSubtract(thisvec, move, lastvec);
		VectorCopy(thisvec, tempvec);
		len+=dec;

		setBeamAngles(lastvec, thisvec, ang_up, ang_right);
		VectorScale (ang_right, width, ang_right);

		while (len>dec) {
			VectorCopy(ang_right, last_right);

			setBeamAngles(lastvec, thisvec, ang_up, ang_right);
			VectorScale (ang_right, width, ang_right);

			angdot = DotProduct(ang_right, last_right);
			{
				VectorAdd(lastvec, ang_right, ParticleVec[1]);
				VectorSubtract(lastvec, ang_right, ParticleVec[2]);

				VectorAdd(thisvec, ang_right, ParticleVec[0]);
				VectorSubtract(thisvec, ang_right, ParticleVec[3]);

				VectorCopy(ParticleVec[0], old_coord[0]);
				VectorCopy(ParticleVec[3], old_coord[1]);
			}
			if (k>0)
				renderParticleShader(p, NULL, 0, false);
				
			k++;

			len-=dec;

			VectorCopy(thisvec, lastvec);

			//now modify stuff
			{
				float diff = lastvec[0] + lastvec[1] + lastvec[2];
				//scale_up += crandom() * size;
				//scale_right -= crandom() * size;
				
				scale_up = size*( cos(diff*0.1+(Sys_Milliseconds() * 0.001f)*15.0) );
				scale_right = size*( sin( diff*0.1+(Sys_Milliseconds() * 0.001f)*15.0) );
			}
			if (scale_right > size) scale_right = size;
			if (scale_right < -size) scale_right = -size;
			if (scale_up > size) scale_up = size;
			if (scale_up < -size) scale_up = -size;


			VectorAdd(tempvec, move, tempvec);
			thisvec[0] = tempvec[0] + abs_up[0]*scale_up + abs_right[0]*scale_right;
			thisvec[1] = tempvec[1] + abs_up[1]*scale_up + abs_right[1]*scale_right;
			thisvec[2] = tempvec[2] + abs_up[2]*scale_up + abs_right[2]*scale_right;
		}

		//one more time
		if (len>0) {
			VectorCopy(p->origin, thisvec);
			VectorCopy(ang_right, last_right);

			setBeamAngles(lastvec, thisvec, ang_up, ang_right);
			VectorScale (ang_right, width, ang_right);

			angdot = DotProduct(ang_right, last_right);
			{
				VectorAdd(lastvec, ang_right, ParticleVec[1]);
				VectorSubtract(lastvec, ang_right, ParticleVec[2]);

				VectorAdd(thisvec, ang_right, ParticleVec[0]);
				VectorSubtract(thisvec, ang_right, ParticleVec[3]);

				VectorCopy(ParticleVec[0], old_coord[0]);
				VectorCopy(ParticleVec[3], old_coord[1]);
			}
			renderParticleShader(p, NULL, 0, false);
		}
	}
	else if (p->flags&PART_DIRECTION) { //streched out in direction...tracers etc...
		//never dissapears because of angle like other engines :D
		vec3_t ang_up, ang_right, vdelta;

		VectorAdd(p->angle, p->origin, vdelta); 
		setBeamAngles(vdelta, p->origin, ang_up, ang_right);

		VectorScale (ang_right, 0.75f, ang_right);
		VectorScale (ang_up, 0.75f * VectorLength(p->angle), ang_up);

		VectorAdd      (ang_up, ang_right, ParticleVec[0]);
		VectorSubtract (ang_right, ang_up, ParticleVec[1]);
		VectorNegate   (ParticleVec[0], ParticleVec[2]);
		VectorNegate   (ParticleVec[1], ParticleVec[3]);
		renderParticleShader(p, p->origin, size, true);

	}
	else if (p->flags&PART_ANGLED) { //facing direction... (decal wannabes)

		vec3_t ang_up, ang_right, ang_forward;

		AngleVectors(p->angle, ang_forward, ang_right, ang_up); 

		VectorScale (ang_right, 0.75f, ang_right);
		VectorScale (ang_up, 0.75f, ang_up);

		VectorAdd      (ang_up, ang_right, ParticleVec[0]);
		VectorSubtract (ang_right, ang_up, ParticleVec[1]);
		VectorNegate   (ParticleVec[0], ParticleVec[2]);
		VectorNegate   (ParticleVec[1], ParticleVec[3]);

		qglDisable (GL_CULL_FACE);
		renderParticleShader(p, p->origin, size, true);

		qglEnable (GL_CULL_FACE);

	}
	else  { //normal sprites

		if (p->angle[2])  { //if we have roll, we do calcs
	
			vec3_t angl_coord[4];
			vec3_t ang_up, ang_right, ang_forward;

			VectorSubtract(p->origin, r_newrefdef.vieworg, angl_coord[0]);

			vectoanglerolled(angl_coord[0], p->angle[2], angl_coord[1]);
			AngleVectors(angl_coord[1], ang_forward, ang_right, ang_up); 

			VectorScale (ang_forward, 0.75f, ang_forward);
			VectorScale (ang_right, 0.75f, ang_right);
			VectorScale (ang_up, 0.75f, ang_up);

			VectorAdd      (ang_up, ang_right, ParticleVec[0]);
			VectorSubtract (ang_right, ang_up, ParticleVec[1]);
			VectorNegate   (ParticleVec[0], ParticleVec[2]);
			VectorNegate   (ParticleVec[1], ParticleVec[3]);
		}
		else {
			VectorCopy(coord[0], ParticleVec[0]);
			VectorCopy(coord[1], ParticleVec[1]);
			VectorCopy(coord[2], ParticleVec[2]);
			VectorCopy(coord[3], ParticleVec[3]);
		}
		renderParticleShader(p, p->origin, size, true);
	}

	if (p->flags&PART_OVERBRIGHT)
		SetParticleOverbright(false);

	if (p->flags&PART_DEPTHHACK)
		qglDepthRange (gldepthmin, gldepthmax);
}

void R_DrawLastElements(void)
{
	if (parts_prerender)
		R_DrawParticles(parts_prerender);
	if (ents_prerender)
		RenderEntTree(ents_prerender);
}

int transCompare(const void *arg1, const void *arg2 ) 
{
	const sortedpart_t *a1, *a2;
	a1 = (sortedpart_t *) arg1;
	a2 = (sortedpart_t *) arg2;
	return (a2->len - a1->len);
}

sortedpart_t NewSortedPart(particle_t *p)
{
	vec3_t result;
	sortedpart_t s_part;

	s_part.p = p;
	VectorSubtract(p->origin, r_origin, result);
	s_part.len = (result[0] * result[0]) + (result[1] * result[1]) + (result[2] * result[2]);

	return s_part;
}

void R_SortParticlesOnList(void)
{
	int i;

	for (i = 0; i < r_newrefdef.num_particles; i++)
		theoldparts[i] = NewSortedPart(&r_newrefdef.particles[i]);
	qsort((void *) theoldparts, r_newrefdef.num_particles, sizeof(sortedpart_t), transCompare);
}

void R_RenderView(refdef_t *fd)
{
	
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value) {
		c_brush_polys = 0;
		c_alias_polys = 0;
		c_flares = 0;
	}

	R_PushDlights ();

	if (gl_finish->value)
		qglFinish ();
		
	R_SetupGL ();

	R_SetupFrame ();

	R_SetFrustum ();

	setupClippingPlanes();

	R_MarkLeaves ();	// done here so we know if we're in water
	
//	drawPlayerReflection();

	R_DrawWorld ();
	
	R_RenderFlares ();
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) { //options menu
		R_DrawAllDecals();
		R_DrawAllEntities(false);
		R_DrawAllParticles();
	}
	else {

		qglDisable(GL_ALPHA_TEST);

		R_RenderDlights ();

		//spaz -> my nice trees
		if (gl_transrendersort->value) {
		
			GL_BuildParticleList();
			R_DrawSolidEntities();
			R_DrawDecals ();

			if (gl_transrendersort->value == 1) {
				R_DrawLastElements();
				R_DrawAlphaSurfaces();
			}
			else {
				R_DrawAlphaSurfaces();
				R_DrawLastElements();
			}
		}
		//nonsorted routines
		else {
			R_DrawAllDecals();
			R_DrawAllEntities(true);
			R_DrawAllParticles();
			R_DrawAlphaSurfaces();
		}

		//always draw vwep last...
		R_DrawEntitiesOnList(ents_viewweaps);
		R_DrawEntitiesOnList(ents_viewweaps_trans);
		
		// if we aren't doing a reflection then we can do the flash and r speeds
		// (we don't want them showing up in our reflection)
		if (g_drawing_refl)
			qglDisable(GL_CLIP_PLANE0);
		else
			R_PolyBlend ();
			
		R_BloomBlend(fd);
		R_Fog();
		
		if (gl_minimap_size->value > 32 && !(r_newrefdef.rdflags & RDF_IRGOGGLES)) {
			qglDisable(GL_ALPHA_TEST);
			GL_DrawRadar();
			numRadarEnts = 0;
		}
		
		qglEnable(GL_ALPHA_TEST);
	}
}
#else
void R_DrawEntitiesOnList(void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	qglDepthMask (0);		// no z writes
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	qglDepthMask (1);		// back to writing

}

void GL_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

    GL_Bind(r_particletexture->texnum);
	qglDepthMask( GL_FALSE );		// no z buffering
	qglEnable( GL_BLEND );
	GL_TexEnv( GL_MODULATE );
	qglBegin( GL_TRIANGLES );

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] + 
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		*(int *)color = colortable[p->color];
		color[3] = p->alpha*255;

		qglColor4ubv( color );

		qglTexCoord2f( 0.0625, 0.0625 );
		qglVertex3fv( p->origin );

		qglTexCoord2f( 1.0625, 0.0625 );
		qglVertex3f( p->origin[0] + up[0]*scale, 
			         p->origin[1] + up[1]*scale, 
					 p->origin[2] + up[2]*scale);

		qglTexCoord2f( 0.0625, 1.0625 );
		qglVertex3f( p->origin[0] + right[0]*scale, 
			         p->origin[1] + right[1]*scale, 
					 p->origin[2] + right[2]*scale);
	}

	qglEnd ();
	qglDisable( GL_BLEND );
	qglColor4f( 1,1,1,1 );
	qglDepthMask( 1 );		// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	if (gl_particles->value) {
		const particle_t *p;
		int		i, k;
		vec3_t		up, right;
		float		scale, size = 1.0F, r, g, b;

		qglDepthMask(GL_FALSE);	/* no z buffering */
		qglEnable(GL_BLEND);
		GL_TexEnv(GL_MODULATE);
		qglEnable(GL_TEXTURE_2D);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE);

		/* Vertex arrays  */
		qglEnableClientState(GL_VERTEX_ARRAY);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglEnableClientState(GL_COLOR_ARRAY);

		qglTexCoordPointer(2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
		qglVertexPointer(3, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
		qglColorPointer(4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

		GL_Bind(r_particletexture->texnum);

		VectorScale(vup, size, up);
		VectorScale(vright, size, right);

		for (p = r_newrefdef.particles, i = 0, k = 0; i < r_newrefdef.num_particles; i++, p++, k += 4) {
			/* hack a scale up to keep particles from disapearing */
			scale = (p->origin[0] - r_origin[0]) * vpn[0] +
			        (p->origin[1] - r_origin[1]) * vpn[1] +
			        (p->origin[2] - r_origin[2]) * vpn[2];

			scale = (scale < 20) ? 1 : 1 + scale * 0.0004;

			r = d_8to24tablef[p->color & 0xFF][0];
			g = d_8to24tablef[p->color & 0xFF][1];
			b = d_8to24tablef[p->color & 0xFF][2];

			VA_SetElem2(tex_array[k], 0, 0);
			VA_SetElem4(col_array[k], r, g, b, p->alpha);
			VA_SetElem3(vert_array[k], p->origin[0] - (up[0] * scale) - (right[0] * scale),
			                           p->origin[1] - (up[1] * scale) - (right[1] * scale),
						   p->origin[2] - (up[2] * scale) - (right[2] * scale));

			VA_SetElem2(tex_array[k + 1], 1, 0);
			VA_SetElem4(col_array[k + 1], r, g, b, p->alpha);
			VA_SetElem3(vert_array[k + 1], p->origin[0] + (up[0] * scale) - (right[0] * scale),
			                               p->origin[1] + (up[1] * scale) - (right[1] * scale),
						       p->origin[2] + (up[2] * scale) - (right[2] * scale));

			VA_SetElem2(tex_array[k + 2], 1, 1);
			VA_SetElem4(col_array[k + 2], r, g, b, p->alpha);
			VA_SetElem3(vert_array[k + 2], p->origin[0] + (right[0] * scale) + (up[0] * scale),
			                               p->origin[1] + (right[1] * scale) + (up[1] * scale),
						       p->origin[2] + (right[2] * scale) + (up[2] * scale));

			VA_SetElem2(tex_array[k + 3], 0, 1);
			VA_SetElem4(col_array[k + 3], r, g, b, p->alpha);
			VA_SetElem3(vert_array[k + 3], p->origin[0] + (right[0] * scale) - (up[0] * scale),
			                               p->origin[1] + (right[1] * scale) - (up[1] * scale),
						       p->origin[2] + (right[2] * scale) - (up[2] * scale));
		}

		qglDrawArrays(GL_QUADS, 0, k);

		qglDisable(GL_BLEND);
		qglColor3f(1, 1, 1);
		qglDepthMask(1);/* back to normal Z buffering */
		GL_TexEnv(GL_REPLACE);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglDisableClientState(GL_VERTEX_ARRAY);
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		qglDisableClientState(GL_COLOR_ARRAY);
		
	} else if ( gl_ext_pointparameters->value && qglPointParameterfEXT ) {
		int i;
		unsigned char color[4];
		const particle_t *p;

		qglDepthMask( GL_FALSE );
		qglEnable( GL_BLEND );
		qglDisable( GL_TEXTURE_2D );

		qglPointSize( gl_particle_size->value );

		qglBegin( GL_POINTS );
		for ( i = 0, p = r_newrefdef.particles; i < r_newrefdef.num_particles; i++, p++ )
		{
			*(int *)color = d_8to24table[p->color];
			color[3] = p->alpha*255;

			qglColor4ubv( color );

			qglVertex3fv( p->origin );
		}
		qglEnd();

		qglDisable( GL_BLEND );
		qglColor4f( 1.0F, 1.0F, 1.0F, 1.0F );
		qglDepthMask( GL_TRUE );
		qglEnable( GL_TEXTURE_2D );

	}
	else
		GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table );
}
#endif

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (v_blend[3] < 0.01f)
		return;

	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();

	qglColor4fv (v_blend);

	qglBegin (GL_TRIANGLES);
	qglVertex2f (-5, -5);
	qglVertex2f (10, -5);
	qglVertex2f (-5, 10);
	qglEnd ();

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_ALPHA_TEST);

	qglColor4f(1,1,1,1);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	// start MPO
	if (!g_drawing_refl) {

		// build the transformation matrix for the given view angles
		AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);
	}

	// we want to look from the mirrored origin's perspective when drawing reflections
	if (g_drawing_refl) {
		
		float	distance;

		distance = DotProduct(r_origin, waterNormals[g_active_refl]) - g_waterDistance2[g_active_refl]; 
		VectorMA(r_newrefdef.vieworg, distance*-2, waterNormals[g_active_refl], r_origin);

		if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		{
			vec3_t	temp;

			temp[0] = g_refl_X[g_active_refl];	// sets PVS over x
			temp[1] = g_refl_Y[g_active_refl];	//  and y coordinate of water surface

			VectorCopy(r_origin, temp);
			if( r_newrefdef.rdflags & RDF_UNDERWATER )  {
				temp[2] = g_refl_Z[g_active_refl] - 1;	// just above water level
			}

			else {
				temp[2] = g_refl_Z[g_active_refl] + 1;	// just below water level
			}

			leaf = Mod_PointInLeaf (temp, r_worldmodel);

			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster) )
			{
				r_viewcluster2 = leaf->cluster;
			}
		}
		return;
	}
	// stop MPO

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)  {
		qglEnable(GL_SCISSOR_TEST);
		qglScissor(r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height);
		if (!(r_newrefdef.rdflags & RDF_NOCLEAR)) {
			qglClearColor(0.3, 0.3, 0.3, 1);
			qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			qglClearColor(1, 0, 0.5, 0.5);
		}
		qglDisable(GL_SCISSOR_TEST);
	}
}


void MYgluPerspective(GLdouble fovy, GLdouble aspect,
		      GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   xmin += -( 2 * gl_state.camera_separation ) / zNear;
   xmax += -( 2 * gl_state.camera_separation ) / zNear;

   qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	int		x, x2, y2, y, w, h;
	static GLdouble farz; // DMP skybox size change
	GLdouble boxsize;  // DMP skybox size change
	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;
	
	if (gl_skydistance->modified) {
	
		gl_skydistance->modified = false;
		boxsize = gl_skydistance->value;
		boxsize -= 252 * ceil(boxsize / 2300);
		
		farz = 1.0;
		
		while (farz < boxsize) { // DMP: make this value a power-of-2
			farz *= 2.0;

			if (farz >= 65536.0)  // DMP: don't make it larger than this
				break;
		}
		
		farz *= 2.0;	// DMP: double since boxsize is distance from camera to
				// edge of skybox - not total size of skybox

		ri.Con_Printf(PRINT_DEVELOPER, "farz now set to %g\n", farz);
	}

	// start MPO
	// MPO : we only want to set the viewport if we aren't drawing the reflection
	if (!g_drawing_refl)
	{
		qglViewport (x, y2, w, h);	// MPO : note this happens every frame interestingly enough
	}
	else
	{
		qglViewport(0, 0, g_reflTexW, g_reflTexH);	// width/height of texture size, not screen size
	}
	// stop MPO

	//
	// set up projection matrix
	//
	screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	MYgluPerspective (r_newrefdef.fov_y,  screenaspect,  4,  /*4096*/ farz);

	qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();

	qglRotatef (-90,  1, 0, 0);	    // put Z going up
	qglRotatef (90,  0, 0, 1);	    // put Z going up
	// MPO : +Z is up, +X is forward, +Y is left according to my calculations

	// start MPO
	// standard transformation
	if (!g_drawing_refl) {

	    qglRotatef (-r_newrefdef.viewangles[2], 1, 0, 0);// MPO : this handles rolling (ie when we strafe side to side we roll slightly)
	    qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);// MPO : this handles up/down rotation
	    qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);// MPO : this handles left/right rotation
	    qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);
	    // MPO : this translate call puts the player at the proper spot in the map
	    // MPO : The map is drawn to absolute coordinates
	}
	// mirrored transformation for reflection
	else {
		R_DoReflTransform(true);
	}
	// end MPO

	if ( gl_state.camera_separation != 0 && gl_state.stereo_enabled )
		qglTranslatef ( gl_state.camera_separation, 0, 0 );

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if ((gl_cull->value) && (!g_drawing_refl)) // MPO : we must disable culling when drawing the reflection
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (gl_clear->value){
		qglClearColor (0.5f, 0.5f, 0.5f, 1.0f);
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else
		qglClear (GL_DEPTH_BUFFER_BIT);

	if (have_stencil && gl_shadows->value == 2) {
		qglClearStencil(1);
		qglClear(GL_STENCIL_BUFFER_BIT);
	}
	
	gldepthmin = 0;
	gldepthmax = 1;
	qglDepthFunc (GL_LEQUAL);
	

	qglDepthRange (gldepthmin, gldepthmax);

	if (have_stencil && gl_shadows->value == 2) {
		qglClearStencil(1);
		qglClear(GL_STENCIL_BUFFER_BIT);
	}

}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
#if !defined(PARTICLESYSTEM)
void R_RenderView (refdef_t *fd)
{
	
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value) {
		c_brush_polys = 0;
		c_alias_polys = 0;
		c_flares = 0;
	}

	R_PushDlights ();

	if (gl_finish->value)
		qglFinish ();
		
	R_SetupGL ();		//MPO moved here ..

	R_SetupFrame ();

	R_SetFrustum ();

	setupClippingPlanes();		// MPO clipping planes for water

	R_MarkLeaves ();	// done here so we know if we're in water
	
//	drawPlayerReflection();

	R_DrawWorld ();
	
	R_RenderFlares ();
	
	R_AddDecals();
	
	R_DrawEntitiesOnList ();

	R_RenderDlights ();

	R_DrawParticles ();

	R_DrawAlphaSurfaces();
	

	R_DrawParticles (); // MPO dukey particles have to be drawn twice .. otherwise you dont get reflection of them.
	
	// start MPO
	// if we are doing a reflection, turn off clipping now
	if (g_drawing_refl)
		qglDisable(GL_CLIP_PLANE0);
	// if we aren't doing a reflection then we can do the flash and r speeds
	// (we don't want them showing up in our reflection)
	else
		R_PolyBlend();
	// stop MPO
	
	R_BloomBlend(fd);
	R_Fog();

	if (gl_minimap_size->value > 32 && !(r_newrefdef.rdflags & RDF_IRGOGGLES)) {
		qglDisable(GL_ALPHA_TEST);
		GL_DrawRadar();
		numRadarEnts = 0;
	}
	
}
#endif

int R_DrawRSpeeds(char *S)
{
	return sprintf(S, "%4i wpoly %4i epoly %i tex %i lmaps", 
                       c_brush_polys, c_alias_polys,
	               c_visible_textures, c_visible_lightmaps);
}


unsigned int	blurtex = 0;
void R_MotionBlur (void) 
{

	if (!gl_state.tex_rectangle)
		return;
		
	if (gl_state.tex_rectangle) {
		if (blurtex) {
			GL_TexEnv(GL_MODULATE);
			qglDisable(GL_TEXTURE_2D);
			qglEnable(GL_TEXTURE_RECTANGLE_NV);
			qglEnable(GL_BLEND);
			qglDisable(GL_ALPHA_TEST);
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if (gl_motionblur_intensity->value >= 1.0)
				qglColor4f(1.0f, 1.0f, 1.0f, 0.45f);
			else
				qglColor4f(1.0f, 1.0f, 1.0f, gl_motionblur_intensity->value);
			qglBegin(GL_QUADS);
			{
				qglTexCoord2f(0, vid.height);
				qglVertex2f(0, 0);
				qglTexCoord2f(vid.width, vid.height);
				qglVertex2f(vid.width, 0);
				qglTexCoord2f(vid.width, 0);
				qglVertex2f(vid.width, vid.height);
				qglTexCoord2f(0, 0);
				qglVertex2f(0, vid.height);
			}
			qglEnd();
			qglDisable(GL_TEXTURE_RECTANGLE_NV);
			qglEnable(GL_TEXTURE_2D);
		}
		if (!blurtex)
			qglGenTextures(1, &blurtex);
		qglBindTexture(GL_TEXTURE_RECTANGLE_NV, blurtex);
		qglCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, vid.width, vid.height, 0);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}

void	R_SetGL2D (void)
{
	// set 2D virtual screen size
	qglViewport (0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
    	qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    	qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	if (gl_motionblur->value == 1 && !(cl_3dcam->value) && r_newrefdef.rdflags & RDF_UNDERWATER)
		R_MotionBlur();
	else if (gl_motionblur->value == 2)
		R_MotionBlur();
	qglDisable (GL_BLEND);
	qglEnable (GL_ALPHA_TEST);
	qglColor4f (1,1,1,1);
	/* ECHON */
	if (r_speeds->value && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) {
		char	S[128];
		int	n = 0;
		int	i;

		n = R_DrawRSpeeds(S);

		for (i = 0; i < n; i++)
			Draw_Char(r_newrefdef.width - 4 + ((i - n) * 8),
			          r_newrefdef.height - 40, 128 + S[i], 255);
	}

}


static void GL_DrawColoredStereoLinePair( float r, float g, float b, float y )
{
	qglColor3f( r, g, b );
	qglVertex2f( 0, y );
	qglVertex2f( vid.width, y );
	qglColor3f( 0, 0, 0 );
	qglVertex2f( 0, y + 1 );
	qglVertex2f( vid.width, y + 1 );
}


static void GL_DrawStereoPattern( void )
{
	int i;

	if ( !( gl_config.renderer & GL_RENDERER_INTERGRAPH ) )
		return;

	if ( !gl_state.stereo_enabled )
		return;

	R_SetGL2D();

	qglDrawBuffer( GL_BACK_LEFT );

	for ( i = 0; i < 20; i++ )
	{
		qglBegin( GL_LINES );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 0 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 2 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 4 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 6 );
			GL_DrawColoredStereoLinePair( 0, 1, 0, 8 );
			GL_DrawColoredStereoLinePair( 1, 1, 0, 10);
			GL_DrawColoredStereoLinePair( 1, 1, 0, 12);
			GL_DrawColoredStereoLinePair( 0, 1, 0, 14);
		qglEnd();
		
		GLimp_EndFrame();
	}
}


/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (r_newrefdef.vieworg, shadelight, true);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1]) {
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else {
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	//start MPO
	if(gl_reflection->value) {
		r_newrefdef = *fd; //need to do this so eye coords update b4 frame is drawn
		R_clear_refl(); //clear our reflections found in last frame
		
		if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
			R_RecursiveFindRefl(r_worldmodel->nodes); //find reflections for this frame
			
		R_UpdateReflTex( fd ); //render reflections to textures
	}
	else 
		R_clear_refl();
	// end MPO

	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGL2D ();

	// start MPO
	// if debugging is enabled and reflections are enabled.. draw it
	if ((gl_reflection_debug->value) && (g_refl_enabled)) 
		R_DrawDebugReflTexture();
	// end MPO
}

void R_Register( void )
{
	r_lefthand = ri.Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get ("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get ("r_lightlevel", "0", 0);

	gl_nosubimage = ri.Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = ri.Cvar_Get( "gl_allow_software", "0", 0 );

#if defined(PARTICLESYSTEM)
	gl_particle_lighting = ri.Cvar_Get ("gl_particle_lighting", "0.75", 0 );
	gl_transrendersort = ri.Cvar_Get("gl_transrendersort", "1", 0);
	gl_particle_min = ri.Cvar_Get ("gl_particle_min", "0", 0 );
	gl_particle_max = ri.Cvar_Get ("gl_particle_max", "0", 0 );
#else
	gl_particle_min_size = ri.Cvar_Get( "gl_particle_min_size", "2", 0 );
	gl_particle_max_size = ri.Cvar_Get( "gl_particle_max_size", "40", 0 );
	gl_particle_size = ri.Cvar_Get( "gl_particle_size", "40", 0 );
	gl_particle_att_a = ri.Cvar_Get( "gl_particle_att_a", "0.01", 0 );
	gl_particle_att_b = ri.Cvar_Get( "gl_particle_att_b", "0.0", 0 );
	gl_particle_att_c = ri.Cvar_Get( "gl_particle_att_c", "0.01", 0 );
	gl_particles = ri.Cvar_Get( "gl_particles", "0", CVAR_ARCHIVE );
#endif

	gl_modulate = ri.Cvar_Get ("gl_modulate", "1", CVAR_ARCHIVE );
	gl_log = ri.Cvar_Get( "gl_log", "0", 0 );
	gl_bitdepth = ri.Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = ri.Cvar_Get( "gl_mode", "4", CVAR_ARCHIVE );
	gl_lightmap = ri.Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = ri.Cvar_Get ("gl_dynamic", "1", 0);
	gl_nobind = ri.Cvar_Get ("gl_nobind", "0", 0);
	gl_round_down = ri.Cvar_Get ("gl_round_down", "1", 0);
	gl_picmip = ri.Cvar_Get ("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = ri.Cvar_Get ("gl_showtris", "0", 0);
	gl_finish = ri.Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = ri.Cvar_Get ("gl_clear", "0", 0);
	gl_cull = ri.Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get ("gl_flashblend", "0", 0);
	gl_playermip = ri.Cvar_Get ("gl_playermip", "0", 0);
	gl_monolightmap = ri.Cvar_Get( "gl_monolightmap", "0", 0 );
	gl_driver = ri.Cvar_Get("gl_driver", GL_DRIVER_LIB, CVAR_ARCHIVE);
	gl_texturemode = ri.Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	gl_texturealphamode = ri.Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = ri.Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = ri.Cvar_Get( "gl_lockpvs", "0", 0 );

//	gl_vertex_arrays = ri.Cvar_Get( "gl_vertex_arrays", "0", 0 );

	gl_ext_swapinterval = ri.Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_palettedtexture = ri.Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );
	gl_ext_multitexture = ri.Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	gl_ext_pointparameters = ri.Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array = ri.Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_drawbuffer = ri.Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = ri.Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );

	gl_saturatelighting = ri.Cvar_Get( "gl_saturatelighting", "0", 0 );

	gl_3dlabs_broken = ri.Cvar_Get( "gl_3dlabs_broken", "1", CVAR_ARCHIVE );
	
	hw_gl_strings = ri.Cvar_Get( "hw_gl_strings", "0", 0 );
	gl_overbrightbits = ri.Cvar_Get( "gl_overbrightbits", "2", CVAR_ARCHIVE );
	gl_ext_mtexcombine = ri.Cvar_Get( "gl_ext_mtexcombine", "1", CVAR_ARCHIVE );
	gl_ext_texture_compression = ri.Cvar_Get("gl_ext_texture_compression", "0", CVAR_ARCHIVE);
	gl_motionblur = ri.Cvar_Get("gl_motionblur", "1", CVAR_ARCHIVE);
	gl_motionblur_intensity = ri.Cvar_Get("gl_motionblur_intensity", "0.8", CVAR_ARCHIVE);
	gl_skydistance = ri.Cvar_Get( "gl_skydistance", "3500", CVAR_ARCHIVE );
	gl_water_waves = ri.Cvar_Get ("gl_water_waves", "0", CVAR_ARCHIVE);
	gl_water_caustics = ri.Cvar_Get("gl_water_caustics", "1", CVAR_ARCHIVE);
	gl_water_caustics_image = ri.Cvar_Get("gl_water_caustics_image", "/textures/water/caustics.pcx", CVAR_ARCHIVE);
	gl_detailtextures = ri.Cvar_Get ("gl_detailtextures", "0.0", CVAR_ARCHIVE);
	gl_reflection = ri.Cvar_Get("gl_water_reflection", "0", CVAR_ARCHIVE);		// MPO
	gl_reflection_debug = ri.Cvar_Get("gl_water_reflection_debug", "0", 0 );		// MPO
	gl_reflection_max = ri.Cvar_Get("gl_water_reflection_max", "2", 0);	// MPO
	gl_reflection_shader = ri.Cvar_Get("gl_water_reflection_shader", "0", CVAR_ARCHIVE);	// MPO
	gl_reflection_shader_image = ri.Cvar_Get("gl_water_reflection_shader_image",
	                                         "/textures/water/distortion.pcx", CVAR_ARCHIVE);
	gl_reflection_fragment_program = ri.Cvar_Get("gl_water_reflection_fragment_program", "0", CVAR_ARCHIVE);
	gl_reflection_water_surf = ri.Cvar_Get("gl_water_reflection_surf", "1", CVAR_ARCHIVE);
	gl_fogenable = ri.Cvar_Get("gl_fogenable", "0", CVAR_ARCHIVE);
	gl_fogstart = ri.Cvar_Get("gl_fogstart", "50.0", CVAR_ARCHIVE);
	gl_fogend = ri.Cvar_Get("gl_fogend", "1500.0", CVAR_ARCHIVE);
	gl_fogdensity = ri.Cvar_Get("gl_fogdensity", "0.001", CVAR_ARCHIVE);
	gl_fogred = ri.Cvar_Get("gl_fogred", "0.4", CVAR_ARCHIVE);
	gl_foggreen = ri.Cvar_Get("gl_foggreen", "0.4", CVAR_ARCHIVE);
	gl_fogblue = ri.Cvar_Get("gl_fogblue", "0.4", CVAR_ARCHIVE);
	gl_fogunderwater = ri.Cvar_Get("gl_fogunderwater", "0", CVAR_ARCHIVE);
	gl_screenshot_jpeg = ri.Cvar_Get("gl_screenshot_jpeg", "1", CVAR_ARCHIVE);
	gl_screenshot_jpeg_quality = ri.Cvar_Get("gl_screenshot_jpeg_quality", "95", CVAR_ARCHIVE);
	levelshots = ri.Cvar_Get ("levelshots", "0", 0);
#if defined(PARTICLESYSTEM)
	r_decals = ri.Cvar_Get("r_decals", "256", CVAR_ARCHIVE);
#else
	r_decals = ri.Cvar_Get("r_decals", "1", CVAR_ARCHIVE);
#endif
	r_decals_time = ri.Cvar_Get("r_decals_time", "30", CVAR_ARCHIVE);
	gl_lightmap_texture_saturation = ri.Cvar_Get("gl_lightmap_texture_saturation", "1", CVAR_ARCHIVE);
	gl_lightmap_saturation = ri.Cvar_Get("gl_lightmap_saturation", "1", CVAR_ARCHIVE);
	gl_anisotropy = ri.Cvar_Get("gl_anisotropy", "8", CVAR_ARCHIVE);
	gl_shell_image = ri.Cvar_Get("gl_shell_image", "/gfx/shell.pcx", CVAR_ARCHIVE);
	gl_minimap = ri.Cvar_Get("gl_minimap", "0", CVAR_ARCHIVE);
	gl_minimap_size = ri.Cvar_Get("gl_minimap_size", "400", CVAR_ARCHIVE);
	gl_minimap_zoom = ri.Cvar_Get("gl_minimap_zoom", "1.5", CVAR_ARCHIVE);
	gl_minimap_style = ri.Cvar_Get("gl_minimap_style", "1", CVAR_ARCHIVE);
	gl_minimap_x = ri.Cvar_Get("gl_minimap_x", "800", CVAR_ARCHIVE);
	gl_minimap_y = ri.Cvar_Get("gl_minimap_y", "400", CVAR_ARCHIVE);
	gl_lensflare = ri.Cvar_Get( "gl_lensflare", "1", CVAR_ARCHIVE );
	gl_lensflare_intens = ri.Cvar_Get ("gl_lensflare_intens", "1.5", CVAR_ARCHIVE);
	gl_cellshade = ri.Cvar_Get("gl_cellshade", "0", CVAR_ARCHIVE);
	gl_cellshade_width = ri.Cvar_Get("gl_cellshade_width", "4", CVAR_ARCHIVE);

	vid_fullscreen = ri.Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = ri.Cvar_Get( "vid_gamma", "0.8", CVAR_ARCHIVE );
	vid_ref = ri.Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE );
	
	cl_3dcam = ri.Cvar_Get ("cl_3dcam", "0", CVAR_ARCHIVE);
	con_font_size = ri.Cvar_Get ("con_font_size", "12", CVAR_ARCHIVE);
	font_size = ri.Cvar_Get ("font_size", "8", CVAR_ARCHIVE);
	alt_text_color = ri.Cvar_Get ("alt_text_color", "2", CVAR_ARCHIVE);
	con_font = ri.Cvar_Get ("con_font", "default", CVAR_ARCHIVE);
	
	ri.Cmd_AddCommand( "imagelist", GL_ImageList_f );
	ri.Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gl_strings", GL_Strings_f );
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

	if ( vid_fullscreen->modified && !gl_config.allow_cds ) {
		ri.Con_Printf( PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n" );
		ri.Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;
	gl_skydistance->modified = true; // DMP skybox size change
	vid_fullscreen->modified = false;
	gl_mode->modified = false;
#if 0
	gl_reflection_shader->modified = false;
	gl_reflection_shader_image->modified = false; 
#endif

	if ((err = GLimp_SetMode((int *)&vid.width, (int *)&vid.height, gl_mode->value, fullscreen)) == rserr_ok)
		gl_state.prev_mode = gl_mode->value;
	else {
		if (err == rserr_invalid_fullscreen) {
			ri.Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf( PRINT_ALL, "GL::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( (int *)&vid.width, (int *)&vid.height, gl_mode->value, false ) ) == rserr_ok )
				return true;
		}
		else if (err == rserr_invalid_mode) {
			ri.Cvar_SetValue( "gl_mode", gl_state.prev_mode );
			gl_mode->modified = false;
			ri.Con_Printf( PRINT_ALL, "GL::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode((int *)&vid.width, (int *)&vid.height, gl_state.prev_mode, false)) != rserr_ok) {
			ri.Con_Printf( PRINT_ALL, "GL::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
}

/*
===============
R_Init
===============
*/
int R_Init( void *hinstance, void *hWnd )
{	
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int		err;
	int		j;
	extern float r_turbsin[256];
	int		init_time;

	init_time = Sys_Milliseconds();
	
	for ( j = 0; j < 256; j++ )
		r_turbsin[j] *= 0.5;

	ri.Con_Printf (PRINT_ALL, "GL version: "REF_VERSION"\n");

	Draw_GetPalette ();

	R_Register();

	// initialize our QGL dynamic bindings
	if ( !QGL_Init( gl_driver->string ) ) {
		QGL_Shutdown();
        	ri.Con_Printf (PRINT_ALL, "GL::R_Init() - could not load \"%s\"\n", gl_driver->string );
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hinstance, hWnd ) ) {
		QGL_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode () ) {
		QGL_Shutdown();
        	ri.Con_Printf (PRINT_ALL, "GL::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	ri.Vid_MenuInit();
	
	Com_Printf("\nGraphic Card Info\n");

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (char *)qglGetString (GL_VENDOR);
		Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = (char *)qglGetString (GL_RENDERER);
		Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = (char *)qglGetString (GL_VERSION);
		Com_Printf ("GL_VERSION: %s\n", gl_config.version_string );
		
	gl_config.extensions_string = (char *)qglGetString (GL_EXTENSIONS);
	if (hw_gl_strings->value) 
		Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensions_string );
	
	Q_strncpyz(renderer_buffer, gl_config.renderer_string, sizeof(renderer_buffer));
	Q_strlwr(renderer_buffer);

	Q_strncpyz(vendor_buffer, gl_config.vendor_string, sizeof(vendor_buffer));
	Q_strlwr(vendor_buffer);

	if ( strstr( renderer_buffer, "voodoo" ) ) {
		if ( !strstr( renderer_buffer, "rush" ) )
			gl_config.renderer = GL_RENDERER_VOODOO;
		else
			gl_config.renderer = GL_RENDERER_VOODOO_RUSH;
	}
	else if ( strstr( vendor_buffer, "sgi" ) )
		gl_config.renderer = GL_RENDERER_SGI;
	else if ( strstr( renderer_buffer, "permedia" ) )
		gl_config.renderer = GL_RENDERER_PERMEDIA2;
	else if ( strstr( renderer_buffer, "glint" ) )
		gl_config.renderer = GL_RENDERER_GLINT_MX;
	else if ( strstr( renderer_buffer, "glzicd" ) )
		gl_config.renderer = GL_RENDERER_REALIZM;
	else if ( strstr( renderer_buffer, "gdi" ) )
		gl_config.renderer = GL_RENDERER_MCD;
	else if ( strstr( renderer_buffer, "pcx2" ) )
		gl_config.renderer = GL_RENDERER_PCX2;
	else if ( strstr( renderer_buffer, "verite" ) )
		gl_config.renderer = GL_RENDERER_RENDITION;
	else
		gl_config.renderer = GL_RENDERER_OTHER;

	if ( toupper( gl_monolightmap->string[1] ) != 'F' ) {
		if ( gl_config.renderer == GL_RENDERER_PERMEDIA2 ) {
			ri.Cvar_Set( "gl_monolightmap", "A" );
			ri.Con_Printf( PRINT_ALL, "Using gl_monolightmap 'a'\n" );
		}
		else if ( gl_config.renderer & GL_RENDERER_POWERVR ) {
			ri.Cvar_Set( "gl_monolightmap", "0" );
		}
		else
			ri.Cvar_Set( "gl_monolightmap", "0" );
	}

	// power vr can't have anything stay in the framebuffer, so
	// the screen needs to redraw the tiled background every frame
	if ( gl_config.renderer & GL_RENDERER_POWERVR ) 
		ri.Cvar_Set( "scr_drawall", "1" );
	else
		ri.Cvar_Set( "scr_drawall", "0" );

#if defined (__unix__)
	ri.Cvar_SetValue( "gl_finish", 1 );
#endif

	// MCD has buffering issues
	if ( gl_config.renderer == GL_RENDERER_MCD )
		ri.Cvar_SetValue( "gl_finish", 1 );

	if ( gl_config.renderer & GL_RENDERER_3DLABS ) {
		if ( gl_3dlabs_broken->value )
			gl_config.allow_cds = false;
		else
			gl_config.allow_cds = true;
	}
	else
		gl_config.allow_cds = true;

	if ( gl_config.allow_cds )
		ri.Con_Printf( PRINT_ALL, "CDS enabled.\n" );
	else
		ri.Con_Printf( PRINT_ALL, "CDS disabled.\n" );

	/*
	** grab extensions
	*/
	if ( strstr( gl_config.extensions_string, "GL_EXT_compiled_vertex_array" ) || 
		 strstr( gl_config.extensions_string, "GL_SGI_compiled_vertex_array" ) ) {
		ri.Con_Printf( PRINT_ALL, "GL_EXT_compiled_vertex_array enabled.\n" );
#ifdef _WIN32
		qglLockArraysEXT = ( void (__stdcall *)(int, int) ) qwglGetProcAddress( "glLockArraysEXT" );
		qglUnlockArraysEXT = ( void (__stdcall *)(void) ) qwglGetProcAddress( "glUnlockArraysEXT" );
#else
		qglLockArraysEXT = ( void * ) qwglGetProcAddress( "glLockArraysEXT" );
		qglUnlockArraysEXT = ( void * ) qwglGetProcAddress( "glUnlockArraysEXT" );
#endif
	}
	else
		ri.Con_Printf( PRINT_ALL, "GL_EXT_compiled_vertex_array not found.\n" );

#ifdef _WIN32
	if ( strstr( gl_config.extensions_string, "WGL_EXT_swap_control" ) ) {
		qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
		ri.Con_Printf( PRINT_ALL, "WGL_EXT_swap_control enabled.\n" );
	}
	else
		ri.Con_Printf( PRINT_ALL, "WGL_EXT_swap_control not found.\n" );
#endif

	if ( strstr( gl_config.extensions_string, "GL_EXT_point_parameters" ) ) {
		if ( gl_ext_pointparameters->value ) {
			qglPointParameterfEXT = ( void (APIENTRY *)( GLenum, GLfloat ) ) qwglGetProcAddress( "glPointParameterfEXT" );
			qglPointParameterfvEXT = ( void (APIENTRY *)( GLenum, const GLfloat * ) ) qwglGetProcAddress( "glPointParameterfvEXT" );
			ri.Con_Printf( PRINT_ALL, "GL_EXT_point_parameters enabled.\n" );
		}
		else
			ri.Con_Printf( PRINT_ALL, "Ignoring GL_EXT_point_parameters.\n" );
	}
	else
		ri.Con_Printf( PRINT_ALL, "GL_EXT_point_parameters not found.\n" );

#if !defined(PARTICLESYSTEM)
	if ( !qglColorTableEXT &&
		strstr( gl_config.extensions_string, "GL_EXT_paletted_texture" ) && 
		strstr( gl_config.extensions_string, "GL_EXT_shared_texture_palette" ) ) {
		if ( gl_ext_palettedtexture->value ) {
			ri.Con_Printf( PRINT_ALL, "GL_EXT_shared_texture_palette enabled.\n" );
			qglColorTableEXT = ( void ( APIENTRY * ) ( int, int, int, int, int, const void * ) ) qwglGetProcAddress( "glColorTableEXT" );
		}
		else
			ri.Con_Printf( PRINT_ALL, "Ignoring GL_EXT_shared_texture_palette.\n" );
	}
	else
		ri.Con_Printf( PRINT_ALL, "GL_EXT_shared_texture_palette not found.\n" );
#endif
	if ( strstr( gl_config.extensions_string, "GL_ARB_multitexture" ) ) {
		if ( gl_ext_multitexture->value ) {
			ri.Con_Printf( PRINT_ALL, "GL_ARB_multitexture enabled.\n" );
#ifdef _WIN32
			qglMTexCoord2fSGIS = ( void (__stdcall *)(GLenum, GLfloat, GLfloat) ) 
			                         qwglGetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( void (__stdcall *)(GLenum) ) 
			                         qwglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( void (__stdcall *)(GLenum) ) 
			                         qwglGetProcAddress( "glClientActiveTextureARB" );
			qglMultiTexCoord3fvARB = ( void (__stdcall *)(GLenum, const GLfloat * v) )
			                         qwglGetProcAddress( "glMultiTexCoord3fvARB" ); //mpo
#else	
			qglMTexCoord2fSGIS = ( void * ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( void * ) qwglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( void * ) qwglGetProcAddress( "glClientActiveTextureARB" );
			qglMultiTexCoord3fvARB = ( void * ) qwglGetProcAddress( "glMultiTexCoord3fvARB" ); //mpo
#endif
			qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&maxTextureUnits);// MH - see how many texture units card supports
			GL_TEXTURE0 = GL_TEXTURE0_ARB;
			GL_TEXTURE1 = GL_TEXTURE1_ARB;
			GL_TEXTURE2 = GL_TEXTURE2_ARB; // MH - add extra texture unit here
			
			ri.Con_Printf( PRINT_ALL, "Texture units : %d\n", maxTextureUnits );
		}
		else
			ri.Con_Printf( PRINT_ALL, "Ignoring GL_ARB_multitexture.\n" );
	}
	else
		ri.Con_Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );

	if ( strstr( gl_config.extensions_string, "GL_SGIS_multitexture" ) ) {
		if ( qglActiveTextureARB )
			ri.Con_Printf( PRINT_ALL, "GL_SGIS_multitexture deprecated in favor of ARB_multitexture\n" );
		else if ( gl_ext_multitexture->value ) {
			ri.Con_Printf( PRINT_ALL, "GL_SGIS_multitexture enabled.\n" );
#ifdef _WIN32
			qglMTexCoord2fSGIS = ( void (__stdcall *)(GLenum, GLfloat, GLfloat) ) 
			                     qwglGetProcAddress( "glMTexCoord2fSGIS" );
			qglSelectTextureSGIS = ( void (__stdcall *)(GLenum) ) 
			                       qwglGetProcAddress( "glSelectTextureSGIS" );
#else
			qglMTexCoord2fSGIS = ( void * ) qwglGetProcAddress( "glMTexCoord2fSGIS" );
			qglSelectTextureSGIS = ( void * ) qwglGetProcAddress( "glSelectTextureSGIS" );
#endif			
			qglGetIntegerv(GL_MAX_TEXTURE_UNITS_SGIS,&maxTextureUnits);	// MH - see how many texture units card supports
			GL_TEXTURE0 = GL_TEXTURE0_SGIS;
			GL_TEXTURE1 = GL_TEXTURE1_SGIS;
			GL_TEXTURE2 = GL_TEXTURE2_SGIS;		// MH - add extra texture unit
		}
		else
			ri.Con_Printf( PRINT_ALL, "Ignoring GL_SGIS_multitexture.\n" );
	}
	else
		ri.Con_Printf( PRINT_ALL, "GL_SGIS_multitexture not found.\n" );
	
	{
		if (strstr(gl_config.extensions_string, "GL_NV_register_combiners")) {
		
			ri.Con_Printf(PRINT_ALL, "GL_NV_register_combiners enabled.\n");

			qglCombinerParameterfvNV = 
			(PFNGLCOMBINERPARAMETERFVNVPROC) qwglGetProcAddress("glCombinerParameterfvNV");
			qglCombinerParameterfNV = 
			(PFNGLCOMBINERPARAMETERFNVPROC) qwglGetProcAddress("glCombinerParameterfNV");
			qglCombinerParameterivNV = 
			(PFNGLCOMBINERPARAMETERIVNVPROC) qwglGetProcAddress("glCombinerParameterivNV");
			qglCombinerParameteriNV = 
			(PFNGLCOMBINERPARAMETERINVPROC) qwglGetProcAddress("glCombinerParameteriNV");
			qglCombinerInputNV = 
			(PFNGLCOMBINERINPUTNVPROC) qwglGetProcAddress("glCombinerInputNV");
			qglCombinerOutputNV = 
			(PFNGLCOMBINEROUTPUTNVPROC) qwglGetProcAddress("glCombinerOutputNV");
			qglFinalCombinerInputNV = 
			(PFNGLFINALCOMBINERINPUTNVPROC) qwglGetProcAddress("glFinalCombinerInputNV");
			qglGetCombinerInputParameterfvNV = 
			(PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) qwglGetProcAddress("glGetCombinerInputParameterfvNV");
			qglGetCombinerInputParameterivNV = 
			(PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) qwglGetProcAddress("glGetCombinerInputParameterivNV");
			qglGetCombinerOutputParameterfvNV = 
			(PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) qwglGetProcAddress("glGetCombinerOutputParameterfvNV");
			qglGetCombinerOutputParameterivNV = 
			(PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) qwglGetProcAddress("glGetCombinerOutputParameterivNV");
			qglGetFinalCombinerInputParameterfvNV = 
			(PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) qwglGetProcAddress("glGetFinalCombinerInputParameterfvNV");
			qglGetFinalCombinerInputParameterivNV = 
			(PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) qwglGetProcAddress("glGetFinalCombinerInputParameterivNV");

			gl_state.reg_combiners = true;
		} else {
			ri.Con_Printf(PRINT_ALL, "Ignoring GL_NV_register_combiners.\n");
			gl_state.reg_combiners = false;
		}

		if (strstr(gl_config.extensions_string, "GL_ARB_texture_compression")) {
			if (!gl_ext_texture_compression->value) {
				ri.Con_Printf(PRINT_ALL, "Ignoring GL_ARB_texture_compression.\n");
				gl_state.texture_compression = false;
			} else {
				ri.Con_Printf(PRINT_ALL, "GL_ARB_texture_compression enabled.\n");
				gl_state.texture_compression = true;
			}
		} else {
			ri.Con_Printf(PRINT_ALL, "GL_ARB_texture_compression not found.\n");
			gl_state.texture_compression = false;
			ri.Cvar_Set("gl_ext_texture_compression", "0");
		}

		if (strstr(gl_config.extensions_string, "GL_NV_texture_rectangle")) {
			Com_Printf("GL_NV_texture_rectangle enabled.\n");
			gl_state.tex_rectangle = true;
		} else if (strstr(gl_config.extensions_string, "GL_EXT_texture_rectangle")) {
			Com_Printf("GL_EXT_texture_rectangle enabled.\n");
			gl_state.tex_rectangle = true;
		} else {
			Com_Printf("GL_NV_texture_rectangle not found.\n");
			gl_state.tex_rectangle = false;
		}

		/* Vic - begin */
		gl_config.mtexcombine = false;

		if (strstr(gl_config.extensions_string, "GL_ARB_texture_env_combine")) {
			if (gl_ext_mtexcombine->value) {
				Com_Printf("GL_ARB_texture_env_combine enabled.\n");
				gl_config.mtexcombine = true;
			} else
				Com_Printf("Ignoring GL_ARB_texture_env_combine.\n");
		} else
			Com_Printf("GL_ARB_texture_env_combine not found.\n");

		if (!gl_config.mtexcombine) {
			if (strstr(gl_config.extensions_string, "GL_EXT_texture_env_combine")) {
				if (gl_ext_mtexcombine->value) {
					Com_Printf("GL_EXT_texture_env_combine enabled.\n");
					gl_config.mtexcombine = true;
				} else
					Com_Printf("Ignoring GL_EXT_texture_env_combine.\n");
			} else
				Com_Printf("GL_EXT_texture_env_combine not found.\n");
		}
		/* Vic - end */
		
		// mpo === jitwater
		if (strstr(gl_config.extensions_string, "GL_ARB_fragment_program")) {

			if(maxTextureUnits>=3 && (gl_reflection_fragment_program->value))
				gl_state.fragment_program = true;

			ri.Con_Printf(PRINT_ALL, "GL_ARB_fragment_program enabled.\n");
#ifdef _WIN32
			qglGenProgramsARB = (void (__stdcall *)(GLint, GLuint *programs))qwglGetProcAddress("glGenProgramsARB");
			qglDeleteProgramsARB = (void (__stdcall *)(GLint, const GLuint *programs))qwglGetProcAddress("glDeleteProgramsARB");
			qglBindProgramARB = (void (__stdcall *)(GLenum, GLuint))qwglGetProcAddress("glBindProgramARB");
			qglProgramStringARB = (void (__stdcall *)(GLenum, GLenum, GLint, const void *))qwglGetProcAddress("glProgramStringARB");
			qglProgramEnvParameter4fARB = (void (__stdcall *)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat))qwglGetProcAddress("glProgramEnvParameter4fARB");
			qglProgramLocalParameter4fARB = (void (__stdcall *)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat))qwglGetProcAddress("glProgramLocalParameter4fARB");
#else
			qglGenProgramsARB = (void *)qwglGetProcAddress("glGenProgramsARB");
			qglDeleteProgramsARB = (void *)qwglGetProcAddress("glDeleteProgramsARB");
			qglBindProgramARB = (void *)qwglGetProcAddress("glBindProgramARB");
			qglProgramStringARB = (void *)qwglGetProcAddress("glProgramStringARB");
			qglProgramEnvParameter4fARB = (void *)qwglGetProcAddress("glProgramEnvParameter4fARB");
			qglProgramLocalParameter4fARB = (void *)qwglGetProcAddress("glProgramLocalParameter4fARB");
#endif		
			if (!gl_reflection_fragment_program->value) {
				gl_state.fragment_program = false;
				ri.Con_Printf(PRINT_ALL, "GL_ARB_fragment_program available but disabled, set the variable\n");
				ri.Con_Printf(PRINT_ALL, "gl_water_reflection_fragment_program to 1 and type vid_restart.\n");
			}
	
			if (!(qglGenProgramsARB && 
		              qglDeleteProgramsARB && 
		              qglBindProgramARB &&
		              qglProgramStringARB && 
		              qglProgramEnvParameter4fARB && 
		              qglProgramLocalParameter4fARB)) {
					gl_state.fragment_program = false;
					ri.Cvar_Set("gl_water_reflection_fragment_program", "0");
					ri.Con_Printf(PRINT_ALL, "... Failed!  Fragment programs disabled\n");
			}
		}
		else {
			gl_state.fragment_program = false;
			ri.Con_Printf(PRINT_ALL,"GL_ARB_fragment_program not found.\n");
			ri.Con_Printf(PRINT_ALL,"Don't try to run water distorsion, it will crash.\n");
		}
		// jitwater ===
		// jitanisotropy
		gl_state.max_anisotropy = 0;

		if (strstr(gl_config.extensions_string, "texture_filter_anisotropic")) {
			qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_state.max_anisotropy);
		}
			ri.Con_Printf(PRINT_ALL, "Max anisotropy level: %g\n", gl_state.max_anisotropy);
	}
	
	GL_SetDefaultState();

	/*
	** draw our stereo patterns
	*/
#if 1 // commented out until H3D pays us the money they owe us
	GL_DrawStereoPattern();
#endif
	// jitanisotropy(make sure nobody goes out of bounds)
	if (gl_anisotropy->value < 0)
		ri.Cvar_Set("gl_anisotropy", "0");
	else if (gl_anisotropy->value > gl_state.max_anisotropy)
		ri.Cvar_SetValue("gl_anisotropy", gl_state.max_anisotropy);

	if (gl_lightmap_texture_saturation->value > 1 || gl_lightmap_texture_saturation->value < 0)
		ri.Cvar_Set("gl_lightmap_texture_saturation", "1");	/* jitsaturation */

	GL_InitImages ();
	Mod_Init ();
	R_InitParticleTexture ();
	Draw_InitLocal ();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Con_Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);
		
	R_init_refl(gl_reflection_max->value);	// MPO : init reflections

	Com_Printf("Render Initialized in: %.2fs\n", (Sys_Milliseconds() - init_time) * 0.001);
		
	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{	
	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("gl_strings");

	Mod_FreeAll ();

	GL_ShutdownImages ();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	QGL_Shutdown();

	/*
	** shutdown our reflective arrays
	*/
	R_shutdown_refl();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{

	gl_state.camera_separation = camera_separation;
	
	// Knightmare- added Psychospaz's console font size option
	if (con_font->modified)
		RefreshFont ();
		
	if (con_font_size->modified) {
		if (con_font_size->value<8)
			ri.Cvar_Set( "con_font_size", "8" );
		else if (con_font_size->value>32)
			ri.Cvar_Set( "con_font_size", "32" );

		con_font_size->modified = false;
	}

	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified || vid_fullscreen->modified /*|| gl_reflection_shader->modified ||
	     gl_reflection_shader_image->modified */)
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = ri.Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	if ( gl_log->modified )
	{
		GLimp_EnableLogging( gl_log->value );
		gl_log->modified = false;
	}

	if ( gl_log->value )
	{
		GLimp_LogNewFrame();
	}

	/*
	** update 3Dfx gamma -- it is expected that a user will do a vid_restart
	** after tweaking this value
	*/
	if ( vid_gamma->modified ) {
	
		vid_gamma->modified = false;
	
		if (gl_state.hwgamma) UpdateHardwareGamma();
		
	}
	
#if defined(PARTICLESYSTEM)
	if (gl_particle_lighting->modified) {
		gl_particle_lighting->modified = false;
		if (gl_particle_lighting->value < 0)
			gl_particle_lighting->value = 0;
		if (gl_particle_lighting->value > 1)
			gl_particle_lighting->value = 1;
	}
#endif

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	qglViewport (0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglEnable (GL_ALPHA_TEST);
	qglColor4f (1,1,1,1);

	/*
	** draw buffer stuff
	*/
	if ( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !gl_state.stereo_enabled )
		{
			if ( Q_stricmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GL_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if ( gl_texturealphamode->modified )
	{
		GL_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GL_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
#if !defined(PARTICLESYSTEM)
	GL_SetTexturePalette( r_rawpalette );
#endif
	qglClearColor (0,0,0,0);
	qglClear (GL_COLOR_BUFFER_BIT);
	qglClearColor (1,0, 0.5 , 0.5);
}

/*
** R_DrawBeam
*/
#if defined(PARTICLESYSTEM)
void R_DrawBeam( entity_t *e )
{

	R_RenderBeam( e->origin, e->oldorigin, e->frame, 
		( d_8to24table[e->skinnum & 0xFF] ) & 0xFF,
		( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF,
		( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF,
		e->alpha*254 );
}

void R_RenderBeam( vec3_t start, vec3_t end, float size, float red, float green, float blue, float alpha )
{
	particle_t beam;
	vec3_t	up = {vup[0] * 0.75f, vup[1] * 0.75f, vup[2] * 0.75f};
	vec3_t	right = {vright[0] * 0.75f, vright[1] * 0.75f, vright[2] * 0.75f};

	VectorAdd      (up, right, particle_coord[0]);
	VectorSubtract (right, up, particle_coord[1]);
	VectorNegate   (particle_coord[0], particle_coord[2]);
	VectorNegate   (particle_coord[1], particle_coord[3]);

	qglEnable (GL_TEXTURE_2D);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask   (false);
	qglEnable(GL_BLEND);
	GL_ShadeModel (GL_SMOOTH);
	qglDisable(GL_ALPHA_TEST);
	ParticleOverbright = false;

	beam.alpha = alpha*DIV255*2.5;
	beam.blendfunc_src = GL_SRC_ALPHA;
	beam.blendfunc_dst = GL_ONE;
	beam.blue	= blue*255;
	beam.red	= red*255;
	beam.green	= green*255;
	beam.decal = NULL;
	beam.flags = PART_BEAM;
	beam.image = 0;
	VectorCopy(start, beam.angle);
	VectorCopy(end, beam.origin);
	beam.size = size*2;

	renderParticle (&beam);

	qglDepthRange (gldepthmin, gldepthmax);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_TexEnv( GL_MODULATE );
	qglDepthMask (true);
	qglDisable(GL_BLEND);
	qglColor4f   (1,1,1,1);
}
#else
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_BLEND );
	qglDepthMask( GL_FALSE );

	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	qglColor4f( r, g, b, e->alpha );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		qglVertex3fv( start_points[i] );
		qglVertex3fv( end_points[i] );
		qglVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		qglVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	qglEnd();

	qglEnable( GL_TEXTURE_2D );
	qglDisable( GL_BLEND );
	qglDepthMask( GL_TRUE );
}
#endif

//===================================================================


void	R_BeginRegistration (char *map);
void	R_SetSky (char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawScaledPic = Draw_ScaledPic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawStretchPic2 = Draw_StretchPic2;
	re.DrawChar = Draw_Char;
	re.DrawScaledChar = Draw_ScaledChar;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFill2 = Draw_Fill2;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	re.AppActivate = GLimp_AppActivate;
	
#if defined(PARTICLESYSTEM)
	re.SetParticlePicture = SetParticlePicture;
#endif
	re.AddDecal = R_AddDecal;
	Swap_Init ();

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

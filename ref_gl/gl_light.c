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
// r_light.c

#include "gl_local.h"

int	r_dlightframecount;

#define	DLIGHT_CUTOFF	0

int c_flares = 0;
int r_numflares;
flare_t r_flares[MAX_FLARES];

void R_RenderFlare (flare_t *light)
{
	vec3_t	v, tmp;
	int 	j, i;
	float	dist;
	unsigned	flaretex;
	
	for (i=0; i<MAX_FLARE; i++)
        	flaretex =  r_flare[i & (MAX_FLARE-1)]->texnum;

	VectorSubtract (light->origin, r_origin, v);
	dist = VectorLength(v) * (light->size*0.01);

	qglDisable(GL_DEPTH_TEST);
	qglEnable (GL_TEXTURE_2D);
	GL_Bind(flaretex);
	qglEnableClientState( GL_COLOR_ARRAY );
	GL_TexEnv( GL_MODULATE );
	
	VectorScale(light->color, 0.25f, tmp );
	
	for (j=0; j<4; j++)
		VA_SetElem4(col_array[j], tmp[0],tmp[1],tmp[2], 1);
		

	VectorMA (light->origin, -1-dist, vup, vert_array[0]);
	VectorMA (vert_array[0], 1+dist, vright, vert_array[0]);
	VA_SetElem2(tex_array[0], 0, 1);

	VectorMA (light->origin, -1-dist, vup, vert_array[1]);
	VectorMA (vert_array[1], -1-dist, vright, vert_array[1]);
	VA_SetElem2(tex_array[1], 0, 0);

	VectorMA (light->origin, 1+dist, vup, vert_array[2]);
	VectorMA (vert_array[2], -1-dist, vright, vert_array[2]);
	VA_SetElem2(tex_array[2], 1, 0);
        
	VectorMA (light->origin, 1+dist, vup, vert_array[3]);
	VectorMA (vert_array[3], 1+dist, vright, vert_array[3]);
	VA_SetElem2(tex_array[3], 1, 1);
	 
	qglDrawArrays(GL_QUADS, 0 , 4);

	GL_TexEnv( GL_REPLACE );
	qglEnable(GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);
	qglDisableClientState(GL_COLOR_ARRAY);

}

void R_RenderFlares (void)
{
	flare_t		*l;
	int i;
	
	if(!gl_lensflare->value)
		return;
                
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;
	
	qglDepthMask (0);
	qglDisable (GL_TEXTURE_2D);
	qglShadeModel (GL_SMOOTH); 
	qglEnable (GL_BLEND); 
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE);	// fog bug fix by Kirk Barnes 

	l = r_flares;
	for (i=0; i<r_numflares ; i++, l++) {
		if (ri.IsVisible(r_origin,l->origin)) {
			R_RenderFlare (l);
				c_flares++;
		}
	}
	
	qglColor3f (1,1,1);
	qglDisable (GL_BLEND); 
	qglEnable (GL_TEXTURE_2D);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}

float glowcos[17] =  
{ 
	 1.000000f, 
	 0.923880f, 
	 0.707105f, 
	 0.382680f, 
	 0.000000f, 
	-0.382680f, 
	-0.707105f, 
	-0.923880f, 
	-1.000000f, 
	-0.923880f, 
	-0.707105f, 
	-0.382680f, 
	 0.000000f, 
	 0.382680f, 
	 0.707105f, 
	 0.923880f, 
	 1.000000f 
}; 

float glowsin[17] =  
{ 
	 0.000000f, 
	 0.382680f, 
	 0.707105f, 
	 0.923880f, 
	 1.000000f, 
	 0.923880f, 
	 0.707105f, 
	 0.382680f, 
	-0.000000f, 
	-0.382680f, 
	-0.707105f, 
	-0.923880f, 
	-1.000000f, 
	-0.923880f, 
	-0.707105f, 
	-0.382680f, 
	 0.000000f 
}; 

void V_AddBlend(float r, float g, float b, float intensity, float *blend)
{
	blend[0] += r * intensity;
	blend[1] += g * intensity;
	blend[2] += b * intensity;

	if(blend[0] > 1.f) blend[0] = 1.f;
	if(blend[1] > 1.f) blend[1] = 1.f;
	if(blend[2] > 1.f) blend[2] = 1.f;

	if(!blend[3])
	blend[3] = intensity;
}

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void R_RenderDlight (dlight_t *light)
{
	int		i, j;
	vec3_t	v;
	float	rad;

	rad = light->intensity * 0.35;

	VectorSubtract (light->origin, r_origin, v);
	
	if (VectorLength (v) < rad) {
		extern float v_blend[4];
		// view is inside the dlight
		V_AddBlend (light->color[0], light->color[1], light->color[2], 0.15, v_blend);
		return;
	}

	qglBegin (GL_TRIANGLE_FAN);
	qglColor3f (light->color[0]*0.2, light->color[1]*0.2, light->color[2]*0.2);
	for (i=0 ; i<3 ; i++)
		v[i] = light->origin[i] - vpn[i]*rad;
	qglVertex3fv (v);
	qglColor3f (0,0,0);
	for (i=16 ; i>=0 ; i--)
	{
		for (j=0 ; j<3 ; j++)
			v[j] = light->origin[j] + vright[j]*glowcos[i]*rad
				 + vup[j]*glowsin[i]*rad;
		qglVertex3fv (v);
	}
	qglEnd ();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights (void)
{
	int		i;
	dlight_t	*l;

	if (!gl_flashblend->value)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
						//  advanced yet for this frame
	qglDepthMask (GL_FALSE);
	qglDisable (GL_TEXTURE_2D);
	qglShadeModel (GL_SMOOTH);
	qglEnable (GL_BLEND);
	qglBlendFunc (GL_ONE, GL_ONE);

	l = r_newrefdef.dlights;
	for (i=0 ; i<r_newrefdef.num_dlights ; i++, l++)
		R_RenderDlight (l);

	qglColor3f (1,1,1);
	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (GL_TRUE);
}


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	cplane_t	*splitplane;
	float		dist, l, maxdist;
	msurface_t	*surf;
	int		i, j, s, t;
	vec3_t		impact; 
	
loc0:

	if (node->contents != CONTENTS_NODE)
		return;

	splitplane = node->plane;
	
	if ( splitplane->type < 3 )
		dist = light->origin[splitplane->type] - splitplane->dist;
	else 
		dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;
	
	if (dist > light->intensity-DLIGHT_CUTOFF) { 
		node = node->children[0]; 
		goto loc0; 
	}
	
	if (dist < -light->intensity+DLIGHT_CUTOFF) {
		node = node->children[1]; 
		goto loc0; 
	}
	maxdist = (light->intensity-DLIGHT_CUTOFF)*(light->intensity-DLIGHT_CUTOFF);
	
	// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;

	for (i=0 ; i<node->numsurfaces ; i++, surf++) { 
		// LordHavoc: MAJOR dynamic light speedup here,
		// eliminates marking of surfaces that are too far away from light,
		// thus preventing unnecessary renders and uploads.
 
		for (j=0 ; j<3 ; j++) 
			impact[j] = light->origin[j] - surf->plane->normal[j]*dist; 
 
		// clamp center of light to corner and check brightness 
		l = DotProduct (impact, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3] - surf->texturemins[0];

		s = l+0.5;

		if (s < 0)
			s = 0;
		else if (s > surf->extents[0])
			s = surf->extents[0]; 

		s = l - s; 

		l = DotProduct (impact, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3] - surf->texturemins[1];
 
		t = l + 0.5;

		if (t < 0)
			t = 0;
		else if (t > surf->extents[1])
			t = surf->extents[1];
 
		t = l - t;
 
		// compare to minimum light 

		if ((s*s+t*t+dist*dist) < maxdist)  { 
			if (surf->dlightframe != r_dlightframecount) {
			        // not dynamic until now 
				surf->dlightbits = bit; 
				surf->dlightframe = r_dlightframecount; 
			} 
			else // already dynamic 
				surf->dlightbits |= bit; 
		} 
	}

	if (node->children[0]->contents == CONTENTS_NODE) { 
		if (node->children[1]->contents == -1)  { 
			R_MarkLights (light, bit, node->children[0]); 
			node = node->children[1]; 
			goto loc0; 
		} 
		else  { 
			node = node->children[0]; 
			goto loc0; 
		} 
	} 
	else if (node->children[1]->contents == CONTENTS_NODE)  { 
		node = node->children[1]; 
		goto loc0; 
	} 
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	if (gl_flashblend->value)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = r_newrefdef.dlights;
	for (i=0 ; i<r_newrefdef.num_dlights ; i++, l++)
		R_MarkLights ( l, 1<<i, r_worldmodel->nodes );
}
/*
=============
R_PushDlightsForBModel
=============
*/
void R_PushDlightsForBModel (entity_t *e)
{
	int			k;
	dlight_t	*lt;

	lt = r_newrefdef.dlights;
	
	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		vec3_t	temp;
		vec3_t	forward, right, up;
		
		AngleVectors (e->angles, forward, right, up);
		
		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			VectorSubtract (lt->origin, e->origin, temp);
			lt->origin[0] = DotProduct (temp, forward);
			lt->origin[1] = -DotProduct (temp, right);
			lt->origin[2] = DotProduct (temp, up);
			R_MarkLights (lt, 1<<k, e->model->nodes + e->model->firstnode);
			VectorAdd (temp, e->origin, lt->origin);
		}
	} 
	else
	{
		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			VectorSubtract (lt->origin, e->origin, lt->origin);
			R_MarkLights (lt, 1<<k, e->model->nodes + e->model->firstnode);
			VectorAdd (lt->origin, e->origin, lt->origin);
		}
	}
}



/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t			pointcolor;
cplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			ds, dt;
	int			i;
	mtexinfo_t	*tex;
	int			r;

	if (node->contents != CONTENTS_NODE)
		return -1;		// didn't hit anything
	
// calculate mid point

	plane = node->plane;
	if (plane->type < 3)
	{
		front = start[plane->type] - plane->dist;
		back = end[plane->type] - plane->dist;
	}
	else
	{
		front = DotProduct (start, plane->normal) - plane->dist;
		back = DotProduct (end, plane->normal) - plane->dist;
	}

	side = front < 0;
	
	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side	
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something
		
	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing
		
// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & (SURF_DRAWTURB|SURF_DRAWSKY)) 
			continue;	// no lightmaps

		tex = surf->texinfo;
		
		ds = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		if (ds < 0 || ds > surf->extents[0])
			continue;

		dt = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];
		if (dt < 0 || dt > surf->extents[1])
			continue;

		if (surf->samples)
		{
			vec3_t	scale;
			byte	*lightmap;
			int		maps;

			lightmap = surf->samples + 3*((dt>>4) * ((surf->extents[0]>>4)+1) + (ds>>4));
			VectorClear (pointcolor);

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
			{
				for (i=0 ; i<3 ; i++)
					scale[i] = (1.0/255)*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

				pointcolor[0] += lightmap[0] * scale[0];
				pointcolor[1] += lightmap[1] * scale[1];
				pointcolor[2] += lightmap[2] * scale[2];
				lightmap += 3*((surf->extents[0]>>4)+1)*((surf->extents[1]>>4)+1);
			}

			return 1;
		}
		
		return 0;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic)
{
	vec3_t		end;
	float		r;
	int			lnum;
	dlight_t	*dl;
	vec3_t		dist, dlightcolor;
	float		add;
	
	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}
	
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;
	
	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);
	if (r == -1)
		VectorClear (color);
	else
	{
		// VectorCopy (pointcolor, color);
		// jitlight - adjust lightmap saturation for point entities
		register float	r, g, b, a, v;


		r = pointcolor[0];
		g = pointcolor[1];
		b = pointcolor[2];
		a = r * 0.33 + g * 0.34 + b * 0.33;	// greyscale value
		v = gl_lightmap_saturation->value;

		color[0] = r * v + a * (1.0 - v);
		color[1] = g * v + a * (1.0 - v);
		color[2] = b * v + a * (1.0 - v);
	}
	
	if (!addDynamic)
		return;

	//
	// add dynamic lights
	//
	dl = r_newrefdef.dlights;
	VectorClear(dlightcolor);
	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
	{
		VectorSubtract (currententity->origin,
						dl->origin,
						dist);
		add = dl->intensity - VectorLength(dist);
		if (add > 0)
		{
			add *= (1.0/256);
			VectorMA (dlightcolor, add, dl->color, dlightcolor);
		}
	}

	VectorMA(color, gl_modulate->value, dlightcolor, color);
}


//===================================================================

static float s_blocklights[34*34*3];
/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		fdist, frad, fminlight;
	vec3_t		impact, local, dlorigin;
	int			s, t;
	int			smax, tmax;
	mtexinfo_t	*tex;
	dlight_t	*dl;
	float		*pfBL;
	float		fsacc, ftacc;
	qboolean	rotated = false;
	vec3_t		temp;
	vec3_t		forward, right, up;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;


	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		rotated = true;
		AngleVectors (currententity->angles, forward, right, up);
	}

	dl = r_newrefdef.dlights;
	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		frad = dl->intensity;
		VectorCopy(dl->origin, dlorigin);
		VectorSubtract (dlorigin, currententity->origin, dlorigin);
		if (rotated)
		{
			VectorCopy (dlorigin, temp);
			dlorigin[0] = DotProduct (temp, forward);
			dlorigin[1] = -DotProduct (temp, right);
			dlorigin[2] = DotProduct (temp, up);
		}
		if (surf->plane->type < 3)
			fdist = dlorigin[surf->plane->type] - surf->plane->dist;
		else
			fdist = DotProduct (dl->origin, surf->plane->normal) - surf->plane->dist;
			
		frad -= fabs(fdist);
		// rad is now the highest intensity on the plane

		fminlight = DLIGHT_CUTOFF;	// FIXME: make configurable?
		if (frad < fminlight)
			continue;
		fminlight = frad - fminlight;

		if (surf->plane->type < 3)
		{
			VectorCopy (dlorigin, impact);
			impact[surf->plane->type] -= fdist;
		}
		else 
		{
			VectorMA (dlorigin, -fdist, surf->plane->normal, impact);
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

		pfBL = s_blocklights;
		for (t = 0, ftacc = 0 ; t<tmax ; t++, ftacc += 16)
		{
			td = local[1] - ftacc;
			if ( td < 0 )
				td = -td;

			for ( s=0, fsacc = 0 ; s<smax ; s++, fsacc += 16, pfBL += 3)
			{
				sd = Q_ftol( local[0] - fsacc );

				if ( sd < 0 )
					sd = -sd;

				if (sd > td)
					fdist = sd + (td>>1);
				else
					fdist = td + (sd>>1);

				if ( fdist < fminlight )
				{
					pfBL[0] += ( fminlight - fdist ) * dl->color[0];
					pfBL[1] += ( fminlight - fdist ) * dl->color[1];
					pfBL[2] += ( fminlight - fdist ) * dl->color[2];
				}
			}
		}
	}
}


/*
** R_SetCacheState
*/
void R_SetCacheState( msurface_t *surf )
{
	int maps;

	for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
		 maps++)
	{
		surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	int			r, g, b, a, max;
	int			i, j, size;
	byte		*lightmap;
	float		scale[4];
	int			nummaps;
	float		*bl;
	lightstyle_t	*style;
	int monolightmap;
	float		sat;	// jitlight

	if ( surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) )
		ri.Sys_Error (ERR_DROP, "R_BuildLightMap called for non-lit surface");

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	if (size > (sizeof(s_blocklights)>>4) )
		ri.Sys_Error (ERR_DROP, "Bad s_blocklights size");

	sat = gl_lightmap_saturation->value;

	if (sat < 0.0)
		sat = 0.0;

	if (sat > 1.0)
		sat = 1.0;
		
// set to full bright if no light data
	if (!surf->samples)
	{
		int maps;

		for (i=0 ; i<size*3 ; i++)
			s_blocklights[i] = 255;
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			style = &r_newrefdef.lightstyles[surf->styles[maps]];
		}
		goto store;
	}
#if 0
	// count the # of maps
	for ( nummaps = 0 ; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255 ;
		 nummaps++)
		;
#else
	nummaps = 0;
	while (nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255)
		nummaps++;

#endif
	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * 3 );

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (surf);

// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;

	monolightmap = gl_monolightmap->string[0];

	if ( monolightmap == '0' )
	{
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
				
				r = Q_ftol( bl[0] );
				g = Q_ftol( bl[1] );
				b = Q_ftol( bl[2] );

				// catch negative lights
				if (r < 0)
					r = 0;
				if (g < 0)
					g = 0;
				if (b < 0)
					b = 0;

				/*
				** determine the brightest of the three color components
				*/
				if (r > g)
					max = r;
				else
					max = g;
				if (b > max)
					max = b;

				/* 
				** jitlight -- reduce oversaturation:
				** greyscale value: 
				*/
				a = (int)((float)r * 0.33 + (float)g * 0.34 + (float)b * 0.33);

				r = r * sat + a * (1.0 - sat);
				g = g * sat + a * (1.0 - sat);
				b = b * sat + a * (1.0 - sat);

				/*
				** alpha is ONLY used for the mono lightmap case.  For this reason
				** we set it to the brightest of the color components so that 
				** things don't get too dim.
				*/
				a = max;

				/*
				** rescale all the color components if the intensity of the greatest
				** channel exceeds 1.0
				*/
				if (max > 255)
				{
					float t = 255.0F / max;

					r = r*t;
					g = g*t;
					b = b*t;
					a = a*t;
				}

				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
				dest[3] = a;

				bl += 3;
				dest += 4;
			}
		}
	}
	else
	{
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
				
				r = Q_ftol( bl[0] );
				g = Q_ftol( bl[1] );
				b = Q_ftol( bl[2] );

				// catch negative lights
				if (r < 0)
					r = 0;
				if (g < 0)
					g = 0;
				if (b < 0)
					b = 0;

				/*
				** determine the brightest of the three color components
				*/
				if (r > g)
					max = r;
				else
					max = g;
				if (b > max)
					max = b;

				/*
				** alpha is ONLY used for the mono lightmap case.  For this reason
				** we set it to the brightest of the color components so that 
				** things don't get too dim.
				*/
				a = max;

				/*
				** rescale all the color components if the intensity of the greatest
				** channel exceeds 1.0
				*/
				if (max > 255)
				{
					float t = 255.0F / max;

					r = r*t;
					g = g*t;
					b = b*t;
					a = a*t;
				}

				/*
				** So if we are doing alpha lightmaps we need to set the R, G, and B
				** components to 0 and we need to set alpha to 1-alpha.
				*/
				switch ( monolightmap )
				{
				case 'L':
				case 'I':
					r = a;
					g = b = 0;
					break;
				case 'C':
					// try faking colored lighting
					a = 255 - ((r+g+b)*0.3333333);
					r *= a*0.003921568627450980392156862745098;
					g *= a*0.003921568627450980392156862745098;
					b *= a*0.003921568627450980392156862745098;
					break;
				case 'A':
				default:
					r = g = b = 0;
					a = 255 - a;
					break;
				}

				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
				dest[3] = a;

				bl += 3;
				dest += 4;
			}
		}
	}
}






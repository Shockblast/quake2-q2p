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
// cl_fx.c -- entity effects parsing and management

#include "client.h"

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

vec3_t		avelocities[NUMVERTEXNORMALS];
cparticle_t	*active_particles, *free_particles;
cparticle_t	particles[MAX_PARTICLES];
int		cl_numparticles = MAX_PARTICLES;

#if defined(PARTICLESYSTEM)
const byte default_pal[768] =
{
0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,
155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,99,75,35,91,67,31,83,63,31,79,59,27,71,55,
27,63,47,23,59,43,23,51,39,19,47,35,19,43,31,19,39,27,15,35,23,15,27,19,11,23,15,11,19,15,7,15,11,7,95,95,
111,91,91,103,91,83,95,87,79,91,83,75,83,79,71,75,71,63,67,63,59,59,59,55,55,51,47,47,47,43,43,39,39,39,35,
35,35,27,27,27,23,23,23,19,19,19,143,119,83,123,99,67,115,91,59,103,79,47,207,151,75,167,123,59,139,103,47,
111,83,39,235,159,39,203,139,35,175,119,31,147,99,27,119,79,23,91,59,15,63,39,11,35,23,7,167,59,43,159,47,
35,151,43,27,139,39,19,127,31,15,115,23,11,103,23,7,87,19,0,75,15,0,67,15,0,59,15,0,51,11,0,43,11,0,35,11,
0,27,7,0,19,7,0,123,95,75,115,87,67,107,83,63,103,79,59,95,71,55,87,67,51,83,63,47,75,55,43,67,51,39,63,47,
35,55,39,27,47,35,23,39,27,19,31,23,15,23,15,11,15,11,7,111,59,23,95,55,23,83,47,23,67,43,23,55,35,19,39,
27,15,27,19,11,15,11,7,179,91,79,191,123,111,203,155,147,215,187,183,203,215,223,179,199,211,159,183,195,
135,167,183,115,151,167,91,135,155,71,119,139,47,103,127,23,83,111,19,75,103,15,67,91,11,63,83,7,55,75,7,47,
63,7,39,51,0,31,43,0,23,31,0,15,19,0,7,11,0,0,0,139,87,87,131,79,79,123,71,71,115,67,67,107,59,59,99,51,51,
91,47,47,87,43,43,75,35,35,63,31,31,51,27,27,43,19,19,31,15,15,19,11,11,11,7,7,0,0,0,151,159,123,143,151,115,
135,139,107,127,131,99,119,123,95,115,115,87,107,107,79,99,99,71,91,91,67,79,79,59,67,67,51,55,55,43,47,47,
35,35,35,27,23,23,19,15,15,11,159,75,63,147,67,55,139,59,47,127,55,39,119,47,35,107,43,27,99,35,23,87,31,19,
79,27,15,67,23,11,55,19,11,43,15,7,31,11,7,23,7,0,11,0,0,0,0,0,119,123,207,111,115,195,103,107,183,99,99,167,
91,91,155,83,87,143,75,79,127,71,71,115,63,63,103,55,55,87,47,47,75,39,39,63,35,31,47,27,23,35,19,15,23,11,7,
7,155,171,123,143,159,111,135,151,99,123,139,87,115,131,75,103,119,67,95,111,59,87,103,51,75,91,39,63,79,27,
55,67,19,47,59,11,35,47,7,27,35,0,19,23,0,11,15,0,0,255,0,35,231,15,63,211,27,83,187,39,95,167,47,95,143,51,
95,123,51,255,255,255,255,255,211,255,255,167,255,255,127,255,255,83,255,255,39,255,235,31,255,215,23,255,
191,15,255,171,7,255,147,0,239,127,0,227,107,0,211,87,0,199,71,0,183,59,0,171,43,0,155,31,0,143,23,0,127,15,
0,115,7,0,95,0,0,71,0,0,47,0,0,27,0,0,239,0,0,55,55,255,255,0,0,0,0,255,43,43,35,27,27,23,19,19,15,235,151,
127,195,115,83,159,87,51,123,63,27,235,211,199,199,171,155,167,139,119,135,107,87,159,91,83
};

int color8red(int color8)   { return (default_pal[color8*3+0]); }
int color8green(int color8) { return (default_pal[color8*3+1]); }
int color8blue(int color8)  { return (default_pal[color8*3+2]); }

void SetParticleImages(void)
{
	re.SetParticlePicture(particle_generic,       "gfx/basic.pcx");
	re.SetParticlePicture(particle_smoke,         "gfx/smoke.pcx");
	re.SetParticlePicture(particle_splash,        "gfx/splash.pcx");
	re.SetParticlePicture(particle_wave,          "gfx/wave.pcx");
	re.SetParticlePicture(particle_blood,         "gfx/blood.pcx");
	re.SetParticlePicture(particle_bloodred,      "gfx/blood_red.pcx");
	re.SetParticlePicture(particle_blooddrip,     "gfx/blood_drip.pcx");
	re.SetParticlePicture(particle_lensflare,     "gfx/lensflare.pcx");
	re.SetParticlePicture(particle_inferno,       "gfx/inferno.pcx");
	re.SetParticlePicture(particle_bulletmark,    "gfx/decal_bullet.pcx");
	re.SetParticlePicture(particle_impact,        "gfx/impact.pcx");
	re.SetParticlePicture(particle_impactwater,   "gfx/impact_water.pcx");
	re.SetParticlePicture(particle_impactblaster, "gfx/decal_blaster.pcx");
	re.SetParticlePicture(particle_railhit,       "gfx/railhit.pcx");
	re.SetParticlePicture(particle_footprint,     "gfx/footprint.pcx");
	re.SetParticlePicture(particle_blaster,       "gfx/blaster.pcx");
	re.SetParticlePicture(particle_beam,          "gfx/beam.pcx");
	re.SetParticlePicture(particle_lightning,     "gfx/lightning.pcx");
	re.SetParticlePicture(particle_lightflare,    "gfx/lightflare.pcx");
	re.SetParticlePicture(particle_bubble,        "gfx/bubble.pcx");
	re.SetParticlePicture(particle_bubblesplash,  "gfx/bubble_splash.pcx");
	re.SetParticlePicture(particle_waterexplode,  "gfx/water_explode.pcx");
	re.SetParticlePicture(particle_fly1,          "gfx/fly_1.pcx");
	re.SetParticlePicture(particle_fly2,          "gfx/fly_2.pcx");
	re.SetParticlePicture(particle_fly3,          "gfx/fly_3.pcx");
	re.SetParticlePicture(particle_fly4,          "gfx/fly_4.pcx");
	re.SetParticlePicture(particle_bloodred1,     "gfx/decal_blood1.pcx");
	re.SetParticlePicture(particle_bloodred2,     "gfx/decal_blood2.pcx");
	re.SetParticlePicture(particle_bloodred3,     "gfx/decal_blood3.pcx");
	re.SetParticlePicture(particle_bloodred4,     "gfx/decal_blood4.pcx");
	re.SetParticlePicture(particle_bloodred5,     "gfx/decal_blood5.pcx");
	re.SetParticlePicture(particle_rexplosion1,   "gfx/r_explod_1.pcx");
	re.SetParticlePicture(particle_rexplosion2,   "gfx/r_explod_2.pcx");
	re.SetParticlePicture(particle_rexplosion3,   "gfx/r_explod_3.pcx");
	re.SetParticlePicture(particle_rexplosion4,   "gfx/r_explod_4.pcx");
	re.SetParticlePicture(particle_rexplosion5,   "gfx/r_explod_5.pcx");
	re.SetParticlePicture(particle_rexplosion6,   "gfx/r_explod_6.pcx");
	re.SetParticlePicture(particle_rexplosion7,   "gfx/r_explod_7.pcx");
	re.SetParticlePicture(particle_dexplosion1,   "gfx/d_explod_1.pcx");
	re.SetParticlePicture(particle_dexplosion2,   "gfx/d_explod_2.pcx");
	re.SetParticlePicture(particle_dexplosion3,   "gfx/d_explod_3.pcx");
}

int particleBlood(void) { return particle_bloodred1 + rand()%5; }

void addParticleLight (cparticle_t *p, float light, float lightvel, float lcol0, float lcol1, float lcol2)
{
	int i;

	for (i=0; i<P_LIGHTS_MAX; i++) {
		cplight_t *plight = &p->lights[i];
		if (!plight->isactive) {
			plight->isactive = true;
			plight->light = light;
			plight->lightvel = lightvel;
			plight->lightcol[0] = lcol0;
			plight->lightcol[1] = lcol1;
			plight->lightcol[2] = lcol2;
			return;
		}
	}
}
#endif


/*
===============
CL_ClearParticles
===============
*/
void CL_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<cl_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[cl_numparticles-1].next = NULL;
}

#if defined(PARTICLESYSTEM)

qboolean runningParticle = false;
cparticle_t *newParticles = NULL;

cparticle_t *setupParticle(float angle0, float angle1, float angle2,
                           float org0, float org1, float org2,
			   float vel0, float vel1, float vel2,
			   float accel0, float accel1, float accel2,
			   float color0, float color1, float color2,
			   float colorvel0, float colorvel1, float colorvel2,
			   float alpha, float alphavel,
			   int blendfunc_src, int blendfunc_dst,
			   float size, float sizevel,
			   int image, int flags,
			   void (*think)(cparticle_t *p, vec3_t org, vec3_t angle,
			                 float *alpha, float *size, int *image, float *time),
			   qboolean thinknext)
{
	int j;
	cparticle_t	*p = NULL;

	if (!free_particles)
		return NULL;
	p = free_particles;
	free_particles = p->next;
	
	if (runningParticle) {
		if (newParticles) {
        		p->next = newParticles;
			newParticles = p;
		}
		else {
			p->next = active_particles;
			newParticles = p;
		}
	}
	else {
		p->next = active_particles;
		active_particles = p;
	}

	p->start = p->ptime = cl.stime;

	p->angle[0]=angle0;
	p->angle[1]=angle1;
	p->angle[2]=angle2;

	p->org[0]=org0;
	p->org[1]=org1;
	p->org[2]=org2;
	p->oldorg[0]=org0;
	p->oldorg[1]=org1;
	p->oldorg[2]=org2;

	p->vel[0]=vel0;
	p->vel[1]=vel1;
	p->vel[2]=vel2;

	p->accel[0]=accel0;
	p->accel[1]=accel1;
	p->accel[2]=accel2;

	p->color[0]=color0;
	p->color[1]=color1;
	p->color[2]=color2;

	p->colorvel[0]=colorvel0;
	p->colorvel[1]=colorvel1;
	p->colorvel[2]=colorvel2;

	p->blendfunc_src = blendfunc_src;
	p->blendfunc_dst = blendfunc_dst;

	p->alpha=alpha;
	p->alphavel=alphavel;
	p->size=size;
	p->sizevel=sizevel;

	p->image=image;
	p->flags=flags;

	p->src_ent=0;
	p->dst_ent=0;

	if (think)
		p->think=think;
	else
		p->think=NULL;
	p->thinknext=thinknext;

	for (j=0;j<P_LIGHTS_MAX;j++) {
		cplight_t *plight = &p->lights[j];
		plight->isactive = false;
		plight->light = 0;
		plight->lightvel = 0;
		plight->lightcol[0] = 0;
		plight->lightcol[1] = 0;
		plight->lightcol[2] = 0;
	}

	p->decalnum = 0;

	if (flags & PART_DECAL) {
		vec3_t dir;

		if (VectorCompare (p->angle, vec3_origin)) { //area decals w/o normal
			VectorCopy(vec3_origin, dir);
		}
		else {
			AngleVectors (p->angle, dir, NULL, NULL);
			VectorNegate(dir, dir);
		}

		clipDecal(p, p->size, -p->angle[2], p->org, dir);

		if (!p->decalnum) //kill on viewframe
			p->alpha = 0;
	}

	return p;
}

void clipDecal (cparticle_t *part, float radius, float orient, vec3_t origin, vec3_t dir)
{
	vec3_t	axis[3], verts[MAX_DECAL_VERTS];
	vec2_t	tcoords[MAX_DECAL_VERTS];
	int		numfragments, j, i;
	markFragment_t *fr, fragments[MAX_DECAL_FRAGMENTS];
	
	// invalid decal
	if ( radius <= 0 ) 
		return;

	if (VectorCompare (dir, vec3_origin)) {
		VectorCopy(vec3_origin, axis[0]);
		VectorCopy(vec3_origin, axis[1]);
		VectorCopy(vec3_origin, axis[2]);
	}
	else {
		// calculate orientation matrix
		VectorNormalize2 ( dir, axis[0] );
		PerpendicularVector ( axis[1], axis[0] );
		RotatePointAroundVector ( axis[2], axis[0], axis[1], orient );
		CrossProduct ( axis[0], axis[2], axis[1] );
	}

	numfragments = re.AddDecal (origin, axis, radius, MAX_DECAL_VERTS, verts, tcoords,
		MAX_DECAL_FRAGMENTS, fragments);

	if (!numfragments)
		return;

	part->decalnum = numfragments;
	for ( i = 0, fr = fragments; i < numfragments; i++, fr++ ) {
		decalpolys_t *decal = &part->decal[i];
		for ( j = 0; j < fr->numPoints; j++ ) {
			VectorCopy ( verts[fr->firstPoint+j], decal->polys[j] );
			decal->coords[j][0] = tcoords[fr->firstPoint+j][0];
			decal->coords[j][1] = tcoords[fr->firstPoint+j][1];
		}
		decal->numpolys = fr->numPoints;
	}
}

void CL_ImpactMark (vec3_t pos, vec3_t dir, float size, int flags, int image)
{
	setupParticle(dir[0], dir[1], dir[2],
		      pos[0], pos[1], pos[2],
		      0, 0, 0,
		      0, 0, 0,
		      255, 255, 255,
		      0, 0, 0,
		      1, -0.01,
		      GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
		      size, 0,
		      image, PART_DECAL|PART_DECAL_SUB,
		      pDecalAlphaThink, true);
}

void CL_ForceTrail (vec3_t start, vec3_t end, qboolean light, float size)
{
	cparticle_t *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, length, frac;
	int		i=0;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	length = len = VectorNormalize (vec);

	dec = 1 + size/5;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec; i++;
		frac = len/length;

		if (light)
			p = setupParticle(random()*360, random()*15, 0,
				          move[0], move[1], move[2],
				          0, 0, 0,
				          0, 0, 0,
				          150, 200, 255,
				          0, 0, 0,
				          1, -2.0 / (0.8+frand()*0.2),
				          GL_SRC_ALPHA, GL_ONE,
				          size, size,
				          particle_smoke, 0,
				          pRotateThink,true);
		else
			p = setupParticle(random()*360, random()*15, 0,
				          move[0], move[1], move[2],
				          0, 0, 0,
				          0, 0, 0,
				          255, 255, 255,
				          0, 0, 0,
				          1, -2.0 / (0.8+frand()*0.2),
				          GL_SRC_ALPHA, GL_ONE,
				          size, size,
				          particle_inferno, 0,
				          pRotateThink,true);

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_FlameTrail -- DDAY SPECIFIC STUFF...
===============
*/

void CL_SmokeTrail (vec3_t start, vec3_t end)
{
	cparticle_t *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, length, frac;
	int			i=0;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	length = len = VectorNormalize (vec);

	dec = 1;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec; i++;
		frac = len/length;

		p = setupParticle(random()*360, random()*15, 0,
			          move[0]+crandom()*5.0*frac, move[1]+crandom()*5.0*frac, move[2]+crandom()*5.0*frac,
			          0, 0, 0,
			          0, 0, 0,
			          100, 100, 100,
			          0, 0, 0,
			          frac*.25 + .25, -1.0 / (0.8+frand()*0.2),
			          GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			          2+(4 + random()*2.5)*frac, crandom()*2.0,
			          particle_smoke, PART_SHADED|PART_OVERBRIGHT,
			          pRotateThink,true);

		VectorAdd (move, vec, move);
	}
}

void CL_BlueFlameTrail (vec3_t start, vec3_t end)
{
	cparticle_t *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, length, frac;
	int			i=0;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	length = len = VectorNormalize (vec);

	dec = 1;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec; i++;
		frac = len/length;

		p = setupParticle(random()*360, random()*15, 0,
			          move[0], move[1], move[2],
			          0, 0, 0,
			          0, 0, 0,
			          40+215*frac, 100, 255 - 215*frac,
			          0, 0, 0,
			          1, -2.0 / (0.8+frand()*0.2),
			          GL_SRC_ALPHA, GL_ONE,
			          2.5 + 2.5*frac, 0,
			          particle_smoke, PART_GLARE,
			          pRotateThink,true);

		VectorAdd (move, vec, move);
	}
}

void CL_InfernoTrail (vec3_t start, vec3_t end, float size)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, size2 = size * size;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = (20.0*size2+1);
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		setupParticle(random()*360, random()*45, 0,
			      move[0], move[1], move[2],
			      crandom()*size*50, crandom()*size*50, crandom()*size*50,
			      0, 0, 0,
			      255, 200, 100,
			      0, -100, -200,
			      1, -2.0 / (0.8+frand()*0.2),
			      GL_SRC_ALPHA, GL_ONE,
			      5+10*size, 10 + size*50,
			      particle_inferno, PART_GLARE,
			      pRotateThink,true);

		VectorAdd (move, vec, move);
	}
}

void CL_FlameTrail (vec3_t start, vec3_t end, float size, float grow, qboolean light)
{
	cparticle_t *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec;
	int		i=0;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = (0.75*size);
	VectorScale (vec, dec, vec);

	if (light)
		V_AddLight (start, 50+(size/10.0)*75, 0.5+random()*0.5, 0.5, 0.1);

	while (len > 0)
	{
		len -= dec; i++;

		p = setupParticle(random()*360, random()*15, 0,
			          move[0] + crand()*3, move[1] + crand()*3, move[2] + crand()*3,
			          crand()*size, crand()*size, crand()*size,
			          0, 0, 0,
			          255, 255, 255,
			          0, 0, 0,
			          1, -2.0 / (0.8+frand()*0.2),
			          GL_SRC_ALPHA, GL_ONE,
			          size, grow,
			          particle_inferno, PART_GLARE,
			          pRotateThink,true);

		VectorAdd (move, vec, move);
	}
}

void CL_GloomFlameTrail(vec3_t start, vec3_t end, float size, float grow, qboolean light)
{
	cparticle_t *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec;
	int		i = 0;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = (0.75 * size);
	VectorScale(vec, dec, vec);

	if (light)
		V_AddLight(start, 50 + (size / 10.0) * 75, 0.5 + random() * 0.5, 0.5, 0.1);

	while (len > 0) {
		len -= dec;
		i++;


		p = setupParticle(random()*360, random()*15, 0,
			          move[0] + crand()*3, move[1] + crand()*3, move[2] + crand()*3,
			          crand()*size, crand()*size, crand()*size,
			          0, 0, 0,
			          50, 235, 50,
			          0, 0, 0,
			          1, -2.0 / (0.8+frand()*0.2),
			          GL_SRC_ALPHA, GL_ONE,
			          size, grow,
			          particle_inferno, PART_GLARE,
			          pRotateThink,true);

		VectorAdd(move, vec, move);
	}
}

void CL_Flame (vec3_t start, qboolean light)
{
	cparticle_t *p;

	p = setupParticle(random()*360,	random()*15, 0,
		          start[0], start[1], start[2],
		          crand()*10.0, crand()*10.0, random()*100.0,
		          0, 0, 0,
		          255, 255, 255,
		          0, 0, 0,
		          1, -2.0 / (0.8+frand()*0.2),
		          GL_SRC_ALPHA, GL_ONE,
		          10, -10,
		          particle_inferno, PART_GLARE,
		          pRotateThink,true);

	if (light) {
		if (p)
			addParticleLight(p, 20 + random()*20.0, 0, 0.5+random()*0.5, 0.5, 0.1); //weak big
		if (p)
			addParticleLight (p, 250.0, 0, 0.01, 0.01, 0.01);
		}
}

void CL_GloomFlame(vec3_t start, qboolean light)
{
	cparticle_t    *p;

	p = setupParticle(random()*360,	random()*15, 0,
		          start[0], start[1], start[2],
		          crand()*10.0, crand()*10.0, random()*100.0,
		          0, 0, 0,
		          255, 255, 255,
		          0, 0, 0,
		          1, -2.0 / (0.8+frand()*0.2),
		          GL_SRC_ALPHA, GL_ONE,
		          50, -10,
		          particle_inferno, PART_GLARE,
		          pRotateThink,true);

	if (light) {
		if (p)
			addParticleLight(p, 20 + random() * 20.0, 0, 0.5 + random() * 0.5, 0.5, 0.1);	/* weak big */
		if (p)
			addParticleLight(p, 250.0, 0, 0.01, 0.01, 0.01);
	}
}


/*
===============
MOD SPECIFIC STUFF...
===============
*/

void CL_Tracer (vec3_t origin, vec3_t angle, int red, int green, int blue, float len, float size)
{
	vec3_t		dir;
	
	AngleVectors (angle, dir, NULL, NULL);
	VectorScale(dir, len,dir);

	setupParticle(dir[0], dir[1], dir[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      red, green, blue,
		      0, 0, 0,
		      1, 0,
		      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		      size, 0,
		      particle_generic, PART_DIRECTION|PART_INSTANT|PART_GLARE,
		      NULL,0);
}

void CL_BlasterTracer (vec3_t origin, vec3_t angle, int red, int green, int blue, float len, float size)
{
	int i;
	vec3_t		dir;
	
	AngleVectors (angle, dir, NULL, NULL);
	VectorScale(dir, len,dir);

	for (i=0;i<3;i++)
		setupParticle(dir[0], dir[1], dir[2],
		              origin[0], origin[1], origin[2],
		              0, 0, 0,
		              0, 0, 0,
		              red, green, blue,
		              0, 0, 0,
		              1, 0,
			      GL_SRC_ALPHA, GL_ONE,
			      size, 0,
			      particle_generic, PART_DIRECTION|PART_INSTANT|PART_GLARE,
			      NULL,0);
}

void CL_BlasterSplash (vec3_t origin, int red, int green, int blue, float size)
{
	int i;
	for (i=0;i<16;i++)
		setupParticle(origin[0], origin[1], origin[2],
			      origin[0] + crandom()*size, origin[1] + crandom()*size, origin[2] + crandom()*size,
			      0, 0, 0,
			      0, 0, 0,
			      red, green, blue,
			      0, 0, 0,
			      1, 0,
			      GL_SRC_ALPHA, GL_ONE,
			      size*0.5f, 0,
			      particle_generic, PART_BEAM|PART_INSTANT|PART_GLARE,
			      NULL,0);
}

/*
===============
CL_LightningBeam
===============
*/
void CL_LightningBeam(vec3_t start, vec3_t end, int srcEnt, int dstEnt, float size)
{
	cparticle_t *list;
	cparticle_t *p=NULL;

	for (list=active_particles ; list ; list=list->next)
		if (list->src_ent == srcEnt && list->dst_ent == dstEnt && list->image == particle_lightning) {
			p=list;
			/*p->start =*/ p->ptime = cl.stime;
			VectorCopy(start, p->angle);
			VectorCopy(end, p->org);

			return;
		}

	p = setupParticle(start[0], start[1], start[2],
		          end[0], end[1], end[2],
		          0, 0, 0,
		          0, 0, 0,
		          255, 255, 255,
		          0, 0, 0,
		          1, -2,
		          GL_SRC_ALPHA, GL_ONE,
		          size, 0,
		          particle_lightning, PART_LIGHTNING|PART_GLARE|PART_OVERBRIGHT,
		          0, false);

	if (!p)
		return;

	p->src_ent=srcEnt;
	p->dst_ent=dstEnt;
}

void CL_LightningFlare (vec3_t start, int srcEnt, int dstEnt)
{
	cparticle_t *list;
	cparticle_t *p=NULL;

	for (list=active_particles ; list ; list=list->next)
		if (list->src_ent == srcEnt && list->dst_ent == dstEnt && list->image == particle_lightflare) {
			p = list;
			p->start = p->ptime = cl.stime;
			VectorCopy(start, p->org);
			return;
		}

	p = setupParticle(0, 0, 0,
		          start[0], start[1], start[2],
		          0, 0, 0,
		          0, 0, 0,
		          255, 255, 255,
		          0, 0, 0,
		          1, -2.5,
		          GL_SRC_ALPHA, GL_ONE,
		          15, 0,
		          particle_lightflare, 0,
		          0, false);

	if (!p)
		return;

	p->src_ent=srcEnt;
	p->dst_ent=dstEnt;
}

void CL_Explosion_Decal (vec3_t org, float size)
{
	float scale = cl_explosion_scale->value;

	if (scale<1) scale = 1;
	size *= scale;

	if (r_decals->value) 
		setupParticle(0, 0, 0,
			      org[0], org[1], org[2],
			      0, 0, 0,
			      0, 0, 0,
			      255, 255, 255,
			      0, 0, 0,
			      0.5f, -0.005,
			      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
			      size*0.5, 0,
			      particle_impact, PART_DECAL|PART_ALPHACOLOR|PART_DECAL_SUB,
			      pDecalAlphaThink, true);
}

void CL_Explosion_Underwater_Decal (vec3_t org, float size)
{
	float scale = cl_explosion_scale->value;

	if (scale<1) scale = 1;
	size *= scale;


	if (r_decals->value)
		setupParticle(0, 0, 0,
			      org[0], org[1], org[2],
			      0, 0, 0,
			      0, 0, 0,
			      255, 255, 255,
			      0, 0, 0,
			      1, -0.005,
			      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
			      size*0.5, 0,
			      particle_impactwater, PART_DECAL|PART_ALPHACOLOR|PART_DECAL_SUB,
			      pDecalAlphaThink, true);
}

void CL_Explosion_Particle (vec3_t org, float size, float grow, int water)
{
	cparticle_t *p = NULL;
	float oldsize = size, scale = cl_explosion_scale->value;
	int i;
	particle_type particleType;
	
	if (cl_explosion->value == 2 || cl_explosion->value == 4)
		particleType = particle_inferno;
	else
		particleType = particle_rexplosion1;
	
	if (scale<0.5) scale = 0.5;
	size *= scale;

	if (water) {
		CL_Explosion_Underwater_Decal (org, oldsize);
		size *= 0.85;

		for (i=0; i<4; i++) {
			vec3_t origin = { 
				org[0]+crandom()*size*0.15, 
				org[1]+crandom()*size*0.15, 
				org[2]+crandom()*size*0.15
			};
			
			trace_t trace = CL_Trace (org, origin, 0, 1);

			p = setupParticle(random()*360, crandom()*90, 0,
				          trace.endpos[0], trace.endpos[1], trace.endpos[2],
				          0, 0, 0,
				          0, 0, 0,
				          255, 255, 255,
				          0, 0, 0,
				          1, -1,
				          GL_SRC_ALPHA, GL_ONE,
				          size, -50,
				          particle_waterexplode, PART_SHADED,
				          pWaterExplosionThink, true);
		}
		if (p)
			addParticleLight (p, size*4, 0, 1, 1, 1);
	}
	else {
		CL_Explosion_Decal(org, oldsize);
		for (i=0;i<3;i++) {
			vec3_t origin = { 
				org[0]+crandom()*size*0.5, 
				org[1]+crandom()*size*0.5, 
				org[2]+crandom()*size*0.5 
			};
			
			trace_t trace = CL_Trace (org, origin, 0, 1);

			setupParticle(random()*360, crandom()*30, 0,
				      trace.endpos[0], trace.endpos[1], trace.endpos[2],
				      0, 0, 0,
				      0, 0, 0,
				      255, 255, 255,
				      0, 0, 0,
				      0.5, -0.25,
				      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
				      size*0.1, size,
				      particle_smoke, PART_SHADED|PART_OVERBRIGHT,
				      pRotateThink, true);
		}
		for (i=0; i<(8+sqrt(size)*0.5); i++) {
			vec3_t origin = { 
				org[0]+crandom()*size*0.25, 
				org[1]+crandom()*size*0.25, 
				org[2]+crandom()*size*0.25
			};
			trace_t trace = CL_Trace (org, origin, 0, 1);

			p = setupParticle(random()*360, crandom()*90, 0,
				          trace.endpos[0], trace.endpos[1], trace.endpos[2],
				          0, 0, 0,
				          0, 0, 0,
				          255, 255, 255,
				          0, 0, 0,
				          1, -1.3 / (0.5 + frand()*0.3),
				          GL_SRC_ALPHA, GL_ONE,
				          size*1.0, -grow*scale*1.0,
				          particleType, 0,
				          pRotateThink, true);
		}
		
		if (p) {	//smooth color blend :D
			float lightsize = oldsize / 100.0;

			addParticleLight (p, lightsize*250, 0, 1, 1, 1);
			addParticleLight (p, lightsize*265, 0, 1, 1, 0);
			addParticleLight (p, lightsize*285, 0, 1, 0, 0);
		}
	}

}

void CL_Disruptor_Explosion_Particle (vec3_t org, float size)
{
	cparticle_t *p = NULL;
	int i;
	float	alphastart = 1,
		alphadecel = -5,
		scale = cl_explosion_scale->value;

	if (scale<1) scale = 1;
	size *= scale;

	//now add main sprite
	if (!cl_explosion->value) {
		p = setupParticle(0, 0, 0,
				  org[0], org[1], org[2],
				  0, 0, 0,
				  0, 0, 0,
			          255, 255, 255,
				  0, 0, 0,
				  alphastart, alphadecel,
				  GL_SRC_ALPHA, GL_ONE,
				  size, 0,
				  particle_dexplosion1, PART_DEPTHHACK_SHORT,
				  pDisruptExplosionThink, true);
	}
	else {
		for (i=0; i<(8+sqrt(size)*0.5); i++) {
			vec3_t origin = { 
				org[0]+crandom()*size*0.15, 
				org[1]+crandom()*size*0.15, 
				org[2]+crandom()*size*0.15
			};
			
			trace_t trace = CL_Trace (org, origin, 0, 1);

			p = setupParticle(random()*360, crandom()*90, 0,
				          trace.endpos[0], trace.endpos[1], trace.endpos[2],
				          0, 0, 0,
				          0, 0, 0,
				          255, 255, 255,
				          0, 0, 0,
				          1, -1.3 / (0.5 + frand()*0.3),
				          GL_SRC_ALPHA, GL_ONE,
				          size*0.75, -scale*1.5,
				          particle_dexplosion2, 0,
				          pRotateThink, true);
		}
	}

	if (p) {
		addParticleLight (p, size*1.0f, 0, 1, 1, 1);
		addParticleLight (p, size*1.25f, 0, 0.75, 0, 1);
		addParticleLight (p, size*1.65f, 0, 0.25, 0, 1);
		addParticleLight (p, size*1.9f, 0, 0, 0, 1);
	}

}

void CL_BloodSmack (vec3_t org, vec3_t dir)
{
	setupParticle(crand()*180, crand()*100, 0,
		      org[0], org[1], org[2],
		      dir[0], dir[1], dir[2],
		      0, 0, 0,
		      255, 255, 255,
		      0, 0, 0,
		      1.0, -0.75 / (0.5 + frand()*0.3),
		      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
		      10, 0,
	              particleBlood(),
		      PART_ALPHACOLOR,
		      pRotateThink,true);
}

void CL_BloodBleed (vec3_t org, vec3_t pos, vec3_t dir)
{
	setupParticle(org[0], org[1], org[2],
		      org[0]+((rand()&7)-4)+dir[0], org[1]+((rand()&7)-4)+dir[1], org[2]+((rand()&7)-4)+dir[2],
		      pos[0]*(random()*3+5), pos[1]*(random()*3+5), pos[2]*(random()*3+5),
		      0, 0, 0,
		      255, 255, 255,
		      0, 0, 0,
		      0.25 + 0.75*random(), -0.25 / (0.5 + frand()*0.3),
		      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
		      MAXBLEEDSIZE, 0,
		      particle_blooddrip,
		      PART_DIRECTION | PART_GRAVITY | PART_ALPHACOLOR,
		      pBloodDropThink,true);
}

void CL_BloodSpray (vec3_t move, vec3_t vec)
{
	setupParticle(0, rand()%360, rand()%360,
		      move[0] + crand()*7.5, move[1] + crand()*7.5, move[2] + crand()*7.5,
		      vec[0]*(1+random())*15.0, vec[1]*(1+random())*15.0, vec[2]*(1+random())*15.0,
		      0, 0, 0,
		      255, 255, 255,
		      0, 0, 0,
		      0.95, -0.75 / (1+frand()*0.4),
		      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
		      2 + random()*2.0, 0, 
		      particle_bloodred,
		      PART_GRAVITY|PART_ALPHACOLOR,
		      pBloodThink, true);
}

void CL_BloodHit (vec3_t org, vec3_t dir)
{
	vec3_t	move;
	int		i;

	VectorScale(dir, 50, move);

	if (cl_blood->value<=0)
		CL_BloodSpray(org, dir);	//CL_BloodSmack(org, dir);
	else
	{
		for (i=0;i<cl_blood->value;i++)
		{
			if (cl_blood->value>1)
			{
				VectorSet(move,
					dir[0]+frand()*cl_blood->value,
					dir[1]+frand()*cl_blood->value,
					dir[2]+frand()*cl_blood->value
					);
				VectorNormalize(move);
			}
			else
				VectorCopy(dir, move);

			VectorScale(move, 10 + cl_blood->value*0.1, move);
			CL_BloodBleed (org, move, dir);
		}
	}
}

void CL_GreenBloodHit(vec3_t org, vec3_t dir)
{
	int		i;
	float		d;

	for (i = 0; i < 5; i++) {
		d = rand() & 31;
		setupParticle(crand() * 180, crand() * 100, 0,
		              org[0]+((rand()&7)-4)+d*dir[0],
			      org[1]+((rand()&7)-4)+d*dir[1],
			      org[2]+((rand()&7)-4)+d*dir[2],
		              dir[0]*(crand()*3+5), dir[1]*(crand()*3+5), dir[2]*(crand()*3+5),
		              0, 0, -100,
		              220, 140, 50,
		              0, 0, 0,
		              1.0, -1.0,
		              GL_SRC_ALPHA, GL_ONE,
		              10, 0,
		              particle_blood,
		              PART_SHADED,
		              pRotateThink, true);
	}

}
#endif

/*
===============
CL_ParticleEffect

Wall impact puffs
===============
*/
#if defined(PARTICLESYSTEM)
void CL_ParticleEffect (vec3_t org, vec3_t dir, int color8, int count)
{
	int			i;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&31;
		setupParticle(0, 0, 0,
			      org[0] + ((rand()&7)-4) + d*dir[0],
			      org[1] + ((rand()&7)-4) + d*dir[1],
			      org[2] + ((rand()&7)-4) + d*dir[2],
			      crand()*20, crand()*20, crand()*20,
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      1.0, -1.0 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      1, 0,
			      particle_generic,
			      PART_GRAVITY|PART_GLARE,
			      NULL, 0);
	}
}

void CL_ParticleWaterEffectSplash (vec3_t org, vec3_t dir, int color8, int count, int r)
{
	int		i, shaded = PART_SHADED|PART_OVERBRIGHT;
	float		scale;
	vec3_t		angle, end, dirscaled, color = { color8red(color8), color8green(color8), color8blue(color8)};

	scale = random()*0.25 + 0.75;

	if (r == SPLASH_LAVA)
		shaded = PART_GLARE;

	vectoanglerolled(dir, rand()%360, angle);
	setupParticle(angle[0], angle[1], angle[2],
		      org[0]+dir[0]*0.1, org[1]+dir[1]*0.1, org[2]+dir[2]*0.1,
		      0, 0, 0,
		      0, 0, 0,
		      color[0], color[1], color[2],
		      0, 0, 0,
		      2, -0.3 / (0.5 + frand()*0.3),
		      GL_SRC_ALPHA, GL_ONE,
		      scale*25, 25,
		      particle_wave,
		      PART_ANGLED|shaded,
		      NULL, 0);

	VectorScale(dir, scale, dirscaled);

	for (i=0;i<count*0.75;i++)
	{
		end[0] = org[0] + dirscaled[0]*10 + crand()*5;
		end[1] = org[1] + dirscaled[1]*10 + crand()*5;
		end[2] = org[2] + dirscaled[2]*10 + crand()*5;

		setupParticle(end[0], end[1], end[2],
			      org[0], org[1], org[2],
			      0, 0, 0,
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      1, -1.5 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      scale*10, 10,
			      particle_splash,
			      PART_BEAM|shaded,
			      NULL, 0);
	}

	for (i=0 ; i<count ; i++)
	{
		VectorSet(angle,
			dirscaled[0]*65 + crand()*25,
			dirscaled[1]*65 + crand()*25,
			dirscaled[2]*65 + crand()*25);
		setupParticle(0, 0, 0,
			      org[0]+angle[0]*0.25, org[1]+angle[1]*0.25, org[2]+angle[2]*0.25,
			      angle[0], angle[1], angle[2],
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      1, -1.0 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      scale*(4 + random()*1.75), 1,
			      particle_bubblesplash,
			      PART_DIRECTION|shaded|PART_GRAVITY,
			      pWaterSplashThink,true);
	}
}

void CL_ParticleEffectSplash (vec3_t org, vec3_t dir, int color8, int count)
{
	int			i, flags = PART_GRAVITY|PART_DIRECTION;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&5;
		setupParticle(org[0], org[1], org[2],
			      org[0]+d*dir[0], org[1]+d*dir[1], org[2]+d*dir[2],
			      dir[0]*60.0 + crand()*10, dir[1]*60.0 + crand()*10, dir[2]*60.0 + crand()*10,
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      1, -1.0 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      2 -random()*0.75, -2,
			      particle_generic,
			      flags,
			      pSplashThink,true);
	}
}
#else
void CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = color + (rand()&7);

		d = rand()&31;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}
#endif

/*
===============
CL_ParticleEffect2
===============
*/
#if defined(PARTICLESYSTEM)
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color8, int count)
{
	int			i;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&7;
		setupParticle(0, 0, 0,
		              org[0]+((rand()&7)-4)+d*dir[0], org[1]+((rand()&7)-4)+d*dir[1], org[2]+((rand()&7)-4)+d*dir[2],
			      crand()*20, crand()*20, crand()*20,
			      0, 0, 0,
			      color[0] + colorAdd, color[1] + colorAdd, color[2] + colorAdd,
			      0, 0, 0,
			      1, -1.0 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      1, 0,
			      particle_generic,
			      PART_GRAVITY,
			      NULL, 0);
	}
}
#else
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = color;

		d = rand()&7;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}
#endif

// RAFAEL
/*
===============
CL_ParticleEffect3
===============
*/
#if defined(PARTICLESYSTEM)
void CL_ParticleEffect3 (vec3_t org, vec3_t dir, int color8, int count)
{
	int			i;
	float		d;
	vec3_t color = { color8red(color8), color8green(color8), color8blue(color8)};

	for (i=0 ; i<count ; i++)
	{
		d = rand()&7;
		setupParticle(0, 0, 0,
		              org[0]+((rand()&7)-4)+d*dir[0], org[1]+((rand()&7)-4)+d*dir[1], org[2]+((rand()&7)-4)+d*dir[2],
			      crand()*20, crand()*20, crand()*20,
			      0, 0, 0,
			      color[0] + colorAdd, color[1] + colorAdd, color[2] + colorAdd,
			      0, 0, 0,
			      1, -1.0 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      1, 0,
			      particle_generic,
			      PART_GRAVITY,
			      NULL, false);
	}
}
#else
void CL_ParticleEffect3 (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = color;

		d = rand()&7;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}
#endif

#if defined(PARTICLESYSTEM)
void CL_ParticleEffectSparks (vec3_t org, vec3_t dir, vec3_t color, int count)
{
	int	i;
	float	d;
	for (i=0 ; i<count ; i++)
	{
		d = rand()&7;
		setupParticle(0, 0, 0,
			      org[0]+((rand()&3)-2), org[1]+((rand()&3)-2), org[2]+((rand()&3)-2),
			      crand()*40 + dir[0]*100, crand()*40 + dir[1]*100, crand()*40 + dir[2]*100,
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      0.9, -2.5 / (0.5 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      0.5, 0, 
			      particle_generic, PART_GRAVITY|PART_DIRECTION,
			      pSparksThink,true);
	}
}

void CL_ParticleFootPrint (vec3_t org, vec3_t angle, float size, vec3_t color)
{
	if (!r_decals->value)
		return;

	setupParticle(angle[0], angle[1], angle[2],
	              org[0], org[1], org[2],
		      0, 0, 0,
		      0, 0, 0,
		      color[0], color[1], color[2],
		      0, 0, 0,
		      1, -.001,
		      GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
		      size, 0,
		      particle_footprint,
		      PART_DECAL|PART_DECAL_SUB|PART_ALPHACOLOR,
		      pDecalAlphaThink, true);
}

/*
===============
CL_ParticleBulletDecal
===============
*/
void CL_ParticleBulletDecal (vec3_t org, vec3_t dir, float size)
{
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;

	VectorMA(org, DECAL_OFFSET, dir, origin);
	VectorMA(org, -DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);

	if (tr.fraction==1)
		return;
	if (VectorCompare(tr.plane.normal, vec3_origin))
		return;

	VectorNegate(tr.plane.normal, angle);
	vectoanglerolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	CL_ImpactMark (origin, ang, size, tr.surface->flags, particle_bulletmark);
}

void CL_ParticleBlasterDecal (vec3_t org, vec3_t dir, float size)
{
	cparticle_t	*p;
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;

	VectorMA(org, DECAL_OFFSET, dir, origin);
	VectorMA(org, -DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);

	if (tr.fraction==1)
		return;
	if (VectorCompare(tr.plane.normal, vec3_origin))
		return;

	VectorNegate(tr.plane.normal, angle);
	vectoanglerolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	CL_ImpactMark (origin, ang, size, tr.surface->flags, particle_impactblaster);

	setupParticle(ang[0], ang[1], ang[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      250, 150, 50,
		      0, 0, 0,
		      1, -0.1,
		      GL_SRC_ALPHA, GL_ONE,
		      size, 0,
		      particle_generic,
		      PART_SHADED|PART_DECAL|PART_DECAL_ADD|PART_GLARE,
		      NULL, false);

	setupParticle(ang[0], ang[1], ang[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      250, 150, 50,
		      0, -25, -50,
		      1, -0.5,
		      GL_SRC_ALPHA, GL_ONE,
		      size, 0,
		      particle_generic,
		      PART_DECAL|PART_DECAL_ADD|PART_GLARE,
		      NULL, false);

	p = setupParticle(ang[0], ang[1], ang[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      255, 225, 200,
		      0, 0, 0,
		      1, -1,
		      GL_SRC_ALPHA, GL_ONE,
		      size*0.75, 0,
		      particle_generic,
		      PART_DECAL|PART_DECAL_ADD|PART_GLARE,
		      NULL, false);
#if 0	
	if (p)
		addParticleLight(p, 200, 0, 1, 1, 0);
#endif
}

void CL_ParticleBlasterColorDecal(vec3_t org, vec3_t dir, float size, int red, int green, int blue)
{
	cparticle_t	*p;
	vec3_t		ang, angle, end, origin;
	trace_t		tr;

	if (!r_decals->value)
		return;

	VectorMA(org, DECAL_OFFSET, dir, origin);
	VectorMA(org, -DECAL_OFFSET, dir, end);
	tr = CL_Trace (origin, end, 0, CONTENTS_SOLID);

	if (tr.fraction==1)
		return;
	if (VectorCompare(tr.plane.normal, vec3_origin))
		return;

	VectorNegate(tr.plane.normal, angle);
	vectoanglerolled(angle, rand()%360, ang);
	VectorCopy(tr.endpos, origin);

	CL_ImpactMark (origin, ang, size, tr.surface->flags, particle_impactblaster);

	setupParticle(ang[0], ang[1], ang[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      red, green, blue,
		      0, 0, 0,
		      1, -0.1,
		      GL_SRC_ALPHA, GL_ONE,
		      size, 0,
		      particle_generic,
		      PART_SHADED|PART_DECAL|PART_DECAL_ADD|PART_GLARE,
		      NULL, false);

	setupParticle(ang[0], ang[1], ang[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      red, green, blue,
		      0, 0, 0,
		      1, -0.5,
		      GL_SRC_ALPHA, GL_ONE,
		      size, 0,
		      particle_generic,
		      PART_DECAL|PART_DECAL_ADD|PART_GLARE,
		      NULL, false);

	p = setupParticle(ang[0], ang[1], ang[2],
		      origin[0], origin[1], origin[2],
		      0, 0, 0,
		      0, 0, 0,
		      red, green, blue,
		      0, 0, 0,
		      1, -1,
		      GL_SRC_ALPHA, GL_ONE,
		      size*0.75, 0,
		      particle_generic,
		      PART_DECAL|PART_DECAL_ADD|PART_GLARE,
		      NULL, false);
#if 0	//TODO
	if (p)
		addParticleLight(p, 200, 0, 1, 1, 0);
#endif
}
#endif

/*
===============
CL_TeleporterParticles
===============
*/
#if defined(PARTICLESYSTEM)
#if 0
void CL_TeleporterParticles (entity_state_t *ent)
{
	int		i, speed = 10;

	for (i=0 ; i<8 ; i++)
	{
		setupParticle(0, 0, 0,
			      ent->origin[0] + crandom()*speed,
			      ent->origin[1] + crandom()*speed,
			      ent->origin[2] + crandom()*speed,
			      crandom()*speed*5, crandom()*speed*5, speed*5 + random()*speed*5,
			      0, 0, 0,
			      255, 255, 200,
			      0, 0, 0,
			      1, -1,
			      GL_SRC_ALPHA, GL_ONE,
			      2+random()*3, -3,
			      particle_generic,
			      PART_GRAVITY|PART_GLARE,
			      NULL, 0);
	}
}
#else
void CL_TeleporterParticles(entity_state_t * ent)
{
	int		i;
	particle_type particleType;
	float 	size;
	
	if (cl_particles_type->value == 2) {
		size = 5.0;	
		particleType = particle_lensflare;
	}
	else if (cl_particles_type->value == 1) {
		size = 2.0;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 8; i++) {
		setupParticle(0, 0, 0,
		              ent->origin[0]-16+(rand()&31), ent->origin[1]-16+(rand()&31), ent->origin[2]-16+(rand()&31),
		              crand()*14, crand()*14, crand()*14,
		              0, 0, 0,
		              200+rand()*50, 200+rand()*50, 200+rand()*50,
		              0, 0, 0,
		              1, -0.5,
		              GL_SRC_ALPHA, GL_ONE,
		              size, 0,
		              particleType,
		              PART_GRAVITY|PART_ALPHACOLOR,
		              NULL, 0);
	}
}
#endif
#else
void CL_TeleporterParticles (entity_state_t *ent)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<8 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = 0xdb;

		for (j=0 ; j<2 ; j++)
		{
			p->org[j] = ent->origin[j] - 16 + (rand()&31);
			p->vel[j] = crand()*14;
		}

		p->org[2] = ent->origin[2] - 8 + (rand()&7);
		p->vel[2] = 80 + (rand()&7);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.5;
	}
}
#endif

/*
===============
CL_LogoutEffect

===============
*/
#if defined(PARTICLESYSTEM)
#if 0
void CL_LogoutEffect (vec3_t org, int type)
{
	int			i;
	vec3_t	color;

	for (i=0 ; i<500 ; i++)
	{
		if (type == MZ_LOGIN)// green
		{
			color[0] = 20;
			color[1] = 200;
			color[2] = 20;
		}
		else if (type == MZ_LOGOUT)// red
		{
			color[0] = 200;
			color[1] = 20;
			color[2] = 20;
		}
		else// yellow
		{
			color[0] = 200;
			color[1] = 200;
			color[2] = 20;
		}
		
		setupParticle(0, 0, 0,
			      org[0] - 16 + frand()*32, org[1] - 16 + frand()*32, org[2] - 24 + frand()*56,
			      crand()*20, crand()*20, crand()*20,
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      1, -1.0 / (1.0 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      1, 0,
			      particle_generic,
			      PART_GRAVITY,
			      NULL, 0);
	}
}
#else
void
CL_LogoutEffect(vec3_t org, int type)
{
	int		i;
	vec3_t		color;
	particle_type particleType;
	float 	size;
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.5;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 500; i++) {
		if (type == MZ_LOGIN) {	/* green */
			color[0] = 20;
			color[1] = 200;
			color[2] = 20;
		} else if (type == MZ_LOGOUT) {	/* red */
			color[0] = 200;
			color[1] = 20;
			color[2] = 20;
		} else {	/* yellow */
			color[0] = 200;
			color[1] = 200;
			color[2] = 20;
		}

		setupParticle(0, 0, 0,
		              org[0] - 16 + frand() * 32, org[1] - 16 + frand() * 32, org[2] - 24 + frand() * 56,
			      crand() * 20, crand() * 20, crand() * 20,
			      0, 0, 0,
			      color[0], color[1], color[2],
			      0, 0, 0,
			      1, -1.0 / (1.0 + frand() * 0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      size, 0,
			      particleType,
			      PART_GRAVITY,
			      NULL, 0);
	}
}
#endif
#else
void CL_LogoutEffect (vec3_t org, int type)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<500 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;

		if (type == MZ_LOGIN)
			p->color = 0xd0 + (rand()&7);	// green
		else if (type == MZ_LOGOUT)
			p->color = 0x40 + (rand()&7);	// red
		else
			p->color = 0xe0 + (rand()&7);	// yellow

		p->org[0] = org[0] - 16 + frand()*32;
		p->org[1] = org[1] - 16 + frand()*32;
		p->org[2] = org[2] - 24 + frand()*56;

		for (j=0 ; j<3 ; j++)
			p->vel[j] = crand()*20;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frand()*0.3);
	}
}
#endif

/*
===============
CL_ItemRespawnParticles

===============
*/
#if defined(PARTICLESYSTEM)
#if 0
void CL_ItemRespawnParticles (vec3_t org)
{
	int			i;

	for (i=0 ; i<64 ; i++)
	{
		setupParticle(0, 0, 0,
			      org[0] + crand()*8, org[1] + crand()*8, org[2] + crand()*8,
			      crand()*8, crand()*8, crand()*8,
			      0, 0, PARTICLE_GRAVITY*0.2,
			      0, 150+rand()*25, 0,
			      0, 0, 0,
			      1, -1.0 / (1.0 + frand()*0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      1, 0,
			      particle_generic,
			      PART_GRAVITY,
			      NULL, 0);
	}
}
#else
void
CL_ItemRespawnParticles(vec3_t org)
{
	int		i;
	particle_type particleType;
	float 	size;
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.4;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 64; i++) {
		setupParticle(0, 0, 0,
		              org[0] + crand() * 8, org[1] + crand() * 8, org[2] + crand() * 8,
			      crand() * 8, crand() * 8, crand() * 8,
			      0, 0, PARTICLE_GRAVITY * 0.2,
			      0, 150 + rand() * 25, 0,
			      0, 0, 0,
			      1, -1.0 / (1.0 + frand() * 0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      size, 0,
			      particleType,
			      PART_GRAVITY,
			      NULL, 0);
	}
}
#endif
#else
void CL_ItemRespawnParticles (vec3_t org)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<64 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;

		p->color = 0xd4 + (rand()&3);	// green

		p->org[0] = org[0] + crand()*8;
		p->org[1] = org[1] + crand()*8;
		p->org[2] = org[2] + crand()*8;

		for (j=0 ; j<3 ; j++)
			p->vel[j] = crand()*8;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY*0.2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frand()*0.3);
	}
}
#endif

/*
===============
CL_ExplosionParticles
===============
*/
#if defined(PARTICLESYSTEM)
void CL_Explosion_Sparks(vec3_t org, int size)
{
	int		i;

	for (i = 0; i < 128; i++) {
		setupParticle(0, 0, 0,
		              org[0] + ((rand() % size) - 16),
			      org[1] + ((rand() % size) - 16),
			      org[2] + ((rand() % size) - 16),
			      (rand() % 150) - 75, (rand() % 150) - 75, (rand() % 150) - 75,
			      0, 0, 0,
			      255, 100, 25,
			      0, 0, 0,
			      1, -0.8 / (0.5 + frand() * 0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      4, -6,
			      particle_blaster,
			      PART_GRAVITY | PART_SPARK,
			      pExplosionSparksThink, true);
	}
}

void CL_ExplosionParticles_Old(vec3_t org)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 256; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color[0] = 255;
		p->color[1] = 0;
		p->color[2] = rand() & 7;
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}

void CL_ExplosionParticles (vec3_t org, float scale, int water)
{
	vec3_t vel;
	int	i, flags, img, blend[2], colorvel[3], amount;
	float size, sizevel, speed, alphavel, veladd;
	void *think;

	if (water) {
		img = particle_bubble;
		flags = PART_SHADED;
		size = (rand()%2 + 2)*0.333;
		sizevel = 3;
		blend[0] = GL_SRC_ALPHA;
		blend[1] = GL_ONE_MINUS_SRC_ALPHA;
		colorvel[0] = colorvel[1] = colorvel[2] = 0;
		alphavel = -0.1 / (0.5 + frand()*0.3);
		speed = 25;

		veladd = 10;
		amount = 4;

		think = pBubbleThink;
	}
	else {
		img = particle_blaster;
		flags = PART_DIRECTION|PART_GRAVITY;
		size = scale*0.75;
		sizevel = -scale*0.5;
		blend[0] = GL_SRC_ALPHA;
		blend[1] = GL_ONE;
		colorvel[0] = 0;
		colorvel[1] = -200;
		colorvel[2] = -400;
		alphavel = -1.0 / (0.5 + frand()*0.3);
		speed = 250;

		veladd = 0;
		amount = 2;

		think = pExplosionSparksThink;
	}

	for (i=0 ; i<256 ; i++) {
		if (i%amount!=0)
			continue;

		VectorSet(vel, crandom(), crandom(), crandom());
		VectorNormalize(vel);
		VectorScale(vel, scale*speed, vel);

		setupParticle(crand()*360, 0, 0,
			      org[0]+((rand()%32)-16), org[1]+((rand()%32)-16), org[2]+((rand()%32)-16),
			      vel[0], vel[1], vel[2] + veladd,
			      0, 0, 0,
			      255, 255, 255,
			      colorvel[0], colorvel[1], colorvel[2],
			      1, alphavel,
			      blend[0], blend[1],
			      size, sizevel,
			      img,
			      flags,
			      think, true);
	}
}
#else
void CL_ExplosionParticles (vec3_t org)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<256 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = 0xe0 + (rand()&7);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%384)-192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand()*0.3);
	}
}
#endif

/*
===============
CL_BigTeleportParticles
===============
*/
#if defined(PARTICLESYSTEM)
void CL_BigTeleportParticles (vec3_t org)
{
	int			i, index;
	float		angle, dist;
	static int colortable0[4] = {10,50,150,50};
	static int colortable1[4] = {150,150,50,10};
	static int colortable2[4] = {50,10,10,150};

	for (i=0 ; i<4096 ; i++)
	{

		index = rand()&3;
		angle = M_PI*2*(rand()&1023)/1023.0;
		dist = rand()&31;
		setupParticle (
			0,	0,	0,
			org[0]+cos(angle)*dist,	org[1] + sin(angle)*dist,org[2] + 8 + (rand()%90),
			cos(angle)*(70+(rand()&63)),sin(angle)*(70+(rand()&63)),-100 + (rand()&31),
			-cos(angle)*100,	-sin(angle)*100,PARTICLE_GRAVITY*4,
			colortable0[index],	colortable1[index],	colortable2[index],
			0,	0,	0,
			1,		-0.3 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			1,		0.3 / (0.5 + frand()*0.3),		
			particle_generic,
			0,
			NULL,0);
	}
}
#else
void CL_BigTeleportParticles (vec3_t org)
{
	int			i;
	cparticle_t	*p;
	float		angle, dist;
	static int colortable[4] = {2*8,13*8,21*8,18*8};

	for (i=0 ; i<4096 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;

		p->color = colortable[rand()&3];

		angle = M_PI*2*(rand()&1023)/1023.0;
		dist = rand()&31;
		p->org[0] = org[0] + cos(angle)*dist;
		p->vel[0] = cos(angle)*(70+(rand()&63));
		p->accel[0] = -cos(angle)*100;

		p->org[1] = org[1] + sin(angle)*dist;
		p->vel[1] = sin(angle)*(70+(rand()&63));
		p->accel[1] = -sin(angle)*100;

		p->org[2] = org[2] + 8 + (rand()%90);
		p->vel[2] = -100 + (rand()&31);
		p->accel[2] = PARTICLE_GRAVITY*4;
		p->alpha = 1.0;

		p->alphavel = -0.3 / (0.5 + frand()*0.3);
	}
}
#endif

/*
===============
CL_BlasterParticles

Wall impact puffs
===============
*/
#if defined(PARTICLESYSTEM)
void CL_BlasterParticles (vec3_t org, vec3_t dir, int count)
{
	int			i;
	vec3_t		origin;
	float speed = .75;
	particle_type particleType;

	if (cl_blaster_type->value == 1 ||
            ((cl_blaster_type->value == 2) &&
            (cl.refdef.rdflags & RDF_UNDERWATER)))
		particleType = particle_bubble;
	else
		particleType = particle_generic;

	for (i=0 ; i<count ; i++)
	{
		VectorSet(origin,
			org[0] + dir[0]*(1 + random()*3 + pBlasterMaxSize/2.0),
			org[1] + dir[1]*(1 + random()*3 + pBlasterMaxSize/2.0),
			org[2] + dir[2]*(1 + random()*3 + pBlasterMaxSize/2.0)
			);

		setupParticle(org[0], org[1], org[2],
			      origin[0], origin[1], origin[2],
			     (dir[0]*75 + crand()*40)*speed,	(dir[1]*75 + crand()*40)*speed,	(dir[2]*75 + crand()*40)*speed,
			     0, 0, 0,
			     255, 150, 50,
			     0, -90, -30,
			     1, -0.5 / (0.5 + frand()*0.3),
			     GL_SRC_ALPHA, GL_ONE,
			     pBlasterMaxSize, -0.75,
			     particleType, PART_GRAVITY|PART_GLARE,
			     pBlasterThink,true);
	}
}

void CL_BlasterParticlesColor(vec3_t org, vec3_t dir, int count,
                              int red, int green, int blue,
                              int reddelta, int greendelta, int bluedelta)
{
	int		i;
	float		speed = .75;
	vec3_t		origin;
	particle_type particleType;

	if (cl_blaster_type->value == 1 ||
            ((cl_blaster_type->value == 2) &&
            (cl.refdef.rdflags & RDF_UNDERWATER)))
		particleType = particle_bubble;
	else
		particleType = particle_generic;
		
	for (i = 0; i < count; i++) {
		VectorSet(origin,
		          org[0] + dir[0] * (1 + random() * 3 + pBlasterMaxSize / 2.0),
		          org[1] + dir[1] * (1 + random() * 3 + pBlasterMaxSize / 2.0),
		          org[2] + dir[2] * (1 + random() * 3 + pBlasterMaxSize / 2.0));

		setupParticle(org[0], org[1], org[2],
			      origin[0], origin[1], origin[2],
			      (dir[0]*75+crand()*40)*speed, (dir[1]*75+crand()*40)*speed, (dir[2]*75+crand()*40)*speed,
			      0, 0, 0,
			      red, green, blue,
			      reddelta, greendelta, bluedelta,
			      1, -0.5 / (0.5 + frand() * 0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      pBlasterMaxSize, -0.75,
			      particleType, PART_GRAVITY|PART_GLARE,
			      pBlasterThink, true);
	}
}
#else
void CL_BlasterParticles (vec3_t org, vec3_t dir)
{
	int			i, j;
	cparticle_t	*p;
	float		d;
	int			count;

	count = 40;
	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = 0xe0 + (rand()&7);

		d = rand()&15;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = dir[j] * 30 + crand()*40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}
#endif

/*
===============
CL_BlasterTrail

===============
*/
#if defined(PARTICLESYSTEM)
void CL_BlasterTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;
	particle_type particleType;
	
	if (cl_blaster_type->value == 1 || ((cl_blaster_type->value == 2) &&
            (cl.refdef.rdflags & RDF_UNDERWATER)))
		particleType = particle_bubble;
	else
		particleType = particle_generic;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 2;
	VectorScale (vec, dec, vec);

	for (; len > 0; len -= dec, VectorAdd (move, vec, move))
		setupParticle(0, 0, 0,
			      move[0] + crand(), move[1] + crand(), move[2] + crand(),
			      crand()*5, crand()*5, crand()*5,
			      0, 0, 0,
			      255, 150, 50,
			      0, -90, -30,
			      1, -1.0 / (0.5 + frand() * 0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      4, -6,
			      particleType, PART_GLARE,
			      NULL, 0);
}

void CL_BlasterTrailColor(vec3_t start, vec3_t end,
                     int red, int green, int blue,
		     int reddelta, int greendelta, int bluedelta)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;
	particle_type particleType;
	
	if (cl_blaster_type->value == 1 || ((cl_blaster_type->value == 2) &&
            (cl.refdef.rdflags & RDF_UNDERWATER)))
		particleType = particle_bubble;
	else
		particleType = particle_generic;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 4;
	VectorScale(vec, dec, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -1.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    4, -6,
			    particleType, PART_GLARE,
			    NULL, 0);
		VectorAdd(move, vec, move);
	}
}

void CL_HyperBlasterTrail(vec3_t start, vec3_t end,
                          int red, int green, int blue,
			  int reddelta, int greendelta, int bluedelta)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;
	int		i;
	particle_type particleType;

	if (cl_hyperblaster_particles_type->value == 1 || 
	    ((cl_hyperblaster_particles_type->value == 2) &&
            (cl.refdef.rdflags & RDF_UNDERWATER)))
		particleType = particle_bubble;
	else
		particleType = particle_generic;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	VectorMA(move, 0.5, vec, move);
	len = VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	for (i = 0; i < 18; i++) {
		len -= dec;
		
		setupParticle(0, 0, 0,
			      move[0] + crand(), move[1] + crand(), move[2] + crand(),
			      crand() * 5, crand() * 5, crand() * 5,
			      0, 0, 0,
			      red, green, blue,
			      reddelta, greendelta, bluedelta,
			      1, -16.0 / (0.5 + frand() * 0.3),
			      GL_SRC_ALPHA, GL_ONE,
			      3, -36,
			      particleType, PART_GLARE,
			      NULL, 0);

		VectorAdd(move, vec, move);
	}
}

void CL_HyperBlasterEffect(vec3_t start, vec3_t end, vec3_t angle, 
                           int red, int green, int blue, 
			   int reddelta, int greendelta, int bluedelta, float len, float size)
{
	if (cl_hyperblaster_particles->value)
		CL_HyperBlasterTrail(start, end, red, green, blue, reddelta, greendelta, bluedelta);
	else
		CL_BlasterTracer(end, angle, red, green, blue, len, size);
}
#else
void CL_BlasterTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);
		
		p->ptime = cl.stime;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3+frand()*0.2);
		p->color = 0xe0;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand()*5;
			p->accel[j] = 0;
		}

		VectorAdd (move, vec, move);
	}
}
#endif
/*
===============
CL_QuadTrail

===============
*/
#if defined(PARTICLESYSTEM)
void CL_QuadTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	for (; len > 0; len -= dec, VectorAdd (move, vec, move))
	{

		setupParticle (
			0,	0,	0,
			move[0] + crand()*16,	move[1] + crand()*16,	move[2] + crand()*16,
			crand()*5,	crand()*5,	crand()*5,
			0,		0,		0,
			0,		0,		200,
			0,	0,	0,
			1,		-1.0 / (0.8+frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			1,			0,			
			particle_generic,
			0,
			NULL,0);
	}
}
#else
void CL_QuadTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);
		
		p->ptime = cl.stime;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8+frand()*0.2);
		p->color = 115;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*16;
			p->vel[j] = crand()*5;
			p->accel[j] = 0;
		}

		VectorAdd (move, vec, move);
	}
}
#endif
/*
===============
CL_FlagTrail

===============
*/
#if defined(PARTICLESYSTEM)
void CL_FlagTrail(vec3_t start, vec3_t end, qboolean isred, qboolean isgreen)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, size;
	int		dec;
	particle_type particleType;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 2.0;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}
	
	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0) {
		len -= dec;

		setupParticle(
		    0, 0, 0,
		    move[0] + crand() * 16, move[1] + crand() * 16, move[2] + crand() * 16,
		    crand() * 5, crand() * 5, crand() * 5,
		    0, 0, 0,
		    (isred) ? 255 : 0, (isgreen) ? 255 : 0, (!isred && !isgreen) ? 255 : 0,
		    0, 0, 0,
		    1.5, -1.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    0,
		    NULL, 0);

		VectorAdd(move, vec, move);
	}
}
#else
void CL_FlagTrail (vec3_t start, vec3_t end, float color)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);
		
		p->ptime = cl.stime;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8+frand()*0.2);
		p->color = color;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*16;
			p->vel[j] = crand()*5;
			p->accel[j] = 0;
		}

		VectorAdd (move, vec, move);
	}
}
#endif
/*
===============
CL_DiminishingTrail

===============
*/
#if defined(PARTICLESYSTEM)
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int flags, entity_t *ent)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = (flags & EF_ROCKET) ? 10 : 2;
	VectorScale (vec, dec, vec);

	if (old->trailcount > 900) {
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800) {
		orgscale = 2;
		velscale = 10;
	}
	else {
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;

		if (flags & EF_ROCKET)
		{
			if (cl_explosion->value && CM_PointContents(move,0) & MASK_WATER) {
				setupParticle(crand()*360, 0, 0,
					move[0], move[1], move[2],
					crand()*9, crand()*9, crand()*9+10,
					0, 0, 0,
					255, 255, 255,
					0, 0, 0,
					1, -0.1 / (0.5 + frand()*0.3),
					GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					(rand()%2 + 2)*0.333, 3,
					particle_bubble, PART_SHADED,
					pBubbleThink,true);
			}
			else {
				setupParticle(crand()*180, crand()*100, 0,
					      move[0], move[1], move[2],
					      crand()*5, crand()*5, crand()*5,
					      0, 0, 5,
					      150, 150, 150,
					      100, 100, 100,
					      0.5, -0.25,
					      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					      1, 10,
					      particle_smoke, PART_OVERBRIGHT|PART_SHADED,
					      pRotateThink, true);
			}
		}
		else {
			// drop less particles as it flies
			if ((rand()&1023) < old->trailcount) {
				if (flags & EF_GIB) {
					if (cl_blood->value) {
						if (random()<0.2) {
							setupParticle(0, 0, random()*360,
								      move[0]+crand()*orgscale,
								      move[1]+crand()*orgscale,
								      move[2]+crand()*orgscale,
								      vec[0]*(1+fabs(random()))*velscale,
								      vec[1]*(1+fabs(random()))*velscale,
								      vec[2]*(1+fabs(random()))*velscale,
								      0, 0, 0,
								      255, 255, 255,
								      0, 0, 0,
								      0.75, -0.75 / (1+frand()*0.4),
								      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
								      2+random()*2+len*0.1, 0,
								      particle_bloodred, PART_GRAVITY|PART_ALPHACOLOR,
								      pBloodThink, true);
						}
						if (random()<0.1)
							setupParticle(0, 0, 0,
								      move[0], move[1], move[2],
								      0, 0, 0,
								      0, 0, 0,
								      255, 255, 255,
								      0, 0, 0,
								      1, -0.005,
								      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
								      (4+random())*5.0, 0,
								      particle_bloodred1 + rand()%5,
								      PART_DECAL|PART_ALPHACOLOR|PART_DECAL_SUB,
								      pDecalAlphaThink, true);

					}
					else if (random()<0.2)
						setupParticle(0, 0, random()*360,
							      move[0]+crand()*orgscale,
							      move[1]+crand()*orgscale,
							      move[2]+crand()*orgscale,
							      vec[0]*(1+fabs(random()))*velscale,
							      vec[1]*(1+fabs(random()))*velscale,
							      vec[2]*(1+fabs(random()))*velscale,
							      0, 0, 0,
							      255, 255, 255,
							      0, 0, 0,
							      0.75, -0.75 / (1+frand()*0.4),
							      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
							      2+random()*2, 0,
							      particle_bloodred,
							      PART_GRAVITY|PART_ALPHACOLOR,
							      pBloodThink, true);
				}
				else if (flags & EF_GREENGIB) {
					setupParticle(0, 0, 0,
						      move[0]+crand()*orgscale,
						      move[1]+crand()*orgscale,
						      move[2]+crand()*orgscale,
						      crand()*velscale,
						      crand()*velscale,
						      crand()*velscale,
						      0, 0, 0,
						      0, 255, 0,
						      0, 0, 0,
						      0, -1.0 / (1+frand()*0.4),
						      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
						      5, -1,
						      particle_blood,
						      PART_GRAVITY|PART_SHADED,
						      NULL,0);
				}
				else if (flags & EF_GRENADE) {
					setupParticle(crand()*180, crand()* 50, 0,
					              move[0]+crand()*orgscale,
				        	      move[1]+crand()*orgscale,
						      move[2]+crand()*orgscale,
					              crand()*velscale,
						      crand()*velscale,
						      crand()*velscale,
					              0, 0, 20,
					              255, 255, 255,
					              0, 0, 0,
					              0.5, -0.5,
					              GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					              5, 5,
					              particle_smoke,
					              PART_SHADED,
					              pRotateThink, true);
				}
				else {
					setupParticle(crand()*180, crand()*50, 0,
						      move[0]+crand()*orgscale,
						      move[1]+crand()*orgscale,
						      move[2] + crand()*orgscale,
						      crand()*velscale,
						      crand()*velscale,
						      crand()*velscale,
						      0, 0, 20,
						      255, 255, 255,
						      0, 0, 0,
						      0.5, -0.5,
						      GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
						      5, 5,
						      particle_smoke,
						      PART_OVERBRIGHT|PART_SHADED,
						      pRotateThink,true);
				}
			}

			old->trailcount -= 5;
			if (old->trailcount < 100)
				old->trailcount = 100;
		}

		VectorAdd (move, vec, move);
	}
}
#else
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int flags)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 0.5;
	VectorScale (vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		// drop less particles as it flies
		if ((rand()&1023) < old->trailcount)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear (p->accel);
		
			p->ptime = cl.stime;

			if (flags & EF_GIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1+frand()*0.4);
				p->color = 0xe8 + (rand()&7);
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1+frand()*0.4);
				p->color = 0xdb + (rand()&7);
				for (j=0; j< 3; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1+frand()*0.2);
				p->color = 4 + (rand()&7);
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
				}
				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;
		if (old->trailcount < 100)
			old->trailcount = 100;
		VectorAdd (move, vec, move);
	}
}
#endif
void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}

/*
===============
CL_RocketTrail

===============
*/
#if defined(PARTICLESYSTEM)
#if 0
void CL_RocketTrail(vec3_t start, vec3_t end, centity_t * old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, totallen;
	float		dec;

	/* smoke */
	CL_DiminishingTrail(start, end, old, EF_ROCKET, NULL);

	/* fire */
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	totallen = len = VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;

		/* flame from rocket */
		setupParticle(
		    0, 0, 0,
		    move[0], move[1], move[2],
		    vec[0], vec[1], vec[2],
		    0, 0, 0,
		    255, 200, 100,
		    0, 0, -200,
		    1, -15,
		    GL_SRC_ALPHA, GL_ONE,
		    2.0 * (2 - len / totallen), -15,
		    particle_generic,
		    0,
		    NULL, 0);

		/* falling particles */
		if ((rand() & 7) == 0) {
			setupParticle(
			    0, 0, 0,
			    move[0] + crand() * 5, move[1] + crand() * 5, move[2] + crand() * 5,
			    crand() * 20, crand() * 20, crand() * 20,
			    0, 0, 20,
			    255, 255, 255,
			    0, -100, -200,
			    1, -1.0 / (1 + frand() * 0.2),
			    GL_SRC_ALPHA, GL_ONE,
			    1.5, -3,
			    particle_blaster,
			    PART_GRAVITY,
			    NULL, 0);
		}
		VectorAdd(move, vec, move);
	}

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	totallen = len = VectorNormalize(vec);
	dec = 1.5;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		/* flame */
		setupParticle(
		    crand() * 180, crand() * 100, 0,
		    move[0], move[1], move[2],
		    crand() * 5, crand() * 5, crand() * 5,
		    0, 0, 5,
		    255, 225, 200,
		    -50, -50, -50,
		    0.75, -3,
		    GL_SRC_ALPHA, GL_ONE,
		    5, 5,
		    particle_inferno,
		    0,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}
#else
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old, entity_t *ent)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, totallen;
	float		dec;
	int		count = 0;

	// smoke
	CL_DiminishingTrail (start, end, old, EF_ROCKET, ent);

	if (CM_PointContents(start,0) & MASK_WATER)
	{
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
		totallen = len = VectorNormalize (vec);
		dec = 1.5;
		VectorScale (vec, dec, vec);

		while (len > 0)
		{
			len -= dec;
			count ++;

			//water current stuff
			setupParticle (
				crand()*180, crand()*100, 0,
				move[0],	move[1],	move[2],
				crand()*5,	crand()*5,	crand()*5,
				0,		0,		5,
				255,	255,	255,
				-50,	-50,	-50,
				1,		-10,
				GL_SRC_ALPHA, GL_ONE,
				(5 + count%6)*0.5,			30,
				particle_waterexplode,
				0,
				pRotateThink, true);

			VectorAdd (move, vec, move);
		}
	}
	else
	{

		// fire
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
		totallen = len = VectorNormalize (vec);

		dec = 1;
		VectorScale (vec, dec, vec);

		while (len > 0)
		{
			len -= dec;

			//falling particles
			if ( (rand()&7) == 0)
			{
				setupParticle (
					0,	0,	0,
					move[0] + crand()*5,	move[1] + crand()*5,	move[2] + crand()*5,
					crand()*20,	crand()*20,	crand()*20,
					0,		0,		20,
					255,	255,	255,
					0,		-100,	-200,
					1,		-0.5 / (1+frand()*0.2),
					GL_SRC_ALPHA, GL_ONE,
					1,			-1,			
					particle_blaster,
					PART_GRAVITY,
					NULL,0);
			}

			VectorAdd (move, vec, move);
		}

		len = totallen;
		VectorCopy (start, move);
		dec = 1.5;
		VectorScale (vec, dec, vec);

		while (len > 0)
		{
			len -= dec;
			count ++;

			//flame
			setupParticle (
				crand()*180, crand()*100, 0,
				move[0],	move[1],	move[2],
				crand()*5,	crand()*5,	crand()*5,
				0,		0,		5,
				255,	225,	200,
				-100,	-1000,	-2000,
				1,		-10,
				GL_SRC_ALPHA, GL_ONE,
				(5 + count%6)*0.5,			100,
				particle_inferno,
				PART_GLARE,
				pRotateThink, true);

			VectorAdd (move, vec, move);
		}
	}
}
#endif
#else
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;

	// smoke
	CL_DiminishingTrail (start, end, old, EF_ROCKET);

	// fire
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 1;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		if ( (rand()&7) == 0)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			
			VectorClear (p->accel);
			p->ptime = cl.stime;

			p->alpha = 1.0;
			p->alphavel = -1.0 / (1+frand()*0.2);
			p->color = 0xdc + (rand()&3);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + crand()*5;
				p->vel[j] = crand()*20;
			}
			p->accel[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd (move, vec, move);
	}
}
#endif
/*
===============
CL_RailTrail

===============
*/
#if defined(PARTICLESYSTEM)
/*
===============
FartherPoint
Returns true if the first vector
is farther from the viewpoint.
===============
*/
qboolean FartherPoint (vec3_t pt1, vec3_t pt2)
{
	vec3_t distance1, distance2;

	VectorSubtract(pt1, cl.refdef.vieworg, distance1);
	VectorSubtract(pt2, cl.refdef.vieworg, distance2);
	return (VectorLength(distance1) > VectorLength(distance2));
}

void CL_ParticleRailDecal (vec3_t org, float size)
{
	cparticle_t *p;

	p = setupParticle(0, 0, 0,
		          org[0], org[1], org[2],
		          0, 0, 0,
		          0, 0, 0,
		          255, 255, 255,
		          0, 0, 0,
		          1, -0.01,
		          GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
		          size, 0,
		          particle_railhit,
		          PART_DECAL|PART_DECAL_SUB|PART_ALPHACOLOR,
		          pDecalAlphaThink,
			  true);

	if (p->decalnum) {
		setupParticle(0, 0, 0,
			      org[0], org[1], org[2],
			      0, 0, 0,
			      0, 0, 0,
			      cl_railred->value, cl_railgreen->value, cl_railblue->value,
			      0, 0, 0,
			      1, -0.5,
			      GL_SRC_ALPHA, GL_ONE,
			      size, 0, 
			      particle_generic,
			      PART_DECAL|PART_DECAL_ADD|PART_GLARE,
			      NULL,
			      false);
		setupParticle(0, 0, 0,
			      org[0], org[1], org[2],
			      0, 0, 0,
			      0, 0, 0,
			      cl_railred->value, cl_railgreen->value, cl_railblue->value,
			      0, 0, 0,
			      1, -0.25,
			      GL_SRC_ALPHA, GL_ONE,
			      size, 0,
			      particle_generic,
			      PART_DECAL|PART_DECAL_ADD|PART_GLARE,
			      NULL,
			      false);
	}
}

void CL_RailSpiral(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t		right, up;
	int		i;
	float		d, c, s;
	vec3_t		dir;
	extern cvar_t	*cl_railspiral_lens;

	VectorCopy (end, move);
	VectorSubtract (start, end, vec);
	
	len = VectorNormalize(vec);
	
	MakeNormalVectors(vec, right, up);
	VectorScale(vec, RAILSPACE, vec);

	for (i = 0; i < len; i += RAILSPACE) {
		d = i * 0.1;
		c = cos(d);
		s = sin(d);

		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);
		if (cl_railspiral_lens->value)
			setupParticle( 0, 0, 0,
		                       move[0] + dir[0] * 3, move[1] + dir[1] * 3, move[2] + dir[2] * 3,
		                       dir[0] * 6, dir[1] * 6, dir[2] * 6,
		                       0, 0, 0,
		                       cl_railred->value,
				       cl_railgreen->value,
				       cl_railblue->value,
		                       0, 0, 0,
		                       1.2, -1.0,
		                       GL_SRC_ALPHA, GL_ONE,
		                       2.0 * RAILSPACE, 0,
		                       particle_lensflare,
		                       PART_GLARE,
		                       NULL,
                                       0);
		else
			setupParticle( 0, 0, 0,
		                       move[0] + dir[0] * 3, move[1] + dir[1] * 3, move[2] + dir[2] * 3,
		                       dir[0] * 6, dir[1] * 6, dir[2] * 6,
		                       0, 0, 0,
		                       cl_railred->value,
				       cl_railgreen->value,
				       cl_railblue->value,
		                       0, 0, 0,
		                       1.2, -1.0,
		                       GL_SRC_ALPHA, GL_ONE,
		                       2.5 * RAILSPACE, 0,
		                       particle_generic,
		                       PART_GLARE,
		                       NULL,
				       0);

		VectorAdd(move, vec, move);
	}
}

void
CL_DevRailTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec, point;
	float		len;
	int		dec, i = 0;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize(vec);
	VectorCopy(vec, point);

	dec = 4;
	VectorScale(vec, dec, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;
		i++;

		if (i >= DEVRAILSTEPS) {
			for (i = 3; i > 0; i--)
				setupParticle(point[0], point[1], point[2],
				              move[0], move[1], move[2],
				              0, 0, 0,
				              0, 0, 0,
				              cl_railred->value,
					      cl_railgreen->value,
					      cl_railblue->value,
				              0, -90, -30,
				              1, -.75,
				              GL_SRC_ALPHA, GL_ONE,
				              dec * DEVRAILSTEPS * TWOTHIRDS, 0,
				              particle_beam,
				              PART_DIRECTION,
				              NULL,
					      0);
		}
		setupParticle(0, 0, 0,
		              move[0], move[1], move[2],
		              crand() * 10, crand() * 10, crand() * 10 + 20,
		              0, 0, 0,
		              cl_railred->value,
			      cl_railgreen->value,
			      cl_railblue->value,
		              0, 0, 0,
		              1, -0.75 / (0.5 + frand() * 0.3),
		              GL_SRC_ALPHA, GL_ONE,
		              2, -0.25,
		              0,
		              PART_GRAVITY|PART_SPARK,
		              pDevRailThink,
			      true);

		setupParticle(crand() * 180, crand() * 100, 0,
		              move[0], move[1], move[2],
		              crand() * 10, crand() * 10, crand() * 10 + 20,
		              0, 0, 5,
		              255, 255, 255,
		              0, 0, 0,
		              0.25, -0.25,
		              GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		              5, 10,
		              particle_smoke,
		              PART_TRANS|PART_GRAVITY,
		              pRotateThink,
			      true);
			      
		VectorAdd (move, vec, move);
	}
}

void CL_RailLaser (vec3_t start, vec3_t end)
{
	vec3_t		move, last;
	vec3_t		vec, point;
	float		len;
	vec3_t		right, up;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}

	len = VectorNormalize(vec);
	VectorCopy(vec, point);

	MakeNormalVectors(vec, right, up);
	VectorScale(vec, (cl_railtype->value == 2) ? RAILTRAILSPACE : RAILSPACE, vec);
	VectorCopy(start, move);

	while (len > 0) {
		VectorCopy(move, last);
		VectorAdd(move, vec, move);

		{
			for (; len>0; len -= RAILTRAILSPACE) {
			
				MakeNormalVectors (vec, right, up);
				VectorScale (vec, RAILTRAILSPACE, vec);
				VectorCopy (start, move);

				setupParticle(last[0], last[1], last[2],
					      move[0], move[1], move[2],
					      0, 0, 0,
					      0, 0, 0,
					      cl_railred->value,
					      cl_railgreen->value,
					      cl_railblue->value,
					      0, 0, 0,
					      1.25, -1,
					      GL_SRC_ALPHA, GL_ONE,
					      10, 0,
					      particle_beam,
					      PART_BEAM|PART_GLARE,
					      NULL,
					      0);
			}
				setupParticle(start[0], start[1], start[2],
					      end[0], end[1], end[2],
					      0, 0, 0,
					      0, 0, 0,
					      cl_railred->value,
					      cl_railgreen->value,
					      cl_railblue->value,
					      0, 0, 0,
					      1.25, -1,
					      GL_SRC_ALPHA, GL_ONE,
					      40, 0,
					      particle_beam,
					      PART_BEAM|PART_GLARE,
					      NULL,
					      0);

		}
	}
}

void CL_RailBeam(vec3_t start, vec3_t end)
{
	vec3_t		move, last;
	vec3_t		vec, point;
	int		i;
	float		len;
	vec3_t		right, up;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	
	len = VectorNormalize (vec);
	VectorCopy (vec, point);

	MakeNormalVectors (vec, right, up);
	VectorScale (vec, (cl_railtype->value == 2) ? RAILTRAILSPACE : RAILSPACE, vec);
	VectorCopy (start, move);

	while (len > 0) {
		VectorCopy (move, last);	
		VectorAdd (move, vec, move);
		
		{
			len -= RAILTRAILSPACE;

			for (i=0;i<3;i++)
				setupParticle(last[0], last[1], last[2],
					      move[0], move[1], move[2],
					      0, 0, 0,
					      0, 0, 0,
					      cl_railred->value,
					      cl_railgreen->value,
					      cl_railblue->value,
					      0, 0, 0,
					      1, -1.0,
					      GL_SRC_ALPHA, GL_ONE,
					      RAILTRAILSPACE*TWOTHIRDS, 0,
					      particle_beam,
					      PART_BEAM,
					      NULL,
					      0);

		}
		{
			len -= RAILSPACE;

			setupParticle(last[0], last[1], last[2],
				      move[0], move[1], move[2],
			              0, 0, 0,
			              0, 0, 0,
			              255,
				      255,
				      255,
			              0, 0, 0,
			              1, -1.0,
				      GL_SRC_ALPHA, GL_ONE,
			              3, 0,
			              particle_generic,
			              0,
			              NULL,
				      0);
		}
	}
}

void CL_RailTrail (vec3_t start, vec3_t end)
{

	if (cl_railtype->value == 3) 
		CL_DevRailTrail(start, end);
	else if (cl_railtype->value == 2) {
		CL_RailSpiral(start, end);
		CL_RailBeam (start, end);
	}
	else if (cl_railtype->value == 0)
		CL_RailLaser (start, end);
	else
		CL_RailSpiral(start, end);

	CL_ParticleRailDecal (end, 7.5);
}

#else
void CL_RailTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;
	vec3_t		right, up;
	int			i;
	float		d, c, s;
	vec3_t		dir;
	byte		clr = 0x74;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	MakeNormalVectors (vec, right, up);

	for (i=0 ; i<len ; i++)
	{
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		p->ptime = cl.stime;
		VectorClear (p->accel);

		d = i * 0.1;
		c = cos(d);
		s = sin(d);

		VectorScale (right, c, dir);
		VectorMA (dir, s, up, dir);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.2);
		p->color = clr + (rand()&7);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + dir[j]*3;
			p->vel[j] = dir[j]*6;
		}

		VectorAdd (move, vec, move);
	}

	dec = 0.75;
	VectorScale (vec, dec, vec);
	VectorCopy (start, move);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.6+frand()*0.2);
		p->color = 0x0 + (rand()&15);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*3;
			p->vel[j] = crand()*3;
			p->accel[j] = 0;
		}

		VectorAdd (move, vec, move);
	}
}
#endif

// RAFAEL
/*
===============
CL_IonripperTrail
===============
*/
#if defined(PARTICLESYSTEM)
void CL_IonripperTrail (vec3_t ent, vec3_t start)
{
	vec3_t	move, last;
	vec3_t	vec, aim;
	float	len;
	float	dec;
	float	overlap;

	VectorCopy (start, move);
	VectorSubtract (ent, start, vec);
	len = VectorNormalize (vec);
	VectorCopy(vec, aim);

	dec = len*0.2;
	if (dec<1)
		dec=1;

	overlap = dec*5.0;

	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		VectorCopy(move, last);
		VectorAdd (move, vec, move);

		setupParticle (
			last[0],	last[1],	last[2],
			move[0]+aim[0]*overlap,	move[1]+aim[1]*overlap,	move[2]+aim[2]*overlap,
			0,	0,	0,
			0,		0,		0,
			255,	100,	0,
			0,	0,	0,
			0.5,	-1.0 / (0.3 + frand() * 0.2),
			GL_SRC_ALPHA, GL_ONE,
			3,			3,			
			particle_generic,
			PART_BEAM,
			NULL,0);
	}

	setupParticle (
	0,	0,	0,
	start[0],	start[1],	start[2],
	0,	0,	0,
	0,		0,		0,
	255,	100,	0,
	0,	0,	0,
	0.5,	-1.0 / (0.3 + frand() * 0.2),
	GL_SRC_ALPHA, GL_ONE,
	3,			3,			
	particle_generic,
	PART_GLARE,
	NULL,0);
}
#else
void CL_IonripperTrail (vec3_t start, vec3_t ent)
{
	vec3_t	move;
	vec3_t	vec;
	float	len;
	int		j;
	cparticle_t *p;
	int		dec;
	int     left = 0;

	VectorCopy (start, move);
	VectorSubtract (ent, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);

		p->ptime = cl.stime;
		p->alpha = 0.5;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xe4 + (rand()&3);

		for (j=0; j<3; j++)
		{
			p->org[j] = move[j];
			p->accel[j] = 0;
		}
		if (left)
		{
			left = 0;
			p->vel[0] = 10;
		}
		else 
		{
			left = 1;
			p->vel[0] = -10;
		}

		p->vel[1] = 0;
		p->vel[2] = 0;

		VectorAdd (move, vec, move);
	}
}
#endif

/*
===============
CL_BubbleTrail

===============
*/
#if defined(PARTICLESYSTEM)
void CL_BubbleTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i;
	float		dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 32;
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec)
	{
		setupParticle (
			crand()*360, 0, 0,
			move[0]+crand()*2,	move[1]+crand()*2,	move[2]+crand()*2,
			crand()*5,	crand()*5,	crand()*5+10,
			0,		0,		0,
			255,	255,	255,
			0,	0,	0,
			1,		-0.1 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			(rand()%2 + 2)*0.333,	3,			
			particle_bubble,
			PART_SHADED,
			pBubbleThink,true);

		VectorAdd (move, vec, move);
	}
}
#else
void CL_BubbleTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i, j;
	cparticle_t	*p;
	float		dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 32;
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec)
	{
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear (p->accel);
		p->ptime = cl.stime;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.2);
		p->color = 4 + (rand()&7);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*2;
			p->vel[j] = crand()*5;
		}
		p->vel[2] += 6;

		VectorAdd (move, vec, move);
	}
}
#endif

/*
===============
CL_FlyParticles
===============
*/

#define	BEAMLENGTH			16
#if defined(PARTICLESYSTEM)
void CL_FlyParticles (vec3_t origin, int count)
{
	int			i;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;


	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	ltime = (float)cl.stime / 1000.0;
	for (i=0 ; i<count ; i+=2)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		dist = sin(ltime + i)*64;

		setupParticle (
			0,	0,	160 + rand()%40,
			origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH,origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH,
			origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH,
			0,	0,	0,
			255,	255,	255,
			0,	0,	0,
			0,	0,	0,
			1,		-100,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			1+sin(i+ltime),	1,			
			particle_fly1 + rand()%4,
			PART_SHADED,
			NULL,0);
	}
}
#else
void CL_FlyParticles (vec3_t origin, int count)
{
	int			i;
	cparticle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;


	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	ltime = (float)cl.stime / 1000.0;
	for (i=0 ; i<count ; i+=2)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;

		dist = sin(ltime + i)*64;
		p->org[0] = origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH;

		VectorClear (p->vel);
		VectorClear (p->accel);

		p->color = 0;
		p->colorvel = 0;

		p->alpha = 1;
		p->alphavel = -100;
	}
}
#endif
#if defined(PARTICLESYSTEM)
void CL_FlyEffect (centity_t *ent, vec3_t origin)
{
	float time = cl.stime;
	int		n;
	int		count;
	int		starttime;

	if (ent->fly_stoptime < time)
	{
		starttime = time;
		ent->fly_stoptime = time + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = time - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else
	{
		n = ent->fly_stoptime - time;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CL_FlyParticles (origin, count);
}
#else
void CL_FlyEffect (centity_t *ent, vec3_t origin)
{
	int		n;
	int		count;
	int		starttime;

	if (ent->fly_stoptime < cl.stime)
	{
		starttime = cl.stime;
		ent->fly_stoptime = cl.stime + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = cl.stime - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else
	{
		n = ent->fly_stoptime - cl.stime;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CL_FlyParticles (origin, count);
}
#endif

/*
===============
CL_BfgParticles
===============
*/

#define	BEAMLENGTH			16
#if defined(PARTICLESYSTEM)
void CL_BfgParticles (entity_t *ent)
{
	int			i;
	cparticle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64, dist2;
	vec3_t		v;
	float		ltime;
	
	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	ltime = (float)cl.stime / 1000.0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		dist2 = dist;
		dist = sin(ltime + i)*64;

		p = setupParticle (
			ent->origin[0],	ent->origin[1],	ent->origin[2],
			ent->origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH,ent->origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH,
			ent->origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH,
			0,	0,	0,
			0,	0,	0,
			50,	200*dist2,	20,
			0,	0,	0,
			1,		-100,
			GL_SRC_ALPHA, GL_ONE,
			1,			1,			
			particle_generic,
			PART_GLARE,
			pBFGThink, true);
		
		if (!p)
			return;

		VectorSubtract (p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0;
	}
}
#else
void CL_BfgParticles (entity_t *ent)
{
	int			i;
	cparticle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	vec3_t		v;
	float		ltime;
	
	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	ltime = (float)cl.stime / 1000.0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;

		dist = sin(ltime + i)*64;
		p->org[0] = ent->origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH;
		p->org[1] = ent->origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH;
		p->org[2] = ent->origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH;

		VectorClear (p->vel);
		VectorClear (p->accel);

		VectorSubtract (p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0;
		p->color = floor (0xd0 + dist * 7);
		p->colorvel = 0;

		p->alpha = 1.0 - dist;
		p->alphavel = -100;
	}
}
#endif

/*
===============
CL_TrapParticles
===============
*/
// RAFAEL
#if defined(PARTICLESYSTEM)
void CL_TrapParticles (entity_t *ent)
{
	int colors[][3] = {
		{255, 200, 150},
		{255, 200, 100},
		{255, 200, 50},
		{0, 0, 0}
	};
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start, end;
	float		len;
	int			dec, index;

	ent->origin[2]-=14;
	VectorCopy (ent->origin, start);
	VectorCopy (ent->origin, end);
	end[2]+=64;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 2.5;
	VectorScale (vec, dec, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		index = rand()&3;

		setupParticle (
			0,	0,	0,
			move[0] + crand(),	move[1] + crand(),	move[2] + crand(),
			crand()*15,	crand()*15,	crand()*15,
			0,	0,	PARTICLE_GRAVITY,
			colors[index][0],	colors[index][1],	colors[index][2],
			0,	0,	0,
			1,		-1.0 / (0.3+frand()*0.2),
			GL_SRC_ALPHA, GL_ONE,
			3,			-5,			
			particle_generic,
			0,
			NULL,0);

		VectorAdd (move, vec, move);
	}

	{

		int			i, j, k;
		float		vel;
		vec3_t		dir;
		vec3_t		org;

		
		ent->origin[2]+=14;
		VectorCopy (ent->origin, org);


		for (i=-2 ; i<=2 ; i+=4)
			for (j=-2 ; j<=2 ; j+=4)
				for (k=-2 ; k<=4 ; k+=4)
				{

					dir[0] = j * 8;
					dir[1] = i * 8;
					dir[2] = k * 8;
		
					VectorNormalize (dir);
					vel = 50 + (rand() & 63);

					index = rand()&3;

					setupParticle (
						0,	0,	0,
						org[0] + i + ((rand()&23) * crand()), org[1] + j + ((rand()&23) * crand()),	org[2] + k + ((rand()&23) * crand()),
						dir[0]*vel,	dir[1]*vel,	dir[2]*vel,
						0,	0,	0,
						colors[index][0],	colors[index][1],	colors[index][2],
						0,	0,	0,
						1,		-1.0 / (0.3+frand()*0.2),
						GL_SRC_ALPHA, GL_ONE,
						3,			-10,			
						particle_generic,
						PART_GRAVITY,
						NULL,0);
				}
	}
}
#else
void CL_TrapParticles (entity_t *ent)
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start, end;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;

	ent->origin[2]-=14;
	VectorCopy (ent->origin, start);
	VectorCopy (ent->origin, end);
	end[2]+=64;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear (p->accel);
		
		p->ptime = cl.stime;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3+frand()*0.2);
		p->color = 0xe0;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand()*15;
			p->accel[j] = 0;
		}
		p->accel[2] = PARTICLE_GRAVITY;

		VectorAdd (move, vec, move);
	}

	{

	
	int		i, k;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	
	ent->origin[2]+=14;
	VectorCopy (ent->origin, org);


	for (i=-2 ; i<=2 ; i+=4)
		for (j=-2 ; j<=2 ; j+=4)
			for (k=-2 ; k<=4 ; k+=4)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->ptime = cl.stime;
				p->color = 0xe0 + (rand()&3);

				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand()&7) * 0.02);
				
				p->org[0] = org[0] + i + ((rand()&23) * crand());
				p->org[1] = org[1] + j + ((rand()&23) * crand());
				p->org[2] = org[2] + k + ((rand()&23) * crand());
	
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
	}
}
#endif

/*
===============
CL_BFGExplosionParticles
===============
*/
//FIXME combined with CL_ExplosionParticles
#if defined(PARTICLESYSTEM)
void CL_BFGExplosionParticles (vec3_t org)
{
	float scale = 3;
	vec3_t vel;
	int			i;

	for (i=0 ; i<128 ; i++)
	{

		VectorSet(vel, crandom(), crandom(), crandom());
		VectorNormalize(vel);
		VectorScale(vel, scale*25.0f, vel);

		setupParticle (
			0,	0,	0,
			org[0] + ((rand()%32)-16),	org[1] + ((rand()%32)-16),	org[2] + ((rand()%32)-16),
			vel[0], vel[1], vel[2],
			0,		0,		0,
			255 + rand()%150,	255,	255,
			-300,	0,		-300,
			1,		-0.75 / (0.5 + frand()*0.3),
			GL_SRC_ALPHA, GL_ONE,
			scale,		-scale*0.5,			
			particle_generic,
			PART_GRAVITY|PART_DIRECTION|PART_GLARE,
			pBFGExplosionSparksThink, true);
	}
}

void CL_BFGExplosionParticles_2(vec3_t org) /* ECHON */
{
	int		i;
	float		randcolor, size;
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 8.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 3.0;	
		particleType = particle_bubble;
	}
	else {
		size = 6.0;	
		particleType = particle_generic;
	}

	for (i=0 ; i<256 ; i++) {
		randcolor = (rand()&1) ? 150 + (rand()%26) : 0.0f;
		setupParticle(
			0, 0, 0,
			org[0] + (crand () * 20), org[1] + (crand () * 20), org[2] + (crand () * 20),
			crand () * 50, crand () * 50, crand () * 50,
			0, 0, -40,
			randcolor, 75 + (rand()%150) + randcolor, (rand()%50) + randcolor,
			randcolor, 75 + (rand()%150) + randcolor, (rand()%50) + randcolor,
			1.0f, -0.8f / (0.8f + (frand () * 0.3f)),
		    	GL_SRC_ALPHA, GL_ONE,
			size + (crand () * 5.5f), 0.6f + (crand () * 0.5f),
			particleType,
			PART_GRAVITY,
			pBounceThink, false);
	}
}
#else
void CL_BFGExplosionParticles (vec3_t org)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<256 ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->ptime = cl.stime;
		p->color = 0xd0 + (rand()&7);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%384)-192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand()*0.3);
	}
}
#endif

/*
===============
CL_TeleportParticles

===============
*/
#if defined(PARTICLESYSTEM)
void CL_MakeTeleportParticles_Old (vec3_t org, float min, float max, float size, 
    int red, int green, int blue, particle_type particleType)
{
	cparticle_t	*p;
	int		i, j, k;
	float		vel, resize = 1;
	vec3_t		dir, temp;

	resize += size/10;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=min ; k<=max ; k+=4)
			{
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;

				VectorNormalize (dir);
				vel = (rand()&63);

				p = setupParticle(
					0, 0, 0,
					org[0]+ (i+(rand()&3))*resize, org[1]+(j+(rand()&3))*resize, org[2]+(k+(rand()&3))*resize,
					dir[0]*vel, dir[1]*vel, dir[2]*(25 + vel),
					0, 0, 0,
					red, green, blue,
					0, 0, 0,
					1, -0.75 / (0.3 + (rand()&7) * 0.02),
		    			GL_SRC_ALPHA, GL_ONE,
					(random()*.25+.75)*size*resize, 0,
					particleType,
					PART_GRAVITY|PART_ALPHACOLOR,
					NULL,0);

				if (!p)
					continue;

				VectorCopy(p->org, temp); temp[2] = org[2];
				VectorSubtract(org, temp, p->vel);
				p->vel[2]+=25;
				
				VectorScale(p->vel, 3, p->accel);
			}
}

void CL_MakeTeleportParticles_Opt1(vec3_t org, float min, float max, float size,
    int red, int green, int blue, particle_type particleType)
{
	cparticle_t	*p;
	int		i, j, k;
	float		vel, resize = 1;
	vec3_t		dir;

	resize += size/100;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=min ; k<=max ; k+=4)
			{
				dir[0] = j*16;
				dir[1] = i*16;
				dir[2] = k*16;

				VectorNormalize(dir);
				vel = 150 + (rand()&63);

				p = setupParticle(
				        0, 0, 0,
					org[0]+ (i+(rand()&3)) * resize, 
					org[1]+(j+(rand()&3)) * resize, 
					org[2]+(k+(rand()&3)) * resize,
					dir[0]*vel, dir[1]*vel, dir[2]*(25 + vel),
					0, 0, 0,
		    			200 + 55 * rand(), 200 + 55 * rand(), 200 + 55 * rand(),
					0, 0, 0,
		    			2.5 , -1 / (0.2 + (rand() & 7) * 0.01),
		    			GL_SRC_ALPHA, GL_ONE,
					(random()*.25+.75)*size*resize, 0,
					particleType,
					PART_GRAVITY|PART_ALPHACOLOR,
					pRotateThink, 0);

				if (!p)
					continue;
			}
}

void CL_MakeTeleportParticles_Opt2(vec3_t org, float min, float max, float size,
    int red, int green, int blue, particle_type particleType)
{
	int		i, j, k;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=-16 ; k<=32 ; k+=4) {
				dir[0] = j*16;
				dir[1] = i*16;
				dir[2] = k*16;

				VectorNormalize (dir);
				vel = 150 + (rand()&63);

				setupParticle (
				    0, 0, 0,
				    org[0]+i+(rand()&3), org[1]+j+(rand()&3), org[2]+k+(rand()&3),
				    dir[0]*vel, dir[1]*vel, dir[2]*vel,
				    0, 0, 0,
				    200 + 55*rand(), 200 + 55*rand(), 200 + 55*rand(),
				    0, 0, 0,
				    1, -1.0 / (0.3 + (rand()&7) * 0.02),
				    GL_SRC_ALPHA, GL_ONE,
				    2, 3, 
				    particleType,
				    PART_GRAVITY|PART_ALPHACOLOR,
				    NULL,0);
			}
}

void CL_MakeTeleportParticles_Rotating(vec3_t org, float min, float max, float size, float grow,
    int red, int green, int blue, particle_type particleType)
{
	int		i;
	vec3_t		origin , vel;

	for (i = 0; i < 128; i++) {
		VectorSet(origin,
		    org[0] + rand() % 30 - 15,
		    org[1] + rand() % 30 - 15,
		    org[2] + min + random() * (max - min));

		VectorSubtract(org, origin, vel);
		VectorNormalize(vel);

		setupParticle(
		    random() * 360, crandom() * 45, 0,
		    origin[0], origin[1], origin[2],
		    vel[0] * 5, vel[1] * 5, vel[2] * 5,
		    vel[0] * 20, vel[1] * 20, vel[2] * 5,
		    200 + 55 * rand(), 200 + 55 * rand(), 200 + 55 * rand(),
		    0, 0, 0,
		    2, -0.25 / (1.0 + (rand() & 7) * 0.02),
		    GL_SRC_ALPHA, GL_ONE,
		    (random() * 1.0 + 1.0) * size, grow,
		    particleType,
		    PART_GRAVITY|PART_ALPHACOLOR,
		    pRotateThink, true);
	}
}

void CL_TeleportParticles(vec3_t org)
{
	extern cvar_t	*cl_teleport_particles;
        
        if (cl_teleport_particles->value == 4) {
		CL_MakeTeleportParticles_Rotating (org, -16, 32, 
		   2.5, 0.0 , 255, 255, 200, particle_lensflare);
        }      
        else if (cl_teleport_particles->value == 3) {
		CL_MakeTeleportParticles_Rotating (org, -16, 32, 
		   0.8, 0.0 , 255, 255, 200, particle_bubble);
        }     
        else if (cl_teleport_particles->value == 2) {
		CL_MakeTeleportParticles_Opt2(org, -16, 32,
	    	   3.0, 255, 255, 200, particle_lensflare);
        }
        else if (cl_teleport_particles->value == 1)  {
		CL_MakeTeleportParticles_Opt1(org, -16, 32,
	    	   2.0, 255, 255, 200, particle_bubble);
	}
        else 
		CL_MakeTeleportParticles_Old(org, -16, 32,
	    	   1.0, 200, 255, 255, particle_generic);
}

void CL_Disintegrate(vec3_t pos, int ent)
{
	CL_MakeTeleportParticles_Old(pos, -16, 24,
	    7.5, 100, 100, 255, particle_smoke);
}

void CL_FlameBurst(vec3_t pos, float size)
{
	CL_MakeTeleportParticles_Old(pos, -16, 24,
	    size, 255, 255, 255, particle_inferno);
}

void CL_LensFlare(vec3_t pos, vec3_t color, float size, float time)
{
	setupParticle (
		0,	0,	0,
		pos[0], pos[1], pos[2],
		0, 0, 0,
		0, 0, 0,
		color[0], color[1], color[2],
		0,	0,	0,
		1,		- 2.0f/time,
		GL_SRC_ALPHA, GL_ONE,
		size,	0,
		particle_lensflare,
		PART_LENSFLARE,
		pLensFlareThink, true);
}
#else
void CL_TeleportParticles (vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=-16 ; k<=32 ; k+=4)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->ptime = cl.stime;
				p->color = 7 + (rand()&7);

				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand()&7) * 0.02);
				
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
}
#endif

/*
===============
CL_AddParticles
===============
*/
#if defined(PARTICLESYSTEM)
void CL_AddParticles (void)
{
	cparticle_t		*p, *next;
	float			alpha, size, light;
	float			time1= 0.0f, time2;
	vec3_t			org, color, angle;
	int				i, image, decals, parts, flags;
	cparticle_t		*active, *tail;

	active = NULL;
	tail = NULL;

	decals = parts = 0;

	if (cl_paused->value)
		return;

	if (newParticles)
	{
		newParticles->next = active_particles;
		active_particles = newParticles;
		newParticles = NULL;
	}
	runningParticle = true;

	for (p=active_particles ; p ; p=next)
	{
		next = p->next;

		flags = p->flags;

		if (flags&PART_DECAL) {
				
			//this fixes jumpy particles
			if (cl.stime>p->ptime)
				p->ptime = cl.stime;

			time1 = (p->ptime - p->start)*0.001;
			alpha = p->alpha + time1*p->alphavel;

			if (decals >= r_decals->value || alpha <= 0)
			{	// faded out
				p->alpha = 0;
				p->flags = 0;
				p->next = free_particles;
				p->decalnum = 0;
				free_particles = p;
				continue;
			}
			
			p->next = NULL;
			if (!tail)
				active = tail = p;
			else
			{
				tail->next = p;
				tail = p;
			}
		}
		else if (flags & PART_INSTANT)
		{
			if (cl.stime>p->ptime)
				p->ptime = cl.stime;
				
			time1 = (p->ptime - p->start)*0.001;
			alpha = p->alpha;

			p->ptime  = 0;
			p->start = 0;
			p->flags = 0;
			p->alpha = 0;
			p->next  = free_particles;
			free_particles = p;
		}
		else
		{
			//this fixes jumpy particles
			if (cl.stime>p->ptime)
				p->ptime = cl.stime;

			time1 = (p->ptime - p->start)*0.001;
			alpha = p->alpha + time1*p->alphavel;

			if (alpha <= 0)
			{	// faded out
				p->alpha = 0;
				p->flags = 0;
				p->next = free_particles;
				free_particles = p;
				continue;
			}
			
			p->next = NULL;
			if (!tail)
				active = tail = p;
			else
			{
				tail->next = p;
				tail = p;
			}
		}

		if (alpha > 1.0)
			alpha = 1;
		if (alpha < 0.0)
			alpha = 0;

		time2 = time1*time1;
		image = p->image;

		for (i=0;i<3;i++)
		{
			color[i] = p->color[i] + p->colorvel[i]*time1;
			if (color[i]>255) color[i]=255;
			if (color[i]<0) color[i]=0;
			
			angle[i] = p->angle[i];
			org[i] = p->org[i] + p->vel[i]*time1 + p->accel[i]*time2;
		}

		if (flags&PART_GRAVITY)
		{
			org[2]+=time2*-PARTICLE_GRAVITY;
		}

		size = p->size + p->sizevel*time1;

		for (i=0;i<P_LIGHTS_MAX;i++)
		{
			const cplight_t *plight = &p->lights[i];
			if (plight->isactive)
			{
				light = plight->light*alpha + plight->lightvel*time1;
				V_AddLight (org, light, plight->lightcol[0], plight->lightcol[1], plight->lightcol[2]);
			}
		}

		if (p->thinknext && p->think)
		{
			p->thinknext = false;
			p->think(p, org, angle, &alpha, &size, &image, &time1);
		}

		if (flags & PART_DECAL)
		{
			if (p->decalnum)
			{
				for (i=0; i<p->decalnum; i++)
				{
					V_AddParticle (org, angle, color, alpha, 
							p->blendfunc_src, p->blendfunc_dst, size, image, flags, &p->decal[i]);
				}
			}
			else
				V_AddParticle (org, angle, color, alpha, p->blendfunc_src, 
						p->blendfunc_dst, size, image, flags, NULL);

			decals++;
		}
		else
		{
			if (parts > r_particle->value)
				continue;

			V_AddParticle (org, angle, color, alpha, p->blendfunc_src, 
					p->blendfunc_dst, size, image, flags, NULL);

			parts++;
		}
		VectorCopy(org, p->oldorg);
	}
	active_particles = active;
	runningParticle = false;
}
#else
void CL_AddParticles (void)
{
	cparticle_t		*p, *next;
	float			alpha;
	float			time1= 0.0f, time2;
	vec3_t			org;
	int			color;
	cparticle_t		*active, *tail;

	active = NULL;
	tail = NULL;

	for (p=active_particles ; p ; p=next)
	{
		next = p->next;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if (p->alphavel != INSTANT_PARTICLE)
		{
			time1 = (cl.stime - p->ptime)*0.001;
			alpha = p->alpha + time1*p->alphavel;
			if (alpha <= 0)
			{	// faded out
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		}
		else
		{
			time1 = 0.0;
			alpha = p->alpha;
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else
		{
			tail->next = p;
			tail = p;
		}

		if (alpha > 1.0)
			alpha = 1;
		color = p->color;

		time2 = time1*time1;

		org[0] = p->org[0] + p->vel[0]*time1 + p->accel[0]*time2;
		org[1] = p->org[1] + p->vel[1]*time1 + p->accel[1]*time2;
		org[2] = p->org[2] + p->vel[2]*time1 + p->accel[2]*time2;

		V_AddParticle (org, color, alpha);
		// PMM
		if (p->alphavel == INSTANT_PARTICLE)
		{
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
	}

	active_particles = active;
}
#endif

/*
==============
CL_ClearEffects

==============
*/
void CL_ClearEffects (void)
{
	CL_ClearParticles ();
	CL_ClearDlights ();
	CL_ClearLightStyles ();
}

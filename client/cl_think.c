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
// cl_think.c

#include "client.h"

#if defined(PARTICLESYSTEM)

void vectoanglerolled (vec3_t value1, float angleyaw, vec3_t angles)
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

void calcPartVelocity(cparticle_t *p, float scale, float *time, vec3_t velocity)
{
	float time1 = *time;
	float time2 = time1*time1;

	velocity[0] = scale * (p->vel[0]*time1 + (p->accel[0])*time2);
	velocity[1] = scale * (p->vel[1]*time1 + (p->accel[1])*time2);

	if (p->flags & PART_GRAVITY)
		velocity[2] = scale * (p->vel[2]*time1 + (p->accel[2]-(PARTICLE_GRAVITY))*time2);
	else
		velocity[2] = scale * (p->vel[2]*time1 + (p->accel[2])*time2);
}

void ClipVelocity (vec3_t in, vec3_t normal, vec3_t out)
{
	float	backoff, change;
	int		i;
	
	backoff = VectorLength(in)*0.25 + DotProduct (in, normal) * 3.0f;

	for (i=0 ; i<3 ; i++) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
}

void pBounceThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float clipsize;
	trace_t tr;
	vec3_t velocity;

	clipsize = *size*0.5;
	if (clipsize<0.25) clipsize = 0.25;
	tr = CL_Trace (p->oldorg, org, clipsize, 1);
	
	if (tr.fraction<1) {
		calcPartVelocity(p, 1, time, velocity);
		ClipVelocity(velocity, tr.plane.normal, p->vel);

		VectorCopy(vec3_origin, p->accel);
		VectorCopy(tr.endpos, p->org);
		VectorCopy(p->org, org);
		VectorCopy(p->org, p->oldorg);

		p->alpha = *alpha;
		p->size = *size;

		p->start = p->ptime = cl.stime;

		if (p->flags&PART_GRAVITY && VectorLength(p->vel)<2)
			p->flags &= ~PART_GRAVITY;
	}

	p->thinknext = true;
}

void pBubbleThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	if (CM_PointContents(org,0) & MASK_WATER) {
		if (*size>2)
			*size = 2;
		
		org[0] = org[0] + sin((org[0] + *time) * 0.05) * *size;
		org[1] = org[1] + cos((org[1] + *time) * 0.05) * *size;

		p->thinknext = true;
		pRotateThink (p, org, angle, alpha, size, image, time);
	}
	else {
		p->think = NULL;
		p->alpha = 0;
	}
}

void pWaterExplosionThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{

	*size = *size * (0.75+0.75*sin(*time*M_PI*4.0));

	pRotateThink (p, org, angle, alpha, size, image, time);
}

void pExplosionBubbleThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	if (CM_PointContents(org, 0) & MASK_WATER)
		p->thinknext = true;
	else {
		p->think = NULL;
		p->alpha = 0;
	}
}

void pDisruptExplosionThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{

	if (*alpha>.66666)
		*image = particle_dexplosion1;
	else if (*alpha>.33333)
		*image = particle_dexplosion2;
	else
		*image = particle_dexplosion3;

	*alpha *= 3.0;

	if (*alpha > 1.0)
		*alpha = 1;

	p->thinknext = true;
}

void pBloodDropThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float length;
	vec3_t len;

	VectorSubtract(p->angle, org, len);
	{
		calcPartVelocity(p, 0.2, time, angle);

		length = VectorNormalize(angle);
		if (length>MAXBLEEDSIZE) length = MAXBLEEDSIZE;
			VectorScale(angle, -length, angle);
	}

	//now to trace for impact...
	pBloodThink (p, org, angle, alpha, size, image, time);

}

void pBloodThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	trace_t trace = CL_Trace (p->oldorg, org, 0.1, 1);

	if (trace.fraction < 1.0) { //impact with surface
		float alphaX = *alpha*5.0 + 100;
		
		if (alphaX>255)
			alphaX = 255;
			
		if (r_decals->value) {
			vec3_t normal, angle;
				
			VectorNegate(trace.plane.normal, normal);

			if (!VectorCompare(normal, vec3_origin)) {
				vectoanglerolled(normal, rand()%180-90, angle);
				setupParticle(angle[0], angle[1], angle[2],
					      trace.endpos[0], trace.endpos[1], trace.endpos[2],
					      0, 0, 0,
					      0, 0, 0,
					      255, 255, 255,
					      0, 0, 0,
					      alphaX, -0.005,
					      GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
					      *size*(random()*5.0+5), 0,
					      particleBlood(),
					      PART_DECAL|PART_ALPHACOLOR|PART_DECAL_SUB,
					      pBloodThink, true);
			}
		}

		*alpha=0;
		*size=0;
		p->alpha = 0;
	}
	else
		*size *= 3.5f;

	VectorCopy(org, p->oldorg);

	p->thinknext = true;
}

void pWaterSplashThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float len;

	calcPartVelocity(p, 1, time, angle);
	VectorNormalize(angle);

	len = *alpha * WaterSplashSize;
	if (len<1)
		len=1;

	VectorScale(angle, len, angle);
	
	p->thinknext = true;
}

void pSplashThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float len;

	calcPartVelocity(p, 1, time, angle);
	VectorNormalize(angle);

	len = *alpha * SplashSize;
	if (len<1)
		len=1;

	VectorScale(angle, len, angle);

	p->thinknext = true;
}

void pSparksThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
#if 1
	calcPartVelocity(p, 0.75, time, angle);
#else
	/* setting up angle for sparks */
	{
		int	i;
		float	time1, time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.75 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.75 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);
	}
#endif
	p->thinknext = true;
}

void pDecalAlphaThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	*alpha = sqrt(*alpha);

	p->thinknext = true;
}

void pExplosionSparksThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
#if 1
	pBounceThink (p, org, angle, alpha, size, image, time);
	calcPartVelocity(p, 0.33333333, time, angle);
#else

	/* setting up angle for sparks */
	{
		int	i;
		float	time1, time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.25 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.25 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);

	}
#endif
	p->thinknext = true;
}

void pBlasterThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	vec_t  length;
	vec3_t len;
	VectorSubtract(p->angle, org, len);

	*size *= (float)(pBlasterMaxSize/VectorLength(len)) * 1.0/((pBlasterMaxSize-*size));
	*size += *time * p->sizevel;

	if (*size > pBlasterMaxSize)
		*size = pBlasterMaxSize;
	if (*size < pBlasterMinSize)
		*size = pBlasterMinSize;

	pBounceThink (p, org, angle, alpha, size, image, time);

	length = VectorNormalize(p->vel);
	if (length>pBlasterMaxVelocity)
		VectorScale(p->vel,	pBlasterMaxVelocity,	p->vel);
	else
		VectorScale(p->vel,	length, p->vel);
}

void pBFGThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	vec3_t len;
	VectorSubtract(p->angle, p->org, len);
	
	*size = (float)((300/VectorLength(len))*0.75);
	if (*size > 50) *size = 50;
}

void pBFGExplosionSparksThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	pBounceThink (p, org, angle, alpha, size, image, time);
	calcPartVelocity(p, 0.1, time, angle);

	p->thinknext = true;
}

void pLensFlareThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	angle[2] = anglemod(cl.refdef.viewangles[YAW]);	

	p->thinknext = true;
}

void pRotateThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	angle[2] = angle[0] + *time*angle[1] + *time**time*angle[2];
	p->thinknext=true;
}

void pBlaster2Think (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	vec3_t len;
	VectorSubtract(p->angle, org, len);
	
	*size *= (float)(pBlasterMaxSize/VectorLength(len)) * 1.0/((4-*size));
	if (*size > pBlasterMaxSize)
		*size = pBlasterMaxSize;

	p->thinknext = true;
}

void pExplosionThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{

	if (*alpha > .85)
		*image = particle_rexplosion1;
	else if (*alpha > .7)
		*image = particle_rexplosion2;
	else if (*alpha > .5)
		*image = particle_rexplosion3;
	else if (*alpha > .4)
		*image = particle_rexplosion4;
	else if (*alpha > .25)
		*image = particle_rexplosion5;
	else if (*alpha > .1)
		*image = particle_rexplosion6;
	else
		*image = particle_rexplosion7;

	*alpha *= 3.0;

	if (*alpha > 1.0)
		*alpha = 1;

	p->thinknext = true;
}

void pDevRailThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	int		i;
	vec3_t		len;

	VectorSubtract(p->angle, org, len);

	*size *= (float)(SplashSize / VectorLength(len)) * 0.5 / ((4 - *size));
	if (*size > SplashSize)
		*size = SplashSize;

	/* setting up angle for sparks */
	{
		float	time1, time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 3 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 3 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);
	}

	p->thinknext = true;
}

void pElectricSparksThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	int		i;

	/* setting up angle for sparks */
	{
		float		time1, time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.25 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.25 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);
	}

	p->thinknext = true;
}
#endif

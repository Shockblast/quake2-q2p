/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
/* Decals, from egl, orginally from qfusion? */
#include "gl_local.h"

#if defined(PARTICLESYSTEM)
static qboolean		cm_markSplash;
static int		cm_numMarkPoints;
static int		cm_maxMarkPoints;
static vec3_t		*cm_markPoints;
static vec2_t		*cm_markTCoords;

msurface_t				*cm_markSurface;

static int		cm_numMarkFragments;
static int		cm_maxMarkFragments;
static markFragment_t	*cm_markFragments;

static cplane_t		cm_markPlanes[MAX_FRAGMENT_PLANES];
static int		cm_markCheckCount;
/*
 =================
 Mod_ClipFragment
 =================
*/

float *worldVert (int i, msurface_t *surf)
{
	int e = r_worldmodel->surfedges[surf->firstedge + i];
	if (e >= 0)
		return &r_worldmodel->vertexes[r_worldmodel->edges[e].v[0]].position[0];
	else
		return &r_worldmodel->vertexes[r_worldmodel->edges[-e].v[1]].position[0];

}

static void Mod_ClipFragment (int numPoints, vec3_t points, int stage, markFragment_t *mf)
{
	int			i;
	float		*p;
	qboolean	frontSide;
	vec3_t		front[MAX_FRAGMENT_POINTS];
	int			f;
	float		dist;
	float		dists[MAX_FRAGMENT_POINTS];
	int			sides[MAX_FRAGMENT_POINTS];
	cplane_t	*plane;

	if (numPoints > MAX_FRAGMENT_POINTS-2)
		ri.Sys_Error(ERR_DROP, "Mod_ClipFragment: MAX_FRAGMENT_POINTS hit");

	if (stage == MAX_FRAGMENT_PLANES)
	{
		// Fully clipped
		if (numPoints > 2)
		{
			mf->numPoints = numPoints;
			mf->firstPoint = cm_numMarkPoints;
			
			if (cm_numMarkPoints + numPoints > cm_maxMarkPoints)
				numPoints = cm_maxMarkPoints - cm_numMarkPoints;

			for (i = 0, p = points; i < numPoints; i++, p += 3)
				VectorCopy(p, cm_markPoints[cm_numMarkPoints+i]);

			cm_numMarkPoints += numPoints;
		}

		return;
	}

	frontSide = false;

	plane = &cm_markPlanes[stage];
	for (i = 0, p = points; i < numPoints; i++, p += 3)
	{
		if (plane->type < 3)
			dists[i] = dist = p[plane->type] - plane->dist;
		else
			dists[i] = dist = DotProduct(p, plane->normal) - plane->dist;

		if (dist > ON_EPSILON)
		{
			frontSide = true;
			sides[i] = SIDE_FRONT;
		}
		else if (dist < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
			sides[i] = SIDE_ON;
	}

	if (!frontSide)
		return;		// Not clipped

	// Clip it
	dists[i] = dists[0];
	sides[i] = sides[0];
	VectorCopy(points, (points + (i*3)));

	f = 0;

	for (i = 0, p = points; i < numPoints; i++, p += 3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy(p, front[f]);
			f++;
			break;
		case SIDE_BACK:
			break;
		case SIDE_ON:
			VectorCopy(p, front[f]);
			f++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		dist = dists[i] / (dists[i] - dists[i+1]);

		front[f][0] = p[0] + (p[3] - p[0]) * dist;
		front[f][1] = p[1] + (p[4] - p[1]) * dist;
		front[f][2] = p[2] + (p[5] - p[2]) * dist;

		f++;
	}

	// Continue with planes
	Mod_ClipFragment(f, front[0], stage+1, mf);
}

/*
 =================
 Mod_SplashFragTexCoords
 =================
*/

void Mod_SplashFragTexCoords (vec3_t axis[3], const vec3_t origin, float radius, markFragment_t *fr)
{
	int j;
	vec3_t v;

	if (!cm_markSplash)
		return;

	for ( j = 0; j < fr->numPoints; j++ ) 
	{
		VectorSubtract (cm_markPoints[fr->firstPoint+j], origin, v );
		cm_markTCoords[fr->firstPoint+j][0] = DotProduct ( v, axis[0] ) + 0.5f;
		cm_markTCoords[fr->firstPoint+j][1] = DotProduct ( v, axis[1] ) + 0.5f;
	}
}

/*
 =================
 Mod_ClipFragmentToSurface
 =================
*/
static void Mod_ClipFragmentToSurface (const vec3_t origin, const float radius, msurface_t *surf, const vec3_t normal)
{
	qboolean planeback = surf->flags & SURF_PLANEBACK;
	int		i;
	vec3_t		points[MAX_FRAGMENT_POINTS];
	markFragment_t	*mf;
	vec3_t axis[3];

	if (cm_numMarkPoints >= cm_maxMarkPoints || cm_numMarkFragments >= cm_maxMarkFragments)
		return;		// Already reached the limit somewhere else

	if (cm_markSplash)
	{
		float dot, dist;

		cm_markSurface = surf;

		//First check for facing planes
		if ( surf->plane->type < 3 )
			dist = origin[surf->plane->type] - surf->plane->dist;
		else
			dist = DotProduct (origin, surf->plane->normal) - surf->plane->dist;
		// cull the polygon
		if (dist > ON_EPSILON && (surf->flags & SURF_PLANEBACK) )
				return;
		else if (dist < -ON_EPSILON && !(surf->flags & SURF_PLANEBACK) )
				return;

		VectorNormalize2 ( cm_markSurface->plane->normal, axis[0] );
		PerpendicularVector ( axis[1], axis[0] );
		RotatePointAroundVector ( axis[2], axis[0], axis[1], 0 );
		CrossProduct ( axis[0], axis[2], axis[1] );

		for (i = 0; i < 3; i++)
		{
			dot = DotProduct(origin, axis[i]);

			VectorCopy(axis[i], cm_markPlanes[i*2+0].normal);
			cm_markPlanes[i*2+0].dist = dot - radius;
			cm_markPlanes[i*2+0].type = PlaneTypeForNormal(cm_markPlanes[i*2+0].normal);

			VectorNegate(axis[i], cm_markPlanes[i*2+1].normal);
			cm_markPlanes[i*2+1].dist = -dot - radius;
			cm_markPlanes[i*2+1].type = PlaneTypeForNormal(cm_markPlanes[i*2+1].normal);
		}

		VectorScale ( axis[1], 0.5f / radius, axis[0] );
		VectorScale ( axis[2], 0.5f / radius, axis[1] );
	}
	else
	{
		float d = DotProduct(normal, surf->plane->normal);
		if ((planeback && d > -0.75) || (!planeback && d < 0.75))
			return;		// Greater than X degrees
	}

	for (i = 2; i < surf->numedges; i++)
	{
		mf = &cm_markFragments[cm_numMarkFragments];
		mf->firstPoint = mf->numPoints = 0;
		
		VectorCopy(worldVert(0, surf), points[0]);
		VectorCopy(worldVert(i-1, surf), points[1]);
		VectorCopy(worldVert(i, surf), points[2]);

		Mod_ClipFragment(3, points[0], 0, mf);

		if (mf->numPoints)
		{
			Mod_SplashFragTexCoords (axis, origin, radius, mf);
			cm_numMarkFragments++;

			if (cm_numMarkPoints >= cm_maxMarkPoints || cm_numMarkFragments >= cm_maxMarkFragments)
				return;
		}
	}
}

/*
 =================
 Mod_RecursiveMarkFragments
 =================
*/
static void Mod_RecursiveMarkFragments (const vec3_t origin, const vec3_t normal, float radius, mnode_t *node)
{

	int			i;
	float		dist;
	cplane_t	*plane;
	msurface_t	*surf;

	if (cm_numMarkPoints >= cm_maxMarkPoints || cm_numMarkFragments >= cm_maxMarkFragments)
		return;		// Already reached the limit somewhere else

	if (node->contents != -1)
		return;

	// Find which side of the node we are on
	plane = node->plane;
	if (plane->type < 3)
		dist = origin[plane->type] - plane->dist;
	else
		dist = DotProduct(origin, plane->normal) - plane->dist;
	
	// Go down the appropriate sides
	if (dist > radius)
	{
		Mod_RecursiveMarkFragments(origin, normal, radius, node->children[0]);
		return;
	}
	if (dist < -radius)
	{
		Mod_RecursiveMarkFragments(origin, normal, radius, node->children[1]);
		return;
	}
	// Clip against each surface

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->checkCount == cm_markCheckCount)
			continue;	// Already checked this surface in another node

		if (surf->texinfo->flags & (SURF_SKY|SURF_WARP))//|SURF_TRANS33|SURF_TRANS66|(0x1000000)))
			continue;

		surf->checkCount = cm_markCheckCount;

		Mod_ClipFragmentToSurface(origin, radius, surf, normal);
	}
	// Recurse down the children
	Mod_RecursiveMarkFragments(origin, normal, radius, node->children[0]);
	Mod_RecursiveMarkFragments(origin, normal, radius, node->children[1]);
}

/*
 =================
 Mod_SetTexCoords
 =================
*/

void Mod_SetTexCoords (const vec3_t origin, vec3_t axis[3], float radius)
{
	int i, j;
	vec3_t newaxis[2], v;
	markFragment_t *fr;

	VectorScale ( axis[1], 0.5f / radius, newaxis[0] );
	VectorScale ( axis[2], 0.5f / radius, newaxis[1] );

	for ( i = 0, fr = cm_markFragments; i < cm_numMarkFragments; i++, fr++ )
		for ( j = 0; j < fr->numPoints; j++ ) 
		{
			VectorSubtract (cm_markPoints[fr->firstPoint+j], origin, v );
			cm_markTCoords[fr->firstPoint+j][0] = DotProduct ( v, newaxis[0] ) + 0.5f;
			cm_markTCoords[fr->firstPoint+j][1] = DotProduct ( v, newaxis[1] ) + 0.5f;
		}
}

/*
 =================
 R_AddDecal
 =================
*/
int R_AddDecal(const vec3_t origin,  vec3_t axis[3], float radius, int maxPoints, vec3_t *points, vec2_t *tcoords, int maxFragments, markFragment_t *fragments)
{

	int		i;
	float	dot;

	if (!r_worldmodel || !r_worldmodel->nodes)
		return 0;			// Map not loaded

	cm_markCheckCount++;	// For multi-check avoidance

	// Initialize fragments
	cm_numMarkPoints = 0;
	cm_maxMarkPoints = maxPoints;
	cm_markPoints = points;

	cm_markTCoords = tcoords;

	cm_numMarkFragments = 0;
	cm_maxMarkFragments = maxFragments;
	cm_markFragments = fragments;

	if (VectorCompare(axis[0], vec3_origin))
		cm_markSplash = true;
	else
		cm_markSplash = false;

	// Calculate clipping planes
	if (!cm_markSplash)
		for (i = 0; i < 3; i++)
		{
			dot = DotProduct(origin, axis[i]);

			VectorCopy(axis[i], cm_markPlanes[i*2+0].normal);
			cm_markPlanes[i*2+0].dist = dot - radius;
			cm_markPlanes[i*2+0].type = PlaneTypeForNormal(cm_markPlanes[i*2+0].normal);

			VectorNegate(axis[i], cm_markPlanes[i*2+1].normal);
			cm_markPlanes[i*2+1].dist = -dot - radius;
			cm_markPlanes[i*2+1].type = PlaneTypeForNormal(cm_markPlanes[i*2+1].normal);
		}

	// Clip against world geometry
	Mod_RecursiveMarkFragments(origin, axis[0], radius, r_worldmodel->nodes);

	if (!cm_markSplash)
		Mod_SetTexCoords(origin, axis, radius);

	return cm_numMarkFragments;
}
#else
typedef struct cdecal_t {
	struct cdecal_t *prev, *next;
	float		time;

	int		numverts;
	vec3_t		verts[MAX_DECAL_VERTS];
	vec2_t		stcoords[MAX_DECAL_VERTS];
	mnode_t        *node;

	vec3_t		direction;

	vec4_t		color;
	vec3_t		org;

	int		type;
	int		flags;
	
	float		endTime;
	vec4_t		endColor;

} cdecal_t;

typedef struct {
	int		firstvert;
	int		numverts;
	mnode_t        *node;
	msurface_t     *surf;
} fragment_t;

static cdecal_t	decals[MAX_DECALS];
static cdecal_t	active_decals, *free_decals;

int	R_GetClippedFragments(vec3_t origin, float radius, mat3_t axis, int maxfverts, vec3_t * fverts, int maxfragments, fragment_t * fragments);

/*
** GL_ClearDecals
*/
void GL_ClearDecals(void)
{
	int		i;

	memset(decals, 0, sizeof(decals));

	/* link decals */
	free_decals = decals;
	active_decals.prev = &active_decals;
	active_decals.next = &active_decals;
	for (i = 0; i < MAX_DECALS - 1; i++)
		decals[i].next = &decals[i + 1];
}

/*
** GL_AllocDecal
** Returns either a free decal or the oldest one
*/
static cdecal_t *GL_AllocDecal(void)
{
	cdecal_t       *dl;

	if (free_decals) {	/* take a free decal if possible */
		dl = free_decals;
		free_decals = dl->next;
	} else {		/* grab the oldest one otherwise */
		dl = active_decals.prev;
		dl->prev->next = dl->next;
		dl->next->prev = dl->prev;
	}

	/* put the decal at the start of the list */
	dl->prev = &active_decals;
	dl->next = active_decals.next;
	dl->next->prev = dl;
	dl->prev->next = dl;

	return dl;
}

/*
**GL_FreeDecal
*/
static void GL_FreeDecal(cdecal_t * dl)
{
	if (!dl->prev)
		return;

	/* remove from linked active list */
	dl->prev->next = dl->next;
	dl->next->prev = dl->prev;

	/* insert into linked free list */
	dl->next = free_decals;
	free_decals = dl;
}

/*
** R_AddDecal
*/
void R_AddDecal(vec3_t origin, vec3_t dir, float red, float green, float blue, 
                float alpha, float endRed, float endGreen, float endBlue, 
	        float endAlpha, float size, int type, int flags, float angle)
{
	int		i, j, numfragments;
	vec3_t		verts[MAX_DECAL_VERTS], shade;
	fragment_t     *fr, fragments[MAX_DECAL_FRAGMENTS];
	mat3_t		axis;
	cdecal_t       *d;

	if (!r_decals->value)
		return;

	/* invalid decal */
	if (size <= 0 || VectorCompare(dir, vec3_origin))
		return;

	/* calculate orientation matrix */
	VectorNormalize2(dir, axis[0]);
	PerpendicularVector(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], angle);
	CrossProduct(axis[0], axis[2], axis[1]);

	numfragments = R_GetClippedFragments(origin, size, axis,	/* clip it */
	    MAX_DECAL_VERTS, verts, MAX_DECAL_FRAGMENTS, fragments);

	/* no valid fragments */
	if (!numfragments)
		return;

	VectorScale(axis[1], 0.5f / size, axis[1]);
	VectorScale(axis[2], 0.5f / size, axis[2]);

	for (i = 0, fr = fragments; i < numfragments; i++, fr++) {
		if (fr->numverts > MAX_DECAL_VERTS)
			fr->numverts = MAX_DECAL_VERTS;
		else if (fr->numverts <= 0)
			continue;


		d = GL_AllocDecal();

		d->time = r_newrefdef.time;
		d->endTime = r_newrefdef.time + 20000; /* 20 sec */

		d->numverts = fr->numverts;
		d->node = fr->node;

		VectorCopy(fr->surf->plane->normal, d->direction);
		/* reverse direction */
		if (!(fr->surf->flags & SURF_PLANEBACK))
			VectorNegate(d->direction, d->direction);
		
		VectorCopy(origin, d->org);

		Vector4Set(d->color, red, green, blue, alpha);
		Vector4Set(d->endColor, endRed, endGreen, endBlue, endAlpha);

		if (flags & DF_SHADE) {
			R_LightPoint(origin, shade, false);

			for (j = 0; j < 3; j++)
				d->color[j] = (d->color[j] * shade[j] * 0.6) + (d->color[j] * 0.4);
		}
		d->type = type;
		d->flags = flags;

		for (j = 0; j < fr->numverts; j++) {
			vec3_t		v;

			VectorCopy(verts[fr->firstvert + j], d->verts[j]);
			VectorSubtract(d->verts[j], origin, v);
			d->stcoords[j][0] = ClampCvar(0.0, 1.0, DotProduct (v, axis[1]) + 0.5f);
			d->stcoords[j][1] = ClampCvar(0.0, 1.0, DotProduct (v, axis[2]) + 0.5f);
		}
	}
}


/*
** R_AddDecals
*/
void R_AddDecals(void)
{
	cdecal_t       *dl, *next, *active;
	float		mindist, time, lerp, endLerp;
	int		i, x, r_numdecals = 0;
	vec3_t		v;
	vec4_t		color, fColor;

	if (!r_decals->value)
		return;

	active = &active_decals;

	mindist = DotProduct(r_origin, vpn) + 4.0f;

	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(-3, -1);

	qglDepthMask(GL_FALSE);
	qglEnable(GL_BLEND);
	GL_TexEnv(GL_MODULATE);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (dl = active->next; dl != active; dl = next) {
		next = dl->next;

		if (dl->time + r_decals_time->value <= r_newrefdef.time) {
			GL_FreeDecal(dl);
			continue;
		}
		if (dl->node == NULL || dl->node->visframe != r_visframecount)
			continue;

		/* do not render if the decal is behind the view */
		if (DotProduct(dl->org, vpn) < mindist)
			continue;

		/* do not render if the view origin is behind the decal */
		VectorSubtract(dl->org, r_origin, v);
		if (DotProduct(dl->direction, v) < 0.0)
			continue;

		/* look if it has a bad type */
		if (dl->type < 0 || dl->type >= DECAL_MAX)
			continue;

		endLerp = (float)(r_newrefdef.time - dl->time) / (float)(dl->endTime - dl->time);	
		lerp = 1.0 - endLerp;
		
		for (x=0; x<4; x++) 
			fColor[x] = dl->color[x] + (dl->endColor[x] - dl->color[x]) * endLerp;
			
		qglColor4fv(fColor);
		
		if (dl->type==DECAL_BLOOD1 || 
		    dl->type==DECAL_BLOOD2 || 
		    dl->type==DECAL_BLOOD3 || 
		    dl->type==DECAL_BLOOD4 ||
		    dl->type==DECAL_BLOOD5 ||
		    dl->type==DECAL_BLOOD6 || 
		    dl->type==DECAL_BLOOD7 ||
		    dl->type==DECAL_BLOOD8 ||
		    dl->type==DECAL_BLOOD9 ||
		    dl->type==DECAL_BLOOD10||
		    dl->type==DECAL_BLOOD11|| 
		    dl->type==DECAL_BLOOD12||
		    dl->type==DECAL_BLOOD13||
		    dl->type==DECAL_BLOOD14|| 
		    dl->type==DECAL_BLOOD15||
		    dl->type==DECAL_BLOOD16)
			qglBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		else 
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		GL_Bind(r_decaltexture[dl->type]->texnum);

		ri.Con_Printf(PRINT_DEVELOPER,
	                      "red %f green %f blue %f alpha %f \n lerp = %f endLerp = %f\n", 
			      fColor[0],fColor[1],fColor[2], fColor[3], lerp, endLerp); 

		Vector4Copy(dl->color, color);

		time = dl->time + r_decals_time->value - r_newrefdef.time;

		if (time < 1.5)
			color[3] *= time / 1.5;

		/* Draw it */
		qglColor4fv(color);

		qglBegin(GL_TRIANGLE_FAN);
		for (i = 0; i < dl->numverts; i++) {
			qglTexCoord2fv(dl->stcoords[i]);
			qglVertex3fv(dl->verts[i]);
		}
		qglEnd();

		r_numdecals++;
		if (r_numdecals >= MAX_DECALS)
			break;
	}

	GL_TexEnv(GL_REPLACE);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable(GL_BLEND);
	qglColor4f(1, 1, 1, 1);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_POLYGON_OFFSET_FILL);
}

static int	numFragmentVerts;
static int	maxFragmentVerts;
static vec3_t  *fragmentVerts;

static int	numClippedFragments;
static int	maxClippedFragments;
static fragment_t *clippedFragments;

static int	fragmentFrame;
static cplane_t	fragmentPlanes[6];

/*
** R_ClipPoly
*/

void R_ClipPoly(int nump, vec4_t vecs, int stage, fragment_t * fr)
{
	cplane_t       *plane;
	qboolean	front, back;
	vec4_t		newv[MAX_DECAL_VERTS];
	float          *v, d, dists[MAX_DECAL_VERTS];
	int		newc, i, j, sides[MAX_DECAL_VERTS];

	if (nump > MAX_DECAL_VERTS - 2)
		Com_Printf("R_ClipPoly: MAX_DECAL_VERTS");

	if (stage == 6) {	/* fully clipped */
		if (nump > 2) {
			fr->numverts = nump;
			fr->firstvert = numFragmentVerts;

			if (numFragmentVerts + nump >= maxFragmentVerts)
				nump = maxFragmentVerts - numFragmentVerts;

			for (i = 0, v = vecs; i < nump; i++, v += 4)
				VectorCopy(v, fragmentVerts[numFragmentVerts + i]);

			numFragmentVerts += nump;
		}
		return;
	}
	front = back = false;
	plane = &fragmentPlanes[stage];
	for (i = 0, v = vecs; i < nump; i++, v += 4) {
		d = PlaneDiff(v, plane);
		if (d > ON_EPSILON) {
			front = true;
			sides[i] = SIDE_FRONT;
		} else if (d < -ON_EPSILON) {
			back = true;
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}

		dists[i] = d;
	}

	if (!front)
		return;

	/* clip it */
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy(vecs, (vecs + (i * 4)));
	newc = 0;

	for (i = 0, v = vecs; i < nump; i++, v += 4) {
		switch (sides[i]) {
		case SIDE_FRONT:
			VectorCopy(v, newv[newc]);
			newc++;
			break;
		case SIDE_BACK:
			break;
		case SIDE_ON:
			VectorCopy(v, newv[newc]);
			newc++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i + 1]);
		for (j = 0; j < 3; j++)
			newv[newc][j] = v[j] + d * (v[j + 4] - v[j]);
		newc++;
	}

	/* continue */
	R_ClipPoly(newc, newv[0], stage + 1, fr);
}

/*
** R_PlanarSurfClipFragment
*/
void R_PlanarSurfClipFragment(mnode_t * node, msurface_t * surf, vec3_t normal)
{
	int		i;
	float          *v, *v2, *v3;
	fragment_t     *fr;
	vec4_t		verts[MAX_DECAL_VERTS];

	/* bogus face */
	if (surf->numedges < 3)
		return;

	/* greater than 60 degrees */
	if (surf->flags & SURF_PLANEBACK) {
		if (-DotProduct(normal, surf->plane->normal) < 0.5)
			return;
	} else {
		if (DotProduct(normal, surf->plane->normal) < 0.5)
			return;
	}

	v = surf->polys->verts[0];
	/* copy vertex data and clip to each triangle */
	for (i = 0; i < surf->polys->numverts - 2; i++) {
		fr = &clippedFragments[numClippedFragments];
		fr->numverts = 0;
		fr->node = node;
		fr->surf = surf;

		v2 = surf->polys->verts[0] + (i + 1) * VERTEXSIZE;
		v3 = surf->polys->verts[0] + (i + 2) * VERTEXSIZE;

		VectorCopy(v, verts[0]);
		VectorCopy(v2, verts[1]);
		VectorCopy(v3, verts[2]);
		R_ClipPoly(3, verts[0], 0, fr);

		if (fr->numverts) {
			numClippedFragments++;

			if ((numFragmentVerts >= maxFragmentVerts) || 
			    (numClippedFragments >= maxClippedFragments)) {
				return;
			}
		}
	}
}

/*
** R_RecursiveFragmentNode
*/
void R_RecursiveFragmentNode(mnode_t * node, vec3_t origin, float radius, vec3_t normal)
{
	float		dist;
	cplane_t       *plane;

mark0:
	if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
		return;		/* already reached the limit somewhere else */

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents != CONTENTS_NODE) {
		/* leaf */
		int		c;
		mleaf_t        *leaf;
		msurface_t     *surf, **mark;

		leaf = (mleaf_t *) node;
		if (!(c = leaf->nummarksurfaces))
			return;

		mark = leaf->firstmarksurface;
		do {
			if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
				return;

			surf = *mark++;
			if (surf->checkCount == fragmentFrame)
				continue;

			surf->checkCount = fragmentFrame;
			if (!(surf->texinfo->flags & (SURF_SKY | SURF_WARP | SURF_TRANS33 | SURF_TRANS66 | SURF_FLOWING | SURF_NODRAW))) //{
			//continue;
				R_PlanarSurfClipFragment(node, surf, normal);
			//}
		} while (--c);

		return;
	}
	plane = node->plane;
	dist = PlaneDiff(origin, plane);

	if (dist > radius) {
		node = node->children[0];
		goto mark0;
	}
	if (dist < -radius) {
		node = node->children[1];
		goto mark0;
	}
	R_RecursiveFragmentNode(node->children[0], origin, radius, normal);
	R_RecursiveFragmentNode(node->children[1], origin, radius, normal);
}

/*
** R_GetClippedFragments
*/
int R_GetClippedFragments(vec3_t origin, float radius, mat3_t axis, 
                          int maxfverts, vec3_t * fverts, 
			  int maxfragments, fragment_t *fragments)
{
	int		i;
	float		d;

	fragmentFrame++;

	/* initialize fragments */
	numFragmentVerts = 0;
	maxFragmentVerts = maxfverts;
	fragmentVerts = fverts;

	numClippedFragments = 0;
	maxClippedFragments = maxfragments;
	clippedFragments = fragments;

	/* calculate clipping planes */
	for (i = 0; i < 3; i++) {
		d = DotProduct(origin, axis[i]);

		VectorCopy(axis[i], fragmentPlanes[i * 2].normal);
		fragmentPlanes[i * 2].dist = d - radius;
		fragmentPlanes[i * 2].type = PlaneTypeForNormal(fragmentPlanes[i * 2].normal);

		VectorNegate(axis[i], fragmentPlanes[i * 2 + 1].normal);
		fragmentPlanes[i * 2 + 1].dist = -d - radius;
		fragmentPlanes[i * 2 + 1].type = PlaneTypeForNormal(fragmentPlanes[i * 2 + 1].normal);
	}

	R_RecursiveFragmentNode(r_worldmodel->nodes, origin, radius, axis[0]);

	return numClippedFragments;
}
#endif



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
// cl_tent.c -- client side temporary entities

#include "client.h"

typedef enum
{
	ex_free, ex_explosion, ex_misc, ex_flash, ex_mflash, ex_poly, ex_poly2
} exptype_t;

typedef struct explosion_s
{
	struct explosion_s *prev, *next;
	exptype_t	type;
	entity_t	ent;

	int			frames;
	float		light;
	vec3_t		lightcolor;
	float		start;
	int			baseframe;
} explosion_t;

#define	MAX_EXPLOSIONS	32
explosion_t	cl_explosions[MAX_EXPLOSIONS];
explosion_t	cl_explosions_headnode, *cl_free_expl;


#define	MAX_BEAMS	32
typedef struct
{
	int		entity;
	int		dest_entity;
	struct model_s	*model;
	int		endtime;
	vec3_t	offset;
	vec3_t	start, end;
} beam_t;
beam_t		cl_beams[MAX_BEAMS];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
beam_t		cl_playerbeams[MAX_BEAMS];


#define	MAX_LASERS	32
typedef struct
{
	entity_t	ent;
	int			endtime;
} laser_t;
laser_t		cl_lasers[MAX_LASERS];

//ROGUE
cl_sustain_t	cl_sustains[MAX_SUSTAINS];
//ROGUE


#if defined(PARTICLESYSTEM)
void CL_ExplosionParticles (vec3_t org, float scale, int water);
#else
void CL_BlasterParticles (vec3_t org, vec3_t dir);
void CL_ExplosionParticles (vec3_t org);
#endif
void CL_BFGExplosionParticles (vec3_t org);
// RAFAEL
void CL_BlueBlasterParticles (vec3_t org, vec3_t dir);

struct sfx_s	*cl_sfx_ric1;
struct sfx_s	*cl_sfx_ric2;
struct sfx_s	*cl_sfx_ric3;
struct sfx_s	*cl_sfx_lashit;
struct sfx_s	*cl_sfx_spark5;
struct sfx_s	*cl_sfx_spark6;
struct sfx_s	*cl_sfx_spark7;
struct sfx_s	*cl_sfx_railg;
struct sfx_s	*cl_sfx_rockexp;
struct sfx_s	*cl_sfx_grenexp;
struct sfx_s	*cl_sfx_watrexp;
// RAFAEL
struct sfx_s	*cl_sfx_plasexp;
struct sfx_s	*cl_sfx_footsteps[4];
/* Knightmare- Lazarus footstep sounds */
struct sfx_s	*cl_sfx_metal_footsteps[4];
struct sfx_s	*cl_sfx_dirt_footsteps[4];
struct sfx_s	*cl_sfx_vent_footsteps[4];
struct sfx_s	*cl_sfx_grate_footsteps[4];
struct sfx_s	*cl_sfx_tile_footsteps[4];
struct sfx_s	*cl_sfx_grass_footsteps[4];
struct sfx_s	*cl_sfx_snow_footsteps[4];
struct sfx_s	*cl_sfx_carpet_footsteps[4];
struct sfx_s	*cl_sfx_force_footsteps[4];
struct sfx_s	*cl_sfx_gravel_footsteps[4];
struct sfx_s	*cl_sfx_ice_footsteps[4];
struct sfx_s	*cl_sfx_sand_footsteps[4];
struct sfx_s	*cl_sfx_wood_footsteps[4];
struct sfx_s	*cl_sfx_slosh[4];
struct sfx_s	*cl_sfx_wade[4];
struct sfx_s	*cl_sfx_ladder[4];
/* end Knightmare */

struct model_s	*cl_mod_explode;
struct model_s	*cl_mod_smoke;
struct model_s	*cl_mod_flash;
struct model_s	*cl_mod_parasite_segment;
struct model_s	*cl_mod_grapple_cable;
struct model_s	*cl_mod_parasite_tip;
struct model_s	*cl_mod_explo4;
struct model_s	*cl_mod_bfg_explo;
struct model_s	*cl_mod_powerscreen;
// RAFAEL
struct model_s	*cl_mod_plasmaexplo;

//ROGUE
struct sfx_s	*cl_sfx_lightning;
struct sfx_s	*cl_sfx_disrexp;
struct model_s	*cl_mod_lightning;
struct model_s	*cl_mod_heatbeam;
struct model_s	*cl_mod_monster_heatbeam;
struct model_s	*cl_mod_explo4_big;

//ROGUE
/*
=================
CL_RegisterTEntSounds
=================
*/
void CL_RegisterTEntSounds (void)
{
	int		i;
	char	name[MAX_QPATH];

	cl_sfx_ric1 = S_RegisterSound ("world/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound ("world/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound ("world/ric3.wav");
	cl_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
	cl_sfx_spark5 = S_RegisterSound ("world/spark5.wav");
	cl_sfx_spark6 = S_RegisterSound ("world/spark6.wav");
	cl_sfx_spark7 = S_RegisterSound ("world/spark7.wav");
	cl_sfx_railg = S_RegisterSound ("weapons/railgf1a.wav");
	cl_sfx_rockexp = S_RegisterSound ("weapons/rocklx1a.wav");
	cl_sfx_grenexp = S_RegisterSound ("weapons/grenlx1a.wav");
	cl_sfx_watrexp = S_RegisterSound ("weapons/xpld_wat.wav");
	
	S_RegisterSound ("player/land1.wav");
	S_RegisterSound ("player/fall2.wav");
	S_RegisterSound ("player/fall1.wav");

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/step%i.wav", i+1);
		cl_sfx_footsteps[i] = S_RegisterSound (name);
	}

	/* Knightmare- Lazarus footstep sounds */
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_metal%i.wav", i+1);
		cl_sfx_metal_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_dirt%i.wav", i+1);
		cl_sfx_dirt_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_duct%i.wav", i+1);
		cl_sfx_vent_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_grate%i.wav", i+1);
		cl_sfx_grate_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_tile%i.wav", i+1);
		cl_sfx_tile_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_grass%i.wav", i+1);
		cl_sfx_grass_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_snow%i.wav", i+1);
		cl_sfx_snow_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_carpet%i.wav", i+1);
		cl_sfx_carpet_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_force%i.wav", i+1);
		cl_sfx_force_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_gravel%i.wav", i+1);
		cl_sfx_gravel_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_ice%i.wav", i+1);
		cl_sfx_ice_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_sand%i.wav", i+1);
		cl_sfx_sand_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_wood%i.wav", i+1);
		cl_sfx_wood_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_slosh%i.wav", i+1);
		cl_sfx_slosh[i] = S_RegisterSound (name);
	}
	for (i=0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_wade%i.wav", i+1);
		cl_sfx_wade[i] = S_RegisterSound (name);
	}
	for (i=0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "player/pl_ladder%i.wav", i+1);
		cl_sfx_ladder[i] = S_RegisterSound (name);
	}
	if (cl_footsteps_override->value)
		ReadTextureSurfaceAssignments();
	/* end Knightmare */


//PGM
	cl_sfx_lightning = S_RegisterSound ("weapons/tesla.wav");
	cl_sfx_disrexp = S_RegisterSound ("weapons/disrupthit.wav");
//PGM
}	

/*
=================
CL_RegisterTEntModels
=================
*/
void CL_RegisterTEntModels (void)
{
	cl_mod_explode = re.RegisterModel ("models/objects/explode/tris.md2");
	cl_mod_smoke = re.RegisterModel ("models/objects/smoke/tris.md2");
	cl_mod_flash = re.RegisterModel ("models/objects/flash/tris.md2");
	cl_mod_parasite_segment = re.RegisterModel ("models/monsters/parasite/segment/tris.md2");
	cl_mod_grapple_cable = re.RegisterModel ("models/ctf/segment/tris.md2");
	cl_mod_parasite_tip = re.RegisterModel ("models/monsters/parasite/tip/tris.md2");
	cl_mod_explo4 = re.RegisterModel ("models/objects/r_explode/tris.md2");
	cl_mod_bfg_explo = re.RegisterModel ("sprites/s_bfg2.sp2");
	cl_mod_powerscreen = re.RegisterModel ("models/items/armor/effect/tris.md2");

	re.RegisterModel ("models/objects/laser/tris.md2");
	re.RegisterModel ("models/objects/grenade2/tris.md2");
	re.RegisterModel ("models/weapons/v_machn/tris.md2");
	re.RegisterModel ("models/weapons/v_handgr/tris.md2");
	re.RegisterModel ("models/weapons/v_shotg2/tris.md2");
	re.RegisterModel ("models/objects/gibs/bone/tris.md2");
	re.RegisterModel ("models/objects/gibs/sm_meat/tris.md2");
	re.RegisterModel ("models/objects/gibs/bone2/tris.md2");

	re.RegisterPic ("w_machinegun");
	re.RegisterPic ("a_bullets");
	re.RegisterPic ("i_health");
	re.RegisterPic ("a_grenades");

//ROGUE
	cl_mod_explo4_big = re.RegisterModel ("models/objects/r_explode2/tris.md2");
	cl_mod_lightning = re.RegisterModel ("models/proj/lightning/tris.md2");
	cl_mod_heatbeam = re.RegisterModel ("models/proj/beam/tris.md2");
	cl_mod_monster_heatbeam = re.RegisterModel ("models/proj/widowbeam/tris.md2");
//ROGUE
}	

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	int i;
	memset (cl_beams, 0, sizeof(cl_beams));
	memset (cl_explosions, 0, sizeof(cl_explosions));
	memset (cl_lasers, 0, sizeof(cl_lasers));

	// link explosions
	cl_free_expl = cl_explosions;
	cl_explosions_headnode.prev = &cl_explosions_headnode;
	cl_explosions_headnode.next = &cl_explosions_headnode;
	for ( i = 0; i < MAX_EXPLOSIONS - 1; i++ ) {
		cl_explosions[i].next = &cl_explosions[i+1];
	}

//ROGUE
	memset (cl_playerbeams, 0, sizeof(cl_playerbeams));
	memset (cl_sustains, 0, sizeof(cl_sustains));
//ROGUE
}

/*
=================
CL_AllocExplosion
=================
*/
explosion_t *CL_AllocExplosion (void)
{
	explosion_t *ex;

	if ( cl_free_expl )
	{ // take a free explosion if possible
		ex = cl_free_expl;
		cl_free_expl = ex->next;
	} 
	else 
	{ // grab the oldest one otherwise
		ex = cl_explosions_headnode.prev;
		ex->prev->next = ex->next;
		ex->next->prev = ex->prev;
	}

	memset ( ex, 0, sizeof (*ex) );
	ex->start = cl.frame.servertime - 100;

	// put the decal at the start of the list
	ex->prev = &cl_explosions_headnode;
	ex->next = cl_explosions_headnode.next;
	ex->next->prev = ex;
	ex->prev->next = ex;

	return ex;
}

/*
=================
CL_FreeExplosion
=================
*/
void CL_FreeExplosion ( explosion_t *ex )
{
	// remove from linked active list
	ex->prev->next = ex->next;
	ex->next->prev = ex->prev;

	// insert into linked free list
	ex->next = cl_free_expl;
	cl_free_expl = ex;
}

#if defined(PARTICLESYSTEM)
void CL_R_Explode_SP (vec3_t origin, int water)
{
	CL_Explosion_Particle (origin, 50, 25, water);
}

void CL_G_Explode_SP (vec3_t origin, int water)
{
	CL_Explosion_Particle (origin, 75, 0, water);
}
#endif
/*
=================
CL_SmokeAndFlash
=================
*/
void CL_SmokeAndFlash(vec3_t origin)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion ();
	VectorCopy (origin, ex->ent.origin);
	ex->type = ex_misc;
	ex->frames = 4;
	ex->ent.flags = RF_TRANSLUCENT|RF_NOSHADOW;
	ex->ent.model = cl_mod_smoke;

	ex = CL_AllocExplosion ();
	VectorCopy (origin, ex->ent.origin);
	ex->type = ex_flash;
	ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
	ex->frames = 2;
	ex->ent.model = cl_mod_flash;
}

/*
=================
CL_ParseParticles
=================
*/
void CL_ParseParticles (void)
{
	int		color, count;
	vec3_t	pos, dir;

	MSG_ReadPos (&net_message, pos);
	MSG_ReadDir (&net_message, dir);

	color = MSG_ReadByte (&net_message);

	count = MSG_ReadByte (&net_message);

	CL_ParticleEffect (pos, dir, color, count);
}

/*
=================
CL_ParseBeam
=================
*/
int CL_ParseBeam (struct model_s *model)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	ent = MSG_ReadShort (&net_message);
	
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

// override any beam with the same entity
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.stime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return ent;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.stime)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.stime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return ent;
}

/*
=================
CL_ParseBeam2
=================
*/
int CL_ParseBeam2 (struct model_s *model)
{
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;
	
	ent = MSG_ReadShort (&net_message);
	
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);
	MSG_ReadPos (&net_message, offset);

//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

// override any beam with the same entity

	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.stime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.stime)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.stime + 200;	
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return ent;
}

// ROGUE
/*
=================
CL_ParsePlayerBeam
  - adds to the cl_playerbeam array instead of the cl_beams array
=================
*/
int CL_ParsePlayerBeam (struct model_s *model)
{
	int		ent;
	vec3_t		start, end, offset;
	beam_t		*b;
	int		i;
	
	ent = MSG_ReadShort (&net_message);
	
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);
	// PMM - network optimization
	if (model == cl_mod_heatbeam)
		VectorSet(offset, 2, 7, -3);
	else if (model == cl_mod_monster_heatbeam)
	{
		model = cl_mod_heatbeam;
		VectorSet(offset, 0, 0, 0);
	}
	else
		MSG_ReadPos (&net_message, offset);

//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

// override any beam with the same entity
// PMM - For player beams, we only want one per player (entity) so..
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++)
	{
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.stime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

// find a free beam
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.stime)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.stime + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return ent;
}
//rogue

/*
=================
CL_ParseLightning
=================
*/
#if defined(PARTICLESYSTEM)
int CL_ParseLightning (float size)
{

	vec3_t		start, end;
	int		srcEnt, dstEnt;
	
	srcEnt = MSG_ReadShort (&net_message);
	dstEnt = MSG_ReadShort (&net_message);

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	CL_LightningBeam(start, end, srcEnt, dstEnt, size);
		
	return srcEnt;
}
#else
int CL_ParseLightning (struct model_s *model)
{
	int		srcEnt, destEnt;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	srcEnt = MSG_ReadShort (&net_message);
	destEnt = MSG_ReadShort (&net_message);

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

// override any beam with the same source AND destination entities
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == srcEnt && b->dest_entity == destEnt)
		{
//			Com_Printf("%d: OVERRIDE  %d -> %d\n", cl.time, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.stime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return srcEnt;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.stime)
		{
//			Com_Printf("%d: NORMAL  %d -> %d\n", cl.time, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.stime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return srcEnt;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return srcEnt;
}
#endif

/*
=================
CL_ParseLaser
=================
*/
void CL_ParseLaser (int colors)
{
	vec3_t	start;
	vec3_t	end;
	laser_t	*l;
	int		i;

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	for (i=0, l=cl_lasers ; i< MAX_LASERS ; i++, l++)
	{
		if (l->endtime < cl.stime)
		{
			l->ent.flags = RF_TRANSLUCENT | RF_BEAM | RF_NOSHADOW;
			VectorCopy (start, l->ent.origin);
			VectorCopy (end, l->ent.oldorigin);
			l->ent.alpha = 0.30;
			l->ent.skinnum = (colors >> ((rand() % 4)*8)) & 0xff;
			l->ent.model = NULL;
			l->ent.frame = 4;
			l->endtime = cl.stime + 100;
			return;
		}
	}
}

//=============
//ROGUE
void CL_ParseSteam (void)
{
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		color;
	int		magnitude;
	cl_sustain_t	*s, *free_sustain;

	id = MSG_ReadShort (&net_message);		// an id of -1 is an instant effect
	if (id != -1) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, s->org);
			MSG_ReadDir (&net_message, s->dir);
			r = MSG_ReadByte (&net_message);
			s->color = r & 0xff;
			s->magnitude = MSG_ReadShort (&net_message);
			s->endtime = cl.stime + MSG_ReadLong (&net_message);
			s->think = CL_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cl.stime;
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, pos);
			MSG_ReadDir (&net_message, dir);
			r = MSG_ReadByte (&net_message);
			magnitude = MSG_ReadShort (&net_message);
			magnitude = MSG_ReadLong (&net_message); // really interval
		}
	}
	else // instant
	{
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		r = MSG_ReadByte (&net_message);
		magnitude = MSG_ReadShort (&net_message);
		color = r & 0xff;
#if defined(PARTICLESYSTEM)
		CL_ParticleSteamEffect(pos, dir,
		                       color8red(color),
				       color8green(color),
				       color8blue(color),
		                       -20, -20, -20, cnt, magnitude);
#else
		CL_ParticleSteamEffect(pos, dir, color, cnt, magnitude);
#endif
//		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
	}
}

void CL_ParseWidow (void)
{
	vec3_t	pos;
	int		id, i;
	cl_sustain_t	*s, *free_sustain;

	id = MSG_ReadShort (&net_message);

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		s->id = id;
		MSG_ReadPos (&net_message, s->org);
		s->endtime = cl.stime + 2100;
		s->think = CL_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cl.stime;
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		MSG_ReadPos (&net_message, pos);
	}
}

void CL_ParseNuke (void)
{
	vec3_t	pos;
	int		i;
	cl_sustain_t	*s, *free_sustain;

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		s->id = 21000;
		MSG_ReadPos (&net_message, s->org);
		s->endtime = cl.stime + 1000;
#if defined(PARTICLESYSTEM)
		if (cl_nukeblast_enh->value) {
			s->think = CL_Nukeblast_Opt;
		}
		else 
#endif
		s->think = CL_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cl.stime;
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		MSG_ReadPos (&net_message, pos);
	}
}

//ROGUE
//=============

/*
 =================
 CL_FindTrailPlane

 Disgusting hack
 =================
*/
#if !defined(PARTICLESYSTEM)
void CL_FindTrailPlane (vec3_t start, vec3_t end, vec3_t dir)
{

	trace_t	trace;
	vec3_t	vec, point;
	float	len;

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	VectorMA(start, len + 0.5, vec, point);

	trace = CM_BoxTrace(start, point, vec3_origin, vec3_origin, 0, MASK_SOLID);
	if (trace.allsolid || trace.fraction == 1.0){
		VectorClear(dir);
		return;
	}

	VectorCopy(trace.plane.normal, dir);
}

/*
 =================
 CL_FindExplosionPlane

 Disgusting hack
 =================
*/
void CL_FindExplosionPlane (vec3_t org, float radius, vec3_t dir)
{

	static vec3_t	planes[6] = {{0, 0, 1}, 
				     {0, 1, 0}, 
				     {1, 0, 0}, 
				     {0, 0, -1}, 
				     {0, -1, 0}, 
				     {-1, 0, 0}};
	trace_t		trace;
	vec3_t		point;
	float		best = 1.0;
	int		i;

	VectorClear(dir);

	for (i = 0; i < 6; i++) {
		VectorMA(org, radius, planes[i], point);

		trace = CM_BoxTrace(org, point, vec3_origin, vec3_origin, 0, MASK_SOLID);
		if (trace.allsolid || trace.fraction == 1.0)
			continue;

		if (trace.fraction < best){
			best = trace.fraction;
			VectorCopy(trace.plane.normal, dir);
		}
	}
}
#endif

/*
=================
CL_ParseTEnt
=================
*/
static byte splash_color[] = {0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8};

void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos, pos2, dir;
	explosion_t	*ex;
	int		cnt;
	int		color;
	int		r;
	int		ent;
	int		magnitude;
#if defined(PARTICLESYSTEM)
	int		red, 
			green, 
			blue;
	int		decalsize = 0;
	float		smokesize;
	vec3_t		sparkdir;
#endif

	type = MSG_ReadByte (&net_message);

	switch (type)
	{
	case TE_BLOOD:			// bullet hitting flesh
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
#if defined(PARTICLESYSTEM)
		CL_BloodHit (pos, dir);
#else
		CL_ParticleEffect (pos, dir, 0xe8, 60);
#endif
		break;

	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
#if defined(PARTICLESYSTEM)
	case TE_BULLET_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_GUNSHOT)
			decalsize = 3;
		else if (type == TE_SHOTGUN) 
			decalsize = 4;

		if (type == TE_BULLET_SPARKS)
			VectorScale(dir, 2, dir);

		{
			vec3_t color = {255, 125, 10};
			CL_ParticleEffectSparks(pos, dir, color, (type == TE_GUNSHOT) ? 5 : 10);
		}

		if (type != TE_SPARKS)
		{
			if (cl_gunsmoke->value) 
				CL_ImpactSmokeEffect(pos, dir, 0.5);
			CL_ParticleBulletDecal(pos, dir, decalsize);
			
			// impact sound
			cnt = rand()&15;
			if (cnt == 1)
				S_StartSound (pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				S_StartSound (pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				S_StartSound (pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}
		break;
#else
	case TE_BULLET_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		re.AddDecal(pos, dir, 0, 0, 0, 1.0, 0, 0, 0, 0, 
		            4 + ((rand()%21*0.05) - 0.5),
			    DECAL_BULLET, 0, rand()%361); 
		if (type == TE_GUNSHOT)
			CL_ParticleEffect (pos, dir, 0, 40);
		else
			CL_ParticleEffect (pos, dir, 0xe0, 6);
		if (type != TE_SPARKS)
		{
			CL_SmokeAndFlash(pos);
			// impact sound
			cnt = rand()&15;
			if (cnt == 1)
				S_StartSound (pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				S_StartSound (pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				S_StartSound (pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}
		break;
#endif
	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_SCREEN_SPARKS)
			CL_ParticleEffect (pos, dir, 0xd0, 40);
		else
			CL_ParticleEffect (pos, dir, 0xb0, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
		
	case TE_SHOTGUN:			// bullet hitting wall
#if defined(PARTICLESYSTEM)
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);

		if (type == TE_GUNSHOT) {
			decalsize = 2.5;
			smokesize = 1;
		}
		else if (type == TE_BULLET_SPARKS) {
			decalsize = 3;
			smokesize = 1.25;
		}
		else if (type == TE_SHOTGUN) {
			decalsize = 3.5;
			smokesize = random()*0.75 + 0.25;
		}

		if (!modType("dday") && !modType("action")) {
			decalsize *= 2;
			smokesize *= 1.5;
		}

		if (type == TE_BULLET_SPARKS) {
			VectorScale(dir, 1.25 * smokesize, sparkdir);
		}
		else if (type != TE_SPARKS && type != TE_SHOTGUN) {
			VectorScale(dir, 1.1 * smokesize, sparkdir);
			// impact sound
			cnt = rand()&15;
			if (cnt == 1)
				S_StartSound (pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				S_StartSound (pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				S_StartSound (pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}
		else
			VectorCopy(dir, sparkdir);

		//adding sparks
		{
			vec3_t color = { 255, 125, 10 };
			CL_ParticleEffectSparks (pos, sparkdir, color, (type == TE_GUNSHOT)? 5 : 10);
		}
		
		if (type != TE_SPARKS) {
			VectorNormalize(dir);
			if (cl_gunsmoke->value)
				CL_ImpactSmokeEffect(pos, dir, smokesize);

			CL_ParticleBulletDecal(pos, dir, decalsize);
		}
		break;
#else
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		re.AddDecal(pos, dir, 0, 0, 0, 1.0, 0, 0, 0, 0, 
		            6 + ((rand()%21*0.05) - 0.5), 
			    DECAL_BULLET, 0, rand()%361); 
		CL_ParticleEffect (pos, dir, 0, 20);
		CL_SmokeAndFlash(pos);
		break;
#endif
	case TE_SPLASH:			// bullet hitting water
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		r = MSG_ReadByte (&net_message);
		if (r > 6)
			color = 0x00;
		else
			color = splash_color[r];

		if (r == SPLASH_SPARKS)
		{
			CL_ParticleEffect (pos, dir, color, cnt);
			r = rand() & 3;
			if (r == 0)
				S_StartSound (pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
			else if (r == 1)
				S_StartSound (pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
			else
				S_StartSound (pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
		}
#if defined(PARTICLESYSTEM)
		else			
			CL_ParticleWaterEffectSplash (pos, dir, color, cnt, r);
#endif
		break;

	case TE_LASER_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect2 (pos, dir, color, cnt);
		break;

	// RAFAEL
	case TE_BLUEHYPERBLASTER:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, dir);
#if defined(PARTICLESYSTEM)
		CL_BlasterParticles (pos, dir, 40);
#else
		CL_BlasterParticles (pos, dir);
#endif
		break;

	case TE_BLASTER: // blaster hitting wall
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
#if defined(PARTICLESYSTEM)
		if (cl_blaster_color->value == 1) {
			/* give the bolt a red particle glow */
			CL_BlasterParticlesColor(pos, dir, 40, 235, 50, 50, 0, -90, -30);
			if (cl_gunsmoke->value)
				CL_WeapSmokeEffect(pos, dir, 235, 50, 50, 1.0, 0.4f);
			CL_ParticleBlasterColorDecal(pos, dir, 7.5, 235, 50, 50);
		} else if (cl_blaster_color->value == 2) {
			/* give the bolt a green particle glow */
			CL_BlasterParticlesColor(pos, dir, 40, 50, 235, 50, 0, -90, -30);
			if (cl_gunsmoke->value)
				CL_WeapSmokeEffect(pos, dir, 50, 235, 50, 1.0, 0.4f);
			CL_ParticleBlasterColorDecal(pos, dir, 7.5, 50, 235, 50);
		} else if (cl_blaster_color->value == 3) {
			/* give the bolt a blue particle glow */
			CL_BlasterParticlesColor(pos, dir, 40, 50, 50, 255, 0, -90, -30);
			if (cl_gunsmoke->value)
				CL_WeapSmokeEffect(pos, dir, 50, 50, 255, 1.0, 0.4f);
			CL_ParticleBlasterColorDecal(pos, dir, 7.5, 50, 50, 255);
		} else {
			CL_BlasterParticles(pos, dir, 40);
			if (cl_gunsmoke->value)
				CL_WeapSmokeEffect(pos, dir, 255, 150, 50, 1.0, 0.4f);
			CL_ParticleBlasterDecal(pos, dir, 7.5);
		}

		for (r=0; r<3; r++)
		{
			dir[r]*=10;
			pos[r]+=dir[r];
		}
#else
        	re.AddDecal(pos, dir, 1, 0.5, 0, 1.0, 1, 0.5, 0, 1, 4 + ((rand()%21*0.05) - 0.5), 
			    DECAL_BLASTER, 0, rand()%361);
		CL_BlasterParticles (pos, dir);

		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->ent.angles[0] = acos(dir[2])/M_PI*180;
	// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->ent.angles[1] = atan2(dir[1], dir[0])/M_PI*180;
		else if (dir[1] > 0)
			ex->ent.angles[1] = 90;
		else if (dir[1] < 0)
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->type = ex_misc;
		ex->ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT|RF_NOSHADOW;
		ex->light = 150;
		ex->lightcolor[0] = 1;
		ex->lightcolor[1] = 1;
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
#endif
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
		
	case TE_RAILTRAIL:			// railgun effect
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
#if defined(PARTICLESYSTEM)
		CL_RailTrail (pos, pos2);
#else
		CL_FindTrailPlane(pos, pos2, dir);
		CL_RailTrail (pos, pos2);
		re.AddDecal(pos2, dir, 50, 50, 235, 
		            1, 1, 1, 1, 1, 
			    8, DECAL_RAIL, 0, rand()%361);
#endif
		S_StartSound (pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos (&net_message, pos);
#if defined(PARTICLESYSTEM)
		#if 0
		if (cl_explosion->value == 4 || cl_explosion->value == 3) {
			CL_G_Explode_SP(pos, type==TE_GRENADE_EXPLOSION_WATER);
			CL_ExplosionParticles(pos, 1.25, type == TE_GRENADE_EXPLOSION_WATER);
		} else if (cl_explosion->value == 2 || cl_explosion->value == 1) {
			CL_G_Explode_SP(pos, type==TE_GRENADE_EXPLOSION_WATER);
			CL_ExplosionParticles_Old(pos);
		} else {
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			ex->frames = 19;
			ex->baseframe = 30;
			ex->ent.angles[1] = rand() % 360;
			CL_Explosion_Decal (pos, 75);
		}
		#else
		if (!cl_explosion->value)
		{
			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			ex->frames = 19;
			ex->baseframe = 30;
			ex->ent.angles[1] = rand() % 360;

			CL_Explosion_Decal (pos, 75);
		}
		else
			CL_G_Explode_SP(pos, type==TE_GRENADE_EXPLOSION_WATER);
		CL_ExplosionParticles (pos,1.25, type==TE_GRENADE_EXPLOSION_WATER);
		#endif
#else
		CL_FindExplosionPlane(pos, 80, dir); 
		re.AddDecal(pos, dir, 1, 0.7, 0, 0.85, 0, 0, 0, 0, 
		            56 + ((rand()%21*0.05) - 0.5), 
                            DECAL_EXPLODE, 0, rand()%361); 
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.model = cl_mod_explo4;
		ex->frames = 19;
		ex->baseframe = 30;
		ex->ent.angles[1] = rand() % 360;
		CL_ExplosionParticles (pos);
#endif
		if (type == TE_GRENADE_EXPLOSION_WATER)
			S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound (pos, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;

	// RAFAEL
	case TE_PLASMA_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
#if defined(PARTICLESYSTEM)
		#if 0
		if (cl_explosion->value == 4 || cl_explosion->value == 3) {
			CL_R_Explode_SP(pos, 0);
			CL_ExplosionParticles(pos, 1, 0);
			CL_Explosion_Sparks (pos, 32);
		} else if (cl_explosion->value == 2 || cl_explosion->value == 1) {
			CL_R_Explode_SP(pos, 0);
		} else {
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;
			CL_Explosion_Decal (pos, 50);
		}
		#else
		if (!cl_explosion->value)
		{
			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0; 
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;

			CL_Explosion_Decal (pos, 50);
		}
		else
			CL_R_Explode_SP(pos, 0);

		CL_ExplosionParticles (pos, 1, 0);
		#endif
#else
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 1.0; 
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		CL_ExplosionParticles (pos);
#endif
		S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;
	
	case TE_EXPLOSION1:
	case TE_EXPLOSION1_BIG:						// PMM
#if defined(PARTICLESYSTEM)
		MSG_ReadPos (&net_message, pos);
		#if 0
		if (cl_explosion->value == 4 || cl_explosion->value == 3) {
			CL_Explosion_Particle(pos, 100, 0,0);
			CL_ExplosionParticles(pos, 3, 0);
			CL_Explosion_Sparks(pos, 96);
			{
				vec3_t color = {200, 100, 10};
				CL_ParticleEffectSparks(pos, dir, color, 8);
			}
		} else if (cl_explosion->value == 2 || cl_explosion->value == 1) {
			CL_Explosion_Particle(pos, 100, 0,0);
			CL_ExplosionParticles_Old(pos);
		} else {
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.scale = cl_explosion_scale->value;
			ex->ent.model = cl_mod_explo4_big;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;

			CL_Explosion_Decal (pos, 100);
			CL_ExplosionParticles(pos, 3, 0);
		}
		#else
		if (!cl_explosion->value)
		{

			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.scale = cl_explosion_scale->value;
			ex->ent.model = cl_mod_explo4_big;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;

			CL_Explosion_Decal (pos, 100);
			CL_ExplosionParticles (pos, 3, 0);
		}
		else
		{
			if (modType("dday"))
			{
				CL_Explosion_Particle (pos, 250, 0, 0);
				CL_ExplosionParticles (pos, 5, 0);
			}
			else
			{
				CL_Explosion_Particle (pos, 100, 0, 0);
				CL_ExplosionParticles (pos, 2, 0);
			}
		}
		#endif
		S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;
#endif
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);
#if defined(PARTICLESYSTEM)
		#if 0
		if (cl_explosion->value == 4 || cl_explosion->value == 3) {
			CL_Explosion_Particle (pos, 50, 0, 0);
			CL_ExplosionParticles(pos, 1, type==TE_ROCKET_EXPLOSION_WATER);
			CL_Explosion_Sparks(pos, 21);
		} else if (cl_explosion->value == 2 || cl_explosion->value == 1) {
			CL_Explosion_Particle (pos, 50, 0, 0);
			CL_ExplosionParticles_Old(pos);
		} else {
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;
			CL_Explosion_Decal (pos, 50);

		}
		#else
		if (!cl_explosion->value)
		{
			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0; 
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;

			CL_Explosion_Decal (pos, 50);
		}
		else
			CL_R_Explode_SP(pos, type==TE_ROCKET_EXPLOSION_WATER);

		CL_ExplosionParticles (pos, 1, type==TE_ROCKET_EXPLOSION_WATER);
		#endif
#else
		CL_FindExplosionPlane(pos, 70, dir); 
		re.AddDecal(pos, dir, 1, 0.7, 0, 0.85, 0, 0, 0, 0,
			    56 + ((rand()%21*0.05) - 0.5), 
			    DECAL_EXPLODE, 0, rand()%361); 
		ex = CL_AllocExplosion();
		VectorCopy(pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT | RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		if (type != TE_EXPLOSION1_BIG)	/* PMM */
			ex->ent.model = cl_mod_explo4;	/* PMM */
		else
			ex->ent.model = cl_mod_explo4_big;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		if ((type != TE_EXPLOSION1_BIG) && (type != TE_EXPLOSION1_NP))	/* PMM */
			CL_ExplosionParticles(pos);	/* PMM */
#endif
		if (type == TE_ROCKET_EXPLOSION_WATER)
			S_StartSound(pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound(pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;
	case TE_EXPLOSION1_NP:						// PMM
		MSG_ReadPos (&net_message, pos);
#if defined(PARTICLESYSTEM)
		#if 0
		if (cl_explosion->value == 4 || cl_explosion->value == 3) {
			CL_R_Explode_SP(pos, 0);
			CL_ExplosionParticles (pos, 0.6666666, 0);
			CL_Explosion_Sparks(pos, 32);	/* PMM */
		} else if (cl_explosion->value == 2 || cl_explosion->value == 1) {
			CL_R_Explode_SP(pos, type==TE_ROCKET_EXPLOSION_WATER);
			CL_ExplosionParticles_Old(pos);
		} else {
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;
			CL_Explosion_Decal (pos, 65);

		}
		#else
		if (!cl_explosion->value)
		{
			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0; 
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			ex->ent.scale = cl_explosion_scale->value;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;

			CL_Explosion_Decal (pos, 65);
		}
		else
			CL_Explosion_Particle (pos, 50, 0, 0);

		CL_ExplosionParticles (pos, 0.6666666, 0);
		#endif
#else
		CL_FindExplosionPlane(pos, 80, dir); 
		re.AddDecal(pos, dir, 1, 0.7, 0, 0.85, 0, 0, 0, 0,
			    56 + ((rand()%21*0.05) - 0.5), 
			    DECAL_EXPLODE, 0, rand()%361); 
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		if (type != TE_EXPLOSION1_BIG)				// PMM
			ex->ent.model = cl_mod_explo4;			// PMM
		else
			ex->ent.model = cl_mod_explo4_big;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		if ((type != TE_EXPLOSION1_BIG) && (type != TE_EXPLOSION1_NP))		// PMM
			CL_ExplosionParticles (pos); // PMM
#endif
		if (type == TE_ROCKET_EXPLOSION_WATER)
			S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_BFG_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
#if defined(PARTICLESYSTEM)
		#if 1
		CL_BFGExplosionParticles_2(pos);
		#else
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 0.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.0;
		ex->ent.model = cl_mod_bfg_explo;
		ex->ent.flags |= RF_TRANSLUCENT | RF_TRANS_ADDITIVE;
		ex->ent.alpha = 0.9999;
		ex->frames = 4;
		ex->ent.scale = 1;
		#endif
#else
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 0.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.0;
		ex->ent.model = cl_mod_bfg_explo;
		ex->ent.flags |= RF_TRANSLUCENT|RF_NOSHADOW;
		ex->ent.alpha = 0.30;
		ex->frames = 4;
#endif
		break;

	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_BFGExplosionParticles (pos);
#if defined(PARTICLESYSTEM)
		CL_Explosion_Decal (pos, 75);
#else
		CL_FindExplosionPlane(pos, 96, dir); 
		re.AddDecal(pos, dir, 0.1, 0.1, 0.1, 1, 0.1, 0.1, 0.1, 1,
		            60,
		            DECAL_BFG, 0, rand()%361); 
#endif
		break;

	case TE_BFG_LASER:
		CL_ParseLaser (0xd0d1d2d3);
		break;

	case TE_BUBBLETRAIL:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BubbleTrail (pos, pos2);
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		ent = CL_ParseBeam (cl_mod_parasite_segment);
		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		MSG_ReadPos (&net_message, pos);
		CL_BigTeleportParticles (pos);
		S_StartSound (pos, 0, 0, S_RegisterSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_GRAPPLE_CABLE:
		ent = CL_ParseBeam2 (cl_mod_grapple_cable);
		break;

	// RAFAEL
	case TE_WELDING_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
#if defined(PARTICLESYSTEM)
		{
			vec3_t	sparkcolor = { 
			                       color8red(color), 
					       color8green(color), 
					       color8blue(color)
					     };

			CL_ParticleEffectSparks(pos, dir, sparkcolor, 40);
		}
#else
		CL_ParticleEffect2 (pos, dir, color, cnt);

		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_flash;
		// note to self
		// we need a better no draw flag
		ex->ent.flags = RF_BEAM|RF_NOSHADOW;
		ex->light = 100 + (rand()%75);
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.3;
		ex->ent.model = cl_mod_flash;
		ex->frames = 2;
#endif
		break;

	case TE_GREENBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
#if defined(PARTICLESYSTEM)
		#if 0
		CL_GreenBloodHit(pos, dir);
		#else
		CL_ParticleEffect2 (pos, dir, 0xdf, 30);
		#endif
#else
		CL_ParticleEffect2 (pos, dir, 0xdf, 30);
#endif
		break;

	// RAFAEL
	case TE_TUNNEL_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect3 (pos, dir, color, cnt);
		break;

//=============
//PGM
		// PMM -following code integrated for flechette (different color)
	case TE_BLASTER2:			// green blaster hitting wall
	case TE_FLECHETTE:			// flechette
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
#if defined(PARTICLESYSTEM)
		#if 1
		if (type == TE_BLASTER2) {
			CL_BlasterParticlesColor (pos, dir, 40, 50, 235, 50, -10, 0, -10);
			red=50; 
			green=235; 
			blue=50; 
		}
		else if (type == TE_BLUEHYPERBLASTER) {
			CL_BlasterParticlesColor (pos, dir, 40, 50, 50, 235, -10, 0, -10);
			red=50; 
			green=50; 
			blue=235; 
		}
		else {
 			CL_BlasterParticlesColor (pos, dir, 40, 100, 100, 195, -10, 0, -10);
			red=100; 
			green=100; 
			blue=195; 
		}
		CL_ParticleBlasterColorDecal(pos, dir, 7.5, red, green, blue);
		#else
		CL_BlasterParticlesColor(pos, dir, 40, 50, 50, 235, 0, -90, -30);
		#endif
#else
		// PMM
		if (type == TE_BLASTER2)
			CL_BlasterParticles2 (pos, dir, 0xd0);
		else
			CL_BlasterParticles2 (pos, dir, 0x6f); // 75
        	re.AddDecal(pos, dir, 1, 0.5, 0, 1, 1, 0.5, 0, 1, 4, 
			    DECAL_BLASTER, 0, 0); 
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->ent.angles[0] = acos(dir[2])/M_PI*180;
	// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->ent.angles[1] = atan2(dir[1], dir[0])/M_PI*180;
		else if (dir[1] > 0)
			ex->ent.angles[1] = 90;
		else if (dir[1] < 0)
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->type = ex_misc;
		ex->ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT|RF_NOSHADOW;

		// PMM
		if (type == TE_BLASTER2)
			ex->ent.skinnum = 1;
		else // flechette
			ex->ent.skinnum = 2;

		ex->light = 150;
		// PMM
		if (type == TE_BLASTER2)
			ex->lightcolor[1] = 1;
		else // flechette
		{
			ex->lightcolor[0] = 0.19;
			ex->lightcolor[1] = 0.41;
			ex->lightcolor[2] = 0.75;
		}
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
#endif
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;


	case TE_LIGHTNING:
#if defined(PARTICLESYSTEM)
		ent = CL_ParseLightning (10);
#else
		ent = CL_ParseLightning (cl_mod_lightning);
#endif
		S_StartSound (NULL, ent, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
		break;

	case TE_DEBUGTRAIL:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_DebugTrail (pos, pos2);
		break;

	case TE_PLAIN_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
#if defined(PARTICLESYSTEM)
		#if 0
		if (cl_explosion->value == 4 || cl_explosion->value == 3) {
			CL_Explosion_Particle(pos, 50, true, true);
			CL_ExplosionParticles(pos, 0.6666666, 0);
			CL_Explosion_Sparks(pos, 21);
		} else if (cl_explosion->value == 2 || cl_explosion->value == 1) {
			CL_Explosion_Particle(pos, 50, true, true);
			CL_ExplosionParticles_Old(pos);
		} else {
			ex = CL_AllocExplosion();
			VectorCopy(pos, ex->ent.origin);
			ex->type = ex_poly;
			ex->ent.flags = RF_FULLBRIGHT | RF_NOSHADOW;
			ex->start = cl.frame.servertime - 100;
			ex->light = 350;
			ex->lightcolor[0] = 1.0;
			ex->lightcolor[1] = 0.5;
			ex->lightcolor[2] = 0.5;
			ex->ent.angles[1] = rand() % 360;
			ex->ent.model = cl_mod_explo4;
			if (frand() < 0.5)
				ex->baseframe = 15;
			ex->frames = 15;
		}
		#else
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		#endif
#else
		CL_FindExplosionPlane(pos, 70, dir); 
		re.AddDecal(pos, dir, 1, 0.7, 0, 0.85, 0, 0, 0, 0,
			    56 + ((rand()%21*0.05) - 0.5), 
			    DECAL_EXPLODE, 0, rand()%361);
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
#endif
		if (type == TE_ROCKET_EXPLOSION_WATER)
			S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_FLASHLIGHT:
		MSG_ReadPos(&net_message, pos);
		ent = MSG_ReadShort(&net_message);
		CL_Flashlight(ent, pos);
		break;

	case TE_FORCEWALL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		color = MSG_ReadByte (&net_message);
		CL_ForceWall(pos, pos2, color);
		break;

	case TE_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_heatbeam);
		break;

	case TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_monster_heatbeam);
		break;

	case TE_HEATBEAM_SPARKS:
		cnt = 50;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		r = 8;
		magnitude = 60;
		color = r & 0xff;
#if defined(PARTICLESYSTEM)
		CL_ParticleSteamEffect(pos, dir, 240, 240, 240, -20, -20, -20, cnt, magnitude);
#else
		CL_ParticleSteamEffect(pos, dir, color, cnt, magnitude);
#endif
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
	
	case TE_HEATBEAM_STEAM:
		cnt = 20;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		magnitude = 60;
		color = 0xe0;
#if defined(PARTICLESYSTEM)
		CL_ParticleSteamEffect(pos, dir, 255, 150, 50, 0, -90, -30, cnt, magnitude);
		CL_ParticleBlasterDecal(pos, dir, 7.5);
#else
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
#endif
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;

	case TE_BUBBLETRAIL2:
//		cnt = MSG_ReadByte (&net_message);
		cnt = 8;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BubbleTrail2 (pos, pos2, cnt);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_MOREBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 0xe8, 250);
		break;

	case TE_CHAINFIST_SMOKE:
		dir[0]=0; dir[1]=0; dir[2]=1;
		MSG_ReadPos(&net_message, pos);
#if defined(PARTICLESYSTEM)
		CL_ParticleSmokeEffect(pos, dir, 25, 1);
#else
		CL_ParticleSmokeEffect (pos, dir, 0, 20, 20);
#endif
		break;

	case TE_ELECTRIC_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
//		CL_ParticleEffect (pos, dir, 109, 40);
#if defined(PARTICLESYSTEM)
		CL_ElectricParticles(pos, dir, 40);
#else
		CL_ParticleEffect (pos, dir, 0x75, 40);
#endif
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_TRACKER_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_ColorFlash (pos, 0, 150, -1, -1, -1);
		#if 1
		CL_ColorExplosionParticles(pos, 0, 1);
		#else
		CL_Tracker_Explode (pos);
		#endif
		S_StartSound (pos, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0);
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		MSG_ReadPos (&net_message, pos);
		CL_TeleportParticles (pos);
		break;

	case TE_WIDOWBEAMOUT:
		CL_ParseWidow ();
		break;

	case TE_NUKEBLAST:
		CL_ParseNuke ();
		break;

	case TE_WIDOWSPLASH:
		MSG_ReadPos (&net_message, pos);
		CL_WidowSplash (pos);
		break;

	//NEW CUSTOM TEMP EVENTS

#if defined(PARTICLESYSTEM)
	case TE_LIGHTNINGFLARE: //-psychospaz
		{
			int num, entity;

			entity = MSG_ReadShort (&net_message);
			num = MSG_ReadShort (&net_message);
			MSG_ReadPos (&net_message, pos);

			//double up for opacity endurance
			CL_LightningFlare(pos, entity, 2*num);
			CL_LightningFlare(pos, entity, 2*num+1);
		}
		break;	
	case TE_SMOKEPUFF:
		{
			float size;

			MSG_ReadPos (&net_message, pos);
			MSG_ReadPos (&net_message, dir);
			size = MSG_ReadFloat(&net_message);

			CL_ParticleSmokeEffect (pos, dir, size, 1);
		}
		break;
	case TE_IMPACTMARK:
		{
			float	size;
			int	flags;

			MSG_ReadPos (&net_message, pos);
			MSG_ReadPos (&net_message, dir);
			size = MSG_ReadFloat(&net_message);
			flags = MSG_ReadLong (&net_message);

			CL_ImpactMark (pos, dir, size, flags, particle_impact);
		}
		break;
	case TE_FOOTPRINT:
		{
			float size;
			vec3_t color, angle;

			MSG_ReadPos (&net_message, pos);
			MSG_ReadPos (&net_message, dir);
			MSG_ReadPos (&net_message, color);
			size = MSG_ReadFloat(&net_message);

			angle[0] = anglemod(dir[0] + 180);
			angle[1] = anglemod(dir[1]);
			angle[2] = anglemod(dir[2]);

			CL_ParticleFootPrint (pos, angle, size, color);
		}
		break;
	case TE_FLAMEBURST:
		{
			float size;

			MSG_ReadPos (&net_message, pos);
			size = MSG_ReadFloat(&net_message);

			CL_FlameBurst (pos, size);
		}
		break;
	case TE_DISRUPTOR_EXPLOSION:
		{
			float	size;

			MSG_ReadPos (&net_message, pos);
			size = MSG_ReadFloat(&net_message);

			CL_Disruptor_Explosion_Particle (pos, size);

			S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		}
		break;
	case TE_DISINTEGRATE:
		{
			ent = MSG_ReadByte(&net_message);
			MSG_ReadPos (&net_message, pos);
			CL_Disintegrate (pos, ent);
		}
		break;
#endif
//PGM
//==============

	default:
		Com_Error (ERR_DROP, "CL_ParseTEnt: bad type");
	}
}

/*
=================
CL_AddBeams
=================
*/
extern cvar_t  *hand;
/* Knightmare- backup of client angles */
extern vec3_t	old_viewangles;
void CL_AddBeams (void)
{
	int			i,j;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	float		model_length;

	/* Knightmare added */
	frame_t        *oldframe;
	player_state_t *ps, *ops;
	qboolean	firstperson, chasecam;
	int		handmult;
	vec3_t		thirdp_grapple_offset;
	vec3_t		grapple_offset_dir;

	/* Knightmare- chasecam grapple offset stuff */
	if (hand) {
		if (hand->value == 2)
			handmult = 1;
		else if (hand->value == 1)
			handmult = -1;
		else
			handmult = 1;
	} else
		handmult = 1;
	VectorSet(thirdp_grapple_offset, 6, 16, 16);
	/* end Knightmare */
	
// update beams
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.stime)
			continue;
			
		/* Knightmare- */
		firstperson = ((b->entity == cl.playernum + 1) && !cl_3dcam->value);
		chasecam = ((b->entity == cl.playernum + 1) && cl_3dcam->value);

		// if coming from the player, update the start position
		if (firstperson) {// entity 0 is the world
			VectorCopy (cl.refdef.vieworg, b->start);
			b->start[2] -= 22;	// adjust for view height
		} else if (chasecam) {
			vec3_t		f, r, u;

			ps = &cl.frame.playerstate;
			j = (cl.frame.serverframe - 1) & UPDATE_MASK;
			oldframe = &cl.frames[j];
			if (oldframe->serverframe != cl.frame.serverframe - 1 || !oldframe->valid)
				oldframe = &cl.frame;
			ops = &oldframe->playerstate;

			AngleVectors(old_viewangles, f, r, u);
			VectorClear(grapple_offset_dir);
			for (j = 0; j < 3; j++) {
				grapple_offset_dir[j] += f[j] * thirdp_grapple_offset[1];
				grapple_offset_dir[j] += r[j] * thirdp_grapple_offset[0] * handmult;
				grapple_offset_dir[j] += u[j] * (-thirdp_grapple_offset[2]);
			}

			for (j = 0; j < 3; j++)
				b->start[j] = cl.predicted_origin[j] + ops->viewoffset[j]
				    + cl.lerpfrac * (ps->viewoffset[j] - ops->viewoffset[j])
				    + grapple_offset_dir[j];
		}
		if (chasecam)
			VectorCopy(b->start, org);
		else
			VectorAdd (b->start, b->offset, org);

	// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
	// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2(dist[2], forward) * -180.0 / M_PI);
			if (pitch < 0)
				pitch += 360.0;
		}

	// add new entities for the beams
		d = VectorNormalize(dist);

		memset (&ent, 0, sizeof(ent));
		if (b->model == cl_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		steps = ceil(d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
//			Com_Printf ("special case\n");
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand()%360;
			V_AddEntity (&ent);			
			return;
		}
		while (d > 0)
		{
			VectorCopy (org, ent.origin);
			ent.model = b->model;
			if (b->model == cl_mod_lightning)
			{
				ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = rand()%360;
			}
			else
			{
				ent.angles[0] = pitch;
				ent.angles[1] = yaw;
				ent.angles[2] = rand()%360;
			}
			
//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			ent.flags |= RF_NOSHADOW;
			V_AddEntity (&ent);

			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}


/*
=================
ROGUE - draw player locked beams
CL_AddPlayerBeams
=================
*/
void CL_AddPlayerBeams (void)
{
	int		i,j;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	int		framenum = 0;
	float		model_length;
	
	float		hand_multiplier;
	frame_t		*oldframe;
	player_state_t	*ps, *ops;
	/* Knightmare-  */
	qboolean	firstperson, chasecam;
	int		newhandmult;
	vec3_t		thirdp_pbeam_offset;
	vec3_t		pbeam_offset_dir;

//PMM
	if (hand)
	{
		if (hand->value == 2)
			hand_multiplier = 0;
		else if (hand->value == 1)
			hand_multiplier = -1;
		else
			hand_multiplier = 1;
	}
	else 
	{
		hand_multiplier = 1;
	}
//PMM
	/* Knightmare- chasecam beam offset stuff */
	newhandmult = hand_multiplier;
	if (newhandmult == 0)
		newhandmult = 1;

	VectorSet(thirdp_pbeam_offset, 6.5, 0, 12);
	/* end Knightmare */

// update beams
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++)
	{
		vec3_t		f,r,u;
		
		if (!b->model || b->endtime < cl.stime)
			continue;

		firstperson = ((b->entity == cl.playernum + 1) && !cl_3dcam->value);
		chasecam = ((b->entity == cl.playernum + 1) && cl_3dcam->value);

		if(cl_mod_heatbeam && (b->model == cl_mod_heatbeam))
		{

			// if coming from the player, update the start position
			if (firstperson || chasecam)	// entity 0 is the world
			{	
				// set up gun position
				// code straight out of CL_AddViewWeapon
				ps = &cl.frame.playerstate;
				j = (cl.frame.serverframe - 1) & UPDATE_MASK;
				oldframe = &cl.frames[j];
				if (oldframe->serverframe != cl.frame.serverframe-1 || !oldframe->valid)
					oldframe = &cl.frame;		// previous frame was dropped or involid
				ops = &oldframe->playerstate;
				/* Knightmare- lerp for chasecam mode */
				if (chasecam) {	/* Knightmare- use player's original viewangles */
					AngleVectors(old_viewangles, f, r, u);
					VectorClear(pbeam_offset_dir);
					for (j = 0; j < 3; j++) {
						pbeam_offset_dir[j] += f[j] * thirdp_pbeam_offset[1];
						pbeam_offset_dir[j] += r[j] * thirdp_pbeam_offset[0] * newhandmult;
						pbeam_offset_dir[j] += u[j] * (-thirdp_pbeam_offset[2]);
					}
					for (j = 0; j < 3; j++)
						b->start[j] = cl.predicted_origin[j] + ops->viewoffset[j]
						    + cl.lerpfrac * (ps->viewoffset[j] - ops->viewoffset[j])
						    + pbeam_offset_dir[j];
					VectorMA(b->start, (newhandmult * b->offset[0]), r, org);
					VectorMA(org, b->offset[1], f, org);
					VectorMA(org, b->offset[2], u, org);
				} else {
					for (j=0 ; j<3 ; j++) {
						b->start[j] = cl.refdef.vieworg[j] + ops->gunoffset[j]
							+ cl.lerpfrac * (ps->gunoffset[j] - ops->gunoffset[j]);
					}
					VectorMA (b->start, (hand_multiplier * b->offset[0]), cl.v_right, org);
					VectorMA (     org, b->offset[1], cl.v_forward, org);
					VectorMA (     org, b->offset[2], cl.v_up, org);
					if ((hand) && (hand->value == 2)) {
						VectorMA (org, -1, cl.v_up, org);
					}
					// FIXME - take these out when final
					VectorCopy (cl.v_right, r);
					VectorCopy (cl.v_forward, f);
					VectorCopy (cl.v_up, u);
				}

			}
			else
				VectorCopy (b->start, org);
		}
		else
		{
			// if coming from the player, update the start position
			if (firstperson)	// entity 0 is the world
			{
				VectorCopy (cl.refdef.vieworg, b->start);
				b->start[2] -= 22;	// adjust for view height
			}
			VectorAdd (b->start, b->offset, org);
		}

		// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

//PMM
		if(cl_mod_heatbeam && (b->model == cl_mod_heatbeam) && (firstperson || chasecam))
		{
			vec_t len;

			len = VectorLength (dist);
			VectorScale (f, len, dist);
			if (chasecam)
				VectorMA(dist, (newhandmult * b->offset[0]), r, dist);
			else
				VectorMA (dist, (hand_multiplier * b->offset[0]), r, dist);
			VectorMA (dist, b->offset[1], f, dist);
			VectorMA (dist, b->offset[2], u, dist);
			if (chasecam) {
				VectorMA(dist, -(newhandmult * thirdp_pbeam_offset[0]), r, dist);
				VectorMA(dist, thirdp_pbeam_offset[1], f, dist);
				VectorMA(dist, thirdp_pbeam_offset[2], u, dist);
			}
			if ((hand) && (hand->value == 2) && !chasecam) {
				VectorMA (org, -1, cl.v_up, org);
			}
		}
//PMM

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
	// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2(dist[2], forward) * -180.0 / M_PI);
			if (pitch < 0)
				pitch += 360.0;
		}
		
		if (cl_mod_heatbeam && (b->model == cl_mod_heatbeam)) {
			if (!firstperson) {
				framenum = 2;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = 0;
				
				if (!chasecam) {
					AngleVectors(ent.angles, f, r, u);
					
					// if it's a non-origin offset, it's a player, so use the hardcoded player offset
					if (!VectorCompare (b->offset, vec3_origin)) {
						VectorMA (org, -(b->offset[0])+1, r, org);
						VectorMA (org, -(b->offset[1]), f, org);
						VectorMA (org, -(b->offset[2])-10, u, org);
					}
					else {
						// if it's a monster, do the particle effect
						CL_MonsterPlasma_Shell(b->start);
					}
				}
			} 
			else {
				framenum = 1;
			}
		}

		// if it's the heatbeam, draw the particle effect
		if ((cl_mod_heatbeam && (b->model == cl_mod_heatbeam) && (firstperson || chasecam))) {
			CL_Heatbeam (org, dist);
		}

		// add new entities for the beams
		d = VectorNormalize(dist);

		memset (&ent, 0, sizeof(ent));
		if (b->model == cl_mod_heatbeam) {
			model_length = 32.0;
		}
		else if (b->model == cl_mod_lightning) {
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else {
			model_length = 30.0;
		}
		steps = ceil(d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT|RF_NOSHADOW;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand()%360;
			V_AddEntity (&ent);
			return;
		}
		while (d > 0)
		{
			VectorCopy (org, ent.origin);
			ent.model = b->model;
			if(cl_mod_heatbeam && (b->model == cl_mod_heatbeam))
			{
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = (cl.stime) % 360;
				ent.frame = framenum;
			}
			else if (b->model == cl_mod_lightning)
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = rand()%360;
			}
			else
			{
				ent.angles[0] = pitch;
				ent.angles[1] = yaw;
				ent.angles[2] = rand()%360;
			}
			ent.flags |= RF_NOSHADOW;
			
			V_AddEntity (&ent);

			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}

/*
=================
CL_AddExplosions
=================
*/
void CL_AddExplosions (void)
{
	entity_t	*ent;
	explosion_t	*ex, *next, *hnode;
	float		frac;
	int			f;

	memset (&ent, 0, sizeof(ent));

	hnode = &cl_explosions_headnode;
	for (ex = hnode->next; ex != hnode; ex = next)
	{
		next = ex->next;
		ent = &ex->ent;
		
		frac = (cl.stime - ex->start)/100.0;
		f = floor(frac);

		switch (ex->type)
		{
		case ex_mflash:
			if (f >= ex->frames-1)
				ex->type = ex_free;
			break;
		case ex_misc:
			if (f >= ex->frames-1)
			{
				ex->type = ex_free;
				break;
			}
			ent->alpha = 1.0 - frac/(ex->frames-1);
			break;
		case ex_flash:
			if (f >= 1)
			{
				ex->type = ex_free;
				break;
			}
			ent->alpha = 1.0;
			break;
		case ex_poly:
			if (f >= ex->frames-1)
			{
				ex->type = ex_free;
				break;
			}

			ent->alpha = (16.0 - (float)f)/16.0;

			if (f < 10)
			{
				ent->skinnum = (f>>1);
				if (ent->skinnum < 0)
					ent->skinnum = 0;
			}
			else
			{
				ent->flags |= RF_TRANSLUCENT|RF_NOSHADOW;
				if (f < 13)
					ent->skinnum = 5;
				else
					ent->skinnum = 6;
			}
			break;
		case ex_poly2:
			if (f >= ex->frames-1)
			{
				ex->type = ex_free;
				break;
			}

			ent->alpha = (5.0 - (float)f)/5.0;
			ent->skinnum = 0;
			ent->flags |= RF_TRANSLUCENT|RF_NOSHADOW;
			break;
		default:
			break;
		}

		if (ex->type == ex_free) 
		{
			CL_FreeExplosion (ex);
			continue;
		}
		
		if (ex->light)
		{
			V_AddLight (ent->origin, ex->light*ent->alpha,
				ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2]);
		}

		VectorCopy (ent->origin, ent->oldorigin);

		if (f < 0)
			f = 0;
		ent->frame = ex->baseframe + f + 1;
		ent->oldframe = ex->baseframe + f;
		ent->backlerp = 1.0 - cl.lerpfrac;

		V_AddEntity (ent);
	}
}


/*
=================
CL_AddLasers
=================
*/
void CL_AddLasers (void)
{
	laser_t		*l;
	int			i;

	for (i=0, l=cl_lasers ; i< MAX_LASERS ; i++, l++)
	{
		if (l->endtime >= cl.stime)
			V_AddEntity (&l->ent);
	}
}

/* PMM - CL_Sustains */
void CL_ProcessSustain (void)
{
	cl_sustain_t	*s;
	int				i;

	for (i=0, s=cl_sustains; i< MAX_SUSTAINS; i++, s++)
	{
		if (s->id) {
			if ((s->endtime >= cl.stime) && (cl.stime >= s->nextthink))
			{
//				Com_Printf ("think %d %d %d\n", cl.time, s->nextthink, s->thinkinterval);
				s->think (s);
			}
			else if (s->endtime < cl.stime)
				s->id = 0;
		}
	}
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts (void)
{
	CL_AddBeams ();
	// PMM - draw plasma beams
	CL_AddPlayerBeams ();
	CL_AddExplosions ();
	CL_AddLasers ();
	// PMM - set up sustain
	CL_ProcessSustain();
}

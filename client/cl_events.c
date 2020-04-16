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
// cl_events.c

#include "client.h"

extern struct sfx_s	*cl_sfx_footsteps[4];

// Knightmare- footstep definition stuff
#define MAX_TEX_SURF 256
struct texsurf_s
{
	int	step_id;
	char	tex[32];
};

typedef struct texsurf_s texsurf_t;

texsurf_t tex_surf[MAX_TEX_SURF];
int	num_texsurfs;

//Knightmare- Lazarus footstep sounds
extern struct sfx_s	*cl_sfx_metal_footsteps[4];
extern struct sfx_s	*cl_sfx_dirt_footsteps[4];
extern struct sfx_s	*cl_sfx_vent_footsteps[4];
extern struct sfx_s	*cl_sfx_grate_footsteps[4];
extern struct sfx_s	*cl_sfx_tile_footsteps[4];
extern struct sfx_s	*cl_sfx_grass_footsteps[4];
extern struct sfx_s	*cl_sfx_snow_footsteps[4];
extern struct sfx_s	*cl_sfx_carpet_footsteps[4];
extern struct sfx_s	*cl_sfx_force_footsteps[4];
extern struct sfx_s	*cl_sfx_gravel_footsteps[4];
extern struct sfx_s	*cl_sfx_ice_footsteps[4];
extern struct sfx_s	*cl_sfx_sand_footsteps[4];
extern struct sfx_s	*cl_sfx_wood_footsteps[4];

extern struct sfx_s	*cl_sfx_slosh[4];
extern struct sfx_s	*cl_sfx_wade[4];
extern struct sfx_s	*cl_sfx_ladder[4];

/*
===============
ReadTextureSurfaceAssignments
Reads in defintions for footsteps based on texture name.
===============
*/
qboolean 
buf_gets (char *dest, int destsize, char **f)
{
	char *old = *f;
	*f = strchr (old, '\n');
	if (!*f)
	{	// no more new lines
		*f = old + strlen(old);
		if (!strlen(*f))
			return false;	// end of file, nothing else to grab
	}
	(*f)++; // advance past EOL
	Q_strncpyz(dest, old, min(destsize, (int)(*f-old)));
	return true;
}

void 
ReadTextureSurfaceAssignments(void)
{
	char	filename[MAX_OSPATH];
	char	*footstep_data;
	char	*parsedata;
	char	line[80];

	num_texsurfs = 0;

	Com_sprintf (filename, sizeof(filename), "scripts/texsurfs.txt");
	
	FS_LoadFile (filename, (void **)&footstep_data);
	
	parsedata = footstep_data;
	
	if (!footstep_data) 
		return;
	
	while (buf_gets(line, sizeof(line), &parsedata) && num_texsurfs < MAX_TEX_SURF) {
		sscanf(line,"%d %s",&tex_surf[num_texsurfs].step_id,tex_surf[num_texsurfs].tex);
		/* Com_Printf("%d %s\n",tex_surf[num_texsurfs].step_id,tex_surf[num_texsurfs].tex); */
		num_texsurfs++;
	}
	
	FS_FreeFile (footstep_data);
}


/*
===============
CL_FootSteps
Plays appropriate footstep sound depending on surface flags of the ground surface.
Since this is a replacement for plain Jane EV_FOOTSTEP, we already know
the player is definitely on the ground when this is called.
===============
*/
void CL_FootSteps (entity_state_t *ent, qboolean loud)
{
	trace_t		tr;
	vec3_t		end;
	int		r;
	int		surface;
	struct	sfx_s	*stepsound = NULL;
	float	volume = cl_footsteps_volume->value;

	r = (rand()&3);

	VectorCopy(ent->origin,end);
	end[2] -= 64;
	
	tr = CL_PMSurfaceTrace (ent->number, ent->origin,NULL,NULL,end,MASK_SOLID | MASK_WATER);
	
	if (!tr.surface)
		return;
	
	surface = tr.surface->flags & SURF_STEPMASK;
	
	switch (surface)
	{
	case SURF_METAL:
		stepsound = cl_sfx_metal_footsteps[r];
		break;
	case SURF_DIRT:
		stepsound = cl_sfx_dirt_footsteps[r];
		break;
	case SURF_VENT:
		stepsound = cl_sfx_vent_footsteps[r];
		break;
	case SURF_GRATE:
		stepsound = cl_sfx_grate_footsteps[r];
		break;
	case SURF_TILE:
		stepsound = cl_sfx_tile_footsteps[r];
		break;
	case SURF_GRASS:
		stepsound = cl_sfx_grass_footsteps[r];
		break;
	case SURF_SNOW:
		stepsound = cl_sfx_snow_footsteps[r];
		break;
	case SURF_CARPET:
		stepsound = cl_sfx_carpet_footsteps[r];
		break;
	case SURF_FORCE:
		stepsound = cl_sfx_force_footsteps[r];
		break;
	case SURF_GRAVEL:
		stepsound = cl_sfx_gravel_footsteps[r];
		break;
	case SURF_ICE:
		stepsound = cl_sfx_ice_footsteps[r];
		break;
	case SURF_SAND:
		stepsound = cl_sfx_sand_footsteps[r];
		break;
	case SURF_WOOD:
		stepsound = cl_sfx_wood_footsteps[r];
		break;
	case SURF_STANDARD:
		stepsound = cl_sfx_footsteps[r];
		volume = 1.0;
		break;
	default:
		if (cl_footsteps_override->value && num_texsurfs)
		{
			int	i;
			for (i=0; i<num_texsurfs; i++)
				if (strstr(tr.surface->name,tex_surf[i].tex) && tex_surf[i].step_id > 0)
				{
					tr.surface->flags |= (SURF_METAL << (tex_surf[i].step_id - 1));
					CL_FootSteps (ent, loud); // start over
					return;
				}
		}
		tr.surface->flags |= SURF_STANDARD;
		CL_FootSteps (ent, loud); // start over
		return;
	}

	if (loud)
	{
		if (volume == 1.0)
			S_StartSound (NULL, ent->number, CHAN_AUTO, stepsound, 1.0, ATTN_NORM, 0);
		else
			volume = 1.0;
	}
	
	S_StartSound (NULL, ent->number, CHAN_BODY, stepsound, volume, ATTN_NORM, 0);
}
//end Knightmare


/*
==============
CL_EntityEvent

An entity has just been parsed that has an event value

the female events are there for backwards compatability
==============
*/

void CL_EntityEvent (entity_state_t *ent)
{
	switch (ent->event)
	{
	case EV_ITEM_RESPAWN:
		S_StartSound (NULL, ent->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CL_ItemRespawnParticles (ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		S_StartSound (NULL, ent->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CL_TeleportParticles (ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			CL_FootSteps (ent, false);
		break;
	case EV_LOUDSTEP:
		if (cl_footsteps->value)
			CL_FootSteps (ent, true);
		break;
	case EV_FALLSHORT:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
		break;
	case EV_SLOSH:
		S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_slosh[rand()&3], 0.5, ATTN_NORM, 0);
		break;
	case EV_WADE:
		S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_wade[rand()&3], 0.5, ATTN_NORM, 0);
		break;
	case EV_CLIMB_LADDER:
		S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_ladder[rand()&3], 0.5, ATTN_NORM, 0);
		break;
	}
}

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
// cl_view.c -- player rendering positioning

#include "client.h"

//=============
//
// development tools for weapons
//

//=============

cvar_t		*cl_testparticles;
cvar_t		*cl_testentities;
cvar_t		*cl_testlights;
cvar_t		*cl_testblend;

cvar_t		*cl_stats;

int		r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int		r_numentities;
entity_t	r_entities[MAX_ENTITIES];

int		r_numparticles;
particle_t	r_particles[MAX_PARTICLES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

int		r_numflares;
flare_t		r_flares[MAX_FLARES];

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

qboolean	flashlight_eng;

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
void V_ClearScene (void)
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
}

/*
* ======================== 
* 3D Cam Stuff -psychospaz
* ========================
*/

float viewermodelalpha;
void AddViewerEntAlpha(entity_t * ent)
{
	if (viewermodelalpha == 1 || !cl_3dcam_alpha->value)
		return;

	ent->alpha *= viewermodelalpha;
	if (ent->alpha < .9)
		ent->flags |= RF_TRANSLUCENT;
}

void CalcViewerCamTrans(float distance)
{
	float		alphacalc = cl_3dcam_dist->value;

	/* no div by 0 */
	if (alphacalc < 1)
		alphacalc = 1;

	viewermodelalpha = distance / alphacalc;

	if (viewermodelalpha > 1)
		viewermodelalpha = 1;
}


/*
=====================
V_AddEntity

=====================
*/
void V_AddEntity (entity_t *ent)
{
	if (ent->flags & RF_VIEWERMODEL) {	/* here is our client */
		int		i;

		for (i = 0; i < 3; i++)
			ent->oldorigin[i] = ent->origin[i] = cl.predicted_origin[i];

		/* saber hack */
		if (ent->renderfx)
			ent->flags &= ~RF_VIEWERMODEL;

		if (cl_3dcam->value && !(cl.attractloop && !(cl.cinematictime > 0 && cls.realtime - cl.cinematictime > 1000))) {
			AddViewerEntAlpha(ent);
			ent->flags &= ~RF_VIEWERMODEL;
		} else if (ent->renderfx) {
			ent->flags &= ~RF_VIEWERMODEL;
		}
	}
	
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities++] = *ent;
}


/*
=====================
V_AddParticle

=====================
*/
#if defined(PARTICLESYSTEM)
void V_AddParticle(vec3_t org, vec3_t angle, vec3_t color, 
                   float alpha, int alpha_src, int alpha_dst, 
		   float size, int image, int flags, 
		   decalpolys_t *decal)
{
	int i;
	particle_t	*p;

	if (r_numparticles >= MAX_PARTICLES)
		return;
	p = &r_particles[r_numparticles++];

	for (i=0;i<3;i++) {
		p->origin[i] = org[i];
		p->angle[i] = angle[i];
	}
	
	p->red = color[0];
	p->green = color[1];
	p->blue = color[2];
	p->alpha = alpha;
	p->image = image;
	p->flags = flags;
	p->size  = size;
	p->decal = decal;
	p->blendfunc_src = alpha_src;
	p->blendfunc_dst = alpha_dst;
	
}
#else
void V_AddParticle (vec3_t org, int color, float alpha)
{
	particle_t	*p;

	if (r_numparticles >= MAX_PARTICLES)
		return;
	p = &r_particles[r_numparticles++];
	VectorCopy (org, p->origin);
	p->color = color;
	p->alpha = alpha;
}
#endif

/*
=====================
V_AddLight

=====================
*/
void V_AddLight (vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
#if 0 //defined(PARTICLESYSTEM)
	VectorCopy (vec3_origin, dl->direction);
#endif
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
#if 0 //defined(PARTICLESYSTEM)
	dl->spotlight = false;
#endif
}

#if 0 //defined(PARTICLESYSTEM)
void V_AddSpotLight (vec3_t org, vec3_t direction, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	VectorCopy(direction, dl->direction);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;

	dl->spotlight=true;
}
#endif


/*
=====================
V_AddLightStyle

=====================
*/
void V_AddLightStyle (int style, float r, float g, float b)
{
	lightstyle_t	*ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
		Com_Error (ERR_DROP, "Bad light style %i", style);
	ls = &r_lightstyles[style];

	ls->white = r+g+b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void V_TestParticles (void)
{
	particle_t	*p;
	int			i, j;
	float		d, r, u;

	r_numparticles = MAX_PARTICLES;
	for (i=0 ; i<r_numparticles ; i++)
	{
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);
		p = &r_particles[i];

		for (j=0 ; j<3 ; j++)
			p->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*d +
			cl.v_right[j]*r + cl.v_up[j]*u;

		p->color = 8;
		p->alpha = cl_testparticles->value;
	}
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
void V_TestEntities (void)
{
	int			i, j;
	float		f, r;
	entity_t	*ent;

	r_numentities = 32;
	memset (r_entities, 0, sizeof(r_entities));

	for (i=0 ; i<r_numentities ; i++)
	{
		ent = &r_entities[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;

		ent->model = cl.baseclientinfo.model;
		ent->skin = cl.baseclientinfo.skin;
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	int		i, j;
	float		f, r;
	dlight_t	*dl;

	r_numdlights = 32;
	memset (r_dlights, 0, sizeof(r_dlights));

	for (i=0 ; i<r_numdlights ; i++)
	{
		dl = &r_dlights[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;
		dl->color[0] = ((i%6)+1) & 1;
		dl->color[1] = (((i%6)+1) & 2)>>1;
		dl->color[2] = (((i%6)+1) & 4)>>2;
		dl->intensity = 200;
#if 0 //defined(PARTICLESYSTEM)
		dl->spotlight=true;
#endif
	}
}

/*
*
* ================
* V_FlashLight
*
* Flashlight managed by the client. It uses r_dlights. Creates two lights, one
* near the player (emited by the flashlight to the sides), and the main one
* where the player is aiming at.
* ================
*/
void V_FlashLight(void)
{
	float		dist;	/* Distance from player to light. */
	dlight_t       *dl;	/* Light array pointer. */
	trace_t		trace;	/* Trace to the destination. */
	vec3_t		vdist;	/* Distance from player to light (vector). */
	vec3_t		end;	/* Flashlight max distance. */

	r_numdlights = 1;
	memset(r_dlights, 0, sizeof(r_dlights));
	dl = r_dlights;

	/* Small light near the player. */
	VectorMA(cl.predicted_origin, 50, cl.v_forward, dl->origin);
	VectorSet(dl->color, cl_flashlight_red->value,
	          cl_flashlight_green->value, cl_flashlight_blue->value);

	dl->intensity = cl_flashlight_intensity->value * 0.60;
	
	VectorMA(cl.predicted_origin, cl_flashlight_distance->value,
	    cl.v_forward, end);
	trace = CL_Trace(cl.predicted_origin, end, 0, MASK_SOLID);

	if (trace.fraction == 1)
		return;

	/* Main light. */
	r_numdlights++;
	dl++;

	VectorSubtract(cl.predicted_origin, trace.endpos, vdist);
	dist = VectorLength(vdist);

	VectorCopy(trace.endpos, dl->origin);
	VectorSet(dl->color, cl_flashlight_red->value,
	          cl_flashlight_green->value, cl_flashlight_blue->value);
	    
	dl->intensity = cl_flashlight_intensity->value *
	                (1.0 - (dist / cl_flashlight_distance->value) *
	                cl_flashlight_decscale->value);
}

//===================================================================

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh (void)
{
	char		mapname[32];
	int		i, max;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;
	int		map_time;
	qboolean	newPlaque = needLoadingPlaque();
	
	map_time = Sys_Milliseconds();

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded
    
	if (newPlaque)
		SCR_BeginLoadingPlaque();
 
	loadingMessage = true;
	Com_sprintf (loadingMessages[0], sizeof(loadingMessages[0]), "Loading Map...");
	Com_sprintf (loadingMessages[1], sizeof(loadingMessages[1]), "Loading Models...");
	Com_sprintf (loadingMessages[2], sizeof(loadingMessages[2]), "Loading Pics...");
	Com_sprintf (loadingMessages[3], sizeof(loadingMessages[3]), "Loading Clients...");
	loadingPercent = 0;

	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);

	// let the render dll load the map
	strcpy (mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[strlen(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf ("Map: %s\r", mapname); 
	SCR_UpdateScreen ();
	re.BeginRegistration (mapname);
	Com_Printf ("                                     \r");
	Com_sprintf (loadingMessages[0], sizeof(loadingMessages[0]), "Loading Map...^1done^r");
	loadingPercent += 20;

	// precache status bar pics
	Com_Printf ("pics\r"); 
	SCR_UpdateScreen ();
	SCR_TouchPics ();
	Com_Printf ("                                     \r");

	CL_RegisterTEntModels ();

	num_cl_weaponmodels = 1;
	strcpy(cl_weaponmodels[0], "weapon.md2");
	
	for (i=1, max=0 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
		max++;

	for (i=1 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++) {
		strcpy (name, cl.configstrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*') {
			Com_Printf ("%s\r", name);
			//only make max of 20 chars long
			Com_sprintf(loadingMessages[1], sizeof(loadingMessages[1]), "Loading Models...^1%s^r", 
				    (strlen(name)>20)? &name[strlen(name)-20]: name);
			//check for types
			blaster = 1;
				chainfist = 1;
			smartgun = 1;
			machinegun = 1;
			sshotgun = 1;
			smachinegun = 1;
				etfrifle = 1;
				trap = 1;
			glauncher = 1;
				gproxlaunch = 1;
			rlauncher = 1;
			hyperblaster = 1;
				ripper = 1;
			railgun = 1;
				heatbeam = 1;
				phallanx = 1;
			bfg = 1;
				tesla = 1;
				ired = 1;
				sonic = 1;
				flare = 1;
			adren = 1;
			quad = 1;
			inv = 1;
				knife = 1;
				cannon = 1;
				akimbo = 1;
				m4 = 1;
				mk23 = 1;
				mp5 = 1;
				sniper = 1;
				super90 = 1;
		}
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				strncpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		} 
		else
		{
			cl.model_draw[i] = re.RegisterModel (cl.configstrings[CS_MODELS+i]);
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel (cl.configstrings[CS_MODELS+i]);
			else
				cl.model_clip[i] = NULL;
		}
		if (name[0] != '*')
			Com_Printf ("                                     \r");
		loadingPercent += 60.0f/(float)max;
	}
	Com_sprintf (loadingMessages[1], sizeof(loadingMessages[1]), "Loading Models...^1done^r");
	
	Com_Printf ("images\r", i); 
	SCR_UpdateScreen ();
	for (i=1, max=0 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
		max++;
	for (i=1 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
	{
		cl.image_precache[i] = re.RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
		loadingPercent += 10.0f/(float)max;
	}
	Com_sprintf (loadingMessages[2], sizeof(loadingMessages[2]), "Loading Pics...^1done^r");
	
	Com_Printf ("                                     \r");
	for (i=1, max=0 ; i<MAX_CLIENTS ; i++)
		if (cl.configstrings[CS_PLAYERSKINS+i][0])
			max++;
	for (i=0 ; i<MAX_CLIENTS ; i++) {
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_sprintf (loadingMessages[3], sizeof(loadingMessages[3]), "Loading Clients...^1%i^r", i);
		Com_Printf ("client %i\r", i); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
		Com_Printf ("                                     \r");
		loadingPercent += 10.0f/(float)max;
	}
	Com_sprintf (loadingMessages[3], sizeof(loadingMessages[3]), "Loading Clients...^1done^r");
	
	loadingPercent = 100;

	CL_LoadClientinfo (&cl.baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	Com_Printf ("sky\r", i); 
	SCR_UpdateScreen ();
	rotate = atof (cl.configstrings[CS_SKYROTATE]);
	sscanf (cl.configstrings[CS_SKYAXIS], "%f %f %f", 
		&axis[0], &axis[1], &axis[2]);
	re.SetSky (cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf ("                                     \r");

	// the renderer can now free unneeded stuff
	re.EndRegistration ();

	// clear any lines of console text
	Con_ClearNotify ();
	
	CL_LoadLoc();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

#if defined(CDAUDIO)
	// start the cd track
	CDAudio_Play (atoi(cl.configstrings[CS_CDTRACK]), true);
#endif
	
	loadingMessage = false;
	
	if (newPlaque)
		SCR_EndLoadingPlaque();
	else
		Cvar_Set ("paused", "0");

	Com_Printf("Map load time: %.2fs\n\n", (Sys_Milliseconds () - map_time) * 0.001);
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

/*
==================
V_RenderView

==================
*/
void V_RenderView( float stereo_separation )
{
	extern int entitycmpfnc( const entity_t *, const entity_t * );

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		V_ClearScene ();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_flashlight->value && flashlight_eng) 
				V_FlashLight();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// offset vieworg appropriately if we're doing stereo separation
		if ( stereo_separation != 0 )
		{
			vec3_t tmp;

			VectorScale( cl.v_right, stereo_separation, tmp );
			VectorAdd( cl.refdef.vieworg, tmp, cl.refdef.vieworg );
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.stime*0.001;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (cl_underwater_trans->value && cl.refdef.rdflags & RDF_UNDERWATER) {
			cl.refdef.blend[0] = 0;
			cl.refdef.blend[1] = 0;
			cl.refdef.blend[2] = 0;
			cl.refdef.blend[3] = 0;
			VectorClear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;
		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;
		cl.refdef.num_flares = r_numflares;
		cl.refdef.flares = r_flares;
		cl.refdef.rdflags = cl.frame.playerstate.rdflags;
		
		if (((cl_underwater_movement->value) && cl.refdef.rdflags & RDF_UNDERWATER) && !cl_3dcam->value) {
			float f = sin(cl.stime * 0.001 * 0.4 * M_PI2);
			cl.refdef.fov_x += f;
			cl.refdef.fov_y -= f;
		}

		// sort entities for better cache locality
#if defined(PARTICLESYSTEM)
		if (!gl_transrendersort->value)
#endif
        	qsort(cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), 
		      (int (*)(const void *, const void *))entitycmpfnc );
	}

	cl.refdef.rdflags |= RDF_BLOOM;
	re.RenderFrame (&cl.refdef);
	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);

}


/*
=============
V_Viewpos_f
=============
*/
void V_Viewpos_f (void)
{
	Com_Printf ("(%i %i %i) : %i\n", (int)cl.refdef.vieworg[0],
		(int)cl.refdef.vieworg[1], (int)cl.refdef.vieworg[2], 
		(int)cl.refdef.viewangles[YAW]);
}


// Knightmare- diagnostic commands from Lazarus
/*
=============
V_Texture_f
=============
*/
void V_Texture_f (void)
{
	trace_t	tr;
	vec3_t	forward, start, end;
	
	VectorCopy(cl.refdef.vieworg, start);
	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(start, 8192, forward, end);
	tr = CL_PMSurfaceTrace(cl.playernum+1, start,NULL,NULL,end,MASK_ALL);
	if (!tr.ent)
		Com_Printf("Nothing hit?\n");
	else {
		if (!tr.surface)
			Com_Printf("Not a brush\n");
		else {
			if (developer->value) 
				Com_Printf("Texture: %s, Surface: 0x%08x, Value: %d\n",
				tr.surface->name,tr.surface->flags,tr.surface->value);
			
			else 
				Com_Printf("Texture: ^3%s^r\n", tr.surface->name);
		}	
	}
}

/*
=============
V_FlashLight_f
=============
*/
void V_FlashLight_f(void)
{
	static const char *flashlight_sounds[] = {
		"world/lite_out.wav",
		"boss3/w_loop.wav"
	};
	
	if (!cl_flashlight->value)
		return;
	
	flashlight_eng = !flashlight_eng;
	Cbuf_AddText(va("echo Flashlight %s\n", flashlight_eng ? "on" : "off"));

	if (cl_flashlight_sound->value != 0)
		Cbuf_AddText(va("play %s\n", flashlight_sounds[flashlight_eng]));
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("texture", V_Texture_f);
	Cmd_AddCommand ("viewpos", V_Viewpos_f);
	Cmd_AddCommand("flashlight_eng", V_FlashLight_f);

	cl_testblend = Cvar_Get ("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get ("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get ("cl_testlights", "0", 0);
	cl_stats = Cvar_Get ("cl_stats", "0", 0);
	
	cl_flashlight = Cvar_Get("cl_flashlight", "0", 0);
	cl_flashlight_decscale = Cvar_Get("cl_flashlight_decscale", "0.6", CVAR_ARCHIVE);
	cl_flashlight_distance = Cvar_Get("cl_flashlight_distance", "900", CVAR_ARCHIVE);
	cl_flashlight_sound = Cvar_Get("cl_flashlight_sound", "1", CVAR_ARCHIVE);
	cl_flashlight_red = Cvar_Get("cl_flashlight_red", "1.0", CVAR_ARCHIVE);
	cl_flashlight_green = Cvar_Get("cl_flashlight_green", "1.0", CVAR_ARCHIVE);
	cl_flashlight_blue = Cvar_Get("cl_flashlight_blue", "1.0", CVAR_ARCHIVE);
	cl_flashlight_intensity = Cvar_Get("cl_flashlight_intensity", "150", CVAR_ARCHIVE);
	
	flashlight_eng = false;
}

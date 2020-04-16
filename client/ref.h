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
#ifndef __REF_H
#define __REF_H

#include "../qcommon/qcommon.h"

#if defined(PARTICLESYSTEM)
#define	MAX_DLIGHTS	4096
#define	MAX_ENTITIES	4096
#else
#define	MAX_DLIGHTS	32
#define	MAX_ENTITIES	128
#endif

#define MAX_PARTICLES	4096
#define	MAX_LIGHTSTYLES	256
#define MAX_FLARES      1024

#define POWERSUIT_SCALE		0.25F

#define SHELL_RED_COLOR		0xF2
#define SHELL_GREEN_COLOR	0xD0
#define SHELL_BLUE_COLOR	0xF3

#define SHELL_RG_COLOR		0xDC
#define SHELL_RB_COLOR		0x68
#define SHELL_BG_COLOR		0x78

//ROGUE
#define SHELL_DOUBLE_COLOR	0xDF // 223
#define	SHELL_HALF_DAM_COLOR	0x90
#define SHELL_CYAN_COLOR	0x72
//ROGUE

#define SHELL_WHITE_COLOR	0xD7

#define ENTITY_FLAGS  68

#define	API_VERSION		3

#define MENU_CURSOR_BUTTON_MAX  2
#define MENUITEM_ACTION		1
#define MENUITEM_ROTATE		2
#define MENUITEM_SLIDER		3
#define MENUITEM_TEXT		4

typedef struct
{
	vec3_t origin;
	vec3_t color;
	int size;
	int style;
} flare_t;

#if defined(PARTICLESYSTEM)
typedef struct
{
	int	numPoints;
	int	firstPoint;
} markFragment_t;

typedef struct
{
	int	numpolys;
	vec3_t	polys[MAX_DECAL_VERTS];
	vec2_t	coords[MAX_DECAL_VERTS];
} decalpolys_t;
#endif

typedef struct entity_s
{
	struct model_s	*model;    // opaque type outside refresh
	struct image_s	*skin;	  // NULL for inline skin
	float		angles[3];

	/* most recent data */
	float		origin[3]; // also used as RF_BEAM's "from"
	int		frame;     // also used as RF_BEAM's diameter

	/* previous data for lerping */
	float		oldorigin[3];	// also used as RF_BEAM's "to"
	int		oldframe;

	/* misc */
	float		backlerp;   // 0.0 = current, 1.0 = old
	int		skinnum;    // also used as RF_BEAM's palette index
	int		lightstyle; // for flashing entities
	float		alpha;      // ignore if RF_TRANSLUCENT isn't set
	int		flags;
	int		renderfx;
	float		scale;
} entity_t;


typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
#if 0 //defined(PARTICLESYSTEM)
	qboolean spotlight;
	vec3_t	direction;
#endif
} dlight_t;

typedef struct
{
	vec3_t	origin;
	int	color;
	float	alpha;
#if defined(PARTICLESYSTEM)
	vec3_t	angle;
	float	size,
	        red,
		green,
		blue;
	int	image,
	        flags,
	        blendfunc_src,
		blendfunc_dst;
	decalpolys_t *decal;
#endif
} particle_t;

typedef struct
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;

typedef struct
{
	int		x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	float		vieworg[3];
	float		viewangles[3];
	float		blend[4];			// rgba 0-1 full screen blend
	float		time;				// time is uesed to auto animate
	int		rdflags;			// RDF_UNDERWATER, etc

	byte		*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int		num_entities;
	entity_t	*entities;

	int		num_dlights;
	dlight_t	*dlights;

	int		num_particles;
	particle_t	*particles;

	int		num_flares;
	flare_t		*flares;
} refdef_t;

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// if api_version is different, the dll cannot be used
	int		api_version;

	// called when the library is loaded
	int	(*Init) ( void *hinstance, void *wndproc );

	// called before the library is unloaded
	void	(*Shutdown) (void);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".pcx" postfix)
	void	(*BeginRegistration) (char *map);
	struct model_s *(*RegisterModel) (char *name);
	struct image_s *(*RegisterSkin) (char *name);
	struct image_s *(*RegisterPic) (char *name);
	void	(*SetSky) (char *name, float rotate, vec3_t axis);
	void	(*EndRegistration) (void);

	void	(*RenderFrame) (refdef_t *fd);

	void	(*DrawGetPicSize) (int *w, int *h, char *name);	// will return 0 0 if not found
	void	(*DrawPic) (int x, int y, char *name, float alpha);
	void    (*DrawScaledPic) (int x, int y, float scale, float alpha, char *pic, 
	                          float red, float green, float blue, 
				  qboolean fixcoords, qboolean repscale);
	void	(*DrawStretchPic) (int x, int y, int w, int h, char *name, float alpha);
	void	(*DrawStretchPic2) (int x, int y, int w, int h, char *name,
	                            float red, float green, float blue, float alpha);
	void	(*DrawChar) (int x, int y, int c, int alpha);
	void    (*DrawScaledChar) (float x, float y, int num, float scale, 
	                            int red, int green, int blue, int alpha, qboolean italic);
	void	(*DrawTileClear) (int x, int y, int w, int h, char *name);
	void	(*DrawFill) (int x, int y, int w, int h, int c);
	void	(*DrawFill2) (int x, int y, int w, int h, int red, int green, int blue, int alpha);
	void	(*DrawFadeScreen) (void);

	// Draw images for cinematic rendering (which can have a different palette). Note that calls
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, byte *data);

	/*
	** video mode and refresh state management entry points
	*/
	void	(*CinematicSetPalette)( const unsigned char *palette);	// NULL = game palette
	void	(*BeginFrame)( float camera_separation );
	void	(*EndFrame) (void);

	void	(*AppActivate)( qboolean activate );
#if defined(PARTICLESYSTEM)
	void	(*SetParticlePicture) (int num, char *name);
	int	(*AddDecal) (const vec3_t origin, vec3_t axis[3], float radius, 
	                          int maxPoints, vec3_t *points, vec2_t *tcoords,
				  int maxFragments, markFragment_t *fragments);
#else
	// Decals -Maniac
	void    (*AddDecal) (vec3_t origin, vec3_t dir, float red, float green, float blue, 
	                     float alpha, float endRed, float endGreen, float endBlue, 
			     float endAlpha, float size, int type, int flags, float angle);
#endif
} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	void	(*Sys_Error) (int err_level, char *str, ...);

	void	(*Cmd_AddCommand) (char *name, void(*cmd)(void));
	void	(*Cmd_RemoveCommand) (char *name);
	int	(*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int i);
	void	(*Cmd_ExecuteText) (int exec_when, char *text);

	void	(*Con_Printf) (int print_level, char *str, ...);

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int	(*FS_LoadFile) (char *name, void **buf);
	void	(*FS_FreeFile) (void *buf);
	char	**(*FS_ListPak) (char *find, int *num);

	// gamedir will be the current directory that generated
	// files should be stored to, ie: "f:\quake\id1"
	char	*(*FS_Gamedir) (void);
	char	*(*FS_Mapname) (void); // -Maniac

	cvar_t	*(*Cvar_Get) (char *name, char *value, int flags);
	cvar_t	*(*Cvar_Set)( char *name, char *value );
	void	 (*Cvar_SetValue)( char *name, float value );

	qboolean	(*Vid_GetModeInfo)( int *width, int *height, int mode );
	void		(*Vid_MenuInit)( void );
	void		(*Vid_NewWindow)( int width, int height );
	qboolean        (*IsVisible)(vec3_t org1, vec3_t org2);
	void		(*TextColor) (int colornum, int *red, int *green, int *blue);
#if defined(PARTICLESYSTEM)
	void		(*SetParticleImages) (void);
#endif
} refimport_t;

/* Knightmare- added Psychospaz's menu cursor */
/* cursor - psychospaz */
typedef struct 
{
	/* only 2 buttons for menus */
	float		buttontime[MENU_CURSOR_BUTTON_MAX];
	int		buttonclicks[MENU_CURSOR_BUTTON_MAX];
	int		buttonused [MENU_CURSOR_BUTTON_MAX];
	qboolean	buttondown[MENU_CURSOR_BUTTON_MAX];

	qboolean	mouseaction;

	/* this is the active item that cursor is on. */
	int		menuitemtype;
	void           *menuitem;
	void           *menu;

	/* coords */
	int		x;
	int		y;

	int		oldx;
	int		oldy;

	qboolean	mousewheelup;
	qboolean	mousewheeldown;
	
} cursor_t;

// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);

#endif // __REF_H

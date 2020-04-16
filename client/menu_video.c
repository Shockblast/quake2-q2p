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
#if defined (__unix__)
#include <dlfcn.h> // ELF dl loader
#include <sys/stat.h>
#endif

#include "client.h"
#include "qmenu.h"

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
#if 1
extern cvar_t *vid_gamma;
#endif
cvar_t  *gl_lightmap_saturation;
cvar_t  *gl_lightmap_texture_saturation;

#if defined (__unix__)
static cvar_t *gl_driver;
#else
static cvar_t *gl_finish;
#endif
static cvar_t *gl_mode;
static cvar_t *gl_texturemode;

#if defined (__unix__)
/*
 * ====================================================================
 * REF stuff ... Used to dynamically load the menu with only 
 * those vid_ref's that are present on this system
 * ====================================================================
 */
/* this will have to be updated if ref's are added/removed from ref_t */
#define NUMBER_OF_REFS 2

/* all the refs should be initially set to 0 */
static char    *refs[NUMBER_OF_REFS + 1] = {0};

/* make all these have illegal values, as they will be redefined */
static int	REF_GLX = NUMBER_OF_REFS;
static int	REF_SDLGL = NUMBER_OF_REFS;

static int	GL_REF_START = NUMBER_OF_REFS;

typedef struct {
	char	menuname[32];
	char	realname[32];
	int    *pointer;
} ref_t;

static const ref_t possible_refs[NUMBER_OF_REFS] =
{
	{"X11 OpenGL", "gl", &REF_GLX},
	{"SDL OpenGL", "sdl", &REF_SDLGL}
};

/*
 * ============ VID_CheckRefExists
 *
 * Checks to see if the given ref_NAME.so exists. Placed here to avoid
 * complicating other code if the library .so files ever have their names
 * changed. ============
 */
qboolean
VID_CheckRefExists(const char *ref)
{
	char         fn[MAX_OSPATH];
	char        *path;
	struct stat  st;

#if defined(LIBDIR)
	path = LIBDIR;
#elif defined(DATADIR)
	path = Cvar_Get("basedir", DATADIR, CVAR_NOSET)->string;
#else
	path = Cvar_Get("basedir", ".", CVAR_NOSET)->string;
#endif
	Com_sprintf(fn, sizeof(fn), "%s/vid_%s.so", path, ref);

	if (stat(fn, &st) == 0)
		return true;
	else
		return false;
}
#endif

/*
====================================================================

MENU INTERACTION

====================================================================
*/

#if defined (__unix__) 
#define OPENGL_MENU   0
#endif

static menuframework_s	s_opengl_menu;
static menuframework_s *s_current_menu;
static int		s_current_menu_index;

static menulist_s		s_mode_list;
#if defined (__unix__)
static menulist_s		s_ref_list[2];
#else
static menulist_s  		s_finish_box;
#endif
static menuslider_s		s_brightness_slider;
static menulist_s  		s_fs_box;
static menulist_s  		s_texture_mode;
static menuslider_s		s_lightmap_saturation;
static menuslider_s		s_lightmap_texture_saturation;
static menuaction_s		s_apply_action;

#if defined (__unix__)
static void DriverCallback( void *unused )
{
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;
	s_current_menu = &s_opengl_menu;
	s_current_menu_index = 1;
}
#endif

static void BrightnessCallback( void *s )
{
#if 1
	float gamma = (0.8 - (s_brightness_slider.curvalue * 0.1 - 0.5)) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
#else
	Cvar_SetValue("vid_gamma", s_brightness_slider.curvalue * 0.1);
#endif
}

static void ApplyChanges( void *unused )
{
#if defined (__unix__) 
	int		ref;
	/*
	 * must use an if here (instead of a switch), since the REF_'s are
	 * now variables * and not #DEFINE's (constants)
	 */
	ref = s_ref_list[s_current_menu_index].curvalue;
	if (ref == REF_GLX) {
		Cvar_Set("vid_ref", "gl");
		Cvar_Get("gl_driver", GL_DRIVER_LIB, CVAR_ARCHIVE);
		if (gl_driver->modified)
			vid_ref->modified = true;
	}
	if (ref == REF_SDLGL) {
		Cvar_Set("vid_ref", "sdl");
		Cvar_Get("gl_driver", GL_DRIVER_LIB, CVAR_ARCHIVE);
		if (gl_driver->modified)
			vid_ref->modified = true;
	}

	/*
	** make values consistent
	*/
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;
#else
	Cvar_SetValue( "gl_finish", s_finish_box.curvalue );
#endif
	Cvar_SetValue( "vid_fullscreen", s_fs_box.curvalue );
	Cvar_SetValue( "gl_mode", s_mode_list.curvalue );
	
	if (s_texture_mode.curvalue == 0)
		Cvar_Set("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST"); //Bilinear
	else if (s_texture_mode.curvalue == 1)
		Cvar_Set("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR"); //Trilinear

	Cvar_SetValue("gl_lightmap_saturation", s_lightmap_saturation.curvalue * 0.1);
	Cvar_SetValue("gl_lightmap_texture_saturation", s_lightmap_texture_saturation.curvalue * 0.1);

	M_ForceMenuOff();
}

/* Knightmare */
int TextureMode(void)
{
	char *texmode = Cvar_VariableString("gl_texturemode");

	if (!Q_strcasecmp(texmode, "GL_LINEAR_MIPMAP_NEAREST"))
		return 0;
	else
		return 1;
}

static void LightMapSaturation(void *unused)
{
	Cvar_SetValue("gl_lightmap_saturation",
	              s_lightmap_saturation.curvalue / 10);
}

static void LightMapTextureSaturation(void *unused)
{
	Cvar_SetValue("gl_lightmap_texture_saturation",
	              s_lightmap_texture_saturation.curvalue / 10);
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	int	i, y;
#if defined (__unix__) 
	int	counter;
#endif

	static const char *resolutions[] = 
	{
		"320 x 240 [0]",
		"400 x 300 [1]",
		"512 x 384 [2]",
		"640 x 480 [3]",
		"800 x 600 [4]",
		"960 x 720 [5]",
		"1024 x 768 [6]",
		"1152 x 864 [7]",
		"1280 x 1024 [8]",
		"1600 x 1200 [9]",
		"2048 x 1536 [10]",
		"640 x 400 [11]",
		"800 x 500 [12]",
		"1024 x 480 [13]",
		"1024 x 640 [14]",
		"1152 x 768 [15]",
		"1152 x 854 [16]",
		"1280 x 800 [17]",
		"1280 x 854 [18]",
		"1280 x 960 [19]",
		"1680 x 1050 [20]",
		"1920 x 1200 [21]",
		NULL
	};
	static const char *mip_names[] =
	{
		"Bilinear ",
		"Trilinear",
		NULL
	};
	static const char *yesno_names[] =
	{
		"No",
		"Yes",
		NULL
	};

#if defined (__unix__) 
	/* make sure these are invalided before showing the menu again */
	REF_GLX = NUMBER_OF_REFS;
	REF_SDLGL = NUMBER_OF_REFS;


	GL_REF_START = NUMBER_OF_REFS;

	/* now test to see which ref's are present */
	i = counter = 0;
	while (i < NUMBER_OF_REFS) {
		if (VID_CheckRefExists(possible_refs[i].realname)) {
			*(possible_refs[i].pointer) = counter;

			/* free any previous string */
			if (refs[i])
				Q_free(refs[i]);
			refs[counter] = strdup(possible_refs[i].menuname);

			/*
			 * * if we reach the 3rd item in the list, this
			 * indicates that a * GL ref has been found; this
			 * will change if more software * modes are added to
			 * the possible_ref's array
			 */
			if (i == 3)
				GL_REF_START = counter;

			counter++;
		}
		i++;
	}
	refs[counter] = (char *)0;

	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", GL_DRIVER_LIB, CVAR_ARCHIVE );
#else
	if ( !gl_finish )
		gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
#endif
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "4", CVAR_ARCHIVE );
	if (!gl_texturemode)
		gl_texturemode = Cvar_Get("gl_texturemode", 
		                          "GL_LINEAR_MIPMAP_NEAREST",
					  CVAR_ARCHIVE);
					  
	if ( !gl_lightmap_saturation )
		gl_lightmap_saturation = Cvar_Get( "gl_lightmap_saturation", "1", CVAR_ARCHIVE );
		
	if ( !gl_lightmap_texture_saturation )
		gl_lightmap_texture_saturation = Cvar_Get( "gl_lightmap_texture_saturation", "1", CVAR_ARCHIVE );

	s_mode_list.curvalue = gl_mode->value;
	if (s_mode_list.curvalue < 0)
		s_mode_list.curvalue = 4;

#if defined (__unix__) 
	if ( strcmp( vid_ref->string, "sdl" ) == 0 ) {
		s_current_menu_index = OPENGL_MENU;
		s_ref_list[s_current_menu_index].curvalue = REF_SDLGL;
	}
	else if ( strcmp( vid_ref->string, "gl" ) == 0 ) {
		s_current_menu_index = OPENGL_MENU;
		s_ref_list[s_current_menu_index].curvalue = REF_GLX;
	}
#endif
	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.y = 0;
	s_opengl_menu.nitems = 0;

	for ( i = 0; i < 2; i++ ) {
		y = 0;
#if defined (__unix__) 
		s_ref_list[i].generic.type = MTYPE_SPINCONTROL;
		s_ref_list[i].generic.name = "VIDEO DRIVER";
		s_ref_list[i].generic.x = 0;
		s_ref_list[i].generic.y = y;
		s_ref_list[i].generic.callback = DriverCallback;
		s_ref_list[i].itemnames = (const char **)refs;
#endif
		s_mode_list.generic.type = MTYPE_SPINCONTROL;
		s_mode_list.generic.name = "RESOLUTION";
		s_mode_list.generic.x = 0;
		s_mode_list.generic.y = y += (MENU_FONT_SIZE+2);
		s_mode_list.itemnames = resolutions;
		s_mode_list.generic.statusbar =
		" ^1^sSet 'gl_mode' to -1 and use 'vid_width'/'vid_height' for custom resolutions.";

		s_fs_box.generic.type = MTYPE_SPINCONTROL;
		s_fs_box.generic.x	= 0;
		s_fs_box.generic.y	= y += (MENU_FONT_SIZE+2);
		s_fs_box.generic.name	= "FULLSCREEN";
		s_fs_box.itemnames = yesno_names;
		s_fs_box.curvalue = vid_fullscreen->value;

		s_brightness_slider.generic.type	= MTYPE_SLIDER;
		s_brightness_slider.generic.x	= 0;
		s_brightness_slider.generic.y	= y += 2*(MENU_FONT_SIZE+2);
		s_brightness_slider.generic.name	= "BRIGHTNESS";
		s_brightness_slider.generic.callback = BrightnessCallback;
#if 1
		s_brightness_slider.minvalue = 5;
		s_brightness_slider.maxvalue = 13;
		s_brightness_slider.curvalue = (1.3 - vid_gamma->value + 0.5) * 10;
#else
		s_brightness_slider.minvalue = 1;
		s_brightness_slider.maxvalue = 8;
		s_brightness_slider.curvalue = Cvar_VariableValue("vid_gamma") * 10;
#endif
		s_texture_mode.generic.type = MTYPE_SPINCONTROL;
		s_texture_mode.generic.x = 0;
		s_texture_mode.generic.y = y += (MENU_FONT_SIZE+2);
		s_texture_mode.generic.name = "TEXTURE MODE";
		s_texture_mode.curvalue = TextureMode();
		s_texture_mode.itemnames = mip_names;

		s_lightmap_saturation.generic.type = MTYPE_SLIDER;
		s_lightmap_saturation.generic.x = 0;
		s_lightmap_saturation.generic.y = y+=(MENU_FONT_SIZE+2);
		s_lightmap_saturation.generic.name = "LIGHT MAP SATURATION";
		s_lightmap_saturation.generic.callback = LightMapSaturation;
		s_lightmap_saturation.minvalue = 0;
		s_lightmap_saturation.maxvalue = 10;
		s_lightmap_saturation.curvalue = gl_lightmap_saturation->value * 10;
		s_lightmap_saturation.generic.statusbar = "^3Lightmap saturation";

		s_lightmap_texture_saturation.generic.type = MTYPE_SLIDER;
		s_lightmap_texture_saturation.generic.x = 0;
		s_lightmap_texture_saturation.generic.y = y+=(MENU_FONT_SIZE+2);
		s_lightmap_texture_saturation.generic.name = "LIGHT MAP TEXTURE SATURATION";
		s_lightmap_texture_saturation.generic.callback = LightMapTextureSaturation;
		s_lightmap_texture_saturation.minvalue = 0;
		s_lightmap_texture_saturation.maxvalue = 10;
		s_lightmap_texture_saturation.curvalue = gl_lightmap_texture_saturation->value * 10;
		s_lightmap_texture_saturation.generic.statusbar =
		"^3Lightmap texture saturation, requires vid_restart.";

#if defined _WIN32
		s_finish_box.generic.type = MTYPE_SPINCONTROL;
		s_finish_box.generic.x	= 0;
		s_finish_box.generic.y	= y += (MENU_FONT_SIZE+2);
		s_finish_box.generic.name	= "SYNC EVERY FRAME";
		s_finish_box.curvalue = gl_finish->value;
		s_finish_box.itemnames = yesno_names;
#endif

		s_apply_action.generic.type = MTYPE_ACTION;
		s_apply_action.generic.name = "APPLY";
		s_apply_action.generic.x    = 0;
		s_apply_action.generic.y    = y += 3*(MENU_FONT_SIZE+2);
		s_apply_action.generic.callback = ApplyChanges;
	}

#if defined (__unix__) 
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list[OPENGL_MENU]);
#endif
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_texture_mode);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_lightmap_saturation);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_lightmap_texture_saturation);
#if defined _WIN32
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_finish_box );
#endif
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_apply_action);

	Menu_Center( &s_opengl_menu );
	s_opengl_menu.x -= MENU_FONT_SIZE;
}

#if defined (__unix__) 
/*
 * ================ VID_MenuShutdown ================
 */
void VID_MenuShutdown(void)
{
	int		i;

	for (i = 0; i < NUMBER_OF_REFS; i++) {
		if (refs[i])
			Q_free(refs[i]);
	}
}
#endif

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{

	s_current_menu = &s_opengl_menu;

	/*
	** draw the banner
	*/
	M_Banner("m_banner_video");

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor( s_current_menu, 1 );

	/*
	** draw the menu
	*/
	Menu_Draw( s_current_menu );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey( int key )
{

	menuframework_s *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	switch ( key ) {
		case K_ESCAPE:
			M_PopMenu();
			return NULL;
		case K_UPARROW:
			m->cursor--;
			Menu_AdjustCursor( m, -1 );
			break;
		case K_DOWNARROW:
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			break;
		case K_LEFTARROW:
			Menu_SlideItem( m, -1 );
			break;
		case K_RIGHTARROW:
			Menu_SlideItem( m, 1 );
			break;
		case K_ENTER:
			if ( !Menu_SelectItem( m ) )
				ApplyChanges( NULL );
			break;
	}

	return sound;
}



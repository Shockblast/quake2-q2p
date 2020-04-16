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
// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#include <assert.h>
#include <dlfcn.h> // ELF dl loader
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "../client/client.h"

#include "../unix/rw_unix.h"

// Structure containing functions exported from refresh DLL
refexport_t	re;

// Console variables that we need to access from this module
cvar_t		*vid_gamma;
cvar_t		*vid_ref;			// Name of Refresh DLL loaded
cvar_t		*vid_fullscreen;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
void		*reflib_library;		// Handle to refresh DLL 
qboolean	reflib_active = 0;


/** KEYBOARD **************************************************************/

void Do_Key_Event(int key, qboolean down);

void (*KBD_Update_fp)(void);
void (*KBD_Init_fp)(Key_Event_fp_t fp);
void (*KBD_Close_fp)(void);

/** MOUSE *****************************************************************/

in_state_t in_state;

void (*RW_IN_Init_fp)(in_state_t *in_state_p);
void (*RW_IN_Shutdown_fp)(void);
void (*RW_IN_Activate_fp)(qboolean active);
void (*RW_IN_Commands_fp)(void);
void (*RW_IN_Move_fp)(usercmd_t *cmd, int mcoords[]);
void (*RW_IN_Frame_fp)(void);

void Real_IN_Init (void);

char *(*RW_Sys_GetClipboardData_fp)(void);
extern void	VID_MenuShutdown(void);

cvar_t   *vid_width, *vid_height;

/*
==========================================================================

DLL GLUE

==========================================================================
*/

#define	MAXPRINTMSG	4096
void VID_Printf (int print_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	if (print_level == PRINT_ALL)
		Com_Printf ("%s", msg);
	else
		Com_DPrintf ("%s", msg);
}

void VID_Error (int err_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	Com_Error (err_level,"%s", msg);
}

//==========================================================================

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f (void)
{
	vid_ref->modified = true;
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int         width, height;
	int         mode;
} vidmode_t;

vidmode_t vid_modes[] =
{
	{ "Mode 0:  320x240",    320,  240,   0  },
	{ "Mode 1:  400x300",    400,  300,   1  },
	{ "Mode 2:  512x384",    512,  384,   2  },
	{ "Mode 3:  640x480",    640,  480,   3  },
	{ "Mode 4:  800x600",    800,  600,   4  },
	{ "Mode 5:  960x720",    960,  720,   5  },
	{ "Mode 6:  1024x768",   1024, 768,   6  },
	{ "Mode 7:  1152x864",   1152, 864,   7  },
	{ "Mode 8:  1280x1024",  1280, 1024,  8  },
	{ "Mode 9:  1600x1200",  1600, 1200,  9  },
	{ "Mode 10: 2048x1536",  2048, 1536,  10 },
	{ "Mode 11: 640x400",    640,  400,   11 }, 
	{ "Mode 12: 800x500",    800,  500,   12 },
	{ "Mode 13: 1024x480",   1024, 480,   13 },
	{ "Mode 14: 1024x640",   1024, 640,   14 },
	{ "Mode 15: 1152x768",   1152, 768,   15 }, 
	{ "Mode 16: 1152x864",   1152, 864,   16 },
	{ "Mode 17: 1280x800",   1280, 800,   17 },
	{ "Mode 18: 1280x854",   1280, 854,   18 },
	{ "Mode 19: 1280x960",   1280, 960,   19 },
	{ "Mode 20: 1680x1050",  1680, 1050,  20 },
	{ "Mode 21: 1920x1200",  1920, 1200,  21 }
};

static int	s_numVidModes = (sizeof(vid_modes) / sizeof(vid_modes[0]));

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
	vidmode_t	*vm;
	
	if (mode < -1){
		return false;
	}
	
	if (mode >= s_numVidModes){
		return false;
	}
	
	if(mode == -1) {
		*width  = vid_width->value;
		*height = vid_height->value;
		return true;
	}

	vm = &vid_modes[mode];
	
	*width  = vm->width;
	*height = vm->height;
	
	return true;
}

void VID_ModeList_f(void)
{
	int i;

	for( i=0; i < s_numVidModes; i++) {
		Com_Printf("%s\n",vid_modes[i].description);
	}
	Com_Printf("For custom resolutions, set 'gl_mode' to -1 and use 'vid_width' / 'vid_height'\n");
}

/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;
}

void VID_FreeReflib (void)
{
	if (reflib_library) {
		if (KBD_Close_fp)
			KBD_Close_fp();
		if (RW_IN_Shutdown_fp)
			RW_IN_Shutdown_fp();
		dlclose(reflib_library);
	}

	KBD_Init_fp = NULL;
	KBD_Update_fp = NULL;
	KBD_Close_fp = NULL;
	RW_IN_Init_fp = NULL;
	RW_IN_Shutdown_fp = NULL;
	RW_IN_Activate_fp = NULL;
	RW_IN_Commands_fp = NULL;
	RW_IN_Move_fp = NULL;
	RW_IN_Frame_fp = NULL;
	RW_Sys_GetClipboardData_fp = NULL;

	memset (&re, 0, sizeof(re));
	reflib_library = NULL;
	reflib_active  = false;
}

/*
==============
VID_LoadRefresh
==============
*/
qboolean IsVisible(vec3_t org1,vec3_t org2);
qboolean VID_LoadRefresh( char *name )
{
	refimport_t	ri;
	GetRefAPI_t	GetRefAPI;
	char	fn[MAX_OSPATH];
	struct stat st;
	char 	*path;
	extern uid_t	saved_euid;
	
	if ( reflib_active )
	{
		if (KBD_Close_fp)
			KBD_Close_fp();
		if (RW_IN_Shutdown_fp)
			RW_IN_Shutdown_fp();
		KBD_Close_fp = NULL;
		RW_IN_Shutdown_fp = NULL;
		re.Shutdown();
		VID_FreeReflib ();
	}

	Com_Printf("\nVideo Initialization.\n");
	
	Com_Printf( "Loading %s\n", name );
	
	/* regain root */
	seteuid(saved_euid);

#ifdef LIBDIR
	path = LIBDIR;
#elif defined(DATADIR)
	path = Cvar_Get("basedir", DATADIR, CVAR_NOSET)->string;
#else
	path = Cvar_Get("basedir", ".", CVAR_NOSET)->string;
#endif
	Com_sprintf(fn, sizeof(fn), "%s/%s", path, name);

	if (stat(fn, &st) == -1) {
			Com_Printf( "LoadLibrary(\"%s\") failed: %s\n", name, strerror(errno));
			return false;
	}

	/* permission checking */
	if (strstr(fn, "gl") == NULL &&
	    strstr(fn, "sdl") == NULL) {
	} else {
		setreuid(getuid(), getuid());
		setegid(getgid());
	}

	if ( ( reflib_library = dlopen( fn, RTLD_LAZY) ) == 0 )
	{
		Com_Printf( "LoadLibrary(\"%s\") failed: %s\n", name , dlerror());
		return false;
	}
	
	//Com_Printf( "LoadLibrary(\"%s\")\n", fn );

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Con_Printf = VID_Printf;
	ri.Sys_Error = VID_Error;
	ri.FS_LoadFile = FS_LoadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_Gamedir = FS_Gamedir;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Vid_GetModeInfo = VID_GetModeInfo;
	ri.Vid_MenuInit = VID_MenuInit;
	ri.Vid_NewWindow = VID_NewWindow;
	ri.IsVisible = IsVisible;
	ri.TextColor = TextColor; // Knightmare added
	ri.FS_ListPak = FS_ListPak;
	ri.FS_Mapname = FS_Mapname; // -Maniac
#if defined(PARTICLESYSTEM)
	ri.SetParticleImages = SetParticleImages;
#endif
	
	if ( ( GetRefAPI = (void *) dlsym( reflib_library, "GetRefAPI" ) ) == 0 )
		Com_Error( ERR_FATAL, "dlsym failed on %s", name );

	re = GetRefAPI( ri );

	if (re.api_version != API_VERSION)
	{
		VID_FreeReflib ();
		Com_Error (ERR_FATAL, "%s has incompatible api_version", name);
	}

	/* Init IN (Mouse) */
	in_state.IN_CenterView_fp = IN_CenterView;
	in_state.Key_Event_fp = Do_Key_Event;
	in_state.viewangles = cl.viewangles;
	in_state.in_strafe_state = &in_strafe.state;

	if ((RW_IN_Init_fp = dlsym(reflib_library, "RW_IN_Init")) == NULL ||
		(RW_IN_Shutdown_fp = dlsym(reflib_library, "RW_IN_Shutdown")) == NULL ||
		(RW_IN_Activate_fp = dlsym(reflib_library, "RW_IN_Activate")) == NULL ||
		(RW_IN_Commands_fp = dlsym(reflib_library, "RW_IN_Commands")) == NULL ||
		(RW_IN_Move_fp = dlsym(reflib_library, "RW_IN_Move")) == NULL ||
		(RW_IN_Frame_fp = dlsym(reflib_library, "RW_IN_Frame")) == NULL)
		Sys_Error("No RW_IN functions in REF.\n");
		
	RW_Sys_GetClipboardData_fp = dlsym(reflib_library, "RW_Sys_GetClipboardData");

	Real_IN_Init();

	if ( re.Init( 0, 0 ) == -1 )
	{
		re.Shutdown();
		VID_FreeReflib ();
		return false;
	}

	/* Init KBD */
	if ((KBD_Init_fp = dlsym(reflib_library, "KBD_Init")) == NULL ||
		(KBD_Update_fp = dlsym(reflib_library, "KBD_Update")) == NULL ||
		(KBD_Close_fp = dlsym(reflib_library, "KBD_Close")) == NULL)
		Sys_Error("No KBD functions in REF.\n");
		
	KBD_Init_fp(Do_Key_Event);

	// give up root now
	setreuid(getuid(), getuid());
	setegid(getgid());

	Com_Printf( "------------------------------------\n");
	reflib_active = true;
	return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	char name[100];
	cvar_t *gl_mode;

	if ( vid_ref->modified ) {
		S_StopAllSounds();
	}

	while (vid_ref->modified) {
		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		Com_sprintf(name, sizeof(name), "vid_%s.so", vid_ref->string);
		if ( !VID_LoadRefresh( name ) ) {
			if ( strcmp (vid_ref->string, "gl") == 0 ||
			     strcmp (vid_ref->string, "sdl") == 0 ) {
				Com_Printf("Refresh failed\n");
				gl_mode = Cvar_Get( "gl_mode", "4", CVAR_ARCHIVE );
				
				if (gl_mode->value != 0) {
					Com_Printf("Trying mode 0\n");
					Cvar_SetValue("gl_mode", 0);
					if ( !VID_LoadRefresh( name ) )
						Com_Error (ERR_FATAL, "Couldn't load OpenGL refresh!");
				} else
					Com_Error (ERR_FATAL, "Couldn't load OpenGL refresh!");
			}

			if (getenv("DISPLAY"))
				Cvar_Set( "vid_ref", "gl" );
			else
				Cvar_Set( "vid_ref", "sdl" );
			/*
			** drop the console if we fail to load a refresh
			*/
			if ( cls.key_dest != key_console ) {
				Con_ToggleConsole_f();
			}
		}
		cls.disable_screen = false;
	}

}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	// if DISPLAY is defined, try X
	if (getenv("DISPLAY"))
		vid_ref = Cvar_Get ("vid_ref", "gl", CVAR_ARCHIVE);
	else
		vid_ref = Cvar_Get ("vid_ref", "sdl", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "1", CVAR_ARCHIVE);
	vid_gamma = Cvar_Get( "vid_gamma", "0.8", CVAR_ARCHIVE );
	vid_width= Cvar_Get ( "vid_width", "800", CVAR_ARCHIVE);
	vid_height= Cvar_Get ( "vid_height", "600", CVAR_ARCHIVE);

	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);
	Cmd_AddCommand ("vid_modelist", VID_ModeList_f);
		
	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( reflib_active )
	{
		if (KBD_Close_fp)
			KBD_Close_fp();
		if (RW_IN_Shutdown_fp)
			RW_IN_Shutdown_fp();
		KBD_Close_fp = NULL;
		RW_IN_Shutdown_fp = NULL;
		re.Shutdown ();
		VID_FreeReflib ();
	}
	VID_MenuShutdown();
}


/*****************************************************************************/
/* INPUT                                                                     */
/*****************************************************************************/
//cvar_t	*in_joystick;

extern cursor_t	cursor;

void		refreshCursorMenu(void);
void		refreshCursorLink(void);
void		M_Think_MouseCursor(void);

// This is fake, it's acutally done by the Refresh load
void IN_Init (void)
{
//	in_joystick	= Cvar_Get ("in_joystick", "0", 0);
}

void Real_IN_Init (void)
{
	if (RW_IN_Init_fp)
		RW_IN_Init_fp(&in_state);
		
	refreshCursorMenu();
	refreshCursorLink();
	cursor.mouseaction = false;
}

void IN_Shutdown (void)
{
	if (RW_IN_Shutdown_fp)
		RW_IN_Shutdown_fp();
}

void IN_Commands (void)
{
	if (RW_IN_Commands_fp)
		RW_IN_Commands_fp();
}

void IN_Move (usercmd_t *cmd)
{
	int		mcoords[2];
	
	if (RW_IN_Move_fp)
		RW_IN_Move_fp(cmd, mcoords);

	/* now to set the menu cursor */
	if (cls.key_dest == key_menu) {
		cursor.oldx = cursor.x;
		cursor.oldy = cursor.y;

		cursor.x += mcoords[0] * (cl_mouse_sensitivity->value);
		cursor.y += mcoords[1] * (cl_mouse_sensitivity->value);

		if (cursor.x != cursor.oldx || cursor.y != cursor.oldy)
			cursor.mouseaction = true;

		if (cursor.x < 0)
			cursor.x = 0;
		if (cursor.x > viddef.width)
			cursor.x = viddef.width;
		if (cursor.y < 0)
			cursor.y = 0;
		if (cursor.y > viddef.height)
			cursor.y = viddef.height;

		if ( cls.key_dest != key_console ) 
		{
			M_Think_MouseCursor();
		} else {
			cursor.oldx = 0;
			cursor.oldy = 0;
		}
	}
}

void IN_Frame (void)
{
	if (RW_IN_Activate_fp) {
		if ( !cl.refresh_prepped && cls.key_dest == key_menu )
			RW_IN_Activate_fp(true);
		else if ( !cl.refresh_prepped  || cls.key_dest == key_console )
			RW_IN_Activate_fp(false);
		else
			RW_IN_Activate_fp(true);
	}

	if (RW_IN_Frame_fp)
		RW_IN_Frame_fp();
}

void IN_Activate (qboolean active)
{
}

void Do_Key_Event(int key, qboolean down)
{
	int		time = Sys_Milliseconds();
	
	Key_Event(key, down, Sys_Milliseconds());
	/* set menu cursor buttons */
	if (cls.key_dest == key_menu
	    && key >= K_MOUSE1 && key <= K_MOUSE3) {
		int		multiclicktime = 750;
		int		i = key - K_MOUSE1;

		if (down) {	/* mouse press down */
			if (time - cursor.buttontime[i] < multiclicktime)
				cursor.buttonclicks[i] += 1;
			else
				cursor.buttonclicks[i] = 1;

			if (cursor.buttonclicks[i] > 3)
				cursor.buttonclicks[i] = 3;

			cursor.buttontime[i] = time;

			cursor.buttondown[i] = true;
			cursor.buttonused[i] = false;
			cursor.mouseaction = true;
		} else {	/* mouse let go */
			cursor.buttondown[i] = false;
			cursor.buttonused[i] = false;
			cursor.mouseaction = true;
		}
	}
}

char *Sys_GetClipboardData(void)
{
	if (RW_Sys_GetClipboardData_fp)
		return RW_Sys_GetClipboardData_fp();
	else
		return NULL;
}



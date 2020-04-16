/* $Id: rw_sdl.c,v 1.17 2003/02/23 12:42:34 jaq Exp $
 *
 * all os-specific SDL refresher code
 *
 * Copyright (c) 2002 The QuakeForge Project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
** GL_SDL.C
**
** This file contains ALL Unix specific stuff having to do with the
** software refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_InitGraphics
** GLimp_SetPalette
** GLimp_Shutdown
** GLimp_SwitchFullscreen
*/

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#include "SDL.h"
#include "SDL_syswm.h"
#include <X11/Xlib.h>

#include "../ref_gl/gl_local.h"
#include "../client/keys.h"
#include "../unix/rw_unix.h"
#include "../ref_gl/glw.h"
                                                                 
/*****************************************************************************/

int	mouse_buttonstate,
	mouse_oldbuttonstate,
	mouse_x,
	mouse_y,
	old_mouse_x, 
	old_mouse_y,
	mx, 
	my;
		
cvar_t	*_windowed_mouse,
	*m_filter,
	*_windowed_mouse,
	*print_keymap,
	*sensitivity,
	*autosensitivity,
	*lookstrafe,
	*m_side,
	*m_yaw,
	*m_pitch,
	*m_forward,
	*freelook,
	*use_stencil;

float	old_windowed_mouse;

qboolean SDL_active = false,
         mlooking = false,
	 mouse_avail = false,
	 mouse_active = false,
	 have_stencil = false;


glwstate_t	glw_state;
in_state_t	*in_state;

SDL_Surface 	*surface;

unsigned char KeyStates[SDLK_LAST];
unsigned short gamma_ramp[3][256];

struct {
	int key, down;
} keyq[64];

int	keyq_head = 0,
        keyq_tail = 0;

void install_grabs(void)
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(0);
}

void uninstall_grabs(void)
{
	SDL_ShowCursor(1);
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void Force_CenterView_f(void)
{
	in_state->viewangles[PITCH] = 0;
}

void RW_IN_MLookDown(void) 
{ 
	mlooking = true; 
}

void RW_IN_MLookUp(void) 
{
	mlooking = false;
	in_state->IN_CenterView_fp ();
}

void RW_IN_Init(in_state_t * in_state_p)
{

	in_state = in_state_p;

	// mouse variables
	m_filter = ri.Cvar_Get ("m_filter", "0", 0);
	_windowed_mouse = ri.Cvar_Get ("_windowed_mouse", "1", CVAR_ARCHIVE);
	freelook = ri.Cvar_Get( "freelook", "1", CVAR_ARCHIVE );
	lookstrafe = ri.Cvar_Get ("lookstrafe", "0", 0);
	sensitivity = ri.Cvar_Get ("sensitivity", "3", CVAR_ARCHIVE);
	autosensitivity = ri.Cvar_Get("autosensitivity", "1", 0);
	m_pitch = ri.Cvar_Get ("m_pitch", "0.022", 0);
	m_yaw = ri.Cvar_Get ("m_yaw", "0.022", 0);
	m_forward = ri.Cvar_Get ("m_forward", "1", 0);
	m_side = ri.Cvar_Get ("m_side", "0.8", 0);
	print_keymap = ri.Cvar_Get ("print_keymap", "0", 0);
	use_stencil = ri.Cvar_Get("use_stencil", "1", CVAR_ARCHIVE);
	
	ri.Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
	ri.Cmd_AddCommand ("-mlook", RW_IN_MLookUp);
	ri.Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	mx = my = 0.0;
	
	#if defined(NDEBUG)
	mouse_avail = true;
	#else
	{
		Com_Printf("Fullscreen and mouse grab not allowed when running debug mode\n");
		ri.Cvar_Set("vid_fullscreen", "0");
		mouse_avail = false;
	}
	#endif
}

void RW_IN_Shutdown(void)
{
	if (mouse_avail) {
		mouse_avail = false;
		ri.Cmd_RemoveCommand("+mlook");
		ri.Cmd_RemoveCommand("-mlook");
		ri.Cmd_RemoveCommand("force_centerview");
	}
}

void RW_IN_Commands(void)
{
	int	i;

	if (mouse_avail) {
		for (i = 0; i < 5; i++) {
			if ((mouse_buttonstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
				in_state->Key_Event_fp(K_MOUSE1 + i, true);
			if (!(mouse_buttonstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
				in_state->Key_Event_fp(K_MOUSE1 + i, false);
		}
		mouse_oldbuttonstate = mouse_buttonstate;
	}
}

void RW_IN_Move(usercmd_t * cmd, int *mcoords)
{
	if (!mouse_avail)
		return;
   
	if (m_filter->value)
	{
		mx = (mx + old_mouse_x) * 0.5;
		my = (my + old_mouse_y) * 0.5;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

		/* raw coords for menu mouse */
		mcoords[0] = mx;
		mcoords[1] = my;

		if (autosensitivity->value) {
			mx *= sensitivity->value * (r_newrefdef.fov_x / 90.0);
			my *= sensitivity->value * (r_newrefdef.fov_y / 90.0);
		} else {
			mx *= sensitivity->value;
			my *= sensitivity->value;
		}

	// add mouse X/Y movement to cmd
	if ( (*in_state->in_strafe_state & 1) || 
		(lookstrafe->value && mlooking ))
		cmd->sidemove += m_side->value * mx;
	else
		in_state->viewangles[YAW] -= m_yaw->value * mx;

	if ( (mlooking || freelook->value) && 
		!(*in_state->in_strafe_state & 1))
	{
		in_state->viewangles[PITCH] += m_pitch->value * my;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * my;
	}

	mx = my = 0;
}

void IN_DeactivateMouse(void) 
{ 
	if (!mouse_avail)
		return;

	if (mouse_active) {
		uninstall_grabs();
		mouse_active = false;
	}
}

void IN_ActivateMouse(void) 
{
	if (!mouse_avail)
		return;

	if (!mouse_active) {
		mx = my = 0; // don't spazz
		install_grabs();
		mouse_active = true;
	}
}

void RW_IN_Activate(qboolean active)
{
	if (active || SDL_active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}

void RW_IN_Frame (void) {}

/*****************************************************************************/

int XLateKey(unsigned int keysym)
{
	int key;
	
	key = 0;
	switch(keysym) {
		case SDLK_KP9:		key = K_KP_PGUP;       break;
		case SDLK_PAGEUP:	key = K_PGUP;          break;
		case SDLK_KP3:		key = K_KP_PGDN;       break;
		case SDLK_PAGEDOWN:	key = K_PGDN;          break;
		case SDLK_KP7:		key = K_KP_HOME;       break;
		case SDLK_HOME:		key = K_HOME;          break;
		case SDLK_KP1:		key = K_KP_END;        break;
		case SDLK_END:		key = K_END;           break;
		case SDLK_KP4:		key = K_KP_LEFTARROW;  break;
		case SDLK_LEFT:		key = K_LEFTARROW;     break;
		case SDLK_KP6:		key = K_KP_RIGHTARROW; break;
		case SDLK_RIGHT:	key = K_RIGHTARROW;    break;
		case SDLK_KP2:		key = K_KP_DOWNARROW;  break;
		case SDLK_DOWN:		key = K_DOWNARROW;     break;
		case SDLK_KP8:		key = K_KP_UPARROW;    break;
		case SDLK_UP:		key = K_UPARROW;       break;
		case SDLK_ESCAPE:	key = K_ESCAPE;        break;
		case SDLK_KP_ENTER:	key = K_KP_ENTER;      break;
		case SDLK_RETURN:	key = K_ENTER;         break;
		case SDLK_TAB:		key = K_TAB;           break;
		case SDLK_F1:		key = K_F1;            break;
		case SDLK_F2:		key = K_F2;            break;
		case SDLK_F3:		key = K_F3;            break;
		case SDLK_F4:		key = K_F4;            break;
		case SDLK_F5:		key = K_F5;            break;
		case SDLK_F6:		key = K_F6;            break;
		case SDLK_F7:		key = K_F7;            break;
		case SDLK_F8:		key = K_F8;            break;
		case SDLK_F9:		key = K_F9;            break;
		case SDLK_F10:		key = K_F10;           break;
		case SDLK_F11:		key = K_F11;           break;
		case SDLK_F12:		key = K_F12;           break;
		case SDLK_BACKSPACE:	key = K_BACKSPACE;     break;
		case SDLK_KP_PERIOD:	key = K_KP_DEL;        break;
		case SDLK_DELETE:	key = K_DEL;           break;
		case SDLK_PAUSE:	key = K_PAUSE;         break;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:	key = K_SHIFT;         break;
		case SDLK_LCTRL:
		case SDLK_RCTRL:	key = K_CTRL;          break;
		case SDLK_LMETA:
		case SDLK_RMETA:
		case SDLK_LALT:
		case SDLK_RALT:		key = K_ALT;           break;
		case SDLK_KP5:		key = K_KP_5;          break;
		case SDLK_INSERT:	key = K_INS;           break;
		case SDLK_KP0:		key = K_KP_INS;        break;
		case SDLK_KP_MULTIPLY:	key = '*';             break;
		case SDLK_KP_PLUS:	key = K_KP_PLUS;       break;
		case SDLK_KP_MINUS:	key = K_KP_MINUS;      break;
		case SDLK_KP_DIVIDE:	key = K_KP_SLASH;      break;
		case SDLK_WORLD_7:	key = '`';             break;
		
		case 92:		key = '~';	       break; /* Italy */
		case 94:		key = '~';	       break; /* Deutschland */
		case 178:		key = '~'; 	       break; /* France */
		case 186:		key = '~'; 	       break; /* Spain */
		
		default: /* assuming that the other sdl keys are mapped to ascii */ 
			if (keysym < 256)
				key = keysym;
			break;
	}
        
	if (print_keymap->value)
		printf( "Key '%c' (%d) -> '%c' (%d)\n", keysym, keysym, key, key );

	return key;
}

void HandleEvents(SDL_Event * event)
{
	unsigned int	key;

	switch (event->type) {
	case SDL_MOUSEBUTTONDOWN:
		if (event->button.button == 4) {
			keyq[keyq_head].key = K_MWHEELUP;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
			keyq[keyq_head].key = K_MWHEELUP;
			keyq[keyq_head].down = false;
			keyq_head = (keyq_head + 1) & 63;
		} else if (event->button.button == 5) {
			keyq[keyq_head].key = K_MWHEELDOWN;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
			keyq[keyq_head].key = K_MWHEELDOWN;
			keyq[keyq_head].down = false;
			keyq_head = (keyq_head + 1) & 63;
		}
		break;
		
	case SDL_MOUSEBUTTONUP:
		break;
	case SDL_KEYDOWN:
		if ((KeyStates[SDLK_LALT] || KeyStates[SDLK_RALT]) &&
		    (event->key.keysym.sym == SDLK_RETURN)) {
			cvar_t         *fullscreen;

			SDL_WM_ToggleFullScreen(surface);

			if (surface->flags & SDL_FULLSCREEN) 
				ri.Cvar_SetValue("vid_fullscreen", 1);
			else 
				ri.Cvar_SetValue("vid_fullscreen", 0);

			fullscreen = ri.Cvar_Get("vid_fullscreen", "0", 0);
			fullscreen->modified = false;	/* we just changed it with SDL. */
			break;	/* ignore this key */
		}
		if ((KeyStates[SDLK_LCTRL] || KeyStates[SDLK_RCTRL]) &&
		    (event->key.keysym.sym == SDLK_g)) {
			SDL_GrabMode	gm = SDL_WM_GrabInput(SDL_GRAB_QUERY);
			ri.Cvar_SetValue("_windowed_mouse", (gm == SDL_GRAB_ON) ? 0 : 1);
			break;	/* ignore this key */
		}
		
		KeyStates[event->key.keysym.sym] = 1;

		key = XLateKey(event->key.keysym.sym);
		if (key) {
			keyq[keyq_head].key = key;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
		}
		break;
		
	case SDL_KEYUP:
		if (KeyStates[event->key.keysym.sym]) {
			KeyStates[event->key.keysym.sym] = 0;

			key = XLateKey(event->key.keysym.sym);
			if (key) {
				keyq[keyq_head].key = key;
				keyq[keyq_head].down = false;
				keyq_head = (keyq_head + 1) & 63;
			}
		}
		break;
		
	case SDL_QUIT:
		ri.Cmd_ExecuteText(EXEC_NOW, "quit");
		break;
	}

}

Key_Event_fp_t	Key_Event_fp;
void KBD_Init(Key_Event_fp_t fp) { Key_Event_fp = fp; }

void KBD_Update(void)
{
	SDL_Event	event;
	static int	KBD_Update_Flag;

	if (KBD_Update_Flag == 1)
		return;

	KBD_Update_Flag = 1;

	/* get events from x server */
	if (SDL_active) {
		int		bstate;

		while (SDL_PollEvent(&event))
			HandleEvents(&event);

		if (!mx && !my)
			SDL_GetRelativeMouseState(&mx, &my);
		mouse_buttonstate = 0;
		bstate = SDL_GetMouseState(NULL, NULL);
		if (SDL_BUTTON(1) & bstate)
			mouse_buttonstate |= (1 << 0);
		if (SDL_BUTTON(3) & bstate)	/* quake2 has the right button be mouse2 */
			mouse_buttonstate |= (1 << 1);
		if (SDL_BUTTON(2) & bstate)	/* quake2 has the middle button be mouse3 */
			mouse_buttonstate |= (1 << 2);
		if (SDL_BUTTON(6) & bstate)
			mouse_buttonstate |= (1 << 3);
		if (SDL_BUTTON(7) & bstate)
			mouse_buttonstate |= (1 << 4);


		if (old_windowed_mouse != _windowed_mouse->value) {
			old_windowed_mouse = _windowed_mouse->value;

			if (!_windowed_mouse->value) {
				/* ungrab the pointer */
				SDL_WM_GrabInput(SDL_GRAB_OFF);
			} else {
				/* grab the pointer */
				SDL_WM_GrabInput(SDL_GRAB_ON);
			}
		}
		while (keyq_head != keyq_tail) {
			in_state->Key_Event_fp(keyq[keyq_tail].key, keyq[keyq_tail].down);
			keyq_tail = (keyq_tail + 1) & 63;
		}
	}
	KBD_Update_Flag = 0;
}

void KBD_Close(void)
{
	keyq_head = 0;
	keyq_tail = 0;

	memset(keyq, 0, sizeof(keyq));
}

char *RW_Sys_GetClipboardData(void)
{
	Window		sowner;
	Atom		type, property;
	unsigned long	len, bytes_left, tmp;
	unsigned char  *data;
	int		format, result;
	char           *ret = NULL;
	SDL_SysWMinfo	info;

	SDL_VERSION(&info.version);
	
	if (SDL_GetWMInfo(&info)) {
		if (info.subsystem == SDL_SYSWM_X11) {
			Display *SDL_Display = info.info.x11.display;
			Window SDL_Window = info.info.x11.window;

			/* Enable the special window hook events */
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

			sowner = XGetSelectionOwner(SDL_Display, XA_PRIMARY);
			if ((sowner == None) || (sowner == SDL_Window)) {
				sowner = DefaultRootWindow(SDL_Display);
				property = XA_CUT_BUFFER0;
			}
			else {
				int selection_response = 0;
				SDL_Event event;

				sowner = SDL_Window;
				property = XInternAtom(SDL_Display, "SDL_SELECTION", False);
				XConvertSelection(SDL_Display, XA_PRIMARY, XA_STRING,
						property, sowner, CurrentTime);
				while (!selection_response) {
					SDL_WaitEvent(&event);
					if (event.type == SDL_SYSWMEVENT) {
						XEvent xevent = event.syswm.msg->event.xevent;

						if ((xevent.type == SelectionNotify) &&
						    (xevent.xselection.requestor == sowner))
							selection_response = 1;
					}
				}
			}
			
			XFlush(SDL_Display);
			
			XGetWindowProperty(SDL_Display, sowner, property,
				           0, 0, False, XA_STRING,
				           &type, &format, &len,
				           &bytes_left, &data);
			
			if (bytes_left > 0) {
				result = XGetWindowProperty(SDL_Display, sowner, property, 
				                            0, INT_MAX/4, False, XA_STRING,
							    &type, &format, &len, 
							    &tmp, &data);
					if (result == Success) {
						ret = strdup((char *)data);
					}
					XFree(data);
			}
			
			SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
		}

	}
	return ret;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the implementation
** specific stuff in a software rendering subsystem.
*/
void SDL_Version(void)
{
        const SDL_version *version;
        version = SDL_Linked_Version();
        
        Com_Printf( "SDLGL linked against SDL version %d.%d.%d\n" 
                    "Using SDL library version %d.%d.%d\n",
                    SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
                    version->major, version->minor, version->patch );
}

int GLimp_Init(void *hinstance, void *wndproc)
{
/*
	char	buffer[MAX_QPATH];
*/
	if (SDL_WasInit(SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_VIDEO) == 0) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			Sys_Error("SDL Init failed: %s\n", SDL_GetError());
			return false;
		}
	} else if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
			Sys_Error("SDL Init failed: %s\n", SDL_GetError());
			return false;
		}
	}
	SDL_Version();
/*
	if (SDL_VideoDriverName(buffer, sizeof(buffer)) != NULL)
		Com_Printf("Using SDL video driver: %s\n", buffer) ;
*/ 
	return true;
}

void UpdateHardwareGamma(void)
{
	float g = (1.3 - vid_gamma->value + 1);
	g = (g > 1 ? g : 1);
	
	{
		float v, i_f;
		int i, o;

		for (o = 0; o < 3; o++) {
			for(i = 0; i < 256; i++) {
				i_f = (float)i/255.0f;
				v = pow(i_f, vid_gamma->value);
				v += 0.0 * (1.0f - v);

				if (v < 0.0f) v = 0.0f;
				else
				if (v > 1.0f) v = 1.0f;
				gamma_ramp[o][i] = (v * 65535.0f + 0.5f);
			}
		}
	}	
	SDL_SetGamma(g, g, g);
}

void  SetSDLGamma(void)
{
	gl_state.hwgamma = true;
	vid_gamma->modified = true;
	Com_Printf( "Using hardware gamma\n");
}

qboolean GLimp_InitGraphics(qboolean fullscreen)
{
	int	flags,
		red_bits, 
		blue_bits, 
		green_bits,
		depth_bits,
		color_bits, 
		alpha_bits,
		stencil_bits,
		multisamples;

	/* Just toggle fullscreen if that's all that has been changed */
	if (surface && (surface->w == vid.width) && (surface->h == vid.height)) {
		int		isfullscreen = (surface->flags & SDL_FULLSCREEN) ? 1 : 0;

		if (fullscreen != isfullscreen)
			SDL_WM_ToggleFullScreen(surface);

		isfullscreen = (surface->flags & SDL_FULLSCREEN) ? 1 : 0;
		if (fullscreen == isfullscreen)
			return true;
	}
	
	srandom(getpid());

	/* free resources in use */
	if (surface)
		SDL_FreeSurface(surface);

	/* let the sound and input subsystems know about the new window */
	ri.Vid_NewWindow(vid.width, vid.height);
	
	gl_state.hwgamma = false;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,1);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
	
	if (use_stencil) {
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        }
        else {
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 4);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 4);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 4);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        }
		
	flags = SDL_OPENGL;
	if (fullscreen)
		flags |= SDL_FULLSCREEN;

	if ((surface = SDL_SetVideoMode(vid.width, vid.height, 0, flags)) == NULL) {
		Sys_Error("SDL SetVideoMode failed: %s\n", SDL_GetError());
		return false;
	}

	Com_Printf("\n^3SDL-GL Status^r\n");
	
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &red_bits);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blue_bits);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &green_bits);
	SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &color_bits);
	SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth_bits);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alpha_bits);
	SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multisamples);

	//Com_Printf("Color Red bits : %d\n", red_bits);
	//Com_Printf("Color Blue bits : %d\n", blue_bits);
	//Com_Printf("Color Green bits : %d\n", green_bits);
	Com_Printf("Color bits : %d\n", color_bits);
	Com_Printf("Depth bits : %d\n", depth_bits);
	Com_Printf("Alpha bits : %d\n", alpha_bits);
		
	if (!SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits)) {
		
		if (stencil_bits >= 1 && use_stencil->value) {
			have_stencil = true;
			Com_Printf("Stencil bits : %d\n", stencil_bits);
		}
	}

	Com_Printf("Double buffer enabled.\n");
	
	if (!SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multisamples)) {
		
		if (multisamples >= 1) {
			Com_Printf("Multisample buffer enabled.\n", multisamples);
		}
	}
	
	SDL_WM_SetCaption(Q2PVERSION, Q2PVERSION);
	
	SDL_ShowCursor(0);
	
	SetSDLGamma();
	
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);
	
	SDL_active = true;

	return true;
}


/*
** GLimp_SetMode
*/
int GLimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen)
{
	
	Com_Printf("Initializing OpenGL display\n");
	
	if (fullscreen)
		Com_Printf( "Setting fullscreen mode [%d]:", mode);
	else
		Com_Printf( "Setting windowed mode [%d]:", mode);

	if (!ri.Vid_GetModeInfo(pwidth, pheight, mode)) {
		Com_Printf( " Invalid mode\n");
		return rserr_invalid_mode;
	}
	
	Com_Printf( " %d %d\n", *pwidth, *pheight);

	if (!GLimp_InitGraphics(fullscreen)) {
		/* failed to set a valid mode in windowed mode */
		return rserr_invalid_mode;
	}
	
	// Vertex arrays
	qglEnableClientState (GL_VERTEX_ARRAY);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
	qglVertexPointer (3, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
	qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);
	
	return rserr_ok;
}

/*
** GLimp_Shutdown
**
** System specific graphics subsystem shutdown routine.  Destroys
** DIBs or DDRAW surfaces as appropriate.
*/
void GLimp_Shutdown(void)
{
	if (surface)
		SDL_FreeSurface(surface);
	surface = NULL;

	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_VIDEO)
		SDL_Quit();
	else
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		
	SDL_active = false;
}


void *GLimp_GetProcAddress(const char *func) { return SDL_GL_GetProcAddress(func);}
void GLimp_BeginFrame(float camera_separation) {}
void GLimp_EndFrame(void) { SDL_GL_SwapBuffers();}
void GLimp_AppActivate(qboolean active) {}



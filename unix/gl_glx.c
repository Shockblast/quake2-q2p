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
/*
** GLW_IMP.C
**
** This file contains ALL Unix specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#ifdef USE_XF86_DGA
 #include <X11/extensions/xf86dga.h>
#endif
#include <X11/extensions/xf86vmode.h>


#include "../ref_gl/gl_local.h"
#include "../client/keys.h"
#include "../unix/rw_unix.h"
#include "../ref_gl/glw.h"

#include <GL/glx.h>

glwstate_t glw_state;

Display *dpy = NULL;
int scrnum;
Window win;
GLXContext ctx = NULL;
Atom	wmDeleteWindow;
int    win_x,
       win_y;
Time myxtime;

XF86VidModeModeInfo **vidmodes;
XF86VidModeGamma oldgamma;
int num_vidmodes;
qboolean vidmode_active = false;
qboolean vidmode_ext = false;

// state struct passed in Init
in_state_t *in_state;

float    old_windowed_mouse;

qboolean mouse_avail,
         mouse_active,
         mlooking;
	 
int      mouse_buttonstate,
         mouse_oldbuttonstate,
         mouse_x, mouse_y,
         old_mouse_x, old_mouse_y,
         mx, my;

cvar_t  *m_filter,
        *in_mouse,
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

#if USE_XF86_DGA
cvar_t  *in_dgamouse;
qboolean dgamouse = false;
#endif

qboolean have_stencil = false;

//GLX Functions
XVisualInfo * (*qglXChooseVisual)( Display *dpy, int screen, int *attribList );
GLXContext (*qglXCreateContext)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
void (*qglXDestroyContext)( Display *dpy, GLXContext ctx );
Bool (*qglXMakeCurrent)( Display *dpy, GLXDrawable drawable, GLXContext ctx);
void (*qglXCopyContext)( Display *dpy, GLXContext src, GLXContext dst, GLuint mask );
void (*qglXSwapBuffers)( Display *dpy, GLXDrawable drawable );
int  (*qglXGetConfig) (Display * dpy, XVisualInfo * vis, int attrib, int *value);

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )
/*****************************************************************************/

Cursor CreateNullCursor(Display *display, Window root)
{
    Pixmap cursormask; 
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
          &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

void install_grabs(void)
{
	// inviso cursor
	XDefineCursor(dpy, win, CreateNullCursor(dpy, win));

	XGrabPointer(dpy, win, True, 0,GrabModeAsync, GrabModeAsync, 
                     win, None, CurrentTime);

	#if defined (USE_XF86_DGA)
	if (in_dgamouse->value) {
		int MajorVersion, MinorVersion;

		if (!XF86DGAQueryVersion(dpy, &MajorVersion, &MinorVersion)) { 
			// unable to query, probalby not supported
			Com_Printf("Failed to detect XF86DGA Mouse\n" );
			ri.Cvar_Set( "in_dgamouse", "0" );
		} else {
			dgamouse = true;
			XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		}
	} else 
	#endif
	
	XWarpPointer(dpy, None, win, 0, 0, 0, 0, vid.width / 2, vid.height / 2);
	
	XGrabKeyboard(dpy, win, False, GrabModeAsync, GrabModeAsync, CurrentTime);

}

void uninstall_grabs(void)
{
	if (!dpy || !win)
		return;

	#if defined (USE_XF86_DGA)
	if (dgamouse) {
		dgamouse = false;
		XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	}
	#endif

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

	// inviso cursor
	XUndefineCursor(dpy, win);

	mouse_active = false;
}

void Force_CenterView_f (void)
{
	in_state->viewangles[PITCH] = 0;
}

void RW_IN_MLookDown (void) 
{ 
	mlooking = true; 
}

void RW_IN_MLookUp (void) 
{
	mlooking = false;
	in_state->IN_CenterView_fp ();
}

void RW_IN_Init(in_state_t *in_state_p)
{

	in_state = in_state_p;

	// mouse variables
	m_filter = ri.Cvar_Get ("m_filter", "0", 0);
    	in_mouse = ri.Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	#if defined (USE_XF86_DGA)
    	in_dgamouse = ri.Cvar_Get ("in_dgamouse", "1", CVAR_ARCHIVE);
	#endif
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
	mouse_avail = false;
}

void RW_IN_Commands (void)
{
}


/*
===========
IN_Move
===========
*/
void RW_IN_Move (usercmd_t *cmd, int *mcoords)
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
	if ( (*in_state->in_strafe_state & 1) || (lookstrafe->value && mlooking ))
		cmd->sidemove += m_side->value * mx;
	else
		in_state->viewangles[YAW] -= m_yaw->value * mx;

	if ( (mlooking || freelook->value) && !(*in_state->in_strafe_state & 1))
		in_state->viewangles[PITCH] += m_pitch->value * my;
	else
		cmd->forwardmove -= m_forward->value * my;

	mx = my = 0;
}

void IN_DeactivateMouse( void ) 
{
	if (!mouse_avail || !dpy || !win)
		return;

	if (mouse_active) {
		uninstall_grabs();
		mouse_active = false;
	}
}

void IN_ActivateMouse( void ) 
{
	if (!mouse_avail || !dpy || !win)
		return;

	if (!mouse_active) {
		mx = my = 0; // don't spazz
		install_grabs();
		mouse_active = true;
	}
}

void RW_IN_Activate(qboolean active)
{
    	if (active || vidmode_active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}


void RW_IN_Frame (void)
{
}
int XLateKey(XKeyEvent *ev)
{

	int key;
	char buf[64];
	KeySym keysym;

	key = 0;

	XLookupString(ev, buf, sizeof buf, &keysym, 0);

	switch(keysym)
	{
		case XK_KP_Page_Up:	key = K_KP_PGUP;	break;
		case XK_Page_Up:	key = K_PGUP;		break;
		case XK_KP_Page_Down:	key = K_KP_PGDN;	break;
		case XK_Page_Down:	key = K_PGDN;		break;
		case XK_KP_Home:	key = K_KP_HOME;	break;
		case XK_Home:		key = K_HOME;		break;
		case XK_KP_End:		key = K_KP_END;		break;
		case XK_End:		key = K_END;		break;
		case XK_KP_Left:	key = K_KP_LEFTARROW;	break;
		case XK_Left:		key = K_LEFTARROW;	break;
		case XK_KP_Right:	key = K_KP_RIGHTARROW;	break;
		case XK_Right:		key = K_RIGHTARROW;	break;
		case XK_KP_Down:	key = K_KP_DOWNARROW;	break;
		case XK_Down:		key = K_DOWNARROW;	break;
		case XK_KP_Up:		key = K_KP_UPARROW;	break;
		case XK_Up:		key = K_UPARROW;	break;
		case XK_Escape:		key = K_ESCAPE;		break;
		case XK_KP_Enter:	key = K_KP_ENTER;	break;
		case XK_Return:		key = K_ENTER;		break;
		case XK_Tab:		key = K_TAB;		break;
		case XK_F1:		key = K_F1;		break;
		case XK_F2:		key = K_F2;		break;
		case XK_F3:		key = K_F3;		break;
		case XK_F4:		key = K_F4;		break;
		case XK_F5:		key = K_F5;		break;
		case XK_F6:		key = K_F6;		break;
		case XK_F7:		key = K_F7;		break;
		case XK_F8:		key = K_F8;		break;
		case XK_F9:		key = K_F9;		break;
		case XK_F10:		key = K_F10;		break;
		case XK_F11:		key = K_F11;		break;
		case XK_F12:		key = K_F12;		break;
		case XK_BackSpace:	key = K_BACKSPACE;	break;
		case XK_KP_Delete:	key = K_KP_DEL;		break;
		case XK_Delete:		key = K_DEL;		break;
		case XK_Pause:		key = K_PAUSE;		break;
		case XK_Shift_L:
		case XK_Shift_R:	key = K_SHIFT;		break;
		case XK_Execute: 
		case XK_Control_L: 
		case XK_Control_R:	key = K_CTRL;		break;
		case XK_Alt_L:	
		case XK_Meta_L: 
		case XK_Alt_R:	
		case XK_Meta_R: 	key = K_ALT;		break;
		case XK_KP_Begin:	key = K_KP_5;		break;
		case XK_Insert:		key = K_INS;		break;
		case XK_KP_Insert:	key = K_KP_INS; 	break;
		case XK_KP_Multiply:	key = '*';		break;
		case XK_KP_Add:		key = K_KP_PLUS;	break;
		case XK_KP_Subtract:	key = K_KP_MINUS;	break;
		case XK_KP_Divide:	key = K_KP_SLASH;	break;
		case 92:  /* Italy */	key = '~';		break;
		case 94:  /* Deutschland */ key = '~';		break;
		case 178: /* France */	key = '~'; 		break;
		case 186: /* Spain */	key = '~'; 		break;

		default:
			key = *(unsigned char*)buf;
			if (key >= 'A' && key <= 'Z')
				key = key - 'A' + 'a';
			if (key >= 1 && key <= 26)
				key = key + 'a' - 1;
			break;
	}
        
	if (print_keymap->value) {
		printf( "Key '%c' (%d) -> '%c' (%d)\n", (int)keysym, (int)keysym, key, key );
	}
	 
	return key;
}
void HandleEvents(void)
{
	XEvent event;
	int b;
	qboolean dowarp = false;
	int mwx = vid.width/2;
	int mwy = vid.height/2;
   
	if (!dpy)
		return;

	while (XPending(dpy)) {

		XNextEvent(dpy, &event);

		switch(event.type) {
		case KeyPress:
			myxtime = event.xkey.time;
			if (in_state && in_state->Key_Event_fp)
				in_state->Key_Event_fp(XLateKey(&event.xkey), true);
			break;
		case KeyRelease:
			if (in_state && in_state->Key_Event_fp)
				in_state->Key_Event_fp (XLateKey(&event.xkey), false);
			break;

		case MotionNotify:
			if (mouse_active) {
				#ifdef USE_XF86_DGA
				if (dgamouse) {
					mx += (event.xmotion.x + win_x) * 2;
					my += (event.xmotion.y + win_y) * 2;
				}
				#else
				if(true) {
					int xoffset = ((int)event.xmotion.x - mwx);
					int yoffset = ((int)event.xmotion.y - mwy);
						
					if(xoffset != 0 || yoffset != 0){
						
						mx += xoffset;
						my += yoffset;
							
						XSelectInput(dpy, win, 
						             X_MASK & ~PointerMotionMask);
						XWarpPointer(dpy, None, win, 0, 0, 0, 0,
						             mwx, mwy);
						XSelectInput(dpy, win, X_MASK);
					}
				}
				#endif
				else {
					mx += ((int)event.xmotion.x - mwx) * 2;
					my += ((int)event.xmotion.y - mwy) * 2;
					#ifndef USE_XF86_DGA
					mwx = event.xmotion.x;
					mwy = event.xmotion.y;
					#endif

					if (mx || my)
						dowarp = true;
				}
			}
			break;
			
		case ButtonPress:
			myxtime = event.xbutton.time;
			b=-1;
			if (event.xbutton.button == 1)
				b = 0;
			else if (event.xbutton.button == 2)
				b = 2;
			else if (event.xbutton.button == 3)
				b = 1;
			else if (event.xbutton.button == 4)
				in_state->Key_Event_fp(K_MWHEELUP, 1);
			else if (event.xbutton.button == 5)
				in_state->Key_Event_fp(K_MWHEELDOWN, 1);
			if (b>=0 && in_state && in_state->Key_Event_fp)
				in_state->Key_Event_fp (K_MOUSE1 + b, true);
			if (b>=0)
				mouse_buttonstate |= 1<<b;
			break;

		case ButtonRelease:
			b=-1;
			if (event.xbutton.button == 1)
				b = 0;
			else if (event.xbutton.button == 2)
				b = 2;
			else if (event.xbutton.button == 3)
				b = 1;
			else if (event.xbutton.button == 4)
				in_state->Key_Event_fp (K_MWHEELUP, 0);
      			else if (event.xbutton.button == 5)
				in_state->Key_Event_fp (K_MWHEELDOWN, 0);
			if (b>=0 && in_state && in_state->Key_Event_fp)
				in_state->Key_Event_fp (K_MOUSE1 + b, false);
			if (b>=0)
				mouse_buttonstate &= ~(1<<b);
			break;

		case CreateNotify :
			win_x = event.xcreatewindow.x;
			win_y = event.xcreatewindow.y;
			break;

		case ConfigureNotify :
			win_x = event.xconfigure.x;
			win_y = event.xconfigure.y;
			break;

		case ClientMessage:
			if (event.xclient.data.l[0] == wmDeleteWindow)
				ri.Cmd_ExecuteText(EXEC_NOW, "quit");
			break;
		}
	}
	   
	if (dowarp) {
		/* move the mouse to the window center again */
		XWarpPointer(dpy,None,win,0,0,0,0, vid.width/2,vid.height/2);
	}
}

Key_Event_fp_t Key_Event_fp;
void KBD_Init(Key_Event_fp_t fp)
{
	Key_Event_fp = fp;
}

void KBD_Update(void)
{
	HandleEvents();
}

void KBD_Close(void)
{
}

char *RW_Sys_GetClipboardData(void)
{
	Window sowner;
	Atom type, property;
	unsigned long len, bytes_left, tmp;
	unsigned char *data;
	int format, result;
	char *ret = NULL;
			
	sowner = XGetSelectionOwner(dpy, XA_PRIMARY);
			
	if (sowner != None) {
		property = XInternAtom(dpy,
				       "GETCLIPBOARDDATA_PROP",
				       False);
				
		XConvertSelection(dpy,
				  XA_PRIMARY, XA_STRING,
				  property, win, myxtime); // myxtime == time of last X event

		XFlush(dpy);


		XGetWindowProperty(dpy,
				   win, property,
				   0, 0, False, AnyPropertyType,
				   &type, &format, &len,
				   &bytes_left, &data);
				   
		if (bytes_left > 0) {
			result =
			XGetWindowProperty(dpy,
					   win, property,
					   0, bytes_left, True, AnyPropertyType,
					   &type, &format, &len,
					   &tmp, &data);
			if (result == Success) {
				ret = strdup((char *)data);
			}
			XFree(data);
		}
	}
	return ret;
}

qboolean GLimp_InitGL (void);

extern char*	strsignal(int sig);
static void signal_handler(int sig)
{
	char *sigstr;

	switch(sig) 
	{
		case SIGSEGV:
			sigstr = "SIGSEGV";
			break;
			
		default:
			sigstr = strerror(sig);
	}
	
	Com_Printf("Received signal %d, grrrr...\n", sig, strsignal(sig));
	GLimp_Shutdown();
	_exit(0);
}

static void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

/*
** GLimp_SetMode
*/
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;
	int	attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DOUBLEBUFFER, 1,
		GLX_DEPTH_SIZE, 24,
		GLX_BUFFER_SIZE, 24,
		GLX_ALPHA_SIZE, 8,
		GLX_STENCIL_SIZE, 8, 
		GLX_AUX_BUFFERS, 1, 
		None
	};
	int attrib2[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 4,
		GLX_GREEN_SIZE, 4,
		GLX_BLUE_SIZE, 4,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 16,
		None
	};
	
	Window root;
	XVisualInfo *visinfo;
	XSetWindowAttributes attr;
	unsigned long mask;
	int MajorVersion, MinorVersion;
	int actualWidth, actualHeight;
	int i, multisamples;

	Com_Printf("Initializing OpenGL display\n");

	if (fullscreen)
		Com_Printf( "Setting fullscreen mode [%d]:", mode);
	else
		Com_Printf( "Setting windowed mode [%d]:", mode);

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		Com_Printf(" Invalid mode\n" );
		return rserr_invalid_mode;
	}

	Com_Printf(" %d %d\n", width, height );

	// destroy the existing window
	GLimp_Shutdown ();

	Com_Printf("\n^3X11-GL Status^r\n");
	
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error couldn't open the X display\n");
		return rserr_invalid_mode;
	}

	scrnum = DefaultScreen(dpy);
	root = RootWindow(dpy, scrnum);

	// Get video mode list
	MajorVersion = MinorVersion = 0;
	if (!XF86VidModeQueryVersion(dpy, &MajorVersion, &MinorVersion)) { 
		vidmode_ext = false;
	} else {
		Com_Printf("XFree86-VidModeExtension Version %d.%d\n",
			      MajorVersion, MinorVersion);
		vidmode_ext = true;
	}

	visinfo = qglXChooseVisual(dpy, scrnum, attrib);
	if (!visinfo) {
		fprintf(stderr, "Error couldn't get an RGB, Double-buffered, Depth visual, Stencil Buffered\n");
		visinfo = qglXChooseVisual(dpy, scrnum, attrib2);
		if(!visinfo){
			fprintf(stderr, "Error couldn't get an RGB, Double-buffered, Depth visual\n");
			return rserr_invalid_mode;
		}
	}
	else
		have_stencil = true;
	
	gl_state.hwgamma = false;

	/* do some pantsness */
	if (qglXGetConfig) {
		int	red_bits, 
		        blue_bits, 
			green_bits,
			depth_bits,
			color_bits, 
			alpha_bits;

		qglXGetConfig(dpy, visinfo, GLX_RED_SIZE, &red_bits);
		qglXGetConfig(dpy, visinfo, GLX_BLUE_SIZE, &blue_bits);
		qglXGetConfig(dpy, visinfo, GLX_GREEN_SIZE, &green_bits);
		qglXGetConfig(dpy, visinfo, GLX_BUFFER_SIZE, &color_bits);
		qglXGetConfig(dpy, visinfo, GLX_DEPTH_SIZE, &depth_bits);
		qglXGetConfig(dpy, visinfo, GLX_ALPHA_SIZE, &alpha_bits);
		qglXGetConfig(dpy, visinfo, GLX_AUX_BUFFERS, &multisamples);

		//Com_Printf("Color Red bits : %d\n", red_bits);
		//Com_Printf("Color Blue bits : %d\n", blue_bits);
		//Com_Printf("Color Green bits : %d\n", green_bits);
		Com_Printf("Color bits : %d\n", color_bits);
		Com_Printf("Depth bits : %d\n", depth_bits);
		Com_Printf("Alpha bits : %d\n", alpha_bits);
	}
	/* stencilbuffer shadows */
	if (qglXGetConfig) {
		int	stencil_bits;

		if (!qglXGetConfig(dpy, visinfo, GLX_STENCIL_SIZE, &stencil_bits)) {
			if (stencil_bits >= 1 && use_stencil->value) {
				have_stencil = true;
				Com_Printf("Stencil bits : %d\n", stencil_bits);
			}
			else
				Com_Printf("Not using stencil buffer.\n");
		}
	} else {
		have_stencil = true;
	}
	
	Com_Printf("Double buffer enabled.\n");
	
	if (qglXGetConfig) {
		if (!qglXGetConfig(dpy, visinfo, GLX_AUX_BUFFERS, &multisamples)) {
			if (multisamples >= 1) 
				Com_Printf("Multisample buffer enabled.\n", multisamples);
		}
	}

	if (vidmode_ext) {
		int best_fit, best_dist, dist, x, y;
		
		XF86VidModeGetAllModeLines(dpy, scrnum, &num_vidmodes, &vidmodes);
				
		if (XF86VidModeGetGamma(dpy, scrnum, &oldgamma)) {
			gl_state.hwgamma = true;
			vid_gamma->modified = true;
			Com_Printf("Hardware gamma enabled.\n");
		}

		// Are we going fullscreen?  If so, let's change video mode
		if (fullscreen) {
			best_dist = 9999999;
			best_fit = -1;

			for (i = 0; i < num_vidmodes; i++) {
				if (width > vidmodes[i]->hdisplay ||
					height > vidmodes[i]->vdisplay)
					continue;

				x = width - vidmodes[i]->hdisplay;
				y = height - vidmodes[i]->vdisplay;
				dist = (x * x) + (y * y);
				if (dist < best_dist) {
					best_dist = dist;
					best_fit = i;
				}
			}

			if (best_fit != -1) {
				actualWidth = vidmodes[best_fit]->hdisplay;
				actualHeight = vidmodes[best_fit]->vdisplay;

				// change to the mode
				XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[best_fit]);
				vidmode_active = true;

				// Move the viewport to top left
				XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
			} else
				fullscreen = 0;
		}
	}

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	if (vidmode_active) {
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore | 
			CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	} else
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, 
	                    width, height, 0, 
			    visinfo->depth, InputOutput, 
			    visinfo->visual, mask, &attr);
	XMapWindow(dpy, win);
	
	XStoreName(dpy, win, Q2PVERSION);

	wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wmDeleteWindow, 1);

	if (vidmode_active) {
		XMoveWindow(dpy, win, 0, 0);
		XRaiseWindow(dpy, win);
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		XFlush(dpy);
		// Move the viewport to top left
		XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
	}

	XFlush(dpy);

	ctx = qglXCreateContext(dpy, visinfo, NULL, True);

	qglXMakeCurrent(dpy, win, ctx);

	*pwidth = width;
	*pheight = height;

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	qglXMakeCurrent(dpy, win, ctx);

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
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{
	uninstall_grabs();
	mouse_active = false;
#ifdef USE_XF86_DGA
	dgamouse = false;
#endif

	if (dpy) {
		if (ctx)
			qglXDestroyContext(dpy, ctx);
		if (win)
			XDestroyWindow(dpy, win);
		if (gl_state.hwgamma) {
			XF86VidModeSetGamma(dpy, scrnum, &oldgamma);
		}
		if (vidmode_active)
			XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[0]);
		XCloseDisplay(dpy);
	}
	ctx = NULL;
	dpy = NULL;
	win = 0;
	ctx = NULL;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
int GLimp_Init( void *hinstance, void *wndproc )
{
	InitSig();

	if (glw_state.OpenGLLib) {
#define GPA( a ) dlsym( glw_state.OpenGLLib, a )

		qglXChooseVisual = GPA("glXChooseVisual");
		qglXCreateContext = GPA("glXCreateContext");
		qglXDestroyContext = GPA("glXDestroyContext");
		qglXMakeCurrent = GPA("glXMakeCurrent");
		qglXCopyContext = GPA("glXCopyContext");
		qglXSwapBuffers = GPA("glXSwapBuffers");
		qglXGetConfig = GPA("glXGetConfig");
		return true;
	}
	return false;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame(float camera_separation) {}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
	qglFlush();
	qglXSwapBuffers(dpy, win);
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
}

/*
** UpdateHardwareGamma
**
** We are using gamma relative to the desktop, so that we can share it
** with software renderer and don't require to change desktop gamma
** to match hardware gamma image brightness. It seems that Quake 3 is
** using the opposite approach, but it has no software renderer after
** all.
*/
unsigned short gamma_ramp[3][256];
void UpdateHardwareGamma(void)
{
	XF86VidModeGamma gamma;
	float g;

	g = (1.3 - vid_gamma->value + 1);
	g = (g>1 ? g : 1);
	gamma.red = oldgamma.red * g;
	gamma.green = oldgamma.green * g;
	gamma.blue = oldgamma.blue * g;
	
	{
		float v, i_f;
		int i, o;

		for (o = 0; o < 3; o++)
		{
			for(i = 0; i < 256; i++)
			{
				i_f = (float)i/255.0f;
				v = pow(i_f, vid_gamma->value);
				v += 0.0 * (1.0f - v);

				if (v < 0.0f)
					v = 0.0f;

				else if (v > 1.0f)
					v = 1.0f;

				gamma_ramp[o][i] = (v * 65535.0f + 0.5f);
			}
		}
	}
	
	XF86VidModeSetGamma(dpy, scrnum, &gamma);
}






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
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client/client.h"
#include "../client/qmenu.h"

static int	m_main_cursor;

#define NUM_CURSOR_FRAMES 15
#define MAX_CROSSHAIRS 100
#define MAX_FONTS 32

static char *menu_in_sound	= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound	= "misc/menu3.wav";

void M_Menu_Game_f(void);
void M_Menu_LoadGame_f(void);
void M_Menu_SaveGame_f(void);
void M_Menu_PlayerConfig_f(void);
void M_Menu_DownloadOptions_f(void);
void M_Menu_Credits_f(void);
void M_Menu_Multiplayer_f(void);
void M_Menu_JoinServer_f(void);
void M_Menu_AddressBook_f(void);
void M_Menu_StartServer_f(void);
void M_Menu_DMOptions_f(void);
void M_Menu_Video_f(void);
void M_Menu_Options_f(void);
void M_Menu_Keys_f(void);
void M_Menu_Quit_f(void);
void M_Menu_Credits(void);

void ControlsSetMenuItemValues( void );

qboolean	m_entersound; // play after drawing a frame, so caching won't disrupt the sound

void	(*m_drawfunc) (void);
const char *(*m_keyfunc) (int key);

char **crosshair_names;
int	numcrosshairs;
cvar_t *con_font;
char **font_names;
int	numfonts;

void FreeFileList( char **list, int n );

//=============================================================================
/* Support Routines */

#define	MAX_MENU_DEPTH	8


typedef struct
{
	void	(*draw) (void);
	const char *(*key) (int k);
} menulayer_t;

menulayer_t	m_layers[MAX_MENU_DEPTH];
int		m_menudepth;

void refreshCursorButtons(void)
{
	cursor.buttonused[MOUSEBUTTON2] = true;
	cursor.buttonclicks[MOUSEBUTTON2] = 0;
	cursor.buttonused[MOUSEBUTTON1] = true;
	cursor.buttonclicks[MOUSEBUTTON1] = 0;
	cursor.mousewheeldown = 0;
	cursor.mousewheelup = 0;
}

#if defined(_WIN32)
void M_Banner( char *name )
{
	int w, h;
		
	re.DrawGetPicSize (&w, &h, name);
	re.DrawStretchPic2(viddef.width * 0.5 - ScaledVideo(w) * 0.5,
		           viddef.height * 0.5 - ScaledVideo(250),
		           ScaledVideo(w), ScaledVideo(h), name,
			   cl_menu_red->value, cl_menu_green->value, cl_menu_blue->value,
			   cl_menu_alpha->value);
}
#else 
void M_Banner( char *name )
{
	int w, h;
	int y;
	
	if (name == "m_banner_options")
		y = 215;
	else if (name == "m_banner_quit")
		y = 90;
	else if ((name == "m_banner_load_game") ||
	         (name == "m_banner_save_game"))
		y = 120;
	else 
		y = 110;
		
	re.DrawGetPicSize (&w, &h, name);
	re.DrawStretchPic2(viddef.width * 0.5 - ScaledVideo(w) * 0.5,
		           viddef.height * 0.5 - ScaledVideo(y),
		           ScaledVideo(w), ScaledVideo(h), name,
			   cl_menu_red->value, cl_menu_green->value, cl_menu_blue->value,
			   cl_menu_alpha->value);
}
#endif

void M_PushMenu ( void (*draw) (void), const char *(*key) (int k) )
{
	int	i;

	if (Cvar_VariableValue ("maxclients") == 1 
		&& Com_ServerState ())
		Cvar_Set ("paused", "1");

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<m_menudepth ; i++)
		if (m_layers[i].draw == draw && m_layers[i].key == key)
			m_menudepth = i;

	if (i == m_menudepth) {
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_Error (ERR_FATAL, "M_PushMenu: MAX_MENU_DEPTH");
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_menudepth++;
	}

	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	refreshCursorLink();
	refreshCursorButtons();

	cls.key_dest = key_menu;
}

void M_ForceMenuOff (void)
{
	refreshCursorLink();
	
	m_drawfunc = 0;
	m_keyfunc = 0;
	cls.key_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates ();
	Cvar_Set ("paused", "0");
}

void M_PopMenu (void)
{
	S_StartLocalSound( menu_out_sound );
	if (m_menudepth < 1)
		Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;
	
	refreshCursorLink();
	refreshCursorButtons();

	if (!m_menudepth)
		M_ForceMenuOff ();
}

const char *Default_MenuKey( menuframework_s *m, int key )
{
	const char *sound = NULL;
	menucommon_s *item;

	if ( m ) {
		if ( ( item = Menu_ItemAtCursor( m ) ) != 0 ) {
			if ( item->type == MTYPE_FIELD ) {
				if ( Field_Key( ( menufield_s * ) item, key ) )
					return NULL;
			}
		}
	}

	switch ( key ) {
		case K_ESCAPE:
			M_PopMenu();
			return menu_out_sound;
		case K_KP_UPARROW:
		case K_UPARROW:
			if ( m ) {
				m->cursor--;
				refreshCursorLink();
				Menu_AdjustCursor( m, -1 );
				sound = menu_move_sound;
			}
			break;
		case K_TAB:
			if ( m ) {
				m->cursor++;
				Menu_AdjustCursor( m, 1 );
				sound = menu_move_sound;
			}
			break;
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			if ( m ) {
				m->cursor++;
				refreshCursorLink();
				Menu_AdjustCursor( m, 1 );
				sound = menu_move_sound;
			}
			break;
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if ( m ) {
				Menu_SlideItem( m, -1 );
				sound = menu_move_sound;
			}
			break;
		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if ( m ) {
				Menu_SlideItem( m, 1 );
				sound = menu_move_sound;
			}
			break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX12:
		case K_AUX13:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
		case K_AUX17:
		case K_AUX18:
		case K_AUX19:
		case K_AUX20:
		case K_AUX21:
		case K_AUX22:
		case K_AUX23:
		case K_AUX24:
		case K_AUX25:
		case K_AUX26:
		case K_AUX27:
		case K_AUX28:
		case K_AUX29:
		case K_AUX30:
		case K_AUX31:
		case K_AUX32:
		
		case K_KP_ENTER:
		case K_ENTER:
			if ( m )
				Menu_SelectItem( m );
				sound = menu_move_sound;
			break;
	}

	return sound;
}

//=============================================================================

/*
================
M_DrawCharacter

Draws one solid graphics character
cx and cy are in 320*240 coordinates, and will be centered on
higher res screens.
================
*/
void InitMenuScale (void)
{
	//this is for scaling the menu...
	//this is kinda overkill, but whatever...
	// Knightmare- allow disabling of menu scaling
	// also, don't scale if < 800x600, then it would be smaller
	if (viddef.width > MENU_STATIC_WIDTH ) {
		menuScale.x = viddef.width/MENU_STATIC_WIDTH;
		menuScale.y = viddef.height/MENU_STATIC_HEIGHT;
		menuScale.avg = (menuScale.x + menuScale.y)*0.5f;
	}
	else {
		menuScale.x = 1;
		menuScale.y = 1;
		menuScale.avg = 1;
	}
}

void M_DrawCharacter(int cx, int cy, int num,
                     int red, int green, int blue,
		     int alpha, qboolean italic)
{
	if (VideoScale() > 1)
		re.DrawScaledChar(ScaledVideo(cx + ((MENU_STATIC_WIDTH-320)/2)),
				  ScaledVideo(cy + ((MENU_STATIC_HEIGHT- 240)/2)), num,
				  VideoScale(), red, green, blue, alpha, italic);
	else
		re.DrawScaledChar(cx + ((viddef.width - 320)>>1), cy + ((viddef.height - 240)>>1 ),
				  num, VideoScale(), red, green, blue, alpha, italic);
}

void M_Print (int cx, int cy, char *str)
{
	int red=0, green=255, blue=0;
	
	TextColor ((int)alt_text_color->value, &red, &green, &blue);
	
	while (*str) {
		M_DrawCharacter (cx, cy, (*str)+128, red,green,blue,255, false);
		str++;
		cx += MENU_FONT_SIZE;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str) {
		M_DrawCharacter (cx, cy, *str, 255, 255, 255, 255, false);
		str++;
		cx += MENU_FONT_SIZE;
	}
}

void M_DrawPic (int x, int y, char *pic)
{
	re.DrawPic (x + ((viddef.width - 320)>>1), y + ((viddef.height - 240)>>1), pic, 1.0f);
}

/*
=============
M_DrawCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
void M_DrawCursor( int x, int y, int f )
{
	char	cursorname[80];
	static qboolean cached;
	int w,h;
	
	if ( !cached ) {
		int i;

		for ( i = 0; i < NUM_CURSOR_FRAMES; i++ ) {
			Com_sprintf( cursorname, sizeof( cursorname ), "m_cursor%d", i );

			re.RegisterPic( cursorname );
		}
		cached = true;
	}

	Com_sprintf( cursorname, sizeof(cursorname), "m_cursor%d", f );
	re.DrawGetPicSize( &w, &h, cursorname );
	re.DrawStretchPic(x, y, ScaledVideo(w), ScaledVideo(h), cursorname, 1.0f);
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	M_DrawCharacter (cx, cy, 1, 255,255,255,255, false);
	
	for (n = 0; n < lines; n++) {
		cy += MENU_FONT_SIZE;
		M_DrawCharacter (cx, cy, 4, 255,255,255,255, false);
	}
	
	M_DrawCharacter (cx, cy+MENU_FONT_SIZE, 7, 255,255,255,255, false);

	// draw middle
	cx += MENU_FONT_SIZE;
	while (width > 0) {
		cy = y;
		M_DrawCharacter (cx, cy, 2, 255,255,255,255, false);
		for (n = 0; n < lines; n++) {
			cy += MENU_FONT_SIZE;
			M_DrawCharacter (cx, cy, 5, 255,255,255,255, false);
		}
		M_DrawCharacter (cx, cy+MENU_FONT_SIZE, 8, 255,255,255,255, false);
		width -= 1;
		cx += MENU_FONT_SIZE;
	}

	// draw right side
	cy = y;
	M_DrawCharacter (cx, cy, 3, 255,255,255,255, false);
	for (n = 0; n < lines; n++) {
		cy += MENU_FONT_SIZE;
		M_DrawCharacter (cx, cy, 6, 255,255,255,255, false);
	}
	M_DrawCharacter (cx, cy+MENU_FONT_SIZE, 9, 255,255,255,255, false);
}

/*
 * ===========
 * Q2P MENU
 * ===========
 */
 
static menuframework_s	s_q2p_menu;

static menuseparator_s	s_q2p_client_side;
static menuseparator_s	s_q2p_graphic;
static menuseparator_s	s_q2p_hud;

static menuaction_s	s_options_q2p_action;
static menuaction_s	s_options_q2p_action_client;
static menuaction_s	s_options_q2p_action_graphic;
static menuaction_s	s_options_q2p_action_hud;

static menulist_s	s_q2p_3dcam;
static menuslider_s	s_q2p_3dcam_distance;
static menuslider_s	s_q2p_3dcam_angle;
static menulist_s	s_q2p_bobbing;
#if defined(PARTICLESYSTEM)
static menulist_s	s_q2p_blaster_bubble;
static menulist_s	s_q2p_blaster_color;
static menulist_s	s_q2p_hyperblaster_particles;
static menulist_s	s_q2p_hyperblaster_bubble;
static menulist_s	s_q2p_hyperblaster_color;
static menulist_s	s_q2p_railtrail;
static menulist_s	s_q2p_railspiral_part;
static menuslider_s	s_q2p_railtrail_red;
static menuslider_s	s_q2p_railtrail_green;
static menuslider_s	s_q2p_railtrail_blue;
static menulist_s	s_q2p_gun_smoke;
static menulist_s	s_q2p_explosions;
static menuslider_s	s_q2p_explosions_scale;
static menulist_s	s_q2p_flame_enh;
static menuslider_s	s_q2p_flame_size;
static menulist_s	s_q2p_particles_type;
static menulist_s	s_q2p_teleport;
static menulist_s	s_q2p_nukeblast;
#else
static menulist_s	s_q2p_gl_particles;
#endif
static menulist_s	s_q2p_flashlight;
static menuslider_s	s_q2p_flashlight_red;
static menuslider_s	s_q2p_flashlight_green;
static menuslider_s	s_q2p_flashlight_blue;
static menuslider_s	s_q2p_flashlight_intensity;
static menuslider_s	s_q2p_flashlight_distance;
static menuslider_s	s_q2p_flashlight_decscale;


static menuslider_s	s_q2p_gl_reflection;
static menulist_s	s_q2p_gl_reflection_shader;
static menuslider_s	s_q2p_waves;
static menulist_s	s_q2p_water_trans;
static menulist_s	s_q2p_underwater_mov;
static menulist_s	s_q2p_underwater_view;
static menulist_s	s_q2p_caustics;
static menuslider_s	s_q2p_det_tex;
static menulist_s	s_q2p_fog;
static menuslider_s	s_q2p_fog_density;
static menuslider_s	s_q2p_fog_red;
static menuslider_s	s_q2p_fog_green;
static menuslider_s	s_q2p_fog_blue;
static menulist_s	s_q2p_fog_underwater;
static menulist_s	s_q2p_minimap;
static menulist_s	s_q2p_minimap_style;
static menuslider_s	s_q2p_minimap_size;
static menuslider_s	s_q2p_minimap_zoom;
static menuslider_s	s_q2p_minimap_x;
static menuslider_s	s_q2p_minimap_y;
static menulist_s	s_q2p_shadows;
static menulist_s	s_q2p_motionblur;
static menuslider_s	s_q2p_motionblur_intens;
#if defined(PARTICLESYSTEM)
static menufield_s	s_q2p_r_decals;
#else
static menulist_s	s_q2p_r_decals;
#endif
static menulist_s	s_q2p_cellshading;
static menulist_s	s_q2p_blooms;
static menuslider_s	s_q2p_blooms_alpha;
static menuslider_s	s_q2p_blooms_diamond_size;
static menuslider_s	s_q2p_blooms_intensity;
static menuslider_s	s_q2p_blooms_darken;
static menuslider_s	s_q2p_blooms_sample_size;
static menulist_s	s_q2p_blooms_fast_sample;
static menulist_s	s_q2p_lensflare;
static menuslider_s	s_q2p_lensflare_intens;

static menulist_s	s_q2p_drawclock;
static menulist_s	s_q2p_drawdate;
static menulist_s	s_q2p_drawmaptime;
static menuslider_s	s_q2p_draw_x;
static menuslider_s	s_q2p_draw_y;
static menulist_s	s_q2p_drawchathud;
static menulist_s	s_q2p_drawplayername;
static menulist_s	s_q2p_drawping;
static menulist_s	s_q2p_drawfps;
static menufield_s      s_q2p_option_maxfps;
static menulist_s	s_q2p_drawrate;
static menulist_s	s_q2p_drawnetgraph;
static menulist_s	s_q2p_drawnetgraph_pos;
static menuslider_s	s_q2p_drawnetgraph_pos_x;
static menuslider_s	s_q2p_drawnetgraph_pos_y;
static menuslider_s	s_q2p_drawnetgraph_alpha;
static menuslider_s	s_q2p_drawnetgraph_scale;
static menulist_s	s_q2p_zoom_sniper;
static menufield_s	s_q2p_netgraph_image;
static menulist_s	s_q2p_crosshair_box;
static menuslider_s	s_q2p_cross_scale;
static menuslider_s	s_q2p_cross_red;
static menuslider_s	s_q2p_cross_green;
static menuslider_s	s_q2p_cross_blue;
static menuslider_s	s_q2p_cross_pulse;
static menuslider_s	s_q2p_cross_alpha;
static menulist_s	s_q2p_font_box;
static menuslider_s	s_q2p_fontsize_slider;
static menulist_s	s_q2p_hud_scale;
static menuslider_s	s_q2p_hud_red;
static menuslider_s	s_q2p_hud_green;
static menuslider_s	s_q2p_hud_blue;
static menuslider_s	s_q2p_hud_alpha;
static menufield_s	s_q2p_conback_image;
static menuslider_s	s_q2p_menu_conback_alpha;

extern cvar_t          *scr_netgraph_image;
extern cvar_t          *scr_netgraph_pos_x;
extern cvar_t          *scr_netgraph_pos_y;
#if defined(PARTICLESYSTEM)
extern cvar_t          *cl_flame_enh;
extern cvar_t          *cl_flame_size;
#endif

static void
ThirdPersonCamFunc(void *unused)
{
	Cvar_SetValue("cl_3dcam", s_q2p_3dcam.curvalue);
}

static void
ThirdPersonDistFunc(void *unused)
{
	Cvar_SetValue( "cl_3dcam_dist", s_q2p_3dcam_distance.curvalue * 25 );
}

static void
ThirdPersonAngleFunc(void *unused)
{
	Cvar_SetValue( "cl_3dcam_angle", s_q2p_3dcam_angle.curvalue * 10 );
}

static void
BobbingFunc(void *unused)
{
	Cvar_SetValue("cl_bobbing", s_q2p_bobbing.curvalue);
}

#if defined(PARTICLESYSTEM)
static void
BlasterBubbleFunc(void *unused)
{
	Cvar_SetValue("cl_blaster_bubble", s_q2p_blaster_bubble.curvalue);
}

static void
BlasterColorFunc(void *unused)
{
	Cvar_SetValue("cl_blaster_color", s_q2p_blaster_color.curvalue);
}

static void
HyperBlasterParticlesFunc(void *unused)
{
	Cvar_SetValue("cl_hyperblaster_particles", s_q2p_hyperblaster_particles.curvalue);
}

static void
HyperBlasterBubbleFunc(void *unused)
{
	Cvar_SetValue("cl_hyperblaster_bubble", s_q2p_hyperblaster_bubble.curvalue);
}

static void
HyperBlasterColorFunc(void *unused)
{
	Cvar_SetValue("cl_hyperblaster_color", s_q2p_hyperblaster_color.curvalue);
}

static void
RailTrailFunc(void *unused)
{
	Cvar_SetValue("cl_railtype", s_q2p_railtrail.curvalue);
}

static void
RailSpiralPartFunc(void *unused)
{
	Cvar_SetValue("cl_railspiral_lens", s_q2p_railspiral_part.curvalue);
}

static void
RailTrailColorRedFunc(void *unused)
{
	Cvar_SetValue("cl_railred", s_q2p_railtrail_red.curvalue*16);
}

static void
RailTrailColorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_railgreen", s_q2p_railtrail_green.curvalue*16);
}

static void
RailTrailColorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_railblue", s_q2p_railtrail_blue.curvalue*16);
}

static void
GunSmokeFunc(void *unused)
{
	Cvar_SetValue("cl_gunsmoke", s_q2p_gun_smoke.curvalue);
}

static void
ExplosionsFunc(void *unused)
{
	Cvar_SetValue("cl_explosion", s_q2p_explosions.curvalue);
}

static void
ExplosionsScaleFunc(void *unused)
{
	Cvar_SetValue("cl_explosion_scale", s_q2p_explosions_scale.curvalue / 10);
}

static void
FlameEnhFunc(void *unused)
{
	Cvar_SetValue("cl_flame_enh", s_q2p_flame_enh.curvalue);
}

static void
FlameEnhSizeFunc(void *unused)
{
	Cvar_SetValue("cl_flame_size", s_q2p_flame_size.curvalue / 1 );
}

static void
ParticlesTypeFunc(void *unused)
{
	Cvar_SetValue("cl_particles_type", s_q2p_particles_type.curvalue);
}

static void
TelePortFunc(void *unused)
{
	Cvar_SetValue("cl_teleport_particles", s_q2p_teleport.curvalue);
}

static void
NukeBlastFunc(void *unused)
{
	Cvar_SetValue("cl_nukeblast_enh", s_q2p_nukeblast.curvalue);
}
#else
static void
GLParticleFunc(void *unused)
{
	Cvar_SetValue("gl_particles", s_q2p_gl_particles.curvalue);
}
#endif

static void
FlashLightFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight", s_q2p_flashlight.curvalue);
}

static void
FlashLightColorRedFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_red", s_q2p_flashlight_red.curvalue / 10);
}

static void
FlashLightColorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_green", s_q2p_flashlight_green.curvalue / 10);
}

static void
FlashLightColorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_blue", s_q2p_flashlight_blue.curvalue / 10);
}

static void
FlashLightIntensityFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_intensity", s_q2p_flashlight_intensity.curvalue * 10);
}

static void
FlashLightDistanceFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_distance", s_q2p_flashlight_distance.curvalue * 10);
}

static void
FlashLightDecreaseScaleFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_decscale", s_q2p_flashlight_decscale.curvalue / 10);
}

static void
WaterReflection(void *unused)
{
	Cvar_SetValue("gl_water_reflection", s_q2p_gl_reflection.curvalue / 10);
}

static void
WaterShaderReflection(void *unused)
{
	Cvar_SetValue("gl_water_reflection_shader", s_q2p_gl_reflection_shader.curvalue);
}

static void
DetTexFunc(void *unused)
{
	Cvar_SetValue("gl_detailtextures", s_q2p_det_tex.curvalue);
}

static void
WaterWavesFunc(void *unused)
{
	Cvar_SetValue("gl_water_waves", s_q2p_waves.curvalue);
}

static void
TexCausticFunc(void *unused)
{
	Cvar_SetValue("gl_water_caustics", s_q2p_caustics.curvalue);
}

static void
TransWaterFunc(void *unused)
{
	Cvar_SetValue("cl_underwater_trans", s_q2p_water_trans.curvalue);
}

static void
UnderWaterMovFunc(void *unused)
{
	Cvar_SetValue("cl_underwater_movement", s_q2p_underwater_mov.curvalue);
}

static void
UnderWaterViewFunc(void *unused)
{
	Cvar_SetValue("cl_underwater_view", s_q2p_underwater_view.curvalue);
}

static void
FogFunc(void *unused)
{
	Cvar_SetValue("gl_fogenable", s_q2p_fog.curvalue);
}

static void
FogDensityFunc(void *unused)
{
	Cvar_SetValue("gl_fogdensity", s_q2p_fog_density.curvalue / 10000);
}

static void
FogRedFunc(void *unused)
{
	Cvar_SetValue("gl_fogred", s_q2p_fog_red.curvalue / 10);
}

static void
FogGreenFunc(void *unused)
{
	Cvar_SetValue("gl_foggreen", s_q2p_fog_green.curvalue / 10);
}

static void
FogBlueFunc(void *unused)
{
	Cvar_SetValue("gl_fogblue", s_q2p_fog_blue.curvalue / 10);
}

static void
FogUnderWaterFunc(void *unused)
{
	Cvar_SetValue("gl_fogunderwater", s_q2p_fog_underwater.curvalue);
}

static void
ShadowsFunc(void *unused)
{
	Cvar_SetValue("gl_shadows", s_q2p_shadows.curvalue);
}

static void
MotionBlurFunc(void *unused)
{
	Cvar_SetValue("gl_motionblur", s_q2p_motionblur.curvalue);
}

static void
MotionBlurIntensFunc(void *unused)
{
	Cvar_SetValue("gl_motionblur_intensity", s_q2p_motionblur_intens.curvalue / 10);
}

static void
BloomsFunc(void *unused)
{
	Cvar_SetValue("gl_bloom", s_q2p_blooms.curvalue);
}

static void
BloomsAlphaFunc(void *unused)
{
	Cvar_SetValue("gl_bloom_alpha", s_q2p_blooms_alpha.curvalue / 10);
}

static void
BloomsDiamondSizeFunc(void *unused)
{
	Cvar_SetValue("gl_bloom_diamond_size", s_q2p_blooms_diamond_size.curvalue * 2);
}

static void
BloomsIntensityFunc(void *unused)
{
	Cvar_SetValue("gl_bloom_intensity", s_q2p_blooms_intensity.curvalue / 10);
}

static void
BloomsDarkenFunc(void *unused)
{
	Cvar_SetValue("gl_bloom_darken", s_q2p_blooms_darken.curvalue);
}

static void
BloomsSampleSizeFunc(void *unused)
{
	Cvar_SetValue("gl_bloom_sample_size", pow(2, s_q2p_blooms_sample_size.curvalue));
}

static void
BloomsFastSampleFunc(void *unused)
{
	Cvar_SetValue("gl_bloom_fast_sample", s_q2p_blooms_fast_sample.curvalue);
}

static void
MiniMapFunc(void *unused)
{
	Cvar_SetValue("gl_minimap", s_q2p_minimap.curvalue);
}

static void
MiniMapStyleFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_style", s_q2p_minimap_style.curvalue);
}

static void
MiniMapSizeFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_size", s_q2p_minimap_size.curvalue);
}

static void
MiniMapZoomFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_zoom", s_q2p_minimap_zoom.curvalue / 10);
}

static void
MiniMapXPosFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_x", s_q2p_minimap_x.curvalue / 1);
}

static void
MiniMapYPosFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_y", s_q2p_minimap_y.curvalue / 1);
}

#if !defined(PARTICLESYSTEM)
static void
DecalsFunc(void *unused)
{
	Cvar_SetValue("r_decals", s_q2p_r_decals.curvalue);
}
#endif

static void
CellShadingFunc(void *unused)
{
	Cvar_SetValue("gl_cellshade", s_q2p_cellshading.curvalue);
}

static void
LensFlareFunc(void *unused)
{
	Cvar_SetValue("gl_lensflare", s_q2p_lensflare.curvalue);
}

static void
LensFlareIntensFunc(void *unused)
{
	Cvar_SetValue("gl_lensflare_intens", s_q2p_lensflare_intens.curvalue / 10);
}

static void
DrawPingFunc(void *unused)
{
	Cvar_SetValue("cl_draw_ping", s_q2p_drawping.curvalue);
}

static void
UpdateHudXFunc(void *unused)
{
	Cvar_SetValue("cl_draw_x", s_q2p_draw_x.curvalue / 20.0);
}

static void
UpdateHudYFunc(void *unused)
{
	Cvar_SetValue("cl_draw_y", s_q2p_draw_y.curvalue / 20.0);
}

static void
DrawFPSFunc(void *unused)
{
	Cvar_SetValue("cl_draw_fps", s_q2p_drawfps.curvalue);
}

static void
DrawClockFunc(void *unused)
{
	Cvar_SetValue("cl_draw_clock", s_q2p_drawclock.curvalue);
}

static void
DrawDateFunc(void *unused)
{
	Cvar_SetValue("cl_draw_date", s_q2p_drawdate.curvalue);
}

static void
DrawMapTimeFunc(void *unused)
{
	Cvar_SetValue("cl_draw_maptime", s_q2p_drawmaptime.curvalue);
}

static void
DrawRateFunc(void *unused)
{
	Cvar_SetValue("cl_draw_rate", s_q2p_drawrate.curvalue);
}

static void
DrawNetGraphFunc(void *unused)
{
	Cvar_SetValue("netgraph", s_q2p_drawnetgraph.curvalue);
}

static void
DrawNetGraphPosFunc(void *unused)
{
	Cvar_SetValue("netgraph_pos", s_q2p_drawnetgraph_pos.curvalue);
}

static void
DrawNetGraphPosXFunc(void *unused)
{
	Cvar_SetValue("netgraph_pos_x", s_q2p_drawnetgraph_pos_x.curvalue / 1);
}

static void
DrawNetGraphPosYFunc(void *unused)
{
	Cvar_SetValue("netgraph_pos_y", s_q2p_drawnetgraph_pos_y.curvalue / 1);
}

static void
DrawNetGraphAlphaFunc(void *unused)
{
	Cvar_SetValue("netgraph_trans", s_q2p_drawnetgraph_alpha.curvalue / 10);
}

static void
DrawNetGraphScaleFunc(void *unused)
{
	Cvar_SetValue("netgraph_size", s_q2p_drawnetgraph_scale.curvalue / 1);
}

static void
DrawChatHudFunc(void *unused)
{
	Cvar_SetValue("cl_draw_chathud", s_q2p_drawchathud.curvalue);
}

static void
DrawPlayerNameFunc(void *unused)
{
	Cvar_SetValue("cl_draw_playername", s_q2p_drawplayername.curvalue);
}

static void
DrawZoomSniperFunc(void *unused)
{
	Cvar_SetValue("cl_draw_zoom_sniper", s_q2p_zoom_sniper.curvalue);
}

static void
CrosshairFunc(void *unused)
{
	if (s_q2p_crosshair_box.curvalue == 0) {
		Cvar_SetValue( "crosshair", 0);
		return;
	}
	else
		Cvar_SetValue("crosshair",
		              atoi(strdup(crosshair_names[s_q2p_crosshair_box.curvalue]+2)));
}

static void
CrossScaleFunc(void *unused)
{
	Cvar_SetValue("crosshair_scale", s_q2p_cross_scale.curvalue / 10);
}

static void
CrossColorRedFunc(void *unused)
{
	Cvar_SetValue("crosshair_red", s_q2p_cross_red.curvalue / 10);
}

static void
CrossColorGreenFunc(void *unused)
{
	Cvar_SetValue("crosshair_green", s_q2p_cross_green.curvalue / 10);
}

static void
CrossColorBlueFunc(void *unused)
{
	Cvar_SetValue("crosshair_blue", s_q2p_cross_blue.curvalue / 10);
}

static void
CrossPulseFunc(void *unused)
{
	Cvar_SetValue("crosshair_pulse", s_q2p_cross_pulse.curvalue / 1);
}

static void
CrossAlphaFunc(void *unused)
{
	Cvar_SetValue("crosshair_alpha", s_q2p_cross_alpha.curvalue / 10);
}

static
void FontSizeFunc( void *unused )
{
	Cvar_SetValue( "con_font_size", s_q2p_fontsize_slider.curvalue * 4 );
}

static
void FontFunc( void *unused )
{
	Cvar_Set( "con_font", font_names[s_q2p_font_box.curvalue] );
}

int GetHudRes( void )
{
	int res = cl_hudscale->value;

	if (res<=320)      return 1;
	else if (res<=400) return 2;
	else if (res<=640) return 3;
	else if (res<=800) return 4;

	return 5;
}

static void
HudScaleFunc( void *unused )
{
	if (s_q2p_hud_scale.curvalue == 0)
		Cvar_SetValue( "cl_hudscale", 2 );
	else if (s_q2p_hud_scale.curvalue == 1)
		Cvar_SetValue( "cl_hudscale", 3 );
	else if (s_q2p_hud_scale.curvalue == 2)
		Cvar_SetValue( "cl_hudscale", 4 );
	else if (s_q2p_hud_scale.curvalue == 3)
		Cvar_SetValue( "cl_hudscale", 5 );
	else if (s_q2p_hud_scale.curvalue == 4)
		Cvar_SetValue( "cl_hudscale", 6 );
}

static void
HudColorRedFunc(void *unused)
{
	Cvar_SetValue("cl_hudred", s_q2p_hud_red.curvalue / 10);
}

static void
HudColorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_hudgreen", s_q2p_hud_green.curvalue / 10);
}

static void HudColorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_hudblue", s_q2p_hud_blue.curvalue / 10);
}

static void HudAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_hudalpha", s_q2p_hud_alpha.curvalue / 10);
}

static void MenuConbackAlphaFunc(void *unused)
{
	Cvar_SetValue("menu_alpha", s_q2p_menu_conback_alpha.curvalue / 10);
}


void Q2P_MenuDraw_Client(void)
{
	Menu_AdjustCursor(&s_q2p_menu, 1);
	Menu_Draw(&s_q2p_menu);
	
}

void Q2P_MenuDraw_Graphic(void)
{
	Menu_AdjustCursor(&s_q2p_menu, 1);
	Menu_Draw(&s_q2p_menu);
	
}

void Q2P_MenuDraw_Hud(void)
{
	Menu_AdjustCursor(&s_q2p_menu, 1);
	Menu_Draw(&s_q2p_menu);
	
	DrawMenuCrosshair();
	SetCrosshairCursor();
}

const char *Q2P_MenuKey_Client(int key)
{
	return Default_MenuKey(&s_q2p_menu, key);
}

const char *Q2P_MenuKey_Graphic(int key)
{
#if defined(PARTICLESYSTEM)
	if (key == K_ESCAPE)
		Cvar_Set("r_decals", s_q2p_r_decals.buffer);
#endif
	return Default_MenuKey(&s_q2p_menu, key);
}

const char *Q2P_MenuKey_Hud(int key)
{
	if (key == K_ESCAPE) {
		Cvar_Set("cl_maxfps", s_q2p_option_maxfps.buffer);
		Cvar_Set("netgraph_image", s_q2p_netgraph_image.buffer);
		Cvar_Set("conback_image", s_q2p_conback_image.buffer);
	}
	
	return Default_MenuKey(&s_q2p_menu, key);
}

void SetFontCursor (void)
{
	int i;
	s_q2p_font_box.curvalue = 0;

	if (!con_font)
		con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);

	if (numfonts>1)
		for (i=0; font_names[i]; i++) {
			if (!Q_strcasecmp(con_font->string, font_names[i])) {
				s_q2p_font_box.curvalue = i;
				return;
			}
		}
}

qboolean fontInList (char *check, int num, char **list)
{
	int i;
	
	for (i=0;i<num;i++)
		if (!Q_strcasecmp(check, list[i]))
			return true;
	
	return false;
}

void insertFont (char ** list, char *insert, int len )
{
	int i, j;

	if (!list)
		return;

	//i=1 so default stays first!
	for (i=1; i<len; i++) {
		if (!list[i])
			break;

		if (strcmp( list[i], insert )) {
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = strdup(insert);

			return;
		}
	}

	list[len] = strdup(insert);
}

char **SetFontNames (void)
{
	char *curFont;
	char **list = 0, *p;
	char findname[1024];
	int nfonts = 0, nfontnames;
	char **fontfiles;
	char *path = NULL;
	int i;

	list = Q_malloc( sizeof( char * ) * MAX_FONTS );
	memset( list, 0, sizeof( char * ) * MAX_FONTS );

	list[0] = strdup("default");

	nfontnames = 1;

	path = FS_NextPath( path );
	
	while (path)  {
	
		Com_sprintf( findname, sizeof(findname), "%s/fonts/*.*", path );
		fontfiles = FS_ListFiles( findname, &nfonts, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		for (i=0;i<nfonts && nfontnames<MAX_FONTS;i++) {
			int num;

			if (!fontfiles || !fontfiles[i])	// Knightmare added array base check
				continue;

			p = strstr(fontfiles[i], "/fonts/"); p++;
			p = strstr(p, "/"); p++;

			if (!strstr(p, ".tga") &&
			    !strstr(p, ".png") &&
			    !strstr(p, ".jpg") &&
			    !strstr(p, ".pcx"))
				continue;

			num = strlen(p)-4;
			p[num] = 0;//NULL;

			curFont = p;

			if (!fontInList(curFont, nfontnames, list)) {
				insertFont(list, strdup(curFont),nfontnames);
				nfontnames++;
			}
			
			//set back so whole string get deleted.
			p[num] = '.';
		}
		if (nfonts)
			FreeFileList( fontfiles, nfonts );
		
		path = FS_NextPath( path );
	}

	//check pak after
	if ((fontfiles = FS_ListPak("fonts/", &nfonts))) {
		
		for (i=0;i<nfonts && nfontnames<MAX_FONTS;i++) {

			int num;

			if (!fontfiles || !fontfiles[i])	// Knightmare added array base check
				continue;

			p = strstr(fontfiles[i], "/"); p++;

			if (!strstr(p, ".tga") &&
			    !strstr(p, ".png") &&
			    !strstr(p, ".jpg") &&
			    !strstr(p, ".pcx"))
				continue;

			num = strlen(p)-4;
			p[num] = 0; //NULL;

			curFont = p;

			if (!fontInList(curFont, nfontnames, list)) {
				insertFont(list, strdup(curFont),nfontnames);
				nfontnames++;
			}
			
			//set back so whole string get deleted.
			p[num] = '.';

		}
	}
	
	if (nfonts)
		FreeFileList( fontfiles, nfonts );

	numfonts = nfontnames;

	return list;		
}

void
Q2P_MenuInit_Client(void)
{
	int y = 0;
	
	static const char *yesno_names[] = {
		"Disabled",
		"Enabled ",
		NULL
	};

#if defined(PARTICLESYSTEM)
	static const char *effect_colors[] = {
		"Orange",
		"Red   ",
		"Green ",
		"Blue  ",
		NULL
	};
	static const char *bubble_modes[] = {
		"Disabled  ",
		"Always    ",
		"Underwater",
		NULL
	};
	static const char *railtrail_effect[] = {
		"Laser        ",
		"Spiral       ",
		"Spiral & Beam",
		"Devastator   ",
		NULL
	};
	static const char *railspiral_part[] = {
		"Generic    ",
		"Lens Flares",
		NULL
	};
	static const char *explosion_effect[] = {
		"Old 3D                  ",
		"QMax Explosions         ",
		"QMax Inferno            ",
		"QMax Explosions & Sparks",
		"QMax Inferno & Sparks   ",
		NULL
	};
	static const char *teleport_effect[] = {
		"Generic Particles    ",
		"Exploding Bubbles    ",
		"Exploding Lens Flares",
		"Rotating Bubbles     ",
		"Rotating Lens Flares ",
		NULL
	};
	static const char *particles_type[] = {
		"Generic Particles",
		"Bubbles          ",
		"Lens Flares      ",
		NULL
	};
	static const char *nukeblast_effect[] = {
		"Generic Particles",
		"Thunder          ",
		NULL
	};
	static const char *gunsmoke_effect[] = {
		"Disabled         ",
		"Impacts          ",
		"Impacts & Weapons",
		NULL
	};
#endif

	s_q2p_menu.x = viddef.width * 0.50;
	s_q2p_menu.y = viddef.height * 0.50 - ScaledVideo(200);
	s_q2p_menu.nitems = 0;

	s_q2p_client_side.generic.type = MTYPE_SEPARATOR;
	s_q2p_client_side.generic.name = "Client Side Options";
	s_q2p_client_side.generic.x = 70;
	s_q2p_client_side.generic.y = y;

	s_q2p_3dcam.generic.type = MTYPE_SPINCONTROL;
	s_q2p_3dcam.generic.x = 0;
	s_q2p_3dcam.generic.y = y+=2*(MENU_FONT_SIZE+2);
	s_q2p_3dcam.generic.name = "THIRD PERSON CAMERA";
	s_q2p_3dcam.generic.callback = ThirdPersonCamFunc;
	s_q2p_3dcam.itemnames = yesno_names;
	s_q2p_3dcam.generic.statusbar = "^3Third Person Camera";

	s_q2p_3dcam_distance.generic.type = MTYPE_SLIDER;
	s_q2p_3dcam_distance.generic.x = 0;
	s_q2p_3dcam_distance.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_3dcam_distance.generic.name = "CAMERA DISTANCE";
	s_q2p_3dcam_distance.generic.callback = ThirdPersonDistFunc;
	s_q2p_3dcam_distance.minvalue = 1;
	s_q2p_3dcam_distance.maxvalue = 8;
	s_q2p_3dcam_distance.generic.statusbar = "^3Third Person Camera Distance";

	s_q2p_3dcam_angle.generic.type = MTYPE_SLIDER;
	s_q2p_3dcam_angle.generic.x = 0;
	s_q2p_3dcam_angle.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_3dcam_angle.generic.name = "CAMERA ANGLE";
	s_q2p_3dcam_angle.generic.callback = ThirdPersonAngleFunc;
	s_q2p_3dcam_angle.minvalue = 0;
	s_q2p_3dcam_angle.maxvalue = 10;
	s_q2p_3dcam_angle.generic.statusbar = "^3Third Person Camera Angle";

	s_q2p_bobbing.generic.type = MTYPE_SPINCONTROL;
	s_q2p_bobbing.generic.x = 0;
	s_q2p_bobbing.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_bobbing.generic.name = "BOBBING ITEMS & WEAPONS";
	s_q2p_bobbing.generic.callback = BobbingFunc;
	s_q2p_bobbing.itemnames = yesno_names;
	s_q2p_bobbing.generic.statusbar = "^3Bobbing Weapons & Items";

#if defined(PARTICLESYSTEM)
	s_q2p_blaster_color.generic.type = MTYPE_SPINCONTROL;
	s_q2p_blaster_color.generic.x = 0;
	s_q2p_blaster_color.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_blaster_color.generic.name = "BLASTER PARTICLES COLOR";
	s_q2p_blaster_color.generic.callback = BlasterColorFunc;
	s_q2p_blaster_color.itemnames = effect_colors;
	s_q2p_blaster_color.generic.statusbar = "^3Blaster Particles Color";

	s_q2p_blaster_bubble.generic.type = MTYPE_SPINCONTROL;
	s_q2p_blaster_bubble.generic.x = 0;
	s_q2p_blaster_bubble.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blaster_bubble.generic.name = "BLASTER BUBBLES";
	s_q2p_blaster_bubble.generic.callback = BlasterBubbleFunc;
	s_q2p_blaster_bubble.itemnames = bubble_modes;
	s_q2p_blaster_bubble.generic.statusbar = "^3Blaster bubbles particles effect";

	s_q2p_hyperblaster_particles.generic.type = MTYPE_SPINCONTROL;
	s_q2p_hyperblaster_particles.generic.x = 0;
	s_q2p_hyperblaster_particles.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hyperblaster_particles.generic.name = "HYPERBLASTER PARTICLES";
	s_q2p_hyperblaster_particles.generic.callback = HyperBlasterParticlesFunc;
	s_q2p_hyperblaster_particles.itemnames = yesno_names;
	s_q2p_hyperblaster_particles.generic.statusbar = "^3Hyper Blaster particles";

	s_q2p_hyperblaster_color.generic.type = MTYPE_SPINCONTROL;
	s_q2p_hyperblaster_color.generic.x = 0;
	s_q2p_hyperblaster_color.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hyperblaster_color.generic.name = "HYPERBLASTER PARTICLES COLOR";
	s_q2p_hyperblaster_color.generic.callback = HyperBlasterColorFunc;
	s_q2p_hyperblaster_color.itemnames = effect_colors;
	s_q2p_hyperblaster_color.generic.statusbar = "^3Hyper Blaster Particles Color";

	s_q2p_hyperblaster_bubble.generic.type = MTYPE_SPINCONTROL;
	s_q2p_hyperblaster_bubble.generic.x = 0;
	s_q2p_hyperblaster_bubble.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hyperblaster_bubble.generic.name = "HYPERBLASTER BUBBLES";
	s_q2p_hyperblaster_bubble.generic.callback = HyperBlasterBubbleFunc;
	s_q2p_hyperblaster_bubble.itemnames = bubble_modes;
	s_q2p_hyperblaster_bubble.generic.statusbar = "^3Hyperblaster bubbles particles effect";

	s_q2p_particles_type.generic.type = MTYPE_SPINCONTROL;
	s_q2p_particles_type.generic.x = 0;
	s_q2p_particles_type.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_particles_type.generic.name = "PARTICLES TYPE";
	s_q2p_particles_type.generic.callback = ParticlesTypeFunc;
	s_q2p_particles_type.itemnames = particles_type;
	s_q2p_particles_type.generic.statusbar = 
	"^3Particle type effects, Flag trail, Respawn, BFG explosion, Teleport...";

	s_q2p_teleport.generic.type = MTYPE_SPINCONTROL;
	s_q2p_teleport.generic.x = 0;
	s_q2p_teleport.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_teleport.generic.name = "TELEPORT EFFECT";
	s_q2p_teleport.generic.callback = TelePortFunc;
	s_q2p_teleport.itemnames = teleport_effect;
	s_q2p_teleport.generic.statusbar = "^3Teleport particle effects";

	s_q2p_nukeblast.generic.type = MTYPE_SPINCONTROL;
	s_q2p_nukeblast.generic.x = 0;
	s_q2p_nukeblast.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_nukeblast.generic.name = "NUKEBLAST EFFECT";
	s_q2p_nukeblast.generic.callback = NukeBlastFunc;
	s_q2p_nukeblast.itemnames = nukeblast_effect;
	s_q2p_nukeblast.generic.statusbar =  "^3Nuke Blast particle effects";

	s_q2p_railtrail.generic.type = MTYPE_SPINCONTROL;
	s_q2p_railtrail.generic.x = 0;
	s_q2p_railtrail.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_railtrail.generic.name = "RAILGUN RAILTRAIL";
	s_q2p_railtrail.generic.callback = RailTrailFunc;
	s_q2p_railtrail.itemnames = railtrail_effect;
	s_q2p_railtrail.generic.statusbar = "^3Railgun Railtrail";

	s_q2p_railspiral_part.generic.type = MTYPE_SPINCONTROL;
	s_q2p_railspiral_part.generic.x = 0;
	s_q2p_railspiral_part.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_railspiral_part.generic.name = "SPIRAL PARTICLES";
	s_q2p_railspiral_part.generic.callback = RailSpiralPartFunc;
	s_q2p_railspiral_part.itemnames = railspiral_part;
	s_q2p_railspiral_part.generic.statusbar = "^3Railgun Railspiral particles";

	s_q2p_railtrail_red.generic.type = MTYPE_SLIDER;
	s_q2p_railtrail_red.generic.x = 0;
	s_q2p_railtrail_red.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_railtrail_red.generic.name = "RED";
	s_q2p_railtrail_red.generic.callback = RailTrailColorRedFunc;
	s_q2p_railtrail_red.minvalue = 0;
	s_q2p_railtrail_red.maxvalue = 16;
	s_q2p_railtrail_red.generic.statusbar = "^3Railtrail Red";

	s_q2p_railtrail_green.generic.type = MTYPE_SLIDER;
	s_q2p_railtrail_green.generic.x = 0;
	s_q2p_railtrail_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_railtrail_green.generic.name = "GREEN";
	s_q2p_railtrail_green.generic.callback = RailTrailColorGreenFunc;
	s_q2p_railtrail_green.minvalue = 0;
	s_q2p_railtrail_green.maxvalue = 16;
	s_q2p_railtrail_green.generic.statusbar = "^3Railtrail Green";

	s_q2p_railtrail_blue.generic.type = MTYPE_SLIDER;
	s_q2p_railtrail_blue.generic.x = 0;
	s_q2p_railtrail_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_railtrail_blue.generic.name = "BLUE";
	s_q2p_railtrail_blue.generic.callback = RailTrailColorBlueFunc;
	s_q2p_railtrail_blue.minvalue = 0;
	s_q2p_railtrail_blue.maxvalue = 16;
	s_q2p_railtrail_blue.generic.statusbar = "^3Railtrail Blue";

	s_q2p_flame_enh.generic.type = MTYPE_SPINCONTROL;
	s_q2p_flame_enh.generic.x = 0;
	s_q2p_flame_enh.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_flame_enh.generic.name = "FLAMES";
	s_q2p_flame_enh.generic.callback = FlameEnhFunc;
	s_q2p_flame_enh.itemnames = yesno_names;
	s_q2p_flame_enh.generic.statusbar = 
	"^3Flamethrower, used in mods such as dday, awaken, awaken2, lox, ...";

	s_q2p_flame_size.generic.type = MTYPE_SLIDER;
	s_q2p_flame_size.generic.x = 0;
	s_q2p_flame_size.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flame_size.generic.name = "FLAMES SIZE";
	s_q2p_flame_size.generic.callback = FlameEnhSizeFunc;
	s_q2p_flame_size.minvalue = 5;
	s_q2p_flame_size.maxvalue = 35;
	s_q2p_flame_size.generic.statusbar = 
	"^3Flamethrower flame size, note that more value is less size";

	s_q2p_explosions.generic.type = MTYPE_SPINCONTROL;
	s_q2p_explosions.generic.x = 0;
	s_q2p_explosions.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_explosions.generic.name = "EXPLOSIONS";
	s_q2p_explosions.generic.callback = ExplosionsFunc;
	s_q2p_explosions.itemnames = explosion_effect;
	s_q2p_explosions.generic.statusbar = "^3Explosions effects";

	s_q2p_explosions_scale.generic.type = MTYPE_SLIDER;
	s_q2p_explosions_scale.generic.x = 0;
	s_q2p_explosions_scale.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_explosions_scale.generic.name = "EXPLOSIONS SCALE";
	s_q2p_explosions_scale.generic.callback = ExplosionsScaleFunc;
	s_q2p_explosions_scale.minvalue = 0.5;
	s_q2p_explosions_scale.maxvalue = 20;
	s_q2p_explosions_scale.generic.statusbar = "^3Explosions size";

	s_q2p_gun_smoke.generic.type = MTYPE_SPINCONTROL;
	s_q2p_gun_smoke.generic.x = 0;
	s_q2p_gun_smoke.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_gun_smoke.generic.name = "GUN SMOKE";
	s_q2p_gun_smoke.generic.callback = GunSmokeFunc;
	s_q2p_gun_smoke.itemnames = gunsmoke_effect;
	s_q2p_gun_smoke.generic.statusbar = "^3Gun Smoke puff";
#else
	s_q2p_gl_particles.generic.type = MTYPE_SPINCONTROL;
	s_q2p_gl_particles.generic.x = 0;
	s_q2p_gl_particles.generic.y = y+=2*(MENU_FONT_SIZE+2);
	s_q2p_gl_particles.generic.name = "PARTICLE EFFECTS";
	s_q2p_gl_particles.generic.callback = GLParticleFunc;
	s_q2p_gl_particles.itemnames = yesno_names;
	s_q2p_gl_particles.generic.statusbar = "^3Particle Effects";
#endif

	s_q2p_flashlight.generic.type = MTYPE_SPINCONTROL;
	s_q2p_flashlight.generic.x = 0;
	s_q2p_flashlight.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_flashlight.generic.name = "FLASHLIGHT";
	s_q2p_flashlight.generic.callback = FlashLightFunc;
	s_q2p_flashlight.itemnames = yesno_names;
	s_q2p_flashlight.generic.statusbar = "^3Flashlight Enabe/Disable";
	
	s_q2p_flashlight_intensity.generic.type = MTYPE_SLIDER;
	s_q2p_flashlight_intensity.generic.x = 0;
	s_q2p_flashlight_intensity.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flashlight_intensity.generic.name = "FLASHLIGHT INTENSITY";
	s_q2p_flashlight_intensity.generic.callback = FlashLightIntensityFunc;
	s_q2p_flashlight_intensity.minvalue = 5;
	s_q2p_flashlight_intensity.maxvalue = 40;
	s_q2p_flashlight_intensity.generic.statusbar = "^3Flashlight Intensity";

	s_q2p_flashlight_distance.generic.type = MTYPE_SLIDER;
	s_q2p_flashlight_distance.generic.x = 0;
	s_q2p_flashlight_distance.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flashlight_distance.generic.name = "DISTANCE";
	s_q2p_flashlight_distance.generic.callback = FlashLightDistanceFunc;
	s_q2p_flashlight_distance.minvalue = 1;
	s_q2p_flashlight_distance.maxvalue = 150;
	s_q2p_flashlight_distance.generic.statusbar = "^3Maximum effective flashlight distance";

	s_q2p_flashlight_decscale.generic.type = MTYPE_SLIDER;
	s_q2p_flashlight_decscale.generic.x = 0;
	s_q2p_flashlight_decscale.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flashlight_decscale.generic.name = "SCALE";
	s_q2p_flashlight_decscale.generic.callback = FlashLightDecreaseScaleFunc;
	s_q2p_flashlight_decscale.minvalue = 0;
	s_q2p_flashlight_decscale.maxvalue = 20;
	s_q2p_flashlight_decscale.generic.statusbar = "^3How much the intensity decreases with distance";
	
	s_q2p_flashlight_red.generic.type = MTYPE_SLIDER;
	s_q2p_flashlight_red.generic.x = 0;
	s_q2p_flashlight_red.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flashlight_red.generic.name = "RED";
	s_q2p_flashlight_red.generic.callback = FlashLightColorRedFunc;
	s_q2p_flashlight_red.minvalue = 0;
	s_q2p_flashlight_red.maxvalue = 10;
	s_q2p_flashlight_red.generic.statusbar = "^3Flashlight Red";

	s_q2p_flashlight_green.generic.type = MTYPE_SLIDER;
	s_q2p_flashlight_green.generic.x = 0;
	s_q2p_flashlight_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flashlight_green.generic.name = "GREEN";
	s_q2p_flashlight_green.generic.callback = FlashLightColorGreenFunc;
	s_q2p_flashlight_green.minvalue = 0;
	s_q2p_flashlight_green.maxvalue = 10;
	s_q2p_flashlight_green.generic.statusbar = "^3Flashlight Green";

	s_q2p_flashlight_blue.generic.type = MTYPE_SLIDER;
	s_q2p_flashlight_blue.generic.x = 0;
	s_q2p_flashlight_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_flashlight_blue.generic.name = "BLUE";
	s_q2p_flashlight_blue.generic.callback = FlashLightColorBlueFunc;
	s_q2p_flashlight_blue.minvalue = 0;
	s_q2p_flashlight_blue.maxvalue = 10;
	s_q2p_flashlight_blue.generic.statusbar = "^3Flashlight Red";

	ControlsSetMenuItemValues();

	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_client_side);
	
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_3dcam);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_3dcam_distance);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_3dcam_angle);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_bobbing);
#if defined(PARTICLESYSTEM)
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blaster_color);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blaster_bubble);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hyperblaster_particles);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hyperblaster_color);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hyperblaster_bubble);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_particles_type);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_teleport);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_nukeblast);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_railtrail);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_railspiral_part);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_railtrail_red);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_railtrail_green);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_railtrail_blue);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flame_enh);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flame_size);
//	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_explosions);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_explosions_scale);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_gun_smoke);
#else
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_gl_particles);
#endif
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight_intensity);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight_distance);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight_decscale);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight_red);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight_green);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_flashlight_blue);
}


void
Q2P_MenuInit_Graphic(void)
{
	int y = 0;
	
	static const char *yesno_names[] = {
		"Disabled",
		"Enabled ",
		NULL
	};

	static const char *shadows_names[] = {
		"Disabled",
		"Basic   ",
		"Stencil ",
		NULL
	};

	static const char *minimap_names[] = {
		"Disabled     ",
		"Star entities",
		"Dot entities ",
		NULL
	};

	static const char *minimap_style_names[] = {
		"Rotating",
		"Fixed   ",
		NULL
	};

	static const char *blur_names[] = {
		"Disabled        ",
		"Underwater only ",
		"Global          ",
		NULL
	};

	s_q2p_menu.x = viddef.width * 0.50;
	s_q2p_menu.y = viddef.height * 0.50 - ScaledVideo(220);
	s_q2p_menu.nitems = 0;

	s_q2p_graphic.generic.type = MTYPE_SEPARATOR;
	s_q2p_graphic.generic.name = "GRAPHIC OPTIONS";
	s_q2p_graphic.generic.x = 50;
	s_q2p_graphic.generic.y = y;

	s_q2p_gl_reflection.generic.type = MTYPE_SLIDER;
	s_q2p_gl_reflection.generic.x = 0;
	s_q2p_gl_reflection.generic.y = y+=2*(MENU_FONT_SIZE+2);
	s_q2p_gl_reflection.generic.name = "WATER REFLECTIONS";
	s_q2p_gl_reflection.generic.callback = WaterReflection;
	s_q2p_gl_reflection.minvalue = 0;
	s_q2p_gl_reflection.maxvalue = 10;
	s_q2p_gl_reflection.generic.statusbar = "^3Water reflections";

	s_q2p_gl_reflection_shader.generic.type = MTYPE_SPINCONTROL;
	s_q2p_gl_reflection_shader.generic.x = 0;
	s_q2p_gl_reflection_shader.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_gl_reflection_shader.generic.name = "REFRACTIONS";
	s_q2p_gl_reflection_shader.generic.callback = WaterShaderReflection;
	s_q2p_gl_reflection_shader.itemnames = yesno_names;
	s_q2p_gl_reflection_shader.generic.statusbar = "^3Water shader refractions";

	s_q2p_waves.generic.type = MTYPE_SLIDER;
	s_q2p_waves.generic.x = 0;
	s_q2p_waves.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_waves.generic.name = "WAVES";
	s_q2p_waves.generic.callback = WaterWavesFunc;
	s_q2p_waves.minvalue = 0;
	s_q2p_waves.maxvalue = 10;
	s_q2p_waves.generic.statusbar = "Water Waves Level";

	s_q2p_caustics.generic.type = MTYPE_SPINCONTROL;
	s_q2p_caustics.generic.x = 0;
	s_q2p_caustics.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_caustics.generic.name = "WATER CAUSTICS";
	s_q2p_caustics.generic.callback = TexCausticFunc;
	s_q2p_caustics.itemnames = yesno_names;
	s_q2p_caustics.generic.statusbar = "^3Water Caustics";

	s_q2p_underwater_mov.generic.type = MTYPE_SPINCONTROL;
	s_q2p_underwater_mov.generic.x = 0;
	s_q2p_underwater_mov.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_underwater_mov.generic.name = "UNDER WATER MOVEMENT";
	s_q2p_underwater_mov.generic.callback = UnderWaterMovFunc;
	s_q2p_underwater_mov.itemnames = yesno_names;
	s_q2p_underwater_mov.generic.statusbar = "^3Under water movement, ala Quake III";

	s_q2p_underwater_view.generic.type = MTYPE_SPINCONTROL;
	s_q2p_underwater_view.generic.x = 0;
	s_q2p_underwater_view.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_underwater_view.generic.name = "UNDER WATER EFFECT";
	s_q2p_underwater_view.generic.callback = UnderWaterViewFunc;
	s_q2p_underwater_view.itemnames = yesno_names;
	s_q2p_underwater_view.generic.statusbar = "^3Under water effect";

	s_q2p_water_trans.generic.type = MTYPE_SPINCONTROL;
	s_q2p_water_trans.generic.x = 0;
	s_q2p_water_trans.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_water_trans.generic.name = "UNDER WATER TRANSPARENCY";
	s_q2p_water_trans.generic.callback = TransWaterFunc;
	s_q2p_water_trans.itemnames = yesno_names;
	s_q2p_water_trans.generic.statusbar = "^3Water transparency.";

	s_q2p_fog.generic.type = MTYPE_SPINCONTROL;
	s_q2p_fog.generic.x = 0;
	s_q2p_fog.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_fog.generic.name = "GLOBAL FOG";
	s_q2p_fog.generic.callback = FogFunc;
	s_q2p_fog.itemnames = yesno_names;
	s_q2p_fog.generic.statusbar = "^3Fog Ambience";

	s_q2p_fog_density.generic.type = MTYPE_SLIDER;
	s_q2p_fog_density.generic.x = 0;
	s_q2p_fog_density.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_fog_density.generic.name = "FOG DENSITY";
	s_q2p_fog_density.generic.callback = FogDensityFunc;
	s_q2p_fog_density.generic.statusbar = "^3Fog density";
	s_q2p_fog_density.minvalue = 1;
	s_q2p_fog_density.maxvalue = 40;

	s_q2p_fog_red.generic.type = MTYPE_SLIDER;
	s_q2p_fog_red.generic.x = 0;
	s_q2p_fog_red.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_fog_red.generic.name = "RED";
	s_q2p_fog_red.generic.callback = FogRedFunc;
	s_q2p_fog_red.minvalue = 0;
	s_q2p_fog_red.maxvalue = 10;
	s_q2p_fog_red.generic.statusbar = "^3Fog Red";

	s_q2p_fog_green.generic.type = MTYPE_SLIDER;
	s_q2p_fog_green.generic.x = 0;
	s_q2p_fog_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_fog_green.generic.name = "GREEN";
	s_q2p_fog_green.generic.callback = FogGreenFunc;
	s_q2p_fog_green.minvalue = 0;
	s_q2p_fog_green.maxvalue = 10;
	s_q2p_fog_green.generic.statusbar = "^3Fog Green";

	s_q2p_fog_blue.generic.type = MTYPE_SLIDER;
	s_q2p_fog_blue.generic.x = 0;
	s_q2p_fog_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_fog_blue.generic.name = "BLUE";
	s_q2p_fog_blue.generic.callback = FogBlueFunc;
	s_q2p_fog_blue.minvalue = 0;
	s_q2p_fog_blue.maxvalue = 10;
	s_q2p_fog_blue.generic.statusbar = "^3Fog Blue";
	
	s_q2p_fog_underwater.generic.type = MTYPE_SPINCONTROL;
	s_q2p_fog_underwater.generic.x = 0;
	s_q2p_fog_underwater.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_fog_underwater.generic.name = "UNDER WATER FOG";
	s_q2p_fog_underwater.generic.callback = FogUnderWaterFunc;
	s_q2p_fog_underwater.itemnames = yesno_names;
	s_q2p_fog_underwater.generic.statusbar = "^3Under Water Fog";

	s_q2p_minimap.generic.type = MTYPE_SPINCONTROL;
	s_q2p_minimap.generic.x = 0;
	s_q2p_minimap.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_minimap.generic.name = "RADAR";
	s_q2p_minimap.generic.callback = MiniMapFunc;
	s_q2p_minimap.itemnames = minimap_names;
	s_q2p_minimap.generic.statusbar = "^3Mini Map.";

	s_q2p_minimap_style.generic.type = MTYPE_SPINCONTROL;
	s_q2p_minimap_style.generic.x = 0;
	s_q2p_minimap_style.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_minimap_style.generic.name = "RADAR STYLE";
	s_q2p_minimap_style.generic.callback = MiniMapStyleFunc;
	s_q2p_minimap_style.itemnames = minimap_style_names;
	s_q2p_minimap_style.generic.statusbar = "^3Mini Map Style";

	s_q2p_minimap_size.generic.type = MTYPE_SLIDER;
	s_q2p_minimap_size.generic.x = 0;
	s_q2p_minimap_size.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_minimap_size.generic.name = "RADAR SIZE";
	s_q2p_minimap_size.generic.callback = MiniMapSizeFunc;
	s_q2p_minimap_size.minvalue = 200;
	s_q2p_minimap_size.maxvalue = 600;
	s_q2p_minimap_size.generic.statusbar = "^3Mini Map Size";

	s_q2p_minimap_zoom.generic.type = MTYPE_SLIDER;
	s_q2p_minimap_zoom.generic.x = 0;
	s_q2p_minimap_zoom.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_minimap_zoom.generic.name = "RADAR ZOOM";
	s_q2p_minimap_zoom.generic.callback = MiniMapZoomFunc;
	s_q2p_minimap_zoom.minvalue = 0;
	s_q2p_minimap_zoom.maxvalue = 20;
	s_q2p_minimap_zoom.generic.statusbar = "^3Mini Map Zoom";

	s_q2p_minimap_x.generic.type = MTYPE_SLIDER;
	s_q2p_minimap_x.generic.x = 0;
	s_q2p_minimap_x.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_minimap_x.generic.name = "X POSITION";
	s_q2p_minimap_x.generic.callback = MiniMapXPosFunc;
	s_q2p_minimap_x.minvalue = 200;
	s_q2p_minimap_x.maxvalue = 2048;
	s_q2p_minimap_x.generic.statusbar = "^3Mini Map X position on the screen";

	s_q2p_minimap_y.generic.type = MTYPE_SLIDER;
	s_q2p_minimap_y.generic.x = 0;
	s_q2p_minimap_y.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_minimap_y.generic.name = "Y POSITION";
	s_q2p_minimap_y.generic.callback = MiniMapYPosFunc;
	s_q2p_minimap_y.minvalue = 200;
	s_q2p_minimap_y.maxvalue = 1600;
	s_q2p_minimap_y.generic.statusbar = "^3Mini Map Y position on the screen";

	s_q2p_blooms.generic.type = MTYPE_SPINCONTROL;
	s_q2p_blooms.generic.x = 0;
	s_q2p_blooms.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_blooms.generic.name = "BLOOM";
	s_q2p_blooms.generic.callback = BloomsFunc;
	s_q2p_blooms.itemnames = yesno_names;
	s_q2p_blooms.generic.statusbar = "^3Blooms effects";

	s_q2p_blooms_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_blooms_alpha.generic.x = 0;
	s_q2p_blooms_alpha.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blooms_alpha.generic.name = "BLOOM ALPHA";
	s_q2p_blooms_alpha.generic.callback = BloomsAlphaFunc;
	s_q2p_blooms_alpha.generic.statusbar = "^3Blooms alpha";
	s_q2p_blooms_alpha.minvalue = 0;
	s_q2p_blooms_alpha.maxvalue = 10;

	s_q2p_blooms_diamond_size.generic.type = MTYPE_SLIDER;
	s_q2p_blooms_diamond_size.generic.x = 0;
	s_q2p_blooms_diamond_size.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blooms_diamond_size.generic.name = "BLOOM DIAMONDS SIZE";
	s_q2p_blooms_diamond_size.generic.callback = BloomsDiamondSizeFunc;
	s_q2p_blooms_diamond_size.generic.statusbar = "^3Blooms diamond size";
	s_q2p_blooms_diamond_size.minvalue = 2;
	s_q2p_blooms_diamond_size.maxvalue = 4;

	s_q2p_blooms_intensity.generic.type = MTYPE_SLIDER;
	s_q2p_blooms_intensity.generic.x = 0;
	s_q2p_blooms_intensity.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blooms_intensity.generic.name = "BLOOM INTENSITY";
	s_q2p_blooms_intensity.generic.callback = BloomsIntensityFunc;
	s_q2p_blooms_intensity.generic.statusbar = "^3Blooms intensity";
	s_q2p_blooms_intensity.minvalue = 0;
	s_q2p_blooms_intensity.maxvalue = 10;

	s_q2p_blooms_darken.generic.type = MTYPE_SLIDER;
	s_q2p_blooms_darken.generic.x = 0;
	s_q2p_blooms_darken.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blooms_darken.generic.name = "BLOMM DARKEN";
	s_q2p_blooms_darken.generic.callback = BloomsDarkenFunc;
	s_q2p_blooms_darken.generic.statusbar = "^3Blooms darken";
	s_q2p_blooms_darken.minvalue = 0;
	s_q2p_blooms_darken.maxvalue = 40;

	s_q2p_blooms_sample_size.generic.type = MTYPE_SLIDER;
	s_q2p_blooms_sample_size.generic.x = 0;
	s_q2p_blooms_sample_size.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blooms_sample_size.generic.name = "BLOOM SAMPLE SIZE";
	s_q2p_blooms_sample_size.generic.callback = BloomsSampleSizeFunc;
	s_q2p_blooms_sample_size.generic.statusbar =
	"^3Blooms sample size, scale is logarithmic instead of linear. Needs vid_restart.";
	s_q2p_blooms_sample_size.minvalue = 5;
	s_q2p_blooms_sample_size.maxvalue = 11;

	s_q2p_blooms_fast_sample.generic.type = MTYPE_SPINCONTROL;
	s_q2p_blooms_fast_sample.generic.x = 0;
	s_q2p_blooms_fast_sample.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_blooms_fast_sample.generic.name = "BLOOM FAST SAMPLE";
	s_q2p_blooms_fast_sample.generic.callback = BloomsFastSampleFunc;
	s_q2p_blooms_fast_sample.itemnames = yesno_names;
	s_q2p_blooms_fast_sample.generic.statusbar = "^3Blooms fast sample";

	s_q2p_motionblur.generic.type = MTYPE_SPINCONTROL;
	s_q2p_motionblur.generic.x = 0;
	s_q2p_motionblur.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_motionblur.generic.name = "MOTION BLUR";
	s_q2p_motionblur.generic.callback = MotionBlurFunc;
	s_q2p_motionblur.itemnames = blur_names;
	s_q2p_motionblur.generic.statusbar = "^3Motion Blur";

	s_q2p_motionblur_intens.generic.type = MTYPE_SLIDER;
	s_q2p_motionblur_intens.generic.x = 0;
	s_q2p_motionblur_intens.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_motionblur_intens.generic.name = "MOTION BLUR INTENSITY";
	s_q2p_motionblur_intens.generic.callback = MotionBlurIntensFunc;
	s_q2p_motionblur_intens.minvalue = 0;
	s_q2p_motionblur_intens.maxvalue = 10;
	s_q2p_motionblur_intens.generic.statusbar = "^3Motion Blur Intensity";

	s_q2p_lensflare.generic.type = MTYPE_SPINCONTROL;
	s_q2p_lensflare.generic.x = 0;
	s_q2p_lensflare.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_lensflare.generic.name = "LENS FLARES";
	s_q2p_lensflare.generic.callback = LensFlareFunc;
	s_q2p_lensflare.itemnames = yesno_names;
	s_q2p_lensflare.generic.statusbar = "^3Lens Flares";

	s_q2p_lensflare_intens.generic.type = MTYPE_SLIDER;
	s_q2p_lensflare_intens.generic.x = 0;
	s_q2p_lensflare_intens.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_lensflare_intens.generic.name = "LENS FLARES INTENSITY";
	s_q2p_lensflare_intens.generic.callback = LensFlareIntensFunc;
	s_q2p_lensflare_intens.minvalue = 0;
	s_q2p_lensflare_intens.maxvalue = 20;
	s_q2p_lensflare_intens.generic.statusbar = "^3Lens Flares Intensity";

	s_q2p_det_tex.generic.type = MTYPE_SLIDER;
	s_q2p_det_tex.generic.x = 0;
	s_q2p_det_tex.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_det_tex.generic.name = "DETAILED TEXTURES";
	s_q2p_det_tex.generic.callback = DetTexFunc;
	s_q2p_det_tex.minvalue = 0;
	s_q2p_det_tex.maxvalue = 20;
	s_q2p_det_tex.generic.statusbar = "^3Detailed textures value";

	s_q2p_shadows.generic.type = MTYPE_SPINCONTROL;
	s_q2p_shadows.generic.x = 0;
	s_q2p_shadows.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_shadows.generic.name = "SHADOWS";
	s_q2p_shadows.generic.callback = ShadowsFunc;
	s_q2p_shadows.itemnames = shadows_names;
	s_q2p_shadows.generic.statusbar = "^3Shadows";

#if defined(PARTICLESYSTEM)
	s_q2p_r_decals.generic.type = MTYPE_FIELD;
	s_q2p_r_decals.generic.flags = QMF_NUMBERSONLY;
	s_q2p_r_decals.generic.x = 0;
	s_q2p_r_decals.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_r_decals.generic.name = "BLOOD & EXPLOSIONS DECALS";
	s_q2p_r_decals.length = 4;
	s_q2p_r_decals.visible_length =4;
	strcpy(s_q2p_r_decals.buffer, Cvar_VariableString("r_decals"));
	s_q2p_r_decals.generic.statusbar =
	"^3Type the desired decals in the box, ESCAPE will set the new value.";
#else
	s_q2p_r_decals.generic.type = MTYPE_SPINCONTROL;
	s_q2p_r_decals.generic.x = 0;
	s_q2p_r_decals.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_r_decals.generic.name = "BLOOD & EXPLOSIONS DECALS";
	s_q2p_r_decals.generic.callback = DecalsFunc;
	s_q2p_r_decals.itemnames = yesno_names;
	s_q2p_r_decals.generic.statusbar = "^3Blood & Explosion Decals";
#endif
	s_q2p_cellshading.generic.type = MTYPE_SPINCONTROL;
	s_q2p_cellshading.generic.x = 0;
#if defined(PARTICLESYSTEM)
	s_q2p_cellshading.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
#else
	s_q2p_cellshading.generic.y = y+=(MENU_FONT_SIZE+2);
#endif
	s_q2p_cellshading.generic.name = "CELL SHADING";
	s_q2p_cellshading.generic.callback = CellShadingFunc;
	s_q2p_cellshading.itemnames = yesno_names;
	s_q2p_cellshading.generic.statusbar = "^3Cell Shading";
	
	ControlsSetMenuItemValues();

	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_graphic);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_gl_reflection);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_gl_reflection_shader);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_waves);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_caustics);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_underwater_mov);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_underwater_view);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_water_trans);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fog);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fog_density);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fog_red);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fog_green);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fog_blue);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fog_underwater);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_minimap);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_minimap_style);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_minimap_size);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_minimap_zoom);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_minimap_x);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_minimap_y);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms_alpha);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms_diamond_size);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms_intensity);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms_darken);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms_sample_size);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_blooms_fast_sample);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_motionblur);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_motionblur_intens);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_lensflare);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_lensflare_intens);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_det_tex);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_shadows);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_r_decals);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cellshading);
}

void SetCrosshairCursor (void)
{
	int i;
	s_q2p_crosshair_box.curvalue = 0;

	if (numcrosshairs > 1)
		for (i=0; crosshair_names[i]; i++) {
			if (!Q_strcasecmp(va("ch%i", (int)crosshair->value), crosshair_names[i])) {
				s_q2p_crosshair_box.curvalue = i;
				return;
			}
		}
}

qboolean crosshairInList (char *check, int num, char **list)
{
	int i;
	for (i=0; i<num; i++)
		if (!Q_strcasecmp(check, list[i]))
			return true;
	return false;
}

void insertCrosshair (char ** list, char *insert, int len )
{
	int i;
	if (!list) return;

	//i=1 so none stays first!
	for (i=1; i<len; i++)
	{
		if (!list[i])
		{
			list[i] = strdup(insert);
			return;
		}
	}
	list[len] = strdup(insert);
}

void sortCrosshairs (char ** list, int len )
{
	int i, j;
	char *temp;
	qboolean moved;

	if (!list || len < 2)
		return;

	for (i=(len-1); i>0; i--) {
		moved = false;
		for (j=0; j<i; j++) {
			if (!list[j]) break;
			if ( atoi(strdup(list[j]+2)) > atoi(strdup(list[j+1]+2)) ) {
				temp = list[j];
				list[j] = list[j+1];
				list[j+1] = temp;
				moved = true;
			}
		}
		if (!moved) break; //done sorting
	}
}

qboolean isNumeric (char ch)
{
	if (ch >= '0' && ch <= '9')
		return true;
	else
		return false;
}

char **SetCrosshairNames (void)
{
	char *curCrosshair;
	char **list = 0, *p;
	char findname[1024];
	int ncrosshairs = 0, ncrosshairnames;
	char **crosshairfiles;
	char *path = NULL;
	int i;

	list = Q_malloc( sizeof( char * ) * MAX_CROSSHAIRS+1 );
	memset( list, 0, sizeof( char * ) * MAX_CROSSHAIRS+1 );

	list[0] = strdup("none"); //was default
	ncrosshairnames = 1;

	path = FS_NextPath( path );
	while (path) {
		Com_sprintf( findname, sizeof(findname), "%s/pics/ch*.*", path );
		crosshairfiles = FS_ListFiles( findname, &ncrosshairs, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		for (i=0; i < ncrosshairs && ncrosshairnames < MAX_CROSSHAIRS; i++) {
			int num, namelen;

			if (!crosshairfiles || !crosshairfiles[i])
				continue;

			p = strstr(crosshairfiles[i], "/pics/"); p++;
			p = strstr(p, "/"); p++;

			if (!strstr(p, ".tga") &&
			    !strstr(p, ".png") &&
			    !strstr(p, ".jpg") &&
			    !strstr(p, ".pcx"))
				continue;

			// filename must be chxxx
			if (strncmp(p, "ch", 2)) 
				continue;
			namelen = strlen(strdup(p));
			if (namelen < 7 || namelen > 9)
				continue;
			if (!isNumeric(p[2]))
				continue;
			if (namelen >= 8 && !isNumeric(p[3]))
				continue;
			// ch100 is only valid 5-char name
			if (namelen == 9 && (p[2] != '1' || p[3] != '0' || p[4] != '0'))
				continue;

			num = strlen(p)-4;
			p[num] = 0; //NULL;

			curCrosshair = p;

			if (!fontInList(curCrosshair, ncrosshairnames, list)) {
				insertCrosshair(list, strdup(curCrosshair), ncrosshairnames);
				ncrosshairnames++;
			}
			
			//set back so whole string get deleted.
			p[num] = '.';
		}
		if (ncrosshairs)
			FreeFileList( crosshairfiles, ncrosshairs );
		
		path = FS_NextPath( path );
	}

	//check pak after
	if ((crosshairfiles = FS_ListPak("pics/", &ncrosshairs))) {
		for (i=0; i<ncrosshairs && ncrosshairnames < MAX_CROSSHAIRS; i++) {
			int num, namelen;

			if (!crosshairfiles || !crosshairfiles[i])
				continue;

			p = strstr(crosshairfiles[i], "/"); p++;

			if (!strstr(p, ".tga") &&
			    !strstr(p, ".png") &&
			    !strstr(p, ".jpg") &&
			    !strstr(p, ".pcx"))
				continue;

			// filename must be chxxx
			if (strncmp(p, "ch", 2))
				continue;
			namelen = strlen(strdup(p));
			if (namelen < 7 || namelen > 9)
				continue;
			if (!isNumeric(p[2]))
				continue;
			if (namelen >= 8 && !isNumeric(p[3]))
				continue;
			// ch100 is only valid 5-char name
			if (namelen == 9 && (p[2] != '1' || p[3] != '0' || p[4] != '0'))
				continue;

			num = strlen(p)-4;
			p[num] = 0; //NULL;

			curCrosshair = p;

			if (!crosshairInList(curCrosshair, ncrosshairnames, list)) {
				insertCrosshair(list, strdup(curCrosshair), ncrosshairnames);
				ncrosshairnames++;
			}
			
			//set back so whole string get deleted.
			p[num] = '.';
		}
	}
	// sort the list
	sortCrosshairs (list, ncrosshairnames);

	if (ncrosshairs)
		FreeFileList( crosshairfiles, ncrosshairs );

	numcrosshairs = ncrosshairnames;

	return list;		
}

void DrawMenuCrosshair (void)
{

	if (s_q2p_crosshair_box.curvalue < 1)
		return;

	re.DrawStretchPic(viddef.width/2 - ScaledVideo(32)/2 + ScaledVideo(140),
		          s_q2p_menu.y + ScaledVideo(215),
		          ScaledVideo(32), ScaledVideo(32), crosshair_names[s_q2p_crosshair_box.curvalue], 1.0f);
}

void
Q2P_MenuInit_Hud(void)
{
	int y = 0;
	
	static const char *yesno_names[] = {
		"Disabled",
		"Enabled ",
		NULL
	};

	static const char *clock_names[] = {
		"Disabled   ",
		"24 H Format",
		"12 H Format",
		NULL
	};

	static const char *netgraph_names[] = {
		"Disabled     ",
		"Original     ",
		"Box          ",
		"Box plus Info",
		NULL
	};

	static const char *netgraph_pos_names[] = {
		"Custom      ",
		"Bottom Right",
		"Bottom Left ",
		"Top Right   ",
		"Top Left    ",
		NULL
	};
	
	static const char *hud_scale[] =
	{
		"320x240  [2]",
		"400x300  [3]",
		"640x480  [4]",
		"800x600  [5]",
		"1024x768 [6]",
		NULL
	};
	
	s_q2p_menu.x = viddef.width * 0.50;
	s_q2p_menu.y = viddef.height * 0.50 - ScaledVideo(220);
	s_q2p_menu.nitems = 0;

	s_q2p_hud.generic.type = MTYPE_SEPARATOR;
	s_q2p_hud.generic.name = "Hud Options";
	s_q2p_hud.generic.x = 50;
	s_q2p_hud.generic.y = y;

	s_q2p_drawfps.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawfps.generic.x = 0;
	s_q2p_drawfps.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_drawfps.generic.name = "FPS";
	s_q2p_drawfps.generic.callback = DrawFPSFunc;
	s_q2p_drawfps.itemnames = yesno_names;
	s_q2p_drawfps.generic.statusbar = "^3Draw frames-per-second on the hud";

	s_q2p_option_maxfps.generic.type = MTYPE_FIELD;
	s_q2p_option_maxfps.generic.name = "Max FPS";
	s_q2p_option_maxfps.generic.flags = QMF_NUMBERSONLY;
	s_q2p_option_maxfps.generic.x = 0;
	s_q2p_option_maxfps.generic.y = y+=1.2*(MENU_FONT_SIZE+2);
	s_q2p_option_maxfps.length = 4;
	s_q2p_option_maxfps.visible_length = 4;
	Q_strncpyz(s_q2p_option_maxfps.buffer, Cvar_VariableString("cl_maxfps"), sizeof(s_q2p_option_maxfps.buffer));
	s_q2p_option_maxfps.generic.statusbar =
	"^3Type the maximum desired for FPS in the box, ESCAPE will set the new value.";

	s_q2p_drawclock.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawclock.generic.x = 0;
	s_q2p_drawclock.generic.y = y+=1.2*(MENU_FONT_SIZE+2);
	s_q2p_drawclock.generic.name = "CLOCK";
	s_q2p_drawclock.generic.callback = DrawClockFunc;
	s_q2p_drawclock.itemnames = clock_names;
	s_q2p_drawclock.generic.statusbar =
	"^3Draw a clock in the screen, type cl_drawclock 2 for 12/24 format";

	s_q2p_drawdate.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawdate.generic.x = 0;
	s_q2p_drawdate.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawdate.generic.name = "DATE";
	s_q2p_drawdate.generic.callback = DrawDateFunc;
	s_q2p_drawdate.itemnames = yesno_names;
	s_q2p_drawdate.generic.statusbar = "^3Draw the date in the screen";

	s_q2p_drawmaptime.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawmaptime.generic.x = 0;
	s_q2p_drawmaptime.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawmaptime.generic.name = "INGAME TIME";
	s_q2p_drawmaptime.generic.callback = DrawMapTimeFunc;
	s_q2p_drawmaptime.itemnames = yesno_names;
	s_q2p_drawmaptime.generic.statusbar = "^3Draw a clock playing the game";

	s_q2p_drawping.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawping.generic.x = 0;
	s_q2p_drawping.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawping.generic.name = "PING";
	s_q2p_drawping.generic.callback = DrawPingFunc;
	s_q2p_drawping.itemnames = yesno_names;
	s_q2p_drawping.generic.statusbar = "^3Draw ping on the hud";

	s_q2p_drawrate.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawrate.generic.x = 0;
	s_q2p_drawrate.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawrate.generic.name = "NET RATE";
	s_q2p_drawrate.generic.callback = DrawRateFunc;
	s_q2p_drawrate.itemnames = yesno_names;
	s_q2p_drawrate.generic.statusbar = "^3Draw upload/download rate on the hud";

	s_q2p_drawchathud.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawchathud.generic.x = 0;
	s_q2p_drawchathud.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawchathud.generic.name = "CHAT MESSAGES";
	s_q2p_drawchathud.generic.callback = DrawChatHudFunc;
	s_q2p_drawchathud.itemnames = yesno_names;
	s_q2p_drawchathud.generic.statusbar = "^3Keep the chat messages on the hud, 4 lines";

	s_q2p_draw_x.generic.type = MTYPE_SLIDER;
	s_q2p_draw_x.generic.x = 0;
	s_q2p_draw_x.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_draw_x.generic.name = "X POSITION";
	s_q2p_draw_x.generic.callback = UpdateHudXFunc;
	s_q2p_draw_x.minvalue = 0;
	s_q2p_draw_x.maxvalue = 20;
	s_q2p_draw_x.generic.statusbar = "^3Percentage down screen for hud";

	s_q2p_draw_y.generic.type = MTYPE_SLIDER;
	s_q2p_draw_y.generic.x = 0;
	s_q2p_draw_y.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_draw_y.generic.name = "Y POSITION";
	s_q2p_draw_y.generic.callback = UpdateHudYFunc;
	s_q2p_draw_y.minvalue = 0;
	s_q2p_draw_y.maxvalue = 20;
	s_q2p_draw_y.generic.statusbar = "^3Percentage across screen for hud";

	s_q2p_drawnetgraph.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawnetgraph.generic.x = 0;
	s_q2p_drawnetgraph.generic.y = y+=1.2*(MENU_FONT_SIZE+2);
	s_q2p_drawnetgraph.generic.name = "NETGRAPH";
	s_q2p_drawnetgraph.generic.callback = DrawNetGraphFunc;
	s_q2p_drawnetgraph.itemnames = netgraph_names;
	s_q2p_drawnetgraph.generic.statusbar =
	"^3Draw net graphic in mode basic or showing extra info.";

	s_q2p_drawnetgraph_pos.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawnetgraph_pos.generic.x = 0;
	s_q2p_drawnetgraph_pos.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawnetgraph_pos.generic.name = "NETGRAPH POSITION";
	s_q2p_drawnetgraph_pos.generic.callback = DrawNetGraphPosFunc;
	s_q2p_drawnetgraph_pos.itemnames = netgraph_pos_names;
	s_q2p_drawnetgraph_pos.generic.statusbar = "^3Net graphic hud location";

	s_q2p_drawnetgraph_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_drawnetgraph_alpha.generic.x = 0;
	s_q2p_drawnetgraph_alpha.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawnetgraph_alpha.generic.name = "NETGRAPH ALPHA";
	s_q2p_drawnetgraph_alpha.generic.callback = DrawNetGraphAlphaFunc;
	s_q2p_drawnetgraph_alpha.minvalue = 0;
	s_q2p_drawnetgraph_alpha.maxvalue = 10;
	s_q2p_drawnetgraph_alpha.generic.statusbar = "^3Net graphic Alpha";

	s_q2p_drawnetgraph_scale.generic.type = MTYPE_SLIDER;
	s_q2p_drawnetgraph_scale.generic.x = 0;
	s_q2p_drawnetgraph_scale.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawnetgraph_scale.generic.name = "NETGRAPH SCALE";
	s_q2p_drawnetgraph_scale.generic.callback = DrawNetGraphScaleFunc;
	s_q2p_drawnetgraph_scale.minvalue = 110;
	s_q2p_drawnetgraph_scale.maxvalue = 200;
	s_q2p_drawnetgraph_scale.generic.statusbar = "^3Net graphic Scale";

	s_q2p_netgraph_image.generic.type = MTYPE_FIELD;
	s_q2p_netgraph_image.generic.name = "NETGRAPH IMAGE";
	s_q2p_netgraph_image.generic.callback = 0;
	s_q2p_netgraph_image.generic.x = 0;
	s_q2p_netgraph_image.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_netgraph_image.length = 20;
	s_q2p_netgraph_image.visible_length = 10;
	Q_strncpyz(s_q2p_netgraph_image.buffer, scr_netgraph_image->string, sizeof(s_q2p_netgraph_image.buffer));
	s_q2p_netgraph_image.cursor = strlen(scr_netgraph_image->string);
	s_q2p_netgraph_image.generic.statusbar =
	"^3Type the name for the netgraph image in the box, ESCAPE will set the new value.";

	s_q2p_drawnetgraph_pos_x.generic.type = MTYPE_SLIDER;
	s_q2p_drawnetgraph_pos_x.generic.x = 0;
	s_q2p_drawnetgraph_pos_x.generic.y = y+=1.2*(MENU_FONT_SIZE+2);
	s_q2p_drawnetgraph_pos_x.generic.name = "X POSITION";
	s_q2p_drawnetgraph_pos_x.generic.callback = DrawNetGraphPosXFunc;
	s_q2p_drawnetgraph_pos_x.minvalue = 0;
	s_q2p_drawnetgraph_pos_x.maxvalue = 2048;
	s_q2p_drawnetgraph_pos_x.generic.statusbar = 
	"^3Net graphic X position on the hud, only on custom place.";

	s_q2p_drawnetgraph_pos_y.generic.type = MTYPE_SLIDER;
	s_q2p_drawnetgraph_pos_y.generic.x = 0;
	s_q2p_drawnetgraph_pos_y.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_drawnetgraph_pos_y.generic.name = "Y POSITION";
	s_q2p_drawnetgraph_pos_y.generic.callback = DrawNetGraphPosYFunc;
	s_q2p_drawnetgraph_pos_y.minvalue = 0;
	s_q2p_drawnetgraph_pos_y.maxvalue = 1600;
	s_q2p_drawnetgraph_pos_y.generic.statusbar = 
	"^3Net graphic Y position on the hud, only on custom place.";

	s_q2p_drawplayername.generic.type = MTYPE_SPINCONTROL;
	s_q2p_drawplayername.generic.x = 0;
	s_q2p_drawplayername.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_drawplayername.generic.name = "DRAW PLAYERS NAME";
	s_q2p_drawplayername.generic.callback = DrawPlayerNameFunc;
	s_q2p_drawplayername.itemnames = yesno_names;
	s_q2p_drawplayername.generic.statusbar = "^3Draw the player name over the crosshair";

	s_q2p_zoom_sniper.generic.type = MTYPE_SPINCONTROL;
	s_q2p_zoom_sniper.generic.x = 0;
	s_q2p_zoom_sniper.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_zoom_sniper.generic.name = "DRAW ZOOM SNIPER";
	s_q2p_zoom_sniper.generic.callback = DrawZoomSniperFunc;
	s_q2p_zoom_sniper.itemnames = yesno_names;
	s_q2p_zoom_sniper.generic.statusbar = "^3Draw a zoom";

	crosshair_names = SetCrosshairNames ();
	s_q2p_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_q2p_crosshair_box.generic.x = 0;
	s_q2p_crosshair_box.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_crosshair_box.generic.name = "CROSSHAIR";
	s_q2p_crosshair_box.generic.callback = CrosshairFunc;
	s_q2p_crosshair_box.itemnames = (const char **)crosshair_names;

	s_q2p_cross_scale.generic.type = MTYPE_SLIDER;
	s_q2p_cross_scale.generic.x = 0;
	s_q2p_cross_scale.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_cross_scale.generic.name = "SIZE";
	s_q2p_cross_scale.generic.callback = CrossScaleFunc;
	s_q2p_cross_scale.minvalue = 3;
	s_q2p_cross_scale.maxvalue = 10;

	s_q2p_cross_red.generic.type = MTYPE_SLIDER;
	s_q2p_cross_red.generic.x = 0;
	s_q2p_cross_red.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_cross_red.generic.name = "RED";
	s_q2p_cross_red.generic.callback = CrossColorRedFunc;
	s_q2p_cross_red.minvalue = 0;
	s_q2p_cross_red.maxvalue = 10;

	s_q2p_cross_green.generic.type = MTYPE_SLIDER;
	s_q2p_cross_green.generic.x = 0;
	s_q2p_cross_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_cross_green.generic.name = "GREEN";
	s_q2p_cross_green.generic.callback = CrossColorGreenFunc;
	s_q2p_cross_green.minvalue = 0;
	s_q2p_cross_green.maxvalue = 10;

	s_q2p_cross_blue.generic.type = MTYPE_SLIDER;
	s_q2p_cross_blue.generic.x = 0;
	s_q2p_cross_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_cross_blue.generic.name = "BLUE";
	s_q2p_cross_blue.generic.callback = CrossColorBlueFunc;
	s_q2p_cross_blue.minvalue = 0;
	s_q2p_cross_blue.maxvalue = 10;

	s_q2p_cross_pulse.generic.type = MTYPE_SLIDER;
	s_q2p_cross_pulse.generic.x = 0;
	s_q2p_cross_pulse.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_cross_pulse.generic.name = "PULSE";
	s_q2p_cross_pulse.generic.callback = CrossPulseFunc;
	s_q2p_cross_pulse.minvalue = 0;
	s_q2p_cross_pulse.maxvalue = 3;

	s_q2p_cross_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_cross_alpha.generic.x = 0;
	s_q2p_cross_alpha.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_cross_alpha.generic.name = "ALPHA";
	s_q2p_cross_alpha.generic.callback = CrossAlphaFunc;
	s_q2p_cross_alpha.minvalue = 2;
	s_q2p_cross_alpha.maxvalue = 10;

	font_names = SetFontNames ();
	s_q2p_font_box.generic.type = MTYPE_SPINCONTROL;
	s_q2p_font_box.generic.x	= 0;
	s_q2p_font_box.generic.y	= y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_font_box.generic.name	= "FONT";
	s_q2p_font_box.generic.callback = FontFunc;
	s_q2p_font_box.itemnames = (const char **)font_names;

	s_q2p_fontsize_slider.generic.type	= MTYPE_SLIDER;
	s_q2p_fontsize_slider.generic.x		= 0;
	s_q2p_fontsize_slider.generic.y		= y+=(MENU_FONT_SIZE+2);
	s_q2p_fontsize_slider.generic.name	= "FONT SIZE";
	s_q2p_fontsize_slider.generic.callback	= FontSizeFunc;
	s_q2p_fontsize_slider.minvalue		= 2;
	s_q2p_fontsize_slider.maxvalue		= 6;
	
	s_q2p_hud_scale.generic.type = MTYPE_SPINCONTROL;
	s_q2p_hud_scale.generic.x = 0;
	s_q2p_hud_scale.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_hud_scale.generic.name = "HUD SCALE";
	s_q2p_hud_scale.generic.callback = HudScaleFunc;
	s_q2p_hud_scale.itemnames = hud_scale;

	s_q2p_hud_red.generic.type = MTYPE_SLIDER;
	s_q2p_hud_red.generic.x = 0;
	s_q2p_hud_red.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hud_red.generic.name = "RED";
	s_q2p_hud_red.generic.callback = HudColorRedFunc;
	s_q2p_hud_red.minvalue = 0;
	s_q2p_hud_red.maxvalue = 10;
	s_q2p_hud_red.generic.statusbar = "^3Color red intensity for numbers on the hud";

	s_q2p_hud_green.generic.type = MTYPE_SLIDER;
	s_q2p_hud_green.generic.x = 0;
	s_q2p_hud_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hud_green.generic.name = "GREEN";
	s_q2p_hud_green.generic.callback = HudColorGreenFunc;
	s_q2p_hud_green.minvalue = 0;
	s_q2p_hud_green.maxvalue = 10;
	s_q2p_hud_green.generic.statusbar = "^3Color green intensity for numbers on the hud";

	s_q2p_hud_blue.generic.type = MTYPE_SLIDER;
	s_q2p_hud_blue.generic.x = 0;
	s_q2p_hud_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hud_blue.generic.name = "BLUE";
	s_q2p_hud_blue.generic.callback = HudColorBlueFunc;
	s_q2p_hud_blue.minvalue = 0;
	s_q2p_hud_blue.maxvalue = 10;
	s_q2p_hud_blue.generic.statusbar = "^3Color blue intensity for numbers on the hud";

	s_q2p_hud_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_hud_alpha.generic.x = 0;
	s_q2p_hud_alpha.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_hud_alpha.generic.name = "ALPHA";
	s_q2p_hud_alpha.generic.callback = HudAlphaFunc;
	s_q2p_hud_alpha.minvalue = 2;
	s_q2p_hud_alpha.maxvalue = 10;
	s_q2p_hud_alpha.generic.statusbar = "^3Alpha level for numbers, pics, icons, ... on the hud";

	s_q2p_conback_image.generic.type = MTYPE_FIELD;
	s_q2p_conback_image.generic.name = "CONBACK IMAGE";
	s_q2p_conback_image.generic.callback = 0;
	s_q2p_conback_image.generic.x = 0;
	s_q2p_conback_image.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_conback_image.length = 20;
	s_q2p_conback_image.visible_length = 10;
	Q_strncpyz(s_q2p_conback_image.buffer, conback_image->string, sizeof(s_q2p_conback_image.buffer));
	s_q2p_conback_image.cursor = strlen(conback_image->string);
	s_q2p_conback_image.generic.statusbar =
	"^3Type the name for the conback in the box, ESCAPE will set the new value.";

	s_q2p_menu_conback_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_menu_conback_alpha.generic.x = 0;
	s_q2p_menu_conback_alpha.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_q2p_menu_conback_alpha.generic.name = "MENU ALPHA";
	s_q2p_menu_conback_alpha.generic.callback = MenuConbackAlphaFunc;
	s_q2p_menu_conback_alpha.minvalue = 0;
	s_q2p_menu_conback_alpha.maxvalue = 10;
	s_q2p_menu_conback_alpha.generic.statusbar = "^3Menu alpha level ingame";

	ControlsSetMenuItemValues();
	
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hud);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawfps);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_option_maxfps);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawclock);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawdate);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawmaptime);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawping);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawrate);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawchathud);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_draw_x);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_draw_y);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawnetgraph);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawnetgraph_pos);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawnetgraph_alpha);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawnetgraph_scale);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_netgraph_image);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawnetgraph_pos_x);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawnetgraph_pos_y);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_drawplayername);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_zoom_sniper);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_crosshair_box);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cross_scale);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cross_red);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cross_green);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cross_blue);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cross_pulse);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_cross_alpha);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_font_box);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_fontsize_slider);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hud_scale);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hud_red);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hud_green);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hud_blue);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_hud_alpha);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_conback_image);
	Menu_AddItem(&s_q2p_menu, (void *)&s_q2p_menu_conback_alpha);
	
}

void
M_Menu_Q2P_f_Client(void)
{
	Q2P_MenuInit_Client();
	M_PushMenu(Q2P_MenuDraw_Client, Q2P_MenuKey_Client);
}

void
M_Menu_Q2P_f_Graphic(void)
{
	Q2P_MenuInit_Graphic();
	M_PushMenu(Q2P_MenuDraw_Graphic, Q2P_MenuKey_Graphic);
}

void
M_Menu_Q2P_f_Hud(void)
{
	Q2P_MenuInit_Hud();
	M_PushMenu(Q2P_MenuDraw_Hud, Q2P_MenuKey_Hud);
	DrawMenuCrosshair();
	SetCrosshairCursor();
}


static void
Q2PClientFunc(void *unused)
{
	M_Menu_Q2P_f_Client();
}

static void
Q2PGraphicFunc(void *unused)
{
	M_Menu_Q2P_f_Graphic();
}

static void
Q2PHudFunc(void *unused)
{
	M_Menu_Q2P_f_Hud();
}


/*
=======================================================================

MAIN MENU

=======================================================================
*/
#define	MAIN_ITEMS	5

int MainMenuMouseHover;
typedef struct {
	int	min[2],
	        max[2];
	void    (*OpenMenu)(void);
} mainmenuobject_t;

char *main_names[] =
{
	"m_main_game",
	"m_main_multiplayer",
	"m_main_options",
	"m_main_video",
	"m_main_quit",
	NULL
};

void findMenuCoords(int *xoffset, int *ystart, int *totalheight, int *widest)
{
	int		w, h, i;

	*totalheight = 0;
	*widest = -1;

	for (i = 0; main_names[i] != 0; i++) {
		re.DrawGetPicSize(&w, &h, main_names[i]);

		if (w > *widest)
			*widest = w;
		*totalheight += (h + 12);
	}

	*ystart = (viddef.height / 2 - ScaledVideo(110) );
	*xoffset = (viddef.width - ScaledVideo(*widest) + ScaledVideo(70) ) / 2;
}

void M_Main_Draw (void)
{
	int i;
	int w, h, last_h;
	int ystart;
	int	xoffset;
	int widest = -1;
	int totalheight = 0;
	char litname[80];

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	for ( i = 0; main_names[i] != 0; i++ ) {
		if ( i != m_main_cursor ) {
			re.DrawGetPicSize( &w, &h, main_names[i] );
			re.DrawStretchPic2( xoffset, ystart + ScaledVideo(i * 40 + 13),
			                    ScaledVideo(w), ScaledVideo(h), main_names[i],
					    cl_menu_red->value, cl_menu_green->value, cl_menu_blue->value,
					    cl_menu_alpha->value);
		}
	}
	
	Q_strncpyz(litname, main_names[m_main_cursor], sizeof(litname));
	strncat(litname, "_sel", sizeof(litname) - strlen(litname) - 1);
	
	re.DrawGetPicSize( &w, &h, litname );
	re.DrawStretchPic2(xoffset, ystart + ScaledVideo(m_main_cursor * 40 + 13),
		          ScaledVideo(w+2), ScaledVideo(h+2), litname, cl_menu_red->value, cl_menu_green->value,
			  cl_menu_blue->value, cl_menu_alpha->value);
/*
	M_DrawCursor(xoffset - ScaledVideo(25),
		     ystart + ScaledVideo(m_main_cursor * 40 + 11),
		     (int)(cls.realtime / 100)%NUM_CURSOR_FRAMES );
*/
	re.DrawGetPicSize( &w, &h, "m_main_plaque" );
	re.DrawStretchPic2(xoffset - ScaledVideo(w/2 + 50), ystart + ScaledVideo(10),
		           ScaledVideo(w), ScaledVideo(h), "m_main_plaque",
			   cl_menu_red->value, cl_menu_green->value, cl_menu_blue->value,
			   cl_menu_alpha->value);
	last_h = h;

	re.DrawGetPicSize( &w, &h, "m_main_logo" );
	re.DrawStretchPic2(xoffset - ScaledVideo(w/2 + 50),
		           ystart + ScaledVideo(last_h) + ScaledVideo(30),
		           ScaledVideo(w), ScaledVideo(h), "m_main_logo",
			   cl_menu_red->value, cl_menu_green->value, cl_menu_blue->value,
			   cl_menu_alpha->value);
}

void addButton(mainmenuobject_t * thisObj, int index, int x, int y)
{
	int		w, h;
	float		ratio;

	re.DrawGetPicSize(&w, &h, main_names[index]);

	if (w) {
		ratio = 32.0 / (float)h;
		h = 32;
		w *= ratio;
	}
	
	thisObj->min[0] = x;
	thisObj->max[0] = x + ScaledVideo(w);
	thisObj->min[1] = y;
	thisObj->max[1] = y + ScaledVideo(h);
	
	switch (index) {
		case 0:
			thisObj->OpenMenu = M_Menu_Game_f;
		case 1:
			thisObj->OpenMenu = M_Menu_Multiplayer_f;
		case 2:
			thisObj->OpenMenu = M_Menu_Options_f;
		case 3:
			thisObj->OpenMenu = M_Menu_Video_f;
		case 4:
			thisObj->OpenMenu = M_Menu_Quit_f;
	}
}

void openMenuFromMain(void)
{
	switch (m_main_cursor) {
		case 0:
			M_Menu_Game_f();
			break;

		case 1:
			M_Menu_Multiplayer_f();
			break;

		case 2:
			M_Menu_Options_f();
			break;

		case 3:
			M_Menu_Video_f();
			break;

		case 4:
			M_Menu_Quit_f();
			break;
	}
}

void CheckMainMenuMouse(void)
{
	int		ystart;
	int		xoffset;
	int		widest;
	int		totalheight;
	int		i, oldhover;
	char           *sound = NULL;
	mainmenuobject_t buttons[MAIN_ITEMS];

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);
	
	for (i = 0; main_names[i] != 0; i++)
		addButton (&buttons[i], i, xoffset, ystart + ScaledVideo(i * 40 + 13));
		
	/* Exit with double click 2nd mouse button */
	if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] == 2) {
		M_PopMenu();
		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
	}
	for (i = MAIN_ITEMS - 1; i >= 0; i--) {
		if (cursor.x >= buttons[i].min[0] && cursor.x <= buttons[i].max[0] &&
		    cursor.y >= buttons[i].min[1] && cursor.y <= buttons[i].max[1]) {
			if (cursor.mouseaction)
				m_main_cursor = i;

			MainMenuMouseHover = 1 + i;

			if (oldhover == MainMenuMouseHover && MainMenuMouseHover - 1 == m_main_cursor &&
			    !cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1] == 1) {
				openMenuFromMain();
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON1] = true;
				cursor.buttonclicks[MOUSEBUTTON1] = 0;
			}
			break;
		}
	}

	if (!MainMenuMouseHover) {
		cursor.buttonused[MOUSEBUTTON1] = false;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
		cursor.buttontime[MOUSEBUTTON1] = 0;
	}
	cursor.mouseaction = false;

	if (sound)
		S_StartLocalSound(sound);
}

const char *M_Main_Key (int key)
{
	const char *sound = menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
		M_PopMenu ();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;
		openMenuFromMain();
	}

	return NULL;
}

void M_Menu_Main_f (void)
{
	M_PushMenu (M_Main_Draw, M_Main_Key);
}

/*
=======================================================================

MULTIPLAYER MENU

=======================================================================
*/
static menuframework_s	s_multiplayer_menu;
static menuaction_s		s_join_network_server_action;
static menuaction_s		s_start_network_server_action;
static menuaction_s		s_player_setup_action;

static void Multiplayer_MenuDraw (void)
{
	M_Banner( "m_banner_multiplayer" );

	Menu_AdjustCursor( &s_multiplayer_menu, 1 );
	Menu_Draw( &s_multiplayer_menu );
}

static void PlayerSetupFunc( void *unused )
{
	M_Menu_PlayerConfig_f();
}

static void JoinNetworkServerFunc( void *unused )
{
	M_Menu_JoinServer_f();
}

static void StartNetworkServerFunc( void *unused )
{
	M_Menu_StartServer_f ();
}

void Multiplayer_MenuInit( void )
{
	s_multiplayer_menu.x = viddef.width * 0.50 - ScaledVideo(8*MENU_FONT_SIZE);
	s_multiplayer_menu.nitems = 0;

	s_join_network_server_action.generic.type	= MTYPE_ACTION;
	s_join_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_join_network_server_action.generic.x		= 0;
	s_join_network_server_action.generic.y		= 0;
	s_join_network_server_action.generic.name	= " Join network server";
	s_join_network_server_action.generic.callback = JoinNetworkServerFunc;

	s_start_network_server_action.generic.type	= MTYPE_ACTION;
	s_start_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_start_network_server_action.generic.x		= 0;
	s_start_network_server_action.generic.y		= 2*MENU_FONT_SIZE;
	s_start_network_server_action.generic.name	= " Start network server";
	s_start_network_server_action.generic.callback = StartNetworkServerFunc;

	s_player_setup_action.generic.type	= MTYPE_ACTION;
	s_player_setup_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_player_setup_action.generic.x		= 0;
	s_player_setup_action.generic.y		= 4*MENU_FONT_SIZE;
	s_player_setup_action.generic.name	= " Player Setup";
	s_player_setup_action.generic.callback = PlayerSetupFunc;

	Menu_AddItem( &s_multiplayer_menu, ( void * ) &s_join_network_server_action );
	Menu_AddItem( &s_multiplayer_menu, ( void * ) &s_start_network_server_action );
	Menu_AddItem( &s_multiplayer_menu, ( void * ) &s_player_setup_action );

	Menu_SetStatusBar( &s_multiplayer_menu, NULL );

	Menu_Center( &s_multiplayer_menu );
}

const char *Multiplayer_MenuKey( int key )
{
	return Default_MenuKey( &s_multiplayer_menu, key );
}

void M_Menu_Multiplayer_f( void )
{
	Multiplayer_MenuInit();
	M_PushMenu( Multiplayer_MenuDraw, Multiplayer_MenuKey );
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/

char *bindnames[][2] =
{
	{"+attack", "Attack"},
	{"weapnext", "Next weapon"},
	{"weapprev", "Previous weapon"},
	{"MENUSPACE", ""},
	{"+forward", "Walk forward"},
	{"+back", "Backpedal"},
	{"+moveleft", "Step left"},
	{"+moveright", "Step right"},
	{"+left", "Turn left"},
	{"+right", "Turn right"},
	{"MENUSPACE", ""},
	{"+speed", "Run"},
	{"+strafe", "Sidestep"},
	{"+lookup", "Look up"},
	{"+lookdown", "Look down"},
	{"centerview", "Center view"},
	{"MENUSPACE", ""},
	{"+mlook", "Mouse look"},
	{"+klook", "Keyboard look"},
	{"+moveup", "Up / Jump"},
	{"+movedown", "Down / Crouch"},
	{"MENUSPACE", ""},
	{"inven", "Inventory"},
	{"invuse", "Use item"},
	{"invdrop", "Drop item"},
	{"invprev", "Prev item"},
	{"invnext", "Next item"},
	{"MENUSPACE", ""},
	{"cmd help", "Help computer"},
	{"MENUSPACE", ""},
	{"flashlight_eng", "Flashlight"},
	{0, 0}
};

int		keys_cursor;
static int	bind_grab;

static menuframework_s	s_keys_menu;
static menuaction_s	s_keys_binds[64];

static void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}

static void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void KeyCursorDrawFunc( menuframework_s *menu )
{
	if ( bind_grab )
		re.DrawScaledChar(menu->x, menu->y + ScaledVideo(menu->cursor * (MENU_FONT_SIZE+2) ),
				  '=', VideoScale(), 255,255,255,255, false);
	else
		re.DrawScaledChar(menu->x, menu->y + ScaledVideo(menu->cursor * (MENU_FONT_SIZE+2) ),
				  12 + ( ( int ) ( Sys_Milliseconds() / 250 ) & 1 ),
				  VideoScale(), 255,255,255,255, false);
}

static void DrawKeyBindingFunc( void *self )
{
	int keys[2];
	menuaction_s *a = ( menuaction_s * ) self;

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys);
		
	if (keys[0] == -1)
	{
		Menu_DrawString(
			ScaledVideo(a->generic.x + 16) + a->generic.parent->x,
			ScaledVideo(a->generic.y) + a->generic.parent->y,
			"???", 255 );
	}
	else
	{
		int x;
		const char *name;

		name = Key_KeynumToString (keys[0]);

		Menu_DrawString(
			ScaledVideo(a->generic.x + 16) + a->generic.parent->x,
			ScaledVideo(a->generic.y) + a->generic.parent->y,
			name , 255);

		x = ScaledVideo(strlen(name) * MENU_FONT_SIZE);

		if (keys[1] != -1)
		{
			Menu_DrawString(
				ScaledVideo(a->generic.x + MENU_FONT_SIZE*3 + x) + a->generic.parent->x,
				ScaledVideo(a->generic.y) + a->generic.parent->y,
				"or", 255 );
			Menu_DrawString(
				ScaledVideo(a->generic.x + MENU_FONT_SIZE*6 + x) + a->generic.parent->x,
				ScaledVideo(a->generic.y) + a->generic.parent->y,
				Key_KeynumToString (keys[1]), 255 );
		}
	}
}

static void KeyBindingFunc( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;
	int keys[2];

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys );

	if (keys[1] != -1)
		M_UnbindCommand( bindnames[a->generic.localdata[0]][0]);

	bind_grab = true;

	Menu_SetStatusBar( &s_keys_menu, "Press a key or button for this action" );
}

int listSize(char *list[][2])
{
	int		i = 0;
	while (list[i][1])
		i++;

	return i;
}

void addBindOption (int i, char* list[][2])
{		
	s_keys_binds[i].generic.type	= MTYPE_ACTION;
	s_keys_binds[i].generic.flags  = QMF_GRAYED;
	s_keys_binds[i].generic.x		= 0;
	s_keys_binds[i].generic.y		= i*(MENU_FONT_SIZE+2);
	s_keys_binds[i].generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_binds[i].generic.localdata[0] = i;
	s_keys_binds[i].generic.name	= list[s_keys_binds[i].generic.localdata[0]][1];
	s_keys_binds[i].generic.callback = KeyBindingFunc;

	if (strstr ("MENUSPACE", list[i][0]))
		s_keys_binds[i].generic.type	= MTYPE_SEPARATOR;
}

static void Keys_MenuInit(void)
{
	int		BINDS_MAX;
	int		i = 0;

	s_keys_menu.x = viddef.width * 0.50;
	s_keys_menu.y = viddef.height * 0.50 - ScaledVideo(7.25*MENU_FONT_SIZE);
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	BINDS_MAX = listSize(bindnames);
	for (i = 0; i < BINDS_MAX; i++)
		addBindOption(i, bindnames);


	for (i = 0; i < BINDS_MAX; i++)
		Menu_AddItem(&s_keys_menu, (void *)&s_keys_binds[i]);

	Menu_SetStatusBar(&s_keys_menu, "^1^sENTER to change, BACKSPACE to clear");
	Menu_Center(&s_keys_menu);
}

static void Keys_MenuDraw (void)
{
	Menu_AdjustCursor( &s_keys_menu, 1 );
	Menu_Draw( &s_keys_menu );
}

static const char *Keys_MenuKey( int key )
{
	menuaction_s *item = ( menuaction_s * ) Menu_ItemAtCursor( &s_keys_menu );

	if (bind_grab && !(cursor.buttonused[MOUSEBUTTON1] && key == K_MOUSE1))
	{	
		if ( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Com_sprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), bindnames[item->generic.localdata[0]][0]);
			Cbuf_InsertText (cmd);
		}
		
		refreshCursorButtons();
		
		if (key == K_MOUSE1)
			cursor.buttonclicks[MOUSEBUTTON1] = -1;
			
		Menu_SetStatusBar( &s_keys_menu, "^1^sENTER to change, BACKSPACE to clear");
		bind_grab = false;
		return menu_out_sound;
	}

	switch ( key ) {
		case K_KP_ENTER:
		case K_ENTER:
			KeyBindingFunc( item );
			return menu_in_sound;
		case K_BACKSPACE:		// delete bindings
		case K_DEL:				// delete bindings
		case K_KP_DEL:
			M_UnbindCommand( bindnames[item->generic.localdata[0]][0] );
			return menu_out_sound;
		default:
			return Default_MenuKey( &s_keys_menu, key );
	}
}

void M_Menu_Keys_f (void)
{
	Keys_MenuInit();
	M_PushMenu( Keys_MenuDraw, Keys_MenuKey );
}


/*
=======================================================================

CONTROLS MENU

=======================================================================
*/
#ifdef _WIN32
static cvar_t *win_noalttab;
extern cvar_t *in_joystick;
#endif

static menuframework_s	s_options_menu;
static menuaction_s		s_options_defaults_action;
static menuaction_s		s_options_customize_options_action;
static menuslider_s		s_options_sensitivity_slider;
static menulist_s		s_options_freelook_box;
#ifdef _WIN32
static menulist_s		s_options_noalttab_box;
#endif
static menulist_s		s_options_alwaysrun_box;
static menulist_s		s_options_invertmouse_box;
static menulist_s		s_options_lookspring_box;
static menulist_s		s_options_lookstrafe_box;
static menuslider_s		s_options_sfxvolume_slider;
static menuslider_s		s_options_ogg_volume;
static menulist_s		s_options_ogg_sequence;
static menulist_s		s_options_ogg_autoplay;
#if defined(CDAUDIO)
static menulist_s		s_options_cdvolume_box;
#endif
static menulist_s		s_options_quality_list;
#ifdef _WIN32
static menulist_s		s_options_joystick_box;
static menulist_s		s_options_compatibility_list;
#endif
static menulist_s		s_options_console_action;
static menuslider_s		s_q2p_menucursor_scale;
static menuslider_s		s_q2p_menucursor_red;
static menuslider_s		s_q2p_menucursor_green;
static menuslider_s		s_q2p_menucursor_blue;
static menuslider_s		s_q2p_menucursor_alpha;
static menuslider_s		s_q2p_menucursor_sensitivity;
static menulist_s		s_q2p_menucursor_type;
static menuslider_s		s_q2p_menu_red;
static menuslider_s		s_q2p_menu_green;
static menuslider_s		s_q2p_menu_blue;
static menuslider_s		s_q2p_menu_alpha;

#ifdef _WIN32
static void JoystickFunc( void *unused )
{
	Cvar_SetValue( "in_joystick", s_options_joystick_box.curvalue );
}
#endif

static void CustomizeControlsFunc( void *unused )
{
	M_Menu_Keys_f();
}

static void AlwaysRunFunc( void *unused )
{
	Cvar_SetValue( "cl_run", s_options_alwaysrun_box.curvalue );
}

static void FreeLookFunc( void *unused )
{
	Cvar_SetValue( "freelook", s_options_freelook_box.curvalue );
}

static void MouseSpeedFunc( void *unused )
{
	Cvar_SetValue( "sensitivity", s_options_sensitivity_slider.curvalue / 2.0f );
}
#ifdef _WIN32
static void NoAltTabFunc( void *unused )
{
	Cvar_SetValue( "win_noalttab", s_options_noalttab_box.curvalue );
}
#endif

static const char *ogg_autoplay_values[] = {
	"",
	"#1",
	"#-1",
	"?",
	NULL,
	NULL
};

void ControlsSetMenuItemValues( void )
{
	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;
	s_options_ogg_volume.curvalue = Cvar_VariableValue("ogg_volume") * 10;
#if defined(CDAUDIO)
	s_options_cdvolume_box.curvalue 		= !Cvar_VariableValue("cd_nocd");
#endif
	// DMP convert setting into index for option display text
	switch ((int)Cvar_VariableValue("s_khz")) {
		case 11:
			s_options_quality_list.curvalue = 1;
			break;
		case 22:
			s_options_quality_list.curvalue = 2;
			break;
		case 44:
			s_options_quality_list.curvalue = 3;
			break;
		case 48:
			s_options_quality_list.curvalue = 4;
			break;
		default:
			s_options_quality_list.curvalue = 3;
			break;
	}
	// DMP end sound menu changes
	
	s_options_sensitivity_slider.curvalue	= ( sensitivity->value ) * 2;

	Cvar_SetValue( "cl_run", ClampCvar( 0, 1, cl_run->value ) );
	s_options_alwaysrun_box.curvalue		= cl_run->value;

	s_options_invertmouse_box.curvalue		= m_pitch->value < 0;

	Cvar_SetValue( "lookspring", ClampCvar( 0, 1, lookspring->value ) );
	s_options_lookspring_box.curvalue		= lookspring->value;

	Cvar_SetValue( "lookstrafe", ClampCvar( 0, 1, lookstrafe->value ) );
	s_options_lookstrafe_box.curvalue		= lookstrafe->value;

	Cvar_SetValue( "freelook", ClampCvar( 0, 1, freelook->value ) );
	s_options_freelook_box.curvalue			= freelook->value;
	
#ifdef _WIN32
	Cvar_SetValue( "in_joystick", ClampCvar( 0, 1, in_joystick->value ) );
	s_options_joystick_box.curvalue		= in_joystick->value;

	s_options_noalttab_box.curvalue			= win_noalttab->value;
#endif

	Cvar_SetValue("cl_3dcam", ClampCvar(0, 1, Cvar_VariableValue("cl_3dcam")));
	s_q2p_3dcam.curvalue = Cvar_VariableValue("cl_3dcam");
	s_q2p_3dcam_distance.curvalue = Cvar_VariableValue("cl_3dcam_dist") / 25;
	s_q2p_3dcam_angle.curvalue = Cvar_VariableValue("cl_3dcam_angle") / 10;
	Cvar_SetValue("cl_bobbing", ClampCvar(0, 1, Cvar_VariableValue("cl_bobbing")));
	s_q2p_bobbing.curvalue = Cvar_VariableValue("cl_bobbing");

#if defined(PARTICLESYSTEM)
	Cvar_SetValue("cl_blaster_color", ClampCvar(0, 3, Cvar_VariableValue("cl_blaster_color")));
	s_q2p_blaster_color.curvalue = Cvar_VariableValue("cl_blaster_color");
	Cvar_SetValue("cl_blaster_bubble", ClampCvar(0, 2, Cvar_VariableValue("cl_blaster_bubble")));
	s_q2p_blaster_bubble.curvalue = Cvar_VariableValue("cl_blaster_bubble");
	Cvar_SetValue("cl_hyperblaster_particles", ClampCvar(0, 1, Cvar_VariableValue("cl_hyperblaster_particles")));
	s_q2p_hyperblaster_particles.curvalue = Cvar_VariableValue("cl_hyperblaster_particles");
	Cvar_SetValue("cl_hyperblaster_color", ClampCvar(0, 3, Cvar_VariableValue("cl_hyperblaster_color")));
	s_q2p_hyperblaster_color.curvalue = Cvar_VariableValue("cl_hyperblaster_color");
	Cvar_SetValue("cl_hyperblaster_bubble", ClampCvar(0, 2, Cvar_VariableValue("cl_hyperblaster_bubble")));
	s_q2p_hyperblaster_bubble.curvalue = Cvar_VariableValue("cl_hyperblaster_bubble");

	Cvar_SetValue("cl_railtype", ClampCvar(0, 3, Cvar_VariableValue("cl_railtype")));
	s_q2p_railtrail.curvalue = Cvar_VariableValue("cl_railtype");
	Cvar_SetValue("cl_railspiral_lens", ClampCvar(0, 1, Cvar_VariableValue("cl_railspiral_lens")));
	s_q2p_railspiral_part.curvalue = Cvar_VariableValue("cl_railspiral_lens");
	s_q2p_railtrail_red.curvalue = Cvar_VariableValue("cl_railred") / 16;
	s_q2p_railtrail_green.curvalue = Cvar_VariableValue("cl_railgreen") / 16;
	s_q2p_railtrail_blue.curvalue = Cvar_VariableValue("cl_railblue") / 16;

	Cvar_SetValue("cl_gunsmoke", ClampCvar(0, 2, Cvar_VariableValue("cl_gunsmoke")));
	s_q2p_gun_smoke.curvalue = Cvar_VariableValue("cl_gunsmoke");
	Cvar_SetValue("cl_explosion", ClampCvar(0, 4, Cvar_VariableValue("cl_explosion")));
	s_q2p_explosions.curvalue = Cvar_VariableValue("cl_explosion");
	s_q2p_explosions_scale.curvalue = Cvar_VariableValue("cl_explosion_scale") * 10;
	Cvar_SetValue("cl_flame_enh", ClampCvar(0, 1, Cvar_VariableValue("cl_flame_enh")));
	s_q2p_flame_enh.curvalue = Cvar_VariableValue("cl_flame_enh");
	s_q2p_flame_size.curvalue = Cvar_VariableValue("cl_flame_size");
	Cvar_SetValue("cl_teleport_particles", ClampCvar(0, 4, Cvar_VariableValue("cl_teleport_particles")));
	s_q2p_teleport.curvalue = Cvar_VariableValue("cl_teleport_particles");
	Cvar_SetValue("cl_particles_type", ClampCvar(0, 2, Cvar_VariableValue("cl_particles_type")));
	s_q2p_particles_type.curvalue = Cvar_VariableValue("cl_particles_type");
	Cvar_SetValue("cl_nukeblast_enh", ClampCvar(0, 1, Cvar_VariableValue("cl_nukeblast_enh")));
	s_q2p_nukeblast.curvalue = Cvar_VariableValue("cl_nukeblast_enh");
#else
	Cvar_SetValue("gl_particles", ClampCvar(0, 1, Cvar_VariableValue("gl_particles")));
	s_q2p_gl_particles.curvalue = Cvar_VariableValue("gl_particles");
#endif

	Cvar_SetValue("cl_flashlight", ClampCvar(0, 1, Cvar_VariableValue("cl_flashlight")));
	s_q2p_flashlight.curvalue = Cvar_VariableValue("cl_flashlight");
	s_q2p_flashlight_red.curvalue = Cvar_VariableValue("cl_flashlight_red") * 10;
	s_q2p_flashlight_green.curvalue = Cvar_VariableValue("cl_flashlight_green") * 10;
	s_q2p_flashlight_blue.curvalue = Cvar_VariableValue("cl_flashlight_blue") * 10;
	s_q2p_flashlight_intensity.curvalue = Cvar_VariableValue("cl_flashlight_intensity") / 10;
	s_q2p_flashlight_distance.curvalue = Cvar_VariableValue("cl_flashlight_distance") / 10;
	s_q2p_flashlight_decscale.curvalue = Cvar_VariableValue("cl_flashlight_decscale") * 10;
	
	s_q2p_gl_reflection.curvalue = Cvar_VariableValue("gl_water_reflection") * 10;
	Cvar_SetValue("gl_water_reflection_shader", ClampCvar(0, 1, Cvar_VariableValue("gl_water_reflection_shader")));
	s_q2p_gl_reflection_shader.curvalue = Cvar_VariableValue("gl_water_reflection_shader");
	s_q2p_waves.curvalue = Cvar_VariableValue("gl_water_waves");
	Cvar_SetValue("cl_underwater_trans", ClampCvar(0, 1, Cvar_VariableValue("cl_underwater_trans")));
	s_q2p_water_trans.curvalue = Cvar_VariableValue("cl_underwater_trans");
	Cvar_SetValue("gl_water_caustics", ClampCvar(0, 2, Cvar_VariableValue("gl_water_caustics")));
	s_q2p_caustics.curvalue = Cvar_VariableValue("gl_water_caustics");
	Cvar_SetValue("cl_underwater_movement", ClampCvar(0, 1, Cvar_VariableValue("cl_underwater_movement")));
	s_q2p_underwater_mov.curvalue = Cvar_VariableValue("cl_underwater_movement");
	Cvar_SetValue("cl_underwater_view", ClampCvar(0, 1, Cvar_VariableValue("cl_underwater_view")));
	s_q2p_underwater_view.curvalue = Cvar_VariableValue("cl_underwater_view");

	Cvar_SetValue("gl_fogenable", ClampCvar(0, 1, Cvar_VariableValue("gl_fogenable")));
	s_q2p_fog.curvalue = Cvar_VariableValue("gl_fogenable");
	s_q2p_fog_density.curvalue = Cvar_VariableValue("gl_fogdensity") * 10000;
	s_q2p_fog_red.curvalue = Cvar_VariableValue("gl_fogred") * 10;
	s_q2p_fog_green.curvalue = Cvar_VariableValue("gl_foggreen") * 10;
	s_q2p_fog_blue.curvalue = Cvar_VariableValue("gl_fogblue") * 10;
	Cvar_SetValue("gl_fogunderwater", ClampCvar(0, 1, Cvar_VariableValue("gl_fogunderwater")));
	s_q2p_fog_underwater.curvalue = Cvar_VariableValue("gl_fogunderwater");

	Cvar_SetValue("gl_minimap", ClampCvar(0, 2, Cvar_VariableValue("gl_minimap")));
	s_q2p_minimap.curvalue = Cvar_VariableValue("gl_minimap");
	Cvar_SetValue("gl_minimap_style", ClampCvar(0, 1, Cvar_VariableValue("gl_minimap_style")));
	s_q2p_minimap_style.curvalue = Cvar_VariableValue("gl_minimap_style");
	s_q2p_minimap_size.curvalue = Cvar_VariableValue("gl_minimap_size");
	s_q2p_minimap_zoom.curvalue = Cvar_VariableValue("gl_minimap_zoom") * 10;
	s_q2p_minimap_x.curvalue = Cvar_VariableValue("gl_minimap_x");	
	s_q2p_minimap_y.curvalue = Cvar_VariableValue("gl_minimap_y");	

	s_q2p_det_tex.curvalue = Cvar_VariableValue("gl_detailtextures");
	Cvar_SetValue("gl_shadows", ClampCvar(0, 2, Cvar_VariableValue("gl_shadows")));
	s_q2p_shadows.curvalue = Cvar_VariableValue("gl_shadows");
#if !defined(PARTICLESYSTEM)
	Cvar_SetValue("r_decals", ClampCvar(0, 1, Cvar_VariableValue("r_decals")));
	s_q2p_r_decals.curvalue = Cvar_VariableValue("r_decals");
#endif
	Cvar_SetValue("gl_cellshade", ClampCvar(0, 1, Cvar_VariableValue("gl_cellshade")));
	s_q2p_cellshading.curvalue = Cvar_VariableValue("gl_cellshade");

	Cvar_SetValue("gl_motionblur", ClampCvar(0, 2, Cvar_VariableValue("gl_motionblur")));
	s_q2p_motionblur.curvalue = Cvar_VariableValue("gl_motionblur");
	s_q2p_motionblur_intens.curvalue = Cvar_VariableValue("gl_motionblur_intensity") * 10;
	Cvar_SetValue("gl_bloom", ClampCvar(0, 1, Cvar_VariableValue("gl_bloom")));
	s_q2p_blooms.curvalue = Cvar_VariableValue("gl_bloom");
	s_q2p_blooms_alpha.curvalue = Cvar_VariableValue("gl_bloom_alpha") * 10;
	s_q2p_blooms_diamond_size.curvalue = Cvar_VariableValue("gl_bloom_diamond_size") / 2;
	s_q2p_blooms_intensity.curvalue = Cvar_VariableValue("gl_bloom_intensity") * 10;
	s_q2p_blooms_darken.curvalue = Cvar_VariableValue("gl_bloom_darken");
	s_q2p_blooms_sample_size.curvalue = log10(Cvar_VariableValue("gl_bloom_sample_size"))/log10(2);
	Cvar_SetValue("gl_bloom_fast_sample", ClampCvar(0, 1, Cvar_VariableValue("gl_bloom_fast_sample")));
	s_q2p_blooms_fast_sample.curvalue = Cvar_VariableValue("gl_bloom_fast_sample");
	
	Cvar_SetValue("gl_lensflare", ClampCvar(0, 1, Cvar_VariableValue("gl_lensflare")));
	s_q2p_lensflare.curvalue = Cvar_VariableValue("gl_lensflare");
	s_q2p_lensflare_intens.curvalue = Cvar_VariableValue("gl_lensflare_intens") * 10;
	
	Cvar_SetValue("cl_draw_clock", ClampCvar(0, 2, Cvar_VariableValue("cl_draw_clock")));
	s_q2p_drawclock.curvalue = Cvar_VariableValue("cl_draw_clock");
	Cvar_SetValue("cl_draw_rate", ClampCvar(0, 1, Cvar_VariableValue("cl_draw_rate")));
	s_q2p_drawdate.curvalue = Cvar_VariableValue("cl_draw_date");
	Cvar_SetValue("cl_draw_maptime", ClampCvar(0, 1, Cvar_VariableValue("cl_draw_maptime")));
	s_q2p_drawmaptime.curvalue = Cvar_VariableValue("cl_draw_maptime");
	Cvar_SetValue("cl_draw_fps", ClampCvar(0, 1, Cvar_VariableValue("cl_draw_fps")));
	s_q2p_drawfps.curvalue = Cvar_VariableValue("cl_draw_fps");
	Cvar_SetValue("cl_draw_ping", ClampCvar(0, 1, Cvar_VariableValue("cl_draw_ping")));
	s_q2p_drawping.curvalue = Cvar_VariableValue("cl_draw_ping");
	Cvar_SetValue("cl_draw_rate", ClampCvar(0, 1, Cvar_VariableValue("cl_draw_rate")));
	s_q2p_drawrate.curvalue = Cvar_VariableValue("cl_draw_rate");
	s_q2p_drawnetgraph.curvalue = Cvar_VariableValue("netgraph");
	s_q2p_drawnetgraph_pos.curvalue = Cvar_VariableValue("netgraph_pos");
	s_q2p_drawnetgraph_alpha.curvalue = Cvar_VariableValue("netgraph_trans") * 10;
	s_q2p_drawnetgraph_scale.curvalue = Cvar_VariableValue("netgraph_size") * 1;
	s_q2p_drawnetgraph_pos_x.curvalue = Cvar_VariableValue("netgraph_pos_x");
	s_q2p_drawnetgraph_pos_y.curvalue = Cvar_VariableValue("netgraph_pos_y");
	Cvar_SetValue("cl_draw_chathud", ClampCvar(0, 1, Cvar_VariableValue("cl_draw_chathud")));
	s_q2p_drawchathud.curvalue = Cvar_VariableValue("cl_draw_chathud");
	s_q2p_drawplayername.curvalue = Cvar_VariableValue("cl_draw_playername");
	s_q2p_zoom_sniper.curvalue = Cvar_VariableValue("cl_draw_zoom_sniper");
	s_q2p_draw_x.curvalue = Cvar_VariableValue("cl_draw_x") * 20;
	s_q2p_draw_y.curvalue = Cvar_VariableValue("cl_draw_y") * 20;

	s_q2p_crosshair_box.curvalue = crosshair->value;
	s_q2p_crosshair_box.curvalue = Cvar_VariableValue("crosshair");
	s_q2p_cross_scale.curvalue = Cvar_VariableValue("crosshair_scale") * 10;
	s_q2p_cross_red.curvalue = Cvar_VariableValue("crosshair_red") * 10;
	s_q2p_cross_green.curvalue = Cvar_VariableValue("crosshair_green") * 10;
	s_q2p_cross_blue.curvalue = Cvar_VariableValue("crosshair_blue") * 10;
	s_q2p_cross_pulse.curvalue = Cvar_VariableValue("crosshair_pulse") * 1;
	s_q2p_cross_alpha.curvalue = Cvar_VariableValue("crosshair_alpha") * 10;

	s_q2p_hud_red.curvalue = Cvar_VariableValue("cl_hudred") * 10;
	s_q2p_hud_green.curvalue = Cvar_VariableValue("cl_hudgreen") * 10;
	s_q2p_hud_blue.curvalue = Cvar_VariableValue("cl_hudblue") * 10;
	s_q2p_hud_alpha.curvalue = Cvar_VariableValue("cl_hudalpha") * 10;

	s_q2p_menu_conback_alpha.curvalue = Cvar_VariableValue("menu_alpha") * 10;
	
	s_q2p_hud_scale.curvalue = GetHudRes();
}

static void ControlsResetDefaultsFunc( void *unused )
{
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

static void InvertMouseFunc( void *unused )
{
	Cvar_SetValue( "m_pitch", -m_pitch->value );
}

static void LookspringFunc( void *unused )
{
	Cvar_SetValue( "lookspring", !lookspring->value );
}

static void LookstrafeFunc( void *unused )
{
	Cvar_SetValue( "lookstrafe", !lookstrafe->value );
}

static void UpdateVolumeFunc( void *unused )
{
	Cvar_SetValue( "s_volume", s_options_sfxvolume_slider.curvalue / 10 );
}

static void
UpdateVolumeOggFunc(void *unused)
{
	Cvar_SetValue("ogg_volume", s_options_ogg_volume.curvalue / 10);
}

static void
OggSequenceFunc(void *unused)
{
	Cvar_Set("ogg_sequence", (char *)s_options_ogg_sequence.itemnames[s_options_ogg_sequence.curvalue]);
}

static void
OggAutoplayFunc(void *unused)
{
	Cvar_Set("ogg_autoplay", (char *)ogg_autoplay_values[s_options_ogg_autoplay.curvalue]);
}

#if defined(CDAUDIO)
static void UpdateCDVolumeFunc( void *unused )
{
	Cvar_SetValue( "cd_nocd", !s_options_cdvolume_box.curvalue );
}
#endif

static void ConsoleFunc( void *unused )
{
	/*
	** the proper way to do this is probably to have ToggleConsole_f accept a parameter
	*/
	extern void Key_ClearTyping( void );

	if ( cl.attractloop )
	{
		Cbuf_AddText ("killserver\n");
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	M_ForceMenuOff ();
	cls.key_dest = key_console;
}

static void UpdateSoundQualityFunc( void *unused )
{
	// DMP check the newly added sound quality menu options
	switch (s_options_quality_list.curvalue) {
		case 1:
			Cvar_SetValue( "s_khz", 11 );
			Cvar_SetValue( "s_loadas8bit", true );
			break;
		case 2:
			Cvar_SetValue( "s_khz", 22 );
			Cvar_SetValue( "s_loadas8bit", false );
			break;
		case 3:
			Cvar_SetValue( "s_khz", 44 );
			Cvar_SetValue( "s_loadas8bit", false );
			break;
		case 4:
			Cvar_SetValue( "s_khz", 48 );
			Cvar_SetValue( "s_loadas8bit", false );
			break;
		default:
			Cvar_SetValue( "s_khz", 44 );
			Cvar_SetValue( "s_loadas8bit", false );
			break;
	}
	// DMP end sound menu changes
	
#ifdef _WIN32
	Cvar_SetValue( "s_primary", s_options_compatibility_list.curvalue );
#endif

	M_DrawTextBox( 8, 120 - 48, 36, 3 );
	M_Print( 28, 120 - 48 + MENU_FONT_SIZE,  "Restarting the sound system. This" );
	M_Print( 28, 120 - 48 + MENU_FONT_SIZE*2, "could take up to a minute, so" );
	M_Print( 28, 120 - 48 + MENU_FONT_SIZE*3, "please be patient." );
	
	// the text box won't show up unless we do a buffer swap
	re.EndFrame();

	CL_Snd_Restart_f();
}

static void
MenuCursorScaleFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_scale", s_q2p_menucursor_scale.curvalue / 10);
}

static void
ColorMenuCursorRedFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_red", s_q2p_menucursor_red.curvalue / 10);
}

static void
ColorMenuCursorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_green", s_q2p_menucursor_green.curvalue / 10);
}

static void
ColorMenuCursorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_blue", s_q2p_menucursor_blue.curvalue / 10);
}

static void
MenuCursorAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_alpha", s_q2p_menucursor_alpha.curvalue / 10);
}

static void
MenuCursorSensitivityFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_sensitivity", s_q2p_menucursor_sensitivity.curvalue / 2);
}

static void
MenuCursorTypeFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_cursor", s_q2p_menucursor_type.curvalue);
}

static void
ColorMenuRedFunc(void *unused)
{
	Cvar_SetValue("cl_menu_red", s_q2p_menu_red.curvalue / 10);
}

static void
ColorMenuGreenFunc(void *unused)
{
	Cvar_SetValue("cl_menu_green", s_q2p_menu_green.curvalue / 10);
}

static void
ColorMenuBlueFunc(void *unused)
{
	Cvar_SetValue("cl_menu_blue", s_q2p_menu_blue.curvalue / 10);
}

static void
MenuAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_menu_alpha", s_q2p_menu_alpha.curvalue / 10);
}

/*
 * GetPosInList
 *
 * Get the position of "name" in "list", where "list" is a string pointer. The
 * final element (string) is expected to be NULL.
 */
int GetPosInList(const char **list, const char *name)
{
	int		i;

	for (i = 0; list[i] != NULL; i++)
		if (strcmp(list[i], name) == 0)
			return (i);

	return (-1);
}

void Options_MenuInit( void )
{
	int y = 0;
	
#if defined(CDAUDIO)
	static const char *cd_music_items[] =
	{
		"Disabled",
		"Enabled",
		NULL
	};
#endif
	static const char *quality_items[] =
	{
		"Default [44]",
		"Low [11] ??",
		"Medium [22] ?",
		"High [44]",
		"Highest [48]",
		NULL
	};

	static const char *ogg_sequence_names[] =
	{
		"none",
		"next",
		"prev",
		"random",
		"loop",
		NULL
	};

	static const char *ogg_autoplay_names[] =
	{
		"none",
		"first",
		"last",
		"random",
		NULL,
		NULL
	};
	
#ifdef _WIN32
	static const char *compatibility_items[] =
	{
		"Max compatibility",
		"Max performance", 
		NULL
	};
#endif
	static const char *yesno_names[] =
	{
		"No",
		"Yes",
		NULL
	};

	static const char *menucursor_names[] =
	{
		"None",
		"Default",
		"Custom ",
		NULL
	};
	
#ifdef _WIN32
	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );
#endif
	/*
	** configure controls menu and menu items
	*/
	s_options_menu.x = viddef.width * 0.50;
#ifdef _WIN32
	s_options_menu.y = viddef.height * 0.50 - ScaledVideo(200);
#else
	s_options_menu.y = viddef.height * 0.50 - ScaledVideo(160);
#endif
	s_options_menu.nitems = 0;

	s_options_sfxvolume_slider.generic.type	= MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x	= 0;
	s_options_sfxvolume_slider.generic.y	= y;
	s_options_sfxvolume_slider.generic.name = "VOLUME";
	s_options_sfxvolume_slider.generic.callback	= UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue		= 0;
	s_options_sfxvolume_slider.maxvalue		= 10;
	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;

	s_options_ogg_volume.generic.type = MTYPE_SLIDER;
	s_options_ogg_volume.generic.x = 0;
	s_options_ogg_volume.generic.y = y+=1.5*(MENU_FONT_SIZE+2);
	s_options_ogg_volume.generic.name = "OGG VOLUME";
	s_options_ogg_volume.generic.callback = UpdateVolumeOggFunc;
	s_options_ogg_volume.minvalue = 0;
	s_options_ogg_volume.maxvalue = 20;
	s_options_ogg_volume.curvalue = Cvar_VariableValue("ogg_volume") * 10;

	s_options_ogg_sequence.generic.type = MTYPE_SPINCONTROL;
	s_options_ogg_sequence.generic.x = 0;
	s_options_ogg_sequence.generic.y = y+=(MENU_FONT_SIZE+2);
	s_options_ogg_sequence.generic.name = "OGG SEQUENCE";
	s_options_ogg_sequence.generic.callback = OggSequenceFunc;
	s_options_ogg_sequence.itemnames = ogg_sequence_names;
	s_options_ogg_sequence.curvalue =
	GetPosInList(s_options_ogg_sequence.itemnames, Cvar_VariableString("ogg_sequence"));

	s_options_ogg_autoplay.generic.type = MTYPE_SPINCONTROL;
	s_options_ogg_autoplay.generic.x = 0;
	s_options_ogg_autoplay.generic.y = y+=(MENU_FONT_SIZE+2);
	s_options_ogg_autoplay.generic.name = "OGG AUTOPLAY";
	s_options_ogg_autoplay.generic.callback = OggAutoplayFunc;
	s_options_ogg_autoplay.itemnames = ogg_autoplay_names;

	ogg_autoplay_names[4] = ogg_autoplay_values[4] = NULL;

	s_options_ogg_autoplay.curvalue = GetPosInList(ogg_autoplay_values,
	    Cvar_VariableString("ogg_autoplay"));

	if (s_options_ogg_autoplay.curvalue == -1) {
		s_options_ogg_autoplay.curvalue = 4;
		ogg_autoplay_names[4] = "custom";
		ogg_autoplay_values[4] = Cvar_VariableString("ogg_autoplay");
	}

#if defined(CDAUDIO)
	s_options_cdvolume_box.generic.type	= MTYPE_SPINCONTROL;
	s_options_cdvolume_box.generic.x		= 0;
	s_options_cdvolume_box.generic.y		= y+=1.5*(MENU_FONT_SIZE+2);
	s_options_cdvolume_box.generic.name	= "CD MUSIC";
	s_options_cdvolume_box.generic.callback	= UpdateCDVolumeFunc;
	s_options_cdvolume_box.itemnames		= cd_music_items;
	s_options_cdvolume_box.curvalue 		= !Cvar_VariableValue("cd_nocd");
#endif

	s_options_quality_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_quality_list.generic.x		= 0;
	s_options_quality_list.generic.y		= y+=1.5*(MENU_FONT_SIZE+2);
	s_options_quality_list.generic.name		= "SOUND QUALITY";
	s_options_quality_list.generic.callback = UpdateSoundQualityFunc;
	s_options_quality_list.itemnames		= quality_items;
	s_options_quality_list.curvalue			= !Cvar_VariableValue( "s_loadas8bit" );

#ifdef _WIN32
	s_options_compatibility_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_compatibility_list.generic.x		= 0;
	s_options_compatibility_list.generic.y		= y+=(MENU_FONT_SIZE+2);
	s_options_compatibility_list.generic.name	= "SOUND COMPATIBILITY";
	s_options_compatibility_list.generic.callback = UpdateSoundQualityFunc;
	s_options_compatibility_list.itemnames		= compatibility_items;
	s_options_compatibility_list.curvalue		= Cvar_VariableValue( "s_primary" );
#endif
	s_options_sensitivity_slider.generic.type	= MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x		= 0;
	s_options_sensitivity_slider.generic.y		= y+=2*(MENU_FONT_SIZE+2);
	s_options_sensitivity_slider.generic.name	= "MOUSE SPEED";
	s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	s_options_sensitivity_slider.minvalue		= 2;
	s_options_sensitivity_slider.maxvalue		= 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x	= 0;
	s_options_alwaysrun_box.generic.y	= y+=(MENU_FONT_SIZE+2);
	s_options_alwaysrun_box.generic.name	= "ALWAYS RUN";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x	= 0;
	s_options_invertmouse_box.generic.y	= y+=(MENU_FONT_SIZE+2);
	s_options_invertmouse_box.generic.name	= "INVERT MOU8SE";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x	= 0;
	s_options_lookspring_box.generic.y	= y+=(MENU_FONT_SIZE+2);
	s_options_lookspring_box.generic.name	= "LOOKSPRING";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yesno_names;

	s_options_lookstrafe_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookstrafe_box.generic.x	= 0;
	s_options_lookstrafe_box.generic.y	= y+=(MENU_FONT_SIZE+2);
	s_options_lookstrafe_box.generic.name	= "LOOKSTRAFE";
	s_options_lookstrafe_box.generic.callback = LookstrafeFunc;
	s_options_lookstrafe_box.itemnames = yesno_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x	= 0;
	s_options_freelook_box.generic.y	= y+=(MENU_FONT_SIZE+2);
	s_options_freelook_box.generic.name	= "FREE LOOK";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yesno_names;

#ifdef _WIN32
	s_options_noalttab_box.generic.type = MTYPE_SPINCONTROL;
	s_options_noalttab_box.generic.x	= 0;
	s_options_noalttab_box.generic.y	= y+=2*(MENU_FONT_SIZE+2);
	s_options_noalttab_box.generic.name	= "DISABLE alt+tab";
	s_options_noalttab_box.generic.callback = NoAltTabFunc;
	s_options_noalttab_box.itemnames = yesno_names;

	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x	= 0;
	s_options_joystick_box.generic.y	= y+=(MENU_FONT_SIZE+2);
	s_options_joystick_box.generic.name	= "USE JOYSTICK";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;
#endif

	s_q2p_menucursor_scale.generic.type = MTYPE_SLIDER;
	s_q2p_menucursor_scale.generic.x = 0;
	s_q2p_menucursor_scale.generic.y = y+=2*(MENU_FONT_SIZE+2);
	s_q2p_menucursor_scale.generic.name = "MENU MOUSE SIZE";
	s_q2p_menucursor_scale.generic.callback = MenuCursorScaleFunc;
	s_q2p_menucursor_scale.minvalue = 3;
	s_q2p_menucursor_scale.maxvalue = 10;
	s_q2p_menucursor_scale.curvalue = Cvar_VariableValue("cl_mouse_scale") * 10;

	s_q2p_menucursor_red.generic.type = MTYPE_SLIDER;
	s_q2p_menucursor_red.generic.x = 0;
	s_q2p_menucursor_red.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menucursor_red.generic.name = "RED";
	s_q2p_menucursor_red.generic.callback = ColorMenuCursorRedFunc;
	s_q2p_menucursor_red.minvalue = 0;
	s_q2p_menucursor_red.maxvalue = 10;
	s_q2p_menucursor_red.curvalue = Cvar_VariableValue("cl_mouse_red") * 10;

	s_q2p_menucursor_green.generic.type = MTYPE_SLIDER;
	s_q2p_menucursor_green.generic.x = 0;
	s_q2p_menucursor_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menucursor_green.generic.name = "GREEN";
	s_q2p_menucursor_green.generic.callback = ColorMenuCursorGreenFunc;
	s_q2p_menucursor_green.minvalue = 0;
	s_q2p_menucursor_green.maxvalue = 10;
	s_q2p_menucursor_green.curvalue = Cvar_VariableValue("cl_mouse_green") * 10;

	s_q2p_menucursor_blue.generic.type = MTYPE_SLIDER;
	s_q2p_menucursor_blue.generic.x = 0;
	s_q2p_menucursor_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menucursor_blue.generic.name = "BLUE";
	s_q2p_menucursor_blue.generic.callback = ColorMenuCursorBlueFunc;
	s_q2p_menucursor_blue.minvalue = 0;
	s_q2p_menucursor_blue.maxvalue = 10;
	s_q2p_menucursor_blue.curvalue = Cvar_VariableValue("cl_mouse_blue") * 10;

	s_q2p_menucursor_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_menucursor_alpha.generic.x = 0;
	s_q2p_menucursor_alpha.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menucursor_alpha.generic.name = "ALPHA";
	s_q2p_menucursor_alpha.generic.callback = MenuCursorAlphaFunc;
	s_q2p_menucursor_alpha.minvalue = 2;
	s_q2p_menucursor_alpha.maxvalue = 10;
	s_q2p_menucursor_alpha.curvalue = Cvar_VariableValue("cl_mouse_alpha") * 10;

	s_q2p_menucursor_sensitivity.generic.type = MTYPE_SLIDER;
	s_q2p_menucursor_sensitivity.generic.x = 0;
	s_q2p_menucursor_sensitivity.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menucursor_sensitivity.generic.name = "SENSITIVITY";
	s_q2p_menucursor_sensitivity.generic.callback = MenuCursorSensitivityFunc;
	s_q2p_menucursor_sensitivity.minvalue = 0.5;
	s_q2p_menucursor_sensitivity.maxvalue = 4;
	s_q2p_menucursor_sensitivity.curvalue = Cvar_VariableValue("cl_mouse_sensitivity") * 2;

	s_q2p_menucursor_type.generic.type = MTYPE_SPINCONTROL;
	s_q2p_menucursor_type.generic.x = 0;
	s_q2p_menucursor_type.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menucursor_type.generic.name = "CURSOR TYPE";
	s_q2p_menucursor_type.generic.callback = MenuCursorTypeFunc;
	s_q2p_menucursor_type.itemnames = menucursor_names;
	s_q2p_menucursor_type.curvalue = Cvar_VariableValue("cl_mouse_cursor");

	s_q2p_menu_red.generic.type = MTYPE_SLIDER;
	s_q2p_menu_red.generic.x = 0;
	s_q2p_menu_red.generic.y = y+=2*(MENU_FONT_SIZE+2);
	s_q2p_menu_red.generic.name = "MENU RED";
	s_q2p_menu_red.generic.callback = ColorMenuRedFunc;
	s_q2p_menu_red.minvalue = 0;
	s_q2p_menu_red.maxvalue = 10;
	s_q2p_menu_red.curvalue = Cvar_VariableValue("cl_menu_red") * 10;

	s_q2p_menu_green.generic.type = MTYPE_SLIDER;
	s_q2p_menu_green.generic.x = 0;
	s_q2p_menu_green.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menu_green.generic.name = "MENU GREEN";
	s_q2p_menu_green.generic.callback = ColorMenuGreenFunc;
	s_q2p_menu_green.minvalue = 0;
	s_q2p_menu_green.maxvalue = 10;
	s_q2p_menu_green.curvalue = Cvar_VariableValue("cl_menu_green") * 10;

	s_q2p_menu_blue.generic.type = MTYPE_SLIDER;
	s_q2p_menu_blue.generic.x = 0;
	s_q2p_menu_blue.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menu_blue.generic.name = "MENU BLUE";
	s_q2p_menu_blue.generic.callback = ColorMenuBlueFunc;
	s_q2p_menu_blue.minvalue = 0;
	s_q2p_menu_blue.maxvalue = 10;
	s_q2p_menu_blue.curvalue = Cvar_VariableValue("cl_menu_blue") * 10;

	s_q2p_menu_alpha.generic.type = MTYPE_SLIDER;
	s_q2p_menu_alpha.generic.x = 0;
	s_q2p_menu_alpha.generic.y = y+=(MENU_FONT_SIZE+2);
	s_q2p_menu_alpha.generic.name = "MENU ALPHA";
	s_q2p_menu_alpha.generic.callback = MenuAlphaFunc;
	s_q2p_menu_alpha.minvalue = 2;
	s_q2p_menu_alpha.maxvalue = 10;
	s_q2p_menu_alpha.curvalue = Cvar_VariableValue("cl_menu_alpha") * 10;

	s_options_q2p_action.generic.type = MTYPE_SEPARATOR;
	s_options_q2p_action.generic.x = 30;
	s_options_q2p_action.generic.y = y+=2*(MENU_FONT_SIZE+2);
	s_options_q2p_action.generic.name = "Q2P - OPTIONS";

	s_options_q2p_action_client.generic.type = MTYPE_ACTION;
	s_options_q2p_action_client.generic.x = 0;
	s_options_q2p_action_client.generic.y = y+=(MENU_FONT_SIZE+2);
	s_options_q2p_action_client.generic.name = "CLIENT SIDE";
	s_options_q2p_action_client.generic.callback = Q2PClientFunc;

	s_options_q2p_action_graphic.generic.type = MTYPE_ACTION;
	s_options_q2p_action_graphic.generic.x = 0;
	s_options_q2p_action_graphic.generic.y =  y+=(MENU_FONT_SIZE+2);
	s_options_q2p_action_graphic.generic.name = "GRAPHICS";
	s_options_q2p_action_graphic.generic.callback = Q2PGraphicFunc;

	s_options_q2p_action_hud.generic.type = MTYPE_ACTION;
	s_options_q2p_action_hud.generic.x = 0;
	s_options_q2p_action_hud.generic.y = y+=(MENU_FONT_SIZE+2);
	s_options_q2p_action_hud.generic.name = "HUD";
	s_options_q2p_action_hud.generic.callback = Q2PHudFunc;

	s_options_customize_options_action.generic.type	= MTYPE_ACTION;
	s_options_customize_options_action.generic.x		= 0;
	s_options_customize_options_action.generic.y		= y+=2*(MENU_FONT_SIZE+2);
	s_options_customize_options_action.generic.name	= "CONTROLS";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_defaults_action.generic.type	= MTYPE_ACTION;
	s_options_defaults_action.generic.x		= 0;
	s_options_defaults_action.generic.y		= y+=2*(MENU_FONT_SIZE+2);
	s_options_defaults_action.generic.name	= "DEFAULTS";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type	= MTYPE_ACTION;
	s_options_console_action.generic.x		= 0;
	s_options_console_action.generic.y		= y+=(MENU_FONT_SIZE+2);
	s_options_console_action.generic.name	= "CONSOLE";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sfxvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_ogg_volume);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_ogg_sequence);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_ogg_autoplay);
#if defined(CDAUDIO)
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_cdvolume_box );
#endif
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_quality_list );
#ifdef _WIN32
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_compatibility_list );
#endif
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_alwaysrun_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_invertmouse_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_lookspring_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_lookstrafe_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_freelook_box );
#ifdef _WIN32
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_joystick_box );
#endif
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_scale);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_red);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_green);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_blue);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_alpha);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_sensitivity);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menucursor_type);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menu_red);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menu_green);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menu_blue);
	Menu_AddItem( &s_options_menu, ( void * ) &s_q2p_menu_alpha);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_q2p_action);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_q2p_action_client);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_q2p_action_graphic);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_q2p_action_hud);
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_customize_options_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_defaults_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_console_action );
	
}

void Options_MenuDraw (void)
{
	M_Banner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_menu, 1 );
	Menu_Draw( &s_options_menu );
}

const char *Options_MenuKey( int key )
{
	return Default_MenuKey( &s_options_menu, key );
}

void M_Menu_Options_f (void)
{
	Options_MenuInit();
	M_PushMenu ( Options_MenuDraw, Options_MenuKey );
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

void M_Menu_Video_f (void)
{
	VID_MenuInit();
	M_PushMenu( VID_MenuDraw, VID_MenuKey );
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static const char **credits;
static char *creditsIndex[256];
static char *creditsBuffer;
static const char *idcredits[] =
{
	"^b^s^aQUAKE II BY ID SOFTWARE",
	"",
	"^b^s^aPROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	"^b^s^aART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	"^b^s^aLEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	"^b^s^aBIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	"^b^s^aSPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	"^b^s^aADDITIONAL SUPPORT",
	"",
	"^b^s^aLINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	"^b^s^aCINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	"^b^s^aSOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	"^b^s^aTHANKS TO ACTIVISION",
	"^b^s^aIN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char *xatcredits[] =
{
	"^b^s^aQUAKE II MISSION PACK: THE RECKONING",
	"^b^s^aBY",
	"^b^s^aXATRIX ENTERTAINMENT, INC.",
	"",
	"^b^s^aDESIGN AND DIRECTION",
	"Drew Markham",
	"",
	"^b^s^aPRODUCED BY",
	"Greg Goodrich",
	"",
	"^b^s^aPROGRAMMING",
	"Rafael Paiz",
	"",
	"^b^s^aLEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	"^b^s^aLEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	"^b^s^aART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	"^b^s^aCOMPUTER GRAPHICS SUPERVISOR AND",
	"^b^s^aCHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	"^b^s^aSENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	"^b^s^aCHARACTER ANIMATION AND",
	"^b^s^aMOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	"^b^s^aART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	"^b^s^aINTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	"^b^s^aADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	"^b^s^a3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	"^b^s^aADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	"^b^s^aADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	"^b^s^aPRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	"^b^s^aSOUND DESIGN",
	"Gary Bradfield",
	"",
	"^b^s^aMUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	"^b^s^aSPECIAL THANKS",
	"^b^s^aTO",
	"^b^s^aOUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	"^b^s^aTHANKS TO ACTIVISION",
	"^b^s^aIN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	"^b^s^aAND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	"^b^s^aTHANKS TO INTERGRAPH COMPUTER SYTEMS",
	"^b^s^aIN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char *roguecredits[] =
{
	"^b^s^aQUAKE II MISSION PACK 2: GROUND ZERO",
	"^b^s^aBY",
	"^b^s^aROGUE ENTERTAINMENT, INC.",
	"",
	"^b^s^aPRODUCED BY",
	"Jim Molinets",
	"",
	"^b^s^aPROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	"^b^s^aLEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	"^b^s^aART DIRECTION",
	"Rich Fleider",
	"",
	"^b^s^aART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	"^b^s^aANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	"^b^s^aADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	"^b^s^aSOUND",
	"James Grunke",
	"",
	"^b^s^aGROUND ZERO THEME",
	"^b^s^aAND",
	"^b^s^aMUSIC BY",
	"Sonic Mayhem",
	"",
	"^b^s^aVWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	"^b^s^aSPECIAL THANKS",
	"^b^s^aTO",
	"^b^s^aOUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	"^b^s^aTHANKS TO ACTIVISION",
	"^b^s^aIN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	"^b^s^aAND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

// Knigthtmare added- allow credits to scroll past top of screen
static int credits_start_line;
void M_Credits_MenuDraw( void )
{
	float alpha;
	int i, y, x, len;


	if ((viddef.height - ((cls.realtime - credits_start_time)/40.0F) + credits_start_line * ScaledVideo(MENU_FONT_SIZE+2)) < 0) {
		credits_start_line++;
		if (!credits[credits_start_line]) {
			credits_start_line = 0;
			credits_start_time = cls.realtime;
		}
	}

	/*
	** draw the credits
	*/
	for (i=credits_start_line, y=viddef.height - ((cls.realtime - credits_start_time)/40.0F) + credits_start_line * ScaledVideo(MENU_FONT_SIZE+2);
		credits[i] && y < viddef.height; y += ScaledVideo(MENU_FONT_SIZE+2), i++)
	{
		int	stringoffset = 0;
		int bold = false;

		if ( y <= -MENU_FONT_SIZE )
			continue;
		if ( y > viddef.height )
			continue;

		if ( credits[i][0] == '+' ) {
			bold = true;
			stringoffset = 1;
		}
		else {
			bold = false;
			stringoffset = 0;
		}

		if (y > 7*viddef.height/8) {
			float y_test, h_test;
			y_test = y - (7.0/8.0)*viddef.height;
			h_test = viddef.height/8;

			alpha = 1 - (y_test/h_test);

			if (alpha>1) alpha=1; if (alpha<0) alpha=0;
		}
		else if (y < viddef.height/8) {
			float y_test, h_test;
			y_test = y;
			h_test = viddef.height/8;

			alpha = y_test/h_test;

			if (alpha>1) alpha=1; if (alpha<0) alpha=0;
		}
		else
			alpha = 1;

		len = strlen(credits[i]) - StringLengthExtra(credits[i]);

		x = (viddef.width - len * ScaledVideo(MENU_FONT_SIZE) - stringoffset * ScaledVideo(MENU_FONT_SIZE) ) / 2 + ( stringoffset ) * ScaledVideo(MENU_FONT_SIZE);
		Menu_DrawString(x, y, credits[i], alpha*255);
	}
}

const char *M_Credits_Key( int key )
{
	switch (key)
	{
	case K_ESCAPE:
		if (creditsBuffer)
			FS_FreeFile (creditsBuffer);
		M_PopMenu ();
		break;
	}

	return menu_out_sound;

}

extern int Developer_searchpath (int who);

void M_Menu_Credits_f( void )
{
	int		n;
	int		count;
	char	*p;
	int		isdeveloper = 0;

	creditsBuffer = NULL;
	count = FS_LoadFile ("credits", (void **)&creditsBuffer);
	if (count != -1)
	{
		p = creditsBuffer;
		for (n = 0; n < 255; n++)
		{
			creditsIndex[n] = p;
			while (*p != '\r' && *p != '\n')
			{
				p++;
				if (--count == 0)
					break;
			}
			if (*p == '\r')
			{
				*p++ = 0;
				if (--count == 0)
					break;
			}
			*p++ = 0;
			if (--count == 0)
				break;
		}
		creditsIndex[++n] = 0;
		credits = (const char **)creditsIndex;
	}
	else
	{
		isdeveloper = Developer_searchpath (1);
		
		if (modType("xatrix"))
			credits = xatcredits;
		else if (modType("rogue"))
			credits = roguecredits;
		else
			credits = idcredits;	

	}

	credits_start_time = cls.realtime;
	credits_start_line = 0; // Knightmare- allow credits to scroll past top of screen
	M_PushMenu( M_Credits_MenuDraw, M_Credits_Key);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static int		m_game_cursor;

static menuframework_s	s_game_menu;
static menuaction_s		s_easy_game_action;
static menuaction_s		s_medium_game_action;
static menuaction_s		s_hard_game_action;
static menuaction_s 		s_nightmare_game_action;
static menuaction_s		s_load_game_action;
static menuaction_s		s_save_game_action;
static menuaction_s		s_credits_action;
static menuseparator_s	s_blankline;

static void StartGame( void )
{
	// disable updates and start the cinematic going
	cl.servercount = -1;
	M_ForceMenuOff ();
	Cvar_SetValue( "deathmatch", 0 );
	Cvar_SetValue( "coop", 0 );

	Cvar_SetValue( "gamerules", 0 );		//PGM

	Cbuf_AddText ("loading ; killserver ; wait ; newgame\n");
	cls.key_dest = key_game;
}

static void EasyGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "0" );
	StartGame();
}

static void MediumGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "1" );
	StartGame();
}

static void HardGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "2" );
	StartGame();
}

static void NightmareGameFunc( void *data )
{
	Cvar_ForceSet("skill", "3");
	StartGame();
}

static void LoadGameFunc( void *unused )
{
	M_Menu_LoadGame_f ();
}

static void SaveGameFunc( void *unused )
{
	M_Menu_SaveGame_f();
}

static void CreditsFunc( void *unused )
{
	M_Menu_Credits_f();
}

void Game_MenuInit( void )
{
	int y = 0;

	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	s_easy_game_action.generic.type	= MTYPE_ACTION;
	s_easy_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x		= 0;
	s_easy_game_action.generic.y		= y;
	s_easy_game_action.generic.name	= "Easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type	= MTYPE_ACTION;
	s_medium_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x		= 0;
	s_medium_game_action.generic.y		= y += (MENU_FONT_SIZE+2);
	s_medium_game_action.generic.name	= "Medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type	= MTYPE_ACTION;
	s_hard_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x		= 0;
	s_hard_game_action.generic.y		= y += (MENU_FONT_SIZE+2);
	s_hard_game_action.generic.name	= "Hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_nightmare_game_action.generic.type = MTYPE_ACTION;
	s_nightmare_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_nightmare_game_action.generic.x 	= 0;
	s_nightmare_game_action.generic.y 	= y += (MENU_FONT_SIZE+2);
	s_nightmare_game_action.generic.name = "Nightmare";
	s_nightmare_game_action.generic.callback = NightmareGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_load_game_action.generic.type	= MTYPE_ACTION;
	s_load_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x		= 0;
	s_load_game_action.generic.y		= y += 2*(MENU_FONT_SIZE+2);
	s_load_game_action.generic.name	= "Load game";
	s_load_game_action.generic.callback = LoadGameFunc;

	s_save_game_action.generic.type	= MTYPE_ACTION;
	s_save_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x		= 0;
	s_save_game_action.generic.y		= y += (MENU_FONT_SIZE+2);
	s_save_game_action.generic.name	= "Save game";
	s_save_game_action.generic.callback = SaveGameFunc;

	s_credits_action.generic.type	= MTYPE_ACTION;
	s_credits_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x		= 0;
	s_credits_action.generic.y		= y += (MENU_FONT_SIZE+2);
	s_credits_action.generic.name	= "Credits";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem( &s_game_menu, ( void * ) &s_easy_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_medium_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_hard_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_nightmare_game_action);
	Menu_AddItem( &s_game_menu, ( void * ) &s_blankline );
	Menu_AddItem( &s_game_menu, ( void * ) &s_load_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_save_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_blankline );
	Menu_AddItem( &s_game_menu, ( void * ) &s_credits_action );

	Menu_Center( &s_game_menu );
}

void Game_MenuDraw( void )
{
	M_Banner( "m_banner_game" );
//	Menu_AdjustCursor( &s_game_menu, 1 );
	Menu_Draw( &s_game_menu );
}

const char *Game_MenuKey( int key )
{
	return Default_MenuKey( &s_game_menu, key );
}

void M_Menu_Game_f (void)
{
	Game_MenuInit();
	M_PushMenu( Game_MenuDraw, Game_MenuKey );
	m_game_cursor = 1;
}

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

#define	MAX_SAVEGAMES	15

static menuframework_s	s_savegame_menu;

static menuframework_s	s_loadgame_menu;
static menuaction_s		s_loadgame_actions[MAX_SAVEGAMES];

char		m_savestrings[MAX_SAVEGAMES][32];
qboolean	m_savevalid[MAX_SAVEGAMES];

void Create_Savestrings (void)
{
	int		i;
	fileHandle_t	f;
	char	name[MAX_OSPATH];

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		Com_sprintf(name, sizeof(name), "save/save%i/server.ssv", i);
		FS_FOpenFile(name, &f, FS_READ);
		if (!f)
		{
			strcpy (m_savestrings[i], "<EMPTY>");
			m_savevalid[i] = false;
		}
		else
		{
			FS_Read (m_savestrings[i], sizeof(m_savestrings[i]), f);
			FS_FCloseFile(f);
			m_savevalid[i] = true;
		}
	}
}

void LoadGameCallback( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;

	if ( m_savevalid[ a->generic.localdata[0] ] )
		Cbuf_AddText (va("load save%i\n",  a->generic.localdata[0] ) );
	M_ForceMenuOff ();
}

void LoadGame_MenuInit( void )
{
	int i;

	s_loadgame_menu.x = viddef.width / 2 - ScaledVideo(240);
	s_loadgame_menu.y = viddef.height / 2 - ScaledVideo(7.25*MENU_FONT_SIZE);
	s_loadgame_menu.nitems = 0;

	Create_Savestrings();

	for ( i = 0; i < MAX_SAVEGAMES; i++ )
	{
		s_loadgame_actions[i].generic.name			= m_savestrings[i];
		s_loadgame_actions[i].generic.flags			= QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0]	= i;
		s_loadgame_actions[i].generic.callback		= LoadGameCallback;

		s_loadgame_actions[i].generic.x = 0;
		s_loadgame_actions[i].generic.y = (i) * (MENU_FONT_SIZE+2);
		if (i>0)	// separate from autosave
			s_loadgame_actions[i].generic.y += 10;

		s_loadgame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_loadgame_menu, &s_loadgame_actions[i] );
	}
}

void LoadGame_MenuDraw( void )
{
	M_Banner( "m_banner_load_game" );
	Menu_AdjustCursor( &s_loadgame_menu, 1 );
	Menu_Draw( &s_loadgame_menu );
}

const char *LoadGame_MenuKey( int key )
{
	if ( key == K_ESCAPE || key == K_ENTER )
	{
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if ( s_savegame_menu.cursor < 0 )
			s_savegame_menu.cursor = 0;
	}
	return Default_MenuKey( &s_loadgame_menu, key );
}

void M_Menu_LoadGame_f (void)
{
	LoadGame_MenuInit();
	M_PushMenu( LoadGame_MenuDraw, LoadGame_MenuKey );
}


/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/
static menuaction_s		s_savegame_actions[MAX_SAVEGAMES];

void SaveGameCallback( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;

	Cbuf_AddText (va("save save%i\n", a->generic.localdata[0] ));
	M_ForceMenuOff ();
}

void SaveGame_MenuDraw( void )
{
	M_Banner( "m_banner_save_game" );
	Menu_AdjustCursor( &s_savegame_menu, 1 );
	Menu_Draw( &s_savegame_menu );
}

void SaveGame_MenuInit( void )
{
	int i;

	s_savegame_menu.x = viddef.width / 2 - ScaledVideo(15*MENU_FONT_SIZE);	// was 120
	s_savegame_menu.y = viddef.height / 2 - ScaledVideo(7.25*MENU_FONT_SIZE);//was 58
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	// don't include the autosave slot
	for ( i = 0; i < MAX_SAVEGAMES-1; i++ )
	{
		s_savegame_actions[i].generic.name = m_savestrings[i+1];
		s_savegame_actions[i].generic.localdata[0] = i+1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = SaveGameCallback;

		s_savegame_actions[i].generic.x = 0;
		s_savegame_actions[i].generic.y = ( i ) * (MENU_FONT_SIZE+2);
		s_savegame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_savegame_menu, &s_savegame_actions[i] );
	}
}

const char *SaveGame_MenuKey( int key )
{
	if ( key == K_ENTER || key == K_ESCAPE )
	{
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if ( s_loadgame_menu.cursor < 0 )
			s_loadgame_menu.cursor = 0;
	}
	return Default_MenuKey( &s_savegame_menu, key );
}

void M_Menu_SaveGame_f (void)
{
	if (!Com_ServerState())
		return;		// not playing a game

	SaveGame_MenuInit();
	M_PushMenu( SaveGame_MenuDraw, SaveGame_MenuKey );
	Create_Savestrings ();
}


/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 9

static menuframework_s	s_joinserver_menu;
static menuseparator_s	s_joinserver_server_title;
static menuaction_s		s_joinserver_search_action;
static menuaction_s		s_joinserver_address_book_action;
static menuaction_s		s_joinserver_server_actions[MAX_LOCAL_SERVERS];

int		m_num_servers;
#define	NO_SERVER_STRING	"<no server>"

// user readable information
static char local_server_names[MAX_LOCAL_SERVERS][80];

// network address
static netadr_t local_server_netadr[MAX_LOCAL_SERVERS];

void M_AddToServerList (netadr_t adr, char *info)
{
	int		i;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;
	while ( *info == ' ' )
		info++;

	// ignore if duplicated
	for (i=0 ; i<m_num_servers ; i++)
		if (!strcmp(info, local_server_names[i]))
			return;

	local_server_netadr[m_num_servers] = adr;
	strncpy (local_server_names[m_num_servers], info, sizeof(local_server_names[0])-1);
	m_num_servers++;
}


void JoinServerFunc( void *self )
{
	char	buffer[128];
	int		ser_index;

	ser_index = ( menuaction_s * ) self - s_joinserver_server_actions;

	if ( Q_stricmp( local_server_names[ser_index], NO_SERVER_STRING ) == 0 )
		return;

	if (ser_index >= m_num_servers)
		return;

	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (local_server_netadr[ser_index]));
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

void AddressBookFunc( void *self )
{
	M_Menu_AddressBook_f();
}

void NullCursorDraw( void *self )
{
}

void SearchLocalGames( void )
{
	int		i;

	m_num_servers = 0;
	for (i=0 ; i<MAX_LOCAL_SERVERS ; i++)
		strcpy (local_server_names[i], NO_SERVER_STRING);

	M_DrawTextBox( 8, 120 - 48, 36, 3 );
	M_Print( 28, 120 - 48 + MENU_FONT_SIZE,  "Searching for local servers, this" );
	M_Print( 28, 120 - 48 + MENU_FONT_SIZE*2, "could take up to a minute, so" );
	M_Print( 28, 120 - 48 + MENU_FONT_SIZE*3, "please be patient." );
	// the text box won't show up unless we do a buffer swap
	re.EndFrame();

	// send out info packets
	CL_PingServers_f();
}

void SearchLocalGamesFunc( void *self )
{
	SearchLocalGames();
}

void JoinServer_MenuInit( void )
{
	int i;
	int y = 30;
	
	s_joinserver_menu.x = viddef.width * 0.50 - ScaledVideo(120);
	s_joinserver_menu.nitems = 0;

	s_joinserver_address_book_action.generic.type	= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name	= "Address Book";
	s_joinserver_address_book_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x		= 0;
	s_joinserver_address_book_action.generic.y		= y;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name	= "Refresh Server list";
	s_joinserver_search_action.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x	= 0;
	s_joinserver_search_action.generic.y	= y += (MENU_FONT_SIZE+2);
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "^3Search for Servers";

	s_joinserver_server_title.generic.type = MTYPE_SEPARATOR;
	s_joinserver_server_title.generic.name = "Connect to...";
	s_joinserver_server_title.generic.x    = 80;
	s_joinserver_server_title.generic.y	   = y += (MENU_FONT_SIZE+2);

	y += (MENU_FONT_SIZE+2);
	for ( i = 0; i < MAX_LOCAL_SERVERS; i++ )
	{
		s_joinserver_server_actions[i].generic.type	= MTYPE_ACTION;
		strcpy (local_server_names[i], NO_SERVER_STRING);
		s_joinserver_server_actions[i].generic.name	= local_server_names[i];
		s_joinserver_server_actions[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.x		= 0;
		s_joinserver_server_actions[i].generic.y		= y + i*(MENU_FONT_SIZE+2);
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[i].generic.statusbar = "^3Press ENTER to connect";
	}

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_address_book_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_title );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_search_action );

	for ( i = 0; i < 8; i++ )
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_actions[i] );

	Menu_Center( &s_joinserver_menu );

	SearchLocalGames();
}

void JoinServer_MenuDraw(void)
{
	M_Banner( "m_banner_join_server" );
	Menu_Draw( &s_joinserver_menu );
}


const char *JoinServer_MenuKey( int key )
{
	return Default_MenuKey( &s_joinserver_menu, key );
}

void M_Menu_JoinServer_f (void)
{
	JoinServer_MenuInit();
	M_PushMenu( JoinServer_MenuDraw, JoinServer_MenuKey );
}


/*
=============================================================================

START SERVER MENU

=============================================================================
*/
static menuframework_s s_startserver_menu;
static char **mapnames;
static int	  nummaps;

static menuaction_s	s_startserver_start_action;
static menuaction_s	s_startserver_dmoptions_action;
static menufield_s	s_timelimit_field;
static menufield_s	s_fraglimit_field;
static menufield_s	s_maxclients_field;
static menufield_s	s_hostname_field;
static menulist_s	s_startmap_list;
static menulist_s	s_rules_box;
static menulist_s	s_dedicated_box;

void DMOptionsFunc( void *self )
{
	if (s_rules_box.curvalue == 1)
		return;
	M_Menu_DMOptions_f();
}

void RulesChangeFunc ( void *self )
{
	// DM
	if (s_rules_box.curvalue == 0)
	{
		s_maxclients_field.generic.statusbar = NULL;
		s_startserver_dmoptions_action.generic.statusbar = NULL;
	}
	else if(s_rules_box.curvalue == 1)		// coop				// PGM
	{
		s_maxclients_field.generic.statusbar = "^34 Maximum for Cooperative";
		if (atoi(s_maxclients_field.buffer) > 4)
			strcpy( s_maxclients_field.buffer, "4" );
		s_startserver_dmoptions_action.generic.statusbar = "^3N/A for Cooperative";
	}
//=====
//PGM
	// ROGUE GAMES
	else if(Developer_searchpath(2) == 2)
	{
		if (s_rules_box.curvalue == 2)			// tag	
		{
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
/*
		else if(s_rules_box.curvalue == 3)		// deathball
		{
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
*/
	}
//PGM
//=====
}

void StartServerActionFunc( void *self )
{
	char	startmap[1024];
	int		timelimit;
	int		fraglimit;
	int		maxclients;
	char	*spot;

	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );

	maxclients  = atoi( s_maxclients_field.buffer );
	timelimit	= atoi( s_timelimit_field.buffer );
	fraglimit	= atoi( s_fraglimit_field.buffer );

	Cvar_SetValue( "maxclients", ClampCvar( 0, maxclients, maxclients ) );
	Cvar_SetValue ("timelimit", ClampCvar( 0, timelimit, timelimit ) );
	Cvar_SetValue ("fraglimit", ClampCvar( 0, fraglimit, fraglimit ) );
	Cvar_Set("hostname", s_hostname_field.buffer );
//	Cvar_SetValue ("deathmatch", !s_rules_box.curvalue );
//	Cvar_SetValue ("coop", s_rules_box.curvalue );
	if(s_dedicated_box.curvalue) {
		Cvar_ForceSet("dedicated", "1");
		Cvar_Set("sv_maplist", startmap); 
	}

//PGM
	if((s_rules_box.curvalue < 2) || (Developer_searchpath(2) != 2))
	{
		Cvar_SetValue ("deathmatch", !s_rules_box.curvalue );
		Cvar_SetValue ("coop", s_rules_box.curvalue );
		Cvar_SetValue ("gamerules", 0 );
	}
	else
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for rogue games, right?
		Cvar_SetValue ("coop", 0 );			// FIXME - this might need to depend on which game we're running
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
//PGM

	spot = NULL;
	if (s_rules_box.curvalue == 1)		// PGM
	{
 		if(Q_stricmp(startmap, "bunk1") == 0)
  			spot = "start";
 		else if(Q_stricmp(startmap, "mintro") == 0)
  			spot = "start";
 		else if(Q_stricmp(startmap, "fact1") == 0)
  			spot = "start";
 		else if(Q_stricmp(startmap, "power1") == 0)
  			spot = "pstart";
 		else if(Q_stricmp(startmap, "biggun") == 0)
  			spot = "bstart";
 		else if(Q_stricmp(startmap, "hangar1") == 0)
  			spot = "unitstart";
 		else if(Q_stricmp(startmap, "city1") == 0)
  			spot = "unitstart";
 		else if(Q_stricmp(startmap, "boss1") == 0)
			spot = "bosstart";
	}

	if (spot)
	{
		if (Com_ServerState())
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText (va("gamemap \"*%s$%s\"\n", startmap, spot));
	}
	else
	{
		Cbuf_AddText (va("map %s\n", startmap));
	}

	M_ForceMenuOff ();
}

void StartServer_MenuInit( void )
{
	int y = 0;
	
	static const char *dm_coop_names[] =
	{
		"Deathmatch",
		"Cooperative",
		NULL
	};
//=======
//PGM
	static const char *dm_coop_names_rogue[] =
	{
		"Deathmatch",
		"Cooperative",
		"Tag",
//		"deathball",
		NULL
	};
	static const char *dedicated[] = 
	{
		"No",
		"Yes",
		NULL
	};
//PGM
//=======
	char *buffer;
	char  mapsname[1024];
	char *s;
	int length;
	int i;
	FILE *fp;

	/*
	** load the list of map names
	*/
	Com_sprintf( mapsname, sizeof( mapsname ), "%s/maps.lst", FS_Gamedir() );
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		if ( ( length = FS_LoadFile( "maps.lst", ( void ** ) &buffer ) ) == -1 )
			Com_Error( ERR_DROP, "Couldn't find maps.lst\n" );
	}
	else
	{
#ifdef _WIN32
		length = filelength( fileno( fp  ) );
#else
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
#endif
		buffer = Q_malloc( length );
		fread( buffer, length, 1, fp );
	}

	s = buffer;

	i = 0;
	while ( i < length )
	{
		if ( s[i] == '\r' )
			nummaps++;
		i++;
	}

	if ( nummaps == 0 )
		Com_Error( ERR_DROP, "No maps in maps.lst\n" );

	mapnames = Q_malloc( sizeof( char * ) * ( nummaps + 1 ) );
	memset( mapnames, 0, sizeof( char * ) * ( nummaps + 1 ) );

	s = buffer;

	for ( i = 0; i < nummaps; i++ )
	{
		char  shortname[MAX_TOKEN_CHARS];
		char  longname[MAX_TOKEN_CHARS];
		char  scratch[200];
		int		j, l;

		strcpy( shortname, COM_Parse( &s ) );
		l = strlen(shortname);
		for (j=0 ; j<l ; j++)
			shortname[j] = toupper(shortname[j]);
		strcpy( longname, COM_Parse( &s ) );
		Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", longname, shortname );

		mapnames[i] = Q_malloc( strlen( scratch ) + 1 );
		strcpy( mapnames[i], scratch );
	}
	mapnames[nummaps] = 0;

	if ( fp != 0 )
	{
		fp = 0;
		Q_free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}

	/*
	** initialize the menu stuff
	*/
	s_startserver_menu.x = viddef.width * 0.50;
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x	= 0;
	s_startmap_list.generic.y	= y;
	s_startmap_list.generic.name	= "Initial Map";
	s_startmap_list.itemnames = (const char **)mapnames;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x	= 0;
	s_rules_box.generic.y	= y += 2*(MENU_FONT_SIZE+2);
	s_rules_box.generic.name	= "Rules";
	
//PGM - rogue games only available with rogue DLL.
	if(Developer_searchpath(2) == 2)
		s_rules_box.itemnames = dm_coop_names_rogue;
	else
		s_rules_box.itemnames = dm_coop_names;
//PGM

	if (Cvar_VariableValue("coop"))
		s_rules_box.curvalue = 1;
	else
		s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "Time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x	= 0;
	s_timelimit_field.generic.y	= y += 2*MENU_FONT_SIZE;
	s_timelimit_field.generic.statusbar = "^30 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	strcpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit") );

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "Frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x	= 0;
	s_fraglimit_field.generic.y	= y += 2*MENU_FONT_SIZE;
	s_fraglimit_field.generic.statusbar = "^30 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.visible_length = 3;
	strcpy( s_fraglimit_field.buffer, Cvar_VariableString("fraglimit") );

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is. 
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "Max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x	= 0;
	s_maxclients_field.generic.y	= y += 2*MENU_FONT_SIZE;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.length = 3;
	s_maxclients_field.visible_length = 3;
	if ( Cvar_VariableValue( "maxclients" ) == 1 )
		strcpy( s_maxclients_field.buffer, "8" );
	else 
		strcpy( s_maxclients_field.buffer, Cvar_VariableString("maxclients") );

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "Hostname";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x	= 0;
	s_hostname_field.generic.y	= y += 2*MENU_FONT_SIZE;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	strcpy( s_hostname_field.buffer, Cvar_VariableString("hostname") );

	s_dedicated_box.generic.type = MTYPE_SPINCONTROL;
	s_dedicated_box.generic.x	= 0;
	s_dedicated_box.generic.y	= y += 2*MENU_FONT_SIZE;
	s_dedicated_box.generic.name = "Dedicated server";
	s_dedicated_box.itemnames = dedicated;
	s_dedicated_box.curvalue = 0;

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name	= " Deathmatch flags";
	s_startserver_dmoptions_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_dmoptions_action.generic.x	= 24;
	s_startserver_dmoptions_action.generic.y	= y += 2*MENU_FONT_SIZE;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name	= " Begin";
	s_startserver_start_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x	= 24;
	s_startserver_start_action.generic.y	= y += 2*MENU_FONT_SIZE;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	Menu_AddItem( &s_startserver_menu, &s_startmap_list );
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	Menu_AddItem( &s_startserver_menu, &s_timelimit_field );
	Menu_AddItem( &s_startserver_menu, &s_fraglimit_field );
	Menu_AddItem( &s_startserver_menu, &s_maxclients_field );
	Menu_AddItem( &s_startserver_menu, &s_hostname_field );
	//Menu_AddItem( &s_startserver_menu, &s_dedicated_box );
	Menu_AddItem( &s_startserver_menu, &s_startserver_dmoptions_action );
	Menu_AddItem( &s_startserver_menu, &s_startserver_start_action );

	Menu_Center( &s_startserver_menu );

	// call this now to set proper inital state
	RulesChangeFunc ( NULL );
}

void StartServer_MenuDraw(void)
{
	Menu_Draw( &s_startserver_menu );
}

const char *StartServer_MenuKey( int key )
{
	if ( key == K_ESCAPE )
	{
		if ( mapnames )
		{
			int i;

			for ( i = 0; i < nummaps; i++ )
				Q_free( mapnames[i] );
			Q_free( mapnames );
		}
		mapnames = 0;
		nummaps = 0;
	}

	return Default_MenuKey( &s_startserver_menu, key );
}

void M_Menu_StartServer_f (void)
{
	StartServer_MenuInit();
	M_PushMenu( StartServer_MenuDraw, StartServer_MenuKey );
}

/*
=============================================================================

DMOPTIONS BOOK MENU

=============================================================================
*/
static char dmoptions_statusbar[128];

static menuframework_s s_dmoptions_menu;

static menulist_s	s_friendlyfire_box;
static menulist_s	s_falls_box;
static menulist_s	s_weapons_stay_box;
static menulist_s	s_instant_powerups_box;
static menulist_s	s_powerups_box;
static menulist_s	s_health_box;
static menulist_s	s_spawn_farthest_box;
static menulist_s	s_teamplay_box;
static menulist_s	s_samelevel_box;
static menulist_s	s_force_respawn_box;
static menulist_s	s_armor_box;
static menulist_s	s_allow_exit_box;
static menulist_s	s_infinite_ammo_box;
static menulist_s	s_fixed_fov_box;
static menulist_s	s_quad_drop_box;

//ROGUE
static menulist_s	s_no_mines_box;
static menulist_s	s_no_nukes_box;
static menulist_s	s_stack_double_box;
static menulist_s	s_no_spheres_box;
//ROGUE

static void DMFlagCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;
	int flags;
	int bit = 0;

	flags = Cvar_VariableValue( "dmflags" );

	if ( f == &s_friendlyfire_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if ( f == &s_falls_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	}
	else if ( f == &s_weapons_stay_box ) 
	{
		bit = DF_WEAPONS_STAY;
	}
	else if ( f == &s_instant_powerups_box )
	{
		bit = DF_INSTANT_ITEMS;
	}
	else if ( f == &s_allow_exit_box )
	{
		bit = DF_ALLOW_EXIT;
	}
	else if ( f == &s_powerups_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	}
	else if ( f == &s_health_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	}
	else if ( f == &s_spawn_farthest_box )
	{
		bit = DF_SPAWN_FARTHEST;
	}
	else if ( f == &s_teamplay_box )
	{
		if ( f->curvalue == 1 )
		{
			flags |=  DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		}
		else if ( f->curvalue == 2 )
		{
			flags |=  DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		}
		else
		{
			flags &= ~( DF_MODELTEAMS | DF_SKINTEAMS );
		}

		goto setvalue;
	}
	else if ( f == &s_samelevel_box )
	{
		bit = DF_SAME_LEVEL;
	}
	else if ( f == &s_force_respawn_box )
	{
		bit = DF_FORCE_RESPAWN;
	}
	else if ( f == &s_armor_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	}
	else if ( f == &s_infinite_ammo_box )
	{
		bit = DF_INFINITE_AMMO;
	}
	else if ( f == &s_fixed_fov_box )
	{
		bit = DF_FIXED_FOV;
	}
	else if ( f == &s_quad_drop_box )
	{
		bit = DF_QUAD_DROP;
	}

//=======
//ROGUE
	else if (Developer_searchpath(2) == 2)
	{
		if ( f == &s_no_mines_box)
		{
			bit = DF_NO_MINES;
		}
		else if ( f == &s_no_nukes_box)
		{
			bit = DF_NO_NUKES;
		}
		else if ( f == &s_stack_double_box)
		{
			bit = DF_NO_STACK_DOUBLE;
		}
		else if ( f == &s_no_spheres_box)
		{
			bit = DF_NO_SPHERES;
		}
	}
//ROGUE
//=======

	if ( f )
	{
		if ( f->curvalue == 0 )
			flags &= ~bit;
		else
			flags |= bit;
	}

setvalue:
	Cvar_SetValue ("dmflags", flags);

	Com_sprintf( dmoptions_statusbar, sizeof( dmoptions_statusbar ), "dmflags = %d", flags );

}

void DMOptions_MenuInit( void )
{
	static const char *yes_no_names[] =
	{
		"No",
		"Yes",
		NULL
	};
	static const char *teamplay_names[] = 
	{
		"Disabled",
		"By Skin",
		"By Model",
		NULL
	};
	int dmflags = Cvar_VariableValue( "dmflags" );
	int y = 0;

	s_dmoptions_menu.x = viddef.width * 0.50;
	s_dmoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.x	= 0;
	s_falls_box.generic.y	= y;
	s_falls_box.generic.name	= "Falling damage";
	s_falls_box.generic.callback = DMFlagCallback;
	s_falls_box.itemnames = yes_no_names;
	s_falls_box.curvalue = ( dmflags & DF_NO_FALLING ) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.x	= 0;
	s_weapons_stay_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_weapons_stay_box.generic.name	= "Weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = ( dmflags & DF_WEAPONS_STAY ) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x	= 0;
	s_instant_powerups_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_instant_powerups_box.generic.name	= "Instant Powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = ( dmflags & DF_INSTANT_ITEMS ) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x	= 0;
	s_powerups_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_powerups_box.generic.name	= "Allow Powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = ( dmflags & DF_NO_ITEMS ) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x	= 0;
	s_health_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name	= "Allow Health";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = ( dmflags & DF_NO_HEALTH ) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x	= 0;
	s_armor_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_armor_box.generic.name	= "Allow Armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = ( dmflags & DF_NO_ARMOR ) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x	= 0;
	s_spawn_farthest_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_spawn_farthest_box.generic.name	= "Spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = ( dmflags & DF_SPAWN_FARTHEST ) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x	= 0;
	s_samelevel_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_samelevel_box.generic.name	= "Same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = ( dmflags & DF_SAME_LEVEL ) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x	= 0;
	s_force_respawn_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_force_respawn_box.generic.name	= "Force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = ( dmflags & DF_FORCE_RESPAWN ) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.x	= 0;
	s_teamplay_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_teamplay_box.generic.name	= "Teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemnames = teamplay_names;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x	= 0;
	s_allow_exit_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_allow_exit_box.generic.name	= "Allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = ( dmflags & DF_ALLOW_EXIT ) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x	= 0;
	s_infinite_ammo_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_infinite_ammo_box.generic.name	= "Infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = ( dmflags & DF_INFINITE_AMMO ) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.x	= 0;
	s_fixed_fov_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_fixed_fov_box.generic.name	= "Fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemnames = yes_no_names;
	s_fixed_fov_box.curvalue = ( dmflags & DF_FIXED_FOV ) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x	= 0;
	s_quad_drop_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_quad_drop_box.generic.name	= "Quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = ( dmflags & DF_QUAD_DROP ) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x	= 0;
	s_friendlyfire_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_friendlyfire_box.generic.name	= "Friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = ( dmflags & DF_NO_FRIENDLY_FIRE ) == 0;

//============
//ROGUE
	if(Developer_searchpath(2) == 2)
	{
		s_no_mines_box.generic.type = MTYPE_SPINCONTROL;
		s_no_mines_box.generic.x	= 0;
		s_no_mines_box.generic.y	= y += (MENU_FONT_SIZE+2);
		s_no_mines_box.generic.name	= "Remove mines";
		s_no_mines_box.generic.callback = DMFlagCallback;
		s_no_mines_box.itemnames = yes_no_names;
		s_no_mines_box.curvalue = ( dmflags & DF_NO_MINES ) != 0;

		s_no_nukes_box.generic.type = MTYPE_SPINCONTROL;
		s_no_nukes_box.generic.x	= 0;
		s_no_nukes_box.generic.y	= y += (MENU_FONT_SIZE+2);
		s_no_nukes_box.generic.name	= "Remove nukes";
		s_no_nukes_box.generic.callback = DMFlagCallback;
		s_no_nukes_box.itemnames = yes_no_names;
		s_no_nukes_box.curvalue = ( dmflags & DF_NO_NUKES ) != 0;

		s_stack_double_box.generic.type = MTYPE_SPINCONTROL;
		s_stack_double_box.generic.x	= 0;
		s_stack_double_box.generic.y	= y += (MENU_FONT_SIZE+2);
		s_stack_double_box.generic.name	= "2x/4x Stacking off";
		s_stack_double_box.generic.callback = DMFlagCallback;
		s_stack_double_box.itemnames = yes_no_names;
		s_stack_double_box.curvalue = ( dmflags & DF_NO_STACK_DOUBLE ) != 0;

		s_no_spheres_box.generic.type = MTYPE_SPINCONTROL;
		s_no_spheres_box.generic.x	= 0;
		s_no_spheres_box.generic.y	= y += (MENU_FONT_SIZE+2);
		s_no_spheres_box.generic.name	= "Remove spheres";
		s_no_spheres_box.generic.callback = DMFlagCallback;
		s_no_spheres_box.itemnames = yes_no_names;
		s_no_spheres_box.curvalue = ( dmflags & DF_NO_SPHERES ) != 0;

	}
//ROGUE
//============

	Menu_AddItem( &s_dmoptions_menu, &s_falls_box );
	Menu_AddItem( &s_dmoptions_menu, &s_weapons_stay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_instant_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_health_box );
	Menu_AddItem( &s_dmoptions_menu, &s_armor_box );
	Menu_AddItem( &s_dmoptions_menu, &s_spawn_farthest_box );
	Menu_AddItem( &s_dmoptions_menu, &s_samelevel_box );
	Menu_AddItem( &s_dmoptions_menu, &s_force_respawn_box );
	Menu_AddItem( &s_dmoptions_menu, &s_teamplay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_allow_exit_box );
	Menu_AddItem( &s_dmoptions_menu, &s_infinite_ammo_box );
	Menu_AddItem( &s_dmoptions_menu, &s_fixed_fov_box );
	Menu_AddItem( &s_dmoptions_menu, &s_quad_drop_box );
	Menu_AddItem( &s_dmoptions_menu, &s_friendlyfire_box );

//=======
//ROGUE
	if(Developer_searchpath(2) == 2)
	{
		Menu_AddItem( &s_dmoptions_menu, &s_no_mines_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_nukes_box );
		Menu_AddItem( &s_dmoptions_menu, &s_stack_double_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_spheres_box );
	}
//ROGUE
//=======

	Menu_Center( &s_dmoptions_menu );

	// set the original dmflags statusbar
	DMFlagCallback( 0 );
	Menu_SetStatusBar( &s_dmoptions_menu, dmoptions_statusbar );
}

void DMOptions_MenuDraw(void)
{
	Menu_Draw( &s_dmoptions_menu );
}

const char *DMOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_dmoptions_menu, key );
}

void M_Menu_DMOptions_f (void)
{
	DMOptions_MenuInit();
	M_PushMenu( DMOptions_MenuDraw, DMOptions_MenuKey );
}

/*
=============================================================================

DOWNLOADOPTIONS BOOK MENU

=============================================================================
*/
static menuframework_s s_downloadoptions_menu;

static menuseparator_s	s_download_title;
static menulist_s	s_allow_download_box;
static menulist_s	s_allow_download_maps_box;
static menulist_s	s_allow_download_models_box;
static menulist_s	s_allow_download_players_box;
static menulist_s	s_allow_download_sounds_box;

static void DownloadCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;

	if (f == &s_allow_download_box)
	{
		Cvar_SetValue("allow_download", f->curvalue);
	}

	else if (f == &s_allow_download_maps_box)
	{
		Cvar_SetValue("allow_download_maps", f->curvalue);
	}

	else if (f == &s_allow_download_models_box)
	{
		Cvar_SetValue("allow_download_models", f->curvalue);
	}

	else if (f == &s_allow_download_players_box)
	{
		Cvar_SetValue("allow_download_players", f->curvalue);
	}

	else if (f == &s_allow_download_sounds_box)
	{
		Cvar_SetValue("allow_download_sounds", f->curvalue);
	}
}

void DownloadOptions_MenuInit( void )
{
	static const char *yes_no_names[] =
	{
		"No",
		"Yes",
		NULL
	};
	int y = 0;

	s_downloadoptions_menu.x = viddef.width * 0.50;
	s_downloadoptions_menu.nitems = 0;

	s_download_title.generic.type = MTYPE_SEPARATOR;
	s_download_title.generic.name = "Download Options";
	s_download_title.generic.x    = 48;
	s_download_title.generic.y	 = y;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x	= 0;
	s_allow_download_box.generic.y	= y += 2*(MENU_FONT_SIZE+2);
	s_allow_download_box.generic.name	= "Allow downloading";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = (Cvar_VariableValue("allow_download") != 0);

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x	= 0;
	s_allow_download_maps_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_allow_download_maps_box.generic.name	= "Maps";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = (Cvar_VariableValue("allow_download_maps") != 0);

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x	= 0;
	s_allow_download_players_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_allow_download_players_box.generic.name	= "Player Models/Skins";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = (Cvar_VariableValue("allow_download_players") != 0);

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x	= 0;
	s_allow_download_models_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_allow_download_models_box.generic.name	= "Models";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = (Cvar_VariableValue("allow_download_models") != 0);

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x	= 0;
	s_allow_download_sounds_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_allow_download_sounds_box.generic.name	= "Sounds";
	s_allow_download_sounds_box.generic.callback = DownloadCallback;
	s_allow_download_sounds_box.itemnames = yes_no_names;
	s_allow_download_sounds_box.curvalue = (Cvar_VariableValue("allow_download_sounds") != 0);

	Menu_AddItem( &s_downloadoptions_menu, &s_download_title );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_maps_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_players_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_models_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_sounds_box );

	Menu_Center( &s_downloadoptions_menu );

	// skip over title
	if (s_downloadoptions_menu.cursor == 0)
		s_downloadoptions_menu.cursor = 1;
}

void DownloadOptions_MenuDraw(void)
{
	Menu_Draw( &s_downloadoptions_menu );
}

const char *DownloadOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_downloadoptions_menu, key );
}

void M_Menu_DownloadOptions_f (void)
{
	DownloadOptions_MenuInit();
	M_PushMenu( DownloadOptions_MenuDraw, DownloadOptions_MenuKey );
}
/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_s	s_addressbook_menu;
static menufield_s		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

void AddressBook_MenuInit( void )
{
	int i;


	s_addressbook_menu.x = viddef.width / 2 - ScaledVideo(MENU_FONT_SIZE*17.75);
	s_addressbook_menu.y = viddef.height / 2 - ScaledVideo(MENU_FONT_SIZE*7.25);
	s_addressbook_menu.nitems = 0;

	for ( i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++ )
	{
		cvar_t *adr;
		char buffer[20];

		Com_sprintf( buffer, sizeof( buffer ), "adr%d", i );

		adr = Cvar_Get( buffer, "", CVAR_ARCHIVE );

		s_addressbook_fields[i].generic.type = MTYPE_FIELD;
		s_addressbook_fields[i].generic.name = 0;
		s_addressbook_fields[i].generic.callback = 0;
		s_addressbook_fields[i].generic.x		= 0;
		s_addressbook_fields[i].generic.y		= i * 2.25*(MENU_FONT_SIZE+2) + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor			= 0;
		s_addressbook_fields[i].length			= 60;
		s_addressbook_fields[i].visible_length	= 30;

		strcpy( s_addressbook_fields[i].buffer, adr->string );

		Menu_AddItem( &s_addressbook_menu, &s_addressbook_fields[i] );
	}
}

const char *AddressBook_MenuKey( int key )
{
	if ( key == K_ESCAPE )
	{
		int add_index;
		char buffer[20];

		for ( add_index = 0; add_index < NUM_ADDRESSBOOK_ENTRIES; add_index++ )
		{
			Com_sprintf( buffer, sizeof( buffer ), "adr%d", add_index );
			Cvar_Set( buffer, s_addressbook_fields[add_index].buffer );
		}
	}
	return Default_MenuKey( &s_addressbook_menu, key );
}

void AddressBook_MenuDraw(void)
{
	M_Banner( "m_banner_addressbook" );
	Menu_Draw( &s_addressbook_menu );
}

void M_Menu_AddressBook_f(void)
{
	AddressBook_MenuInit();
	M_PushMenu( AddressBook_MenuDraw, AddressBook_MenuKey );
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
static menuframework_s	s_player_config_menu;
static menufield_s		s_player_name_field;
static menulist_s		s_player_model_box;
static menulist_s		s_player_skin_box;
static menulist_s		s_player_handedness_box;
static menulist_s		s_player_rate_box;
static menuseparator_s	s_player_skin_title;
static menuseparator_s	s_player_model_title;
static menuseparator_s	s_player_hand_title;
static menuseparator_s	s_player_rate_title;
static menuaction_s		s_player_download_action;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

typedef struct
{
	int		nskins;
	char	**skindisplaynames;
	char	displayname[MAX_DISPLAYNAME];
	char	directory[MAX_QPATH];
} playermodelinfo_s;

static playermodelinfo_s s_pmi[MAX_PLAYERMODELS];
static char *s_pmnames[MAX_PLAYERMODELS];
static int s_numplayermodels;

static int rate_tbl[] = { 2500, 3200, 5000, 10000, 25000, 0 };
static const char *rate_names[] = 
{
	"28.8 Modem",
	"33.6 Modem",
	"Single ISDN",
	"Dual ISDN/Cable",
	"T1/LAN",
	"User defined",
	NULL 
};

struct model_s *playermodel; 
struct model_s *weaponmodel; 
struct image_s *playerskin; 
struct image_s *weaponskin; 

void DownloadOptionsFunc( void *self )
{
	M_Menu_DownloadOptions_f();
}

static void HandednessCallback( void *unused )
{
	Cvar_SetValue( "hand", s_player_handedness_box.curvalue );
}

static void RateCallback( void *unused )
{
	if (s_player_rate_box.curvalue != sizeof(rate_tbl) / sizeof(*rate_tbl) - 1)
		Cvar_SetValue( "rate", rate_tbl[s_player_rate_box.curvalue] );
}

static void ModelCallback( void *unused )
{
	char scratch[MAX_QPATH];
	
	s_player_skin_box.itemnames = (const char **)s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;
	
	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/tris.md2", s_pmi[s_player_model_box.curvalue].directory);
	playermodel = re.RegisterModel(scratch);
	
	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/%s.pcx", s_pmi[s_player_model_box.curvalue].directory,
		    s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);
	playerskin = re.RegisterSkin(scratch);
	
	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/weapon.md2", s_pmi[s_player_model_box.curvalue].directory);
	weaponmodel = re.RegisterModel(scratch);
	
	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/weapon.pcx", s_pmi[s_player_model_box.curvalue].directory);
	weaponskin = re.RegisterSkin(scratch);
}

static void SkinCallback(void *unused)
{
	char scratch[MAX_QPATH];

	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/%s.pcx", s_pmi[s_player_model_box.curvalue].directory,
		    s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );
	
	playerskin = re.RegisterSkin(scratch);
}

void FreeFileList( char **list, int n )
{
	int i;

	for ( i = 0; i < n; i++ )
	{
		if ( list[i] )
		{
			Q_free( list[i] );
			list[i] = 0;
		}
	}
	Q_free( list );
}

static qboolean IconOfSkinExists( char *skin, char **pcxfiles, int npcxfiles )
{
	int i;
	char scratch[1024];

	strcpy( scratch, skin );
	*strrchr( scratch, '.' ) = 0;
	strcat( scratch, "_i.pcx" );

	for ( i = 0; i < npcxfiles; i++ )
	{
		if ( strcmp( pcxfiles[i], scratch ) == 0 )
			return true;
	}

	return false;
}

static void PlayerConfig_ScanDirectories( void )
{
	char findname[1024];
	char scratch[1024];
	int ndirs = 0, npms = 0;
	char **dirnames;
	char *path = NULL;
	int i;

	s_numplayermodels = 0;

	/*
	** get a list of directories
	*/
	do 
	{
		path = FS_NextPath( path );
		Com_sprintf( findname, sizeof(findname), "%s/players/*.*", path );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, SFF_SUBDIR, 0 ) ) != 0 )
			break;
	} while ( path );

	if ( !dirnames )
		return;

	/*
	** go through the subdirectories
	*/
	npms = ndirs;
	if ( npms > MAX_PLAYERMODELS )
		npms = MAX_PLAYERMODELS;

	for ( i = 0; i < npms; i++ )
	{
		int k, s;
		char *aa, *bb, *cc;
		char **pcxnames;
		char **skinnames;
		int npcxfiles;
		int nskins = 0;

		if ( dirnames[i] == 0 )
			continue;

		// verify the existence of tris.md2
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/tris.md2" );
		if ( !Sys_FindFirst( scratch, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ) )
		{
			Q_free( dirnames[i] );
			dirnames[i] = 0;
			Sys_FindClose();
			continue;
		}
		Sys_FindClose();

		// verify the existence of at least one pcx skin
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/*.pcx" );
		pcxnames = FS_ListFiles( scratch, &npcxfiles, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if ( !pcxnames )
		{
			Q_free( dirnames[i] );
			dirnames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for ( k = 0; k < npcxfiles-1; k++ )
		{
			if ( !strstr( pcxnames[k], "_i.pcx" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
				{
					nskins++;
				}
			}
		}
		if ( !nskins )
			continue;

		skinnames = Q_malloc( sizeof( char * ) * ( nskins + 1 ) );
		memset( skinnames, 0, sizeof( char * ) * ( nskins + 1 ) );

		// copy the valid skins
		for ( s = 0, k = 0; k < npcxfiles-1; k++ )
		{
			char *a, *b, *c;

			if ( !strstr( pcxnames[k], "_i.pcx" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
				{
					a = strrchr( pcxnames[k], '/' );
					b = strrchr( pcxnames[k], '\\' );

					if ( a > b )
						c = a;
					else
						c = b;

					strcpy( scratch, c + 1 );

					if ( strrchr( scratch, '.' ) )
						*strrchr( scratch, '.' ) = 0;

					skinnames[s] = strdup( scratch );
					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[s_numplayermodels].nskins = nskins;
		s_pmi[s_numplayermodels].skindisplaynames = skinnames;

		// make short name for the model
		aa = strrchr( dirnames[i], '/' );
		bb = strrchr( dirnames[i], '\\' );

		if ( aa > bb )
			cc = aa;
		else
			cc = bb;

		strncpy( s_pmi[s_numplayermodels].displayname, cc + 1, MAX_DISPLAYNAME-1 );
		strcpy( s_pmi[s_numplayermodels].directory, cc + 1 );

		FreeFileList( pcxnames, npcxfiles );

		s_numplayermodels++;
	}
	if ( dirnames )
		FreeFileList( dirnames, ndirs );
}

static int pmicmpfnc( const void *_a, const void *_b )
{
	const playermodelinfo_s *a = ( const playermodelinfo_s * ) _a;
	const playermodelinfo_s *b = ( const playermodelinfo_s * ) _b;

	/*
	** sort by male, female, then alphabetical
	*/
	if ( strcmp( a->directory, "male" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "male" ) == 0 )
		return 1;

	if ( strcmp( a->directory, "female" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "female" ) == 0 )
		return 1;

	return strcmp( a->directory, b->directory );
}

qboolean PlayerConfig_MenuInit( void )
{
	extern cvar_t *name;
	extern cvar_t *skin;
	char currentdirectory[1024];
	char currentskin[1024];
	int i = 0;
	int y = 0;
	char scratch[MAX_QPATH];

	int currentdirectoryindex = 0;
	int currentskinindex = 0;

	cvar_t *hand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	static const char *handedness[] = { "right", "left", "center", 0 };

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	if ( hand->value < 0 || hand->value > 2 )
		Cvar_SetValue( "hand", 0 );

	strcpy( currentdirectory, skin->string );

	if ( strchr( currentdirectory, '/' ) ) {
		strcpy( currentskin, strchr( currentdirectory, '/' ) + 1 );
		*strchr( currentdirectory, '/' ) = 0;
	}
	else if ( strchr( currentdirectory, '\\' ) ) {
		strcpy( currentskin, strchr( currentdirectory, '\\' ) + 1 );
		*strchr( currentdirectory, '\\' ) = 0;
	}
	else {
		strcpy( currentdirectory, "male" );
		strcpy( currentskin, "grunt" );
	}

	qsort( s_pmi, s_numplayermodels, sizeof( s_pmi[0] ), pmicmpfnc );

	memset( s_pmnames, 0, sizeof( s_pmnames ) );
	for ( i = 0; i < s_numplayermodels; i++ ) {
		s_pmnames[i] = s_pmi[i].displayname;
		if ( Q_stricmp( s_pmi[i].directory, currentdirectory ) == 0 ) {
			int j;

			currentdirectoryindex = i;

			for ( j = 0; j < s_pmi[i].nskins; j++ ) {
				if ( Q_stricmp( s_pmi[i].skindisplaynames[j], currentskin ) == 0 ) {
					currentskinindex = j;
					break;
				}
			}
		}
	}

	s_player_config_menu.x = viddef.width  / 2 - ScaledVideo(250);
	s_player_config_menu.y = viddef.height / 2 - ScaledVideo(70.0);
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "Name";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x		= -MENU_FONT_SIZE;
	s_player_name_field.generic.y		= y;
	s_player_name_field.length	= 15;
	s_player_name_field.visible_length = 15;
	strcpy( s_player_name_field.buffer, name->string );
	s_player_name_field.cursor = strlen( name->string );

	s_player_model_title.generic.type = MTYPE_SEPARATOR;
	s_player_model_title.generic.name = "Model";
	s_player_model_title.generic.x    = -MENU_FONT_SIZE;
	s_player_model_title.generic.y	 = y += 3*(MENU_FONT_SIZE+2);

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.x	= -7*MENU_FONT_SIZE;
	s_player_model_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -6*MENU_FONT_SIZE;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char **)s_pmnames;

	s_player_skin_title.generic.type = MTYPE_SEPARATOR;
	s_player_skin_title.generic.name = "Skin";
	s_player_skin_title.generic.x    = -2*MENU_FONT_SIZE;
	s_player_skin_title.generic.y	 = y += 2*(MENU_FONT_SIZE+2);
	
	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.x	= -7*MENU_FONT_SIZE;
	s_player_skin_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_player_skin_box.generic.name	= 0;
	s_player_skin_box.generic.callback = SkinCallback;
	s_player_skin_box.generic.cursor_offset = -6*MENU_FONT_SIZE;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char **)s_pmi[currentdirectoryindex].skindisplaynames;

	s_player_hand_title.generic.type = MTYPE_SEPARATOR;
	s_player_hand_title.generic.name = "Handedness";
	s_player_hand_title.generic.x    = 4*MENU_FONT_SIZE;
	s_player_hand_title.generic.y	 = y += 2*(MENU_FONT_SIZE+2);

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.x	= -7*MENU_FONT_SIZE;
	s_player_handedness_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_player_handedness_box.generic.name	= 0;
	s_player_handedness_box.generic.cursor_offset = -6*MENU_FONT_SIZE;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableValue( "hand" );
	s_player_handedness_box.itemnames = handedness;

	for (i = 0; i < sizeof(rate_tbl) / sizeof(*rate_tbl) - 1; i++)
		if (Cvar_VariableValue("rate") == rate_tbl[i])
			break;

	s_player_rate_title.generic.type = MTYPE_SEPARATOR;
	s_player_rate_title.generic.name = "Connect speed";
	s_player_rate_title.generic.x    = 7*MENU_FONT_SIZE;
	s_player_rate_title.generic.y	 = y += 2*(MENU_FONT_SIZE+2);

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x	= -7*MENU_FONT_SIZE;
	s_player_rate_box.generic.y	= y += (MENU_FONT_SIZE+2);
	s_player_rate_box.generic.name	= 0;
	s_player_rate_box.generic.cursor_offset = -6*MENU_FONT_SIZE;
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_player_download_action.generic.type = MTYPE_ACTION;
	s_player_download_action.generic.name	= "Download options";
	s_player_download_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_player_download_action.generic.x	= -5*MENU_FONT_SIZE;
	s_player_download_action.generic.y	= y += 2*(MENU_FONT_SIZE+2);
	s_player_download_action.generic.statusbar = NULL;
	s_player_download_action.generic.callback = DownloadOptionsFunc;
	
	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/tris.md2", s_pmi[s_player_model_box.curvalue].directory);
	playermodel = re.RegisterModel(scratch);

	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/%s.pcx", s_pmi[s_player_model_box.curvalue].directory,
		    s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);
	playerskin = re.RegisterSkin(scratch);

	Com_sprintf(scratch, sizeof(scratch),
	            "players/%s/weapon.md2", s_pmi[s_player_model_box.curvalue].directory);
	weaponmodel = re.RegisterModel(scratch);

	Menu_AddItem( &s_player_config_menu, &s_player_name_field );
	Menu_AddItem( &s_player_config_menu, &s_player_model_title );
	Menu_AddItem( &s_player_config_menu, &s_player_model_box );
	if ( s_player_skin_box.itemnames )
	{
		Menu_AddItem( &s_player_config_menu, &s_player_skin_title );
		Menu_AddItem( &s_player_config_menu, &s_player_skin_box );
	}
	Menu_AddItem( &s_player_config_menu, &s_player_hand_title );
	Menu_AddItem( &s_player_config_menu, &s_player_handedness_box );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_title );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_box );
	Menu_AddItem( &s_player_config_menu, &s_player_download_action );

	return true;
}

void PlayerConfig_MenuDraw( void )
{
	extern float CalcFov(float fov_x, float w, float h);
	refdef_t refdef;
	char scratch[MAX_QPATH];

	memset(&refdef, 0, sizeof(refdef));
	
	refdef.width = viddef.width;
	refdef.height = viddef.height;
	refdef.x = 0;
	refdef.y = 0;
	refdef.fov_x = 90;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime*0.001;

	if ( s_pmi[s_player_model_box.curvalue].skindisplaynames ) {
	
		static int yaw;
		static int mframe;
		entity_t entity[2];
		int w, h;

		memset(&entity, 0, sizeof(entity));
		
		yaw += 0.5;
		if (++yaw > 360) yaw = 0;
		
		mframe += 1;
		if (mframe > 390) mframe = 0;

		entity[0].model = playermodel;
		entity[0].skin = playerskin;
		
		entity[1].model = weaponmodel;
		entity[1].skin = weaponskin;

		entity[0].flags = RF_FULLBRIGHT|RF_DEPTHHACK;
		entity[0].origin[0] = 80;
		entity[0].origin[1] = 0;
		entity[0].origin[2] = 0;
		entity[0].frame = mframe/6; 
		entity[0].oldframe = 0;
		entity[0].backlerp = 0.0;
		entity[0].angles[1] = yaw;
		
		entity[1].flags = RF_FULLBRIGHT|RF_DEPTHHACK;
		entity[1].origin[0] = 80;
		entity[1].origin[1] = 0;
		entity[1].origin[2] = 0;
		entity[1].frame = mframe/6; 
		entity[1].oldframe = 0;
		entity[1].backlerp = 0.0;
		entity[1].angles[1] = yaw;
		
		VectorCopy(entity[0].origin, entity[0].oldorigin);
		VectorCopy(entity[1].origin, entity[1].oldorigin);

		refdef.areabits = 0;
		refdef.num_entities = 2;
		refdef.entities = entity;
		refdef.lightstyles = 0;
		refdef.rdflags = RDF_NOWORLDMODEL|RDF_NOCLEAR;
		
		Menu_Draw(&s_player_config_menu);
		
		refdef.height += 4;
		
		re.RenderFrame(&refdef);

		Com_sprintf(scratch, sizeof(scratch), "/players/%s/%s_i.pcx",
			    s_pmi[s_player_model_box.curvalue].directory,
			    s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);
			    
		//skin selection preview
		{
			float	icon_x = viddef.width  - ScaledVideo(325),
					icon_y = viddef.height - ScaledVideo(400),
					icon_offset = 0, icon_scale;
			int i, count;
			vec3_t color =	{
					 (int)(cl.stime/10)%255,
			                 255,
					 (int)(cl.stime/10)%255
					};

			if (s_pmi[s_player_model_box.curvalue].nskins<7 || s_player_skin_box.curvalue<4)
				i=0;
			else if (s_player_skin_box.curvalue>s_pmi[s_player_model_box.curvalue].nskins-4)
				i=s_pmi[s_player_model_box.curvalue].nskins-7;
			else
				i=s_player_skin_box.curvalue-3;

			if (i>0)
				re.DrawScaledChar(icon_x - ScaledVideo(37),
					          icon_y - ScaledVideo(30),
					          '<' , 3*VideoScale(),
						  color[0], color[1], color[2], 255, false);
				
			for (count=0; count<7;i++,count++) {
				if (i<0 || i>=s_pmi[s_player_model_box.curvalue].nskins)
					continue;

				icon_scale = (i==s_player_skin_box.curvalue)? 2: 1;

				Com_sprintf (scratch, sizeof(scratch), "/players/%s/%s_i.pcx", 
					s_pmi[s_player_model_box.curvalue].directory,
					s_pmi[s_player_model_box.curvalue].skindisplaynames[i] );
					
				re.DrawGetPicSize(&w, &h, scratch);
				
				icon_scale = icon_scale * (32.0/(float)w); // fix for smaller icons
				re.DrawStretchPic(icon_x + icon_offset, icon_y - ScaledVideo(32),
					          ScaledVideo(w)*icon_scale,
						  ScaledVideo(h)*icon_scale, scratch, 1.0);
				
				icon_offset += ScaledVideo(w)*icon_scale;
			}

			if (s_pmi[s_player_model_box.curvalue].nskins-i>0)
				re.DrawScaledChar(icon_x + icon_offset + ScaledVideo(5),
					          icon_y - ScaledVideo(30),
					          '>' , 3*VideoScale(),
						  color[0], color[1], color[2], 255, false);
		}
	}
}

const char *PlayerConfig_MenuKey (int key)
{
	int i;

	if ( key == K_ESCAPE ) {
		char scratch[1024];

		Cvar_Set( "name", s_player_name_field.buffer );

		Com_sprintf( scratch, sizeof( scratch ), "%s/%s", 
			s_pmi[s_player_model_box.curvalue].directory, 
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );

		Cvar_Set( "skin", scratch );

		for ( i = 0; i < s_numplayermodels; i++ ) {
			int j;

			for ( j = 0; j < s_pmi[i].nskins; j++ ) {
				if ( s_pmi[i].skindisplaynames[j] )
					Q_free( s_pmi[i].skindisplaynames[j] );
				s_pmi[i].skindisplaynames[j] = 0;
			}
			Q_free( s_pmi[i].skindisplaynames );
			s_pmi[i].skindisplaynames = 0;
			s_pmi[i].nskins = 0;
		}
	}
	return Default_MenuKey( &s_player_config_menu, key );
}


void M_Menu_PlayerConfig_f (void)
{
	if (!PlayerConfig_MenuInit()) {
		Menu_SetStatusBar( &s_multiplayer_menu, "No valid player models found" );
		return;
	}
	Menu_SetStatusBar( &s_multiplayer_menu, NULL );
	M_PushMenu( PlayerConfig_MenuDraw, PlayerConfig_MenuKey );
}

/*
=======================================================================

QUIT MENU

=======================================================================
*/


static menuframework_s	s_quit_menu;
static menuseparator_s	s_quit_question;
static menuaction_s	s_quit_yes_action;
static menuaction_s	s_quit_no_action;

const char *M_Quit_Key (int key)
{
	if (menu_quit_yesno->value)
		return Default_MenuKey( &s_quit_menu, key );
	else {
		switch (key) {
			case K_ESCAPE:
			case 'n':
			case 'N':
				M_PopMenu ();
				break;

			case 'Y':
			case 'y':
				cls.key_dest = key_console;
				CL_Quit_f ();
				break;

			default:
				break;
		}
		return NULL;
	}
}

void M_Quit_Draw (void)
{
	int		w, h;
	
	if (menu_quit_yesno->value) {
		M_Banner( "m_banner_quit" );
		Menu_AdjustCursor( &s_quit_menu, 1 );
		Menu_Draw( &s_quit_menu );
	}
	else {
		re.DrawGetPicSize(&w, &h, "quit");
		re.DrawStretchPic((viddef.width-ScaledVideo(w))/2, 
		                  (viddef.height-ScaledVideo(h))/2,
				  ScaledVideo(w), ScaledVideo(h), "quit", 0.5f);
	}
}

void QuitActionNo (void *blah)
{
	M_PopMenu();
}

void QuitActionYes (void *blah)
{
	CL_Quit_f();
}

void Quit_MenuInit (void)
{
	s_quit_menu.x = viddef.width * 0.50 - ScaledVideo(20);
	s_quit_menu.y = viddef.height * 0.50 - ScaledVideo(58);
	s_quit_menu.nitems = 0;

	s_quit_question.generic.type	= MTYPE_SEPARATOR;
	s_quit_question.generic.name	= "EXIT GAME?";
	s_quit_question.generic.x	= strlen(s_quit_question.generic.name)*MENU_FONT_SIZE*0.5;
	s_quit_question.generic.y	= MENU_FONT_SIZE*2;

	s_quit_yes_action.generic.type	= MTYPE_ACTION;
	s_quit_yes_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_quit_yes_action.generic.x	= 0;
	s_quit_yes_action.generic.y	= MENU_FONT_SIZE*5;
	s_quit_yes_action.generic.name	= "YES";
	s_quit_yes_action.generic.callback = QuitActionYes;

	s_quit_no_action.generic.type	= MTYPE_ACTION;
	s_quit_no_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_quit_no_action.generic.x	= 0;
	s_quit_no_action.generic.y	= MENU_FONT_SIZE*7;
	s_quit_no_action.generic.name	= "NO";
	s_quit_no_action.generic.callback = QuitActionNo;
	
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_question );
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_yes_action );
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_no_action );

	Menu_SetStatusBar( &s_quit_menu, NULL );

	Menu_Center( &s_quit_menu );
}

void M_Menu_Quit_f (void)
{
	Quit_MenuInit();
	M_PushMenu (M_Quit_Draw, M_Quit_Key);
}

void CheckQuitMenuMouse(void)
{
	int	maxs[2], mins[2];
	int	w, h;

	re.DrawGetPicSize(&w, &h, "quit");

	mins[0] = (viddef.width - w) / 2;
	maxs[0] = mins[0] + w;
	mins[1] = (viddef.height - h) / 2;
	maxs[1] = mins[1] + h;

	if (cursor.x >= mins[0] && cursor.x <= maxs[0] &&
	    cursor.y >= mins[1] && cursor.y <= maxs[1]) {
		if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] == 2) {
			M_PopMenu();
			cursor.buttonused[MOUSEBUTTON2] = true;
			cursor.buttonclicks[MOUSEBUTTON2] = 0;
		}
		if (!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1] == 2) {
			CL_Quit_f();
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
		}
	}
}



//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init (void)
{
	// Knightmare- init this cvar here so M_Print can use it
	if (!alt_text_color)
		alt_text_color = Cvar_Get ("alt_text_color", "2", CVAR_ARCHIVE);
		
	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_game", M_Menu_Game_f);
	Cmd_AddCommand ("menu_loadgame", M_Menu_LoadGame_f);
	Cmd_AddCommand ("menu_savegame", M_Menu_SaveGame_f);
	Cmd_AddCommand ("menu_joinserver", M_Menu_JoinServer_f);
	Cmd_AddCommand ("menu_addressbook", M_Menu_AddressBook_f);
	Cmd_AddCommand ("menu_startserver", M_Menu_StartServer_f);
	Cmd_AddCommand ("menu_dmoptions", M_Menu_DMOptions_f);
	Cmd_AddCommand ("menu_playerconfig", M_Menu_PlayerConfig_f);
	Cmd_AddCommand ("menu_downloadoptions", M_Menu_DownloadOptions_f);
	Cmd_AddCommand ("menu_credits", M_Menu_Credits_f );
	Cmd_AddCommand ("menu_multiplayer", M_Menu_Multiplayer_f );
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


/*
=================
M_Draw
=================
*/
void
refreshCursorMenu(void)
{
	cursor.menu = NULL;
}

void refreshCursorLink(void)
{
	cursor.menuitem = NULL;
}

int Slider_CursorPositionX(menuslider_s * s)
{
	float		range;

	range = (s->curvalue - s->minvalue) / (float)(s->maxvalue - s->minvalue);

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;

	return ( int )( ScaledVideo(MENU_FONT_SIZE) + RCOLUMN_OFFSET + (SLIDER_RANGE)*ScaledVideo(MENU_FONT_SIZE) * range );
}

int newSliderValueForX(int x, menuslider_s * s)
{
	float	newValue;
	int	newValueInt;
	int	pos = x - ScaledVideo(MENU_FONT_SIZE + RCOLUMN_OFFSET + s->generic.x) - s->generic.parent->x;
	
	newValue = ((float)pos)/((SLIDER_RANGE-1)*ScaledVideo(MENU_FONT_SIZE));
	newValueInt = s->minvalue + newValue * (float)(s->maxvalue - s->minvalue);

	return newValueInt;
}

void Slider_CheckSlide(menuslider_s * s)
{
	if (s->curvalue > s->maxvalue)
		s->curvalue = s->maxvalue;
	else if (s->curvalue < s->minvalue)
		s->curvalue = s->minvalue;

	if (s->generic.callback)
		s->generic.callback(s);
}

void Menu_DragSlideItem(menuframework_s * menu, void *menuitem)
{
	menuslider_s   *slider = (menuslider_s *) menuitem;

	slider->curvalue = newSliderValueForX(cursor.x, slider);
	Slider_CheckSlide(slider);
}

void Menu_ClickSlideItem(menuframework_s * menu, void *menuitem)
{
	int		min       , max;
	menucommon_s   *item = (menucommon_s *) menuitem;
	menuslider_s   *slider = (menuslider_s *) menuitem;

	min = menu->x + ScaledVideo(item->x + Slider_CursorPositionX(slider) - 4);
	max = menu->x + ScaledVideo(item->x + Slider_CursorPositionX(slider) + 4);

	if (cursor.x < min)
		Menu_SlideItem(menu, -1);
	if (cursor.x > max)
		Menu_SlideItem(menu, 1);
}

void M_Think_MouseCursor(void)
{
	char           *sound = NULL;
	menuframework_s *m = (menuframework_s *) cursor.menu;

	if (m_drawfunc == M_Main_Draw) {	/* have to hack for main menu :p */
		CheckMainMenuMouse();
		return;
	}

	if (!menu_quit_yesno->value && m_drawfunc == M_Quit_Draw) {
		/* have to hack for main menu :p */
		CheckQuitMenuMouse();
		return;
	}

	if (m_drawfunc == M_Credits_MenuDraw) {	/* have to hack for credits :p */
		if (cursor.buttonclicks[MOUSEBUTTON2]) {
			cursor.buttonused[MOUSEBUTTON2] = true;
			cursor.buttonclicks[MOUSEBUTTON2] = 0;
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
			S_StartLocalSound(menu_out_sound);
			if (creditsBuffer)
				FS_FreeFile(creditsBuffer);
			M_PopMenu();
			return;
		}
	}
	
	if (!m)
		return;

	/* Exit with double click 2nd mouse button */
	if (cursor.menuitem) {
		/* MOUSE1 */
		if (cursor.buttondown[MOUSEBUTTON1]) {
			if (cursor.menuitemtype == MENUITEM_SLIDER) {
				Menu_DragSlideItem(m, cursor.menuitem);
			} else if (!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1]) {
				if (cursor.menuitemtype == MENUITEM_ROTATE) {
					if (cl_mouse_rotate->value)
						Menu_SlideItem(m, -1);
					else
						Menu_SlideItem(m, 1);
					sound = menu_move_sound;
					cursor.buttonused[MOUSEBUTTON1] = true;
				} else {
					cursor.buttonused[MOUSEBUTTON1] = true;
					Menu_MouseSelectItem(cursor.menuitem);
					sound = menu_move_sound;
				}
			}
		}
		/* MOUSE2 */
		if (cursor.buttondown[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2]) {
			if (cursor.menuitemtype == MENUITEM_SLIDER && !cursor.buttonused[MOUSEBUTTON2]) {
				Menu_ClickSlideItem(m, cursor.menuitem);
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON2] = true;
			} else if (!cursor.buttonused[MOUSEBUTTON2]) {
				if (cursor.menuitemtype == MENUITEM_ROTATE) {
					if (cl_mouse_rotate->value)
						Menu_SlideItem(m, 1);
					else
						Menu_SlideItem(m, -1);
					sound = menu_move_sound;
					cursor.buttonused[MOUSEBUTTON2] = true;
				}
			}
		}
		/* MOUSEWHEELS */
		if (cursor.mousewheeldown) {
			if (cursor.menuitemtype == MENUITEM_SLIDER) {
				Menu_SlideItem(m, -1);
				sound = NULL;
			} else if (cursor.menuitemtype == MENUITEM_ROTATE) {
				Menu_SlideItem(m, -1);
				sound = menu_move_sound;
			}
		}
		if (cursor.mousewheelup) {
			if (cursor.menuitemtype == MENUITEM_SLIDER) {
				Menu_SlideItem(m, 1);
				sound = NULL;
			} else if (cursor.menuitemtype == MENUITEM_ROTATE) {
				Menu_SlideItem(m, 1);
				sound = menu_move_sound;
			}
		}
	} else if (!cursor.buttonused[MOUSEBUTTON2] &&
	           cursor.buttonclicks[MOUSEBUTTON2] == 2 &&
		   cursor.buttondown[MOUSEBUTTON2]) {
		if (m_drawfunc == Options_MenuDraw) {
			Cvar_SetValue("options_menu", 0);
			refreshCursorLink();
			M_PopMenu();
		} else
			M_PopMenu();

		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
		cursor.buttonused[MOUSEBUTTON1] = true;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
	}
	
	cursor.mousewheeldown = 0;
	cursor.mousewheelup = 0;


	if (sound)
		S_StartLocalSound(sound);
}

void M_Draw_Cursor_Opt (void)
{
	float alpha = 1;
	int w,h;
	char *overlay = NULL;
	char *cur_img = NULL;
	
	if (!cl_mouse_cursor->value)
		return;

	if (m_drawfunc == M_Main_Draw) {
		if (MainMenuMouseHover) {
			if ((cursor.buttonused[0] && cursor.buttonclicks[0]) ||
			    (cursor.buttonused[1] && cursor.buttonclicks[1])) {
				cur_img = "/ui/m_cur_click.pcx";
				alpha = 0.85 + 0.15*sin(anglemod(cl.stime*0.005));
			}
			else {
				cur_img = "/ui/m_cur_hover.pcx";
				alpha = 0.85 + 0.15*sin(anglemod(cl.stime*0.005));
			}
		}
		else
			cur_img = "/ui/m_cur_main.pcx";
			overlay = "/ui/m_cur_over.pcx";
	}
	else {
		if (cursor.menuitem) {
			if (cursor.menuitemtype == MENUITEM_TEXT) {
				cur_img = "/ui/m_cur_text.pcx";
			}
			else {
				if ((cursor.buttonused[0] && cursor.buttonclicks[0]) ||
				    (cursor.buttonused[1] && cursor.buttonclicks[1])) {
					cur_img = "/ui/m_cur_click.pcx";
					alpha = 0.85 + 0.15*sin(anglemod(cl.stime*0.005));
				}
				else {
					cur_img = "/ui/m_cur_hover.pcx";
					alpha = 0.85 + 0.15*sin(anglemod(cl.stime*0.005));
				}
				overlay = "/ui/m_cur_over.pcx";
			}
		}
		else {
			cur_img = "/ui/m_cur_main.pcx";
			overlay = "/ui/m_cur_over.pcx";
		}
	}
	
	if (cur_img) {
		re.DrawGetPicSize( &w, &h, cur_img );
		re.DrawScaledPic(cursor.x - w * 0.5, cursor.y - h * 0.5,
			         ScaledVideo(cl_mouse_scale->value), cl_mouse_alpha->value, cur_img,
			         cl_mouse_red->value, cl_mouse_green->value, 
				 cl_mouse_blue->value, true, false);
		if (overlay) {
			re.DrawGetPicSize( &w, &h, "/ui/m_cur_over.pcx" );
			re.DrawScaledPic(cursor.x - w * 0.5, cursor.y - h * 0.5, 
	                                  ScaledVideo(cl_mouse_scale->value), 1, overlay, 
			                  cl_mouse_red->value, cl_mouse_green->value, 
					  cl_mouse_blue->value, true, false);
		}
	}
}

void M_Draw_Cursor(void)
{

	if (!cl_mouse_cursor->value)
		return;

	if (cl_mouse_cursor->value > 1)
		M_Draw_Cursor_Opt();
	else {
		int	w, h;
		char *cur_img = NULL;
		
		cur_img = "/ui/mouse_cursor.pcx";
	
		/* get sizing vars */
		re.DrawGetPicSize(&w, &h, cur_img);
		re.DrawScaledPic(cursor.x - w * 0.5, cursor.y - h * 0.5, 
	        	         ScaledVideo(cl_mouse_scale->value), cl_mouse_alpha->value, cur_img, 
				 cl_mouse_red->value, cl_mouse_green->value, cl_mouse_blue->value, 
				 true, false);
	}
}

void M_Draw (void)
{
	char	*conback_img;


	conback_img = conback_image->string;
	
	if (cls.key_dest != key_menu)
		return;
	
	if (cl_menu_alpha->value < 0.1)
		Cvar_SetValue("cl_menu_alpha", 0.5);
	
	// Knigthmare- added Psychospaz's scaled menu stuff
	InitMenuScale();
	
	// repaint everything next frame
	SCR_DirtyScreen ();

	// dim everything behind it down
	if (cl.cinematictime > 0 || cls.state == ca_disconnected)
		re.DrawFadeScreen();
	else if (re.RegisterPic("conback"))
		re.DrawStretchPic(0, 0, viddef.width, viddef.height, conback_img, menu_alpha->value);
	else
		re.DrawFadeScreen();
	
	refreshCursorMenu();

	m_drawfunc ();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound( menu_in_sound );
		m_entersound = false;
	}
	
	/* Knigthmare- added Psychospaz's mouse support */
	/* menu cursor for mouse usage :) */
	M_Draw_Cursor();
}


/*
=================
M_Keydown
=================
*/
void M_Keydown (int key)
{
	const char *s;

	if (m_keyfunc)
		if ( ( s = m_keyfunc( key ) ) != 0 )
			S_StartLocalSound( ( char * ) s );
}



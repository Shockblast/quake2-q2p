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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

  */

#include "client.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display
qboolean	scr_initialized;	// ready to draw
int		scr_draw_loading;
vrect_t		scr_vrect;		// position of render window on screen

cvar_t		*scr_viewsize;
cvar_t		*scr_conspeed;
cvar_t		*scr_centertime;
cvar_t		*scr_showturtle;
cvar_t		*scr_showpause;
cvar_t		*scr_printspeed;
cvar_t		*scr_netgraph;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
cvar_t		*scr_graphheight;
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;
cvar_t		*scr_drawall;
cvar_t          *scr_netgraph_trans;
cvar_t          *scr_netgraph_image;
cvar_t          *scr_netgraph_size;
cvar_t          *scr_netgraph_pos;
cvar_t          *scr_netgraph_pos_x;
cvar_t          *scr_netgraph_pos_y;
cvar_t          *con_height;
cvar_t          *cl_draw_x;
cvar_t          *cl_draw_y;
cvar_t          *cl_draw_date;
cvar_t          *cl_draw_chathud;
cvar_t          *cl_draw_clock;
cvar_t          *cl_draw_timestamps;
cvar_t          *cl_draw_ping;
cvar_t          *cl_draw_rate;
cvar_t          *cl_draw_maptime;
cvar_t          *cl_draw_fps;
cvar_t          *cl_draw_speed;
cvar_t          *cl_draw_zoom_sniper;
cvar_t          *cl_highlight;

cvar_t		*crosshair;
cvar_t          *crosshair_x;
cvar_t          *crosshair_y;
cvar_t          *crosshair_scale;
cvar_t          *crosshair_red;
cvar_t          *crosshair_green;
cvar_t          *crosshair_blue;
cvar_t          *crosshair_alpha;
cvar_t          *crosshair_pulse;
cvar_t          *cl_draw_playername;
cvar_t          *cl_draw_playername_x;
cvar_t          *cl_draw_playername_y;
cvar_t          *cl_draw_hud;

cvar_t          *scr_levelshots;

typedef struct
{
	int	x1, y1, x2, y2;
} dirty_t;

dirty_t		scr_dirty, scr_old_dirty[2];

char		crosshair_pic[MAX_QPATH];
int		crosshair_width, crosshair_height;

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);

#define		MAX_SCRN_PINGS		16
#define		MAX_FPS_FRAMES		128
int		currentping = 0, ScrnPings[MAX_SCRN_PINGS];
extern NiceAss_Chat_t ChatMessages;

struct model_s	*clientGun;

float hud_char_size = 8.0;

#define HUD_FONT_SCALE con_font_size->value/hud_char_size
#define HUD_FONT_SIZE hud_char_size

// Knghtmare- scaled HUD support functions
float ScaledHud (float param)
{
	return param*hudScale.avg;
}

float HudScale (void)
{
	return hudScale.avg;
}

void InitHudScale (void)
{
	switch((int)cl_hudscale->value) {
		case 0:
			Cvar_SetValue( "_cl_hudwidth", 0);
			Cvar_SetValue( "_cl_hudheight", 0);
			break;
		case 1:
			Cvar_SetValue( "_cl_hudwidth", 1024);
			Cvar_SetValue( "_cl_hudheight", 768);
			break;
		case 2:
			Cvar_SetValue( "_cl_hudwidth", 800);
			Cvar_SetValue( "_cl_hudheight", 600);
			break;
		case 3:
			Cvar_SetValue( "_cl_hudwidth", 640);
			Cvar_SetValue( "_cl_hudheight", 480);
			break;
		case 4:
			Cvar_SetValue( "_cl_hudwidth", 512);
			Cvar_SetValue( "_cl_hudheight", 384);
			break;
		case 5:
			Cvar_SetValue( "_cl_hudwidth", 400);
			Cvar_SetValue( "_cl_hudheight", 300);
			break;
		case 6:
			Cvar_SetValue( "_cl_hudwidth", 320);
			Cvar_SetValue( "_cl_hudheight", 240);
			break;
		default:
			Cvar_SetValue( "_cl_hudwidth", 640);
			Cvar_SetValue( "_cl_hudheight", 480);
			break;
	}
	// Knightmare- allow disabling of menu scaling
	// also, don't scale if < 800x600, then it would be smaller
	if (cl_hudscale->value && viddef.width > _cl_hudwidth->value ) {
		hudScale.x = viddef.width/_cl_hudwidth->value;
		hudScale.y = viddef.height/_cl_hudheight->value;
		hudScale.avg = (hudScale.x + hudScale.y)*0.5f;
	}
	else {
		hudScale.x = 1;
		hudScale.y = 1;
		hudScale.avg = 1;
	}
}

void DrawHudString( int x, int y, const char *string, int alpha )
{
	unsigned i, j;
	char modifier, character;
	int len, red, green, blue, italic, shadow, bold, reset;
	qboolean modified = false;

	//default
	red = 255;
	green = 255;
	blue = 255;
	italic = false;
	shadow = false;
	bold = false;

	len = strlen( string );
	for ( i = 0, j = 0; i < len; i++ ) {
		modifier = string[i];
		if (modifier&128) modifier &= ~128;

		if (modifier == '^' && i < len) {
			i++;

			reset = 0;
			modifier = string[i];
			if (modifier&128) modifier &= ~128;

			if (modifier!='^') {
				modified = SetParams(modifier, &red, &green, &blue,
				                     &bold, &shadow, &italic, &reset);

				if (reset) {
					red = 255;
					green = 255;
					blue = 255;
					italic = false;
					shadow = false;
					bold = false;
				}
				if (modified)
					continue;
				else
					i--;
			}
		}
		j++;

		character = string[i];
		if (bold && (unsigned) character<128)
			character += 128;
		else if (bold && (unsigned) character>128)
			character -= 128;

		// Knightmare- hack for alternate text color
		if (!modified && (character & 128))
			//red = blue = 0;
			TextColor((int)alt_text_color->value, &red, &green, &blue);

		if (shadow)
			re.DrawScaledChar((x+(j-1)*ScaledHud(HUD_FONT_SIZE)+ScaledHud(HUD_FONT_SIZE/4)),
			                  y+ScaledHud(HUD_FONT_SIZE/8),character, HudScale(),
					  0, 0, 0, alpha, italic);
		re.DrawScaledChar((x+(j-1)*ScaledHud(HUD_FONT_SIZE)), y, character, 
		                  HudScale(), red, green, blue, alpha, italic);
	}
}

void DrawHudString_2 (char *string, int x, int y, int centerwidth, int xor)
{
	int	margin;
	char	line[1024];
	int	width;

	margin = x;

	while (*string) {
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;

		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - width*ScaledHud(8))/2;
		else
			x = margin;


		if (xor) {	// Knightmare- text color hack
			Com_sprintf (line, sizeof(line), "^a%s", line);
			DrawAltHudString(x, y, line, 255);
		}
		else
			DrawHudString(x, y, line, 255);

		if (*string) {
			string++;	// skip the \n
			x = margin;
			y += ScaledHud(8);
		}
	}
}

void DrawAltHudString( int x, int y, const char *string, int alpha )
{
	unsigned i, j;
	char modifier, character;
	int len, red, green, blue, italic, shadow, bold, reset;
	qboolean modified = false;


	//default
	red = 255;
	green = 255;
	blue = 255;
	italic = false;
	shadow = false;
	bold = true;

	len = strlen( string );
	for ( i = 0, j = 0; i < len; i++ ) {
		modifier = string[i];
		if (modifier&128) modifier &= ~128;

		if (modifier == '^' && i < len) {
			i++;

			reset = 0;
			modifier = string[i];
			if (modifier&128) modifier &= ~128;

			if (modifier!='^') {
			modified = SetParams(modifier, &red, &green, &blue, 
			                     &bold, &shadow, &italic, &reset);

				if (reset) {
					red = 255;
					green = 255;
					blue = 255;
					italic = false;
					shadow = false;
					bold = true;
				}
				if (modified)
					continue;
				else
					i--;
			}
		}
		j++;

		character = string[i];
		if (bold && (unsigned) character < 128)
			character += 128;
		else if (bold && (unsigned)character > 128)
			character -= 128;

		// Knightmare- hack for alternate text color
		if (!modified) {
			character ^= 0x80;
			if (character & 128)
				TextColor((int)alt_text_color->value, &red, &green, &blue);
		}

		if (shadow)
			re.DrawScaledChar((x+(j-1)*ScaledHud(HUD_FONT_SIZE)+ScaledHud(HUD_FONT_SIZE/4)),
			                  y+ScaledHud(HUD_FONT_SIZE/4), character, HudScale(), 
					  0, 0, 0, alpha, italic);
		re.DrawScaledChar((x+(j-1)*ScaledHud(HUD_FONT_SIZE)), y, character, 
		                  HudScale(), red, green, blue, alpha, italic);
	}
}

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void)
{
	int		i,
			in,
			ping,
			ScrnPingsIndex = 0;

	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP - 1);
	ping = cls.realtime - cl.cmd_time[in];
	currentping = cls.realtime - cl.cmd_time[in];

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	/* NiceAss: */
	ScrnPings[ScrnPingsIndex % MAX_SCRN_PINGS] = ping;
	ScrnPingsIndex++;
	ping = currentping / 30;
	/* NiceAss: */
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph (ping, 0xd0);
}


typedef struct
{
	float	value;
	int	color;
} graphsamp_t;

static	int		current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph_Original (void)
{
	int	a, x, y, w, i, h;
	float	v;
	int	color;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y+scr_vrect.height;
	if (!scr_netgraph_trans->value)
		re.DrawFill (x, y-scr_graphheight->value,
			     w, scr_graphheight->value, 8);

	for (a=0 ; a<w ; a++) {
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value + scr_graphshift->value;
		
		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));
		h = (int)v % (int)scr_graphheight->value;
		re.DrawFill (x+w-1-a, y - h, 1, h, color);
	}
}

void SCR_DrawDebugGraph_Box(void)
{
	int		a, x, y, w, i, h, min, max;
	float		v, avgs = 0, avgr = 0;
	int		color, time_net;
	static float	lasttime = 0;
	static int	fps, ping;
	char           *netgraph_image;
	struct tm      *now;
	time_t		tnow;
	char		timebuf[32], 
			date_timebuf[32],
			temp_map[32];
	int		time_clock, 
			date_day, 
			map_time,
			time_map, 
			hour, 
			mins, 
			secs;
	static double	octavo = (1.0/8.0);
	player_state_t *player_speed;
	vec3_t		hvel;
#if defined (__unix__)
#include <sys/utsname.h>
	struct utsname	info;
	uname (&info);
#endif
	time_net = Sys_Milliseconds();
	tnow = time((time_t *) 0);
	now = localtime(&tnow);
	map_time = 0;
	
	if (!cl_draw_hud->value)
		return;
	
	h = w = scr_netgraph_size->value;

	if (scr_netgraph_pos->value == 1) {		/* bottom right */
		x = viddef.width - (w + 2) - 1;
		y = viddef.height - (h + 2) - 1;
	} else if (scr_netgraph_pos->value == 2) {	/* bottom left */
		x = 0;
		y = viddef.height - (h + 2) - 1;
	} else if (scr_netgraph_pos->value == 3) {	/* top right */
		x = viddef.width - (w + 2) - 1;
		y = 0;
	} else if (scr_netgraph_pos->value == 4) {  	/* top left */
		x = 0;
		y = 0;
	}
	else  {  					/* custom */
		x = scr_netgraph_pos_x->value;
		y = scr_netgraph_pos_y->value;
	}

	netgraph_image = scr_netgraph_image->string;

	re.DrawStretchPic(x, y, w + 2, h + 2, netgraph_image, scr_netgraph_trans->value);

	for (a = 0; a < w; a++) {
		i = (current - 1 - a + 1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v * scr_graphscale->value;

		if (v < 1)
			v += h * (1 + (int)(-v / h));

		max = (int)v % h + 1;
		min = y + h - max - scr_graphshift->value;

		/* bind to box! */
		if (min < y + 1)
			min = y + 1;
		if (min > y + h)
			min = y + h;
		if (min + max > y + h)
			max = y + h - max;

		re.DrawFill(x + w - a, min, 1, max, color);
	}

	if (cls.realtime - lasttime > 50) {
		lasttime = cls.realtime;
		fps = (cls.frametime) ? 1 / cls.frametime : 0;
		ping = currentping;
	}
	
	for (i = 0; i < MAX_NET_HISTORY; i++)
		avgs += Net_History.SendsSize[i];

	if (Net_History.SendsStartTime)
		avgs /= (time_net - Net_History.SendsStartTime);
	avgs *= 1000.0;
	avgs /= 1024.0;

	for (i = 0; i < MAX_NET_HISTORY; i++)
		avgr += Net_History.RecsSize[i];

	if (Net_History.RecsStartTime)
		avgr /= (time_net - Net_History.RecsStartTime);
	avgr *= 1000.0;
	avgr /= 1024.0;
	
	if (scr_netgraph->value == 3 ) {
#if 0	
		if (cl_draw_clock->value == 1) {
			time_clock = strftime( timebuf, 32, "Time: %H:%M", now );
		} else {
			time_clock = strftime( timebuf, 32, "Time: %I:%M %p", now );	
		}
#else
		time_clock = strftime(timebuf, 32, "Time: ^5%H:%M^r", now);
#endif
		date_day = strftime(date_timebuf, 32, "^5%a, %d %b %Y^r", now);
		
		time_map = cl.stime / 1000;
		hour = time_map/3600;
		mins = (time_map%3600) /60;
		secs = time_map%60;
		
		if (hour > 0) {
			Com_sprintf(temp_map, sizeof(temp_map), "Map: ^5%i:%02i:%02i^r", hour, mins, secs);
		}
		else {
			Com_sprintf(temp_map, sizeof(temp_map), "Map: ^5%i:%02i^r", mins, secs);
		}
				
		player_speed = &cl.frame.playerstate;
				
		VectorSet(hvel, player_speed->pmove.velocity[0] * octavo, 
		          player_speed->pmove.velocity[1] * octavo, 0);

		Con_DrawString(x, y + 5 + FONT_SIZE * 0, va("Q2P: ^5%s^r", Q2P_VERSION), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 1, va(date_timebuf, date_day), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 2, va(timebuf, time_clock), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 3, va("FPS: ^5%3i^r", fps), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 4, va("Speed: ^5%d^r", (int)VectorLength(hvel)), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 5, va("Ping: ^5%3i^r", ping), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 6, va("DL: ^2%2.1f^r", avgr), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 7, va("UL: ^3%2.1f^r", avgs), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 8, va(temp_map, map_time), 255);
#if defined (__unix__)
		Con_DrawString(x, y + 5 + FONT_SIZE * 9, va("OS: ^5%s^r", info.sysname), 255);
		Con_DrawString(x, y + 5 + FONT_SIZE * 10, va("^5%s^r", info.release), 255);
#endif
	}
#if 0	
	/* border... */
	re.DrawFill(x, y, (w + 2), 1, 0);
	re.DrawFill(x, y + (h + 2), (w + 2), 1, 0);
	re.DrawFill(x, y, 1, (h + 2), 0);
	re.DrawFill(x + (w + 2), y, 1, (h + 2), 0);
#endif
}

void SCR_DrawDebugGraph(void)
{
	if (scr_netgraph->value == 1)
		SCR_DrawDebugGraph_Original();
	else if (scr_netgraph->value == 2 || scr_netgraph->value == 3)
		SCR_DrawDebugGraph_Box();
	else
		return;	
}


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
float		scr_centertime_end;
int		scr_center_lines;
int		scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;
	char	line[64];
	int	i, j, l;

	Q_strncpyz (scr_centerstring, str, sizeof(scr_centerstring));
	scr_centertime_off = scr_centertime->value;
	scr_centertime_end = scr_centertime_off;
	scr_centertime_start = cl.stime;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}

	// echo it to the console
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	s = str;
	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (s[l] == '\n' || !s[l])
				break;
		for (i=0 ; i<(40-l)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
		{
			line[i++] = s[j];
		}

		line[i] = '\n';
		line[i+1] = 0;

		Com_Printf ("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;
		s++;		// skip the \n
	} while (1);
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_ClearNotify ();
}

void SCR_DrawCenterString (void)
{
	char		*start;
	int		l;
	int		j;
	int		y;
	int		remaining;
	char		line[512];
	int		alpha = 254 * sqrt( 1 - (((cl.stime-scr_centertime_start)/1000.0)/(scr_centertime_end)));

// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = viddef.height*0.35;
	else
		y = 48;

	do {
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
				
		Com_sprintf (line, sizeof(line), "");
		for (j=0 ; j<l ; j++) {
			
			Com_sprintf (line, sizeof(line), "%s%c", line, start[j]);
			
			if (!remaining--)
				return;
		}
		Con_DrawString ( (int)((viddef.width-strlen(line)*FONT_SIZE)*0.5), y, line, alpha);
		y += FONT_SIZE;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.frametime;
	
	if (scr_centertime_off <= 0)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{
	int		size;

	// bound viewsize
	if (scr_viewsize->value < 40)
		Cvar_Set ("viewsize","40");
	if (scr_viewsize->value > 100)
		Cvar_Set ("viewsize","100");

	size = scr_viewsize->value;

	scr_vrect.width = viddef.width*size/100;
	scr_vrect.width &= ~7;

	scr_vrect.height = viddef.height*size/100;
	scr_vrect.height &= ~1;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (viddef.height - scr_vrect.height)/2;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value-10);
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis;

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;
	if (Cmd_Argc() == 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	re.SetSky (Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", 0);
	scr_showturtle = Cvar_Get ("scr_showturtle", "0", 0);
	scr_showpause = Cvar_Get ("scr_showpause", "1", 0);
	scr_centertime = Cvar_Get ("scr_centertime", "2.5", 0);
	scr_printspeed = Cvar_Get ("scr_printspeed", "8", 0);
	scr_netgraph = Cvar_Get ("netgraph", "3", CVAR_ARCHIVE);
	scr_timegraph = Cvar_Get ("timegraph", "0", 0);
	scr_debuggraph = Cvar_Get ("debuggraph", "0", 0);
	scr_graphheight = Cvar_Get ("graphheight", "32", 0);
	scr_graphscale = Cvar_Get ("graphscale", "1", 0);
	scr_graphshift = Cvar_Get ("graphshift", "0", 0);
	scr_drawall = Cvar_Get ("scr_drawall", "0", 0);
	scr_netgraph_trans = Cvar_Get("netgraph_trans", "0.3", CVAR_ARCHIVE);
	scr_netgraph_image = Cvar_Get("netgraph_image", "net", CVAR_ARCHIVE);
	scr_netgraph_size = Cvar_Get("netgraph_size", "200", CVAR_ARCHIVE);
	scr_netgraph_pos = Cvar_Get("netgraph_pos", "1", CVAR_ARCHIVE);
	scr_netgraph_pos_x = Cvar_Get("netgraph_pos_x", "0", CVAR_ARCHIVE);
	scr_netgraph_pos_y = Cvar_Get("netgraph_pos_y", "0", CVAR_ARCHIVE);
	con_height = Cvar_Get("con_height", "0.5", 0);
	cl_draw_x = Cvar_Get("cl_draw_x", "0", CVAR_ARCHIVE);
	cl_draw_y = Cvar_Get("cl_draw_y", "0.7", CVAR_ARCHIVE);
	cl_draw_date = Cvar_Get("cl_draw_date", "0", 0);
	cl_draw_chathud = Cvar_Get("cl_draw_chathud", "1", CVAR_ARCHIVE);
	cl_draw_ping = Cvar_Get("cl_draw_ping", "0", CVAR_ARCHIVE);
	cl_draw_rate = Cvar_Get("cl_draw_rate", "0", CVAR_ARCHIVE);
	cl_draw_maptime = Cvar_Get("cl_draw_maptime", "0", CVAR_ARCHIVE);
	cl_draw_fps = Cvar_Get("cl_draw_fps", "0", CVAR_ARCHIVE);
	cl_draw_speed = Cvar_Get("cl_draw_speed", "0", CVAR_ARCHIVE);
	cl_draw_clock = Cvar_Get("cl_draw_clock", "0", CVAR_ARCHIVE);
	cl_draw_timestamps = Cvar_Get("cl_draw_timestamps", "0", CVAR_ARCHIVE);
	cl_draw_zoom_sniper = Cvar_Get("cl_draw_zoom_sniper", "1", CVAR_ARCHIVE);
	cl_highlight = Cvar_Get("cl_highlight", "1", CVAR_ARCHIVE);
	
	crosshair = Cvar_Get ("crosshair", "1", CVAR_ARCHIVE);
	crosshair_x = Cvar_Get("crosshair_x", "0", 0);
	crosshair_y = Cvar_Get("crosshair_y", "0", 0);
	crosshair_scale = Cvar_Get("crosshair_scale", "0.5", CVAR_ARCHIVE);
	crosshair_red = Cvar_Get("crosshair_red", "1", CVAR_ARCHIVE);
	crosshair_green = Cvar_Get("crosshair_green", "1", CVAR_ARCHIVE);
	crosshair_blue = Cvar_Get("crosshair_blue", "1", CVAR_ARCHIVE);
	crosshair_alpha = Cvar_Get("crosshair_alpha", "1", CVAR_ARCHIVE);
	crosshair_pulse = Cvar_Get("crosshair_pulse", "1", CVAR_ARCHIVE);

	cl_draw_playername = Cvar_Get("cl_draw_playername", "1", CVAR_ARCHIVE);
	cl_draw_playername_x = Cvar_Get("cl_draw_playername_x", "300", 0);
	cl_draw_playername_y = Cvar_Get("cl_draw_playername_y", "220", 0);
	cl_draw_hud = Cvar_Get("cl_draw_hud", "1", 0);
	
	scr_levelshots = Cvar_Get("scr_levelshots", "1", CVAR_ARCHIVE);
	
//
// register our commands
//
	Cmd_AddCommand ("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",SCR_Loading_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand ("sky",SCR_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged 
		< CMD_BACKUP-1)
		return;

	re.DrawPic (scr_vrect.x+ScaledHud(64), scr_vrect.y, "net", cl_hudalpha->value);
}

/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause (void)
{
	int	w, h;

	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl_paused->value)
		return;

	if (cls.key_dest == key_menu)
		return;

	re.DrawGetPicSize (&w, &h, "pause");
	re.DrawStretchPic2((viddef.width-ScaledVideo(w))/2, (viddef.height-ScaledVideo(h))/2 + 8,
		          ScaledVideo(w), ScaledVideo(h), "pause", cl_menu_red->value, cl_menu_green->value,
			  cl_menu_blue->value, cl_menu_alpha->value);
}

/*
==============
SCR_DrawLoading
==============
*/
void Rogue_Weap(void)
{
	//check for instance of icons we would like to show in loading process, ala q3
	if(blaster)
		re.DrawPic(0,
		           (int)(viddef.height/3.2)*2.8, "w_blaster", 1.0f);
		
	if(chainfist)
		re.DrawPic(0+blaster_d,
		           (int)(viddef.height/3.2)*2.8, "w_chainfist", 1.0f);
		
	if(smartgun)
		re.DrawPic(0+blaster_d+chainfist_d, 
		           (int)(viddef.height/3.2)*2.8, "w_shotgun", 1.0f);
	
	if(machinegun)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d, 
		           (int)(viddef.height/3.2)*2.8, "w_machinegun", 1.0f);
	
	if(sshotgun)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_sshotgun", 1.0f);
	
	if(smachinegun)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_chaingun", 1.0f);
	
	if(etfrifle)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_etf_rifle", 1.0f);
	
	if(glauncher)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d,
		           (int)(viddef.height/3.2)*2.8, "w_glauncher", 1.0f);
	
	if(gproxlaunch)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_proxlaunch", 1.0f);
	
	if(rlauncher)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d,
		           (int)(viddef.height/3.2)*2.8, "w_rlauncher", 1.0f);
		
	if(hyperblaster)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_hyperblaster", 1.0f);
		
	if(heatbeam)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d+hyperblaster_d,
		           (int)(viddef.height/3.2)*2.8, "w_heatbeam", 1.0f);
		
	if(bfg)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d+hyperblaster_d+heatbeam_d,
		           (int)(viddef.height/3.2)*2.8, "w_bfg", 1.0f);
		
	if(tesla)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d+hyperblaster_d+heatbeam_d+bfg_d,
		           (int)(viddef.height/3.2)*2.8, "w_tesla", 1.0f);
	
	if(adren)
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d+hyperblaster_d+heatbeam_d+bfg_d+tesla_d,
		           (int)(viddef.height/3.2)*2.8, "p_adrenaline", 1.0f);
	
	if(quad) 
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d+hyperblaster_d+heatbeam_d+bfg_d+tesla_d+adren_d,
		           (int)(viddef.height/3.2)*2.8, "p_quad", 1.0f);
	
	if(inv) 
		re.DrawPic(0+blaster_d+chainfist_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+etfrifle_d+glauncher_d+gproxlaunch_d+rlauncher_d+hyperblaster_d+heatbeam_d+bfg_d+tesla_d+adren_d+quad_d,
		           (int)(viddef.height/3.2)*2.8, "p_invulnerability", 1.0f);
			
	if(!blaster_d) blaster_d = 50;
	if(!chainfist_d) chainfist_d = 50;
	if(!smartgun_d) smartgun_d = 50;
	if(!machinegun_d) machinegun_d = 50;
	if(!sshotgun_d) sshotgun_d = 50;
	if(!smachinegun_d) smachinegun_d = 50;
	if(!etfrifle_d) etfrifle_d = 50;
	if(!glauncher_d) glauncher_d = 50;
	if(!gproxlaunch_d) gproxlaunch_d = 50;
	if(!rlauncher_d) rlauncher_d = 50;
	if(!hyperblaster_d) hyperblaster_d = 50;
	if(!heatbeam_d) heatbeam_d = 50;
	if(!bfg_d) bfg_d = 50;
	if(!tesla_d) tesla_d = 50;
	if(!adren_d) adren_d = 50;
	if(!quad_d) quad_d = 50;
	if(!inv_d) inv_d = 50;
	
}

void Xatrix_Weap()
{
	//check for instance of icons we would like to show in loading process, ala q3
	if(blaster)
		re.DrawPic(0,
		           (int)(viddef.height/3.2)*2.8, "w_blaster", 1.0f);
		
	if(smartgun)
		re.DrawPic(0+blaster_d+chainfist_d, 
		           (int)(viddef.height/3.2)*2.8, "w_shotgun", 1.0f);
	
	if(machinegun)
		re.DrawPic(0+blaster_d+smartgun_d, 
		           (int)(viddef.height/3.2)*2.8, "w_machinegun", 1.0f);
	
	if(sshotgun)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_sshotgun", 1.0f);
	
	if(smachinegun)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_chaingun", 1.0f);
	
	if(trap)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_trap", 1.0f);
	
	if(glauncher)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d,
		           (int)(viddef.height/3.2)*2.8, "w_glauncher", 1.0f);
	
	if(rlauncher)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_rlauncher", 1.0f);
		
	if(hyperblaster)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_hyperblaster", 1.0f);
		
	if(ripper)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d,
		           (int)(viddef.height/3.2)*2.8, "w_ripper", 1.0f);
		
	if(railgun)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d+ripper_d,
		           (int)(viddef.height/3.2)*2.8, "w_railgun", 1.0f);
		
	if(phallanx)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d+ripper_d+railgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_phallanx", 1.0f);
		
	if(bfg)
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d+ripper_d+railgun_d+phallanx_d,
		           (int)(viddef.height/3.2)*2.8, "w_bfg", 1.0f);
	
	if(adren) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d+ripper_d+railgun_d+phallanx_d+bfg_d,
		           (int)(viddef.height/3.2)*2.8, "p_adrenaline", 1.0f);
	
	if(quad) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d+ripper_d+railgun_d+phallanx_d+bfg_d+adren_d,
		           (int)(viddef.height/3.2)*2.8, "p_quad", 1.0f);
	
	if(inv) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+trap_d+glauncher_d+rlauncher_d+hyperblaster_d+ripper_d+railgun_d+phallanx_d+bfg_d+adren_d+quad_d,
		           (int)(viddef.height/3.2)*2.8, "p_invulnerability", 1.0f);
			
	if(!blaster_d) blaster_d = 50;
	if(!smartgun_d) smartgun_d = 50;
	if(!machinegun_d) machinegun_d = 50;
	if(!sshotgun_d) sshotgun_d = 50;
	if(!smachinegun_d) smachinegun_d = 50;
	if(!trap_d) trap_d = 50;
	if(!glauncher_d) glauncher_d = 50;
	if(!rlauncher_d) rlauncher_d = 50;
	if(!hyperblaster_d) hyperblaster_d = 50;
	if(!ripper_d) ripper_d = 50;
	if(!railgun_d) railgun_d = 50;
	if(!phallanx_d) phallanx_d = 50;
	if(!bfg_d) bfg_d = 50;
	if(!adren_d) adren_d = 50;
	if(!quad_d) quad_d = 50;
	if(!inv_d) inv_d = 50;
}

void Zaero_Weap()
{
		
	//check for instance of icons we would like to show in loading process, ala q3
	if(blaster) 
		re.DrawPic(0,
		           (int)(viddef.height/3.2)*2.8, "w_blaster", 1.0f);

	if(smartgun) 
		re.DrawPic(0+blaster_d, 
		           (int)(viddef.height/3.2)*2.8, "w_shotgun", 1.0f);
	
	if(machinegun) 
		re.DrawPic(0+blaster_d+smartgun_d, 
		           (int)(viddef.height/3.2)*2.8, "w_machinegun", 1.0f);
	
	if(sshotgun) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_sshotgun", 1.0f);
	
	if(smachinegun) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_chaingun", 1.0f);
	
	if(ired) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_ired", 1.0f);
	
	if(glauncher) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d,
		           (int)(viddef.height/3.2)*2.8, "w_glauncher", 1.0f);
	   
	if(rlauncher) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_rlauncher", 1.0f);
	
	if(hyperblaster) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_hyperblaster", 1.0f);
	
	if(railgun) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d,
		           (int)(viddef.height/3.2)*2.8, "w_railgun", 1.0f);
	
	if(bfg) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_bfg", 1.0f);
	
	if(sonic) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d,
		           (int)(viddef.height/3.2)*2.8, "w_sonic", 1.0f);
	
	if(flare) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d+sonic_d,
		           (int)(viddef.height/3.2)*2.8, "w_flare", 1.0f);
	
	if(adren) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d+sonic_d+flare_d,
		           (int)(viddef.height/3.2)*2.8, "p_adrenaline", 1.0f);
	
	if(quad) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d+sonic_d+flare_d+adren_d,
		           (int)(viddef.height/3.2)*2.8, "p_quad", 1.0f);
	
	if(inv) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+ired_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d+sonic_d+flare_d+adren_d+quad_d,
		           (int)(viddef.height/3.2)*2.8, "p_invulnerability", 1.0f);

	if(!blaster_d) blaster_d = 50;
	if(!smartgun_d) smartgun_d = 50;
	if(!machinegun_d) machinegun_d = 50;
	if(!sshotgun_d) sshotgun_d = 50;
	if(!smachinegun_d) smachinegun_d = 50;
	if(!ired_d) ired_d = 50;
	if(!glauncher_d) glauncher_d = 50;
	if(!rlauncher_d) rlauncher_d = 50;
	if(!hyperblaster_d) hyperblaster_d = 50;
	if(!railgun_d) railgun_d = 50;
	if(!bfg_d) bfg_d = 50;
	if(!sonic_d) sonic_d = 50;
	if(!flare_d) flare_d = 50;
	if(!adren_d) adren_d = 50;
	if(!quad_d) quad_d = 50;
	if(!inv_d) inv_d = 50;
}

void Action_Weap(void)
{

	//check for instance of icons we would like to show in loading process, ala q3
	if(knife) 
		re.DrawPic(0,
		           (int)(viddef.height/3.2)*2.8, "w_knife", 1.0f);
	
	if(cannon) 
		re.DrawPic(0+knife_d, 
		           (int)(viddef.height/3.2)*2.8, "w_cannon", 1.0f);
	
	if(akimbo) 
		re.DrawPic(0+knife_d+cannon_d, 
		           (int)(viddef.height/3.2)*2.8, "w_akimbo", 1.0f);
	
	if(m4) 
		re.DrawPic(0+knife_d+cannon_d+akimbo_d, 
		           (int)(viddef.height/3.2)*2.8, "w_m4", 1.0f);
	
	if(mk23) 
		re.DrawPic(0+knife_d+cannon_d+akimbo_d+m4_d, 
		           (int)(viddef.height/3.2)*2.8, "w_mk23", 1.0f);
	
	if(mp5) 
		re.DrawPic(0+knife_d+cannon_d+akimbo_d+m4_d+mk23_d,
		           (int)(viddef.height/3.2)*2.8, "w_mp5", 1.0f);
	
	if(sniper) 
		re.DrawPic(0+knife_d+cannon_d+akimbo_d+m4_d+mk23_d+mp5_d,
		           (int)(viddef.height/3.2)*2.8, "w_sniper", 1.0f);
	
	if(super90) 
		re.DrawPic(0+knife_d+cannon_d+akimbo_d+m4_d+mk23_d+mp5_d+sniper_d,
		           (int)(viddef.height/3.2)*2.8, "w_super90", 1.0f);
	
	if(!knife_d) knife_d = 50;
	if(!cannon_d) cannon_d = 50;
	if(!akimbo_d) akimbo_d = 50;
	if(!m4_d) m4_d = 50;
	if(!mk23_d) mk23_d = 50;
	if(!mp5_d) mp5_d = 50;
	if(!sniper_d) sniper_d = 50;
	if(!super90_d) super90_d = 50;
	
}

void Q2_Weap(void)
{
	
	//check for instance of icons we would like to show in loading process, ala q3
	if(blaster)
		re.DrawPic(0,
		           (int)(viddef.height/3.2)*2.8, "w_blaster", 1.0f);
	if(smartgun) 
		re.DrawPic(0+blaster_d, 
		           (int)(viddef.height/3.2)*2.8, "w_shotgun", 1.0f);
	
	if(machinegun) 
		re.DrawPic(0+blaster_d+smartgun_d, 
		           (int)(viddef.height/3.2)*2.8, "w_machinegun", 1.0f);
	
	if(sshotgun) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_sshotgun", 1.0f);
	
	if(smachinegun) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_chaingun", 1.0f);
	
	if(glauncher) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d,
		           (int)(viddef.height/3.2)*2.8, "w_glauncher", 1.0f);
	
	if(rlauncher) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_rlauncher", 1.0f);
	
	if(hyperblaster) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d+rlauncher_d,
		           (int)(viddef.height/3.2)*2.8, "w_hyperblaster", 1.0f);
	
	if(railgun) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d+rlauncher_d+hyperblaster_d,
		           (int)(viddef.height/3.2)*2.8, "w_railgun", 1.0f);
	
	if(bfg) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d,
		           (int)(viddef.height/3.2)*2.8, "w_bfg", 1.0f);
	
	if(adren) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d,
		           (int)(viddef.height/3.2)*2.8, "p_adrenaline", 1.0f);
	
	if(quad) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d+adren_d,
		           (int)(viddef.height/3.2)*2.8, "p_quad", 1.0f);
	
	if(inv) 
		re.DrawPic(0+blaster_d+smartgun_d+machinegun_d+sshotgun_d+smachinegun_d+glauncher_d+rlauncher_d+hyperblaster_d+railgun_d+bfg_d+adren_d+quad_d,
		           (int)(viddef.height/3.2)*2.8, "p_invulnerability", 1.0f);
	
	
	if(!blaster_d) blaster_d = 50;
	if(!smartgun_d) smartgun_d = 50;
	if(!machinegun_d) machinegun_d = 50;
	if(!sshotgun_d) sshotgun_d = 50;
	if(!smachinegun_d) smachinegun_d = 50;
	if(!glauncher_d) glauncher_d = 50;
	if(!rlauncher_d) rlauncher_d = 50;
	if(!hyperblaster_d) hyperblaster_d = 50;
	if(!railgun_d) railgun_d = 50;
	if(!bfg_d) bfg_d = 50;
	if(!adren_d) adren_d = 50;
	if(!quad_d) quad_d = 50;
	if(!inv_d) inv_d = 50;
	
}

qboolean needLoadingPlaque(void)
{
	if (!cls.disable_screen || !scr_draw_loading)
		return true;
	return false;
}

void SCR_DrawAlertMessagePicture (char *name, qboolean center)
{
	float ratio, scale;
	int w, h;

	InitMenuScale();
	scale = VideoScale();

	re.DrawGetPicSize (&w, &h, name );
	
	if (w) {
		ratio = 35.0/(float)h;
		h = 35;
		w *= ratio;
	}
	else
		return;

	if(center)
		re.DrawStretchPic2((viddef.width - w*scale)/2,
			           (viddef.height - h*scale)/2,
			           scale*w, scale*h, name,
				   cl_menu_red->value, cl_menu_green->value,
				   cl_menu_blue->value, cl_menu_alpha->value);
	else
		re.DrawStretchPic2((viddef.width - w*scale)/2,
			           viddef.height / 2 - scale*130,
			           scale*w, scale*h, name,
				   cl_menu_red->value, cl_menu_green->value,
				   cl_menu_blue->value, cl_menu_alpha->value);
}

void SCR_DrawLoadingBar (float percent, float scale)
{
	re.DrawStretchPic2(viddef.width*.5-scale*15 + 1,viddef.height*.5 + scale*5+1, 
			  (scale*30-2)*percent*0.01, scale*2-2, "/ui/bar_loading.pcx",
			  0, 0, 0, 1.0f);
	re.DrawFill2(viddef.width*.5-scale*15 + 1,viddef.height*.5 + scale*5+1, 
	             scale*30-2, scale*2-2, 0, 0, 0, 0);
}

void SCR_DrawLoading(void)
{
	char	mapfile[32];
	qboolean isMap = false;

	if (!scr_draw_loading)
		return;
		
	scr_draw_loading = false;


	//loading a map...
	if (loadingMessage && cl.configstrings[CS_MODELS+1][0]) {
		strcpy (mapfile, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
		mapfile[strlen(mapfile)-4] = 0;		// cut off ".bsp"

		if (re.RegisterPic(va("/levelshots/%s.pcx", mapfile)))
			re.DrawStretchPic(0, 0, 
			                  viddef.width,
					  viddef.height,
					  va("/levelshots/%s.pcx", mapfile), 1.0f);
		else 
			re.DrawStretchPic(0, 0, viddef.width, viddef.height,
			                  "conback", 1.0f);
		
		isMap = true;
	}
	else
		re.DrawStretchPic(0, 0, viddef.width, viddef.height, "conback", 1.0f);
		
	SCR_DrawAlertMessagePicture("loading", true);

	if (isMap) {
	        //loading message stuff...
		Con_DrawString(0, ScaledVideo(10),
			       va("^3%s ^1'%s'", cl.configstrings[CS_NAME], mapfile), 255);
		Con_DrawString(0, ScaledVideo(25),
			       va("^5%s", loadingMessages[0]), 255);
		Con_DrawString(0, ScaledVideo(35),
		               va("^5%s", loadingMessages[1]), 255);
		Con_DrawString(0, ScaledVideo(45),
		               va("^5%s", loadingMessages[2]), 255);
		Con_DrawString(0, ScaledVideo(55),
			       va("^5%s", loadingMessages[3]), 255);
				
		if (modType("rogue"))
			Rogue_Weap();
		else if (modType("xatrix"))
			Xatrix_Weap();
		else if (modType("zaero"))
			Zaero_Weap();
//		else if (modType("action"))
//			Action_Weap();
		else
			Q2_Weap();

	}
	else {
		re.DrawStretchPic (0, 0, viddef.width, viddef.height, "conback", 1.0f);
		Con_DrawString(0, 0, va("^2^sAwaiting Connection..."), 255);
	}

	// Add Download info stuff...
	if (cls.download) {
		Con_DrawString(0, 0, va("^2^sDownloading %s...", cls.downloadname), 255);
		SCR_DrawLoadingBar(cls.downloadpercent, 8);
		Con_DrawString(viddef.width*.5 - 8*3, 
			       viddef.height*.5 + 8*5.5, 
			       va("%3d%%", (int)cls.downloadpercent), 255);
	}
	else if (isMap) {
	        //loading bar...
		SCR_DrawLoadingBar(loadingPercent, 8);
		Con_DrawString(viddef.width*.5 - 8*3, 
			       viddef.height*.5 + 8*5.5, 
			       va("%3d%%", (int)loadingPercent), 255);
	}
}

void SCR_OldDrawLoading(void)
{
	int	w, h;
		
	if (!scr_draw_loading)
		return;

	scr_draw_loading = false;
	re.DrawGetPicSize (&w, &h, "loading");
	re.DrawPic ((viddef.width-w)/2, (viddef.height-h)/2, "loading", 1.0f);
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
// decide on the height of the console
	if (cls.key_dest == key_console) {
		if (con_height->value < 0.1)
			Cvar_SetValue("con_height", 0.1);
		scr_conlines = con_height->value;		// user controllable
	} else
		scr_conlines = 0;				// none visible
	
	
	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*cls.frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*cls.frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	Con_CheckResize ();
	
	if (cls.state == ca_disconnected || cls.state == ca_connecting) {
	// forced full screen console
		Con_DrawConsole (1.0, false);
		return;
	}

	if ((cls.state != ca_active || !cl.refresh_prepped) && cl.cinematicframe<=0) { 
	// connected, but can't render
		if (con_height->value < 0.1)
			Cvar_SetValue("con_height", 0.1);
		else if (con_height->value > 1)
			Cvar_SetValue("con_height", 1);
		Con_DrawConsole (0.5, false);
		re.DrawFill (0, viddef.height/2, viddef.width, viddef.height/2, 0);
		return;
	}

	if (scr_con_current) 
		Con_DrawConsole (scr_con_current, true);
	else if (!cl.cinematictime && (cls.key_dest == key_game || cls.key_dest == key_message))
		Con_DrawNotify (); // only draw notify in game
}

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds ();
	cl.sound_prepped = false;		// don't play ambients
	
#if defined(CDAUDIO)
	CDAudio_Stop ();
#endif
	if (cls.disable_screen)
		return;
	if (developer->value)
		return;
#if 0
/* Disabled loading levelshots */
	if (cls.state == ca_disconnected)
		return;	// if at console, don't bring up the plaque
	if (cls.key_dest == key_console)
		return;
#endif
	if (cl.cinematictime > 0)
		scr_draw_loading = 2;	// clear to balack first
	else
		scr_draw_loading = 1;
		
	SCR_UpdateScreen ();
	cls.disable_screen = Sys_Milliseconds ();
	cls.disable_servercount = cl.servercount;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	cls.disable_screen = 0;
	scr_draw_loading = 0;
	Con_ClearNotify ();
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

/*
================
SCR_TimeRefresh_f
================
*/
int entitycmpfnc( const entity_t *a, const entity_t *b )
{
	/*
	** all other models are sorted by model then skin
	*/
	if ( a->model == b->model )
		return (long int) a->skin - (long int) b->skin;
	else
		return (long int) a->model - (long int) b->model;
}

void SCR_TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	sstime;

	if ( cls.state != ca_active )
		return;

	start = Sys_Milliseconds ();

	if (Cmd_Argc() == 2) {
	// run without page flipping
		re.BeginFrame( 0 );
		for (i=0 ; i<128 ; i++) {
			cl.refdef.viewangles[1] = i/128.0*360.0;
			re.RenderFrame (&cl.refdef);
		}
		re.EndFrame();
	}
	else {
		for (i=0 ; i<128 ; i++) {
			cl.refdef.viewangles[1] = i/128.0*360.0;
			re.BeginFrame( 0 );
			re.RenderFrame (&cl.refdef);
			re.EndFrame();
		}
	}

	stop = Sys_Milliseconds ();
	sstime = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", sstime, 128/sstime);
}

/*
=================
SCR_AddDirtyPoint
=================
*/
void SCR_AddDirtyPoint (int x, int y)
{
	if (x < scr_dirty.x1)
		scr_dirty.x1 = x;
	if (x > scr_dirty.x2)
		scr_dirty.x2 = x;
	if (y < scr_dirty.y1)
		scr_dirty.y1 = y;
	if (y > scr_dirty.y2)
		scr_dirty.y2 = y;
}

void SCR_DirtyScreen (void)
{
	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);
}

/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void SCR_TileClear (void)
{
	int		i;
	int		top, bottom, left, right;
	dirty_t		clear;
	
	if (scr_viewsize->value == 100)
		return;		// full screen rendering

	if (scr_drawall->value)
		SCR_DirtyScreen (); // for power vr or broken page flippers...

	if (scr_con_current == 1.0)
		return;		// full screen console
		
	if (cl.cinematictime > 0)
		return;		// full screen cinematic

	// erase rect will be the union of the past three frames
	// so tripple buffering works properly
	clear = scr_dirty;
	for (i=0 ; i<2 ; i++) {
		if (scr_old_dirty[i].x1 < clear.x1)
			clear.x1 = scr_old_dirty[i].x1;
		if (scr_old_dirty[i].x2 > clear.x2)
			clear.x2 = scr_old_dirty[i].x2;
		if (scr_old_dirty[i].y1 < clear.y1)
			clear.y1 = scr_old_dirty[i].y1;
		if (scr_old_dirty[i].y2 > clear.y2)
			clear.y2 = scr_old_dirty[i].y2;
	}

	scr_old_dirty[1] = scr_old_dirty[0];
	scr_old_dirty[0] = scr_dirty;

	scr_dirty.x1 = 9999;
	scr_dirty.x2 = -9999;
	scr_dirty.y1 = 9999;
	scr_dirty.y2 = -9999;

	// don't bother with anything convered by the console)
	top = scr_con_current*viddef.height;
	if (top >= clear.y1)
		clear.y1 = top;

	if (clear.y2 <= clear.y1)
		return;		// nothing disturbed

	top = scr_vrect.y;
	bottom = top + scr_vrect.height-1;
	left = scr_vrect.x;
	right = left + scr_vrect.width-1;

	if (clear.y1 < top) {
	// clear above view screen
		i = clear.y2 < top-1 ? clear.y2 : top-1;
		re.DrawTileClear (clear.x1 , clear.y1,
			clear.x2 - clear.x1 + 1, i - clear.y1+1, "backtile");
		clear.y1 = top;
	}
	if (clear.y2 > bottom) { 
	// clear below view screen
		i = clear.y1 > bottom+1 ? clear.y1 : bottom+1;
		re.DrawTileClear (clear.x1, i,
			clear.x2-clear.x1+1, clear.y2-i+1, "backtile");
		clear.y2 = bottom;
	}
	if (clear.x1 < left) {
	// clear left of view screen
		i = clear.x2 < left-1 ? clear.x2 : left-1;
		re.DrawTileClear (clear.x1, clear.y1,
			i-clear.x1+1, clear.y2 - clear.y1 + 1, "backtile");
		clear.x1 = left;
	}
	if (clear.x2 > right) {
	// clear left of view screen
		i = clear.x1 > right+1 ? clear.x1 : right+1;
		re.DrawTileClear (i, clear.y1,
			clear.x2-i+1, clear.y2 - clear.y1 + 1, "backtile");
		clear.x2 = right;
	}

}


//===============================================================


#define STAT_MINUS		10	// num frame for '-' stats digit
char		*sb_nums[2][11] = 
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	"num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	"anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

#define	CHAR_WIDTH	16
/*
================
SizeHUDString

Allow embedded \n in the string
================
*/
void SizeHUDString (char *string, int *w, int *h)
{
	int		lines, width, s_current = 0;

	lines = 1;
	width = 0;

	while (*string) {
		if (*string == '\n') {
			lines++;
			s_current = 0;
		}
		else {
			s_current++;
			if (s_current > width)
				width = s_current;
		}
		string++;
	}

	*w = width * ScaledHud(8);
	*h = lines * ScaledHud(8);
}

void DrawHUDString (char *string, int x, int y, int centerwidth, int xor)
{
	int		margin;
	char		line[1024];
	int		width;

	margin = x;

	while (*string) {
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - width*ScaledHud(8))/2;
		else
			x = margin;
		if (xor) {
		// Knightmare- text color hack
			Com_sprintf (line, sizeof(line), "^a%s", line);
			DrawAltHudString(x, y, line, 255);
		}
		else
			DrawHudString(x, y, line, 255);
		
		if (*string) {
			string++; // skip the \n
			x = margin;
			y += ScaledHud(8);
		}
	}
}

/*
==============
SCR_DrawField
==============
*/
void SCR_DrawField (int x, int y, int color, int width, int value)
{
	char		num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	SCR_AddDirtyPoint (x, y);
	SCR_AddDirtyPoint (x+width*ScaledHud(CHAR_WIDTH+2), y+ScaledHud(24)-1);

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + ScaledHud(CHAR_WIDTH)*(width - l);

	ptr = num;
	while (*ptr && l) {
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';
			
		re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, sb_nums[color][frame], 
		                 cl_hudred->value, cl_hudgreen->value, cl_hudblue->value, 
				 false, true);
		x += ScaledHud(CHAR_WIDTH);
		ptr++;
		l--;
	}
}

/*
 * ================= SCR_AdjustFrom640 =================
 */
void SCR_AdjustFrom640(float *x, float *y, float *w, float *h, qboolean refdef)
{
	float		xscale, yscale, xofs, yofs;

	if (refdef) {
		xscale = cl.refdef.width / 640.0;
		yscale = cl.refdef.height / 480.0;
		xofs = cl.refdef.x;
		yofs = cl.refdef.y;
	} else {
		xscale = viddef.width / 640.0;
		yscale = viddef.height / 480.0;
		xofs = 0;
		yofs = 0;
	}

	if (x) {
		*x *= xscale;
		*x += xofs;
	}
	if (y) {
		*y *= yscale;
		*y += yofs;
	}
	if (w) {
		*w *= xscale;
	}
	if (h) {
		*h *= yscale;
	}
}

/*
 * ================= SCR_ScanForEntityNames from Q2Pro =================
 */
void SCR_ScanForEntityNames(void)
{
	trace_t		trace, worldtrace;
	vec3_t		end;
	entity_state_t *ent;
	int		i, x, zd, zu;
	int		headnode;
	int		num;
	vec3_t		bmins, bmaxs;
	vec3_t		forward;

	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(cl.refdef.vieworg, 131072, forward, end);

	worldtrace = CM_BoxTrace(cl.refdef.vieworg, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

	for (i = 0; i < cl.frame.num_entities; i++) {
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->number == cl.playernum + 1) 
			continue;
		
		if (ent->modelindex != 255) 
			continue;	/* not a player */
		
		if (ent->frame >= 173 || ent->frame <= 0) 
			continue;	/* player is dead */
		
		if (!ent->solid || ent->solid == 31) 
			continue;	/* not solid or bmodel */
		
		x = 8 * (ent->solid & 31);
		zd = 8 * ((ent->solid >> 5) & 31);
		zu = 8 * ((ent->solid >> 10) & 63) - 32;

		bmins[0] = bmins[1] = -x;
		bmaxs[0] = bmaxs[1] = x;
		bmins[2] = -zd;
		bmaxs[2] = zu;

		headnode = CM_HeadnodeForBox(bmins, bmaxs);

		trace = CM_TransformedBoxTrace(cl.refdef.vieworg, end,
		    vec3_origin, vec3_origin, headnode, MASK_PLAYERSOLID,
		    ent->origin, vec3_origin);

		if (trace.allsolid || trace.startsolid || trace.fraction < worldtrace.fraction) {
			cl.playername = &cl.clientinfo[ent->skinnum & 0xFF];
			cl.playernametime = cls.realtime;
			break;
		}
	}
}


/*
 * ================= SCR_DrawPlayerNames =================
 */

void SCR_DrawPlayerNames(void)
{
	float	x, y;
	
	if (!cl_draw_playername->value ||
	    /*!crosshair->value ||*/
	    cl_3dcam->value)
	    return;
	    
	SCR_ScanForEntityNames();

	if (cls.realtime - cl.playernametime > 1000 * cl_draw_playername->value) 
		return;
	
	
	if (!cl.playername) 
		return;
	
	
	x = cl_draw_playername_x->value;
	y = cl_draw_playername_y->value;

	SCR_AdjustFrom640(&x, &y, NULL, NULL, true);

	Con_DrawString(x, y, va("^a^s%s", cl.playername->name), 255);

}

/*
=================
SCR_DrawCrosshair
=================
*/
void SCR_DrawCrosshair (void)
{
	float	scale;
	int	x, y;

	if ((!crosshair->value ||
	    !cl_draw_hud->value ||
	    cl_3dcam->value) ||
	    (cl.refdef.rdflags & RDF_IRGOGGLES))
		return;

	if (crosshair->modified) {
		crosshair->modified = false;
		SCR_TouchPics ();
	}
	
	if (crosshair_scale->modified) {
		crosshair_scale->modified = false;
		if (crosshair_scale->value > 1)
			Cvar_SetValue("crosshair_scale", 1);
		else if (crosshair_scale->value < 0.10)
			Cvar_SetValue("crosshair_scale", 0.10);
	}

	if (!crosshair_pic[0])
		return;

	x = max(0, crosshair_x->value);
	y = max(0, crosshair_y->value);

	scale = crosshair_scale->value * (viddef.width * DIV640);

	re.DrawScaledPic(scr_vrect.x + x + ((scr_vrect.width - crosshair_width) >> 1),
	                 scr_vrect.y + y + ((scr_vrect.height - crosshair_height) >> 1),
	                 scale, (0.75 * crosshair_alpha->value) + (0.25 * crosshair_alpha->value) *
			 sin(anglemod((cl.stime * 0.005) * crosshair_pulse->value)), crosshair_pic,
			 crosshair_red->value, crosshair_green->value, crosshair_blue->value, true, false);

}

void SCR_DrawSniperCrosshair (void)
{

	if ((!cl_draw_zoom_sniper->value) ||
	    (cl_3dcam->value))
		return;
		
	if (modType("dday")) {
	
		if (!clientGun)
			return;
		
		if (cl.refdef.fov_x>=85)
			return;

		/* continue if model is sniper */
		if ((int)(clientGun)!=(int)(re.RegisterModel("models/weapons/usa/v_m1903/tris.md2")) &&
	            (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/grm/v_m98ks/tris.md2")) &&
	            (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/gbr/v_303s/tris.md2")) &&
	            (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/jpn/v_arisakas/tris.md2")) &&
	            (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/rus/v_m9130s/tris.md2")))
			return;
	}
	else if (modType("action")) {
	
		if (clientGun)
			return;
		
		if (cl.refdef.fov_x>=85)
			return;
	}
	else if (!(cls.state == ca_disconnected ||
	           cls.state == ca_connecting) &&
		 cl.refdef.fov_x<=20)
		;
	else
		return;
	
	re.DrawStretchPic2(0, 0, viddef.width, viddef.height, "/gfx/reticle.pcx", 0, 255, 0, 0.1f);
	re.DrawStretchPic(0, 0, viddef.width, viddef.height, "/gfx/sniper.pcx", 0.8f);
}

void SCR_DrawIRGoggles (void)
{
	if (cl_3dcam->value)
		return;

	re.DrawStretchPic(0, 0, viddef.width, viddef.height, "/gfx/ir_crosshair.pcx", 1.0f);
}

void SCR_DrawUnderWaterView (void)
{
	if (cl_3dcam->value ||
	    !cl_underwater_view->value)
		return;
		
	re.DrawStretchPic(0, 0, viddef.width, viddef.height, "/gfx/uw_view.pcx", 0.3f);
}

/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		i, j;
		
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			re.RegisterPic (sb_nums[i][j]);

	if (crosshair->value > 20 || crosshair->value < 0)
		crosshair->value = 1;

	Com_sprintf (crosshair_pic, sizeof(crosshair_pic), "ch%i", (int)(crosshair->value));
	re.DrawGetPicSize (&crosshair_width, &crosshair_height, crosshair_pic);
	
	if (!crosshair_width)
		crosshair_pic[0] = 0;
}

/*
================
SCR_ExecuteLayoutString 

================
*/
void SCR_ExecuteLayoutString (char *s)
{
	int		x, y;
	int		value;
	char	*token;
	int		width;
	int		s_index;
	clientinfo_t	*ci;
	
	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0])
		return;

	x = 0;
	y = 0;
	width = 3;

	InitHudScale();

	while (s) {
		token = COM_Parse (&s);
		if (!strcmp(token, "xl")) {
			token = COM_Parse (&s);
			x = ScaledHud(atoi(token));
			continue;
		}
		if (!strcmp(token, "xr")) {
			token = COM_Parse (&s);
			x = viddef.width + ScaledHud(atoi(token));
			continue;
		}
		if (!strcmp(token, "xv")) {
			token = COM_Parse (&s);
			x = viddef.width/2 - ScaledHud(160) + ScaledHud(atoi(token));
			continue;
		}

		if (!strcmp(token, "yt")) {
			token = COM_Parse (&s);
			y = ScaledHud(atoi(token));
			continue;
		}
		if (!strcmp(token, "yb")) {
			token = COM_Parse (&s);
			y = viddef.height + ScaledHud(atoi(token));
			continue;
		}
		if (!strcmp(token, "yv")) {
			token = COM_Parse (&s);
			y = viddef.height/2 - ScaledHud(120) + ScaledHud(atoi(token));
			continue;
		}

		if (!strcmp(token, "pic")) {	// draw a pic from a stat number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (value >= MAX_IMAGES) {
				Com_Printf ("^3Warning: Pic >= MAX_IMAGES\n");
				value = MAX_IMAGES-1;
			}
			if (cl.configstrings[CS_IMAGES+value]) {
				SCR_AddDirtyPoint (x, y);
				SCR_AddDirtyPoint (x+ScaledHud(24)-1, y+ScaledHud(24)-1);
				re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, 
				                 cl.configstrings[CS_IMAGES + value], 1, 1, 1, 
						 false, true);
			}
			continue;
		}

		if (!strcmp(token, "client")) {	// draw a deathmatch client block
			int		score, ping, dtime;

			token = COM_Parse (&s);
			x = viddef.width/2 - ScaledHud(160) + ScaledHud(atoi(token));
			token = COM_Parse (&s);
			y = viddef.height/2 - ScaledHud(120) + ScaledHud(atoi(token));
			SCR_AddDirtyPoint (x, y);
			SCR_AddDirtyPoint (x+ScaledHud(160)-1, y+ScaledHud(32)-1);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);

			token = COM_Parse (&s);
			dtime = atoi(token);

			DrawAltHudString (x+ScaledHud(32), y, va("^a%s", ci->name), 255);
			DrawHudString (x+ScaledHud(32), y+ScaledHud(8),  "Score: ", 255);
			DrawAltHudString (x+ScaledHud(32+7*8), y+ScaledHud(8),  va("^a%i", score), 255);
			DrawHudString (x+ScaledHud(32), y+ScaledHud(16), va("Ping:  %i", ping), 255);
			DrawHudString (x+ScaledHud(32), y+ScaledHud(24), va("Time:  %i", dtime), 255);

			if (!ci->icon)
				ci = &cl.baseclientinfo;
			re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, ci->iconname, 
			                 cl_hudred->value, cl_hudgreen->value, cl_hudblue->value, 
					 false, true);
			continue;
		}

		if (!strcmp(token, "ctf")) {	// draw a ctf client block
			int		score, ping;
			char	block[80];

			token = COM_Parse (&s);
			x = viddef.width/2 - ScaledHud(160) + ScaledHud(atoi(token));
			token = COM_Parse (&s);
			y = viddef.height/2 - ScaledHud(120) + ScaledHud(atoi(token));
			SCR_AddDirtyPoint (x, y);
			SCR_AddDirtyPoint (x+ScaledHud(160)-1, y+ScaledHud(32)-1);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);
			if (ping > 999)
				ping = 999;

			sprintf(block, "%3d %3d %-12.12s", score, ping, ci->name);

			if (value == cl.playernum)
				DrawAltHudString (x, y, block, 255);
			else
				DrawHudString (x, y, block, 255);
			continue;
		}

		if (!strcmp(token, "picn")) {	// draw a pic from a name
			token = COM_Parse (&s);
			SCR_AddDirtyPoint (x, y);
			SCR_AddDirtyPoint (x+ScaledHud(24)-1, y+ScaledHud(24)-1);
			re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, token, 1, 1, 1, 
			                 false, true);
			continue;
		}

		if (!strcmp(token, "num")) {	// draw a number
			token = COM_Parse (&s);
			width = atoi(token);
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			SCR_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!strcmp(token, "hnum")) {	// health number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_HEALTH];
			if (value > 25)
				color = 0;	// green
			else if (value > 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				color = 1;

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 1)
				re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, "field_3", 
				                 cl_hudred->value, cl_hudgreen->value, 
						 cl_hudblue->value, false, true);

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!strcmp(token, "anum")) {	// ammo number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_AMMO];
			if (value > 5)
				color = 0;	// green
			else if (value >= 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 4)
				re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, "field_3", 
				                 cl_hudred->value, cl_hudgreen->value, 
						 cl_hudblue->value, false, true);

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!strcmp(token, "rnum")) {	// armor number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_ARMOR];
			if (value < 1)
				continue;

			color = 0;	// green

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 2)
				re.DrawScaledPic(x, y, HudScale(), cl_hudalpha->value, "field_3", 
				                 cl_hudred->value, cl_hudgreen->value, 
						 cl_hudblue->value, false, true);

			SCR_DrawField (x, y, color, width, value);
			continue;
		}


		if (!strcmp(token, "stat_string")) {
			token = COM_Parse (&s);
			s_index = atoi(token);
			if (s_index < 0 || s_index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			s_index = cl.frame.playerstate.stats[s_index];
			if (s_index < 0 || s_index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			DrawHudString(x, y, cl.configstrings[s_index], 255);
			continue;
		}

		if (!strcmp(token, "cstring")) {
			token = COM_Parse (&s);
			DrawHudString_2 (token, x, y, ScaledHud(320), 0);
			continue;
		}

		if (!strcmp(token, "string")) {
			token = COM_Parse (&s);
			DrawHudString (x, y, token, 255);
			continue;
		}

		if (!strcmp(token, "cstring2")) {
			token = COM_Parse (&s);
			DrawHudString_2 (token, x, y, ScaledHud(320),0x80);
			continue;
		}

		if (!strcmp(token, "string2")) {
			char	string[1024];
			token = COM_Parse (&s);
			Com_sprintf (string, sizeof(string), "^a%s", token);
			DrawAltHudString (x, y, string, 255);
			continue;
		}

		if (!strcmp(token, "if")) {	// draw a number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (!value) {	// skip to endif
				while (s && strcmp(token, "endif") ) {
					token = COM_Parse (&s);
				}
			}
			continue;
		}
	}
}

/*
================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
================
*/
void SCR_DrawStats (void)
{
	if (!cl_draw_hud->value)
		return;
		
	SCR_ExecuteLayoutString (cl.configstrings[CS_STATUSBAR]);
}

/*
================
SCR_DrawLayout

================
*/
#define	STAT_LAYOUTS		13

void SCR_DrawLayout (void)
{
	if (!cl.frame.playerstate.stats[STAT_LAYOUTS])
		return;
	SCR_ExecuteLayoutString (cl.layout);
}

//=======================================================

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void DrawDemoMessage (void)
{
	//running demo message
	if ( cl.attractloop && !(cl.cinematictime > 0 && cls.realtime - cl.cinematictime > 1000)) {
		int len;
		char *message = "^1^sRunning Demo";

		re.DrawFill2(0, viddef.height-FONT_SIZE, viddef.width, FONT_SIZE, 0, 0, 0, 0);
		len = strlen(message);
		Con_DrawString(viddef.width/2-len*4, viddef.height - FONT_SIZE, message, 255);

	}
}

void SCR_UpdateScreen (void)
{
	int		numframes,
			i,
			y = 0,
			avg;
	float		separation[2] = { 0, 0 };
	static int	prevTime = 0, s_index = 0;
	static char	fps [32];
	char		timebuf[32];
	char		s[96];
	struct tm	*now;
	time_t		tnow;
	
	tnow = time((time_t *) 0);
	now = localtime(&tnow);

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (cls.download) // Knightmare- don't time out on downloads
			cls.disable_screen = Sys_Milliseconds ();
		if (Sys_Milliseconds() - cls.disable_screen > 120000 && 
		    cl.refresh_prepped && !(cl.cinematictime > 0)) { // Knightmare- dont time out on vid restart
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
			return; // Knightmare- moved here for loading screen 
		}
		scr_draw_loading = 2;  // Knightmare- added for loading screen
	}
	
	if (!scr_initialized || !con.initialized)
		return;				// not initialized yet

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if ( cl_stereo_separation->value > 1.0 )
		Cvar_SetValue( "cl_stereo_separation", 1.0 );
	else if ( cl_stereo_separation->value < 0 )
		Cvar_SetValue( "cl_stereo_separation", 0.0 );

	if ( cl_stereo->value ) {
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}		
	else {
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}

	for ( i = 0; i < numframes; i++ ) {
		re.BeginFrame( separation[i] );
		if (scr_draw_loading == 2) {
		//  loading plaque over black screen
			re.CinematicSetPalette(NULL);
			
			if (scr_levelshots->value)
				SCR_DrawLoading();
			else
				SCR_OldDrawLoading();
				
			if (cls.disable_screen)
				scr_draw_loading = 2;
			//NO FULLSCREEN CONSOLE!!!
			continue;
		}
		
		// if a cinematic is supposed to be running, handle menus
		// and console specially
		else if (cl.cinematictime > 0) {
			if (cls.key_dest == key_menu) {
				if (cl.cinematicpalette_active) {
					re.CinematicSetPalette(NULL);
					cl.cinematicpalette_active = false;
				}
				M_Draw ();
			}
			else if (cls.key_dest == key_console) {
				if (cl.cinematicpalette_active) {
					re.CinematicSetPalette(NULL);
					cl.cinematicpalette_active = false;
				}
				SCR_DrawConsole ();
			}
			else
				SCR_DrawCinematic();
		}
		else {
			y = viddef.height * cl_draw_y->value;

			// make sure the game palette is active
			if (cl.cinematicpalette_active) {
				re.CinematicSetPalette(NULL);
				cl.cinematicpalette_active = false;
			}

			// do 3D refresh drawing, and then update the screen
			SCR_CalcVrect ();

			// clear any dirty part of the background
			SCR_TileClear ();

			V_RenderView ( separation[i] );

			SCR_AddDirtyPoint (scr_vrect.x, scr_vrect.y);
			SCR_AddDirtyPoint (scr_vrect.x+scr_vrect.width-1,
		                           scr_vrect.y+scr_vrect.height-1);

			if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
				SCR_DrawDebugGraph();

			SCR_DrawStats();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
				SCR_DrawLayout();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
				CL_DrawInventory();

			SCR_DrawNet();
			
			SCR_CheckDrawCenterString();
			
			//if (cls.key_dest != key_menu) 
				SCR_DrawCrosshair();
			
			SCR_DrawPlayerNames();
	
			if (!(cl.refdef.rdflags & RDF_UNDERWATER))
				SCR_DrawSniperCrosshair();
			
			if (cl.refdef.rdflags & RDF_IRGOGGLES)
				SCR_DrawIRGoggles();
				
			if (cl.refdef.rdflags & RDF_UNDERWATER)
				SCR_DrawUnderWaterView();

			if (cl_draw_chathud->value && cl_draw_hud->value) {
				
				Con_DrawString(viddef.width * cl_draw_x->value, y, ChatMessages.chathud[0], 256);
				y += ScaledHud(8);
				Con_DrawString(viddef.width * cl_draw_x->value, y, ChatMessages.chathud[1], 256);
				y += ScaledHud(8);
				Con_DrawString(viddef.width * cl_draw_x->value, y, ChatMessages.chathud[2], 256);
				y += ScaledHud(8);
				Con_DrawString(viddef.width * cl_draw_x->value, y, ChatMessages.chathud[3], 256);
				y += ScaledHud(10);
			}
			
			if (cl_draw_clock->value && cl_draw_hud->value) {
				
				/* 12hr, else 24hr */
				if (cl_draw_clock->value == 1)
					strftime(timebuf, 32, "Time : %H:%M", now);
				else
					strftime(timebuf, 32, "Time : %I:%M %p", now);

				Con_DrawString(viddef.width * cl_draw_x->value, y, timebuf, 255);
				y += ScaledHud(9);
			}
			
			if (cl_draw_date->value && cl_draw_hud->value) {
				
				strftime(timebuf, 32, "Date: %a, %d %b %Y", now);
				Con_DrawString(viddef.width * cl_draw_x->value, y, timebuf, 255);
				y += ScaledHud(9);
			}
			
			if (cl_draw_fps->value && cl_draw_hud->value) {
				s_index++;

				if ((s_index % MAX_FPS_FRAMES) == 0) {
					Com_sprintf(fps, 32, "FPS : %d", 
					            (int)(1000 / ((float)(Sys_Milliseconds() - prevTime) / MAX_FPS_FRAMES)));
					prevTime = Sys_Milliseconds();
				}
				if (s_index > MAX_FPS_FRAMES) {
					Con_DrawString(viddef.width * cl_draw_x->value, y, fps, 255);
					y += ScaledHud(9);
				}
			}

			if (cl_draw_maptime->value && cl_draw_hud->value) {
				char		temp[32];
				int		mtime, 
				                hour, 
						mins, 
						secs;

				mtime = cl.stime / 1000;

				hour = mtime / 3600;
				mins = (mtime % 3600) / 60;
				secs = mtime % 60;

				if (hour > 0)
					Com_sprintf(temp, sizeof(temp), " Map time : %i:%02i:%02i",
					            hour, mins, secs);
				else
					Com_sprintf(temp, sizeof(temp), "Map time : %i:%02i",
					            mins, secs);

				Con_DrawString(viddef.width * cl_draw_x->value, y, temp, 255);
				y += ScaledHud(9);
			}
			
			if (cl_draw_ping->value && cl_draw_hud->value) {

				avg = 0;

				for (i = 0; i < MAX_SCRN_PINGS; i++)
					avg += ScrnPings[i];

				avg /= MAX_SCRN_PINGS;

				Com_sprintf(s, sizeof(s), "Ping : %dms", avg);
				Con_DrawString(viddef.width * cl_draw_x->value, y, s, 255);
				y += ScaledHud(9);
			}
			
			if (cl_draw_rate->value && cl_draw_hud->value) {

				float		avgs = 0, avgr = 0;
				int		rtime;

				rtime = Sys_Milliseconds();

				for (i = 0; i < MAX_NET_HISTORY; i++)
					avgs += Net_History.SendsSize[i];

				if (Net_History.SendsStartTime)
					avgs /= (rtime - Net_History.SendsStartTime);
				avgs *= 1000.0;
				avgs /= 1024.0;

				for (i = 0; i < MAX_NET_HISTORY; i++)
					avgr += Net_History.RecsSize[i];

				if (Net_History.RecsStartTime)
					avgr /= (rtime - Net_History.RecsStartTime);
				avgr *= 1000.0;
				avgr /= 1024.0;

				Com_sprintf(s, sizeof(s), "DL: ^2%2.1f^r / UL: ^1%2.1f^r",
				            avgr, avgs);
				Con_DrawString(viddef.width * cl_draw_x->value, y, s, 255);
				y += ScaledHud(9);
			}
	
			if(cl_draw_speed->value && cl_draw_hud->value) {
				static double octavo = (1.0/8.0);
				player_state_t *player_speed;
				vec3_t hvel;
				
				player_speed = &cl.frame.playerstate;
				
				VectorSet(hvel, player_speed->pmove.velocity[0] * octavo,
				          player_speed->pmove.velocity[1] * octavo, 0);
				Con_DrawString(viddef.width * cl_draw_x->value, y, va("Speed: %d",
				               (int)VectorLength(hvel)), 255);
				y += ScaledHud(9);
			}
			
			if (scr_timegraph->value)
				SCR_DebugGraph (cls.frametime*300, 0);

			SCR_DrawPause ();
			
			DrawDemoMessage();

			SCR_DrawConsole ();

			M_Draw ();

			if (scr_levelshots->value)
				SCR_DrawLoading();
			else
				SCR_OldDrawLoading();
		}
	}
	re.EndFrame();
}

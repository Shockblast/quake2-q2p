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
// console.c

#include "client.h"

console_t	con;

cvar_t		*con_notifytime;
cvar_t		*con_trans;


#define		MAXCMDLINE	256
extern	char	key_lines[32][MAXCMDLINE];
extern	int	edit_line;
extern	int	key_linepos;

extern  qboolean	key_insert;

void Con_DrawString(int x, int y, char *string, int alpha)
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
		if (bold && (unsigned)character<128)
			character += 128;
		else if (bold && (unsigned)character>128)
			character -= 128;

		// Knightmare- hack for alternate text color
		if (!modified && (character & 128))
			//red = blue = 0;
			TextColor((int)alt_text_color->value, &red, &green, &blue);

		if (shadow)
			re.DrawScaledChar((x+(j-1)*FONT_SIZE+FONT_SIZE/4 ), y+(FONT_SIZE/8), 
			                  character, CON_FONT_SIZE, 0, 0, 0, alpha, italic);
		re.DrawScaledChar((x+(j-1)*FONT_SIZE ), y, character, CON_FONT_SIZE, 
		                  red, green, blue, alpha, italic);
	}
}

void Con_DrawAltString(int x, int y, char *string, int alpha)
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
		if (bold && (unsigned)character<128)
			character += 128;
		else if (bold && (unsigned)character>128)
			character -= 128;

		// Knightmare- hack for alternate text color
		if (!modified) {
			character ^= 0x80;
			if (character & 128)
				//red = blue = 0;
				TextColor((int)alt_text_color->value, &red, &green, &blue);
		}

		if (shadow)
			re.DrawScaledChar((x+(j-1)*FONT_SIZE+FONT_SIZE/4), y+(FONT_SIZE/8), 
			                  character,1.0f, 0, 0, 0, alpha, italic);
		re.DrawScaledChar((x+(j-1)*FONT_SIZE), y, character , 1.0f, 
		                  red, green, blue, alpha, italic);
	}
}

void Key_ClearTyping (void)
{
	key_lines[edit_line][1] = 0;	// clear any typing
	key_linepos = 1;
	con.backedit = 0;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	SCR_EndLoadingPlaque ();	// get rid of loading plaque

	if (cl.attractloop) {
		Cbuf_AddText ("killserver\n");
		return;
	}

	if (cls.state == ca_disconnected) { // start the demo loop again
		Cbuf_AddText ("d1\n");
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	if (cls.key_dest == key_console) {
		M_ForceMenuOff ();
		Cvar_Set ("paused", "0");
	}
	else {
		M_ForceMenuOff ();
		cls.key_dest = key_console;	

		if (Cvar_VariableValue ("maxclients") == 1 
			&& Com_ServerState ())
			Cvar_Set ("paused", "1");
	}
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (cls.key_dest == key_console) {
		if (cls.state == ca_active) {
			M_ForceMenuOff ();
			cls.key_dest = key_game;
		}
	}
	else
		cls.key_dest = key_console;
	
	Con_ClearNotify ();
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	memset (con.text, ' ', CON_TEXTSIZE);
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int	l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		strncpy (buffer, line, con.linewidth);
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con.times[i] = 0;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	if (con_font_size)
		width = viddef.width/con_font_size->value - 2;
	else
		width = (viddef.width >> 3) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)	{		// video hasn't been initialized yet
		width = 38;
		con.linewidth = width;
		con.backedit = 0;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		memset (con.text, ' ', CON_TEXTSIZE);
	}
	else {
		oldwidth = con.linewidth;
		con.linewidth = width;
		con.backedit = 0;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		memcpy (tbuf, con.text, CON_TEXTSIZE);
		memset (con.text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++) {
			for (j=0 ; j<numchars ; j++) {
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con.linewidth = -1;
	con.backedit = 0;

	Con_CheckResize ();
	
	Com_Printf ("Console initialized.\n");

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_trans = Cvar_Get ("con_trans", "0.5", CVAR_ARCHIVE);

//
// register our commands
//
	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);
	
	con.initialized = true;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	memset (&con.text[(con.current%con.totallines)*con.linewidth]
	, ' ', con.linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;

	if (!con.initialized)
		return;

	if (txt[0] == 1 || txt[0] == 2) {
		mask = 128; // go to colored text
		txt++;
	}
	else
		mask = 0;


	while ( (c = *txt) ) {
	// count word length
		for (l=0 ; l< con.linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con.linewidth && (con.x + l > con.linewidth) )
			con.x = 0;

		txt++;

		if (cr) {
			con.current--;
			cr = false;
		}

		
		if (!con.x) {
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con.current >= 0)
				con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}

		switch (c) {
			case '\n':
				con.x = 0;
				break;

			case '\r':
				con.x = 0;
				cr = 1;
				break;

			default:	// display character and advance
				y = con.current % con.totallines;
				con.text[y*con.linewidth+con.x] = c | mask | con.ormask;
				con.x++;
				if (con.x >= con.linewidth)
					con.x = 0;
				break;
		}
		
	}
}

/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint (char *text)
{
	int		l;
	char	buffer[1024];

	l = strlen(text);
	l = (con.linewidth-l)/2;
	if (l < 0)
		l = 0;
	memset (buffer, ' ', l);
	strcpy (buffer+l, text);
	strcat (buffer, "\n");
	Con_Print (buffer);
}

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void)
{
	int	y, i;
	char	*text, output[2048];

	if (cls.key_dest == key_menu)
		return;
		
	if (cls.key_dest != key_console && cls.state == ca_active)
		return;		// don't draw anything (always draw if not active)

	text = key_lines[edit_line];
	
// add the cursor frame
	if (con.backedit)
		text[key_linepos] = ' ';
	else
		text[key_linepos] = 10+((int)(cls.realtime>>8)&1);
	
// fill out remainder with spaces
	for (i=key_linepos+1 ; i< con.linewidth ; i++)
		text[i] = ' ';
		
//	prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
		text += 1 + key_linepos - con.linewidth;
		
// draw it
	y = con.vislines-FONT_SIZE*2; // was 16

	Com_sprintf (output, sizeof(output), "^5^s"Q2PVERSION"^r");
	for (i=0 ; i<con.linewidth ; i++)
	{
		if (con.backedit == key_linepos-i && ((int)(cls.realtime>>8)&1))
			Com_sprintf (output, sizeof(output), "%s%c", output, 12);
		else
			Com_sprintf (output, sizeof(output), "%s%c", output, text[i]);
	}
	Con_DrawString(FONT_SIZE/2, con.vislines - (int)(2.75*FONT_SIZE), output, 255);

// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}
 
/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int	x;
	char	*text, output[2048];
	int	i, j;
	char	*s;
	int	alpha, lines;
	float	v, time;

	lines = 0;

	v = 0;

	Com_sprintf (output, sizeof(output), "");

	//this is the say msg while typeing...
	if (cls.key_dest == key_message) {
		if (chat_team)
			Com_sprintf (output, sizeof(output), "%s", " say_team: ");
		else
			Com_sprintf (output, sizeof(output), "%s", " say: ");

		s = chat_buffer;
		x = 0;
		if (chat_bufferlen > (viddef.width/FONT_SIZE)-(strlen(output)+1))
			x += chat_bufferlen - (int)((viddef.width/FONT_SIZE)-(strlen(output)+1));

		while(s[x]) {
			if (chat_backedit && chat_backedit == chat_bufferlen-x && ((int)(cls.realtime>>8)&1))
				Com_sprintf (output, sizeof(output), "%s%c", output, 11 );
			else
				Com_sprintf (output, sizeof(output), "%s%c", output, (char)s[x]);

			x++;
		}

		if (!chat_backedit)
			Com_sprintf (output, sizeof(output), "%s%c", output, 10+((int)(cls.realtime>>8)&1) );		

		Con_DrawString (0, v, output, 255);

		v += FONT_SIZE*2; //make extra space so we have room
	}

	for (i= con.current-NUM_CON_TIMES+1; i<=con.current; i++) {
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		lines++;
	}

	if (lines)
		for (j=0, i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++, j++) {
			if (i < 0)
				continue;
			time = con.times[i % NUM_CON_TIMES];
			if (time == 0)
				continue;
			time = cls.realtime - time;
			if (time > con_notifytime->value*1000)
				continue;

			text = con.text + (i % con.totallines)*con.linewidth;
			
			alpha = 255 * sqrt( (1.0-time/(con_notifytime->value*1000.0+1.0)) * (((float)v+8.0)) / (8.0*lines) );
			if (alpha < 0) alpha=0;
			if (alpha > 255) alpha=255;

			Com_sprintf (output, sizeof(output), "");
			for (x = 0 ; x < con.linewidth ; x++)
				Com_sprintf (output, sizeof(output), "%s%c", output, (char)text[x]);

			Con_DrawString (FONT_SIZE/2, v, output, alpha);

			v += FONT_SIZE;
		}
	
	if (v) {
		SCR_AddDirtyPoint (0,0);
		SCR_AddDirtyPoint (viddef.width-1, v);
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (float frac, qboolean in_game)
{
	int		i, j, x, y;
	int		rows;
	char		*text, output[1024];
	int		row;
	int		lines;
	char		dlbar[1024];
	float		alpha;
	char            *conback_img;
	char		p_time[32];
	time_t		c_time;
	struct tm	*n_time;

	lines = viddef.height * frac;
	if (lines <= 0)
		return;

	if (lines > viddef.height)
		lines = viddef.height;

	if (!in_game)
		alpha = 1;
	else
		alpha = con_trans->value;

// draw the background
	conback_img = conback_image->string;
	re.DrawStretchPic (0, lines-viddef.height, viddef.width, viddef.height, conback_img, alpha);
	SCR_AddDirtyPoint (0,0);
	SCR_AddDirtyPoint (viddef.width-1,lines-1);
	
	c_time = time(NULL);
	n_time = localtime (&c_time);
	strftime(p_time, sizeof(p_time), "%a, %d %b %Y - %H:%M:%S", n_time);
	// Kill the initial zero
	if (p_time[0] == '0') {
		for (i=0 ; i<(int)(strlen (p_time) + 1) ; i++)
			p_time[i] = p_time[i+1];
		p_time[i+1] = '\0';
	}

	Con_DrawString(viddef.width - (strlen(p_time)*FONT_SIZE), 0, va("^a^s%s", p_time), 255);
	
// draw the text
	con.vislines = lines;
	rows = (lines-(int)(2.75*FONT_SIZE))/FONT_SIZE;		// rows of text to draw
	y = lines - (int)(3.75*FONT_SIZE);

// draw from the bottom up
	if (con.display != con.current) {
	// draw arrows to show the buffer is backscrolled
		for (x=0; x<con.linewidth; x+=(int)FONT_SIZE/2)
				re.DrawScaledChar((x+1)*FONT_SIZE, y, '^', CON_FONT_SIZE, 255, 50, 50, 255, false);
	
		y -= FONT_SIZE;
		rows--;
	}
	
	row = con.display;
	for (i=0; i<rows; i++, y-=FONT_SIZE, row--) {
		if (row < 0)
			break;
		if (con.current - row >= con.totallines)
			break;		// past scrollback wrap point
			
		text = con.text + (row % con.totallines)*con.linewidth;

		Com_sprintf (output, sizeof(output), "");
		for (x=0; x<con.linewidth; x++)
			Com_sprintf (output, sizeof(output), "%s%c", output, text[x]);
			
		Con_DrawString(4, y, output, 256);
	}

//ZOID
	// draw the download bar
	if (cls.download) {
		int graph_x, graph_y, graph_h, graph_w;
		// Knightmare- changeable download/map load bar color
		int red, green, blue;
		TextColor((int)alt_text_color->value, &red, &green, &blue);

		if ((text = strrchr(cls.downloadname, '/')) != NULL)
			text++;
		else
			text = cls.downloadname;

		// make this a little shorter in case of longer version string
		x = con.linewidth - ((con.linewidth * 7) / 40);

		y = x - strlen(text) - 8;
		i = con.linewidth/3;
		if (strlen(text) > i)
		{
			y = x - i - 11;
			strncpy(dlbar, text, i);
			dlbar[i] = 0;
			strcat(dlbar, "...");
		}
		else
			strcpy(dlbar, text);
		strcat(dlbar, ": ");
		i = strlen(dlbar);
		
		// init solid color download bar
		graph_x = (i+1)*FONT_SIZE;
		graph_y = con.vislines - (int)(FONT_SIZE*1.5); // was -12
		graph_w = y*FONT_SIZE;
		graph_h = FONT_SIZE;

		for (j = 0; j < y; j++) // add blank spaces
			dlbar[i++] = ' ';
		sprintf(dlbar + strlen(dlbar), " %02d%%", cls.downloadpercent);

		// draw it
		y = graph_y;
		for (i = 0; i < strlen(dlbar); i++)
			if (dlbar[i] != ' ')
				re.DrawScaledChar((i+1)*FONT_SIZE, y, dlbar[i],
				                  CON_FONT_SIZE, 255, 255, 255, 255, false);

		// new solid color download bar
		graph_y = y;
		re.DrawFill2(graph_x-1, graph_y-1, graph_w+2, graph_h+2,
			     0, 0, 0, 0);

		re.DrawFill2(graph_x+1, graph_y+1,
			     (int)((graph_w-2)*cls.downloadpercent*0.01), graph_h-2,
			     red, green, blue, 255);
		// end Knightmare
		
	}
//ZOID
// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}

/*
Con_DisplayList
New function for tab-completion system
Added by EvilTypeGuy
MEGA Thanks to Taniwha
*/
void Con_DisplayList(char **list)
{
	int	i = 0;
	int	pos = 0;
	int	len = 0;
	int	maxlen = 0;
	int	width = (con.linewidth - 4);
	char	**walk = list;

	while (*walk) {
		len = strlen(*walk);
		if (len > maxlen)
			maxlen = len;
		walk++;
	}
	maxlen += 1;

	while (*list) {
		len = strlen(*list);

		if (pos + maxlen >= width) {
			Com_Printf("\n");
			pos = 0;
		}

		Com_Printf("%s", *list);
		for (i = 0; i < (maxlen - len); i++)
			Com_Printf(" ");

		pos += maxlen;
		list++;
	}

	if (pos)
		Com_Printf("\n\n");
}

/*
Con_CompleteCommandLine
New function for tab-completion system
Added by EvilTypeGuy
Thanks to Fett erich@heintz.com
Thanks to taniwha
*/
void Con_CompleteCommandLine (void)
{
	char	*cmd = "";
	char	*s;
	int		c, v, a, i;
	int		cmd_len;
	char	**list[3] = {0, 0, 0};

	s = key_lines[edit_line] + 1;
	if (*s == '\\' || *s == '/')
		s++;
	if (!*s)
		return;

	// Count number of possible matches
	c = Cmd_CompleteCountPossible(s);
	v = Cvar_CompleteCountPossible(s);
	a = Cmd_CompleteAliasCountPossible(s);
	
	if (!(c + v + a)) {	// No possible matches, let the user know they're insane
		Com_Printf("\n\nNo matching aliases, commands, or cvars were found.\n\n");
		return;
	}
	
	if (c + v + a == 1) {
		if (c)
			list[0] = Cmd_CompleteBuildList(s);
		else if (v)
			list[0] = Cvar_CompleteBuildList(s);
		else
			list[0] = Cmd_CompleteAliasBuildList(s);
		cmd = *list[0];
		cmd_len = strlen (cmd);
	} else {
		if (c)
			cmd = *(list[0] = Cmd_CompleteBuildList(s));
		if (v)
			cmd = *(list[1] = Cvar_CompleteBuildList(s));
		if (a)
			cmd = *(list[2] = Cmd_CompleteAliasBuildList(s));

		cmd_len = strlen (s);
		do {
			for (i = 0; i < 3; i++) {
				char ch = cmd[cmd_len];
				char **l = list[i];
				if (l) {
					while (*l && (*l)[cmd_len] == ch)
						l++;
					if (*l)
						break;
				}
			}
			if (i == 3)
				cmd_len++;
		} while (i == 3);

		// Print Possible Commands
		if (c) {
			Com_Printf("\n^3%i possible command%s\n", c, (c > 1) ? "s: " : ":");
			Con_DisplayList(list[0]);
		}
		
		if (v) {
			Com_Printf("^3%i possible variable%s\n", v, (v > 1) ? "s: " : ":");
			Con_DisplayList(list[1]);
		}
		
		if (a) {
			Com_Printf("^3%i possible alias%s\n", a, (a > 1) ? "es: " : ":");
			Con_DisplayList(list[2]);
		}
	}
	
	if (cmd) {
		strcpy(key_lines[edit_line] + 1, cmd);
		key_linepos = cmd_len + 1;
		if (c + v + a == 1) {
			key_lines[edit_line][key_linepos] = ' ';
			key_linepos++;
		}
		key_lines[edit_line][key_linepos] = 0;
	}

	for (i = 0; i < 3; i++)
		if (list[i])
			Q_free (list[i]);
}



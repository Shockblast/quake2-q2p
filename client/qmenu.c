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
#include <string.h>
#include <ctype.h>

#include "client.h"
#include "qmenu.h"

static void	Action_DoEnter( menuaction_s *a );
static void	Action_Draw( menuaction_s *a );
static void	Menu_DrawStatusBar( const char *string );
static void	MenuList_Draw( menulist_s *l );
static void	Separator_Draw( menuseparator_s *s );
static void	Slider_DoSlide( menuslider_s *s, int dir );
static void	Slider_Draw( menuslider_s *s );
static void	SpinControl_Draw( menulist_s *s );
static void	SpinControl_DoSlide( menulist_s *s, int dir );

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

#define Draw_Char re.DrawChar
#define Draw_ScaledChar re.DrawScaledChar
#define Draw_Fill2 re.DrawFill2

float ScaledVideo (float param)
{
	return param*menuScale.avg;
}

float VideoScale (void)
{
	return menuScale.avg;
}

int mouseOverAlpha( menucommon_s *m )
{
	if (cursor.menuitem == m) {
		int alpha;

		alpha = 125 + 25 * cos(anglemod(cl.stime*0.005));

		if (alpha>255) alpha = 255;
		if (alpha<0) alpha = 0;

		return alpha;
	}
	else 
		return 255;
}

void Action_DoEnter( menuaction_s *a )
{
	if ( a->generic.callback )
		a->generic.callback( a );
}

void Action_Draw( menuaction_s *a )
{
	int alpha = mouseOverAlpha(&a->generic);


	if ( a->generic.flags & QMF_LEFT_JUSTIFY )
	{
		if ( a->generic.flags & QMF_GRAYED )
		{	// Knightmare- text color hack
			char	string[1024];

			if ( !alt_text_color )
				alt_text_color = Cvar_Get ("alt_text_color", "2", CVAR_ARCHIVE);

			Com_sprintf (string, sizeof(string), "^a%s", a->generic.name);
			Menu_DrawStringDark(
			ScaledVideo(a->generic.x) + a->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
			ScaledVideo(a->generic.y) + a->generic.parent->y,
			string, alpha );
		}
		else
			Menu_DrawString(
			ScaledVideo(a->generic.x) + a->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
			ScaledVideo(a->generic.y) + a->generic.parent->y,
			a->generic.name, alpha );
	}
	else
	{
		if ( a->generic.flags & QMF_GRAYED )
			Menu_DrawStringR2LDark(
			ScaledVideo(a->generic.x) + a->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
			ScaledVideo(a->generic.y) + a->generic.parent->y,
			a->generic.name, alpha );
		else
			Menu_DrawStringR2L(
			ScaledVideo(a->generic.x) + a->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
			ScaledVideo(a->generic.y) + a->generic.parent->y,
			a->generic.name, alpha );
	}
	if ( a->generic.ownerdraw )
		a->generic.ownerdraw( a );
}

qboolean Field_DoEnter( menufield_s *f )
{
	if ( f->generic.callback ) {
		f->generic.callback( f );
		return true;
	}
	return false;
}

void Field_Draw( menufield_s *f )
{
	int i, alpha = mouseOverAlpha(&f->generic), xtra;
	char tempbuffer[128]="";
	int offset;

	if ( f->generic.name )
		Menu_DrawStringR2LDark(ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
		                       ScaledVideo(f->generic.y) + f->generic.parent->y, f->generic.name, 255 );

	if ((xtra = StringLengthExtra(f->buffer))) {
		strncpy( tempbuffer, f->buffer + f->visible_offset, f->visible_length );
		offset = strlen(tempbuffer) - xtra;

		if (offset > f->visible_length) {
			f->visible_offset = offset - f->visible_length;
			strncpy( tempbuffer, f->buffer + f->visible_offset - xtra, f->visible_length + xtra );
			offset = f->visible_offset;
		}
	}
	else {
		strncpy( tempbuffer, f->buffer + f->visible_offset, f->visible_length );
		offset = strlen(tempbuffer);
	}

	Draw_ScaledChar(ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(MENU_FONT_SIZE*2),
			ScaledVideo(f->generic.y) + f->generic.parent->y - ScaledVideo(MENU_FONT_SIZE/2+1),
			18, VideoScale(), 255,255,255,255, false);
	Draw_ScaledChar( ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(MENU_FONT_SIZE*2),
			ScaledVideo(f->generic.y) + f->generic.parent->y + ScaledVideo(MENU_FONT_SIZE/2-1),
			24, VideoScale(), 255,255,255,255, false);

	Draw_ScaledChar(ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(MENU_FONT_SIZE*3 + f->visible_length * MENU_FONT_SIZE),
			ScaledVideo(f->generic.y) + f->generic.parent->y - ScaledVideo(MENU_FONT_SIZE/2+1),
			20, VideoScale(), 255,255,255,255, false);
	Draw_ScaledChar(ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(MENU_FONT_SIZE*3 + f->visible_length * MENU_FONT_SIZE),
			ScaledVideo(f->generic.y) + f->generic.parent->y + ScaledVideo(MENU_FONT_SIZE/2-1),
			26, VideoScale(), 255,255,255,255, false);

	for ( i = 0; i < f->visible_length; i++ ) {
		Draw_ScaledChar(ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(MENU_FONT_SIZE*3 + i * MENU_FONT_SIZE),
				ScaledVideo(f->generic.y) + f->generic.parent->y - ScaledVideo(MENU_FONT_SIZE/2+1),
				19, VideoScale(), 255,255,255,255, false);
		Draw_ScaledChar(ScaledVideo(f->generic.x) + f->generic.parent->x + ScaledVideo(MENU_FONT_SIZE*3 + i * MENU_FONT_SIZE),
				ScaledVideo(f->generic.y) + f->generic.parent->y + ScaledVideo(MENU_FONT_SIZE/2-1),
				25, VideoScale(), 255,255,255,255, false);
	}

	//add cursor thingie
	if ( Menu_ItemAtCursor(f->generic.parent)==f  && ((int)(Sys_Milliseconds()/250))&1 )
			Com_sprintf(tempbuffer, sizeof(tempbuffer), "%s%c", tempbuffer, 11);

	Menu_DrawString(ScaledVideo(f->generic.x + MENU_FONT_SIZE*3) + f->generic.parent->x,
			ScaledVideo(f->generic.y-1) + f->generic.parent->y, tempbuffer, alpha );
}

qboolean Field_Key( menufield_s *f, int key )
{
	extern int keydown[];

	switch ( key ) {
		case K_KP_SLASH:
			key = '/';
			break;
		case K_KP_MINUS:
			key = '-';
			break;
		case K_KP_PLUS:
			key = '+';
			break;
		case K_KP_HOME:
			key = '7';
			break;
		case K_KP_UPARROW:
			key = '8';
			break;
		case K_KP_PGUP:
			key = '9';
			break;
		case K_KP_LEFTARROW:
			key = '4';
			break;
		case K_KP_5:
			key = '5';
			break;
		case K_KP_RIGHTARROW:
			key = '6';
			break;
		case K_KP_END:
			key = '1';
			break;
		case K_KP_DOWNARROW:
			key = '2';
			break;
		case K_KP_PGDN:
			key = '3';
			break;
		case K_KP_INS:
			key = '0';
			break;
		case K_KP_DEL:
			key = '.';
			break;
	}

	if ( key > 127 ) {
		switch (key){
			case K_DEL:
			
			default:
			return false;
		}
	}

	/*
	** support pasting from the clipboard
	*/
	if ((toupper(key) == 'V' && keydown[K_CTRL]) ||
	    (((key == K_INS) || (key == K_KP_INS)) && keydown[K_SHIFT])) {
		
		char *cbd;
		
		if ((cbd = Sys_GetClipboardData()) != 0 ) {
			
			strtok( cbd, "\n\r\b" );
			strncpy( f->buffer, cbd, f->length - 1 );
			
			f->cursor = strlen( f->buffer );
			f->visible_offset = f->cursor - f->visible_length;
			
			if ( f->visible_offset < 0 )
				f->visible_offset = 0;
				
			Q_free( cbd );
		}
		return true;
	}

	switch (key) {
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
		case K_BACKSPACE:
			if (f->cursor > 0) {
			
				memmove(&f->buffer[f->cursor-1],
				        &f->buffer[f->cursor],
					strlen( &f->buffer[f->cursor] ) + 1 );
				f->cursor--;

				if (f->visible_offset)
					f->visible_offset--;
			}
			break;

		case K_KP_DEL:
		case K_DEL:
			memmove(&f->buffer[f->cursor],
			        &f->buffer[f->cursor+1],
				strlen( &f->buffer[f->cursor+1] ) + 1 );
			break;

		case K_KP_ENTER:
		case K_ENTER:
		case K_ESCAPE:
		case K_TAB:
			return false;

		case K_SPACE:
		default:
			if (!isdigit(key) && (f->generic.flags & QMF_NUMBERSONLY))
				return false;

			if (f->cursor < f->length) {
				f->buffer[f->cursor++] = key;
				f->buffer[f->cursor] = 0;

				if (f->cursor > f->visible_length)
					f->visible_offset++;
			}
		}
	return true;
}

void Menu_AddItem( menuframework_s *menu, void *item )
{
	if ( menu->nitems == 0 )
		menu->nslots = 0;

	if ( menu->nitems < MAXMENUITEMS ) {
		menu->items[menu->nitems] = item;
		((menucommon_s *) menu->items[menu->nitems])->parent = menu;
		menu->nitems++;
	}

	menu->nslots = Menu_TallySlots( menu );
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor( menuframework_s *m, int dir )
{
	menucommon_s *citem;

	/*
	** see if it's in a valid spot
	*/
	if (m->cursor >= 0 && m->cursor < m->nitems) {
		if ((citem = Menu_ItemAtCursor(m)) != 0) {
			if ( citem->type != MTYPE_SEPARATOR )
				return;
		}
	}

	/*
	** it's not in a valid spot, so crawl in the direction indicated until we
	** find a valid spot
	*/
	if (dir == 1) {
		while (1) {
			citem = Menu_ItemAtCursor(m);
			if (citem)
				if (citem->type != MTYPE_SEPARATOR)
					break;
			m->cursor += dir;
			if (m->cursor >= m->nitems)
				m->cursor = 0;
		}
	}
	else {
		while (1) {
			citem = Menu_ItemAtCursor(m);
			if (citem)
				if (citem->type != MTYPE_SEPARATOR)
					break;
				m->cursor += dir;
				
				if (m->cursor < 0)
				m->cursor = m->nitems - 1;
		}
	}
}

void Menu_Center( menuframework_s *menu )
{
	int height = ScaledVideo( ((menucommon_s *) menu->items[menu->nitems-1])->y + 10 );

	menu->y = (VID_HEIGHT - height) / 2;
}

void Menu_Draw( menuframework_s *menu )
{
	int i;
	menucommon_s *item;

	/*
	** draw contents
	*/
	for (i = 0; i < menu->nitems; i++) {
		switch (((menucommon_s *) menu->items[i])->type) {
			case MTYPE_FIELD:
				Field_Draw((menufield_s *) menu->items[i]);
				break;
			case MTYPE_SLIDER:
				Slider_Draw((menuslider_s *) menu->items[i]);
				break;
			case MTYPE_LIST:
				MenuList_Draw((menulist_s *) menu->items[i]);
				break;
			case MTYPE_SPINCONTROL:
				SpinControl_Draw((menulist_s *) menu->items[i]);
				break;
			case MTYPE_ACTION:
				Action_Draw((menuaction_s *) menu->items[i]);
				break;
			case MTYPE_SEPARATOR:
				Separator_Draw(( menuseparator_s *) menu->items[i]);
				break;
		}
	}

	/*
	** now check mouseovers - psychospaz
	*/
	cursor.menu = menu;

	if (cursor.mouseaction) {
		menucommon_s *lastitem = cursor.menuitem;
		refreshCursorLink();

		for ( i = menu->nitems; i >= 0 ; i-- ) {
			int type;
			int len;
			int min[2], max[2];

			item = ((menucommon_s * )menu->items[i]);

			if (!item || item->type == MTYPE_SEPARATOR)
				continue;

			max[0] = min[0] = menu->x + ScaledVideo(item->x + RCOLUMN_OFFSET); //+ 2 chars for space + cursor
			max[1] = min[1] = menu->y + ScaledVideo(item->y);
			max[1] += ScaledVideo(MENU_FONT_SIZE);

			switch ( item->type ) {
				case MTYPE_ACTION:
					{
						len = strlen(item->name);
						
						if (item->flags & QMF_LEFT_JUSTIFY) {
							min[0] += ScaledVideo(LCOLUMN_OFFSET*2);
							max[0] = min[0] + ScaledVideo(len*MENU_FONT_SIZE);
						}
						else
							min[0] -= ScaledVideo(len*MENU_FONT_SIZE + MENU_FONT_SIZE*3);

						type = MENUITEM_ACTION;
					}
					break;
				case MTYPE_SLIDER:
					{
						min[0] -= ScaledVideo(16);
						max[0] += ScaledVideo((SLIDER_RANGE + 4)*MENU_FONT_SIZE);
						type = MENUITEM_SLIDER;
					}
					break;
				case MTYPE_LIST:
				case MTYPE_SPINCONTROL:
					{
						int len;
						menulist_s *spin = menu->items[i];


						if (item->name) {
							len = strlen(item->name);
							min[0] -= ScaledVideo(len*MENU_FONT_SIZE - LCOLUMN_OFFSET*2);
						}

						len = strlen(spin->itemnames[spin->curvalue]);
						max[0] += ScaledVideo(len*MENU_FONT_SIZE);

						type = MENUITEM_ROTATE;
					}
					break;
				case MTYPE_FIELD:
					{
						menufield_s *text = menu->items[i];

						len = text->visible_length + 2;

						max[0] += ScaledVideo(len*MENU_FONT_SIZE);
						type = MENUITEM_TEXT;
					}
					break;
				default:
					continue;
			}

			if (cursor.x>=min[0] && 
				cursor.x<=max[0] &&
				cursor.y>=min[1] && 
				cursor.y<=max[1]) {
				//new item
				if (lastitem!=item) {
					int j;

					for (j=0;j<MENU_CURSOR_BUTTON_MAX;j++) {
						cursor.buttonclicks[j] = 0;
						cursor.buttontime[j] = 0;
					}
				}

				cursor.menuitem = item;
				cursor.menuitemtype = type;
				
				menu->cursor = i;

				break;
			}
		}
	}

	cursor.mouseaction = false;
	// end Knightmare

	item = Menu_ItemAtCursor( menu );

	if ( item && item->cursordraw )
		item->cursordraw( item );
	else if ( menu->cursordraw )
		menu->cursordraw( menu );
	else if ( item && item->type != MTYPE_FIELD ) {
		if ( item->flags & QMF_LEFT_JUSTIFY )
			Draw_ScaledChar(menu->x + ScaledVideo(item->x - 24 + item->cursor_offset),
					menu->y + ScaledVideo(item->y),
					12 + ( ( int ) ( Sys_Milliseconds()/250 ) & 1 ),
					VideoScale(), 255,255,255,255, false);
		else
			Draw_ScaledChar(menu->x + ScaledVideo(item->cursor_offset),
					menu->y + ScaledVideo(item->y),
					12 + ( ( int ) ( Sys_Milliseconds()/250 ) & 1 ),
					VideoScale(), 255,255,255,255, false);
	}

	if ( item ) {
		if ( item->statusbarfunc )
			item->statusbarfunc( ( void * ) item );
		else if ( item->statusbar )
			Menu_DrawStatusBar( item->statusbar );
		else
			Menu_DrawStatusBar( menu->statusbar );

	}
	else
		Menu_DrawStatusBar( menu->statusbar );
}

void Menu_DrawStatusBar( const char *string )
{
	if ( string ) {
		int l = strlen( string );
		int maxcol = VID_WIDTH / MENU_FONT_SIZE;
		int col = maxcol / 2 - ScaledVideo(l / 2);

		Draw_Fill2( 0, VID_HEIGHT-ScaledVideo(MENU_FONT_SIZE+2), VID_WIDTH,
		           ScaledVideo(MENU_FONT_SIZE+2), 0, 0, 0, 0);
		Draw_Fill2( 0, VID_HEIGHT-ScaledVideo(MENU_FONT_SIZE+2)-1, VID_WIDTH,
		            1, 0, 0, 0, 0 ); // Knightmare added
		Menu_DrawString( col*8, VID_HEIGHT - ScaledVideo(MENU_FONT_SIZE+1), string, 255 );
	}
	else
		Draw_Fill2( 0, VID_HEIGHT-ScaledVideo(MENU_FONT_SIZE+2)-1, VID_WIDTH,
		           ScaledVideo(MENU_FONT_SIZE+2)+2, 0, 0, 0, 0);
}

void Menu_DrawString( int x, int y, const char *string, int alpha )
{
	unsigned i, j;
	char modifier, character;
	int len, red, green, blue, italic, shadow, bold, reset;
	qboolean modified;

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
				modified = SetParams(modifier, &red, &green, &blue, &bold, &shadow, &italic, &reset);

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
		if (bold && (unsigned) character < 128)
			character += 128;
		else if (bold && (unsigned) character >128)
			character -= 128;

		if (shadow)
			Draw_ScaledChar((x + ScaledVideo((j-1)*MENU_FONT_SIZE+2) ), y+ScaledVideo(MENU_FONT_SIZE/8),
			                character, VideoScale(), 0, 0, 0, alpha, italic);
		Draw_ScaledChar((x + ScaledVideo((j-1)*MENU_FONT_SIZE) ), y, character, VideoScale(),
		                red, green, blue, alpha, italic);
	}
}

void Menu_DrawStringDark( int x, int y, const char *string, int alpha )
{
	unsigned i, j;
	char modifier, character;
	int len, red, green, blue, italic, shadow, bold, reset;
	qboolean modified;

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
				modified = SetParams(modifier, &red, &green, &blue, &bold, &shadow, &italic, &reset);

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

		if (shadow)
			Draw_ScaledChar((x + ScaledVideo(j*MENU_FONT_SIZE+2) ), y+ScaledVideo(MENU_FONT_SIZE/8),
			                character,VideoScale(), 0, 0, 0, alpha, italic);
		Draw_ScaledChar(( x + ScaledVideo(j*MENU_FONT_SIZE) ), y, character , VideoScale(),
		                red, green, blue, alpha, italic);
	}
}

void Menu_DrawStringR2L (int x, int y, const char *string, int alpha)
{
	x -= StringLen(string)*ScaledVideo(MENU_FONT_SIZE);
	Menu_DrawString(x, y, string, alpha);
}

void Menu_DrawStringR2LDark (int x, int y, const char *string, int alpha)
{
	// Knightmare- text color hack
	char	newstring[1024];

	if ( !alt_text_color )
		alt_text_color = Cvar_Get ("alt_text_color", "2", CVAR_ARCHIVE);

	x -= StringLen(string)*ScaledVideo(MENU_FONT_SIZE);
	Com_sprintf (newstring, sizeof(newstring), "^a%s", string);
	Menu_DrawStringDark(x, y, newstring, alpha);
}

void *Menu_ItemAtCursor( menuframework_s *m )
{
	if (m->cursor < 0 ||
	    m->cursor >= m->nitems)
		return 0;

	return m->items[m->cursor];
}

qboolean Menu_SelectItem( menuframework_s *s )
{
	menucommon_s *item = (menucommon_s *) Menu_ItemAtCursor(s);

	if (item) {
		switch (item->type) {
			case MTYPE_FIELD:
				return Field_DoEnter((menufield_s *) item) ;
			case MTYPE_ACTION:
				Action_DoEnter((menuaction_s *) item);
				return true;
			case MTYPE_LIST:
				return false;
			case MTYPE_SPINCONTROL:
				return false;
		}
	}
	return false;
}

qboolean Menu_MouseSelectItem(menucommon_s * item)
{
	if (item) {
		switch (item->type) {
			case MTYPE_FIELD:
			return Field_DoEnter((menufield_s *) item);
			case MTYPE_ACTION:
				Action_DoEnter((menuaction_s *) item);
				return true;
			case MTYPE_LIST:
			case MTYPE_SPINCONTROL:
				return false;
		}
	}
	return false;
}

void Menu_SetStatusBar(menuframework_s *m, const char *string)
{
	m->statusbar = string;
}

void Menu_SlideItem(menuframework_s *s, int dir)
{
	menucommon_s *item = (menucommon_s *) Menu_ItemAtCursor(s);

	if ( item ) {
		switch (item->type) {
			case MTYPE_SLIDER:
				Slider_DoSlide((menuslider_s *) item, dir);
				break;
			case MTYPE_SPINCONTROL:
				SpinControl_DoSlide((menulist_s *) item, dir);
				break;
		}
	}
}

int Menu_TallySlots(menuframework_s *menu)
{
	int i;
	int total = 0;

	for ( i = 0; i < menu->nitems; i++ ) {
		if (((menucommon_s *) menu->items[i])->type == MTYPE_LIST) {
			int nitems = 0;
			const char **n = ((menulist_s *) menu->items[i])->itemnames;

			while (*n)
				nitems++, n++;

			total += nitems;
		}
		else
			total++;
	}

	return total;
}

void MenuList_Draw( menulist_s *l )
{
	const char **n;
	int y = 0, alpha = mouseOverAlpha(&l->generic);

	Menu_DrawStringR2LDark(ScaledVideo(l->generic.x) + l->generic.parent->x + ScaledVideo(-2*MENU_FONT_SIZE),
	                       ScaledVideo(l->generic.y) + l->generic.parent->y, l->generic.name, alpha );

	n = l->itemnames;

  	Draw_Fill2(ScaledVideo(l->generic.x - 112) + l->generic.parent->x,
	                       l->generic.parent->y + ScaledVideo(l->generic.y + l->curvalue*10 + 10),
				128, 10, 0, 0, 0, 16 );
	while ( *n ) {
		Menu_DrawStringR2LDark(ScaledVideo(l->generic.x) + l->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
		                       ScaledVideo(l->generic.y) + l->generic.parent->y + ScaledVideo(y + 10), *n, alpha );

		n++;
		y += 10;
	}
}

void Separator_Draw( menuseparator_s *s )
{
	int alpha = mouseOverAlpha(&s->generic);

	if ( s->generic.name )
		Menu_DrawStringR2LDark(ScaledVideo(s->generic.x) + s->generic.parent->x,
			               ScaledVideo(s->generic.y) + s->generic.parent->y,
			               s->generic.name, alpha );
}

void Slider_DoSlide(menuslider_s *s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue > s->maxvalue)
		s->curvalue = s->maxvalue;
	else if (s->curvalue < s->minvalue)
		s->curvalue = s->minvalue;

	if (s->generic.callback)
		s->generic.callback(s);
}

void Slider_Draw( menuslider_s *s )
{
	int	i, alpha = mouseOverAlpha(&s->generic);

	Menu_DrawStringR2LDark(ScaledVideo(s->generic.x) + s->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET),
			       ScaledVideo(s->generic.y) + s->generic.parent->y, 
			       s->generic.name, alpha );

	s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( s->range < 0)
		s->range = 0;
	if ( s->range > 1)
		s->range = 1;

	Draw_ScaledChar(ScaledVideo(s->generic.x) + s->generic.parent->x + ScaledVideo(RCOLUMN_OFFSET),
			ScaledVideo(s->generic.y) + s->generic.parent->y,
			128, VideoScale(), 255,255,255,255, false);

	for ( i = 0; i < SLIDER_RANGE; i++ )
		Draw_ScaledChar(ScaledVideo(RCOLUMN_OFFSET + s->generic.x + i*MENU_FONT_SIZE + MENU_FONT_SIZE) + s->generic.parent->x,
				ScaledVideo(s->generic.y) + s->generic.parent->y,
				129, VideoScale(), 255,255,255,255, false);

	Draw_ScaledChar(ScaledVideo(RCOLUMN_OFFSET + s->generic.x + i*MENU_FONT_SIZE + MENU_FONT_SIZE) + s->generic.parent->x,
			ScaledVideo(s->generic.y) + s->generic.parent->y,
			130, VideoScale(), 255,255,255,255, false);

	Draw_ScaledChar((int)(s->generic.parent->x + ScaledVideo(s->generic.x + RCOLUMN_OFFSET + MENU_FONT_SIZE + (SLIDER_RANGE-1)*MENU_FONT_SIZE * s->range) ),
			ScaledVideo(s->generic.y) + s->generic.parent->y,
			131, VideoScale(), 255,255,255,255, false);
}

void SpinControl_DoSlide( menulist_s *s, int dir )
{
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else if (s->itemnames[s->curvalue] == 0)
		s->curvalue--;

	if (s->generic.callback)
		s->generic.callback(s);
}

void SpinControl_Draw( menulist_s *s )
{
	int alpha = mouseOverAlpha(&s->generic);
	char buffer[100];

	if ( s->generic.name )
		Menu_DrawStringR2LDark(ScaledVideo(s->generic.x) + s->generic.parent->x + ScaledVideo(LCOLUMN_OFFSET), 
				       ScaledVideo(s->generic.y) + s->generic.parent->y, 
				       s->generic.name, alpha );
	if ( !strchr(s->itemnames[s->curvalue], '\n' ) )
		Menu_DrawString(ScaledVideo(RCOLUMN_OFFSET + s->generic.x) + s->generic.parent->x,
				ScaledVideo(s->generic.y) + s->generic.parent->y,
				s->itemnames[s->curvalue], alpha );
	else {
		strcpy( buffer, s->itemnames[s->curvalue] );
		*strchr( buffer, '\n' ) = 0;
		Menu_DrawString(ScaledVideo(RCOLUMN_OFFSET + s->generic.x) + s->generic.parent->x,
				ScaledVideo(s->generic.y) + s->generic.parent->y, buffer, alpha );
		strcpy( buffer, strchr( s->itemnames[s->curvalue], '\n' ) + 1 );
		Menu_DrawString(ScaledVideo(RCOLUMN_OFFSET + s->generic.x) + s->generic.parent->x,
				ScaledVideo(s->generic.y) + s->generic.parent->y + ScaledVideo(MENU_FONT_SIZE+2),
				buffer, alpha );
	}
}





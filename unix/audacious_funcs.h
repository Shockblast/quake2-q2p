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

#ifndef AUDACIOUS_FUNC
#define AUDACIOUS_FUNC(ret, func, params)
#define UNDEF_AUDACIOUS_FUNC
#endif

AUDACIOUS_FUNC (void, xmms_remote_play, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_pause, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_stop, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_jump_to_time, (gint session, gint pos))
AUDACIOUS_FUNC (gboolean, xmms_remote_is_running, (gint session))
AUDACIOUS_FUNC (gboolean, xmms_remote_is_playing, (gint session))
AUDACIOUS_FUNC (gboolean, xmms_remote_is_paused, (gint session))
AUDACIOUS_FUNC (gboolean, xmms_remote_is_repeat, (gint session))
AUDACIOUS_FUNC (gboolean, xmms_remote_is_shuffle, (gint session))
AUDACIOUS_FUNC (gint, xmms_remote_get_playlist_pos, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_set_playlist_pos, (gint session, gint pos))
AUDACIOUS_FUNC (gint, xmms_remote_get_playlist_length, (gint session))
AUDACIOUS_FUNC (gchar *, xmms_remote_get_playlist_title, (gint session, gint pos))
AUDACIOUS_FUNC (void, xmms_remote_playlist_prev, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_playlist_next, (gint session))
AUDACIOUS_FUNC (gint, xmms_remote_get_output_time, (gint session))
AUDACIOUS_FUNC (gint, xmms_remote_get_playlist_time, (gint session, gint pos))
AUDACIOUS_FUNC (gint, xmms_remote_get_main_volume, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_set_main_volume, (gint session, gint v))
AUDACIOUS_FUNC (void, xmms_remote_toggle_repeat, (gint session))
AUDACIOUS_FUNC (void, xmms_remote_toggle_shuffle, (gint session))
AUDACIOUS_FUNC (void, g_free, (gpointer))

#ifdef UNDEF_AUDACIOUS_FUNC
#undef AUDACIOUS_FUNC
#undef UNDEF_AUDACIOUS_FUNC
#endif

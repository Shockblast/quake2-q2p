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
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#if defined (__unix__)  && defined (WITH_AUDACIOUS)

#include "../client/client.h"
#include "../client/sound.h"
#include "audacious_player.h"

#define AUDACIOUS_SESSION	(audacious_session->value)

static cvar_t *audacious_dir;
static cvar_t *audacious_session;
static cvar_t *audacious_messages;

#define Sys_MSleep(x) usleep((x) * 1000)

static char *mp3_notrunning_msg = AUDACIOUS_MP3_PLAYERNAME_LEADINGCAP " is not running";

// Define all dynamic AUDACIOUS functions...
#define AUDACIOUS_FUNC(ret, func, params) \
static ret (*q##func) params;
#include "audacious_funcs.h"
#undef AUDACIOUS_FUNC


#define QLIB_FREELIBRARY(lib) (dlclose(lib), lib = NULL)

static void *libaudacious_handle = NULL;
static int AUDACIOUS_pid = 0;

static void AUDACIOUS_LoadLibrary(void)
{
	if (!(libaudacious_handle = dlopen("libaudacious.so", RTLD_NOW))) {
		if (!(libaudacious_handle = dlopen("libaudacious.so.4", RTLD_NOW))) {
			Com_Printf("Could not open 'libaudacious.so' or 'libaudacious.so.1'\n");
			return;
		}
	}
#define AUDACIOUS_FUNC(ret, func, params)                                   \
        if (!(q##func = dlsym(libaudacious_handle, #func))) {               \
                Com_Printf("Couldn't load AUDACIOUS function %s\n", #func); \
                QLIB_FREELIBRARY(libaudacious_handle);                      \
                return;                                                     \
        }
#include "audacious_funcs.h"
#undef AUDACIOUS_FUNC
}

static void AUDACIOUS_FreeLibrary(void)
{
	if (libaudacious_handle) 
		QLIB_FREELIBRARY(libaudacious_handle);
}

qboolean AUDACIOUS_MP3_IsActive(void)
{
	return !!libaudacious_handle;
}

static qboolean AUDACIOUS_MP3_IsPlayerRunning(void)
{
	return (qxmms_remote_is_running((gint) AUDACIOUS_SESSION));
}

void AUDACIOUS_MP3_Execute_f(void)
{
	char path[MAX_OSPATH], *argv[2] = {"audacious", NULL};
	int i, length;

	if (AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("Audacious is already running\n");
		return;
	}
	
	Q_strncpyz(path, audacious_dir->string, sizeof(path) - strlen("/audacious"));
	
	length = strlen(path);
	for (i = 0; i < length; i++) {
		if (path[i] == '\\')
			path[i] = '/';
	}
	
	if (length && path[length - 1] == '/')
		path[length - 1] = 0;
	
	strcat(path, "/audacious");

	if (!(AUDACIOUS_pid = fork())) {
		execv(path, argv);
		exit(-1);
	}
	
	if (AUDACIOUS_pid == -1) {
		Com_Printf ("Couldn't execute Audacious\n");
		return;
	}
	
	for (i = 0; i < 6; i++) {
		Sys_MSleep(50);
		if (AUDACIOUS_MP3_IsPlayerRunning()) {
			Com_Printf("Audacious is now running\n");
			return;
		}
	}
	
	Com_Printf("Audacious (probably) failed to run\n");
}

AUDACIOUS_COMMAND(Prev, playlist_prev);
AUDACIOUS_COMMAND(Play, play);
AUDACIOUS_COMMAND(Pause, pause);
AUDACIOUS_COMMAND(Stop, stop);
AUDACIOUS_COMMAND(Next, playlist_next);
AUDACIOUS_COMMAND(FadeOut, stop);

void AUDACIOUS_MP3_FastForward_f(void)
{
	int current;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
		return;
	}
	
	current = qxmms_remote_get_output_time(AUDACIOUS_SESSION) + 5 * 1000;
	qxmms_remote_jump_to_time(AUDACIOUS_SESSION, current);
}

void AUDACIOUS_MP3_Rewind_f(void)
{
	int current;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
		return;
	}
	
	current = qxmms_remote_get_output_time(AUDACIOUS_SESSION) - 5 * 1000;
	current = max(0, current);
	
	qxmms_remote_jump_to_time(AUDACIOUS_SESSION, current);
}

int AUDACIOUS_MP3_GetStatus(void)
{
	
	if (!AUDACIOUS_MP3_IsPlayerRunning())
		return AUDACIOUS_MP3_NOTRUNNING;
	
	if (qxmms_remote_is_paused(AUDACIOUS_SESSION))
		return AUDACIOUS_MP3_PAUSED;
	
	if (qxmms_remote_is_playing(AUDACIOUS_SESSION))
		return AUDACIOUS_MP3_PLAYING;	
	
	return AUDACIOUS_MP3_STOPPED;
}

static void AUDACIOUS_Set_ToggleFn(char *name, void *togglefunc, void *getfunc)
{
	int ret, set;
	gboolean (*xmms_togglefunc)(gint) = togglefunc;
	gboolean (*xmms_getfunc)(gint) = getfunc;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);	
		return;
	}
	
	if (Cmd_Argc() >= 3) {
		Com_Printf("Usage: %s [on|off|toggle]\n", Cmd_Argv(0));
		return;
	}
	
	ret = xmms_getfunc(AUDACIOUS_SESSION);
	
	if (Cmd_Argc() == 1) {
		Com_Printf("%s is %s\n", name, (ret == 1) ? "on" : "off");
		return;
	}
	
	if (!Q_stricmp(Cmd_Argv(1), "on")) 
		set = 1;
	else if (!Q_stricmp(Cmd_Argv(1), "off"))
		set = 0;
	else if (!Q_stricmp(Cmd_Argv(1), "toggle"))
		set = ret ? 0 : 1;
	else {
		Com_Printf("Usage: %s [on|off|toggle]\n", Cmd_Argv(0));
		return;
	}
	
	if (set && !ret)
		xmms_togglefunc(AUDACIOUS_SESSION);
	else if (!set && ret)
		xmms_togglefunc(AUDACIOUS_SESSION);
	
	Com_Printf("%s set to %s\n", name, set ? "on" : "off");
}

void AUDACIOUS_MP3_Repeat_f(void)
{
	AUDACIOUS_Set_ToggleFn("Repeat", qxmms_remote_toggle_repeat, qxmms_remote_is_repeat);
}

void AUDACIOUS_MP3_Shuffle_f(void)
{
	AUDACIOUS_Set_ToggleFn("Shuffle", qxmms_remote_toggle_shuffle, qxmms_remote_is_shuffle);
}

void AUDACIOUS_MP3_ToggleRepeat_f(void)
{

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);	
		return;
	}
	
	qxmms_remote_toggle_repeat(AUDACIOUS_SESSION);
}

void AUDACIOUS_MP3_ToggleShuffle_f(void)
{
	
	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);	
		return;
	}
	
	qxmms_remote_toggle_shuffle(AUDACIOUS_SESSION);
}

char *AUDACIOUS_MP3_Macro_MP3Info(void)
{
	int playlist_pos;
	char *s;
	static char title[AUDACIOUS_MP3_MAXSONGTITLE];

	title[0] = 0;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("Audacious not running\n");
		return title;
	}
	
	playlist_pos = qxmms_remote_get_playlist_pos(AUDACIOUS_SESSION);
	s = qxmms_remote_get_playlist_title(AUDACIOUS_SESSION, playlist_pos);
	
	if(s) {
		Q_strncpyz(title, s, sizeof(title));
		COM_MakePrintable(title);
		qg_free(s);
	}

	return title;
}

qboolean AUDACIOUS_MP3_GetTrackTime(int *elapsed, int *total)
{
	int pos;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
		return false;
	}

	if(elapsed)
		*elapsed = qxmms_remote_get_output_time(AUDACIOUS_SESSION) / 1000.0;

	if(total) {
		pos = qxmms_remote_get_playlist_pos(AUDACIOUS_SESSION);
		*total = qxmms_remote_get_playlist_time(AUDACIOUS_SESSION, pos) / 1000.0;
	}

	return true;
}

#if 0
static void AUDACIOUS_MP3_SongTitle_m ( char *buffer, int bufferSize )
{
	char *songtitle;
	int total = 0;

	if (!AUDACIOUS_MP3_IsPlayerRunning())
		return;

	if(!AUDACIOUS_MP3_GetTrackTime(NULL, &total))
		return;

	songtitle = AUDACIOUS_MP3_Macro_MP3Info();
	if (!*songtitle)
		return;

	Com_sprintf(buffer, bufferSize, "%s [%i:%02i]\n", songtitle, total / 60, total % 60);
}
#endif

qboolean AUDACIOUS_MP3_GetToggleState(int *shuffle, int *repeat)
{
	if (!AUDACIOUS_MP3_IsPlayerRunning()) 
		return false;
	
	*shuffle = qxmms_remote_is_shuffle(AUDACIOUS_SESSION);
	*repeat = qxmms_remote_is_repeat(AUDACIOUS_SESSION);
	
	return true;
}

void AUDACIOUS_MP3_SongInfo_f(void)
{
	char *status_string, *title, *s;
	int status, elapsed, total;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
		return;
	}
	
	if (Cmd_Argc() != 1) {
		Com_Printf("%s : no arguments expected\n", Cmd_Argv(0));
		return;
	}

	status = AUDACIOUS_MP3_GetStatus();
	if (status == AUDACIOUS_MP3_STOPPED)
		status_string = "Stopped";
	else if (status == AUDACIOUS_MP3_PAUSED)
		status_string = "Paused";
	else
		status_string = "Playing";

	for (s = title = AUDACIOUS_MP3_Macro_MP3Info(); *s; s++)
		*s |= 128;

	if (!AUDACIOUS_MP3_GetTrackTime(&elapsed, &total) || elapsed < 0 || total < 0) {
		Com_Printf(va("%s %s\n", status_string, title));
		return;
	}
	
	Com_Printf("%s %s [%i:%02i]\n", status_string, title, total / 60, total % 60);
}

char *AUDACIOUS_MP3_Menu_SongTitle(void)
{
	static char title[AUDACIOUS_MP3_MAXSONGTITLE], *macrotitle;
	int current;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
	    Q_strncpyz(title, mp3_notrunning_msg, sizeof(title));
	    return title;
	}
	
	macrotitle = AUDACIOUS_MP3_Macro_MP3Info();
	
	AUDACIOUS_MP3_GetPlaylistInfo(&current, NULL);
	
	if (*macrotitle)
		Q_strncpyz(title, va("%d. %s", current + 1, macrotitle), sizeof(title));
	else
		Q_strncpyz(title, AUDACIOUS_MP3_PLAYERNAME_ALLCAPS, sizeof(title));
	
	return title;
}

void AUDACIOUS_MP3_GetPlaylistInfo(int *current, int *length)
{
	
	if (!AUDACIOUS_MP3_IsPlayerRunning()) 
		return;
	
	if (length)
		*length = qxmms_remote_get_playlist_length(AUDACIOUS_SESSION);
	
	if (current)
		*current = qxmms_remote_get_playlist_pos(AUDACIOUS_SESSION);
}

void AUDACIOUS_MP3_PrintPlaylist_f(void)
{
	int current, length, i;
	char *title, *s;

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
		return;
	}
	
	AUDACIOUS_MP3_GetPlaylistInfo(&current, &length);
	
	for (i = 0 ; i < length; i ++) {
	
		title = qxmms_remote_get_playlist_title(AUDACIOUS_SESSION, i);
		if(!title)
			continue;

		COM_MakePrintable(title);
		if (i == current)
			for (s = title; *s; s++)
				*s |= 128;

		Com_Printf("%3d %s\n", i + 1, title);
		qg_free(title);
	}
	
	return;
}

qboolean AUDACIOUS_MP3_PlayTrack (int num)
{
	int length = -1;

	num -= 1;
	if (num < 0)
		return false;

	AUDACIOUS_MP3_GetPlaylistInfo(NULL, &length);

	if(num > length) {
		Com_Printf("Audacious: playlist got only %i tracks\n", length);
        	return false;
	}

	qxmms_remote_set_playlist_pos(AUDACIOUS_SESSION, num);
	AUDACIOUS_MP3_Play_f();

	return true;
}

void AUDACIOUS_MP3_PlayTrackNum_f(void)
{

	if (!AUDACIOUS_MP3_IsPlayerRunning()) {
		Com_Printf("%s\n", mp3_notrunning_msg);
		return;
	}
	
	if (Cmd_Argc() > 2) {
		Com_Printf("Usage: %s [track #]\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() == 2)
	{
		AUDACIOUS_MP3_PlayTrack (atoi(Cmd_Argv(1)));
		return;
	}

	AUDACIOUS_MP3_Play_f();
}

int AUDACIOUS_MP3_GetPlaylistSongs(mp3_tracks_t *songList, char *filter)
{
	int current, length, i;
	int playlist_size = 0, tracknum = 0, songCount = 0;
	char *s;

	if (!AUDACIOUS_MP3_IsPlayerRunning())
		return 0;

	AUDACIOUS_MP3_GetPlaylistInfo(&current, &length);
	
	for (i = 0 ; i < length; i ++) {
		s = qxmms_remote_get_playlist_title(AUDACIOUS_SESSION, i);
		if(!s)
			continue;
		if(!Q_stristr(s, filter)) {
			qg_free(s);
			continue;
		}
		qg_free(s);
		songCount++;
	}

	if(!songCount)
		return 0;

	songList->name = Z_TagMalloc (sizeof(char *) * songCount, 0);
	songList->num = Z_TagMalloc (sizeof(int) * songCount, 0);

	for (i = 0 ; i < length; i ++) {
		s = qxmms_remote_get_playlist_title(AUDACIOUS_SESSION, i);

		tracknum++;
		if(!s)
			continue;
		if(!Q_stristr(s, filter)) {
			qg_free(s);
			continue;
		}

		if (strlen(s) >= AUDACIOUS_MP3_MAXSONGTITLE-1)
			s[AUDACIOUS_MP3_MAXSONGTITLE-1] = 0;

		COM_MakePrintable(s);
		songList->num[playlist_size] = tracknum;
		songList->name[playlist_size++] = CopyString(va("%i. %s", tracknum, s));
		qg_free(s);

		if(playlist_size >= songCount)
			break;
	}

	return playlist_size;
}

void AUDACIOUS_MP3_Init(void)
{
	
	AUDACIOUS_LoadLibrary();

	if (!AUDACIOUS_MP3_IsActive())
		return;
	
	Cmd_AddCommand("audacious_prev", AUDACIOUS_MP3_Prev_f);
	Cmd_AddCommand("audacious_play", AUDACIOUS_MP3_PlayTrackNum_f);
	Cmd_AddCommand("audacious_pause", AUDACIOUS_MP3_Pause_f);
	Cmd_AddCommand("audacious_stop", AUDACIOUS_MP3_Stop_f);
	Cmd_AddCommand("audacious_next", AUDACIOUS_MP3_Next_f);
	Cmd_AddCommand("audacious_fforward", AUDACIOUS_MP3_FastForward_f);
	Cmd_AddCommand("audacious_rewind", AUDACIOUS_MP3_Rewind_f);
	Cmd_AddCommand("audacious_fadeout", AUDACIOUS_MP3_FadeOut_f);
	Cmd_AddCommand("audacious_shuffle", AUDACIOUS_MP3_Shuffle_f);
	Cmd_AddCommand("audacious_repeat",AUDACIOUS_MP3_Repeat_f);
	Cmd_AddCommand("audacious_playlist", AUDACIOUS_MP3_PrintPlaylist_f);
	Cmd_AddCommand("audacious_songinfo", AUDACIOUS_MP3_SongInfo_f);
	Cmd_AddCommand("audacious_start", AUDACIOUS_MP3_Execute_f);

	audacious_dir = Cvar_Get("audacious_dir", "/usr/bin", CVAR_ARCHIVE);
	audacious_session = Cvar_Get("audacious_session", "0", CVAR_ARCHIVE);
	audacious_messages = Cvar_Get("audacious_messages", "0", CVAR_ARCHIVE);
}

void AUDACIOUS_MP3_Shutdown(void)
{
	
	if (AUDACIOUS_pid) {
		if (!kill(AUDACIOUS_pid, SIGTERM))
			waitpid(AUDACIOUS_pid, NULL, 0);
	}

	AUDACIOUS_FreeLibrary();
}

void AUDACIOUS_MP3_Frame (void)
{
	char *songtitle;
	int total, track = -1;
	static int curTrack = -1;

	if (!audacious_messages->value)
		return;

	if(!((int)(cls.realtime>>8)&8))
		return;

	AUDACIOUS_MP3_GetPlaylistInfo (&track, NULL);
	if(track == -1 || track == curTrack)
		return;

	if(!AUDACIOUS_MP3_GetTrackTime(NULL, &total))
		return;

	songtitle = AUDACIOUS_MP3_Macro_MP3Info();
	if (!*songtitle)
		return;

	curTrack = track;

	Com_Printf ("Audacious Title: %s [%i:%02i]\n", songtitle, total / 60, total % 60);
}
#endif


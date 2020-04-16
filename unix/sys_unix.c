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
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>

#if defined (__linux__)
#include <mntent.h>
#endif

#include <dlfcn.h>

#include "../qcommon/qcommon.h"

#include "../unix/rw_unix.h"

cvar_t *nostdout;

unsigned	sys_frame_time;

uid_t saved_euid;
qboolean stdin_active = true;

// =======================================================================
// General routines
// =======================================================================

void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

	if (nostdout && nostdout->value)
        	return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
}

void Sys_Quit (void)
{
	CL_Shutdown ();
	Qcommon_Shutdown ();
	
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	
	_exit(0);
}

void Sys_Init(void)
{
}

void Sys_Error (char *error, ...)
{ 
	va_list     argptr;
	char        string[1024];

	// change stdin to non blocking
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	CL_Shutdown ();
	Qcommon_Shutdown ();
    
	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	_exit (1);
} 

void Sys_Warn (char *warning, ...)
{ 
	va_list     argptr;
	char        string[1024];
    
	va_start (argptr,warning);
	vsprintf (string,warning,argptr);
	va_end (argptr);
	
	fprintf(stderr, "Warning: %s", string);
} 

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}

char *Sys_ConsoleInput(void)
{
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!dedicated || !dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof(text));
	if (len == 0) { // eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}

/*****************************************************************************/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (game_library) 
		dlclose (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);

	char	name[MAX_OSPATH];
	char	*path;

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");


	Com_Printf("------- Loading %s -------\n", GAME_NAME);

	// now run through the search paths
	path = NULL;
	while (1)
	{
		path = FS_NextPath (path);
		if (!path)
			return NULL;		// couldn't find one anywhere
		Com_sprintf(name, sizeof(name), "%s/%s", path, GAME_NAME);
		game_library = dlopen (name, RTLD_LAZY );
		if (game_library) {
			Com_Printf ("LoadLibrary (%s)\n",name);
			break;
		} 
		else {
			Com_sprintf(name, sizeof(name), "%s", GAME_NAME);
			if (game_library)
			{
				Com_Printf ("LoadLibrary (%s)\n",name);
				break;
			}
		}
	}

	GetGameAPI = (void *)dlsym (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

void Sys_SendKeyEvents (void)
{
#ifndef DEDICATED_ONLY
	if (KBD_Update_fp)
		KBD_Update_fp();
#endif

	// grab frame time 
	sys_frame_time = Sys_Milliseconds();
}

/*****************************************************************************/


#if defined (__linux__)
void
CPU_Info(void)
{
	#define CPUINFO "/proc/cpuinfo"
	#define MEMINFO "/proc/meminfo"
	FILE	*type,
		*model,
		*freq,
		*cache_size,
		*bogomips,
		*meminfo;
	char	type_buf[1024],
		model_buf[1024],
		freq_buf[1024],
		cache_size_buf[1024],
		bogomips_buf[1024],
		meminfo_buf[1024];
	
	type = fopen(CPUINFO, "r");
	if (type) {
		while (fgets(type_buf, sizeof(type_buf), type) != NULL) {
			if (strncmp(type_buf, "vendor_id", 7) == 0) { 
                		strcpy(type_buf, strchr(type_buf, ':') + 2);
                		type_buf[strlen(type_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(type);
	
	model = fopen(CPUINFO, "r");
	if (model) {
		while (fgets(model_buf, sizeof(model_buf), model) != NULL) {
			if (strncmp(model_buf, "model name", 7) == 0) { 
                		strcpy(model_buf, strchr(model_buf, ':') + 2);
                		model_buf[strlen(model_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(model);
	
	freq = fopen(CPUINFO, "r");
	if (freq) {
		while (fgets(freq_buf, sizeof(freq_buf), freq) != NULL) {
			if (strncmp(freq_buf, "cpu MHz", 7) == 0) { 
                		strcpy(freq_buf, strchr(freq_buf, ':') + 2);
                		freq_buf[strlen(freq_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(freq);
	
	cache_size = fopen(CPUINFO, "r");
	if (cache_size) {
		while (fgets(cache_size_buf, sizeof(cache_size_buf), cache_size) != NULL) {
			if (strncmp(cache_size_buf, "cache size", 7) == 0) { 
                		strcpy(cache_size_buf, strchr(cache_size_buf, ':') + 2);
                		cache_size_buf[strlen(cache_size_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(cache_size);
	
	bogomips = fopen(CPUINFO, "r");
	if (bogomips) {
		while (fgets(bogomips_buf, sizeof(bogomips_buf), bogomips) != NULL) {
			if (strncmp(bogomips_buf, "bogomips", 7) == 0) { 
                		strcpy(bogomips_buf, strchr(bogomips_buf, ':') + 2);
                		bogomips_buf[strlen(bogomips_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(bogomips);
	
	meminfo = fopen(MEMINFO, "r");
	if (meminfo) {
		while (fgets(meminfo_buf, sizeof(meminfo_buf), meminfo) != NULL) {
			if (strncmp(meminfo_buf, "MemTotal", 7) == 0) { 
                		strcpy(meminfo_buf, strchr(meminfo_buf, ':') + 4);
                		meminfo_buf[strlen(meminfo_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(meminfo);

	Com_Printf("^3CPU Info^r\n");
		Com_Printf("Processor:%s\n", type_buf);
		Com_Printf("Model:    %s\n", model_buf);
		Com_Printf("Speed:    %s MHz\n", freq_buf);
		Com_Printf("Cache:    %s\n", cache_size_buf);
		Com_Printf("BogoMips: %s M/sec\n", bogomips_buf);
		Com_Printf("Memory:%s\n\n", meminfo_buf);

}
#if 0
void GPU_Info(void) 
{
	#define CARDINFO "/proc/driver/nvidia/cards/0"
	#define GPUINFO "/proc/driver/nvidia/agp/card"
	#define GPUSTATUS "/proc/driver/nvidia/agp/status"
	
	FILE	*model,
		*type,
		*driver,
		*sba,
		*sba_status,
		*fast_writes,
		*fast_writes_status,
		*agp_rates,
		*agp_rates_status,
		*dma;
	char	model_buf[1024],
		type_buf[1024],
		driver_buf[1024],
		sba_buf[1024],
		sba_status_buf[1024],
		fast_writes_buf[1024],
		fast_writes_status_buf[1024],
		agp_rates_buf[1024],
		agp_rates_status_buf[1024],
		dma_buf[1024];

	
	model = fopen(CARDINFO, "r");
	if (model) {
		while (fgets(model_buf, sizeof(model_buf), model) != NULL) {
			if (strncmp(model_buf, "Model", 1) == 0) { 
                		strcpy(model_buf, strchr(model_buf, ':') + 5);
                		model_buf[strlen(model_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(model);
	
	type = fopen(CARDINFO, "r");
	if (type) {
		while (fgets(type_buf, sizeof(type_buf), type) != NULL) {
			if (strncmp(type_buf, "Card Type", 1) == 0) { 
                		strcpy(type_buf, strchr(type_buf, ':') + 4);
                		type_buf[strlen(type_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(type);
	
	driver = fopen(GPUSTATUS, "r");
	if (driver) {
		while (fgets(driver_buf, sizeof(driver_buf), driver) != NULL) {
			if (strncmp(driver_buf, "Driver", 1) == 0) { 
                		strcpy(driver_buf, strchr(driver_buf, ':') + 4);
                		driver_buf[strlen(driver_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(driver);
	
	sba = fopen(GPUINFO, "r");
	if (sba) {
		while (fgets(sba_buf, sizeof(sba_buf), sba) != NULL) {
			if (strncmp(sba_buf, "SBA", 1) == 0) { 
                		strcpy(sba_buf, strchr(sba_buf, ':') + 5);
                		sba_buf[strlen(sba_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(sba);
	
	sba_status = fopen(GPUSTATUS, "r");
	if (sba_status) {
		while (fgets(sba_status_buf, sizeof(sba_status_buf), sba_status) != NULL) {
			if (strncmp(sba_status_buf, "SBA", 1) == 0) { 
                		strcpy(sba_status_buf, strchr(sba_status_buf, ':') + 4);
                		sba_status_buf[strlen(sba_status_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(sba_status);
	
	fast_writes = fopen(GPUINFO, "r");
	if (fast_writes) {
		while (fgets(fast_writes_buf, sizeof(fast_writes_buf), fast_writes) != NULL) {
			if (strncmp(fast_writes_buf, "Fast Writes", 1) == 0) { 
                		strcpy(fast_writes_buf, strchr(fast_writes_buf, ':') + 4);
                		fast_writes_buf[strlen(fast_writes_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(fast_writes);
	
	fast_writes_status = fopen(GPUSTATUS, "r");
	if (fast_writes_status) {
		while (fgets(fast_writes_status_buf, sizeof(fast_writes_status_buf), fast_writes_status) != NULL) {
			if (strncmp(fast_writes_status_buf, "Fast Writes", 1) == 0) { 
                		strcpy(fast_writes_status_buf, strchr(fast_writes_status_buf, ':') + 4);
                		fast_writes_status_buf[strlen(fast_writes_status_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(fast_writes_status);
	
	agp_rates = fopen(GPUINFO, "r");
	if (agp_rates) {
		while (fgets(agp_rates_buf, sizeof(agp_rates_buf), agp_rates) != NULL) {
			if (strncmp(agp_rates_buf, "AGP Rates", 1) == 0) { 
                		strcpy(agp_rates_buf, strchr(agp_rates_buf, ':') + 4);
                		agp_rates_buf[strlen(agp_rates_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(agp_rates);
	
	agp_rates_status = fopen(GPUSTATUS, "r");
	if (agp_rates_status) {
		while (fgets(agp_rates_status_buf, sizeof(agp_rates_status_buf), agp_rates_status) != NULL) {
			if (strncmp(agp_rates_status_buf, "AGP Rate", 1) == 0) { 
                		strcpy(agp_rates_status_buf, strchr(agp_rates_status_buf, ':') + 4);
                		agp_rates_status_buf[strlen(agp_rates_status_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(agp_rates_status);
	
	dma = fopen(CARDINFO, "r");
	if (dma) {
		while (fgets(dma_buf, sizeof(dma_buf), dma) != NULL) {
			if (strncmp(dma_buf, "DMA Size", 1) == 0) { 
                		strcpy(dma_buf, strchr(dma_buf, ':') + 4);
                		dma_buf[strlen(dma_buf) - 1] = '\0';
                	break;
                	}
        	}
	}
	fclose(dma);
	
	Com_Printf("^3GPU Info^r\n");
		Com_Printf("Model:      %s\n", model_buf);
		Com_Printf("Type:       %s\n", type_buf);
		Com_Printf("Driver:     %s\n", driver_buf);
		Com_Printf("SBA:        %s, %s\n", sba_buf, sba_status_buf);
		Com_Printf("Fast Writes:%s, %s\n", fast_writes_buf, fast_writes_status_buf);
		Com_Printf("AGP Rates:  %s, %s\n", agp_rates_buf, agp_rates_status_buf);
		Com_Printf("DMA Size:   %s\n\n", dma_buf);
}
#endif
#endif

void OS_Info(void) 
{
#include <sys/utsname.h>
	struct utsname	info;
	
	uname (&info);
	Com_Printf("^3OS Info^r\n");
	Com_Printf("System: %s\n", info.sysname);
	Com_Printf("Kernel: %s\n", info.release);
	Com_Printf("Arch:   %s\n\n", info.machine);

}

int main (int argc, char **argv)
{
	int 	time, oldtime, newtime;
	int	init_time;
	
	init_time = Sys_Milliseconds ();
	
	
	// go back to real user for config loads
#if 1	
	saved_euid = geteuid();
	seteuid(getuid());
#else	
	if (getuid() == 0 || geteuid() == 0) 
		Sys_Error("Q2P does not run as root.");
#endif

#ifdef DEDICATED_ONLY
	Com_Printf("\n");
	Com_Printf("================= Initialization ====================\n");
	Com_Printf("%s -- Dedicated\n", Q2PVERSION);
	Com_Printf("A Quake2 modification built by QuDos at\n");
	Com_Printf("http://qudos.quakedev.com/\n");
	Com_Printf("Compiled: " __DATE__ " -- " __TIME__ "\n");
	Com_Printf("GCC: %s\n", CC_VERSION);
	Com_Printf("OS: %s\n\n", OP_SYSTEM);
	Com_Printf("=====================================================\n\n");
#endif

	Qcommon_Init(argc, argv);
	
	#if defined (__linux__)
	CPU_Info();
	#if 0
	GPU_Info();
	#endif
	#endif
	OS_Info();
	
	Com_Printf("Load time: %.2fs\n\n", (Sys_Milliseconds () - init_time) * 0.001);

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	}

    oldtime = Sys_Milliseconds ();
    while (1)
    {
// find time spent rendering last frame
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
        Qcommon_Frame (time);
		oldtime = newtime;
    }

}

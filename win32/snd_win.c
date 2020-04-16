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

/**
 * Ben Lane
 * ========
 * 1. Deleted the following items that were used to support DirectSound:
 *     * The functions DSoundError(), DS_CreateBuffers(),
 *       DS_DestroyBuffers(), SNDDMA_InitDirect() and S_Activate().
 *     * The macro iDirectSoundCreate().
 *     * The constant #define SECONDARY_BUFFER_SIZE.
 *     * The enum sndinitstat.
 *     * The variables pDirectSoundCreate, dsound_init, snd_isdirect, pDS,
 *       pDSBuf, pDSPBuf and hInstDS.
 *     * The console variable s_wavonly.
 * 2. Removed DirectSound specific code from functions FreeSound(),
 *     SNDDMA_Init(), SNDDMA_GetDMAPos(), SNDDMA_BeginPainting() and
 *     SNDDMA_Submit().
 **/

#include <float.h>

#include "../client/client.h"
#include "../client/snd_loc.h"
#include "winquake.h"

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400

static qboolean	wav_init;
static qboolean	snd_firsttime = true, snd_iswave;
static qboolean	primary_format_set;

// starts at 0 for disabled
static int	snd_buffer_count = 0;
static int	sample16;
static int	snd_sent, snd_completed;

/* 
 * Global variables. Must be visible to window-procedure function 
 *  so it can unlock and free the data block after it has been played. 
 */ 


HANDLE		hData;
HPSTR		lpData, lpData2;

HGLOBAL		hWaveHdr;
LPWAVEHDR	lpWaveHdr;

HWAVEOUT    hWaveOut; 

WAVEOUTCAPS	wavecaps;

DWORD	gSndBufSize;

MMTIME		mmstarttime;
DWORD	locksize;

qboolean SNDDMA_InitWav (void);

/*
==================
FreeSound
==================
*/
void FreeSound (void)
{
	int		i;

	Com_DPrintf( "Shutting down sound system\n" );
	
	if ( hWaveOut )
	{
		Com_DPrintf( "...resetting waveOut\n" );
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			Com_DPrintf( "...unpreparing headers\n" );
			for (i=0 ; i< WAV_BUFFERS ; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
		}

		Com_DPrintf( "...closing waveOut\n" );
		waveOutClose (hWaveOut);

		if (hWaveHdr)
		{
			Com_DPrintf( "...freeing WAV header\n" );
			GlobalUnlock(hWaveHdr);
			GlobalFree(hWaveHdr);
		}

		if (hData)
		{
			Com_DPrintf( "...freeing WAV buffer\n" );
			GlobalUnlock(hData);
			GlobalFree(hData);
		}

	}
	
	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	wav_init = false;
}

/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qboolean SNDDMA_InitWav (void)
{
	WAVEFORMATEX  format; 
	int	 i;
	HRESULT	 hr;

	Com_Printf( "Initializing wave sound\n" );
	
	snd_sent = 0;
	snd_completed = 0;

	dma.channels = 2;
	dma.samplebits = 16;

	//** DMP change how sampling rate is set
	switch ((int)s_khz->value) {
		case 48:  
			dma.speed = 48000;  
			break;
		case 44:  
			dma.speed = 44100;  
			break;
		case 22:  
			dma.speed = 22050;  
			break;
		case 11:  
			dma.speed = 11025;  
			break;
		default:  
			dma.speed = 44100;  
			break;
	}
	
	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels
		*format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec
		*format.nBlockAlign; 
	
	/* Open a waveform device for output using window callback. */ 
	Com_DPrintf ("...opening waveform device: ");
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
					&format, 
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			Com_Printf ("failed\n");
			return false;
		}

		if (MessageBox (NULL,
			"The sound hardware is in use by another app.\n\n"
			"Select Retry to try to start sound again or Cancel to run Quake 2 with no sound.",
			"Sound not available",
			MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Com_Printf ("hw in use\n" );
			return false;
		}
	} 
	Com_DPrintf( "ok\n" );

	/* 
	 * Allocate and lock memory for the waveform data. The memory 
	 * for waveform data must be globally allocated with 
	 * GMEM_MOVEABLE and GMEM_SHARE flags. 

	*/ 
	Com_DPrintf ("...allocating waveform buffer: ");
	gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, gSndBufSize); 
	if (!hData) 
	{ 
		Com_Printf( " failed\n" );
		FreeSound ();
		return false; 
	}
	Com_DPrintf( "ok\n" );

	Com_DPrintf ("...locking waveform buffer: ");
	lpData = GlobalLock(hData);
	if (!lpData)
	{ 
		Com_Printf( " failed\n" );
		FreeSound ();
		return false; 
	} 
	memset (lpData, 0, gSndBufSize);
	Com_DPrintf( "ok\n" );

	/* 
	 * Allocate and lock memory for the header. This memory must 
	 * also be globally allocated with GMEM_MOVEABLE and 
	 * GMEM_SHARE flags. 
	 */ 
	Com_DPrintf ("...allocating waveform header: ");
	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
		(DWORD) sizeof(WAVEHDR) * WAV_BUFFERS); 

	if (hWaveHdr == NULL)
	{ 
		Com_Printf( "failed\n" );
		FreeSound ();
		return false; 
	} 
	Com_DPrintf( "ok\n" );

	Com_DPrintf ("...locking waveform header: ");
	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 

	if (lpWaveHdr == NULL)
	{ 
		Com_Printf( "failed\n" );
		FreeSound ();
		return false; 
	}
	memset (lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);
	Com_DPrintf( "ok\n" );

	/* After allocation, set up and prepare headers. */ 
	Com_DPrintf ("...preparing headers: ");
	for (i=0 ; i<WAV_BUFFERS ; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE; 
		lpWaveHdr[i].lpData = lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader(hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR)) !=
				MMSYSERR_NOERROR)
		{
			Com_Printf ("failed\n");
			FreeSound ();
			return false;
		}
	}
	Com_DPrintf ("ok\n");

	dma.samples = gSndBufSize/(dma.samplebits/8);
	dma.samplepos = 0;
	dma.submission_chunk = 512;
	dma.buffer = (unsigned char *) lpData;
	sample16 = (dma.samplebits/8) - 1;

	wav_init = true;

	return true;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
qboolean SNDDMA_Init(void)
{
	memset ((void *)&dma, 0, sizeof (dma));

	wav_init = 0;

	if (snd_firsttime || snd_iswave)
	{
		snd_iswave = SNDDMA_InitWav ();
		
		if (snd_iswave)
		{
			if (snd_firsttime)
				Com_Printf ("Wave sound init succeeded\n");
		}
		else
		{
			Com_Printf ("Wave sound init failed\n");
		}
	}

	snd_firsttime = false;

	snd_buffer_count = 1;

	if (!wav_init)
	{
		if (snd_firsttime)
			Com_Printf ("*** No sound device initialized ***\n");

		return 0;
	}

	return 1;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void)
{
	int		s;

	if (wav_init)
	{
		s = snd_sent * WAV_BUFFER_SIZE;
	}


	s >>= sample16;

	s &= (dma.samples-1);

	return s;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting (void)
{
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit(void)
{
	LPWAVEHDR	h;
	int		wResult;

	if (!dma.buffer)
		return;

	if (!wav_init)
		return;

	//
	// find which sound blocks have completed
	//
	while (1)
	{
		if ( snd_completed == snd_sent )
		{
			Com_DPrintf ("Sound overrun\n");
			break;
		}

		if ( ! (lpWaveHdr[ snd_completed & WAV_MASK].dwFlags & WHDR_DONE) )
		{
			break;
		}

		snd_completed++;	// this buffer has been played
	}

//Com_Printf ("completed %i\n", snd_completed);
	//
	// submit a few new sound blocks
	//
	while (((snd_sent - snd_completed) >> sample16) < 8)
	{
		h = lpWaveHdr + ( snd_sent&WAV_MASK );
	if (paintedtime/256 <= snd_sent)
		break;	//	Com_Printf ("submit overrun\n");
//Com_Printf ("send %i\n", snd_sent);
		snd_sent++;
		/* 
		 * Now the data block can be sent to the output device. The 
		 * waveOutWrite function returns immediately and waveform 
		 * data is sent to the output device in the background. 
		 */ 
		wResult = waveOutWrite(hWaveOut, h, sizeof(WAVEHDR)); 

		if (wResult != MMSYSERR_NOERROR)
		{ 
			Com_Printf ("Failed to write block to device\n");
			FreeSound ();
			return; 
		} 
	}
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	FreeSound ();
}


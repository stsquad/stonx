/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
 *	Sound Emulation Routines,
 *	Martin GRIFFiths, 1995.
 *      Till Harbaum, 2001
 *
 *	Last Updated : Febuary 13th 2001. 
 */

/*

  TODO: do not honor new register setting at the next frame but from
        the moment they were received (recalculate buffer) ... but on
	the other side, hardware events are reported all 100ms only 
	anyway ...

 */

#include "main.h"
#include "tosdefs.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include "io.h"

extern int snd_porta;

#ifdef STONX_AUDIO_LINUX
#include <sys/time.h>
#include <sys/ioctl.h>
#define HAVE_AUDIO
#if defined(__linux)
#include <linux/soundcard.h>
#define SOUND_DEVICE "/dev/dsp"
#if 0
#include <linux/sched.h>
#include <linux/unistd.h>
#endif
#elif defined(__NetBSD__) || defined(__OpenBSD__)
/* NetBSD's Linux API emulation, require -lossaudio too */
#include <soundcard.h>
#define SOUND_DEVICE "/dev/audio"
#else
#warning unknow sound system
#define SOUND_DEVICE "/dev/dsp"
#endif
#include <unistd.h>
#define LINUX_DEFAULT_FREQ (44100)
#endif

#ifdef STONX_AUDIO_HPUX
#define HAVE_AUDIO
#include <sys/ioctl.h>
#include <sys/inode.h>
#include <sys/audio.h>
#include <sys/time.h>
#include <unistd.h>
#define HPUX_DEFAULT_FREQ (22050)
#define HPUX_DEFAULT_BUFFERSIZE (16384)
static struct audio_describe audioDesc;
#endif

#ifdef STONX_AUDIO_SPARC
#define HAVE_AUDIO
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#ifndef __FreeBSD__
#include <stropts.h>
#ifdef SOLARIS
#include <sys/audioio.h>
#else
#include <sun/audioio.h>
#endif
#else /* __FreeBSD__ */
#include <sys/time.h>
#include <sys/ioctl.h>
#include <machine/pcaudioio.h>
#define AUDIO_ENCODING_LINEAR AUDIO_ENCODING_RAW
#endif
#include <sys/file.h>
#include <sys/stat.h>
#define SPARC_DEFAULT_FREQ (22050)
static audio_info_t audio_info;
#endif

#ifdef STONX_AUDIO_SGI
#define HAVE_AUDIO
#include <errno.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <audio.h> 
#include <math.h>

#define SGI_MAX_VOL (255)
#define SGI_MIN_VOL (0)

static ALport port;
static ALconfig conf;

#define SGI_DEFAULT_FREQ (22050)
#define SGI_DEFAULT_BUFFERSIZE (2048)
#endif

#ifdef STONX_AUDIO_DEC 
#define HAVE_AUDIO
#include <sys/ioctl.h>
#include <sys/time.h>
#include <mme/mme_api.h>
static HWAVEOUT		mms_device_handle = 0;
static UINT		mms_device_id = 0;
static int		mms_device_volume = FALSE; 
static LPWAVEHDR	mms_lpWaveHeader;
static LPSTR		mms_audio_buffer;
static int		mms_buffers_outstanding = 0;
static int		mms_next_buffer = 0;

#define DEC_DEFAULT_FREQ (22050)
#define DEC_DEFAULT_BUFFERSIZE (2048)
#endif

#ifdef STONX_AUDIO_DIRECTSOUND
#include <windows.h>  /* Need these to compile stadium.cpp */
#include <mmsystem.h> /* */
#include <dsound.h>
#define HAVE_AUDIO
#endif

static pid_t child_pid;	
static pid_t parent_pid;	
static int child_pipe[2];
static int devAudio;
extern long tv_usec;
extern int hz200pt;  
extern int verbose;
static int parent_pipe[2];

#define DEBUG_SND 0

typedef struct
{	void (*machine_audio_open)(void);
	void (*machine_audio_close)(void);
	void (*machine_audio_volume)(long);
} Machine_Audio;


/*
 *	YM Audio Stuff.
 */

#define MAX_SAMPLES (2048)

#define ENV_SIZE (256)
#define NOISE_BITS (14)
#define NOISE_SIZE (1<<NOISE_BITS)

typedef struct 
{	unsigned int finetune;
	unsigned int crsetune;
	unsigned int amplitude;
} YM_Voice;

unsigned int YM_select;

struct Snd
{		
	YM_Voice YM_ChannelA; 
	YM_Voice YM_ChannelB; 
	YM_Voice YM_ChannelC; 
	unsigned int YM_NoiseControl;
	unsigned int YM_MixerControl;
	unsigned int YM_Envelope_fine_period;
	unsigned int YM_Envelope_crse_period;
	unsigned int YM_Envelope_shape;
	unsigned int IO_PortA;
	unsigned int IO_PortB;
	unsigned char YM_flag;
	unsigned char Server_Exit; 
} Snd;


/*
 * Hardware Register emulation functions 
 */ 
  
B LOAD_B_ff8800(void) 
{ 	
  //  fprintf(stderr,"Load $ff8800 - current selected : %d\n",YM_select);
	switch (YM_select)
	{	case 0:  
			return Snd.YM_ChannelA.finetune;
		case 1:
			return Snd.YM_ChannelA.crsetune;
		case 2:  
			return Snd.YM_ChannelB.finetune;
		case 3:
			return Snd.YM_ChannelB.crsetune;
		case 4:  
			return Snd.YM_ChannelC.finetune;
		case 5:
			return Snd.YM_ChannelC.crsetune;
		case 6:
			return Snd.YM_NoiseControl;
		case 7:
			return Snd.YM_MixerControl;
		case 8:
			return Snd.YM_ChannelA.amplitude;
		case 9:
			return Snd.YM_ChannelB.amplitude;
		case 10:
			return Snd.YM_ChannelC.amplitude;
		case 11:
			return Snd.YM_Envelope_fine_period;
		case 12:
			return Snd.YM_Envelope_crse_period;
		case 13:
			return Snd.YM_Envelope_shape;
		case 14:
			return snd_porta & 0xf8;
		case 15:
			return Snd.IO_PortB;
	}

	return 0;
}

B LOAD_B_ff8801(void)
{	return (B) 0xff;			/* checked : always returns 0xff */
}

B LOAD_B_ff8802(void)
{	return (B) 0;
}

B LOAD_B_ff8803(void)
{	return (B) 0xff;			/* checked : always returns 0xff */
}

void STORE_B_ff8800(B v)
{	
  //  fprintf(stderr,"Store $ff8800 : %d\n",v);
  YM_select = v & 15;
}

void STORE_B_ff8801(B v)
{	
  //  fprintf(stderr,"Store $ff8801 : %d\n",v);

  /* should this do what 8800 does??? on a Falcon it seems to!!! */
}

void STORE_B_ff8802(B v)
{	
  //  fprintf(stderr,"Store $ff8802 : %d\n",v);
	switch (YM_select)
	{	case 0:  
			Snd.YM_ChannelA.finetune=v & 0xff;
#if (DEBUG_SND)
			fprintf(stderr,"Channel A fine tune:%d\n",(UL) v);
#endif
			break;	
		case 1:
			Snd.YM_ChannelA.crsetune=v & 0x0f; 
#if (DEBUG_SND)
			fprintf(stderr,"Channel A course tune:%d\n",(UL)v);
#endif
			break;					
		case 2:  
			Snd.YM_ChannelB.finetune=v & 0xff;
			break;	
		case 3:
			Snd.YM_ChannelB.crsetune=v & 0x0f;
			break;	
		case 4:  
			Snd.YM_ChannelC.finetune=v & 0xff;
			break;	
		case 5:
			Snd.YM_ChannelC.crsetune=v & 0x0f;
			break;	
		case 6:
			Snd.YM_NoiseControl=v & 0x1f;
			break;
		case 7:
			Snd.YM_MixerControl=v & 0xff;
#if (DEBUG_SND)
			fprintf(stderr,"Mixer Control:%d\n",(UL) v);
#endif
			break;
		case 8:
			Snd.YM_ChannelA.amplitude=v & 0x1f;
#if (DEBUG_SND)
			if (Snd.YM_ChannelA.amplitude & 0x10)
				fprintf(stderr,"Channel A Enveloping ON:%d\n",v);
#endif
			break;
		case 9:
			Snd.YM_ChannelB.amplitude=v & 0x1f;
#if (DEBUG_SND)
			if (Snd.YM_ChannelB.amplitude & 0x10)
				fprintf(stderr,"Channel B Enveloping ON:%d\n",v);
#endif
			break;
		case 10:
			Snd.YM_ChannelC.amplitude=v & 0x1f;
#if (DEBUG_SND)
			if (Snd.YM_ChannelC.amplitude & 0x10)
				fprintf(stderr,"Channel C Enveloping ON:\n");
#endif
			break;
		case 11:
			Snd.YM_Envelope_fine_period=v & 0xff;
#if (DEBUG_SND)
			fprintf(stderr,"Fine envelope:%d\n",(UL) v);
#endif
			break;
		case 12:
			Snd.YM_Envelope_crse_period=v & 0xff;
#if (DEBUG_SND)
			fprintf(stderr,"Course envelope:%d\n",(UL) v);
#endif
			break;
		case 13:
			Snd.YM_Envelope_shape=v & 0x0f;
#if (DEBUG_SND)
			fprintf(stderr,"Envelope shape set/trigger :%d\n",v);
#endif
			Snd.YM_flag = 1;
			break;
		case 14:
			snd_porta = v;
			break;
		case 15:
			Snd.IO_PortB = v;
			write_parallel(v);
			break;
	}
}

void STORE_B_ff8803(B v)
{	/*fprintf(stderr,"Store $ff8803 : %d\n",v);*/

	
}



#ifdef HAVE_AUDIO


static void init_noise(void);
static void init_envelope(void);
static void init_cliptable(void);

static Machine_Audio Audio;
static unsigned char *audio_buffer;
static int audio_buffsize; 
static int audio_frequency;
static int audio_samplebits;
static int samples_to_generate;
static int FREQ_base; 
static int STE_Microwire_volume = 64;
static unsigned int YM_voice1_IntFrac;
static unsigned int YM_voice2_IntFrac;
static unsigned int YM_voice3_IntFrac;
static unsigned int Env_IntFrac=0;
static unsigned int Noise_IntFrac=0;


/* Envelope Definition stuff. */

typedef struct 
{	unsigned int Def[8];	
} Envelope_Definition;

static Envelope_Definition Envelope00xx = {{ 1,0,0,0,0,0,0,0 }};
static Envelope_Definition Envelope01xx = {{ 0,1,0,0,0,0,0,0 }};
static Envelope_Definition Envelope1000 = {{ 1,0,1,0,1,0,1,0 }};
static Envelope_Definition Envelope1001 = {{ 1,0,0,0,0,0,0,0 }};
static Envelope_Definition Envelope1010 = {{ 1,0,0,1,1,0,0,1 }};
static Envelope_Definition Envelope1011 = {{ 1,0,1,1,1,1,1,1 }};
static Envelope_Definition Envelope1100 = {{ 0,1,0,1,0,1,0,1 }};
static Envelope_Definition Envelope1101 = {{ 0,1,1,1,1,1,1,1 }};
static Envelope_Definition Envelope1110 = {{ 0,1,1,0,0,1,1,0 }};
static Envelope_Definition Envelope1111 = {{ 0,1,0,0,0,0,0,0 }};

static Envelope_Definition *Envelopes[] = 
{
	&Envelope00xx,
	&Envelope00xx,
	&Envelope00xx,
	&Envelope00xx,
	&Envelope01xx,
	&Envelope01xx,
	&Envelope01xx,
	&Envelope01xx,
	&Envelope1000,
	&Envelope1001,
	&Envelope1010,
	&Envelope1011,
	&Envelope1100,
	&Envelope1101,
	&Envelope1110,
	&Envelope1111
};

static int SquarewaveW[16] =
{ 	-128,-128,-128,-128,-128,-128,-128,-128,	
	+127,+127,+127,+127,+127,+127,+127,+127
}; 

static int *clip_tab;
static int clip_table[512];
static unsigned char envs[16][ENV_SIZE*4];
static char Noise_Table[NOISE_SIZE];
static int Envelope[MAX_SAMPLES];
static int Noise[MAX_SAMPLES];
static int mix_buffer[MAX_SAMPLES];


static int amp_tab[16]= { 	 0, 1, 1, 2, 3,  4,  5,  7,	
				12,20,28,44,70,110,165,255	};

/* 
 *	Generate a single YM 2149 Voice.
 */

static void generate_YM_voice(YM_Voice *YM,unsigned int *Current_IntFrac,int TONE_MASK)
{ 	
	int i;
	unsigned int F=*Current_IntFrac;
	unsigned int YM_freq;
	unsigned int tune = YM->finetune + (YM->crsetune<<8);
	int *V = mix_buffer;
	if (tune)
	{	float t  = ((float)FREQ_base*65536*256*16) / tune;
		YM_freq = (unsigned int) t;
	}
	else
		YM_freq = 0;

	if (YM->amplitude & 0x10)	/* voice using Envelope? */
	{	
		if (Snd.YM_MixerControl & TONE_MASK)
		{	
			if (!(Snd.YM_MixerControl & (TONE_MASK<<3)))
			{	/* Has Envelope,No Tone, Has Noise */ 
				for (i = 0 ; i < samples_to_generate  ; i++)
					V[i] = clip_tab[V[i]+((Noise[i]*Envelope[i])>>8)];
			}
		} else
		{	
			if (Snd.YM_MixerControl & (TONE_MASK<<3))
			{ /* Has Envelope,Has Tone, No Noise */
				for (i = 0 ; i < samples_to_generate ; i++,F+=YM_freq)
					V[i] = clip_tab[V[i]+((SquarewaveW[F>>28]*Envelope[i])>>8)];
			} else
			{ /* Has Envelope,Has Tone and Noise */ 
				for (i = 0 ; i < samples_to_generate  ; i++,F+=YM_freq)
					V[i] = clip_tab[V[i]+((clip_tab[SquarewaveW[F>>28]+Noise[i]]*Envelope[i])>>8)];
 			}
		}
	} else				/* voice using amplitude */
	{	
#if (1)
		int amp = amp_tab[YM->amplitude];
#else
		int amp = (YM->amplitude) * 16;
#endif

		if (Snd.YM_MixerControl & TONE_MASK)
		{	
			if (!(Snd.YM_MixerControl & (TONE_MASK<<3)))
			{	/* No Envelope, No Tone, Has Noise */ 
				for (i = 0 ; i < samples_to_generate  ; i++)
					V[i] = clip_tab[V[i]+((Noise[i]*amp)>>8)];
			}	
		} else
		{	
			if (Snd.YM_MixerControl & (TONE_MASK<<3))
			{ /* No Envelope, Has Tone, No Noise */
				for (i = 0 ; i < samples_to_generate ; i++,F+=YM_freq)
					V[i] = clip_tab[V[i]+((SquarewaveW[F>>28]*amp)>>8)]; 
			} else
			{ /* No Envelope, Has Tone, Has Noise */ 
				for (i = 0 ; i < samples_to_generate  ; i++,F+=YM_freq)
					V[i] = clip_tab[V[i]+((clip_tab[SquarewaveW[F>>28]+Noise[i]]*amp)>>8)];
			} 
		} 
	}	 
	*Current_IntFrac = F; 
} 

void audio_generate(void)
{	
  int i;
  if (Snd.YM_flag) {
    Env_IntFrac=0; 
    Snd.YM_flag=0;
  }

  if ((Snd.YM_MixerControl & 0x38) != 0x38) { /*any noise channels? */ 
    unsigned int Noise_Freq;
    if (Snd.YM_NoiseControl > 0) {	
      Noise_Freq = (FREQ_base<<(4+8))/(Snd.YM_NoiseControl);
    } else
      Noise_Freq = 0;
    for (i = 0 ; i < samples_to_generate ; i++,Noise_IntFrac+=Noise_Freq)
      Noise[i] = (int) Noise_Table[(Noise_IntFrac >> 16) & (NOISE_SIZE-1)];
  }
  
  if ( (Snd.YM_ChannelA.amplitude & 0x10) || (Snd.YM_ChannelB.amplitude & 0x10) || (Snd.YM_ChannelC.amplitude & 0x10) ) 
    {	/* Generate Frequency Shifted Envelope */
      unsigned char *Env= &envs[Snd.YM_Envelope_shape][0];
      unsigned int env_period = (Snd.YM_Envelope_crse_period<<8) + Snd.YM_Envelope_fine_period;
      unsigned int Env_Freq;
		if (env_period > 0)
		{	Env_Freq = (FREQ_base<<16) / env_period;
		} else
			Env_Freq = 0;
		for (i = 0 ; i < samples_to_generate ; i++)
		{	Envelope[i] =  amp_tab[Env[Env_IntFrac>>16]>>4];
			Env_IntFrac += Env_Freq;
			/* this is particularly crap... */
			while (Env_IntFrac >= ((ENV_SIZE*4)<<16)) 
				Env_IntFrac -= ((ENV_SIZE*2)<<16);	
		}
	}

	memset(mix_buffer,0,samples_to_generate*sizeof(int)); 

	generate_YM_voice(&Snd.YM_ChannelA,&YM_voice1_IntFrac,1); 
	generate_YM_voice(&Snd.YM_ChannelB,&YM_voice2_IntFrac,2);
	generate_YM_voice(&Snd.YM_ChannelC,&YM_voice3_IntFrac,4);
}



/* 
 *	Init clip table 
 */

void init_cliptable(void)
{	int i;	
	for (i=0 ; i < 512 ; i++)
	{	int j = i - 256;	
		clip_table[i]= j<-128 ? -128 : j>127 ? 127 : j; 
	}
	clip_tab = &clip_table[256];
}

/*
 *	Create a 'noise' sample.
 */

static void init_noise(void)
{	int i;
	srand(0xa354e767);
	for (i = 0 ; i < NOISE_SIZE ; i++)
		Noise_Table[i] = (rand() & 0xff)-128;
}

/*
 *	Initialise tables for the '16' YM Amplitude Envelopes.
 */
 
static void init_envelope(void)
{	int i,j,k;
	for (i = 0 ; i < 15 ; i++)
	{	for (j = 0 ; j < 4; j++)
		{ 	int Env_Src = Envelopes[i]->Def[2*j]*(ENV_SIZE-1);
			int Env_Dst = Envelopes[i]->Def[2*j+1]*(ENV_SIZE-1);
			int Env_Cur = Env_Src;
			for (k = 0 ; k < ENV_SIZE ; k++)
			{	envs[i][(j*ENV_SIZE)+k] = Env_Cur;
				if (Env_Dst > Env_Src)
					Env_Cur++;
				else if (Env_Dst < Env_Src)
					Env_Cur--;
			}
		}
	}
}






/*
 *		DirectSound associated stuff
 *		(updated Griff April 97)
 *
 */

#ifdef STONX_AUDIO_DIRECTSOUND

static LPDIRECTSOUND       lpDirectSound = NULL;
static LPDIRECTSOUNDBUFFER lpPrimary     = NULL;
static LPDIRECTSOUNDBUFFER lpDSB		 = NULL;
static BOOL					ds_emulated_sound = 0;
static BOOL					audio_running = 0;
static int SoundBufferOffset;
static int SoundBufferSize;
static int primary_size;
static int open_sound(LPDIRECTSOUND *lpDS,HWND hwnd) {	

    HRESULT hr;

	if (DS_OK == DirectSoundCreate(NULL,lpDS,NULL)) {	

        hr = (*lpDS)->SetCooperativeLevel(hwnd,DSSCL_EXCLUSIVE);
        
		if (hr == DS_OK) {
		
            return 0;    /* success */
			
		}
		
	} 

	*lpDS = NULL;

	return 1;

}


static int close_sound(LPDIRECTSOUND lpDS) 
{
	if (lpDS != NULL)
    {	lpDS->Release();
		lpDS = NULL;
	}

    return 0;

}


static int ds_get_buffer_size(LPDIRECTSOUNDBUFFER buf) 		
{	DSBCAPS dsbcaps;
	memset(&dsbcaps, 0, sizeof(DSBCAPS));		/* Zero it out. */
	dsbcaps.dwSize = sizeof(DSBCAPS);
	buf->GetCaps(&dsbcaps);
	return dsbcaps.dwBufferBytes;
}

void DS_audio_open(void) 
{	extern HWND hWndMain;

	open_sound(&lpDirectSound,hWndMain);		

	if (lpDirectSound != NULL)	/* just incase there is no directsound object(i.e. no sound-card then don't try to create a primary object) */
	{	DSCAPS dscaps;
		WAVEFORMATEX pcmwf;
		DSBUFFERDESC dsbdesc;
		DSBCAPS dsbcaps;

		memset(&dscaps, 0, sizeof(DSCAPS));     
		dscaps.dwSize    = sizeof(DSCAPS);
		lpDirectSound->GetCaps(&dscaps);

		ds_emulated_sound = (dscaps.dwFlags & DSCAPS_EMULDRIVER) != 0;  
		
		memset(&pcmwf, 0, sizeof(WAVEFORMATEX));	/* Set up wave format structure. */
		pcmwf.wFormatTag = WAVE_FORMAT_PCM;
		pcmwf.nSamplesPerSec = 22050;

		if (dscaps.dwFlags & DSCAPS_PRIMARYSTEREO)
			pcmwf.nChannels = 2;
		else
			pcmwf.nChannels = 1;

		if (dscaps.dwFlags & DSCAPS_PRIMARY16BIT)
			pcmwf.wBitsPerSample = 16;
		else
			pcmwf.wBitsPerSample = 8;
		pcmwf.nBlockAlign = (pcmwf.wBitsPerSample / 8) * pcmwf.nChannels;
		pcmwf.nAvgBytesPerSec =  pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;

		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));		/* Zero it out. */
		dsbdesc.dwSize = sizeof(DSBUFFERDESC);
		dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
		dsbdesc.dwBufferBytes = 0;  
		dsbdesc.lpwfxFormat = NULL;						/* Must be NULL for primary buffers. */

		if (lpDirectSound->CreateSoundBuffer(&dsbdesc, &lpPrimary, NULL) == DS_OK)
		{
			if (lpPrimary->SetFormat(&pcmwf) == DS_OK)
			{
				primary_size = ds_get_buffer_size(lpPrimary);

				audio_frequency = 22050;
				audio_samplebits = 	8;
				FREQ_base = (2000000 / audio_frequency);

				memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); /* Zero it out. */
				dsbdesc.dwSize = sizeof(DSBUFFERDESC);
				dsbdesc.dwFlags = DSBCAPS_CTRLDEFAULT;
				SoundBufferSize = dsbdesc.dwBufferBytes = 4096;
				SoundBufferOffset = 0;
				dsbdesc.lpwfxFormat = &pcmwf;				
				memset(&pcmwf, 0, sizeof(WAVEFORMATEX));	/* Set up wave format structure. */
				pcmwf.wFormatTag = WAVE_FORMAT_PCM;
				pcmwf.nChannels = 1;
				pcmwf.nSamplesPerSec = 22050;
				pcmwf.nBlockAlign = 1;
				pcmwf.nAvgBytesPerSec =  pcmwf.nSamplesPerSec * 1;
				pcmwf.wBitsPerSample = 8;
				
				if(DS_OK == lpDirectSound->CreateSoundBuffer(&dsbdesc, &lpDSB, NULL)) 
				{	lpDSB->SetFrequency(22050);
					lpDSB->Play(0, 0, DSBPLAY_LOOPING);
					audio_running = 1;
					return;			/* success */

				}	


					
			}
		}
	}
 
	
}

void DS_audio_volume(long volume) 
{
}

void DS_audio_close(void) 
{	
	if (lpDSB != NULL)
	{	lpDSB->Release();
		lpDSB = NULL;
	}

    if (lpPrimary != NULL) {
	
        lpPrimary->Release();
        lpPrimary = NULL;

    }
	
    close_sound(lpDirectSound);
	
}


static volatile  int SamplesToWrite(void)
{		DWORD dwPlayCursor,dwWriteCursor;
		lpDSB->GetCurrentPosition(&dwPlayCursor,&dwWriteCursor);
		if (SoundBufferOffset <= dwPlayCursor)
			return (dwPlayCursor - SoundBufferOffset);
		else
			return (SoundBufferSize - SoundBufferOffset + dwPlayCursor);
}


void audio_update(void)
{	
	if (audio_running)
	{	char *buffer1;
		char *buffer2;
		DWORD buff1_len,buff2_len;
		int i;
		int *src = &mix_buffer[0];
		samples_to_generate = SamplesToWrite();
		audio_generate();
		if (lpDSB->Lock(SoundBufferOffset,samples_to_generate,(int *)&buffer1,&buff1_len,(int *)&buffer2,&buff2_len,0L) == DS_OK)
		{	for (i = 0 ; i < buff1_len ; i++)
				buffer1[i] = (*src++) ^0x80;
			if (buff2_len)
			{	for (i = 0 ; i < buff2_len ; i++)
					buffer2[i] = (*src++) ^0x80;
			}
			SoundBufferOffset = (SoundBufferOffset+samples_to_generate) % SoundBufferSize;;
			lpDSB->Unlock(buffer1,buff1_len,buffer2,buff2_len);
		}

	}

}


#else

int audio_server(void) {
  register int i;
  fd_set rd_fds,wr_fds,ex_fds;
  struct timeval timeout, start, now;
  int ret;
  long to = tv_usec;
 
  if (nice(-15) == -1)	/* increase cpu time for sound */ 
    nice(0);
  
  FD_ZERO(&rd_fds);
  FD_ZERO(&wr_fds);
  FD_ZERO(&ex_fds);
  
  Audio.machine_audio_open(); 
  
  while (1) {	
    FD_SET(parent_pipe[0],&rd_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = to;

    gettimeofday(&start, NULL);
    ret = select(FD_SETSIZE,&rd_fds,&wr_fds,&ex_fds,&timeout);
    gettimeofday(&now, NULL);
    
    /* some error occured */
    if (ret <= 0) {	
      if (kill(parent_pid,0) < 0) {	
	Audio.machine_audio_close();
	if (verbose)
	  fprintf(stderr,"STonX audio server exited(parent process died).\n");
	exit(0);	
      }	
    }
    
    if (FD_ISSET(parent_pipe[0],&rd_fds)) {
      if(read(parent_pipe[0],&Snd,sizeof(struct Snd)) != sizeof(struct Snd))
	perror("sound server reading pipe:");      

      /* adjust timout value for select */
      if((now.tv_usec - start.tv_usec) >= tv_usec) to = tv_usec;
      else to = tv_usec - (now.tv_usec - start.tv_usec);
    } else {

      /* timeout, reload with full timeout value */
      to = tv_usec;

      if (Snd.Server_Exit) {	
	Audio.machine_audio_close();
	exit(0);	
      }
      
      audio_generate();
      
      if (audio_samplebits == 8) {	
	char *buf = (char *) audio_buffer;
	for (i = 0 ; i < samples_to_generate ; i++)
	  buf[i] = mix_buffer[i]^0x80;

#ifdef STONX_AUDIO_DEC
	mmeProcessCallbacks();
	{	
	  MMRESULT	status;
	  int		bytes;
	  mms_lpWaveHeader->lpData = (LPSTR)(mms_audio_buffer);
	  mms_lpWaveHeader->dwBufferLength = samples_to_generate; 
	  memcpy( mms_lpWaveHeader->lpData, buf, samples_to_generate);
	  status = waveOutWrite(mms_device_handle, mms_lpWaveHeader, sizeof(WAVEHDR));
	  if( status != MMSYSERR_NOERROR ) {	
	    fprintf(stderr,"waveOutWrite failed - status = %d\n",status);
	  }
	}
	
#else
	{ 
	  static long cnt=0, totwr=0;
	  ssize_t written;
	  static time_t start=0, now;
	  static called = 0;
	  
	  cnt += samples_to_generate;
	  called++;
	  
	  if(start == 0) start = time(NULL);
	  now = time(NULL); if(now == start) now = start+1;
	  
#if 0
	  if(!(called % 100))
	    printf("write %d bytes to %d (rate = %d bytes/sec, written = %d bytes/sec %d)\n", samples_to_generate, devAudio, cnt/(now-start), totwr/(now-start), called/(now-start));
#endif
	  
	  /* some soundcards seem to start quite slow */
	  /* delay startup for them */
	  if(called >100) {
	    
	    written = write(devAudio,audio_buffer,samples_to_generate);	    
	    if(written >= 0)
	      totwr += written;
	  }
	}
#endif
      } else {	
	UW *buf = (UW *) audio_buffer;
#ifdef STONX_AUDIO_SGI
	for (i = 0 ; i < samples_to_generate ; i++)
	  buf[i] = (mix_buffer[i]<<6); 
	ALwritesamps(port,audio_buffer, (samples_to_generate) );
#else
	for (i = 0 ; i < samples_to_generate ; i++) 
	  buf[i] = (mix_buffer[i]<<6)^0x8000; 
	
	write(devAudio,audio_buffer,samples_to_generate*2);
#endif
      }
    }
  }
}


/*
 * 	Update Audio 
 *	'STonX' "writes" via a pipe to the forked sound server.
 */

void audio_update(void)
{	
  write(parent_pipe[1],&Snd,sizeof(struct Snd));
  Snd.YM_flag = 0;
}


#endif		/* STONX_AUDIO_DIRECTSOUND */


/* 
 *	Linux Specific...
 */

#ifdef STONX_AUDIO_LINUX
 
static void LINUX_audio_open(void)
{	int tmp,ret;	
	devAudio = open(SOUND_DEVICE, O_RDWR | O_NDELAY, 0);
  	if (devAudio == -1)
  	{	fprintf(stderr,"Linux Audio:Can't Open Audio device\n");
		exit(1);
  	}

	/* SETUP No. of sample bits */

  	tmp = 8; 
  	ret = ioctl(devAudio, SNDCTL_DSP_SAMPLESIZE, &tmp);
  	if (ret == -1) 
	{	fprintf(stderr,"Linux Audio: Error setting sample size\n");
		exit(1);
	}
	audio_samplebits = 8;

  	/* SETUP Mono/Stereo - mono(0) stereo(1) */

  	tmp = 0;  
  	ret = ioctl(devAudio, SNDCTL_DSP_STEREO, &tmp);
	if (ret == -1) 
	{	fprintf(stderr,"Linux Audio: Error setting mono/stereo\n");
		exit(1);
	}

	/* Get the Size of Audio Hardware Buffer */
	
  	ret = ioctl(devAudio, SNDCTL_DSP_GETBLKSIZE, &tmp);
  	if (ret == -1) 
	{	fprintf(stderr,"Linux Audio: Error getting Audio buffer size\n");
		exit(1);
	}
	audio_buffsize = tmp;

	if (verbose)
		fprintf(stderr,"Linux Audio : Hardware buffer size:%d\n",audio_buffsize);

	if ((audio_buffer =
		(unsigned char *)malloc(audio_buffsize * sizeof(char))) == NULL)
	{
		fprintf(stderr,"Linux Audio: Failed to allocate audio buffer memory\n");
		exit(1);
	}

	/* Setup Hardware replay frequency */

	tmp = LINUX_DEFAULT_FREQ;

      	ret = ioctl(devAudio, SNDCTL_DSP_SPEED, &tmp);
      	if (ret == -1)
        {	fprintf(stderr,"Linux_Audio: Error setting sound frequency(%dhz)\n" ,audio_frequency);
		exit(1);
	}
	audio_frequency = tmp;
	if (verbose)
		fprintf(stderr,"Linux Audio : Sound Frequency:%d\n",audio_frequency);

	FREQ_base = (2000000 / audio_frequency);
	samples_to_generate = ((int)(audio_frequency /
				     (1000000.0/(double)(tv_usec*hz200pt)))) /* *99/100 */; 

	if (verbose)
		fprintf(stderr,"Linux Audio : Sound Configuration Successful.\n");
}

static void LINUX_audio_close(void)
{	
	free(audio_buffer);
	close(devAudio);
}

static void LINUX_audio_volume(long x)
{
}

#endif

/*
 * 	HPUX Specific... 
 */

#ifdef STONX_AUDIO_HPUX
 
static void HPUX_audio_open(void)
{ 	struct audio_limits limits;
	if ((devAudio = open ("/dev/audio", O_WRONLY | O_NDELAY, 0)) < 0)
   	{	if (errno == EBUSY) 
			fprintf(stderr,"HPUX Audio: Audio device is busy\n");
	   	else
			fprintf(stderr,"HPUX Audio: Error opening audio device\n");
		exit(1);
	}

	/* Get description of /dev/audio: */

    	if (ioctl (devAudio, AUDIO_DESCRIBE, &audioDesc))
      	{	fprintf(stderr,"HPUX Audio : Error setting ioctl AUDIO_DESCRIBE on /dev/audio\n");
		exit(1);
	}	

	/* Set Audio Format */

	if (ioctl (devAudio, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT))
    	{	fprintf(stderr,"HPUX Audio: Error setting ioctl AUDIO_SET_DATA_FORMAT on /dev/audio");
		exit(1);
	}
	audio_samplebits = 16;

	/* Set Mono Sound */

	if (ioctl (devAudio, AUDIO_SET_CHANNELS, 1))
	{	fprintf(stderr,"HPUX Audio: Error setting ioctl AUDIO_SET_CHANNELS on /dev/Audio");
		exit(1);
	}

	/* Set device internal buffer size: */

	audio_buffsize = HPUX_DEFAULT_BUFFERSIZE;
	if (ioctl (devAudio, AUDIO_SET_TXBUFSIZE, audio_buffsize))
	{	fprintf (stderr,"HPUX Audio: Error setting ioctl AUDIO_SET_TXBUFSIZE on /dev/audio\n");
		exit(1);
	}
	if (verbose)
		fprintf(stderr,"HPUX Audio: Hardware buffer size:%d\n",audio_buffsize);

	if ((audio_buffer =
		(unsigned char *)malloc(MAX_SAMPLES * sizeof(UW))) == NULL)
	{
		fprintf(stderr,"HPUX Audio: Failed to allocate audio buffer memory\n");
		exit(1);
	}

	/* Set Audio frequency */

	audio_frequency = HPUX_audio_closest_freq(HPUX_DEFAULT_FREQ); 

	if (ioctl (devAudio, AUDIO_SET_SAMPLE_RATE, audio_frequency))
	{	fprintf(stderr,"HPUX Audio: Error setting ioctl AUDIO_SET_SAMPLE_RATE on /dev/audio\n");
		exit(1);
	}

	if (verbose)
		fprintf(stderr,"HPUX Audio: Sound Frequency:%d\n",audio_frequency);

	FREQ_base = (2000000 / audio_frequency);
	HPUX_audio_volume(100);
	samples_to_generate = (audio_frequency / (1000000.0/(tv_usec*hz200pt)) )+1; 
	if (verbose)
		fprintf(stderr,"HPUX Audio: Sound Configuration Successful.\n");

}

static void HPUX_audio_close(void)
{
	if (ioctl (devAudio, AUDIO_DRAIN, 0))
      		fprintf(stderr,"HPUX Audio: Error setting ioctl AUDIO_DRAIN on /dev/audio\n");
	close(devAudio);
	free(audio_buffer);
}

static void HPUX_audio_volume(long volume)
{	struct audio_describe description;
	struct audio_gains gains;
	float floatvolume = (float) volume / 100.0;

	if (ioctl (devAudio, AUDIO_DESCRIBE, &description))
	{	fprintf(stderr,"HPUX Audio: Error setting ioctl AUDIO_DESCRIBE on /dev/audio\n");
		return;
	}
	if (ioctl (devAudio, AUDIO_GET_GAINS, &gains))
	{	fprintf(stderr,"HPUX Audio: Error setting ioctl AUDIO_GET_GAINS on /dev/audio\n");
		return;
	}
	gains.transmit_gain = (int)((float)description.min_transmit_gain + (float)(description.max_transmit_gain - description.min_transmit_gain) * floatvolume);
	if (ioctl (devAudio, AUDIO_SET_GAINS, &gains))
	{	fprintf (stderr,"HPUX Audio: Error setting ioctl AUDIO_SET_GAINS on /dev/audio\n");
		return;
	}	
}

static int HPUX_audio_closest_freq(int ifreq)
{	unsigned int rate_index;
	long nearest_frequency = 8000;
	long frequency_diff = 999999;
	for (rate_index = 0; rate_index < audioDesc.nrates; ++rate_index)
	if (abs (audioDesc.sample_rate[rate_index] - ifreq) <= frequency_diff)
	{	nearest_frequency = audioDesc.sample_rate[rate_index];
		frequency_diff = abs (audioDesc.sample_rate[rate_index] - ifreq); 
	}
	return nearest_frequency;
}

#endif

/*
 * 	SPARC specific...
 */

#ifdef STONX_AUDIO_SPARC

static void SPARC_audio_open(void)
{	int ret;
#ifdef SOLARIS
	audio_device_t type;
#else
   	int type;
#endif
	devAudio = open("/dev/audio", O_WRONLY | O_NDELAY);
	if (devAudio == -1)
	{	if (errno == EBUSY) 
			fprintf(stderr,"SPARC Audio: Audio device is busy\n");
		else 
			fprintf(stderr,"SPARC Audio: Error %lx opening audio device. - ",errno);
		exit(1);				
	}
#ifndef __FreeBSD__
	ret = ioctl(devAudio, AUDIO_GETDEV, &type);

#ifdef SOLARIS
	if ( (   (strcmp(type.name, "SUNW,dbri"))   /* Not DBRI (SS10's) */
	&& (strcmp(type.name, "SUNW,CS4231"))) /* and not CS4231 (SS5's) */
		|| ret) /* or ioctrl failed */
#else
	if (ret || (type==AUDIO_DEV_UNKNOWN) || (type==AUDIO_DEV_AMD) )
#endif
	{ /* AU Audio - Doh! */ 
		audio_samplebits = 8;
		audio_frequency = 8000;
    		fprintf(stderr,"SPARC Audio: Unsupported Audio Device\n");
		exit(1);
		/*Gen_Signed_2_Ulaw(); */
  	} else /* DBRI or CS4231 */
#endif /* __FreeBSD__ */
	{
		AUDIO_INITINFO(&audio_info);
		audio_info.play.sample_rate = 11025;
		audio_info.play.precision = 16;
		audio_info.play.channels = 1;
		audio_info.play.encoding = AUDIO_ENCODING_LINEAR;
		ret = ioctl(devAudio, AUDIO_SETINFO, &audio_info);
		if (ret)
    		{	fprintf(stderr,"SPARC Audio: Error setting ioctl AUDIO_SETINFO\n");
			exit(1);
		}
		audio_frequency = 11025; 
		audio_samplebits = 16;
	}
	{ 	audio_info_t a_info;
		AUDIO_INITINFO(&a_info);
		a_info.play.sample_rate = SPARC_DEFAULT_FREQ;
		ret = ioctl(devAudio, AUDIO_SETINFO, &a_info);
		if (ret != -1)
			audio_frequency = SPARC_DEFAULT_FREQ; 
	}

	if ((audio_buffer = 
		(unsigned char *) malloc(MAX_SAMPLES * sizeof(UW))) == NULL)
	{
		fprintf(stderr,"SPARC Audio: Failed to allocate audio buffer memory\n");
		exit(1);
	}

	FREQ_base = (2000000 / audio_frequency);
	samples_to_generate = (audio_frequency / (1000000.0/(tv_usec*hz200pt)) )+1; 
	if (verbose)
	{	fprintf(stderr,"SPARC Audio: Sound Frequency:%d\n",audio_frequency);
		fprintf(stderr,"SPARC Audio: Sound Configuration Successful.\n");	}
}

static void SPARC_audio_close(void)
{	
	close (devAudio);
	free(audio_buffer);
}

static void SPARC_audio_volume(long volume)
{	audio_info_t a_info;
	AUDIO_INITINFO(&a_info);
    	a_info.play.gain = AUDIO_MIN_GAIN + ((volume * (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN)) / 100);
	if (a_info.play.gain > AUDIO_MAX_GAIN)	
		a_info.play.gain = AUDIO_MAX_GAIN;
	ioctl(devAudio, AUDIO_SETINFO, &a_info);
}

static void SPARC_audio_closest_freq(long ifreq)
{
	if (audio_samplebits==16)
	{	static int valid[] = { 8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800, 44100, 48000, 0};
		long i = 0;
		long best = 8000;
		while(valid[i])
		{
			if (abs(valid[i] - ifreq) < abs(best - ifreq)) 
				best = valid[i];
			i++;
		}
		return(best);
	} else
		return(8000);
}

#endif

#ifdef STONX_AUDIO_SGI

static void SGI_audio_open(void)
{	long sgi_params[4];
  	conf = ALnewconfig();
  	ALsetwidth(conf, AL_SAMPLE_16);
  	ALsetchannels(conf, AL_MONO);
	ALsetqueuesize(conf, SGI_DEFAULT_BUFFERSIZE);
	port = ALopenport("STonX Mono", "w", conf);
	if (port == 0)
	{	fprintf(stderr,"SGI Audio: Cannot Open SGI Audio device\n");
		exit(1);
  	}

	/* get default frequency */
	sgi_params[0] = AL_OUTPUT_RATE;
	sgi_params[1] = 11025;
	ALsetparams(AL_DEFAULT_DEVICE, sgi_params, 2);
	ALgetparams(AL_DEFAULT_DEVICE, sgi_params, 2);

	sgi_params[0] = AL_OUTPUT_RATE;
	sgi_params[1] = SGI_DEFAULT_FREQ;
	ALsetparams(AL_DEFAULT_DEVICE, sgi_params, 2);
	audio_frequency = SGI_DEFAULT_FREQ;

	if ((audio_buffer = (unsigned char *)
		malloc(MAX_SAMPLES * sizeof(char))) == NULL)
	{
		fprintf(stderr,"SGI Audio: Failed to allocate audio buffer memory\n");
		exit(1);
	}

	audio_samplebits = 16;
	FREQ_base = (2000000 / audio_frequency);
	samples_to_generate = (audio_frequency / (1000000.0/(tv_usec*hz200pt)) )+1; 
	if (verbose)
	{	fprintf(stderr,"SGI Audio: Sound Buffersize:%d\n",SGI_DEFAULT_BUFFERSIZE);
		fprintf(stderr,"SGI Audio: Sound Frequency:%d\n",audio_frequency);
		fprintf(stderr,"SGI Audio: Sound Configuration Successful.\n");		}
}

static void SGI_audio_close(void)
{
	while(ALgetfilled(port) != 0) sginap(1); 
  	ALcloseport(port);
	ALfreeconfig(conf);
}

static void SGI_audio_volume(long volume)
{	unsigned long adj_volume;
  	long sgi_params[4];
  	adj_volume = (int)(pow(10.0, (float)volume / 100 * 2.406540183) + 0.5); 
  	if (adj_volume > SGI_MAX_VOL) adj_volume = SGI_MAX_VOL;
 	sgi_params[0] = AL_LEFT_SPEAKER_GAIN;
  	sgi_params[1] = adj_volume;
  	sgi_params[2] = AL_RIGHT_SPEAKER_GAIN;
  	sgi_params[3] = adj_volume;
  	ALsetparams(AL_DEFAULT_DEVICE, sgi_params, 4);
}

static long SGI_audio_closest_freq(long ifreq)
{	static int valid[] = { 8000,11025,16000,22050,32000,44100,0};
  	long i = 0;
  	long ofreq = 8000;
  	long sgi_params[2];
  	while(valid[i])
  	{	if (abs(valid[i] - ifreq) < abs(ofreq - ifreq)) 
			ofreq = valid[i];
    		i++;
  	}
  	sgi_params[0] = AL_OUTPUT_RATE;
  	sgi_params[1] = ofreq;
  	ALsetparams(AL_DEFAULT_DEVICE, sgi_params, 2);
  	ALgetparams(AL_DEFAULT_DEVICE, sgi_params, 2);
  	if (ofreq != sgi_params[1])
		fprintf(stderr,"SGI Audio: freq gotten %ld wasn't wanted %ld\n", sgi_params[1],ofreq);
  	ofreq = sgi_params[1];
  	return(ofreq);
}

#endif

#ifdef STONX_AUDIO_DEC

static void MMS_wave_callback(hWaveOut,wMsg,dwInstance,lParam1,lParam2)
HANDLE hWaveOut;
UINT wMsg;
DWORD dwInstance;
LPARAM lParam1;
LPARAM lParam2;
{
    switch(wMsg)
	{
	  case WOM_OPEN:
		  case WOM_CLOSE: {
	/* Ignore these */
	break;
       	  }
		  case WOM_DONE: {
	LPWAVEHDR lpwh = (LPWAVEHDR)lParam1;
	mms_buffers_outstanding--;
	break;
       	  }
		  default: {
	fprintf(stderr,
	"Unknown MMS waveOut callback messages receieved.\n");
	break;
       	  }
      }
}

static void DEC_audio_open(void)
{	MMRESULT	status;
	LPWAVEOUTCAPS lpCaps;
	LPPCMWAVEFORMAT lpWaveFormat;

  	if( waveOutGetNumDevs() < 1 )
	{	fprintf(stderr,"DEC MMS (Audio): No Multimedia Services compatible\n");
		exit(1);
  	}

	if((lpCaps = (LPWAVEOUTCAPS)mmeAllocMem(sizeof(WAVEOUTCAPS))) == NULL )
	{ 
		fprintf(stderr,"DEC MMS (Audio): Failed to allocate WAVEOUTCAPS struct\n");
		exit(1);
  	}
	status = waveOutGetDevCaps( 0, lpCaps, sizeof(WAVEOUTCAPS));
	if( status != MMSYSERR_NOERROR ) 
	{
		fprintf(stderr,"DEC MMS (Audio): waveOutGetDevCaps failed - status = %d\n", status);
		exit(1);
	}


	if( lpCaps->dwSupport & WAVECAPS_VOLUME )
    	{	mms_device_volume = TRUE;
	} else
   	{ 	mms_device_volume = FALSE;
	}

	mmeFreeMem(lpCaps);

	if((lpWaveFormat = (LPPCMWAVEFORMAT)
			mmeAllocMem(sizeof(PCMWAVEFORMAT))) == NULL )
	{
	    fprintf(stderr,"Failed to allocate PCMWAVEFORMAT struct\n");
	    return;
	}
	lpWaveFormat->wf.nSamplesPerSec = DEC_DEFAULT_FREQ;
	lpWaveFormat->wf.nChannels = 1; 
	lpWaveFormat->wBitsPerSample = 8;
        lpWaveFormat->wf.wFormatTag = WAVE_FORMAT_PCM;

	lpWaveFormat->wf.nBlockAlign = lpWaveFormat->wf.nChannels * ((lpWaveFormat->wBitsPerSample+7)/8);
	lpWaveFormat->wf.nAvgBytesPerSec = lpWaveFormat->wf.nBlockAlign * lpWaveFormat->wf.nSamplesPerSec;

	/* Open the audio device in the appropriate rate/format */

	mms_device_handle = 0;
	status = waveOutOpen( &mms_device_handle, WAVE_MAPPER, (LPWAVEFORMAT)lpWaveFormat, MMS_wave_callback, NULL, CALLBACK_FUNCTION | WAVE_OPEN_SHAREABLE );
	if( status != MMSYSERR_NOERROR ) 
	{	fprintf(stderr,"DEC MMS (Audio): waveOutOpen failed - status = %d\n", status);
		exit(1);
        }

	/* Get & save the device ID for volume control */

	status = waveOutGetID( mms_device_handle, &mms_device_id );
	if( status != MMSYSERR_NOERROR ) 
	{	fprintf(stderr,"DEC MMS (Audio): waveOutGetID failed - status = %d\n", status);
		exit(1);
        }

	mmeFreeMem(lpWaveFormat);

	/* Allocate wave header for use in write */

	if((mms_lpWaveHeader = (LPWAVEHDR) mmeAllocMem(sizeof(WAVEHDR))) == NULL )
	{	fprintf(stderr,"DEC MMS (Audio): Failed to allocate WAVEHDR struct\n");
		exit(1);
        }

	if ( (mms_audio_buffer = (LPSTR) mmeAllocBuffer(DEC_DEFAULT_BUFFERSIZE )) == NULL)
        {	fprintf(stderr,"DEC MMS (Audio): Failed to allocate shared audio buffer\n");
		exit(1);
        }

	if ((audio_buffer =
			(unsigned char *)malloc(MAX_SAMPLES * sizeof(char))) == NULL)
	{
		fprintf(stderr,"DEC MMS (Audio): Failed to allocate audio buffer memory\n");
		exit(1);
	}

	audio_frequency = DEC_DEFAULT_FREQ;
	audio_samplebits = 8;
	FREQ_base = (2000000 / audio_frequency);
	samples_to_generate = (audio_frequency / (1000000.0/(tv_usec*hz200pt)) )+1; 
	if (verbose)
	{	fprintf(stderr,"DEC MMS (Audio): Sound Buffersize:%d\n",DEC_DEFAULT_BUFFERSIZE);
		fprintf(stderr,"DEC MMS (Audio): Sound Frequency:%d\n",audio_frequency);
		fprintf(stderr,"DEC MMS (Audio): Sound Configuration Successful.\n");		}
}

static void DEC_audio_close(void)
{	MMRESULT status;
	MMS_Audio_Off(0);
	status = waveOutReset(mms_device_handle);
	if( status != MMSYSERR_NOERROR ) 
	{	fprintf(stderr,"DEC MMS (Audio): waveOutReset failed - status = %d\n", status);
  	}
  	status = waveOutClose(mms_device_handle);
  	if( status != MMSYSERR_NOERROR ) 
	{	fprintf(stderr,"DEC MMS (Audio): waveOutClose failed - status = %d\n", status);
  	}
	mmeFreeBuffer(mms_audio_buffer);
	mmeFreeMem(mms_lpWaveHeader);
}

static void DEC_audio_volume(long volume)
{	DWORD vol;

	vol = (0xFFFF * volume) / 100; /* convert to 16 bit scale */
	vol = (vol << 16) | vol;	/* duplicate for left & right channels */

	waveOutSetVolume(audio_device_id, vol);

}

static long DEC_audio_closest_freq(long ifreq)
{	static int valid[] = { 8000, 11025, 16000, 22050, 32000, 44100, 48000, 0}; 
	long i = 0;
    	long best = valid[0];

	while(valid[i])
    	{	if (XA_ABS(valid[i] - ifreq) < XA_ABS(best - ifreq)) best = valid[i];
      		i++;
    	}

    	return(best);
}

#endif


/*
 *	Top Level, Audio Open.
 */ 

void audio_open(void) 
{	

	fprintf(stderr, "If STonX hangs shortly after starting up, try the `-noaudio' option!\n");
	
	init_noise();
	init_envelope();
	init_cliptable();

#ifdef STONX_AUDIO_LINUX
	Audio.machine_audio_open = LINUX_audio_open;
	Audio.machine_audio_close = LINUX_audio_close;
	Audio.machine_audio_volume = LINUX_audio_volume;
#endif
#ifdef STONX_AUDIO_HPUX
	Audio.machine_audio_open = HPUX_audio_open;
	Audio.machine_audio_close = HPUX_audio_close;
	Audio.machine_audio_volume = HPUX_audio_volume;
#endif
#ifdef STONX_AUDIO_SPARC
	Audio.machine_audio_open = SPARC_audio_open;
	Audio.machine_audio_close = SPARC_audio_close;
	Audio.machine_audio_volume = SPARC_audio_volume;
#endif

#ifdef STONX_AUDIO_SGI
	Audio.machine_audio_open = SGI_audio_open;
	Audio.machine_audio_close = SGI_audio_close;
	Audio.machine_audio_volume = SGI_audio_volume;
#endif
#ifdef STONX_AUDIO_DEC
	Audio.machine_audio_open = DEC_audio_open;
	Audio.machine_audio_close = DEC_audio_close;
	Audio.machine_audio_volume = DEC_audio_volume;
#endif
#ifdef STONX_AUDIO_NAS
	Audio.machine_audio_open = NAS_audio_open;
	Audio.machine_audio_close = NAS_audio_close;
	Audio.machine_audio_volume = NAS_audio_volume;
#endif

#ifdef STONX_AUDIO_DIRECTSOUND

	Audio.machine_audio_open = DS_audio_open;
	Audio.machine_audio_close = DS_audio_close;
	Audio.machine_audio_volume = DS_audio_volume;
	Audio.machine_audio_open();
#else

	pipe(parent_pipe);

/*	fcntl(parent_pipe[1], F_SETFL,O_NDELAY);*/
	pipe(child_pipe);
	parent_pid = getpid();
#if 0
	child_pid = clone(0,SIGCLD);
#else
	child_pid = fork();
#endif
	switch (child_pid)
	{	case 0: 
			audio_server();	
		case -1:
			fprintf(stderr,"Failed to fork() Audio server\n");
			break;
		default:
			break;
	}
#endif

}


/*
 *	Top Level, Audio Close. 
 */

void audio_close(void)
{	
	Snd.Server_Exit = 1;
	audio_update();
}


#else


void audio_open(void) 
{
}



void audio_close(void)
{	
}



void audio_update(void)
{	
}



#endif /* HAVE_AUDIO */


/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "config.h"
#include "debug.h"
#include "main.h"
#include "cpu.h"
#include "mem.h"
#include "screen.h"
#include "version.h"
#include "audio.h"
#if MIDI
#include "midi.h"
#endif
#include "native.h"
#include "gemdos.h"
#include "fdc.h"
#include "sfp.h"
#include "ikbd.h"
#include "ui.h"
#include "utils.h"
#include "io.h"

char stonxrc[512];
#define STONXRC "stonxrc"
#define EQ(_x,_y) (strcmp(_x,_y)==0)

int gr_mode=MODE_MONO;
int swidth=640, sheight=400;
extern int scr_width, scr_height, scr_planes;
int custom_screensize=0;
int boot_dev=0;
int vblpt=2;	/* interrupts per VBL */
int refresh=2;	/* VBLs per refresh */
int hz200pt=1;	/* interrupts per timer C interrupt */
long tv_usec=10000;
int verbose=1;
int vdi=0;
int vdi_mode=0;
int audio=1;
int midi=1;
int scanlines=200;
int warmboot=0;
int priv_cmap=0;
int redirect_cconws=0;
int fix_screen=0;
int realtime=0;
#if MODEM1
extern char *serial_dev;
#endif
extern char *parallel_dev;
int got_drive=0;
int chunky=0;
int timer_a=0;
int no_cart=0;
int fullscreen=0;
int startinmonitor=0;
char *cartridge_name=NULL;
char *tos_name=NULL;

int tos1;   		/* TRUE when using TOS version 1, FALSE for TOS version 2 */
long tosstart, tosstart1, tosstart2, tosstart4;		/* They point to the beginning of the TOS ROM (and to tosstart-1, -2 and -4) */
long tosend, tosend1, tosend2, tosend4;			/* These variables point to the end of the TOS ROM */
long romstart, romstart1, romstart2, romstart4;		/* Either the beginning of the TOS ROM (TOS 2) or the beginning of the cartridge rom (TOS 1) */
long romend, romend1, romend2, romend4;			/* The end of the ROM */


void process_args (int argc, char *argv[], int is_rc)
{
    int i, u=0;
    for (i=1; i<argc; i++)
    {
	char *x = is_rc ? argv[i] : argv[i] + 1;
	if (EQ(x,"mono")) 
	    gr_mode = MODE_MONO;
	else if (EQ(x,"color")) 
	    gr_mode = MODE_COLOR;
	else if (EQ(x,"colour")) 
	    gr_mode = MODE_COLOR;
	else if (EQ(x,"size"))
	{
	    if (i == argc-1) {
	        if(machine.fullscreen != NULL)
	            error("size needs an argument of the form <width>x<height> or fullscreen\n");
		else
	            error("size needs an argument of the form <width>x<height>\n");
	    }

	    if((strcasecmp(argv[++i], "fullscreen") == 0)&&(machine.fullscreen != NULL)) {

	        /* get screen size from x */
	        machine.fullscreen(&swidth, &sheight);

		if(verbose)
		    printf("fullscreen emulation: %dx%d\n", swidth, sheight);
			  
		priv_cmap=1;
		fullscreen = 1;
		custom_screensize=1;
	    } else {
	        sscanf(argv[i],"%dx%d",&swidth,&sheight);
		if (swidth <= 0 || sheight <= 0)
		    error("invalid argument to -size\n");
		custom_screensize=1;
	    }
	}
	else if (EQ(x,"vbl"))
	{
	    int v;
	    if (i == argc-1 || (v = atoi(argv[++i])) <= 0)
		error("vbl needs a numeric argument > 0\n");
	    else vblpt=v;
	}
	else if (EQ(x, "q"))
	{
	    verbose=0;
	}
	else if (EQ(x,"private"))
	{
	    priv_cmap=1;
	}
	else if (EQ(x,"chunky"))
	{
	    chunky=1;
	}
	else if (EQ(x,"timer-a"))
	{
	    timer_a=1;
	}
	else if (EQ(x,"nocart"))
	{
	    no_cart=1;
	}
	else if (EQ(x, "refresh"))
	{
	    int t;
	    if (i == argc-1 || (t = atoi(argv[++i])) <= 0)
		error("timer-c needs a numeric argument > 0\n");
	    else refresh=t;
	}
	else if (EQ(x,"timer-c"))
	{
	    int t;
	    if (i == argc-1 || (t = atoi(argv[++i])) <= 0)
		error("timer-c needs a numeric argument > 0\n");
	    else hz200pt=t;
	}
	else if (EQ(x,"usec"))
	{
	    long x;
	    if (i == argc-1 || (x = atol(argv[++i])) <= 0)
		error("usec needs a numeric argument > 0\n");
	    else tv_usec = x;
	}
	else if (EQ(x,"vdi"))
	{
	    vdi_mode = vdi = 1;
	}
	else if (EQ(x,"scanlines"))
	{
	    int x;
	    if (i == argc-1 || (x = atoi(argv[++i])) <= 0)
		error("scanlines needs a numeric argument > 0\n");
	    else scanlines = x;
	}
	else if (EQ(x,"boot"))
	{
	    if (i == argc-1 || !isalpha(argv[++i][0]))
		error("boot needs a drive letter as argument (A for example)\n");
	    else
		boot_dev=toupper(argv[i][0])-'A';
	}
	else if (EQ(x,"disk"))
	{
	    char d;
	    char fname[512];	/* use value from limits.h? */
	    if (i == argc-1 || strlen(argv[++i])>=510 || sscanf(argv[i],"%c:%s", &d, fname) < 2)
		error("disk needs an argument of the form <X>:<File> where"
		      " <X> is a drive letter\n");
	    if (!add_drive (d, fname))
		u=1;
	    else
		got_drive=1;
	}
	else if (EQ(x,"fs"))
	{
	    char d;
	    char fname[512];	
	    if (i == argc-1 || strlen(argv[++i])>=510 || sscanf(argv[i],"%c:%s", &d, fname) < 2)
		error("fs needs an argument of the form <X>:<directory> where"
		      " <X> is a drive letter\n");
	    if (!add_gemdos_drive (d, fname)) 
		u=1;
	    else 
		got_drive=1;
	}
#if MODEM1
	else if (EQ(x,"serial"))
	{
	    if (i == argc-1)
		error("serial needs a device as argument (/dev/ttyS1 for example)\n");
	    serial_dev = strdup(argv[++i]);
	}
#endif /* MODEM1 */
	else if (EQ(x,"para"))
	{
	    if (i == argc-1)
		error("para needs a device as argument (/dev/lp0 for example)\n");
	    parallel_dev = strdup(argv[++i]);
	}
	else if (EQ(x,"noaudio"))
	{
	    audio=0;
	}
	else if (EQ(x,"nomidi"))
	{
	    midi=0;
	}
	else if (EQ(x,"realtime"))
	{
	    realtime=1;
	}
	else if (EQ(x,"norealtime"))
	{
	    realtime=0;
	}
	else if (EQ(x,"warmboot"))
	{
	    warmboot=1;
	}
	else if (EQ(x,"cconws"))
	{
	    redirect_cconws=1;
	}
	else if (EQ(x,"monitor"))
	{
	    startinmonitor=1;
	}
	else if (EQ(x,"h") || EQ(x,"?") || EQ(x,"-help"))
	    u=1;
	else if (EQ(x,"cartridge"))
	{
	    if (i == argc-1)
		error("cartridge needs an argument.\n");
	    cartridge_name = strdup(argv[++i]);
	}
	else if (EQ(x,"tos"))
	{
	    if (i == argc-1)
		error("tos needs an argument.\n");
	    tos_name = strdup(argv[++i]);
	}
        else if (EQ(x,"#ifmachine"))
	{
	    if (i == argc-1)
		error("#ifmachine needs an argument.\n");
	    if (!machine.name || !EQ(x,machine.name)) {
		int level;
		for ( level = 1; level > 0 && ++i < argc; ) {
		    if ( !strncmp( argv[i], "#if", 3 ) )
			level++;
		    if ( !strcmp( argv[i], "#fi" ) )
			level--;
		}
	    }
	}
	else if (EQ(x,"#fi"))
	{
	    /* simply do nothing */
	    /* FIXME: We have to count the #ifXXX and see, if there are
	     *        more #fi
	     */
	}
	else 
	{
	    int n = -1;
	    if ( machine.process_arg ) {
		n = machine.process_arg( x, argc - i - 1, argv + i + 1 );
	    }
	    if ( n >= 0 )
		i += n;
	    else {
		fprintf(stderr, "unknown option %s\n", argv[i]);
		u=1;
	    }
	}
	
    }
    if (u)
    {
	if (is_rc)
	    fprintf(stderr,"Error in %s, please fix!\n",stonxrc);
	else
	    fprintf (stderr, "usage: %s [<options>]\n"
		     "<options> = \n"
		     " -q                        Be Quiet\n"
		     " -size <W>x<H>             set screen size to <W>x<H>\n", argv[0]);
	    if(machine.fullscreen != NULL)
	        fprintf (stderr, 
		     " -size fullscreen          use fullscreen emulation (implies private)\n");

	    fprintf (stderr,
		     " -disk <X>:<File>          Load diskfile <File> as Drive <X>:\n"
		     " -fs <X>:<Directory>       Map <Directory> to Drive <X>:\n"
		     " -colo[u]r|-mono           Set `monitor type'\n"
		     " -usec <X>                 Set <X> microsecond interval timer\n"
		     " -vbl <X>                  Set <X> * usec VBL interval\n"
#if DOUBLE_HZ200
		     " -timer-c <X>              Set <X> * usec / 2 Timer C interval\n"
#else
		     " -timer-c <X>              Set <X> * usec Timer C interval\n"
#endif
		     " -refresh <X>              Set <X> * VBL refresh interval\n"
		     " -para <file>              Use <file> as the parallel port device\n"
#if MODEM1
		     " -serial <file>            Use <file> as the serial port device\n"
#endif
#if MONITOR
		     " -monitor                  Start in Monitor\n"
#endif
		     " -noaudio                  Disable Audio driver\n"
		     " -chunky                   Use `chunky' update mode\n"
		     " -warmboot                 Attempt to simulate a warm-boot\n"
		     " -boot <X>                 Boot from drive <X>:\n"
		     " -vdi                      Run with accelerated (but ugly) native VDI\n"
		     " -private                  Use private colormap (speeds -color up)\n"
		     " -cconws                   Redirect GEMDOS Cconws() function to terminal\n"
		     " -cartridge <File>         Load cartridgeimage <File> as ROM-port-cartridge\n"
		     " -tos <File>               Load TOS-image <File>\n");
	if ( machine.show_arghelp )
	    machine.show_arghelp( stderr );
	fprintf( stderr, "Example: %s -disk a:Disk -color -usec 10000 -vbl 10 -refresh 2 -timer-c 1\n"
		 "will use `Disk' for drive A:, boot in color mode, and run Timer C at "
#if DOUBLE_HZ200
		 "200Hz"
#else
		 "100Hz"
#endif
		 ",\nVBLs at "
		 "10Hz"
		 ", and refresh with "
		 "5Hz"
		 ", if your system allows interval timers with\n10 ms timer intervals (see setitimer(2))\n\n", argv[0]);
	exit(0);
    }
}

void process_stonxrc(void)
{	
    char *home;
    if ((home=getenv("HOME")) != NULL)
    {	
	FILE *rc = NULL;
	if ( machine.name ) {
	    strncpy(stonxrc,home,512-7-8);	/* Important: Use strNcpy to prevent buffer overflows! */
	    strcat(stonxrc,"/."STONXRC".");
	    strcat(stonxrc,machine.name);
	    rc = fopen(stonxrc,"r");
	}
	if ( !rc ) {
	    strncpy (stonxrc,home,512-7-10);
	    strcat (stonxrc,"/."STONXRC);
	    if ((rc=fopen(stonxrc,"r")) == NULL)
	    {
		if ( machine.name ) {
		    strcpy(stonxrc, STONXETC"/"STONXRC".");
		    strcat(stonxrc,machine.name);
		    rc = fopen(stonxrc,"r");
		}
		if ( !rc ) {
		    strcpy(stonxrc, STONXETC"/"STONXRC);
		    rc = fopen(stonxrc,"r");
		}
	    }
	}
	if (rc != NULL)
	{	
	    int i,c=1;	
	    char *args[1024];
	    for (i=1;i<1024;i++) args[i] = (char*)malloc(sizeof(char)*256);
	    while (fscanf(rc,"%s",args[c]) != EOF) {
		if (args[c][0] == '#' && 
		    strcmp(args[c],"#ifmachine") && strcmp(args[c],"#fi")) {
		    int ch;
		    while ((ch = fgetc(rc)) != EOF &&
			   ch != '\n')
			;
		    continue;
		}
	    c++;
	    }
	    fclose(rc);
	    fprintf (stderr, "Using configuration file:%s\n", stonxrc);
	    process_args (c, args,1);
	    for (i=1023 ;i>0;i--) free(args[i]);
	}
    }
}

void print_timing(void)
{
#ifdef DOUBLE_HZ200
#define MVAL0 2000000.0
#else
#define MVAL0 1000000.0
#endif
    if (verbose) 
    {	fprintf (stderr, "%ld microseconds interval timer\n", tv_usec);
    fprintf (stderr, "%.0f Hz Timer C, ", (double) MVAL0/(tv_usec*hz200pt));
    fprintf (stderr, "%.0f Hz VBLs and %.0f Hz screen update\n",
	     (double)1000000.0/(vblpt*tv_usec),
	     (double)1000000.0/(vblpt*refresh*tv_usec));
    }
}

int stonx_exit(void)
{
    if (verbose)
	fprintf(stderr, "Leaving STonX cleanly...\n");
    
    kill_hardware();
    
#if MONITOR
    kill_monitor();
#endif

#if JOYSTICK
    kill_joystick();
#endif
      
    machine.screen_close();
    if (parallel_dev != NULL) 
        done_parallel();
    if (audio) 
        audio_close();
    exit_debug();
    return 0;
}



/**
 * This function loads a TOS ROM image.
 */
int load_tos(void)
{
    short tosversion;
    FILE *fh;
    char buf[48];
    long toslength;

    fh = fopen(tos_name, "rb");
    if( fh==NULL )
    {
      fprintf (stderr, "Error: Can not open `%s'!\n", tos_name);
      exit(3);
    }
    if( fread(buf, 1, 48, fh) != 48 )   /* Read ROM header */
    {
      fprintf (stderr, "Error: Can not read `%s'!\n", tos_name);
      exit(3);
    }
    fclose(fh);

    tosversion = LM_UW(&buf[2]);
    tosstart = LM_UL((UL *)&buf[8]);
    tos1 = (tosversion < 0x200);

    if (verbose)
    {
      fprintf(stderr, "TOS version: %d.%c%c\n", (tosversion>>8),
              '0'+((tosversion>>4)&0x0f), '0'+(tosversion&0x0F));
      fprintf(stderr, "TOS ROM address: %lx\n",(long)tosstart);
    }

    if(tosversion > 0x500 || tosversion < 0x100
       || (tosstart != 0xe00000 && tosstart != 0xfc0000))
    {
      fprintf(stderr, "%s seems not to be a valid TOS ROM image!\n", tos_name);
      exit(-1);
    }

    /* Load the TOS ROM image: */
    toslength = load_file (tos_name, (char *)MEM(tosstart)); 

    if((tosstart == 0xfc0000 && toslength > 192*1024)
       || (tosstart == 0xe00000 && toslength > 512*1024))
    {
      fprintf(stderr, "TOS size is too big!\n");
      exit(-1);
    }

    tosstart = TRIM(tosstart);
    tosend = TRIM(tosstart + toslength);

    tosstart1 = tosstart-1;  tosstart2 = tosstart-2;  tosstart4 = tosstart-4;
    tosend1 = tosend-1;      tosend2 = tosend-2;      tosend4 = tosend-4;

    if(tos1)
    {
      romstart = TRIM(CARTSTART);     romstart1 = TRIM(CARTSTART-1);
      romstart2 = TRIM(CARTSTART-2);  romstart4 = TRIM(CARTSTART-4);
      romend = tosend;                romend1 = tosend1;
      romend2 = tosend2;              romend4 = tosend4;
    }
    else
    {
      romstart = tosstart;        romstart1 = tosstart1;
      romstart2 = tosstart2;      romstart4 = tosstart4;
      romend = TRIM(CARTEND);     romend1 = TRIM(CARTEND-1);
      romend2 = TRIM(CARTEND-2);  romend4 = TRIM(CARTEND-4);
    }

    return 0;
}



int main (int argc, char *argv[])
{

#if defined(__NeXT__)
    fprintf(stderr,"NOTE: Co-Xist apparently has problems with virtual timers,"
	    " so I set the\n-realtime flag. Use the option -norealtime"
	    " if this bothers you.\n");
    realtime=1;
#endif
    flags = 0;
    
    printf ("--------------------------------------------------------------------------------\n"
	    "STonX Version " VERSION "\n"
	    "(c)1994-1997 by Marinos Yannikos and Martin D. Griffiths\n"
	    "(c)2001-2004 by the STonX development team - http://stonx.sourceforge.net/\n"
	    "(please read the documentation files for more information).\n"
	    "This program is free software, and comes with NO WARRANTY!\n"
	    "Read the file COPYING for details. If this copy of STonX was not accompanied\n"
	    "by a file called `COPYING', containing the GNU GPL text, report this to the\n"
	    "authors at <stonx-development@lists.sourceforge.net> immediately!\n"
	    "--------------------------------------------------------------------------------\n");
    fflush(stdout);
    process_stonxrc();
    process_args(argc, argv, 0);
    if (!custom_screensize && gr_mode == MODE_COLOR)
    {
	swidth=320;
	sheight=200;
	fix_screen=0;
    }
    if ((gr_mode == MODE_MONO && (swidth != 640 || sheight != 400)) 
	|| (gr_mode == MODE_COLOR && (swidth != 320 || sheight != 200)))
	fix_screen=1;
    if (vdi && custom_screensize)
    {
	int i;
	for (i=0; i<3; i++)
	{
	    scr_def[i].w = swidth;
	    scr_def[i].h = sheight;
	}
    }
    else if (gr_mode == MODE_COLOR)
    {
	scr_def[0].w = swidth;
	scr_def[1].w = scr_def[2].w = swidth*2;
	scr_def[0].h = scr_def[1].h = sheight;
	scr_def[2].h = sheight*2;
    }
    else /* mode == MONO or no custom screen size or no VDI */
    {
	scr_def[0].w = swidth/2;
	scr_def[1].w = scr_def[2].w = swidth;
	scr_def[0].h = scr_def[1].h = sheight/2;
	scr_def[2].h = sheight;
    }
    /* need these for init_mem() */
    if (gr_mode == MODE_COLOR)
    {
	scr_width=scr_def[0].w;
	scr_height=scr_def[0].h;
	scr_planes=4;
    }
    else
    {
	scr_width=scr_def[2].w;
	scr_height=scr_def[2].h;
	scr_planes=1;
	shiftmod = 2;
    }
    print_timing();
#ifndef _WIN32
    if (!got_drive)
    {
	char path[500];
	getcwd(path,500);
	if (verbose) 
	    fprintf(stderr, "No drives specified - using: `-fs C:%s'\n", path);
	add_gemdos_drive ('C', path);
    }
#endif
    init_debug();
    init_ui();
    init_fdc();
    init_sfp();
    init_mem();

    if ( !tos_name )
    {
	char *h;

	if ( ( tos_name = malloc( strlen( STONXDIR ) + 10 ) ) == NULL )
	{
	    perror( "STonX" );
	    return 1;
	}
	strcpy( tos_name, STONXDIR );
	if ( ( h = strchr( tos_name, '\0' ) ) != tos_name &&
	     h[-1] != '/' )
	    *h++ = '/';
	strcpy( h, "tos.img" );
    }	
    load_tos();

    if (!no_cart)
    {
	if ( !cartridge_name )
        {
	    char *h;

	    if ( ( cartridge_name = malloc( strlen( tos_name ) + 13 ) ) == NULL ) 
	    {
		perror( "STonX" );
		return 1;
	    }
	    strcpy( cartridge_name, tos_name );
	    if ( ( h = strrchr( cartridge_name, '/' ) ) == NULL )
		h = cartridge_name;
	    else
		h++;
	    strcpy( h, "cartridge.img" );
	}
	load_file (cartridge_name, (char *)MEM(0xfa0000));
    }

    if (audio)
	audio_open();
    machine.screen_open();
#if MIDI
    if (midi) 
	init_midi();
#endif
#if JOYSTICK
     init_joystick();
#endif
    init_hardware();
    init_ikbd();
#if MONITOR
    init_monitor(startinmonitor);
#endif
    if (parallel_dev != NULL) 
	init_parallel();
#if MODEM1
    if (serial_dev != NULL) 
	init_serial();
#endif
    if (verbose) 
	fprintf(stderr,"Initializing CPU state...\n");
    init_cpu();
    if ( verbose )
	fprintf(stderr,"Starting emulation...\n");
    execute_start(tosstart);
    return stonx_exit();
}

int stonx_shutdown(int mode) {
    switch (mode)
    {
      case 0x00:
	  if ( verbose )
	      fprintf(stderr, "HALT --> terminate STonX\n");
	  stonx_exit();
	  exit(0);
	  break;
	  
      case 0x01:
	  if ( verbose )
	      fprintf(stderr,"WARMBOOT --> ignore\n");
	  break;
	  
      case 0x02:
	  if ( verbose )
	      fprintf(stderr,"COLDBOOT --> ignore\n");
	  break;

      default:
	  if ( verbose )
	      fprintf( stderr, "Unknown shutdown mode %#04x.\n", mode );
	  return EINVAL;
    }
    return 0;
}

/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

/* The functions in this file allow STonX to emulate the TOS BIOS */
/* natively - it is disabled by default in options.h              */

#include "defs.h"

#if NATIVE_BIOS      /* Only include this file if really necessary */

#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gemdos.h"
#include "debug.h"
#include "main.h"
#include "utils.h"
#include "tosdefs.h"
#include "cartridge.h"
#include "xlib_vdi.h"

#define TRACE_BIOS 1

#if TRACE_BIOS
#define BD(_x) debug _x
#else
#define BD(_x)
#endif


static int Getmpb(UL ptr)
{
	BD(("Getmpb()\n"));
	return FALSE;
}

static int Bconstat(UW dev)
{
	if (!PC_IN_ROM(pc))
	{
		BD(("Bconstat(%d) pc=%lx\n", dev, pc));
	}
	return FALSE;
}

static int Bconin(UW dev)
{
	BD(("Bconin(%d)\n", dev));
	return FALSE;
}

static int Bconout(UW dev, W c)
{
	BD(("Bconout(%d,%x)\n", dev, c));
	if ((dev == 2 || dev == 5) && vdi)
	{
		return vdi_output_c(c);
	}
	return FALSE;
}

static int Rwabs(char *args)
{
	BD(("Rwabs(%d,$%lx,%d,%d,%d,%ld)\n", LM_W(args), LM_UL(args+2), LM_W(args+6), LM_UW(args+8), LM_W(args+10), LM_UL(args+12)));
	return disk_rw(args);
}

static int Setexc(UW number, UL vector)
{
	BD(("Setexc(%u,%lx)\n", number, vector));
	return FALSE;
}

static int Tickcal(void)
{
	BD(("Tickcal()\n"));
	return FALSE;
}

static int Getbpb(UW dev)
{
	BD(("Getbpb(%d)\n", dev));
	return disk_bpb(dev);
}

static int Bcostat(UW dev)
{
	BD(("Bcostat(%d)\n", dev));
	return FALSE;
}

static int Mediach(UW dev)
{
	BD(("Mediach(%d)\n", dev));
	DREG(0) = 0;
	return TRUE;
}

static int Drvmap(void)
{
	BD(("Drvmap()\n"));
	return FALSE;
}

static int Kbshift(W mode)
{
	BD(("Kbshift(%x)\n", mode));
	return FALSE;
}


int Bios(void)
{
	W n;
	char *args;
	int bios_done;
	
	args = (char *)MEM(SP);
	n = LM_W(args);
	args += 2;
	switch (n)
	{
	case 0:
		bios_done = Getmpb(LM_UL(args));
		break;
	case 1:
		bios_done = Bconstat(LM_UW(args));
		break;
	case 2:
		bios_done = Bconin(LM_UW(args));
		break;
	case 3:
		bios_done = Bconout(LM_UW(args), LM_UW(args+2));
		break;
	case 4:
		bios_done = Rwabs(args);
		break;
	case 5:
		bios_done = Setexc(LM_UW(args), LM_UL(args+2));
		break;
	case 6:
		bios_done = Tickcal();
		break;
	case 7:
		bios_done = Getbpb(LM_UW(args));
		break;
	case 8:
		bios_done = Bcostat(LM_UW(args));
		break;
	case 9:
		bios_done = Mediach(LM_UW(args));
		break;
	case 10:
		bios_done = Drvmap();
		break;
	case 11:
		bios_done = Kbshift(LM_W(args));
		break;
	default:
		BD(("Bios(%d)\n", n));
		bios_done = FALSE;
		break;
	}
	return bios_done;
}


#endif /* NATIVE_BIOS */

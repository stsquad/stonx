/*
 * Copyright 1999, 2000 by Chris Felsch <C.Felsch@gmx.de>
 *
 * See COPYING for details of legal notes.
 *
 * Modified 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * File : serial_dev.h
 *        serial device driver routines and bios emulation
 */

#ifndef _serial_dev_h_
#define _serial_dev_h_

#include "global.h"

/*
 * default definitions
 */

#define RSVF_PORT	0x80
#define RSVF_GEMDOS	0x40
#define RSVF_BIOS	0x20
#define BDEV_OFFSET	7		/* offset for serial2 */

extern DEVDRV *serial_init(void);       /* init serial device driver */

#endif /* _serial_dev_h_ */

/*
 * Part of STonX
 * Copyright (c) Christian Felsch, 2000
 *
 * see mint_serial.c for details
 *
 */

#ifndef _mint_serial_h_
#define _mint_serial_h_

#include "mint_defs.h"

#define MINT_SER_DEVICE  "/dev/modem"    /* link to current modem */
                                         /* FIXME: should be configurable */
#define MINT_SER_LOCK    "/var/lock"     /* FSSTND 1.2 */

extern UL mint_ser_open     (MINT_FILEPTR *f);
extern UL mint_ser_write    (MINT_FILEPTR *f, const char *buf, L bytes);
extern UL mint_ser_read     (MINT_FILEPTR *f, char *buf, L bytes);
extern UL mint_ser_lseek    (MINT_FILEPTR *f, L where, W whence);
extern UL mint_ser_ioctl    (MINT_FILEPTR *f, W mode, void *buf);
extern UL mint_ser_datime   (MINT_FILEPTR *f, W *timeptr, W rwflag);
extern UL mint_ser_close    (MINT_FILEPTR *f, W pid);
extern UL mint_ser_select   (MINT_FILEPTR *f, L proc, W mode);
extern UL mint_ser_unselect (MINT_FILEPTR *f, L proc, W mode);

#endif /* _mint_serial_h */

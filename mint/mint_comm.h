/*
 * Part of STonX
 * Copyright (c) Christian Felsch, 2000
 *
 * see mint_comm.c for details
 *
 */

#ifndef _mint_comm_h_
#define _mint_comm_h_

#include "../defs.h"

extern UL mint_com_open  (MINT_FILEPTR *f);
extern UL mint_com_write (MINT_FILEPTR *f, const char *buf, L bytes);
extern UL mint_com_read  (MINT_FILEPTR *f, char *buf, L bytes);
extern UL mint_com_ioctl (MINT_FILEPTR *f, W mode, void *buf);
extern UL mint_com_close (MINT_FILEPTR *f, W pid);

#endif /* _mint_comm_h_ */

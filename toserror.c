/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

#include <errno.h>
#include "toserror.h"

int unix2toserrno(int unixerrno,int defaulttoserrno)
{
  int retval = defaulttoserrno;
  switch(unixerrno)
    {
    case EACCES: 
    case EPERM:
    case ENFILE:  retval = TOS_EACCDN;break;
    case EBADF:   retval = TOS_EIHNDL;break;
    case ENOTDIR: retval = TOS_EPTHNF;break;
    case ENOENT: /* GEMDOS most time means it should be a file */
    case ECHILD:  retval = TOS_EFILNF;break;
    case ENXIO:   retval = TOS_EDRIVE;break;
    case EIO:     retval = TOS_EIO;break;    /* -90 I/O error */
    case ENOEXEC: retval = TOS_EPLFMT;break;
    case ENOMEM:  retval = TOS_ENSMEM;break;
    case EFAULT:  retval = TOS_EIMBA;break;
    case EEXIST:  retval = TOS_EEXIST;break; /* -85 file exist, try again later */
    case EXDEV:   retval = TOS_ENSAME;break;
    case ENODEV:  retval = TOS_EUNDEV;break;
    case EINVAL:  retval = TOS_EINVFN;break;
    case EMFILE:  retval = TOS_ENHNDL;break;
    case ENOSPC:  retval = TOS_ENOSPC;break; /* -91 disk full */
    case ENAMETOOLONG: retval = TOS_ERANGE; break;
    }
  return retval;
}

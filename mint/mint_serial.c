/*
 * Filename:    stx_ser.c
 * Version:     0.5
 * Author:      Chris Felsch
 * Started:     2000
 * Target O/S:  MiNT running on StonX (Linux)
 * Description: STonX serial device backend
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 * 
 * History:    18.11.00    started (re-implemented from 0.4)
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#ifdef __TOS__
#error This file has to be part of STonX!
#endif

#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "mint_misc.h"
#include "mint_serial.h"

#include "../mem.h"
#include "../main.h"
#include "../toserror.h"

#include "../options.h"

#ifndef MINT_SERIAL
# define MINT_SERIAL 1
#endif

#if MINT_SERIAL

extern UW time2dos (time_t t);   /* from gemdos.c */
extern UW date2dos (time_t t);   /* from gemdos.c */

#ifdef DEBUG_MINT_SERIAL
# define DBG( _args... ) fprintf( stderr, "mint_serial_" ## _args )
# define DBG_NP( _args... ) fprintf( stderr, ## _args )
#else
# define DBG( _args... )
# define DBG_NP( _args... )
#endif

#undef DBG_CLOSE
#undef DBG_OPEN
#undef DBG_READ
#undef DBG_UNSEL
#undef DBG_WRITE

#define DBG_DAT
#define DBG_IOCTL
#define DBG_SEEK
#define DBG_SEL


/*
 * tools for serial device
*/

/* -- baud translation ------------------------------------------------------ */ 

static int mint_baud[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 
			  2400, 1800, 1200, 600, 300, 200, 150, 134, 110, 
			  75, 50, 0};

static int linux_baud[] = {B230400, B115200, B57600, B38400, B19200, B9600,
			   B4800, B2400, B1800, B1200, B600, B300, B200, 
			   B150, B134, B110, B75, B50, B0};


static int mint_speed2value(int speed)
{
    int    i;
    
    for (i=0; linux_baud[i] != 0; i++)
    {
        if (speed == linux_baud[i])
            return mint_baud[i];
    }
    DBG_NP("speed2value: Unknown baud rate %d\n", speed);
    return 0;
}


static int mint_value2speed(int value)
{
    int    i;
    
    for (i=0; mint_baud[i] != 0; i++)
    {
        if (value == mint_baud[i])
            return linux_baud[i];
    }
    DBG_NP("value2speed: Unknown baud rate %d\n", value);
    return TOS_ERANGE;
}


/* -- show terminal parameters ------------------------------------------- */ 

#ifdef DEBUG_MINT_SERIAL
static void mint_show_serial_parm(int fd)
{
    int            r;
    struct termios tty;
    char           s[80];

    DBG_NP("ser_show_termparm:\n");
    r = tcgetattr(fd, &tty);
    if (r == -1)
    {
        DBG_NP("  tcgetattr() returned %d\n", r);
        return;
    }
    
    DBG_NP("  c_cflag:  %#4x\n", (int)tty.c_cflag);
        
    /* Baudrate */
    DBG_NP("  ispeed:   %d\n", mint_speed2value(cfgetispeed(&tty)));
    DBG_NP("  ospeed:   %d\n", mint_speed2value(cfgetospeed(&tty)));
    
    /* Kommunikation */
    switch (tty.c_cflag & CSIZE)
    {
      case CS5: r = 5; break;
      case CS6: r = 6; break;
      case CS7: r = 7; break;
      case CS8: r = 8; break;
      default: r = 8;
    }
    DBG_NP("  charsize: %d bit\n", r);
    
    strcpy(s, "non");
    if (tty.c_cflag & PARENB)
        strcpy(s, "EVEN");
    if (tty.c_cflag & PARODD)
        strcpy(s, "ODD");
    DBG_NP"  parity:   %s\n", s);

    if (tty.c_cflag & CSTOPB)
        r = 2;
    else
        r = 1;
    DBG_NP("  stopbits: %d\n", r);
    
    strcpy(s, "");
    if (tty.c_cflag & CRTSCTS)
        strcat(s, "RTSCTS ");
    if ((tty.c_iflag & IXON) || (tty.c_oflag & IXON))
        strcat(s, "XONXOFF");    
    DBG_NP("  flowctrl: %s\n", s);
}
#else
# define mint_show_serial_parm(fd)
#endif

/* -- Lock file handling ------------------------------------------------- */ 

static char *mbasename(char *s)
{
    char *p;

    if((p = strrchr(s, '/')) == (char *)NULL)
        p = s;
    else
        p++;
    return (p);
}


static void lock_name(char *lock)
{
    struct stat st;
    int         r;
    char        buf[80];
    
    if ((lstat(MINT_SER_DEVICE, &st) == 0) && S_ISLNK(st.st_mode))
    {
        r = readlink(MINT_SER_DEVICE, buf, sizeof(buf));
        buf[r] = '\0';
    }
    else
        strcpy(buf, MINT_SER_DEVICE);
    sprintf(lock, "%s/LCK..%s", MINT_SER_LOCK, mbasename(buf));
}


static int mint_check_serial_lock(void)
{
    char        lock[80], buf[80];
    struct stat st;
    int         fd, r, pid;

    lock_name(lock);    
    if (stat(lock, &st) == 0)  /* file exists: check if STonX have create it */
    {
        fd = open(lock, O_RDONLY);
        if (fd >= 0)
        {
            r = read(fd, buf, sizeof(buf));
            close(fd);
            if (r > 0) 
            {
                pid = -1;
                if (r == 4) /* Kermit-style lockfile. */
                    pid = *(int *)buf;
                else 
                {                /* Ascii lockfile. */
                    buf[r] = 0;
                    sscanf(buf, "%d", &pid);
                }
                if ((pid > 0) && (pid == getpid()))
                {
                    DBG_NP("ser_check_lock: STonX's lockfile\n");
                    return 0;
                }
                if (pid > 0 && kill((pid_t)pid, 0) < 0 && errno == ESRCH) 
                {
                    DBG_NP("ser_check_lock: Lockfile is stale."
			   " Overriding it...\n");
                    sleep(1);
                    unlink(lock);
                } 
                else
                    r = 0;
            }
            if (r == 0) 
            {
		DBG_NP("ser_check_lock: Device %s is locked by PID %d.\n", 
		       MINT_SER_DEVICE, pid);
		errno = EACCES;
		return -1;
            }
        }    
    }
    return 0;
}


static int mint_set_serial_lock(void)
{
    char    lock[80], buf[80];
    int    fd;
    int    pid;
    int    mask;
    
    lock_name(lock);

    /* Create lockfile compatible with UUCP-1.2 */
    fd = open(lock, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) 
    {
        DBG_NP("ser_set_lock: Cannot create lockfile %s (%d).\n", lock, errno);
        errno = EACCES;
        return -1;
    }
    DBG_NP("ser_set_lock: Creating lockfile %s\n", lock);
    sprintf(buf, "%10ld\n", (long)getpid());
    write(fd, buf, strlen(buf));
    close(fd);
    
    return 0;
}


static void mint_reset_serial_lock(void)
{
    char    lock[80];
    
    lock_name(lock);
    DBG_NP("ser_reset_lock: Removing lockfile %s\n", lock);
    unlink(lock);
}


/*
 * --------- The main functions ---------------------------------- 
 */
UL mint_ser_close(MINT_FILEPTR *f, W pid)
{
    int        fd, r;
    struct termios    tty;
    
    fd = (long)LM_L(&(f->devinfo));
    
#ifdef DBG_CLOSE
    DBG("close\n"
	"  f->devinfo: %ld\n", fd);
#endif

    /* reset the line */
    tcgetattr(fd, &tty);
    tty.c_iflag = (IGNBRK|IGNPAR);
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cflag = (CRTSCTS|HUPCL|CLOCAL|CREAD|CS8);
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);
    tcsetattr(fd, TCSANOW, &tty);
mint_show_serial_parm(fd);

    r = close(fd);

#ifdef DBG_CLOSE
    DBG("close:  fd = %d, errno = %d!\n", fd, errno);
#endif
    
    mint_reset_serial_lock();

    if (r == -1)
        return unix2toserrno(errno, TOS_EIHNDL);

    return 0;
}


UL mint_ser_datime(MINT_FILEPTR *f, W *timeptr, W rwflag)
{
#ifdef DBG_DAT
    DBG("datime\n"
	"  rwflag = %d\n", rwflag);
#endif

    return TOS_EFILNF;
#if 0
    if ( rwflag )
    { /* change access and modification time */
	return 0;
    }
    else
    { /* get access time */
	return 0;
    }
#endif
}


UL mint_ser_ioctl(MINT_FILEPTR *f, W mode, void *buf)
{
    int            fd = LM_L(&(f->devinfo)),
	           rv;
    long           l, rl;
    short          i;
    struct termios tty;
    
    rv = tcgetattr(fd, &tty);
    
    if (rv == -1)
        return unix2toserrno(errno, TOS_EIO);
    
    switch (mode)
    {
      case TOS_TIOCIBAUD:
      case TOS_TIOCOBAUD:
	  l = LM_L(buf);
	  if (l == -1)
	  {
	      if (mode == TOS_TIOCIBAUD)
		  rl = mint_speed2value(cfgetispeed(&tty));
	      else
		  rl = mint_speed2value(cfgetospeed(&tty));
/*
  #ifdef DBG_IOCTL
              DBG("ioctl: get i/o baud: %ld\n", rl);
  #endif
*/
	      SM_L(buf, rl);
	  }
	  else if (l == 0)
	  {
	      /* Drop DTR */
	      cfsetospeed(&tty, B0);
	      cfsetispeed(&tty, B0);
	      tcsetattr(fd, TCSANOW, &tty);
	  }
	  else
	  {
	      rv = mint_value2speed(l);
/*
#ifdef DBG_IOCTL
              DBG("mint_ser_ioctl: set i/o baud: %ld -> %d\n", l, rv);
#endif
*/
	      if (rv == TOS_ERANGE)
	      {
		  int    i;
		  
		  for (i=0; mint_baud[i] != 0; i++)
		  {
		      if (l > mint_baud[i])
		      {
			  SM_L(buf, mint_baud[i]);
			  break;
		      }
		  }
		  return TOS_ERANGE;
	      }
	      
	      if (mode == TOS_TIOCIBAUD)
		  cfsetispeed(&tty, rv);
	      else
		  cfsetospeed(&tty, rv);
	      tcsetattr(fd, TCSANOW, &tty);
	  }
	  break;
	  
      case TOS_TIOCGFLAGS:
	  i = 0;
	  
	  /* Stopbits */
	  if (tty.c_cflag & CSTOPB)
	      i |= 0x0003;    /* 2 */
	  else
	      i |= 0x0001;    /* 1 */
	  
	  /* char size */
	  switch (tty.c_cflag & CSIZE)
	  {
	    case CS5:
		i |= 0xC;
		break;
	    case CS6:
		i |= 0x8;
		break;
	    case CS7:
		i |= 0x6;
		break;
	    case CS8:
		i |= 0x0;
		break;
	    default:
		DBG("ioctl: Unknown CSIZE: %#4x\n", (int)tty.c_cflag);
	  }
	  
	  /* Parity */
	  if (tty.c_cflag & PARENB)
	      i |= 0x4000;
	  if (tty.c_cflag & PARODD)
	      i |= 0x8000;
	  
	  /* flow control */
	  if (tty.c_cflag & CRTSCTS)
	      i |= 0x2000;
	  
	  if ((tty.c_iflag & IXON) || (tty.c_oflag & IXON))
	      i |= 0x1000;
	  
/*
#ifdef DBG_IOCTL
          DBG("mint_ser_ioctl: get flags: mint:  %#4x\n", i);
#endif
*/
	  SM_W(buf, i);
	  break;
	  
      case TOS_TIOCSFLAGS:
	  i = LM_W(buf);
	  
/*
  #ifdef DBG_IOCTL
          DGB("mint_ser_ioctl: set flags: mint:  %#4x\n", i);
          ser_show_termparm(fd);
#endif
*/

	  /* Stopbits */
	  if ((i & 0x0003) == 0x0001)            /* 1 bit */
	      tty.c_cflag &= ~CSTOPB;    
	  else if ((i & 0x0003) == 0x0003)        /* 2 bit */
	      tty.c_cflag |= CSTOPB;
	  else
	      DBG("mint_ser_ioctl: Non supported stopbit cfg: %#4x\n",
		  i&0x0003);
	  
	  /* char size */
	  switch (i & 0x000C)
	  {
	    case 0xC:
		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS5;
		break;
	    case 0x8:
		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS6;
		break;
	    case 0x6:
		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7;
		break;
	    case 0x0:
	    default:
		tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
		break;
	  }
	  
	  /* Parity */
	  tty.c_cflag &= ~(PARENB | PARODD);
	  if (i & 0x4000)
	      tty.c_cflag |= PARENB;
	  if (i & 0x8000)
	      tty.c_cflag |= PARODD;
	  
	  /* flow control */
	  if (i & 0x2000)
	      tty.c_cflag |= CRTSCTS;
	  else
	      tty.c_cflag &= ~CRTSCTS;
	  
	  if (i & 0x1000)
	      tty.c_iflag |= IXON | IXOFF;
	  else
	      tty.c_iflag &= ~(IXON|IXOFF|IXANY);
	  
	  tcsetattr(fd, TCSANOW, &tty);
	  
/**/
#ifdef DBG_IOCTL
	  mint_show_serial_parm(fd);
#endif
/**/
	  break;
	  
      case TOS_FIONREAD:
	  l = ioctl(fd, FIONREAD, &rl);
	  if (l == -1)
	      return unix2toserrno(errno, TOS_EINVFN);
	  SM_L(buf, rl);
	  break;
	  
      case TOS_FIONWRITE:
	  /* FIXME: Do you always can write to tty's under Linux???? */
	  SM_L(buf, 1);
	  break;
	  
      case TOS_TIOCCTLGET:
	  ioctl(fd, TIOCMGET, &l);
	  rl = 0L;
	  if (l & TIOCM_LE)
	      rl |= 0x0001;        /* TOS_TIOCM_LE */
	  if (l & TIOCM_DTR)
	      rl |= 0x0002;        /* TOS_TIOCM_DTR */
	  if (l & TIOCM_RTS)
	      rl |= 0x0004;        /* TOS_TIOCM_RTS */
	  if (l & TIOCM_CTS)
	      rl |= 0x0008;         /* TOS_TIOCM_CTS */
	  if (l & TIOCM_CAR)
	      rl |= 0x0010;        /* TOS_TIOCM_CAR */
	  if (rl & TIOCM_RNG)
	      rl |= 0x0020;        /* TOS_TIOCM_RNG */
	  if (l & TIOCM_DSR)
	      rl |= 0x0040;        /* TOS_TIOCM_DSR */
	  
	  SM_L(buf, rl);
	  break;
	  
/*
      case TOS_TIOCFLUSH:
          tcflush(fd, TCIOFLUSH)M
          break;
*/
            
      default:
#ifdef DBG_IOCTL
	  DBG("mint_ser_ioctl: unknown mode %#4x\n", mode);
#endif
	  return TOS_EINVFN;
    }
    return TOS_E_OK;
}


UL mint_ser_lseek(MINT_FILEPTR *f, L where, W whence)
{
    int    fd = LM_L(&(f->devinfo));
    int    rv;

#ifdef DBG_SEEK
    DBG("lseek\n"
	"  where  = %d\n"
	"  whence = %d\n",
	where,
	whence);
#endif

    rv = lseek(fd, where, whence);
#ifdef DBG_SEEK
    DBG("lseek:  rv = %d, errno = %d\n", rv, errno);
#endif    

    if (rv == -1)
        return unix2toserrno(errno, TOS_EACCDN);
    else
        return rv;
}


UL mint_ser_open(MINT_FILEPTR *f)
{
    int        mode, fd;
    UW        flags = LM_UW(&(f->flags));
    struct termios    tty;
    
#ifdef DBG_OPEN
    DBG("open\n"
	"  mintmode: %#04x\n", flags);
#endif

    mode = mint_m2u_openmode(flags);
    mode |= O_NDELAY;
    
#ifdef DBG_OPEN
    DBG("open\n"
	"  unixmode: %#04x\n", mode);
#endif

    if (mint_check_serial_lock() == -1)
        return unix2toserrno(errno, TOS_EACCDN);
    
    fd = open("/dev/modem", mode);
#ifdef DBG_OPEN
    DBG("open\n"
	"  fd= %d, errno: %d\n", fd, errno);
#endif

    if (fd == -1)
        return unix2toserrno(errno, TOS_ERROR);
    
    mint_set_serial_lock();
    
    /* initialize the line */
    tcgetattr(fd, &tty);
    tty.c_iflag = (IGNBRK|IGNPAR);
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cflag = (CRTSCTS|HUPCL|CLOCAL|CREAD|CS8);
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);
    tcsetattr(fd, TCSANOW, &tty);

    SM_L(&(f->devinfo), fd);

#ifdef DBG_OPEN
    DBG("open\n"
	"  f->devinfo = %ld\n", (int)LM_L(&(f->devinfo)));
    mint_show_serial_parm(fd);
#endif

    return 0;
}


UL mint_ser_read( MINT_FILEPTR *f, char *buf, L bytes )
{
    int    fd = LM_L(&(f->devinfo));
    int    rv;

#ifdef DBG_READ
    DBG("read\n"
	"  bytes: %ld\n", bytes);
#endif

    rv = read(fd, buf, bytes);
#ifdef DBG_READ
    DBG("read\n"
	"  read(...) = %d\n", rv);
#endif

    if (rv == -1)
        return unix2toserrno( errno, TOS_EACCDN );

    return rv;
}


UL mint_ser_select( MINT_FILEPTR *f, L proc, W mode )
{
    int    fd = LM_L(&(f->devinfo));
    
#ifdef DBG_SEL
    DBG("select\n" 
	"  mode: %d\n", mode);
#endif

    return 0;
}


UL mint_ser_unselect( MINT_FILEPTR *f, L proc, W mode )
{
    if ( verbose )
	fprintf( stderr, "unselect not yes supported at MINT_SERIAL\n" );

    return 0;
}


UL mint_ser_write( MINT_FILEPTR *f, const char *buf, L bytes )
{
    int    fd = LM_L(&(f->devinfo));
    int    rv;

#ifdef DBG_WRITE
    DBG("write\n"
	"  bytes: %ld\n", bytes);
#endif

    rv = write(fd, buf, bytes);
#ifdef DBG_WRITE
    DBG("write\n"
	"  write(...) = %d\n", rv);
#endif
      
    if (rv == -1)
        return unix2toserrno( errno, TOS_EACCDN );

    return rv;
}

#endif /* MINT_SERIAL */

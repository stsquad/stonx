/*
 * Part of STonX
 *
 * STonX creates a cookie. If necessary it creates a new cookiejar at
 * cartridge ROM memory space.
 * Cookie is created at first BIOS trap call.
 *
 * Name of the cookie is "STon" (see defs.h).
 * Value of the cookie is the address of a structure at cartridge ROM
 * memory space:
 *
 * Offset Length  Meaning
 *   +0    ULONG  STonX-Magic ("STon" see defs.h)
 *   +4    ULONG  Length of the structure (28)
 *   +8    UWORD  Major version of STonX (e.g. 0)
 *  +10    UWORD  Minor version of STonX (e.g. 6)
 *  +12    UWORD  Major version of STonX patch (e.g. 7)
 *  +14    UWORD  Minor version of STonX patch (e.g. 1)
 *  +16  CHAR(8)  First 8 chars of screen-type (e.g. "X", "VGA")
 *  +24    ULONG  MiNT flags
 *  +28    ULONG  address of function: LONG _cdecl shutdown(W mode)
 *
 * The meaning of the MiNT flags is:
 *  Bit0  stonxfs is compiled into STonX and may be used
 *  Bit1  bypass at stonxfs is compiled into STonX
 *        (You do not need to know this.)
 *  Bit2  serial MiNT device is compiled into STonX
 *  Bit3  simple MODEM1 emulation is compiled into STonX
 *  Bit4  communication MiNT device is compiled into STonX
 *  Bit5  STonX uses native build in vdi
 */

#ifndef _cookie_h_
#define _cookie_h_

void init_cookie(void);

#endif /* _cookie_h_ */

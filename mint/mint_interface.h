/*
 * Filename:    mint_interface.h
 * Version:     0.5
 * Author:      Markus Kohm, Chris Felsch
 * Started:     1998
 * Target O/S:  MiNT running on StonX (Linux)
 * Description: Interface definition STonX to MiNT-XFS
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 1998, 1999, 2001 Markus Kohm
 *              Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
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
#ifndef _mint_interface_h_
#define _mint_interface_h_

#ifdef atarist
#error This file is for STonX part
#endif


/*****************************************************************
 * from STonX main part used functions
 *****************************************************************/
/*
 * called by STonX's call_native 
 * --> stack: the argument address at 68000th stack 
 *     func:  MiNT-STonX subfunction number:
 *              0       used by initialization to move infos to native part
 *              1-0x1D  root, ..., sync
 */
extern void mint_call_native(UL stack, UL func);

#endif /* _mint_interface_h_ */

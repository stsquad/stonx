/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include <stdio.h>
main(foo)
{
	int x;
	if (sizeof(short) != 2 || sizeof(int) != 4) return 1;
	x = 0x12345678;
	switch (*(char *)&x)
	{
	case 0x12:	puts ("big"); break;
	case 0x78:	puts ("little"); break;
	default:	puts ("dunno"); break;
	}
	return 0;
}

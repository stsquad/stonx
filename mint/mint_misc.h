/*
 * Part of STonX
 */

#ifndef _mint_misc_h_
#define _mint_misc_h_

#include "mint_defs.h"

#include <stdio.h>

extern time_t mint_m2u_time(UW tostime, UW tosdate);

extern UW mint_u2m_mode(mode_t m);
extern mode_t mint_m2u_openmode(UW m);
extern W  mint_u2m_attrib(mode_t m , const char *filename);

extern char *mint_makefilename(MINT_FSFILE *fs, const char *name, char *buf);
extern const char *mint_maketosname(const char *filename, off_t dirpos);
extern char *mint_searchfortosname(const char *dirname, const char *filename);
extern char *mint_maketosfilename(MINT_FSFILE *fs, const char *name, 
				  char *buf);

#endif /* _mint_misc_h */

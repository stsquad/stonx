#ifndef FONTHDR_H
#define FONTHDR_H
#include "defs.h"

typedef struct FONT_HDR
{
	W font_id;
	W point;
	char name[32];
	UW first_ade;
	UW last_ade;
	UW top;
	UW ascent;
	UW half;
	UW descent;
	UW bottom;
	UW max_char_width;
	UW max_cell_width;
	UW left_offset;
	UW right_offset;
	UW thicken;
	UW ul_size;
	UW lighten;
	UW skew;
	UW flags;
	UL hor_table;
	UL off_table;
	UL dat_table;
	UW form_width;
	UW form_height;
	UL next_font;
} FONT_HDR;

#endif /* FONTHDR_H */

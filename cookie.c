/*
 * Part of STonX
 */

#include <string.h>
#include <stdio.h>

#include "mem.h"
#include "version.h"
#include "screen.h"
#include "main.h"
#include "cartridge/cartridge.h"


static int init_done = 0;

void init_cookie(void) {
    char name[5];
    UL cookieadr = LM_UL(MEM(0x5A0));
    UL cookiename = 0, cookievalue = 0;
    int cnt = 0;
    name[4] = '\0';
    init_done = 1;

    if ( cookieadr ) {
	/* Search for the last cookie */
	do {
	    cookiename = LM_UL(MEM(cookieadr));
	    name[0] = LM_B(MEM(cookieadr++));
	    name[1] = LM_B(MEM(cookieadr++));
	    name[2] = LM_B(MEM(cookieadr++));
	    name[3] = LM_B(MEM(cookieadr++));
	    cookievalue = LM_UL(MEM(cookieadr));
	    cookieadr += 4;
	    cnt++;
	} while ( cookiename && strcmp( name, STONXCOOKIEMAGIC ) );
	if ( !cookiename ) {
	    UL cookie;
	    UL flags;

	    /* STonX not already at cookiejar */
	    if ( cnt >= cookievalue ) {
		/* no more space left at cookiejar, we must copy it */
		if ( cnt >= STONXCOOKIEJAR_SIZE / 4 ) {
		    fprintf( stderr, "Cannot set cookie!\n" );
		    return;
		} else {
		    int n;
		    UL srcadr;
		    for ( srcadr = LM_UL(MEM(0x5A0)), 
			      cookieadr = STONXCOOKIEJAR, 
			      n = 0;
			  n < cnt-1;
			  n++, cookieadr += 8, srcadr += 8 ) {
			SM_UL( MEM(cookieadr), LM_UL(MEM(srcadr)) );
			SM_UL( MEM(cookieadr + 4), LM_UL(MEM(srcadr+4)) );
		    }
		    cnt = n + 1; /* The new cookiejar is at ROM, so it may be
				  * write protected! Hope next TSR copies the
				  * cookiejar to a RAM address. */
		    SM_UL( MEM(cookieadr), 0 );
		    SM_UL( MEM(cookieadr + 4), cnt );
		    SM_UL( MEM(0x5A0), STONXCOOKIEJAR );
		}
	    } else {
		cookieadr -= 8;
	    }

	    /* Fill STonX cookie and add it to the cookiejar.
	     * Note: cookieadr is the valid 0 cookie
	     */
	    cookie = STONXCOOKIEADDR;
	    strncpy( MEM(cookie), STONXCOOKIEMAGIC, 4 );
	    SM_UL( MEM(cookie + 4), 32 ); /* size of cookie */
	    SM_UW( MEM(cookie + 8), STONX_VERSION_MAJOR ); /* 0. */
	    SM_UW( MEM(cookie + 10), STONX_VERSION_MINOR ); /* 6. */
	    SM_UW( MEM(cookie + 12), STONX_PATCH_MAJOR ); /* 7. */
	    SM_UW( MEM(cookie + 14), STONX_PATCH_MINOR ); /* 0 */
	    strncpy( MEM(cookie + 16), machine.name, 8);
	    flags = 0x00000000;
#if MINT_STONXFS
	    flags |= 0x00000001;
#if MINT_USE_BYPASS
	    flags |= 0x00000002;
#endif
#endif
#if MINT_SERIAL
	    flags |= 0x00000004;
#endif
#if MODEM1
	    flags |= 0x00000008;
#endif
#if MINT_COM
	    flags |= 0x00000010;
#endif
	    if ( vdi )
		flags |= 0x00000020;
	    SM_UL( MEM(cookie + 24), flags );
	    SM_UL( MEM(cookie + 28), CART_SHUTDOWN );
	    /* We do this very correct (at STonX we could do it more easy) */
	    SM_UL( MEM(cookieadr + 8), 0 );
	    SM_UL( MEM(cookieadr + 12), cnt );
	    SM_UL( MEM(cookieadr + 4), 0 );
	    SM_B( MEM(cookieadr++), STONXCOOKIEMAGIC[0] );
	    SM_B( MEM(cookieadr++), STONXCOOKIEMAGIC[1] );
	    SM_B( MEM(cookieadr++), STONXCOOKIEMAGIC[2] );
	    SM_B( MEM(cookieadr++), STONXCOOKIEMAGIC[3] );
	    SM_UL( MEM(cookieadr), STONXCOOKIEADDR );
	}
    }
}

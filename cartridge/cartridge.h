/* The fix addresses in the STonX cartridge are defined here. */
/* - This should become more flexible one day, I hope.... -   */

#define CART_OLD_GEMDOS  0xfa0400
#define CART_NEW_GEMDOS  (CART_OLD_GEMDOS+4)
#define CART_SHUTDOWN    0xfa0f00
#define CART_RESVEC      0xfa1400
#define CART_FAKE_BPB    0xfa4000


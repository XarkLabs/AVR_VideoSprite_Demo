// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#include <avr/pgmspace.h>	// avr-libc PROGMEM support
#if 0
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif
#endif
#define PROGMEM2 __attribute__((section(".progmem.data1"))) // used for aligned PROGMEM data (saves flash by grouping after unaligned data)

#define TV_MODE			NTSC			// select TV type: 0 = NTSC (60Hz, ~200 visible lines), 1 = PAL (50Hz, ~256 visible lines)

#define DOUBLE_LINES		0			// 0 = normal, 1 = double each scan line

#define FONT_VHEIGHT		8			// height of font data
#define	CHAR_VHEIGHT		(8<<DOUBLE_LINES)	// height of text character on screen (before DOUBLE_LINES), lines > FONT_VHEIGHT will be blank
#define FONT_CHARS		128
#define	NUM_RAMTILES		128			// number of RAM tiles (8 bytes each, 0 for none up to 128)

#define	V_CHARS			22			// number of text lines vertically
#define	H_CHARS			22
#define	SCREEN_HEIGHT		(1+(V_CHARS-V_SCROLL)*CHAR_VHEIGHT)	// add extra blank line

#define H_POS_ORIGIN		-24			// (int) default horizontal screen position offset
//#define H_POS_ORIGIN		-16			// (int) default horizontal screen position offset
#define	V_POS_ORIGIN		0			// (int) default vertical screen position offset

#define	H_POS_OFFSET		0			// (0/1) enable optional dynamic "h_pos_offset" control
#define V_POS_OFFSET		0			// (0/1) enable optional dynamic "v_pos_offset" control

#define H_SCROLL		1			// (0/1) enable optional horizontal scroll "h_fine_scroll" control
#define V_SCROLL		1			// (0/1) enable optional vertical scroll "v_fine_scroll" control

#define MODE_HANDLER		mode_32cyc_tilex8_ramtiles

#define VIDEO_LITTLE_ENDIAN	0

#define	DEBUG_SERIAL_BAUD	115200L				// baud rate for hardware serial debug output (0 to disable)

#define FONT_NAME	OSI_font8x8				// name of default character font to use (defined in separate .cpp file)

extern "C" const uint8_t OSI_font8x8 [] PROGMEM; 		// declare external character font/tilemap in program memory
extern "C" const uint8_t	hex_font8x8 [] PROGMEM; 		// declare external character font/tilemap in 42program memory
extern "C" const uint8_t	Logo_font8x8 [] PROGMEM2; 		// declare external character font/tilemap in program memory
extern "C" const uint8_t	LogoTiles[] PROGMEM;

extern "C" const uint8_t	HappyFace8x8[8][2*8] PROGMEM;

extern "C" const uint8_t	HappyFace8x8[8][2*8] PROGMEM;
extern "C" const uint8_t	HappyFace8x8_mask[8][2*8] PROGMEM;

//
// TVoutGameKit - "tech-demo" game
//

// Define video settings *before* #include <AVRInvaders_video.h>
#include "AVRInvaders_video_settings.h"

#define DEBUG_LED_PIN		12				// LED for debugging (-1 for none)

#include "FastPinIO.h"

// define custom line end handler to reset scroll parameters and insert a blank line for the "score" lines
extern "C" uint16_t	scan_line;				// current video scan-line
extern "C" void 	(*line_handler)();		// function to handle current scan-line generation
extern "C" void 	(*save_line_handler)();	// saved scan-line generation function
extern PROGMEM const int8_t SinTable[] __attribute__ ((aligned(256)));
extern PROGMEM const int8_t TriTable[] __attribute__ ((aligned(256)));
extern PROGMEM const int8_t NoiseTable[] __attribute__ ((aligned(256)));
extern "C" uint8_t	vol0;
extern "C" uint8_t	wave0;
extern "C" uint16_t	pos0;
extern "C" uint16_t	freq0;
extern "C" uint8_t	vol1;
extern "C" uint8_t	wave1;
extern "C" uint16_t	pos1;
extern "C" uint16_t	freq1;
extern "C" uint8_t	vol2;
extern "C" uint8_t	wave2;
extern "C" uint16_t	pos2;
extern "C" uint16_t	freq2;

uint8_t		vol0 __attribute__((used));
uint8_t		wave0  __attribute__((used)) = ((uint16_t)&SinTable[0])>>8;
uint16_t	pos0 __attribute__((used));
uint16_t	freq0  __attribute__((used));
uint8_t		vol1  __attribute__((used));
uint8_t		wave1  __attribute__((used))= ((uint16_t)&SinTable[0])>>8;
uint16_t	pos1 __attribute__((used));
uint16_t	freq1  __attribute__((used));
uint8_t		vol2  __attribute__((used));
uint8_t		wave2  __attribute__((used)) = ((uint16_t)&TriTable[0])>>8;
uint16_t	pos2 __attribute__((used));
uint16_t	freq2  __attribute__((used));
//void mixaudio(void) __attribute__ ((noinline));
#define LINE_START_HANDLER()	mixaudio()

static inline void mixaudio(void)
{
#if 1
	uint8_t b;
	__asm__ __volatile__
	(
		"		lds	r30,pos0+1\n"		// XL = pos0>>8
		"		lds	r31,pos0\n"
		"		lds	r18,freq0\n"
		"		lds	r19,freq0+1\n"
		"		add	r31,r18\n"
		"		adc	r19,r30\n"
		"		sts	pos0,r31\n"
		"		sts	pos0+1,r19\n"
		"		lds	r31,wave0\n"		// XH = wave address (high, 256 byte aligned)
		"		lpm	r18,Z\n"			// XL = wave byte (signed 8-bits)
		"		lds	r19,vol0\n"			// XH = volume
		"		mulsu r18,r19\n"		// apply volume
		"		mov	%[b],r1\n"			// keep high of result
		
		"		lds	r30,pos1+1\n"		// XL = pos0>>8
		"		lds	r31,pos1\n"
		"		lds	r18,freq1\n"
		"		lds	r19,freq1+1\n"
		"		add	r31,r18\n"
		"		adc	r19,r30\n"
		"		sts	pos1,r31\n"
		"		sts	pos1+1,r19\n"
		"		lds	r31,wave1\n"		// XH = wave address (high, 256 byte aligned)
		"		lpm	r18,Z\n"			// XL = wave byte (signed 8-bits)
		"		lds	r19,vol1\n"			// XH = volume
		"		mulsu r18,r19\n"		// apply volume
		"		add	%[b],r1\n"
		"		lds	r30,pos2+1\n"		// XL = pos0>>8
		"		lds	r31,pos2\n"
		"		lds	r18,freq2\n"
		"		lds	r19,freq2+1\n"
		"		add	r31,r18\n"
		"		adc	r19,r30\n"
		"		sts	pos2,r31\n"
		"		sts	pos2+1,r19\n"
		"		lds	r31,wave2\n"		// XH = wave address (high, 256 byte aligned)
		"		lpm	r18,Z\n"			// XL = wave byte (signed 8-bits)
		"		lds	r19,vol2\n"			// XH = volume
		"		mulsu r18,r19\n"		// apply volume
		"		add	%[b],r1\n"
		"		eor	r1,r1\n"
		"		subi %[b],-128\n"
		
		: [b] "=r" (b)					// outputs
		:								// inputs (none)
		: "r18", "r19", "r20", "r30", "r31" // registers clobbered
	);
	OCR2A = b;
#else
	int8_t s0 = ((int8_t)pgm_read_byte((wave0<<8)+(uint8_t)(pos0>>8))*(int8_t)vol0)>>8;
	int8_t s1 = ((int8_t)pgm_read_byte((wave1<<8)+(uint8_t)(pos1>>8))*(int8_t)vol1)>>8;
	OCR2A = (s0 + s1) + 128;
	pos0 += freq0;
	pos1 += freq1;
#endif
}

// This will be executed after each scanline
#define LINE_END_HANDLER()	\
	do												\
	{												\
		if (scan_line == END_RENDER-17)	/* 2*8+blank before bottom */				\
		{											\
			tile_line=0;			/* reset V scroll */				\
			h_fine_scroll = 0x00;		/* reset H scroll */				\
			h_fine_scroll_mask= 0xff;	/* reset H scroll mask */			\
			save_line_handler = line_handler; /* save handler */				\
			line_handler = blank_line;							\
			screen_ram_ptr += H_CHARS;							\
		}											\
	} while (0)

static void blank_line()
{
	scan_line++;
	line_handler = save_line_handler;
}

#include "AVRInvaders.h"	// video display

// CAUTION: It is important not to define any variables or data above this point (which is why "extern" is used above)
//          This is because the SRAM character set address must on a 256 byte boundary.  If it is the very first thing
//	    defined (as it is in the library header) then it will be located at exactly 0x100 (wasting no space).
//	    If it is not the first thing, it will waste up to 255 bytes of the precious Arduino SRAM to align itself.
//	    A print will issue on serial debug

// Uses same hardware setup as TVOut library (see http://code.google.com/p/arduino-tvout)
//
//		    (AVR pin name)			  (Arduino pin name)
// MCU		SYNC	VIDEO	AUDIO	Arduino		SYNC	VIDEO	AUDIO
//------------- ------- ------- ------- ---------------	-------	-------	-------
// m168,m328	B1	D7	B3	NG,Decimila,UNO	9	7	11
// m1280,m2560	B5	A7	B4	Mega		11	A7(D29)	10
// m644,m1284p	D5	A7	D7	Sanguino	13	A7(D24)	8
// ATmega32U4	B5	D7	C6	Leonardo	9	6	5 (NEW!)
// AT90USB1286	B5	F7	B4	Teensy++2.0	25	45	24
//
// SYNC is on OCR1A and AUDIO is on OC2A (except on ATmega32U4 where it is OC3A...I think)
// NOTE: There are some timing issues with the m1284p, may be related to sanguino core.

#include "hardware_setup.h"		// Hardware details (only needed for informational purposes)

#define VIDEO_FPS		(TV_MODE == NTSC ? 60 : 50)

extern "C" char __data_start[];
extern "C" char _end[];		// used to check amount of SRAM this program's variables uses
extern "C" char __data_load_end[];	// used to check amount of Flash this program and static data uses 
extern "C" char __stack[];

extern "C"
{
	void (*save_line_handler)();	// function to handle current scan-line generation
	char *cursor;			// general purpose "screen cursor"
}

//
// Support functions for debug serial output
//
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
#define SerialPrint(str)	serial_print_P(PSTR(str))	// macro to print string in PROGMEM
#else
#define SerialPrint(str)	(void)0
#endif

#if 1
uint8_t dump;
#define	DPRINT(str, val)	do { if (dump) debug_serial_print(PSTR(str), val); } while(0)
#define	DPRINTLN(str, val)	do { if (dump) debug_serial_println(PSTR(str), val); } while(0)
#else
#define	DPRINT(str, val)	(void)0
#define	DPRINTLN(str, val)	(void)0
#endif

static void serial_putc(uint8_t c)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
#if defined ( UDR0 )
	while (!((UCSR0A) & _BV(UDRE0)))
		;
	UDR0 = c;
#elif defined ( UDR )
	while (!((UCSRA) & _BV(UDRE)))
		;
	UDR = c;
#endif
#endif
}

static void serial_print_P(const char *str)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	char c;
	while ((c = pgm_read_byte(str++)) != 0)
	{
		serial_putc(c);
	}
#endif
}

static void serial_print_hex_u8(uint8_t v)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	char c = '0' + (v >> 4);
	c += c > '9' ? ('A'-':') : 0;
	serial_putc(c);
	c = '0' + (v & 0xf);
	c += c > '9' ? ('A'-':') : 0;
	serial_putc(c);
#endif
}

static void serial_print_hex_u16(uint16_t v)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	serial_print_hex_u8(v >> 8);
	serial_print_hex_u8((uint8_t)v);
#endif
}

static void serial_print_hex_u32(uint32_t v)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	serial_print_hex_u16(v >> 16);
	serial_print_hex_u16((uint16_t)v);
#endif
}

static void debug_serial_print(const char *str, uint16_t val)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	serial_print_P(str);
	serial_print_hex_u16(val);
#endif
}

static void debug_serial_println(const char *str, uint16_t val)
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	debug_serial_print(str, val);
	serial_putc('\n');
#endif
}

static void serial_init()
{
#if defined(DEBUG_SERIAL_BAUD) && (DEBUG_SERIAL_BAUD > 0)
	uint16_t baud_setting = (F_CPU / 8 / (DEBUG_SERIAL_BAUD) - 1) / 2;

#if defined ( UDR0 )
	UCSR0A = 0;
#elif defined ( UDR )
	UCSRA = 0;
#endif

#if defined ( UDR0 )
	UBRR0 = baud_setting;
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
#elif defined ( UDR )
	UBRR = baud_setting;
	UCSRB = _BV(RXEN) | _BV(TXEN);
#endif
#endif
	(void)debug_serial_println;
	(void)debug_serial_print;
	(void)serial_putc;
	(void)serial_print_P;
	(void)serial_print_hex_u32;
	(void)serial_print_hex_u16;
	(void)serial_print_hex_u8;
}

//
// Support functions for printing to screen
//
#define POS(x, y)	(((char *)ScreenMem) + (y * H_CHARS) + x)
#define Print(pstr)	print_P(PSTR(pstr))	// macro to print string in PROGMEM


static inline void CursorPos(uint8_t x, uint8_t y)
{
	cursor = POS(x, y);
}

static void printword(uint16_t v)
{
	char *p = cursor+4;
	cursor = p;
	for (uint8_t l = 0; l < 4; l++)
	{
		char c = '0' + (v & 0xf);
		c += (c > '9') ? ('A'-':') : 0;
		v >>= 4;
		*(--p) = c;
	}
}

static void printbyte(uint8_t v)
{
	char *p = cursor+ 2;
	cursor = p;
	for (uint8_t l = 0; l < 2; l++)
	{
		char c = '0' + (v & 0xf);
		c += (c > '9') ? ('A'-':') : 0;
		v >>= 4;
		*(--p) = c;
	}
}

static void printdec(uint32_t v)
{
	uint32_t div = 100000L;
	
	for (uint8_t d = 0; d < 6; d++)
	{
		uint8_t r = (v / div);
		v = v - (r * div);
		div = div / 10;
		*cursor++ = '0' + r;
	}
}

static void print_P(const char *str)
{
	char *p = cursor;
	char c;
	while ((c = pgm_read_byte(str++)) != 0)
	{
		*p++ = c;
	}
	cursor = p;
}

static void clearto(char *end, char clearchar = ' ')
{
	char *p = cursor;
	while ((uint16_t)p <= (uint16_t)end)
		*p++ = clearchar;
	cursor = p;
}

#if 0
//
// Support LFSR functions for fast (but poor) pseudo random number generation
//
uint32_t lfsr32_v = 0x00000001U;
static uint32_t lfsr32()
{
	/* taps: 32 31 29 1; feedback polynomial: x^32 + x^31 + x^29 + x + 1 */
	lfsr32_v = (lfsr32_v >> 1) ^ (-(lfsr32_v & 1u) & 0xD0000001u);
	
	return lfsr32_v;
}

uint16_t lfsr16_v = 0xace1U;
static uint16_t lfsr16()
{
	/* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
	lfsr16_v = (lfsr16_v >> 1) ^ (-(lfsr16_v & 1u) & 0xB400u);   
  	return lfsr16_v;
}
#endif

//============================================================================
// Pseudo-sprite support routines (using dynamically defined SRAM characters) 
//============================================================================

// dynamically changing sprite info (in SRAM)
struct sprite_ram
{
	uint8_t		def;
	uint8_t		x, y;			// x, y pixel position of upper left corner of sprite
};

// unchanging sprite info (in flash ROM)
struct sprite_rom
{
	int8_t		w, h;
	const uint8_t	*image[8];
};

#include "test_sprites.h"

#define BLANK_TILE	0

#define MAX_SPRITES	8
#define MAX_SAVE_TILES	96

#define CLIP_LEFT	((0)*8)
#define CLIP_RIGHT	((H_CHARS-1)*8)
#define CLIP_TOP	((0)*8)
#define CLIP_BOTTOM	(((V_CHARS-3)*8)+v_fine_scroll)

sprite_ram	sprite_info[MAX_SPRITES];
char		save_tiles[MAX_SAVE_TILES];

uint8_t next_sram_tile;

static void draw_sprites()
{
	// draw screen
	for (int8_t s = 0; s < MAX_SPRITES; s++)
	{
		sprite_ram *sinfo = &sprite_info[s];
		uint8_t def = sinfo->def-1;
		// skip zero or invalid sprite
		if (def >= SPRITE_NUM_IMAGES)
		{
			continue;
		}

		const sprite_rom *sdef = &sprite_def[def];

		// X calculations
		uint8_t x = sinfo->x;
		uint8_t w = pgm_read_byte(&sdef->w);
		uint8_t oh = pgm_read_byte(&sdef->h);				// original height
		const uint8_t *sdat = pgm_read_word(&sdef->image[x & 0x7]);	// select pre-shifted sprite data
		
		// if X off right or left edge 
		uint8_t xw = x + w;
		if (
#if CLIP_LEFT > 0	// avoid warning if CLIP_LEFT 0
		x < CLIP_LEFT ||
#endif
		x >= CLIP_RIGHT)
		{
			// check if X+width wraps around to be visible on left edge
			if (
#if CLIP_LEFT > 0	// avoid warning if CLIP_LEFT 0
			xw >= CLIP_LEFT &&
#endif
			xw < CLIP_RIGHT)
			{
				uint8_t cw = xw - CLIP_LEFT;
				uint8_t skip = ((w - cw) + 7)>>3;
				x = CLIP_LEFT;
				w = cw;
				sdat += (oh * 2) * skip;
			}
			else
			{
				continue;			// totally clipped
			}
		}
		else if (xw >= CLIP_RIGHT)
		{
			w = CLIP_RIGHT - x;
		}
		
		uint8_t tx = x >> 3;				// tile X pos (left edge of sprite)
		uint8_t sx = x & 0x7;				// sub-tile X pixel (0-7)
		uint8_t tw = (((uint16_t)w + sx + 7) >> 3);	// width in tiles

		// Y calculations
		uint8_t y = sinfo->y + v_fine_scroll;		// desired Y position
		uint8_t h = oh;					// sprite height (starts same as original height)

		uint8_t yh = y + h;
		if (
#if CLIP_TOP > 0	// avoid warning if CLIP_TOP 0
		y < CLIP_TOP || 
#endif
		y >= CLIP_BOTTOM)
		{
			// check if X+width is visible
			if (
#if CLIP_TOP > 0	// avoid warning if CLIP_TOP 0
			yh >= CLIP_TOP &&
#endif
			yh < CLIP_BOTTOM)
			{
				uint8_t ch = yh - CLIP_TOP;
				uint8_t skip = h - ch;
				y = CLIP_TOP;
				h = ch;
				sdat += skip << 1;
			}
			else
			{
				continue;			// totally clipped
			}
		}
		else if (yh >= CLIP_BOTTOM)
		{
			h = CLIP_BOTTOM - y;
		}

		uint8_t ty = y >> 3;				// tile Y pos
		uint8_t sy = y & 0x7;				// sub-tile Y pixel (0-7)
		uint8_t th = (((uint16_t)h + sy + 7) >> 3);	// height in tiles
		
		uint8_t numt = tw * th;
		
		if (next_sram_tile + numt >= NUM_RAMTILES)	// out of RAM tiles for sprites?
		{
			sinfo->def |= 0x80;			// disable the sprite
			continue;
		}

		for (x = 0; x != tw; x++)
		{
			uint8_t *pos = (((uint8_t *)ScreenMem) + (ty * H_CHARS) + (tx+x));
			const uint8_t *srom = sdat;
			sdat += oh * 2;

			uint8_t *tram = 0;
			uint8_t nt;
			uint8_t tc = sy;
			for (y = 0; y < h; y++)
			{
				if (tc == 0 || y == 0)
				{
					uint8_t ot = *pos;
					nt = next_sram_tile++;
					// if room, save old tile (unsaved tiles restored to BLANK_TILE)
					if (nt < MAX_SAVE_TILES)
					{
						save_tiles[nt] = ot;
					}

					tram = &RAMTiles[nt];
					// was old character in 
					if (ot < 0x80)
					{
						const uint8_t *rom = (const uint8_t *)((uint16_t)rom_tile_high<<8) + ot;
						
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
						rom += FONT_CHARS;
						tram += NUM_RAMTILES;
						*tram = pgm_read_byte(rom);
					}
					else
					{
						uint8_t *ram = &RAMTiles[ot&0x7f];
						
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
						tram += NUM_RAMTILES;
						ram += NUM_RAMTILES;
						*tram = *ram;
					}
					*pos = 0x80 | nt;
					pos += H_CHARS;

					tram = &RAMTiles[nt];
					if (y == 0)
						tram += tc*NUM_RAMTILES;
				}
				tc = (tc + 1) & 0x7;
				uint8_t m = pgm_read_byte(srom++);
				uint8_t b = pgm_read_byte(srom++);
				*tram = (*tram & m) ^ b;
				tram += NUM_RAMTILES;
			}
		}
	}
}

static void clear_sprites()
{
	// restore screen (in backwards order)
	for (int8_t s = MAX_SPRITES-1; s >= 0; s--)
	{
		sprite_ram *sinfo = &sprite_info[s];
		if (sinfo->def & 0x80)
		{
			sinfo->def &= 0x7f;
			continue;
		}

		uint8_t def = sinfo->def-1;
			
		// skip zero or invalid sprite
		if (def >= SPRITE_NUM_IMAGES)
		{
			continue;
		}

		const sprite_rom *sdef = &sprite_def[def];

		// X calculations
		uint8_t x = sinfo->x;
		uint8_t w = pgm_read_byte(&sdef->w);


		// if X off right or left edge 
		uint8_t xw = x+w;
		if (
#if CLIP_LEFT > 0	// avoid warning if CLIP_LEFT 0
		x < CLIP_LEFT ||
#endif
		x >= CLIP_RIGHT)
		{
			// check if X+width wraps around to be visible on left edge
			if (
#if CLIP_LEFT > 0	// avoid warning if CLIP_LEFT 0
			xw >= CLIP_LEFT &&
#endif
			xw < CLIP_RIGHT)
			{
				x = CLIP_LEFT;
				w = xw - CLIP_LEFT;
			}
			else
			{
				continue;			// totally clipped
			}
		}
		else if (xw >= CLIP_RIGHT)			// is it extending off right edge?
		{
			w = CLIP_RIGHT - x;
		}
		
		uint8_t tx = x >> 3;				// tile X pos
		uint8_t sx = x & 0x7;				// sub-tile X pixel (0-7)
		uint8_t tw = ((uint16_t)w + sx + 7) >> 3;	// width in tiles

		// Y calculations
		uint8_t y = sinfo->y + v_fine_scroll;				// desired Y position
		uint8_t h = pgm_read_byte(&sdef->h);				// sprite height

		uint8_t yh = y + h;
		if (
#if CLIP_TOP > 0	// avoid warning if CLIP_TOP 0
		y < CLIP_TOP ||
#endif
		y >= CLIP_BOTTOM)
		{
			// check if X+width is visible
			if (
#if CLIP_TOP > 0	// avoid warning if CLIP_TOP 0
			yh >= CLIP_TOP &&
#endif
			yh < CLIP_BOTTOM)
			{
				y = CLIP_TOP;
				h = yh - CLIP_TOP;
			}
			else
			{
				continue;			// totally clipped
			}
		}
		else if (yh >= CLIP_BOTTOM)
		{
			h = CLIP_BOTTOM - y;
		}

		uint8_t ty = y >> 3;				// sub-tile Y pixel (0-7)
		uint8_t sy = y & 0x7;				// tile Y pos
		uint8_t th = ((uint16_t)h + sy + 7) >> 3;	// height in tiles

		uint8_t numt = tw * th;

		next_sram_tile -= numt;
		uint8_t st = next_sram_tile;

		for (x = 0; x != tw; x++)
		{
			uint8_t *pos = (((uint8_t *)ScreenMem) + (ty * H_CHARS) + (tx+x));
			for (y = 0; y != th; y++)
			{
				if (st < MAX_SAVE_TILES)
					*pos = save_tiles[st];
				else
					*pos = BLANK_TILE;
				st++;
				pos += H_CHARS;
			}
		}
	}
	
	next_sram_tile = 0;
}

////////////////////////////////////////////////
//
// Game variables and defines
//
////////////////////////////////////////////////

#if 0
uint8_t		note0;
uint8_t		note_count0;
uint8_t		note1;
uint8_t		note_count1;
uint8_t		note2;
uint8_t		note_count2;
#endif

// Demo features config

#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
#define BLINK_LED_PIN		15				// LED to toggle, -1 for no LED
#else
#define BLINK_LED_PIN		13				// LED to toggle, -1 for no LED
#endif

// Hackvision controls (see http://nootropicdesign.com/hackvision/)
#define	BUTTON_RIGHT_PIN	2
#define	BUTTON_LEFT_PIN		3
#define BUTTON_UP_PIN		4
#define BUTTON_DOWN_PIN		5
#define BUTTON_FIRE_PIN		10

#define BUTTON_A_PIN		0				// optional button B, -1 for none
#define ANALOG_A_PIN		3				// analog input to monitor, -1 for none
#define BUTTON_B_PIN		12				// optional button B, -1 for none

#define MAPSIZE_X	H_CHARS
#define MAPSIZE_Y	2048
#define FUEL_MAX	((H_CHARS-13)* 8)
#define PIXELS_X	(H_CHARS*8)

enum screen_st
{
	BOOT_SCREEN,
	TITLE_SCREEN,
	GAME_SCREEN,
	HIGH_SCORE_SCREEN,
};

uint8_t screen_state;
uint8_t old_screen_state;
uint16_t screen_timer;

enum game_st
{
	GAME_INIT,
	GAME_PLAY
};

uint8_t game_state;

int16_t	map_x;
int16_t	map_y;
int font;
int8_t last_vcount;

uint32_t score;
uint8_t fuel;	// topped off
uint8_t smart_bombs;
uint8_t ships;

uint8_t center_x = PIXELS_X/2;
uint8_t min_width = PIXELS_X/3;
uint8_t max_width = PIXELS_X/2;
int8_t left_x;
int8_t right_x;

static void draw_HUD()
{
	// fuel gauge
	CursorPos(0, V_CHARS-2);
	Print("\x12\x13\x14");	// "POWER:(" in compressed font
	char *p = cursor;
	uint8_t f = fuel > FUEL_MAX ? FUEL_MAX : fuel;
	for (uint8_t g = 0; g < FUEL_MAX; g+=8)
	{
		if ((g & 0xf8) < (f & 0xf8))
			*p++ = 0x1d;	// full fuel piece
		else if ((g & 0xf8) > (f & 0xf8))
			*p++ = 0x15;	// empty fuel piece
		else
			*p++ = 0x15+(f & 0x7);	// partial fuel piece
	}
	*p++ = 0x1e;	// ")" fuel right edge
	cursor = p;

	// smart bombs remaining
	Print("\x66\x67\x68");	// "BOMBS:"
	p = cursor;
	for (uint8_t b = 0; b < 5; b++)
	{
		*p++ = (smart_bombs > b) ? 0x1f : 0x00;	// smart bomb char or blank
	}
	
	// score
	CursorPos(0, V_CHARS-1);
	Print("\x60\x61\x62");	// "SCORE:"
	printdec(score);
//	printbyte(score>>16);
//	printword(score&0xffff);

	// blinking fuel warning
	if (fuel < (FUEL_MAX/6) && (vblank_count & 0x10))
		Print(" \x69\x6a ");
	else
		Print("    ");
	
	// ships remaining
	Print("\x63\x64\x65");	// "SHIPS:"
	for (uint8_t s = 0; s < 5; s++)
	{
		if (s < ships)
			*cursor++ = 0x5c;
		else
			*cursor++ = ' ';
	}
}

static void draw_horizontal_playfield_line(char *pos)
{
	uint8_t nl, nr;
	nl = left_x + random(-1, 2);
	nr = right_x + random(-1, 2);

	if ((nr - nl) > max_width)
	{
		nl += 1;
		nr -= 1;
	}
	
	if ((nr - nl) < min_width)
	{
		nl -= 1;
		nr += 1;
	}
	
	if (nl < 8)
		nl = 8;

	if (nr > (PIXELS_X-8))
		nr = (PIXELS_X-8);
		
	char *ep = pos + (nl>>3);
	char *p = pos;

	while (p < ep)
		*p++ = 0x01;	// solid
	
	*p++ = 0x2 + (nl & 0x7);

	left_x = nl;
	ep = pos + (nr>>3);

	while (p < ep)
		*p++ = 0x00;	// blank

	*p++ = 0xa + (nr & 0x7);	// r half

	ep = pos + H_CHARS-1;
	while (p <= ep)
		*p++ = 0x1;
	right_x = nr;
}

static void update_stats()
{
	if (fuel && (vblank_count & 0x003f) == 0)
		fuel--;
		
	if (random(0, 10000) >= 9900)
		score += (random(0, 8) * 13);
	
	if (score > 999999L)
		score = 999999L;

	draw_HUD();
}

static void update_vertical_cavern_screen()
{
	v_fine_scroll = (v_fine_scroll-1)&0x7;
	if (v_fine_scroll == 7)
	{
		memmove(POS(0, 1), POS(0, 0), H_CHARS * (V_CHARS-3));
		draw_horizontal_playfield_line(POS(0, 0));
	}

	sprite_info[0].def = SPRITE_death_star;
	
#if BUTTON_LEFT_PIN >= 0
	if (FastPin<BUTTON_LEFT_PIN>::read() == LOW)
		sprite_info[0].x--;
#endif

#if BUTTON_RIGHT_PIN >= 0
	if (FastPin<BUTTON_RIGHT_PIN>::read() == LOW)
		sprite_info[0].x++;
#endif

#if BUTTON_UP_PIN >= 0
	if (FastPin<BUTTON_UP_PIN>::read() == LOW)
		sprite_info[0].y--;
#endif

#if BUTTON_DOWN_PIN >= 0
	if (FastPin<BUTTON_DOWN_PIN>::read() == LOW)
		sprite_info[0].y++;
#endif

#if BUTTON_FIRE_PIN >= 0
//	if (FastPin<BUTTON_FIRE_PIN>::read() == LOW)
#endif
}

static void draw_initial_vertical_cavern_screen()
{
	for (int8_t i = V_CHARS-3; i >= 0; i--)
	{
		draw_horizontal_playfield_line(POS(0, i));
	}
}

static void draw_game_screen()
{
	if (screen_timer == 0)
	{
		memset(sprite_info, 0, sizeof (sprite_info));
		TTVT_SetFont(OSI_font8x8);
		draw_initial_vertical_cavern_screen();
	}
	else
		update_vertical_cavern_screen();
	
	update_stats();
}

static void draw_BOOT()
{
	if (screen_timer == 0)
	{
		TTVT_SetFont(Logo_font8x8);
		for (uint16_t c = 0; c < 1024; c++)
			RAMTiles[c] = pgm_read_byte(hex_font8x8 + c) ^ 0xff;
		memcpy_P(POS(0,0), LogoTiles, 22*22);
		sprite_info[0].x = 8;
		sprite_info[0].y = 8;
		sprite_info[1].x = 32;
		sprite_info[1].y = 32;
		sprite_info[2].x = 64;
		sprite_info[2].y = 64;
	}
		
	sprite_info[0].def = SPRITE_death_star;
	sprite_info[1].def = SPRITE_square;
	sprite_info[2].def = SPRITE_XOR_test;
	
#if BUTTON_LEFT_PIN >= 0
	if (FastPin<BUTTON_LEFT_PIN>::read() == LOW)
		sprite_info[0].x--, sprite_info[1].x--, sprite_info[2].x--;
#endif

#if BUTTON_RIGHT_PIN >= 0
	if (FastPin<BUTTON_RIGHT_PIN>::read() == LOW)
		sprite_info[0].x++, sprite_info[1].x++, sprite_info[2].x++;
#endif

#if BUTTON_UP_PIN >= 0
	if (FastPin<BUTTON_UP_PIN>::read() == LOW)
		sprite_info[0].y--, sprite_info[1].y--, sprite_info[2].y--;
#endif

#if BUTTON_DOWN_PIN >= 0
	if (FastPin<BUTTON_DOWN_PIN>::read() == LOW)
		sprite_info[0].y++, sprite_info[1].y++, sprite_info[2].y++;
#endif

#if BUTTON_FIRE_PIN >= 0
	if (FastPin<BUTTON_FIRE_PIN>::read() == LOW)
		screen_state = TITLE_SCREEN;
#endif

	if (screen_timer >= (VIDEO_FPS * 10))
	{
		screen_state = TITLE_SCREEN;
	}
}

static uint16_t spr_x[MAX_SPRITES];

static void draw_SPRITES()
{
	if (screen_timer == 0)
	{
		memset(sprite_info, 0, sizeof (sprite_info));
		TTVT_SetFont(OSI_font8x8);
		for (uint16_t c = 0; c < 1024; c++)
			RAMTiles[c] = pgm_read_byte(hex_font8x8 + c) ^ 0xff;
		for (uint8_t v = 0; v < V_CHARS; v++)
			for (uint8_t h = 0; h < H_CHARS; h++)
				*(POS(h,v)) = (h+(v*(H_CHARS-1)))&0x7f;
		
		for (uint8_t s = 1; s < MAX_SPRITES; s++)
		{
			sprite_info[s].def = SPRITE_ball_1;
		}
		CursorPos(0, V_CHARS-2);
		clearto(POS(H_CHARS-1, V_CHARS-1), BLANK_TILE);
		CursorPos(0, V_CHARS-2);
		Print("*SPRITE STRESS TEST* ");
		CursorPos(0, V_CHARS-1);
		Print("SPRITE FLICKER NORMAL");
	}

	sprite_info[0].def = SPRITE_smiley_test;
	
#if BUTTON_LEFT_PIN >= 0
	if (FastPin<BUTTON_LEFT_PIN>::read() == LOW)
		sprite_info[0].x--;
#endif

#if BUTTON_RIGHT_PIN >= 0
	if (FastPin<BUTTON_RIGHT_PIN>::read() == LOW)
		sprite_info[0].x++;
#endif

#if BUTTON_UP_PIN >= 0
	if (FastPin<BUTTON_UP_PIN>::read() == LOW)
		sprite_info[0].y--;
#endif

#if BUTTON_DOWN_PIN >= 0
	if (FastPin<BUTTON_DOWN_PIN>::read() == LOW)
		sprite_info[0].y++;
#endif

	for (uint8_t s = 1; s < MAX_SPRITES; s++)
	{
		spr_x[s] += s;
		sprite_info[s].x = (spr_x[s]>>4);
		uint8_t oy = sprite_info[s].y;
		sprite_info[s].y = (spr_x[s]/3);
		if (oy > 0xf0 &&  sprite_info[s].y < 0x10 && sprite_info[s].x > 0x90)
		{
			uint8_t sp = sprite_info[s].def + 1;
			if (sp > SPRITE_NUM_IMAGES)
				sp = 1;
			sprite_info[s].def = sp;
		}
	}
		
#if BUTTON_FIRE_PIN >= 0
	if (FastPin<BUTTON_FIRE_PIN>::read() == LOW)
	{
		screen_state = GAME_SCREEN;
	}
#endif

}

//
// Arduino setup function (called once after reset)
//
void setup()
{
#if BLINK_LED_PIN >= 0
	FastPin<BLINK_LED_PIN>::setOutput();
	FastPin<BLINK_LED_PIN>::lo();
#endif
#if DEBUG_LED_PIN >= 0
	FastPin<DEBUG_LED_PIN>::setOutput();
	FastPin<DEBUG_LED_PIN>::lo();
#endif
#if BUTTON_FIRE_PIN >= 0
	FastPin<BUTTON_FIRE_PIN>::setInput();
	FastPin<BUTTON_FIRE_PIN>::hi();		// INPUT_PULLUP
#endif
#if BUTTON_UP_PIN >= 0
	FastPin<BUTTON_UP_PIN>::setInput();
	FastPin<BUTTON_UP_PIN>::hi();		// INPUT_PULLUP
#endif
#if BUTTON_DOWN_PIN >= 0
	FastPin<BUTTON_DOWN_PIN>::setInput();
	FastPin<BUTTON_DOWN_PIN>::hi();		// INPUT_PULLUP
#endif
#if BUTTON_LEFT_PIN >= 0
	FastPin<BUTTON_LEFT_PIN>::setInput();
	FastPin<BUTTON_LEFT_PIN>::hi();		// INPUT_PULLUP
#endif
#if BUTTON_RIGHT_PIN >= 0
	FastPin<BUTTON_RIGHT_PIN>::setInput();
	FastPin<BUTTON_RIGHT_PIN>::hi();	// INPUT_PULLUP
#endif
	serial_init();
	
	SerialPrint("\n\nTVoutGameKit Ex4 started!\n");
	SerialPrint("ROM used=");
	serial_print_hex_u16((uint16_t)__data_load_end);
	SerialPrint(" RAM used=");
	serial_print_hex_u16((uint16_t)_end - (uint16_t)__data_start);
	SerialPrint("\n");
	if ((char *)RAMTiles != __data_start)	// check if RAMtiles was allocated first (so its 256 byte alignment doesn't waste SRAM)
	{
		SerialPrint("WARNING: Alignment SRAM waste (up to 255 bytes)\n");
	}
  freq0 = 0x0100;
  freq1 = 0x0380;
  freq2 = 0x0080;
  left_x = (PIXELS_X/2)-((PIXELS_X/2)/2);
  right_x = (PIXELS_X/2)+((PIXELS_X/2)/2);
  fuel = FUEL_MAX+8;  // topped off
  score = 123456;

	// setup TVoutGameKit to begin video display
	TVGTK_Setup();
	
	for (uint8_t i = 0; i < 8; i++)
	{
		uint8_t b = 0;
		do
		{
			RAMTiles[(i*NUM_RAMTILES)+b] = 0xff ^ pgm_read_byte(&OSI_font8x8[(i*128)+b]);
		} while (++b < 128);
	}

	// feeble attempt to make the game not exactly the same each boot
	uint32_t seed = 0xdeadbeef;
	// mix in some analog reads (for hopefully some entropy)
	seed += (uint32_t)analogRead(A0) + (uint32_t)scan_line + (uint32_t)analogRead(A1) + (uint32_t)scan_line + (uint32_t)analogRead(A2);
	// mix in SRAM contents
	for (uint8_t *r = (uint8_t *)__data_start; r < (uint8_t *)__stack; r++)
	{
		seed = (seed << 1) + (((int32_t)seed < 0) ? 1 : 0) + *r;
	}

	randomSeed(seed);	
	SerialPrint("seed=");
	serial_print_hex_u16(seed>>16);
	serial_print_hex_u16(seed&0xffff);
	SerialPrint("\n");

	WaitEndDisplay(VIDEO_FPS * 2);	// wait two seconds before loop() starts getting called
	
	CursorPos(0, 0);
	clearto(POS(H_CHARS-1, V_CHARS-1), ' ');	// clear screen

	screen_state = BOOT_SCREEN;
	last_vcount = vblank_count;
	
	TCCR2A=0x83;
	TCCR2B=0x01;
	OCR2A = 0x80;
	DDR_SND |= _BV(SND_PIN);
	
	SerialPrint("\nSetup has finished\n");
}

//
// Arduino "loop" function (called repeatedly in an infinite loop)
//
void loop()
{
	if (screen_timer == 0)
	{
		uint8_t	curspl, cursph;
		__asm__ __volatile__
		(
			"		in	%[curspl],0x3d\n"
			"		in	%[cursph],0x3e\n"
			: [curspl] "=r" (curspl), [cursph] "=r" (cursph)
			:
			:
		);
		serial_print_hex_u16(((cursph<<8)|curspl) - (uint16_t)_end);
	}
	
	if (last_vcount == (int8_t)(vblank_count & 0xff))	// if this is same frame
	{
		FastPin<BLINK_LED_PIN>::lo();	// LED off while waiting
		WaitEndDisplay();				// wait until after visible area
	}
	else
	{
		uint8_t missed = ((int8_t)(vblank_count & 0xff) - last_vcount);
		serial_putc(missed > 9 ? '!' : '0' + missed);	// too slow, frames missed!
	}
	FastPin<BLINK_LED_PIN>::hi();			// LED on while working (LED showing busy time)
	last_vcount = (int8_t)(vblank_count & 0xff);		// remember frame (low byte is enough)
	
	clear_sprites();
	
	switch (screen_state)
	{
		case BOOT_SCREEN:
			draw_BOOT();
			break;

		case TITLE_SCREEN:
			draw_SPRITES();
			break;

		case HIGH_SCORE_SCREEN:
		case GAME_SCREEN:
			draw_game_screen();
			break;
			
		default:
		{
			SerialPrint("bad screen_state=");
			serial_print_hex_u8(screen_state);
			SerialPrint("\n");
			screen_state = BOOT_SCREEN;
		}
	}

	FastPin<BLINK_LED_PIN>::lo();		// LED off while waiting
//	FastPin<BLINK_LED_PIN>::hi();		// LED off while waiting
	
	draw_sprites();

#if 0
	CursorPos(0, V_CHARS-1);
	Print("RAM:");
	printbyte(next_sram_tile);
	Print(" SAVE:");
	printbyte(next_save_tile);
#endif

	if (old_screen_state != screen_state)
	{
		old_screen_state = screen_state;
		screen_timer = 0;
	}
	else
	{
		screen_timer++;
	}
}

// EOF

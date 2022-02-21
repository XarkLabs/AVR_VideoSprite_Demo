/*
 Copyright (c) 2010 Myles Metzer

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

// TVGTK Tiled rendering and other modifications to the
// original tvout-arduino library by Xark

// README:
// The TVGTK library was derived (i.e., hacked mercilessly)
// from tvout-arduino (see https://github.com/Avamander/arduino-tvout)
// to see what kind of tiled rendering (aka character graphics)
// the Arduino is capable of.  Because of this, I have kept Myles Metzer's
// copyright and license intact (even if not too much original code is left).
// I would like to thank Myles Metzer for releasing his excellent library as
// open-source allowing me to do this (as it is highly unlikely I would have
// been able to do this without being able to examine and experiment with his
// library - and knowing it is even possible to generate video this way).
// I would also like to acknowledge Batsocks (http://www.batsocks.co.uk/) for
// their excellent article "Text on TV" which I found very interesting and
// informative and is a great writeup of most of the issues involved (although
// I did have my prototype character graphics code based on tvout working before
// I found this article).  Also, the SPI method that Batsocks uses for outputting
// pixels is different than the tvout single-bit method (and seems superior,
// although it "hogs" the single SPI port on most Arduinos), so the code and
// hardware video hookup is different.

#ifndef TVOUT_GAMEKIT_H
#define TVOUT_GAMEKIT_H
#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

// macro expands argument (if a #define) and converts to double quoted string literal
#if !defined(STRINGIZE)
#define STRINGIZE2(x)	#x
#define STRINGIZE(x)	STRINGIZE2(x)
#endif

// About tvout "pixels":
// As the display is "painted" from the left sweeping to the right, the exact time the video signal pin
// is changed determines the "width" of the pixels output.  Since up to around 672 16Mhz "cycles" are visible
// on each scanline a 16 Mhz AVR can generate around that horizontal resolution (with many resrictions
// and trade-offs).  Since "ultimate visual fidelity" is not the main goal here, a more modest 160 horizontal
// resolution is nominally targetted (the highest resolution with consistent "quality" and flexibiity).
// This resolution seems to best suit the Arduino hardware constraints for game use (and is the "default").

// Pixel vs cycle guide:
// 1 cycle @ 16Mhz
//         = 1/16000000 of a second
//	   = 0.625 microseconds
//	   = 62.5ns
//	   = The time it takes a neutrino particle to travel ~61.4 feet through the solid Earth. :-)
//
// NOTE: Normally video pixels are not clocked at exactly 16Mhz, so TVOout pixels are a bit "wide"
// compared to typical video displays with similar resolutions.
//
//  16Mhz Cycles  | Approx horiz resolution  |  Older computers with comparable resolutions
// ---------------+--------------------------+--------------------------------------------
// 	1		640			C= C-128 with RGB monitor, CGA mono
// 	2		320			C= PET 2001, CGA & many 8-bits
// 	3		208			almost like NES
//	4		160			C= Vic-20, Atari 2600, OSI-C1P
//
// NOTE: These comparisons are very rough and ignore colors and many other factors.
//       Please forgive me if I have omitted (or "dissed") your favorite system. :-) 

// Due to 16Mhz Adruino timing constraints, the highest "perfect" resolution that can be supported
// with a normal tile map is 4 cycles per pixel (because a read from program memory takes 3 cycles).
// However, you can get nearly 2 cycles-per pixel by "stretching" the last pixel (20 cycles
// per character).  This looks fine for most text (mostly looks bad on "dither" patterns).  You
// can also use a 7-bit wide tileset (skip bit 0) to get a ~40 column display.
// NOTE: These limits don't apply with Batsocks SPI video output method. I believe you could get perfect
// 2 cycle bitmap or tile accuracy using that output method (because you should have enough cycles between
// bytes to load new data for the 9th bit). See http://www.batsocks.co.uk/readme/art_SerialVideo_1.htm

// For vertical pixel size, it is just the integer number of scanlines up to around 216 visible on NTSC
// or 256 on PAL (non-interlaced).
// NOTE: PAL vertical playfield resolutions > 256 may cause issues.

#define	NTSC		(0)
#define	PAL		(1)

// Wait until end of active area has been displayed for num_frames (to avoid updating during display)
void WaitEndDisplay(uint16_t num_frames = 1);

extern "C"
{
	// Video mode functions to pass to Setup to select the desired "video mode" (or have fun making "custom" modes)
	// NOTE: These are not intended to be called directly!

	// low-level video "scan-line render" functions, these actually create the display image and are setup by the mode functions above
	// NOTE: These are not intended to be called directly!
	void empty();				// empty TVTinyText

	// internal rendering variables (these are modified by interrupts)
	extern void 		(*line_handler)();	// function to handle current scan-line generation
	extern uint16_t		scan_line;		// current NTSC/PAL video line
	extern uint8_t*		screen_ram_ptr;		// tile buffer address being displayed
	extern uint8_t		tile_line;		// current tile line being displayed (0 to FONT_VHEIGHT-1)

	extern uint16_t		vblank_count;		// number of frames displayed

	// used to control display properties
	extern uint8_t		ram_tile_high;		// current ROM font address (high byte only, must be 256 byte aligned)
	extern uint8_t		rom_tile_high;		// current RAM font address (high byte only, must be 256 byte aligned)
	
	extern int8_t		h_pos_offset;		// horizontal screen position offset (when not a fixed constant)
	extern int8_t		v_pos_offset;		// vertical screen postition offset (when not a fixed constant)
	
	extern uint8_t		h_fine_scroll;		// horizontal fine scroll 0-7 pixels (if HSCROLL is defined)
	extern uint8_t		v_fine_scroll;		// vertical fine scroll 0-7 pixels (if VSCROLL is defined)
	extern uint8_t		h_fine_scroll_mask;
	
	extern uint8_t		perf_tick;
};

#include "hardware_setup.h"		// AVR hardware setup
#include "video_properties.h"		// video timing definitions

// Get current frame (incremented after active display 50/60 times a second for PAL/NTSC)
static inline uint8_t GetFrameCount()
{
	return vblank_count;
}

// Set current ROM font (font must be on 256 byte boundary using)
static inline void TTVT_SetFont(const uint8_t *font_ptr)
{
	rom_tile_high = (uint8_t)((uint16_t)font_ptr>>8);
}

static inline void TTVT_SetHScroll(uint8_t h)
{
	h &= 0x7;
	h_fine_scroll_mask = ((uint8_t)0xff) >> h;
	h_fine_scroll = h;
}

static inline void TTVT_SetVScroll(uint8_t v)
{
	v_fine_scroll = v & 0x7;
}

//
// Video generation code (if TV_MODE was defined)
//
#if defined(TV_MODE)				// this should be defined if we are actually setting up video mode

void render_inactive_line();

// Define video buffers
extern "C"
{
#if defined(NUM_RAMTILES) && (NUM_RAMTILES > 0)
	uint8_t		RAMTiles[8*NUM_RAMTILES] __attribute__ ((aligned(256))) __attribute__ ((section(".data")));	// hopefully this will land at 0x100 (first possible address) and not waste any RAM (must be 256 byte aligned)
#endif
	uint8_t 	ScreenMem[H_CHARS * V_CHARS];					// screen text buffer to hold character tile indices
}

// setup display variables based on PAL/NTSC flag
#if !defined(SCREEN_HEIGHT)
	#define	SCREEN_HEIGHT	((V_CHARS-V_SCROLL)*CHAR_VHEIGHT)
#endif
#if TV_MODE == PAL

	#if defined(V_POS_OFFSET)
		#define	START_RENDER	(uint16_t)((_PAL_LINE_MID - ((SCREEN_HEIGHT * (_PAL_LINE_DISPLAY / SCREEN_HEIGHT)) / 2)) + V_POS_ORIGIN + v_pos_offset)
	#else
		#define	START_RENDER	(uint16_t)((_PAL_LINE_MID - ((SCREEN_HEIGHT * (_PAL_LINE_DISPLAY / SCREEN_HEIGHT)) / 2)) + V_POS_ORIGIN)
	#endif
	#define END_RENDER		(uint16_t)(START_RENDER + SCREEN_HEIGHT+1)
	#if defined (H_POS_OFFSET)
		#define	OUTPUT_DELAY	(_PAL_CYCLES_OUTPUT_START + H_POS_ORIGIN + h_pos_offset)
	#else
		#define	OUTPUT_DELAY	(_PAL_CYCLES_OUTPUT_START + H_POS_ORIGIN)
	#endif
	#define	VSYNC_END		_PAL_LINE_STOP_VSYNC
	#define	LINES_FRAME		_PAL_LINE_FRAME
	#define CYCLES_LINE		_PAL_CYCLES_SCANLINE

#else	// NTSC

	#if defined(V_POS_OFFSET)
		#define	START_RENDER	(uint16_t)((_NTSC_LINE_MID - ((SCREEN_HEIGHT * (_NTSC_LINE_DISPLAY / SCREEN_HEIGHT)) / 2) + 8) + V_POS_ORIGIN + v_pos_offset)
	#else
		#define	START_RENDER	(uint16_t)((_NTSC_LINE_MID - ((SCREEN_HEIGHT * (_NTSC_LINE_DISPLAY / SCREEN_HEIGHT)) / 2) + 8) + V_POS_ORIGIN)
	#endif
	#define END_RENDER		(uint16_t)(START_RENDER + SCREEN_HEIGHT+1)
	#if defined (H_POS_OFFSET)
		#define	OUTPUT_DELAY	(_NTSC_CYCLES_OUTPUT_START + H_POS_ORIGIN + h_pos_offset)
	#else
		#define	OUTPUT_DELAY	(_NTSC_CYCLES_OUTPUT_START + H_POS_ORIGIN)
	#endif
	#define	VSYNC_END		_NTSC_LINE_STOP_VSYNC
	#define	LINES_FRAME		_NTSC_LINE_FRAME
	#define CYCLES_LINE		_NTSC_CYCLES_SCANLINE

#endif

// interrupt service routine
ISR(TIMER1_OVF_vect)
{
//	PINB	= (1<<4);
#if defined(DEBUG_LED_PIN)
	FastPin<DEBUG_LED_PIN>::hi();
#endif
#if defined(LINE_START_HANDLER)
	LINE_START_HANDLER();			// perform hsync handler task if any (e.g., for UART or PS/2 polling)
#endif

  	// call scanline handler
	line_handler();

#if defined(LINE_END_HANDLER)
	LINE_END_HANDLER();			// perform hsync handler task if any (e.g., for UART or PS/2 polling)
#endif
//	PINB	= (1<<4);
#if defined(DEBUG_LED_PIN)
	FastPin<DEBUG_LED_PIN>::lo();
#endif
}

//
// Video renderer
//
#if VIDEO_LITTLE_ENDIAN
	#define	VSH		"lsr"		// bits shift to right for little endian output (bit 0 first)
#else
	#define	VSH		"lsl"		// bits shift to left for big endian output (bit 7 first)
#endif

// tilemap with 6-pixels/tile at 2 cycles/pixel w/last @ 6 cycles
void render_tile_cyc16x6()
{
	__asm__ __volatile__
	(
		"		lds	r26,screen_ram_ptr\n"
		"		lds	r27,screen_ram_ptr+1\n"
		"		lds	r30,tile_line\n"
#if DOUBLE_LINES
		"		cpi	r30," STRINGIZE(FONT_VHEIGHT) "*2\n"
		"		brlo	1f\n"
		"		clr	__tmp_reg__\n"
		"		rjmp	9f\n"
		"1:		lds	r31,rom_tile_high\n"
		"		lsr 	r30\n"					// tile_line bit 0 -> C
#if FONT_CHARS != 256
		"		lsr 	r30\n"					// tile_line bit 1 -> C
		"		clr	__zero_reg__\n"
		"		ror	__zero_reg__\n"				// C -> bit 7
		"		add	r31,r30\n"				// add to romptr high
#else	// FONT_CHARS == 256
		"		add	r31,r30\n"
#endif
#else	// ! DOUBLE_LINES

		"		cpi	r30," STRINGIZE(FONT_VHEIGHT) "\n"
		"		brlo	1f\n"
		"		clr	__tmp_reg__\n"
		"		rjmp	9f\n"
		"1:		lds	r31,rom_tile_high\n"
#if FONT_CHARS != 256
		"		lsr 	r30\n"					// tile_line bit 0 -> C
		"		clr	__zero_reg__\n"
		"		ror	__zero_reg__\n"				// C -> bit 7
#endif
		"		add	r31,r30\n"				// add to romptr high
#endif	// end !DOUBLE_LINES
		"		ld	r30,X+\n"				// pre-load tile tile index from X++ into ZL
#if FONT_CHARS != 256
		"		add 	r30,__zero_reg__\n"			// add line offset low for font index
#endif
		"		lpm	__tmp_reg__,Z\n"			// pre-load indexed tile bitmap data from ZH:ZL ROM ptr
		"		sub	%[time],%[tcnt1l]\n"
		"0:		subi	%[time],3\n"
		"		brcc	0b\n"
		"		subi	%[time],0-3\n"
		"		breq	1f\n"
		"		dec	%[time]\n"
		"		breq	2f\n"
		"		rjmp	2f\n"
		"1:		nop\n" 
		"2:\n"
		"	.rept	" STRINGIZE(H_CHARS) "\n"	// repeat tile output code for number of tiles
		"		out 	%[port],__tmp_reg__\n"		// pixel 0 = data bit 7
		"		" VSH " __tmp_reg__\n"			// get next pixel ready
		"		out 	%[port],__tmp_reg__\n"		// pixel 1 = data bit 6
		"		" VSH " __tmp_reg__\n"			// get next pixel ready
		"		out 	%[port],__tmp_reg__\n" 		// pixel 2 = data bit 5
		"		" VSH " __tmp_reg__\n"			// get next pixel ready
		"		out 	%[port],__tmp_reg__\n"		// pixel 3 = data bit 4
		"		" VSH " __tmp_reg__\n"			// get next pixel ready
		"		out 	%[port],__tmp_reg__\n"		// pixel 4 = data bit 3 (now in temp)
		"		" VSH " __tmp_reg__\n"			// get next pixel ready
		"		out 	%[port],__tmp_reg__\n"		// pixel 5 = data bit 2
		"		ld	r30,X+\n"			//+(2 cycle slot)
#if FONT_CHARS != 256
		"		add 	r30,__zero_reg__\n"			// add line offset low for font index
#endif
		"		lpm	__tmp_reg__,Z\n"		//+(3 cycle slot)
		"	.endr\n"				// end repeat
		"		cbi	%[port]," STRINGIZE(VID_PIN) "\n" // black after end (or TVs unhappy)
		"		clr	__zero_reg__\n"			 // make sure to restore __zero_reg__
		"9:\n"
		:							// outputs (none)
		: [port] "i" (_SFR_IO_ADDR(PORT_VID)),			// inputs
		  [time] "a" (OUTPUT_DELAY),
		  [tcnt1l] "a" (TCNT1L)
		: "r26", "r27", "r30", "r31"			// registers clobbered
	);
	
	// line is finished, update various counters and state
	if (++tile_line >= CHAR_VHEIGHT)			// if >= tileset height process next tile row
	{
		tile_line = 0;
		screen_ram_ptr += H_CHARS;			// display next tile row
	}

	if (++scan_line >= END_RENDER) 				// if active display rendering completed, call hook then increment framecount
	{
#if defined(END_DISPLAY_HANDLER)
		END_DISPLAY_HANDLER();				// perform end display synchronization tasks if any
#endif
		line_handler = render_inactive_line;		// inactive until next vblank
		vblank_count++;
	}
}

// tilemap with 8 pixels/tile at 4 cycles/pixel and 32  (~24 columns, ~160 pixel res)
void render_tile_cyc32_24()
{
	__asm__ __volatile__
	(
		"		clr	r17\n"				// clear tile low offset (default zero tile bitmap offset for even tile lines)
		"		lds	r18,tile_line\n"		// current tile line (0 to FONT_VHEIGHT-1)
		"		lds	r31,rom_tile_high\n"		// start of ROM tile bitmap data high in ZH
		"		add	r31,r18\n"
		"		lds	r26,screen_ram_ptr\n"		// start of screen tile RAM low in XL
		"		lds	r27,screen_ram_ptr+1\n"		// start of screen tile RAM high in XH

		"		ld	r30,X+\n"			// load tile tile index from tile index ptr in XH:XL into ZL and increment XH:XL
		"		lpm	__tmp_reg__,Z\n"		// load ROM tile bitmap data from ZH:ZL ptr

		"		ld	r30,X+\n"			// pre-load next tile index from tile index ptr in XH:XL into ZL and increment XH:XL
		"		lpm	r18,Z\n"			// pre-load next ROM tile bitmap data from ROM ptr in ZH:ZL into bitmap temp

		"		sub	%[time],%[tcnt1l]\n"
		"0:		subi	%[time],3\n"
		"		brcc	0b\n"
		"		subi	%[time],0-3\n"
		"		breq	1f\n"
		"		dec	%[time]\n"
		"		breq	2f\n"
		"		rjmp	2f\n"
		"1:		nop\n" 
		"2:\n"

		// at this point:
		//	__tmp_reg__	= current 8 pixels to shift out
		//	r18		= pre-loaded next ROM tile bitmap data
		//	ZL		= pre-loaded next tile 0-127 index
		//	SREG T		= pre-loaded next ROM/RAM tile select (0/1)
		"	.rept	" STRINGIZE(H_CHARS) "\n"	// repeat tilemap output code for H_CHARS tiles per line (outputting pixel every 4 cycles)
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 0
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 1
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 2
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 3
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		ld	r30,X+\n"			// 2 - (2 cycles) load tile tile index from tile index ptr in XH:XL into ZL and increment XH:XL
									// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 4
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 5
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 6
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	__zero_reg__,__tmp_reg__\n"	// 2 - move last pixel to temp
		"		mov	__tmp_reg__,r18\n"		// 3 - new next 8 pixels to output copied from bitmap temp
		"		out	%[port],__zero_reg__\n"		// 0 - output pixel 7 from temp
		"		lpm	r18,Z\n"			// 1 - (3 cycles) pre-load next ROM tile bitmap data from ROM ptr in ZH:ZL into bitmap temp
									// 2
									// 3
		"	.endr\n"				// ...and repeat 
		"		cbi	%[port]," STRINGIZE(VID_PIN) "\n" // black after end (or TVs unhappy)
		"		clr	__zero_reg__\n"			 // make sure to restore __zero_reg__
			:
			: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
			  [time] "a" (OUTPUT_DELAY),
			  [tcnt1l] "a" (TCNT1L)
			: "r17", "r18", "r26", "r27", "r28", "r29", "r30", "r31"
	);
	// line is finished, update various counters and state
	if (++tile_line >= CHAR_VHEIGHT)				// if >= tileset height process next tile row
	{
		tile_line = 0;
		screen_ram_ptr += H_CHARS;
	}

	if (++scan_line >= END_RENDER) 				// if active display rendering completed, call hook then increment framecount
	{
#if defined(END_DISPLAY_HANDLER)
		END_DISPLAY_HANDLER();				// perform end display synchronization tasks if any
#endif
		line_handler = render_inactive_line;		// inactive until next vblank
		vblank_count++;
	}
}

// tilemap with 8 pixels/tile at 4 cycles/pixel and 32  (~24 columns, ~160 pixel res)
void render_tile_cyc32_24_ramtiles()
{
	__asm__ __volatile__
	(
		"		clr	r17\n"				// clear tile low offset (default zero tile bitmap offset for even tile lines)
		"		lds	r18,tile_line\n"		// current tile line (0 to FONT_VHEIGHT-1)
		"		lsr	r18\n"				// divide tile line by two for tile bitmap high byte offset
		"		ror	r17\n"				// rotate in carry for tile bitmap low offset (0 or 128)
		"		lds	r31,rom_tile_high\n"		// start of ROM tile bitmap data high in ZH
		"		add	r31,r18\n"			// add high offset to ROM tile bitmap data ptr in ZH
		"		lds	r29,ram_tile_high\n"		// start of RAM tile bitmap data high in YH
		"		add	r29,r18\n"			// add high offset to RAM tile bitmap data ptr in YH
		"		lds	r26,screen_ram_ptr\n"		// start of screen tile RAM low in XL
		"		lds	r27,screen_ram_ptr+1\n"		// start of screen tile RAM high in XH

		"		ld	r30,X+\n"			// load tile tile index from tile index ptr in XH:XL into ZL and increment XH:XL
		"		bst	r30,7\n"			// store ROM/RAM tile select (bit 7 of ZL) in T bit of SREG
		"		andi	r30,0x7f\n"			// clear ROM/RAM tile select bit for 0-127 tile index in ZL
		"		add	r30,r17\n"			// add even/odd tile line offset to ZL
		"		lpm	__tmp_reg__,Z\n"		// load ROM tile bitmap data from ZH:ZL ptr
		"		mov	r28,r30\n"			// copy low offset to RAM tile bitmap data ptr in YL
		"		ld	__zero_reg__,Y\n"		// load RAM tile bitmap data from YH:YL ptr
		"		brtc	.+2\n"				// if it was a ROM tile index (T=0), branch over
		"		mov	__tmp_reg__,__zero_reg__\n"	// else use the RAM tile bitmap data

		"		ld	r30,X+\n"			// pre-load next tile index from tile index ptr in XH:XL into ZL and increment XH:XL
		"		bst	r30,7\n"			// store next ROM/RAM tile select (bit 7 of ZL) to T bit of SREG
		"		andi	r30,0x7f\n"			// clear next ROM/RAM tile select bit for 0-127 tile index in ZL
		"		add	r30,r17\n"			// add even/odd tile line offset to ZL
		"		lpm	r18,Z\n"			// pre-load next ROM tile bitmap data from ROM ptr in ZH:ZL into bitmap temp

		"		sub	%[time],%[tcnt1l]\n"
		"0:		subi	%[time],3\n"
		"		brcc	0b\n"
		"		subi	%[time],0-3\n"
		"		breq	1f\n"
		"		dec	%[time]\n"
		"		breq	2f\n"
		"		rjmp	2f\n"
		"1:		nop\n" 
		"2:\n"

		// at this point:
		//	__tmp_reg__	= current 8 pixels to shift out
		//	r18		= pre-loaded next ROM tile bitmap data
		//	ZL		= pre-loaded next tile 0-127 index
		//	SREG T		= pre-loaded next ROM/RAM tile select (0/1)
		"	.rept	" STRINGIZE(H_CHARS) "\n"	// repeat tilemap output code for H_CHARS tiles per line (outputting pixel every 4 cycles)
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 0
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	r28,r30\n"			// 2 - copy low offset to RAM tile bitmap data ptr in YL
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 1
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		ld	__zero_reg__,Y\n"		// 2 - (2 cycles) load RAM tile bitmap data from YH:YL ptr
									// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 2
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		brtc	.+2\n"				// 2 - if it was a ROM tile index (T=0), branch over
		"		mov	r18,__zero_reg__\n"		// 3 - else use the RAM tile bitmap data
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 3
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		ld	r30,X+\n"			// 2 - (2 cycles) load tile tile index from tile index ptr in XH:XL into ZL and increment XH:XL
									// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 4
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		bst	r30,7\n"			// 2 - store ROM/RAM tile select (bit 7 of ZL) in T bit of SREG
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 5
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		andi	r30,0x7f\n"			// 2 - clear ROM/RAM tile select bit for 0-127 tile index in ZL
		"		add	r30,r17\n"			// 3 - add even/odd tile line offset to ZL
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 6
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	__zero_reg__,__tmp_reg__\n"	// 2 - move last pixel to temp
		"		mov	__tmp_reg__,r18\n"		// 3 - new next 8 pixels to output copied from bitmap temp
		"		out	%[port],__zero_reg__\n"		// 0 - output pixel 7 from temp
		"		lpm	r18,Z\n"			// 1 - (3 cycles) pre-load next ROM tile bitmap data from ROM ptr in ZH:ZL into bitmap temp
									// 2
									// 3
		"	.endr\n"				// ...and repeat 
		"		cbi	%[port]," STRINGIZE(VID_PIN) "\n" // black after end (or TVs unhappy)
		"		clr	__zero_reg__\n"			 // make sure to restore __zero_reg__
			:
			: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
			  [time] "a" (OUTPUT_DELAY),
			  [tcnt1l] "a" (TCNT1L)
			: "r17", "r18", "r26", "r27", "r28", "r29", "r30", "r31"
	);
	// line is finished, update various counters and state
	if (++tile_line >= CHAR_VHEIGHT)			// if >= tileset height process next tile row
	{
		tile_line = 0;
		screen_ram_ptr += H_CHARS;
	}

	if (++scan_line >= END_RENDER) 				// if active display rendering completed, call hook then increment framecount
	{
#if defined(END_DISPLAY_HANDLER)
		END_DISPLAY_HANDLER();				// perform end display synchronization tasks if any
#endif
		line_handler = render_inactive_line;		// inactive until next vblank
		vblank_count++;
	}
}

// tilemap with 8 pixels/tile at 4 cycles/pixel and 32  (~24 columns, ~160 pixel res)
void render_tile_cyc32_24_ramtiles_scroll()
{
	__asm__ __volatile__
	(

		"		clr	r2\n"				// clear tile low offset (default zero tile bitmap offset for even tile lines)
		"		lds	r3,tile_line\n"			// current tile line (0 to FONT_VHEIGHT-1)
		"		lsr	r3\n"				// divide tile line by two for tile bitmap high byte offset
		"		ror	r2\n"				// rotate in carry for tile bitmap low offset (0 or 128)
		"		lds	r31,rom_tile_high\n"		// start of ROM tile bitmap data high in ZH
		"		add	r31,r3\n"			// add high offset to ROM tile bitmap data ptr in ZH
		"		lds	r29,ram_tile_high\n"		// start of RAM tile bitmap data high in YH
		"		add	r29,r3\n"			// add high offset to RAM tile bitmap data ptr in YH
		"		lds	r26,screen_ram_ptr\n"		// start of screen tile RAM low in XL
		"		lds	r27,screen_ram_ptr+1\n"		// start of screen tile RAM high in XH

		"		ld	r30,X+\n"			// load tile tile index from tile index ptr in XH:XL into ZL and increment XH:XL
		"		bst	r30,7\n"			// store ROM/RAM tile select (bit 7 of ZL) in T bit of SREG
		"		andi	r30,0x7f\n"			// clear ROM/RAM tile select bit for 0-127 tile index in ZL
		"		add	r30,r2\n"			// add even/odd tile line offset to ZL
		"		lpm	__tmp_reg__,Z\n"		// load ROM tile bitmap data from ZH:ZL ptr
		"		mov	r28,r30\n"			// copy low offset to RAM tile bitmap data ptr in YL
		"		ld	__zero_reg__,Y\n"		// load RAM tile bitmap data from YH:YL ptr
		"		brtc	.+2\n"				// if it was a ROM tile index (T=0), branch over
		"		mov	__tmp_reg__,__zero_reg__\n"	// else use the RAM tile bitmap data

		"		lds	r17,h_fine_scroll_mask\n"	// get horizontal scroll mask
		"		and	__tmp_reg__,r17\n"		// mask first character
		"		com	r17\n"				// complement for last character mask

		"		ld	r30,X+\n"			// pre-load next tile index from tile index ptr in XH:XL into ZL and increment XH:XL
		"		bst	r30,7\n"			// store next ROM/RAM tile select (bit 7 of ZL) to T bit of SREG
		"		andi	r30,0x7f\n"			// clear next ROM/RAM tile select bit for 0-127 tile index in ZL
		"		add	r30,r2\n"			// add even/odd tile line offset to ZL
		"		lpm	r3,Z\n"				// pre-load next ROM tile bitmap data from ROM ptr in ZH:ZL into bitmap temp

		"		lds	__zero_reg__,h_fine_scroll\n"
		"		lsl	__zero_reg__\n"
		"		lsl	__zero_reg__\n"

		"		sub	%[time],__zero_reg__\n"
		"		sub	%[time],%[tcnt1l]\n"
		"0:		subi	%[time],3\n"
		"		brcc	0b\n"
		"		subi	%[time],0-3\n"
		"		breq	1f\n"
		"		dec	%[time]\n"
		"		breq	2f\n"
		"		rjmp	2f\n"
		"1:		nop\n" 
		"2:\n"
		
		// at this point:
		//	__tmp_reg__	= current 8 pixels to shift out
		//	r3		= pre-loaded next ROM tile bitmap data
		//	ZL		= pre-loaded next tile 0-127 index
		//	SREG T		= pre-loaded next ROM/RAM tile select (0/1)
		"	.rept	" STRINGIZE(H_CHARS-2) "\n"	// repeat tilemap output code for H_CHARS tiles per line (outputting pixel every 4 cycles)
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 0
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	r28,r30\n"			// 2 - copy low offset to RAM tile bitmap data ptr in YL
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 1
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		ld	__zero_reg__,Y\n"		// 2 - (2 cycles) load RAM tile bitmap data from YH:YL ptr
									// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 2
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		brtc	.+2\n"				// 2 - if it was a ROM tile index (T=0), branch over
		"		mov	r3,__zero_reg__\n"		// 3 - else use the RAM tile bitmap data
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 3
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		ld	r30,X+\n"			// 2 - (2 cycles) load tile tile index from tile index ptr in XH:XL into ZL and increment XH:XL
									// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 4
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		bst	r30,7\n"			// 2 - store ROM/RAM tile select (bit 7 of ZL) in T bit of SREG
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 5
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		andi	r30,0x7f\n"			// 2 - clear ROM/RAM tile select bit for 0-127 tile index in ZL
		"		add	r30,r2\n"			// 3 - add even/odd tile line offset to ZL
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 6
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	__zero_reg__,__tmp_reg__\n"	// 2 - move last pixel to temp
		"		mov	__tmp_reg__,r3\n"		// 3 - new next 8 pixels to output copied from bitmap temp
		"		out	%[port],__zero_reg__\n"		// 0 - output pixel 7 from temp
		"		lpm	r3,Z\n"				// 1 - (3 cycles) pre-load next ROM tile bitmap data from ROM ptr in ZH:ZL into bitmap temp
									// 2
									// 3
		"	.endr\n"				// ...and repeat 
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 0
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	r28,r30\n"			// 2 - copy low offset to RAM tile bitmap data ptr in YL
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 1
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		ld	__zero_reg__,Y\n"		// 2 - (2 cycles) load RAM tile bitmap data from YH:YL ptr
									// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 2
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		brtc	.+2\n"				// 2 - if it was a ROM tile index (T=0), branch over
		"		mov	r3,__zero_reg__\n"		// 3 - else use the RAM tile bitmap data
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 3
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 4
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 5
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 6
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		mov	__zero_reg__,__tmp_reg__\n"	// 2 - move last pixel to temp
		"		mov	__tmp_reg__,r3\n"		// 3 - new next 8 pixels to output copied from bitmap temp
		"		out	%[port],__zero_reg__\n"		// 0 - output pixel 7 from temp
		"		and	__tmp_reg__,r17\n"		// 1 - mask last character for horizontal scroll
		"		nop\n"					// 2
		"		nop\n"					// 3
		// output last partially scrolled character
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 0
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 1
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 2
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 3
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 4
		"		" VSH " __tmp_reg__\n"        		// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 5
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 6
		"		" VSH " __tmp_reg__\n"			// 1 - shift to next pixel
		"		nop\n"					// 2
		"		nop\n"					// 3
		"		out	%[port],__tmp_reg__\n"		// 0 - output pixel 7 from temp
		"		nop\n"					// 1
		"		nop\n"					// 2
		"		clr	__zero_reg__\n"			// 3 make sure to restore __zero_reg__
		"		cbi	%[port]," STRINGIZE(VID_PIN) "\n" // black after end (or TVs unhappy)

		:
			: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
			  [time] "a" (OUTPUT_DELAY),
			  [tcnt1l] "a" (TCNT1L)
			: "r2", "r3", "r17", "r26", "r27", "r28", "r29", "r30", "r31"
	);
	// line is finished, update various counters and state
	if (++tile_line >= CHAR_VHEIGHT)			// if >= tileset height process next tile row
	{
		tile_line = 0;
		screen_ram_ptr += H_CHARS;
	}

	if (++scan_line >= END_RENDER) 				// if active display rendering completed, call hook then increment framecount
	{
#if defined(END_DISPLAY_HANDLER)
		END_DISPLAY_HANDLER();				// perform end display synchronization tasks if any
#endif
		line_handler = render_inactive_line;		// inactive until next vblank
		vblank_count++;
	}
}

// Video setup function - starts video generation interrupt
void TVGTK_Setup()
{
#if defined (__AVR_AT90USB1286__) || defined (__AVR_ATmega32U4__)
#warning NOTICE: Disabling AT90USB1286/ATmega32U4 USB (interrupt interferes with video timing).
	Delay(1000);	// make sure there is a chance to upload new code
	// See http://www.pjrc.com/teensy/jump_to_bootloader.html
	USBCON = 0;
#endif
	TIMSK0 = 0;	// disable Arduino millis interrupt (we will take over its duties)
	
	// setup AVR hardware
	DDR_VID |= _BV(VID_PIN);
	DDR_SYNC |= _BV(SYNC_PIN);
	PORT_VID &= ~_BV(VID_PIN);
	PORT_SYNC |= _BV(SYNC_PIN);
	
	// inverted fast pwm mode on timer 1
	TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11);
	TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
	ICR1 = CYCLES_LINE;
	OCR1A = _CYCLES_HORZ_SYNC;
	line_handler = &render_inactive_line;
	scan_line = LINES_FRAME+1;
	rom_tile_high = (uint8_t)((uint16_t)FONT_NAME>>8);
	TIMSK1 = _BV(TOIE1);
}

void mode_16cyc_tilex6()
{
#if V_SCROLL
	tile_line = v_fine_scroll;
#else
	tile_line = 0;
#endif
	line_handler = render_tile_cyc16x6;			// setup initial scaline rendering function
	screen_ram_ptr = (uint8_t*)ScreenMem;			// address of tilemap index buffer
}

void mode_32cyc_tilex8_ramtiles()
{
#if V_SCROLL
	tile_line = v_fine_scroll;
#else
	tile_line = 0;
#endif
#if H_SCROLL
	h_fine_scroll_mask = ((uint8_t)0xff) >> h_fine_scroll;
	line_handler = render_tile_cyc32_24_ramtiles_scroll;	// setup initial scaline rendering function
#else
	line_handler = render_tile_cyc32_24_ramtiles;		// setup initial scaline rendering function
#endif
	screen_ram_ptr = (uint8_t*)ScreenMem;			// address of tilemap index buffer
#if defined(NUM_RAMTILES) && (NUM_RAMTILES > 0)
	ram_tile_high = (uint8_t)((uint16_t)RAMTiles>>8);	// address of RAM tile bitmap buffer (high)
#endif
}

void mode_32cyc_tilex8()
{
	tile_line = 0;
	line_handler = render_tile_cyc32_24;			// setup initial scaline rendering function
	screen_ram_ptr = (uint8_t*)ScreenMem;			// address of tilemap index buffer
}

// generate inactive blank lines until time to start video mode display or vblank
void render_inactive_line()
{
	if (scan_line >= LINES_FRAME)				// if at the end of the display, start vertical-sync period
	{
		OCR1A = _CYCLES_VERT_SYNC;			// generate VSYNC pulse with inverted fast PWM
		scan_line = 0;					// reset scan line counter
	}
	else
	{
		if (scan_line == VSYNC_END)			// if at the end of vertical-sync period, restore horizontal-sync timing
		{
			OCR1A = _CYCLES_HORZ_SYNC;		// generate HSYNC pulse with inverted fast PWM
		}
		else if (scan_line == START_RENDER)		// if at the start of visible display, setup display mode
		{
			MODE_HANDLER();				// setup display mode
		}
		
		scan_line++;					// increment scan line counter
	}
}

#endif	// defined(TV_MODE)

#endif	// TVOUT_TOOLKIT_H

/*
 Copyright (c) 2010 Myles Metzer

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sells
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

// TVGTK Tiled rendering and other drastic modifications to the original TVout Arduino
// library by Xark

#include <avr/interrupt.h>	// avr-libc AVR interrupt support
#include <avr/io.h>		// avr-libc low-level AVR support
#include <avr/pgmspace.h>	// avr-libc PROGMEM support
#include <avr/power.h>		// avr-libc power saving support

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// include library header
#include "AVRInvaders.h"

// include timing constants (mostly unmodified from TVout)
#include "video_properties.h"
#include "hardware_setup.h"

extern "C"
{
	void 		(*line_handler)();	// function handing "scan-out"
	uint16_t scan_line;		// current NTSC/PAL video line
	uint16_t vblank_count;		// incremented each video frame

	uint8_t*	screen_ram_ptr;
	uint8_t		ram_tile_high __attribute__((used));
	uint8_t		rom_tile_high;
	uint8_t		tile_line;
	
	int8_t		h_pos_offset;
	int8_t		v_pos_offset;
	
	uint8_t		h_fine_scroll;
	uint8_t		v_fine_scroll;
	uint8_t		h_fine_scroll_mask __attribute__((used)) ;
}

// Wait until after the last playfield line has been displayed for 'x' frames approximately 50 Hz (PAL) or 60 Hz (NTSC).
// NOTE: This allows largest amount of time to process and update the screen before the next frame.
void WaitEndDisplay(uint16_t n)
{
	while (n--)
	{
		// wait for low byte of vblank_count to change (after last line of "active" display area)
		uint8_t startframe = *(volatile uint8_t*)&vblank_count;
		while (startframe == *(volatile uint8_t*)&vblank_count)
		{
			__asm__ __volatile__ ("	sleep\n");
		}
	}
}

// EOF

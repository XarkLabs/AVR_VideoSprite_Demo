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

// Tiled rendering and other drastic modifications to the original TVout Arduino
// library by Xark

// sound is output on OC2A
// sync output is on OC1A

#ifndef HARDWARE_SETUP_H
#define HARDWARE_SETUP_H

// device specific settings.
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__)
#define AVR_NAME	"ATmega128x"
#define AVR_NICKNAME	"128x"
#else
#define AVR_NAME	"ATmega256x"
#define AVR_NICKNAME	"256x"
#endif

#define	VIDEO_OUT_PIN	29
#define PORT_VID	PORTA
#define	DDR_VID		DDRA
#define VID_PIN		7
//sync
#define	SYNC_OUT_PIN	11
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define	SYNC_PIN	5
//sound
#define	SOUND_OUT_PIN	10
#define PORT_SND	PORTB
#define DDR_SND		DDRB
#define	SND_PIN		4

#elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
#define AVR_NAME	"ATmega644"
#define AVR_NICKNAME	"644"
#else
#define AVR_NAME	"ATmega1284"
#define AVR_NICKNAME	"644"
#endif

//video
#define	VIDEO_OUT_PIN	31
#define PORT_VID	PORTA
#define	DDR_VID		DDRA
#define VID_PIN		7
//sync
#define	SYNC_OUT_PIN	13
#define PORT_SYNC	PORTD
#define DDR_SYNC	DDRD
#define SYNC_PIN	5
//sound
#define	SOUND_OUT_PIN	14
#define PORT_SND	PORTD
#define DDR_SND		DDRD
#define	SND_PIN		6

#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#if defined(__AVR_ATmega8__)
#define AVR_NAME	"ATmega8"
#define AVR_NICKNAME	"8"
#elif defined(__AVR_ATmega88__)
#define AVR_NAME	"ATmega88"
#define AVR_NICKNAME	"88"
#elif defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168__)
#define AVR_NAME	"ATmega168"
#define AVR_NICKNAME	"168"
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#define AVR_NAME	"ATmega328"
#define AVR_NICKNAME	"328"
#endif

#if VIDEO_LITTLE_ENDIAN	// send bit 0 first via PORTC0 (aka A0)
//video
#define	VIDEO_OUT_PIN	A0
#define PORT_VID	PORTC
#define	DDR_VID		DDRC
#define	VID_PIN		0
//sync
#define	SYNC_OUT_PIN	9
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define SYNC_PIN	1
//sound
#define	SOUND_OUT_PIN	3
#define PORT_SND	PORTD
#define DDR_SND		DDRD
#define	SND_PIN		3

#else					// normal TVout style
//video
#define	VIDEO_OUT_PIN	7
#define PORT_VID	PORTD
#define	DDR_VID		DDRD
#define	VID_PIN		7
//sync
#define	SYNC_OUT_PIN	9
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define SYNC_PIN	1
//sound
#define	SOUND_OUT_PIN	11
#define PORT_SND	PORTB
#define DDR_SND		DDRB
#define	SND_PIN		3
#endif

#elif defined (__AVR_AT90USB1286__)
#define AVR_NAME	"AT90USB1286"
#define AVR_NICKNAME	"90U1286"
//video
#define	VIDEO_OUT_PIN	45
#define PORT_VID	PORTF
#define	DDR_VID		DDRF
#define	VID_PIN		7
//sync
#define	SYNC_OUT_PIN	25
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define SYNC_PIN	5
//sound
#define	SOUND_OUT_PIN	24
#define PORT_SND	PORTB
#define DDR_SND		DDRB
#define	SND_PIN		4

#elif defined (__AVR_ATmega32U4__)
#define AVR_NAME	"ATmega32U4"
#define AVR_NICKNAME	"32U4"
//video
#define	VIDEO_OUT_PIN	5
#define PORT_VID	PORTD
#define	DDR_VID		DDRD
#define	VID_PIN		7
//sync
#define	SYNC_OUT_PIN	9
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define SYNC_PIN	5
//sound
#define	SOUND_OUT_PIN	5
#define PORT_SND	PORTC	// uses OC3A, since no OC2A
#define DDR_SND		DDRC
#define	SND_PIN		6

#elif defined (__AVR_ATtiny45__)
#define VIDEO_LITTLE_ENDIAN	1
#define AVR_NAME	"ATtiny45"
#define AVR_NICKNAME	"45"
//video
#define	VIDEO_OUT_PIN	0
#define PORT_VID	PORTB
#define	DDR_VID		DDRB
#define	VID_PIN		0
//sync
#define	SYNC_OUT_PIN	9
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define SYNC_PIN	5
//sound
#define	SOUND_OUT_PIN	5
#define PORT_SND	PORTC	// uses OC3A, since no OC2A
#define DDR_SND		DDRC
#define	SND_PIN		6

#else
#error Sorry, unsupported CPU type or AVR varient
#endif
#endif // HARDWARE_SETUP_H

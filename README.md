# TVOutGameToolKit - WIP NTSC game video library for AVR 328P

The TVGTK library was derived (i.e., hacked mercilessly)
from tvout-arduino (see https://github.com/Avamander/arduino-tvout)
to see what kind of tiled rendering (aka character graphics)
the Arduino is capable of.  Because of this, I have kept Myles Metzer's
copyright and license intact (even if not too much original code is left).
I would like to thank Myles Metzer for releasing his excellent library as
open-source allowing me to do this (as it is highly unlikely I would have
been able to do this without being able to examine and experiment with his
library - and knowing it is even possible to generate video this way).

This has been banging around in my home Perforce for years, so I am tossing it out onto GitHub by request (totally messy WIP).  It sure was a fun project to get very familiar with AVR assembly (and cycle times).

*WARNING:* This project contains severe "abuse" of GCC AVR inline assembly and the preprocessor.  For amusement only.

Last tested to build and run with Arduino IDE 1.8.13 on macOS (but should be fine on most 1.x versions, not sure about 2.x due to linker changes).

This repo also includes some hack-tastic utilities to crunch BMP files into "sprites" and tiles (intestinal fortitude recommended).

The AVR hardware is configured by default to run on the [Hackvision](https://nootropicdesign.com/hackvision/) platform, but can easily run on standard Arduino Uno with a few resistors, a capacitor (for audio) and buttons (see TVout circuit or Hackvision design).

As it stands it is the skeleton for a vertically scrolling game similar to Scramble, but mostly hijacked into a sprite test.  The "numbers" at the bottom represent tiles used for sprites, max tiles used for sprites, sprites "killed" (due to running out of tiles) and "skip" is frams skipped (with a stat for current CPU used).  If you have buttons hooked up like Hackvision, you can move the larger HaD logo around.

Tech stats are something like 20x20 8x8 tiled B & W NTSC (or PAL) video, with 128 tile definitions in flash, and 128 in RAM (along with the tile map).  The code then re-defines RAM tiles on the fly to create the illusion of sprites floating over the tiles.  The video is generated in the background via interrupts using about ~80% of AVR CPU power (but plenty left for a simple game).  Versions of this support horizintal and vertical scrolling as well as other resolutions (however, this code is mostly cut down for game development purposes).  Note that there is very little RAM free running this (so mostly use global variables be careful of the stack).

The (fake) sprites have four "colors": transparent, black, white, and invert.  

The code also supports 3 voice wavetable audio mixed together with PWM output (however not hooked up well in this demo, so mostly a constant droning sound).

-Xark
https://hackaday.io/Xark

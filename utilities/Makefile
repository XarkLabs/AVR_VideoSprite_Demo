#
# Simple makefile for TVoutGameKit utilities
#
all: crunch_tileset crunch_sprites


crunch_tileset:	crunch_tileset_from_bmp.c
	$(CC) -Wall -Os -o $@ $<

crunch_sprites:	crunch_sprites_from_bmp.c
	$(CC) -Wall -Os -o $@ $<

clean:
	$(RM) crunch_tileset crunch_tileset.exe crunch_sprites crunch_sprites.exe



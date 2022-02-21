// Hacktastic Q & D BMP -> TVGameKit sprite cruncher

#if !defined(ARDUINO)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#if defined(_MSC_VER)
#define strdup	_strdup
#endif

uint8_t bmp_header[54];
uint32_t pixel_offset;
uint32_t dib_size;
int32_t bmp_width;
int32_t bmp_height;
int16_t bmp_bpp;
uint32_t bmp_line;
uint8_t *RGB24_bitmap;

#define MAX_SPRITES	256

struct sprite_info
{
	char *name;
	int32_t width;
	int32_t height;
	int32_t byte_width;
	uint8_t *mono_bitmap;
	uint8_t *mask_bitmap;
};

int32_t	num_sprites;
struct sprite_info sprite[MAX_SPRITES];

// data = (data & mask) ^ mono;
//
// mask mono 
//	0	0	= black
//	0	1	= white
//	1	0	= transparent
//	1	1	= xor
//

uint8_t human_readable = 0;
uint8_t verbose = 0;

uint8_t preamble_done = 0;

int main(int argc, char* argv[])
{
	int32_t s, i, j, x, y, size;
	int32_t arg;
	struct sprite_info *spr = NULL;
	
	const char* bmp_name;
	FILE* f = NULL;

	for (arg = 1; arg < argc; arg++)
	{
		spr = &sprite[num_sprites];
		
		if (argv[arg][0] == '-')
		{
			char *namestr = NULL;
			char c = argv[arg][1];
			if (c >= 'A' && c <= 'Z')
			c += 32;
			switch(c)
			{
			case 'v':
				verbose = (verbose + 1) & 0x3;
				break;

			case 'h':
				human_readable ^= 1;
				break;

			case 'n':
				namestr = &argv[arg][2];
				
				if (*namestr == 0 && arg+1 < argc)
				namestr = argv[++arg];
				
				
				if (*namestr == 0)
				{
					fprintf(stderr, "Need symbol name after -n.\n");
					exit(5);
				}

				if (spr->name)
				free(spr->name);
				spr->name = strdup(namestr);
				
				break;

			default:
				printf("Usage: crunch_sprite [options ...] <input BMP ...> [-n <sprite name>]\n");
				printf("\n");
				printf(" -n <name>	- Name of next tilemap or tileset font (or BMP name used)\n");
				printf(" -v	 - Verbose (repeat up to three times to increase)\n");
				printf("\n");
				printf("Output will go to stdout, so use \"> output\" to redirect to file.\n");
				exit(1);
				
				break;
			}
			
			continue;
		}
		
		if (human_readable && !preamble_done)
		{
			printf("// Some macros and enums needed to help the tiles more human readable/editable\n");
			printf("#define _(x)	B ## x\n");
			printf("#if !VIDEO_LITTLE_ENDIAN	// normal (bit 7 is leftmost, so uses port bit 7)\n");
			printf("enum {\n");
			printf("	B________=0x00,B_______W=0x01,B______W_=0x02,B______WW=0x03,B_____W__=0x04,B_____W_W=0x05,B_____WW_=0x06,B_____WWW=0x07,B____W___=0x08,B____W__W=0x09,B____W_W_=0x0a,B____W_WW=0x0b,B____WW__=0x0c,B____WW_W=0x0d,B____WWW_=0x0e,B____WWWW=0x0f,\n");
			printf("	B___W____=0x10,B___W___W=0x11,B___W__W_=0x12,B___W__WW=0x13,B___W_W__=0x14,B___W_W_W=0x15,B___W_WW_=0x16,B___W_WWW=0x17,B___WW___=0x18,B___WW__W=0x19,B___WW_W_=0x1a,B___WW_WW=0x1b,B___WWW__=0x1c,B___WWW_W=0x1d,B___WWWW_=0x1e,B___WWWWW=0x1f,\n");
			printf("	B__W_____=0x20,B__W____W=0x21,B__W___W_=0x22,B__W___WW=0x23,B__W__W__=0x24,B__W__W_W=0x25,B__W__WW_=0x26,B__W__WWW=0x27,B__W_W___=0x28,B__W_W__W=0x29,B__W_W_W_=0x2a,B__W_W_WW=0x2b,B__W_WW__=0x2c,B__W_WW_W=0x2d,B__W_WWW_=0x2e,B__W_WWWW=0x2f,\n");
			printf("	B__WW____=0x30,B__WW___W=0x31,B__WW__W_=0x32,B__WW__WW=0x33,B__WW_W__=0x34,B__WW_W_W=0x35,B__WW_WW_=0x36,B__WW_WWW=0x37,B__WWW___=0x38,B__WWW__W=0x39,B__WWW_W_=0x3a,B__WWW_WW=0x3b,B__WWWW__=0x3c,B__WWWW_W=0x3d,B__WWWWW_=0x3e,B__WWWWWW=0x3f,\n");
			printf("	B_W______=0x40,B_W_____W=0x41,B_W____W_=0x42,B_W____WW=0x43,B_W___W__=0x44,B_W___W_W=0x45,B_W___WW_=0x46,B_W___WWW=0x47,B_W__W___=0x48,B_W__W__W=0x49,B_W__W_W_=0x4a,B_W__W_WW=0x4b,B_W__WW__=0x4c,B_W__WW_W=0x4d,B_W__WWW_=0x4e,B_W__WWWW=0x4f,\n");
			printf("	B_W_W____=0x50,B_W_W___W=0x51,B_W_W__W_=0x52,B_W_W__WW=0x53,B_W_W_W__=0x54,B_W_W_W_W=0x55,B_W_W_WW_=0x56,B_W_W_WWW=0x57,B_W_WW___=0x58,B_W_WW__W=0x59,B_W_WW_W_=0x5a,B_W_WW_WW=0x5b,B_W_WWW__=0x5c,B_W_WWW_W=0x5d,B_W_WWWW_=0x5e,B_W_WWWWW=0x5f,\n");
			printf("	B_WW_____=0x60,B_WW____W=0x61,B_WW___W_=0x62,B_WW___WW=0x63,B_WW__W__=0x64,B_WW__W_W=0x65,B_WW__WW_=0x66,B_WW__WWW=0x67,B_WW_W___=0x68,B_WW_W__W=0x69,B_WW_W_W_=0x6a,B_WW_W_WW=0x6b,B_WW_WW__=0x6c,B_WW_WW_W=0x6d,B_WW_WWW_=0x6e,B_WW_WWWW=0x6f,\n");
			printf("	B_WWW____=0x70,B_WWW___W=0x71,B_WWW__W_=0x72,B_WWW__WW=0x73,B_WWW_W__=0x74,B_WWW_W_W=0x75,B_WWW_WW_=0x76,B_WWW_WWW=0x77,B_WWWW___=0x78,B_WWWW__W=0x79,B_WWWW_W_=0x7a,B_WWWW_WW=0x7b,B_WWWWW__=0x7c,B_WWWWW_W=0x7d,B_WWWWWW_=0x7e,B_WWWWWWW=0x7f,\n");
			printf("	BW_______=0x80,BW______W=0x81,BW_____W_=0x82,BW_____WW=0x83,BW____W__=0x84,BW____W_W=0x85,BW____WW_=0x86,BW____WWW=0x87,BW___W___=0x88,BW___W__W=0x89,BW___W_W_=0x8a,BW___W_WW=0x8b,BW___WW__=0x8c,BW___WW_W=0x8d,BW___WWW_=0x8e,BW___WWWW=0x8f,\n");
			printf("	BW__W____=0x90,BW__W___W=0x91,BW__W__W_=0x92,BW__W__WW=0x93,BW__W_W__=0x94,BW__W_W_W=0x95,BW__W_WW_=0x96,BW__W_WWW=0x97,BW__WW___=0x98,BW__WW__W=0x99,BW__WW_W_=0x9a,BW__WW_WW=0x9b,BW__WWW__=0x9c,BW__WWW_W=0x9d,BW__WWWW_=0x9e,BW__WWWWW=0x9f,\n");
			printf("	BW_W_____=0xa0,BW_W____W=0xa1,BW_W___W_=0xa2,BW_W___WW=0xa3,BW_W__W__=0xa4,BW_W__W_W=0xa5,BW_W__WW_=0xa6,BW_W__WWW=0xa7,BW_W_W___=0xa8,BW_W_W__W=0xa9,BW_W_W_W_=0xaa,BW_W_W_WW=0xab,BW_W_WW__=0xac,BW_W_WW_W=0xad,BW_W_WWW_=0xae,BW_W_WWWW=0xaf,\n");
			printf("	BW_WW____=0xb0,BW_WW___W=0xb1,BW_WW__W_=0xb2,BW_WW__WW=0xb3,BW_WW_W__=0xb4,BW_WW_W_W=0xb5,BW_WW_WW_=0xb6,BW_WW_WWW=0xb7,BW_WWW___=0xb8,BW_WWW__W=0xb9,BW_WWW_W_=0xba,BW_WWW_WW=0xbb,BW_WWWW__=0xbc,BW_WWWW_W=0xbd,BW_WWWWW_=0xbe,BW_WWWWWW=0xbf,\n");
			printf("	BWW______=0xc0,BWW_____W=0xc1,BWW____W_=0xc2,BWW____WW=0xc3,BWW___W__=0xc4,BWW___W_W=0xc5,BWW___WW_=0xc6,BWW___WWW=0xc7,BWW__W___=0xc8,BWW__W__W=0xc9,BWW__W_W_=0xca,BWW__W_WW=0xcb,BWW__WW__=0xcc,BWW__WW_W=0xcd,BWW__WWW_=0xce,BWW__WWWW=0xcf,\n");
			printf("	BWW_W____=0xd0,BWW_W___W=0xd1,BWW_W__W_=0xd2,BWW_W__WW=0xd3,BWW_W_W__=0xd4,BWW_W_W_W=0xd5,BWW_W_WW_=0xd6,BWW_W_WWW=0xd7,BWW_WW___=0xd8,BWW_WW__W=0xd9,BWW_WW_W_=0xda,BWW_WW_WW=0xdb,BWW_WWW__=0xdc,BWW_WWW_W=0xdd,BWW_WWWW_=0xde,BWW_WWWWW=0xdf,\n");
			printf("	BWWW_____=0xe0,BWWW____W=0xe1,BWWW___W_=0xe2,BWWW___WW=0xe3,BWWW__W__=0xe4,BWWW__W_W=0xe5,BWWW__WW_=0xe6,BWWW__WWW=0xe7,BWWW_W___=0xe8,BWWW_W__W=0xe9,BWWW_W_W_=0xea,BWWW_W_WW=0xeb,BWWW_WW__=0xec,BWWW_WW_W=0xed,BWWW_WWW_=0xee,BWWW_WWWW=0xef,\n");
			printf("	BWWWW____=0xf0,BWWWW___W=0xf1,BWWWW__W_=0xf2,BWWWW__WW=0xf3,BWWWW_W__=0xf4,BWWWW_W_W=0xf5,BWWWW_WW_=0xf6,BWWWW_WWW=0xf7,BWWWWW___=0xf8,BWWWWW__W=0xf9,BWWWWW_W_=0xfa,BWWWWW_WW=0xfb,BWWWWWW__=0xfc,BWWWWWW_W=0xfd,BWWWWWWW_=0xfe,BWWWWWWWW=0xff,\n");
			printf("\n");
			printf("};\n");
			printf("#else 				// backwards (bit 0 to the left, so uses port bit 0)\n");
			printf("enum {\n");
			printf("	B________=0x00,BW_______=0x01,B_W______=0x02,BWW______=0x03,B__W_____=0x04,BW_W_____=0x05,B_WW_____=0x06,BWWW_____=0x07,B___W____=0x08,BW__W____=0x09,B_W_W____=0x0a,BWW_W____=0x0b,B__WW____=0x0c,BW_WW____=0x0d,B_WWW____=0x0e,BWWWW____=0x0f,\n");
			printf("	B____W___=0x10,BW___W___=0x11,B_W__W___=0x12,BWW__W___=0x13,B__W_W___=0x14,BW_W_W___=0x15,B_WW_W___=0x16,BWWW_W___=0x17,B___WW___=0x18,BW__WW___=0x19,B_W_WW___=0x1a,BWW_WW___=0x1b,B__WWW___=0x1c,BW_WWW___=0x1d,B_WWWW___=0x1e,BWWWWW___=0x1f,\n");
			printf("	B_____W__=0x20,BW____W__=0x21,B_W___W__=0x22,BWW___W__=0x23,B__W__W__=0x24,BW_W__W__=0x25,B_WW__W__=0x26,BWWW__W__=0x27,B___W_W__=0x28,BW__W_W__=0x29,B_W_W_W__=0x2a,BWW_W_W__=0x2b,B__WW_W__=0x2c,BW_WW_W__=0x2d,B_WWW_W__=0x2e,BWWWW_W__=0x2f,\n");
			printf("	B____WW__=0x30,BW___WW__=0x31,B_W__WW__=0x32,BWW__WW__=0x33,B__W_WW__=0x34,BW_W_WW__=0x35,B_WW_WW__=0x36,BWWW_WW__=0x37,B___WWW__=0x38,BW__WWW__=0x39,B_W_WWW__=0x3a,BWW_WWW__=0x3b,B__WWWW__=0x3c,BW_WWWW__=0x3d,B_WWWWW__=0x3e,BWWWWWW__=0x3f,\n");
			printf("	B______W_=0x40,BW_____W_=0x41,B_W____W_=0x42,BWW____W_=0x43,B__W___W_=0x44,BW_W___W_=0x45,B_WW___W_=0x46,BWWW___W_=0x47,B___W__W_=0x48,BW__W__W_=0x49,B_W_W__W_=0x4a,BWW_W__W_=0x4b,B__WW__W_=0x4c,BW_WW__W_=0x4d,B_WWW__W_=0x4e,BWWWW__W_=0x4f,\n");
			printf("	B____W_W_=0x50,BW___W_W_=0x51,B_W__W_W_=0x52,BWW__W_W_=0x53,B__W_W_W_=0x54,BW_W_W_W_=0x55,B_WW_W_W_=0x56,BWWW_W_W_=0x57,B___WW_W_=0x58,BW__WW_W_=0x59,B_W_WW_W_=0x5a,BWW_WW_W_=0x5b,B__WWW_W_=0x5c,BW_WWW_W_=0x5d,B_WWWW_W_=0x5e,BWWWWW_W_=0x5f,\n");
			printf("	B_____WW_=0x60,BW____WW_=0x61,B_W___WW_=0x62,BWW___WW_=0x63,B__W__WW_=0x64,BW_W__WW_=0x65,B_WW__WW_=0x66,BWWW__WW_=0x67,B___W_WW_=0x68,BW__W_WW_=0x69,B_W_W_WW_=0x6a,BWW_W_WW_=0x6b,B__WW_WW_=0x6c,BW_WW_WW_=0x6d,B_WWW_WW_=0x6e,BWWWW_WW_=0x6f,\n");
			printf("	B____WWW_=0x70,BW___WWW_=0x71,B_W__WWW_=0x72,BWW__WWW_=0x73,B__W_WWW_=0x74,BW_W_WWW_=0x75,B_WW_WWW_=0x76,BWWW_WWW_=0x77,B___WWWW_=0x78,BW__WWWW_=0x79,B_W_WWWW_=0x7a,BWW_WWWW_=0x7b,B__WWWWW_=0x7c,BW_WWWWW_=0x7d,B_WWWWWW_=0x7e,BWWWWWWW_=0x7f,\n");
			printf("	B_______W=0x80,BW______W=0x81,B_W_____W=0x82,BWW_____W=0x83,B__W____W=0x84,BW_W____W=0x85,B_WW____W=0x86,BWWW____W=0x87,B___W___W=0x88,BW__W___W=0x89,B_W_W___W=0x8a,BWW_W___W=0x8b,B__WW___W=0x8c,BW_WW___W=0x8d,B_WWW___W=0x8e,BWWWW___W=0x8f,\n");
			printf("	B____W__W=0x90,BW___W__W=0x91,B_W__W__W=0x92,BWW__W__W=0x93,B__W_W__W=0x94,BW_W_W__W=0x95,B_WW_W__W=0x96,BWWW_W__W=0x97,B___WW__W=0x98,BW__WW__W=0x99,B_W_WW__W=0x9a,BWW_WW__W=0x9b,B__WWW__W=0x9c,BW_WWW__W=0x9d,B_WWWW__W=0x9e,BWWWWW__W=0x9f,\n");
			printf("	B_____W_W=0xa0,BW____W_W=0xa1,B_W___W_W=0xa2,BWW___W_W=0xa3,B__W__W_W=0xa4,BW_W__W_W=0xa5,B_WW__W_W=0xa6,BWWW__W_W=0xa7,B___W_W_W=0xa8,BW__W_W_W=0xa9,B_W_W_W_W=0xaa,BWW_W_W_W=0xab,B__WW_W_W=0xac,BW_WW_W_W=0xad,B_WWW_W_W=0xae,BWWWW_W_W=0xaf,\n");
			printf("	B____WW_W=0xb0,BW___WW_W=0xb1,B_W__WW_W=0xb2,BWW__WW_W=0xb3,B__W_WW_W=0xb4,BW_W_WW_W=0xb5,B_WW_WW_W=0xb6,BWWW_WW_W=0xb7,B___WWW_W=0xb8,BW__WWW_W=0xb9,B_W_WWW_W=0xba,BWW_WWW_W=0xbb,B__WWWW_W=0xbc,BW_WWWW_W=0xbd,B_WWWWW_W=0xbe,BWWWWWW_W=0xbf,\n");
			printf("	B______WW=0xc0,BW_____WW=0xc1,B_W____WW=0xc2,BWW____WW=0xc3,B__W___WW=0xc4,BW_W___WW=0xc5,B_WW___WW=0xc6,BWWW___WW=0xc7,B___W__WW=0xc8,BW__W__WW=0xc9,B_W_W__WW=0xca,BWW_W__WW=0xcb,B__WW__WW=0xcc,BW_WW__WW=0xcd,B_WWW__WW=0xce,BWWWW__WW=0xcf,\n");
			printf("	B____W_WW=0xd0,BW___W_WW=0xd1,B_W__W_WW=0xd2,BWW__W_WW=0xd3,B__W_W_WW=0xd4,BW_W_W_WW=0xd5,B_WW_W_WW=0xd6,BWWW_W_WW=0xd7,B___WW_WW=0xd8,BW__WW_WW=0xd9,B_W_WW_WW=0xda,BWW_WW_WW=0xdb,B__WWW_WW=0xdc,BW_WWW_WW=0xdd,B_WWWW_WW=0xde,BWWWWW_WW=0xdf,\n");
			printf("	B_____WWW=0xe0,BW____WWW=0xe1,B_W___WWW=0xe2,BWW___WWW=0xe3,B__W__WWW=0xe4,BW_W__WWW=0xe5,B_WW__WWW=0xe6,BWWW__WWW=0xe7,B___W_WWW=0xe8,BW__W_WWW=0xe9,B_W_W_WWW=0xea,BWW_W_WWW=0xeb,B__WW_WWW=0xec,BW_WW_WWW=0xed,B_WWW_WWW=0xee,BWWWW_WWW=0xef,\n");
			printf("	B____WWWW=0xf0,BW___WWWW=0xf1,B_W__WWWW=0xf2,BWW__WWWW=0xf3,B__W_WWWW=0xf4,BW_W_WWWW=0xf5,B_WW_WWWW=0xf6,BWWW_WWWW=0xf7,B___WWWWW=0xf8,BW__WWWWW=0xf9,B_W_WWWWW=0xfa,BWW_WWWWW=0xfb,B__WWWWWW=0xfc,BW_WWWWWW=0xfd,B_WWWWWWW=0xfe,BWWWWWWWW=0xff,\n");
			printf("\n");
			printf("};\n");
			printf("#endif\n");
			
			preamble_done = 1;
		}
		
		bmp_name = argv[arg];
		
		if ((f = fopen(bmp_name, "rb")) == NULL)
		{
			fprintf(stderr, "Error opening input BMP file \"%s\".\n", bmp_name);
			exit(5);
		}
		
		if (fread(bmp_header, sizeof (unsigned char), 54, f) != 54)
		{
			fprintf(stderr, "Error reading BMP header from \"%s\".\n", bmp_name);
			exit(5);
		}
		
		if (bmp_header[0] != 'B' || bmp_header[1] != 'M')
		{
			fprintf(stderr, "Error not valid BMP format \"%s\".\n", bmp_name);
			exit(5);
		}
		
		pixel_offset = bmp_header[10] | (bmp_header[11]<<8) | (bmp_header[12]<<16) | (bmp_header[13]<<24);
		dib_size = bmp_header[14] | (bmp_header[15]<<8) | (bmp_header[16]<<16) | (bmp_header[17]<<24);
		
		if (dib_size != 40)
		{
			fprintf(stderr, "Error unsupported BMP format DIB=%d \"%s\".\n", dib_size, bmp_name);
			exit(5);
		}
		
		// extract image height and width from header
		bmp_width = bmp_header[18] | (bmp_header[19]<<8) | (bmp_header[20]<<16) | (bmp_header[21]<<24);
		bmp_height = bmp_header[22] | (bmp_header[23]<<8) | (bmp_header[24]<<16) | (bmp_header[25]<<24);
		if (bmp_width < 0)
		bmp_width = -bmp_width;
		if (bmp_height < 0)
		bmp_height = -bmp_height;
		bmp_bpp = bmp_header[28] | (bmp_header[29]<<8);
		
		if (bmp_width < 1 || bmp_width > 4096 || bmp_height < 1 || bmp_height > 4096 || bmp_bpp != 24)
		{
			fprintf(stderr, "BMP \"%s\" size or depth fails sanity check (%d x %d bpp=%d).\n", bmp_name, bmp_width, bmp_height, bmp_bpp);
			exit(5);
		}
		
		bmp_line = (((bmp_width*bmp_bpp) + 31) / 32) * 4;
		size = bmp_line * bmp_height;

		printf("\n// File: \"%s\": %d x %d (%d bpp, pixel offset %d, bytes per line %d, bitmap size %d).\n", bmp_name, bmp_width, bmp_height, bmp_bpp, pixel_offset, bmp_line, size);

		RGB24_bitmap = malloc(size);
		if (!RGB24_bitmap)
		{
			fprintf(stderr, "Failed allocating %d bytes.\n", size);
			exit(5);
		}
		memset(RGB24_bitmap, 0, size);
		
		// seek to bitmap pixels and read them in
		if (fseek(f, pixel_offset, SEEK_SET) != 0 || fread(RGB24_bitmap, sizeof (unsigned char), size, f) != size)
		{
			fprintf(stderr, "Error reading %d bytes of BMP data from \"%s\".\n", size, bmp_name);
			exit(5);
		}
		fclose(f);
		f = NULL;
		
		// convert bitmap into sprite and fill out sprite_info
		
		// give it a symbol name if not already specified
		if (spr->name == NULL)
		{
			char str_temp[256];
			const char *n = bmp_name;

			// find start of filename after path
			if (strrchr(n, '/'))
			n = strrchr(n, '/')+1;
			else if (strrchr(n, '\\'))
			n = strrchr(n, '\\')+1;
			else if (strrchr(n, ':'))
			n = strrchr(n, ':')+1;
			
			strncpy(str_temp, n, sizeof (str_temp)-1);
			
			// remove extension
			if (strrchr(str_temp, '.'))
			*strrchr(str_temp, '.') = 0;

			spr->name = strdup(str_temp);
		}

		spr->width = bmp_width;
		spr->byte_width = ((spr->width + 7)/8)+1;
		spr->height = bmp_height;
		size = spr->byte_width * spr->height;
		spr->mono_bitmap = malloc(size);
		if (!spr->mono_bitmap)
		{
			fprintf(stderr, "Failed allocating %d bytes for mono bitmap.\n", size);
			exit(5);
		}
		memset(spr->mono_bitmap, 0, size);

		spr->mask_bitmap = malloc(size);
		if (!spr->mask_bitmap)
		{
			fprintf(stderr, "Failed allocating %d bytes for mask bitmap.\n", size);
			exit(5);
		}
		memset(spr->mask_bitmap, 0xff, size);

		printf("// BMP #%d \"%s\" (%d x %d with '.'=trans, ' '=black, 'X'=XOR, 'W'=white):\n\n", num_sprites, spr->name, spr->width, spr->height);
		if (verbose > 1)
		{
			printf("// *");
			for (x = 0; x < bmp_width; x++)
				printf("%c", (x & 0x7) == 0x0 ? '+' : '-');
			printf("*\n");
		}
			
		for (y = 0; y < bmp_height; y++)
		{
			if (verbose > 1)
			printf("// %c", (y & 0x7) == 0x0 ? '+' : '|');
			for (x = 0; x < bmp_width; x++)
			{
				uint8_t* rgb = &RGB24_bitmap[((bmp_height-1-y) * bmp_line) + (x * 3)];
				uint8_t c = (rgb[2] >= 0x40 ? 0x4 : 0x0) | (rgb[1] >= 0x40 ? 0x2 : 0x0) | (rgb[0] >= 0x40 ? 0x1 : 0x0);	// 0b00000rgb
				uint8_t b = 0;
				uint8_t m = 1;
				uint8_t t = '?';
				
				switch (c)
				{
				case 0x0:
					m = 1;
					b = 0;
					t = '.';	// transparent
					break;
				case 0x1:
					m = 0;
					b = 0;
					t = ' ';	// black
					break;
				case 0x4:
					m = 1;
					b = 1;
					t = 'X';	// XOR
					break;
				case 0x7:
					m = 0;
					b = 1;
					t = 'W';	// white
					break;
				}
				if (verbose > 1)
					printf("%c", t);
				
				if (b)
					spr->mono_bitmap[(y * spr->byte_width) + (x/8)] |= 1 << (7-(x & 0x7));

				if (!m)
					spr->mask_bitmap[(y * spr->byte_width) + (x/8)] &= ~(1 << (7-(x & 0x7)));
			}
			if (verbose > 1)
				printf("%c\n", (y & 0x7) == 0x0 ? '+' : '|');
		}
		if (verbose > 1)
		{
			printf("// *");
			for (x = 0; x < bmp_width; x++)
				printf("%c", (x & 0x7) == 0x0 ? '+' : '-');
			printf("*\n");
		}

		free(RGB24_bitmap);
		RGB24_bitmap = NULL;
		
		if (verbose > 1)
			printf("\n");

		num_sprites++;
	}

	for (i = 0; i < num_sprites; i++)
	{
		spr = &sprite[i];
		printf("// #%d (0x%02x) - \"%s\" (%d x %d)\n", i, i, spr->name, spr->width, spr->height);
		
		// make sure name is a legal C/C++ identifier
		for (j = 0; j < (int)strlen(spr->name); j++)
		{
			char c = spr->name[j];
			if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (i == 0 || c < '0' || c > '9'))
				spr->name[j] = '_';
		}
		
		
		// generate pre-shifted bitmaps
		for (s = 0; s < 8; s++)
		{
			printf("const uint8_t %s_bmap_%d[%d*%d*2] PROGMEM =\n", spr->name, s, (spr->width+s+7)/8, spr->height);
			printf("{\n");

			printf("// Bitmap shift %d (%d+%d x %d):\n", s, spr->width, s, spr->height);
			if (verbose > 0)
			{
				printf("// Mask bitmap (%d x %d):\n", spr->width, spr->height);
				for (y = 0; y < spr->height; y++)
				{
					printf("// ");
					for (x = 0; x < spr->width+s; x += 8)
					{
						uint8_t b = spr->mask_bitmap[(y * spr->byte_width) + (x/8)];
						printf("%s%s%s%s%s%s%s%s", b & 0x80 ? "#" : ".",
									   b & 0x40 ? "#" : ".",
									   b & 0x20 ? "#" : ".",
									   b & 0x10 ? "#" : ".",
									   b & 0x08 ? "#" : ".",
									   b & 0x04 ? "#" : ".",
									   b & 0x02 ? "#" : ".",
									   b & 0x01 ? "#" : ".");
					}
					printf("\n");
				}
				printf("// Monochrome bitmap (%d+%d x %d):\n", spr->width, s, spr->height);
				for (y = 0; y < spr->height; y++)
				{
					printf("// ");
					for (x = 0; x < spr->width+s; x += 8)
					{
						uint8_t b = spr->mono_bitmap[(y * spr->byte_width) + (x/8)];
						printf("%s%s%s%s%s%s%s%s", b & 0x80 ? "#" : ".",
									   b & 0x40 ? "#" : ".",
									   b & 0x20 ? "#" : ".",
									   b & 0x10 ? "#" : ".",
									   b & 0x08 ? "#" : ".",
									   b & 0x04 ? "#" : ".",
									   b & 0x02 ? "#" : ".",
									   b & 0x01 ? "#" : ".");
					}
					printf("\n");
				}
			}
			for (x = 0; x < spr->width+s; x += 8)
			{
				printf("\t");
				for (y = 0; y < spr->height; y++)
				{
					uint8_t b = spr->mono_bitmap[(y * spr->byte_width) + (x/8)];
					uint8_t m = spr->mask_bitmap[(y * spr->byte_width) + (x/8)];
					if (human_readable)
					{
						printf("_( %s%s%s%s%s%s%s%s ),", m & 0x80 ? "W" : "_",
										 m & 0x40 ? "W" : "_",
										 m & 0x20 ? "W" : "_",
										 m & 0x10 ? "W" : "_",
										 m & 0x08 ? "W" : "_",
										 m & 0x04 ? "W" : "_",
										 m & 0x02 ? "W" : "_",
										 m & 0x01 ? "W" : "_");
						printf("_( %s%s%s%s%s%s%s%s ),", b & 0x80 ? "W" : "_",
										 b & 0x40 ? "W" : "_",
										 b & 0x20 ? "W" : "_",
										 b & 0x10 ? "W" : "_",
										 b & 0x08 ? "W" : "_",
										 b & 0x04 ? "W" : "_",
										 b & 0x02 ? "W" : "_",
										 b & 0x01 ? "W" : "_");
					}
					else
					{
						printf("0x%02x,", m);
						printf("0x%02x,", b);
					}
				}
				printf("\n");
			}

			printf("};\n\n");

			// shift bits
			for (y = 0; y < spr->height; y++)
			{
				uint8_t c = 0;
				for (x = 0; x < spr->width+s+1; x += 8)
				{
					uint8_t b = spr->mono_bitmap[(y * spr->byte_width) + (x/8)];
					spr->mono_bitmap[(y * spr->byte_width) + (x/8)] = (c << 7) | (b >> 1);
					c = b & 1;
				}
			}
			
			for (y = 0; y < spr->height; y++)
			{
				uint8_t c = 1;
				for (x = 0; x < spr->width+s+1; x += 8)
				{
					uint8_t b = spr->mask_bitmap[(y * spr->byte_width) + (x/8)];
					spr->mask_bitmap[(y * spr->byte_width) + (x/8)] = (c << 7) | (b >> 1);
					c = b & 1;
				}
			}
		}
	}

	printf("sprite_rom sprite_def[%d] =\n", num_sprites+1);
	printf("{\n");
	for (i = 0; i < num_sprites; i++)
	{
		spr = &sprite[i];
		printf("	{\n");
		printf("		%d, %d,	// #%d \"%s\"\n", spr->width, spr->height, i, spr->name);
		printf("		{\n");
		for (j = 0; j < 8; j++)
			printf("			%s_bmap_%d,\n", spr->name, j);
		printf("		},\n");
		printf("	},\n");
	}
	printf("};\n");

	printf("enum\n");
	printf("{\n");
	for (i = 0; i < num_sprites; i++)
	{
		spr = &sprite[i];
		printf("	SPRITE_%s = %d,\n", spr->name, i+1);
	}
	printf("	SPRITE_NUM_IMAGES = %d\n", num_sprites);
	printf("};\n");

	
	if (human_readable)
		printf("#undef _\n");
	printf("// EOF\n");
	fprintf(stderr, "Done!\n");

	return 0;
}

#endif
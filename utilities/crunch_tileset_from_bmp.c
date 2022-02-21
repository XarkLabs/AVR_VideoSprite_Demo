// Hacktastic Q & D BMP -> TVGameKit tile-set cruncher

#if !defined(ARDUINO)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define TILE_X		8
#define TILE_Y		8
#define TILE_BYTES	(TILE_Y)
#define MAX_TILES	256

uint8_t bmp_header[54];
uint32_t pixel_offset;
uint32_t dib_size;
int32_t bmp_width;
int32_t bmp_height;
int16_t bmp_bpp;
uint32_t bmp_line;
uint32_t bmp_size;
uint8_t *RGB24_bitmap;
int32_t *y_bitmap;

int32_t width;
int32_t height;
uint8_t *mono_bitmap;
char tilesetname_str[64];
uint8_t *tileset;
uint8_t *tilemap;
uint8_t *curtile;

int32_t numtiles;

enum { GAMMA_MAP, LINEAR_MAP, IDENTITY_MAP };
uint8_t	mapping = GAMMA_MAP;
uint8_t ordered_dither = 0;
uint8_t permit_dupes = 0;
uint8_t font_only = 0;
uint8_t human_readable = 0;
uint8_t verbose = 0;
int32_t max_tiles = 256;

uint8_t premable_done = 0;

#define MIN_GRAY	0
#define MAX_GRAY	0x40000000
#define GRAY_THRESH	0x20000000

// sRGB luminance(Y) values
const float rY = 0.212655f;
const float gY = 0.715158f;
const float bY = 0.072187f;

// Inverse of sRGB "gamma" function. (approx 2.2)
float inv_gam_sRGB(int32_t ic)
{
    float c = ic/255.0f;
    if ( c <= 0.04045f )
        return c/12.92f;
    else 
        return powf(((c+0.055f)/(1.055f)),2.4f);
}

// sRGB "gamma" function (approx 2.2)
float gam_sRGB(float v)
{
    if(v<=0.0031308f)
        v *= 12.92f;
    else 
        v = 1.055f*powf(v,1.0f/2.4f)-0.055f;
    return v;
}

// GRAY VALUE with gamma correction ("brightness") 0.0 to 1.0
float gray_gamma(int32_t r, int32_t g, int32_t b)
{
    return gam_sRGB(
            rY*inv_gam_sRGB(r) +
            gY*inv_gam_sRGB(g) +
            bY*inv_gam_sRGB(b)
    );
}

// GRAY VALUE ("brightness") 0.0 to 1.0
float gray_linear(int32_t r, int32_t g, int32_t b)
{
    return  rY*(r/255.0f) +
            gY*(g/255.0f) +
            bY*(b/255.0f);
}

const char *charname(int c)
{
	static char str[32];
	switch (c)
	{
	case '\0':
		return "'\\0'";
	case '\a':
		return "'\\a'";
	case '\n':
		return "'\\n'";
	case '\t':
		return "'\\t'";
	case '\v':
		return "'\\v'";
	case '\b':
		return "'\\b'";
	case '\r':
		return "'\\r'";
	case '\f':
		return "'\\f'";
	case '\'':
		return "'\\\"'";
	case '\"':
		return "'\\''";
	case '\\':
		return "'\\\\'";

	default:
		if (c >= ' ' && c <= '~')
			sprintf(str, "'%c'", c);
		else
			sprintf(str, "'\\x%02x'", c);
	}

	return str;
}

int main(int argc, char* argv[])
{
	int32_t i, x, y;
	int32_t arg;
	int32_t newtiles;
	int32_t dupetiles;
	
	float min_l;
	float max_l;
	float avg_l;
	const char* bmp_name;
	char name_str[64] = { 0 };
	FILE* f = NULL;

	for (arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			char c = argv[arg][1];
			if (c >= 'A' && c <= 'Z')
				c += 32;
			switch(c)
			{
				case 'g':
					mapping = GAMMA_MAP;
					break;
				case 'l':
					mapping = LINEAR_MAP;
					break;
				case 'i':
					mapping = IDENTITY_MAP;
					break;
				
				case 'd':
					ordered_dither ^= 1;
					break;

				case 'p':
					permit_dupes ^= 1;
					break;

				case 'f':
					font_only ^= 1;
					break;

				case 'v':
					verbose = (verbose + 1) & 0x3;
					break;

				case 'h':
					human_readable ^= 1;
					break;

				case 'm':
				{
					char *numstr = &argv[arg][2];
					
					if (*numstr == 0 && arg+1 < argc)
						numstr = argv[++arg];
					
					if (*numstr == 0 || sscanf(numstr, "%d", &max_tiles) != 1 || max_tiles < 1 || max_tiles > 256)
					{
						fprintf(stderr, "Need maximum tiles after -m between 1 and 256.\n");
						exit(5);
					}

					break;
				}

				case 'n':
				{
					char *namestr = &argv[arg][2];
					
					if (*namestr == 0 && arg+1 < argc)
						namestr = argv[++arg];
						
					
					if (*namestr == 0)
					{
						fprintf(stderr, "Need symbol name after -n.\n");
						exit(5);
					}
					strncpy(name_str, namestr, sizeof(name_str)-1);
					strncpy(tilesetname_str, namestr, sizeof(tilesetname_str)-1);
					
					break;
				}

				default:
				{
					printf("Usage: crunch_tileset [options ...] <input BMP ...> [-n <tileset name>]\n");
					printf("\n");
					printf(" -m <1-255> - Maximum number of characters in tileset (default %d)\n", max_tiles);
					printf(" -n <name>  - Name of next tilemap or tileset font (or BMP name used)\n");
					printf(" -g         - Use normal gamma 2.2 when mapping RGB to luminance\n");
					printf(" -l         - Use linear gamma 1.0 when mapping RGB to luminance\n");
					printf(" -i         - Use identity mapping RGB to luminance (R+G+B/3)\n");
					printf(" -d         - Use ordered 2x2 dither instead of Floyd–Steinberg\n");
					printf("\n");
					printf("Output will go to stdout, so use \"> output\" to redirect to file.\n");
					exit(1);
					
					break;
				}
			}
		
			continue;
		}
		
		if (human_readable && !premable_done)
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
			
			premable_done = 1;
		}
		
		bmp_name = argv[arg];
		
		if ((f = fopen(bmp_name, "rb")) == NULL)
		{
			fprintf(stderr, "Error opening input BMP file \"%s\".\n", bmp_name);
			exit(5);
		}
		
		if (fread(bmp_header, sizeof(unsigned char), 54, f) != 54)
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
		
		if (bmp_width < TILE_X || bmp_width > 4096 || bmp_height < TILE_Y || bmp_height > 4096 || bmp_bpp != 24)
		{
			fprintf(stderr, "BMP \"%s\" size or depth fails sanity check (%d x %d bpp=%d).\n", bmp_name, bmp_width, bmp_height, bmp_bpp);
			exit(5);
		}
		
		bmp_line = (((bmp_width*bmp_bpp) + 31) / 32) * 4;
		printf("// \"%s\": %d x %d (%d bpp, pixel offset=%d, bytes/line=%d).\n", bmp_name, bmp_width, bmp_height, bmp_bpp, pixel_offset, bmp_line);

		bmp_size = bmp_line * bmp_height;
		RGB24_bitmap = malloc(bmp_size);
		if (!RGB24_bitmap)
		{
			fprintf(stderr, "Failed allocating %d bytes.\n", bmp_size);
			exit(5);
		}
		memset(RGB24_bitmap, 0, bmp_size);
		
		 // seek to bitmap pixels and read them in
		if (fseek(f, pixel_offset, SEEK_SET) != 0 || fread(RGB24_bitmap, sizeof(unsigned char), bmp_size, f) != bmp_size)
		{
			fprintf(stderr, "Error reading %d bytes of BMP data from \"%s\".\n", bmp_size, bmp_name);
			exit(5);
		}
		fclose(f);
		f = NULL;

		// give it a symbol name if not already specified
		if (name_str[0] == 0)
		{
			const char *n = bmp_name;

			if (strrchr(n, '/'))
				n = strrchr(n, '/')+1;
			else if (strrchr(n, '\\'))
				n = strrchr(n, '\\')+1;
			else if (strrchr(n, ':'))
				n = strrchr(n, ':')+1;
			
			strncpy(name_str, n, sizeof (name_str)-1);
			
			if (strrchr(name_str, '.'))
				*strrchr(name_str, '.') = 0;

			for (i = 0; i < (int)strlen(name_str); i++)
			{
				char c = name_str[i];
				if ((c < '0' && c > '9') &&
				    (c < 'A' && c > 'Z') &&
				    (c < 'a' && c > 'z'))
 				    name_str[i] = '_';
			}
		}
		
		// Round monochrome image size up to tile size boundaries
		width = (bmp_width + (TILE_X-1)) & ~(TILE_X-1);
		height = (bmp_height + (TILE_Y-1)) & ~(TILE_Y-1);
		
		mono_bitmap = malloc(width/8 * height);
		if (!mono_bitmap)
		{
			fprintf(stderr, "Failed allocating %d bytes.\n", width/8 * height);
			exit(5);
		}
		memset(mono_bitmap, 0, width/8 * height);

		y_bitmap = malloc(bmp_width * bmp_height * sizeof (int32_t));
		if (!y_bitmap)
		{
			fprintf(stderr, "Failed allocating %lu bytes.\n", bmp_width * bmp_height * sizeof (int32_t));
			exit(5);
		}
		memset(y_bitmap, 0, bmp_width * bmp_height * sizeof (int32_t));

		// convert to luminance
		min_l = 9999.0f;
		max_l = -9999.0f;
		avg_l = 0.0f;
		if (verbose > 1)
			printf("// Luminance map after %s map %d x %d (0-4 levels shown as \" .=LW\"):\n", mapping == GAMMA_MAP ? "gamma" : mapping == LINEAR_MAP ? "linear" : "identity", bmp_width, bmp_height);

		for (y = 0; y < bmp_height; y++)
		{
			if (verbose > 1)
				printf("// ");
			for (x = 0; x < bmp_width; x++)
			{
				float l = 0.0f;
				uint8_t* rgb = &RGB24_bitmap[((bmp_height-1-y) * bmp_line) + (x * 3)];

				if (mapping == GAMMA_MAP)
				{
					l = gray_gamma(rgb[2], rgb[1], rgb[0]);
				}
				else if (mapping == LINEAR_MAP)
				{
					l = gray_linear(rgb[2], rgb[1], rgb[0]);
				}
				else
				{
					l = (rgb[2] + rgb[1] + rgb[0]) / 3.0f;
				}
				if (l < 0.0f)
					l = 0.0f;
				else if (l > 1.0f)
					l = 1.0f;

				y_bitmap[(y * bmp_width) + x] = (int32_t)(l * MAX_GRAY);;

				if (min_l > l)
					min_l = l;
				if (max_l < l)
					max_l = l;
				avg_l += l;

				if (verbose > 1)
					printf("%c", " .=LW"[(int32_t)(l * 4.99f)]);
			}
			if (verbose > 1)
				printf("\n");
		}
		printf("// Luminance: minimum %03.03f / average %03.03f / maximum %03.03f\n", min_l, avg_l / (bmp_width * bmp_height), max_l);
		
		free(RGB24_bitmap);
		RGB24_bitmap = NULL;

		// Floyd–Steinberg dithering (from pseudo-code at https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering)
		if (!ordered_dither)
		{
			for (y = 0; y < bmp_height; y++)
			{
				if (y & 1)	// go backwards on odd lines to reduce "contour line" artifacts
				{
					for (x = bmp_width-1; x >= 0; x--)
					{
						int32_t oldpixel = y_bitmap[(y * bmp_width) + x];
						int32_t newpixel = oldpixel < GRAY_THRESH ? MIN_GRAY : MAX_GRAY;
						int32_t quant_error = oldpixel - newpixel;

						y_bitmap[(y * bmp_width) + x] = newpixel;
						if (x > 0)
							y_bitmap[(y * bmp_width) + x-1] += (int32_t)(quant_error * (7.0f/16.0f) + 0.5f);
						if (y+1 == bmp_height)
							continue;
						if (x+1 < bmp_width)
							y_bitmap[((y+1) * bmp_width) + x+1] += (int32_t)(quant_error * (3.0f/16.0f) + 0.5f);
						y_bitmap[((y+1) * bmp_width) + x] += (int32_t)(quant_error * (5.0f/16.0f) + 0.5f);
						if (x > 0)
							y_bitmap[((y+1) * bmp_width) + x-1] += (int32_t)(quant_error * (1.0f/16.0f) + 0.5f);
					}
				}
				else
				{
					for (x = 0; x < bmp_width; x++)
					{
						int32_t oldpixel = y_bitmap[(y * bmp_width) + x];
						int32_t newpixel = oldpixel < GRAY_THRESH ? MIN_GRAY : MAX_GRAY;
						int32_t quant_error = oldpixel - newpixel;

						y_bitmap[(y * bmp_width) + x] = newpixel;
						if (x+1 < bmp_width)
							y_bitmap[(y * bmp_width) + x+1] += (int32_t)(quant_error * (7.0f/16.0f) + 0.5f);
						if (y+1 == bmp_height)
							continue;
						if (x > 0)
							y_bitmap[((y+1) * bmp_width) + x-1] += (int32_t)(quant_error * (3.0f/16.0f) + 0.5f);
						y_bitmap[((y+1) * bmp_width) + x] += (int32_t)(quant_error * (5.0f/16.0f) + 0.5f);
						if (x+1 < bmp_width)
							y_bitmap[((y+1) * bmp_width) + x+1] += (int32_t)(quant_error * (1.0f/16.0f) + 0.5f);
					}
				}
			}
			printf("// Floyd-Steinberg dithering applied\n");
		}
		
		// convert to monochrome 1-bit per-pixel with ordered 2x2 dither pattern (no-op if already applied Floyd–Steinberg dithering)
		for (y = 0; y < bmp_height; y++)
		{
			for (x = 0; x < bmp_width; x++)
			{
				uint8_t set = 0;
				int32_t l = (int32_t) (y_bitmap[(y * bmp_width) + x] / (MAX_GRAY/5.0f));	// convert to 5 levels for 2x2 dither coverage

				if (l < 0)
					l = 0;
				else if (l > 4)
					l = 4;
				
				switch (l)
				{
					case 0:
						set = 0;
						break;
					case 1:
						set = !(x&1) && !(y&1);
						break;
					case 2:
						set = (!(x&1) && !(y&1)) || ((x&1) && (y&1));
						break;
					case 3:
						set = !((x&1) && (y&1));
						break;
					default:
						set = 1;
						break;
				}
				
				if (set)
				{
					mono_bitmap[(y * (width/8)) + (x/8)] |= 1 << (7-(x & 0x7));
				}
			}
			
		}
		printf("\n");

		free(y_bitmap);
		y_bitmap = NULL;
		
		if (verbose > 0)
		{
			printf("// Monochrome bitmap (%d x %d):\n", width, height);
			for (y = 0; y < height; y++)
			{
				printf("// ");
				for (x = 0; x < width; x+=8)
				{
					uint8_t b = mono_bitmap[((y * width) + x)/8];
					printf("%s%s%s%s%s%s%s%s",	b & 0x80 ? "#" : ".",
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

		// make tileset (only one)
		if (!tileset)
		{
			tileset = malloc(TILE_BYTES * (MAX_TILES + 1));
			if (!tileset)
			{
				fprintf(stderr, "Failed allocating %d bytes.\n", TILE_BYTES * (MAX_TILES + 1));
				exit(5);
			}
			memset(tileset, 0, TILE_BYTES * (MAX_TILES + 1));
			if (permit_dupes)
				numtiles = 0;	// leave tile 0 always blank (unless permitting dupes)
			else
				numtiles = 1;
		}

		// and tilemap for this BMP
		tilemap = malloc((width / TILE_X) * (height / TILE_Y));
		if (!tilemap)
		{
			fprintf(stderr, "Failed allocating %d bytes.\n", (width / TILE_X) * (height / TILE_Y));
			exit(5);
		}
		memset(tilemap, 0, (width / TILE_X) * (height / TILE_Y));
		
		newtiles = 0;
		dupetiles = 0;
		
		for (y = 0; y < height; y += TILE_Y)
		{
			printf("// ");
			for (x = 0; x < width; x += TILE_X)
			{
				curtile = &tileset[numtiles * TILE_BYTES];
				for (i = 0; i < TILE_Y; i++)
					curtile[i] = mono_bitmap[(((y+i) * width) + x)/8];
					
				// brute force check for dupe
				for (i = 0; i < numtiles; i++)
				{
					if (memcmp(&tileset[i * TILE_BYTES], curtile, TILE_BYTES) == 0)
						break;
				}
				

				if (i < numtiles)
				{
					dupetiles++;
				}

				if (permit_dupes)
				{
					i = (curtile - tileset) / TILE_Y;
				}

				if (i >= numtiles)
				{
					newtiles++;
					numtiles++;
					if (numtiles > MAX_TILES)
					{
						fprintf(stderr, "Error: Sorry, more then %d tiles limit\n", MAX_TILES);
						exit(5);
					}
					printf("+%02x ", (uint8_t)i);
				}
				else
					printf("=%02x ", (uint8_t)i);

				tilemap[((y / TILE_Y) * (width / TILE_X)) + (x / TILE_X)] = (uint8_t)i;
			}
			printf("\n");
		}
		printf("// Added %d new tiles with %d duplicates %s (total %d of %d)\n\n", newtiles, dupetiles, permit_dupes ? "permitted" : "combined", numtiles, max_tiles);

		free(mono_bitmap);
		mono_bitmap = NULL;
		
		if (numtiles > max_tiles)
		{
			fprintf(stderr, "Error: Sorry, more than %d tiles needed (%d used).\n", max_tiles, numtiles); 
			exit(5);
		}

		if (!font_only)
		{
			printf("// From \"%s\" (%d x %d tilemap)\n", bmp_name, width / TILE_X, height / TILE_Y);
			printf("const uint8_t %s_tiles[2+(%d*%d)] PROGMEM =\n", name_str, width / TILE_X, height/TILE_Y);
			printf("{\n\t%d, %d,	// width, height\n", width / TILE_X, height/TILE_Y);

			for (y = 0; y < (height/TILE_Y); y++)
			{
				printf("\t");
				for (x = 0; x < (width/TILE_X); x++)
				{
					printf("0x%02x, ", tilemap[(y * (width/TILE_X)) + x]);
				}
				printf("\n");
			}
			printf("};\n\n");
		}

		if (verbose > 2)
		{
			printf("// Reconstruction:\n");
			for (y = 0; y < height; y++)
			{
				printf("// ");
				for (x = 0; x < width; x += TILE_X)
				{
					uint8_t t = tilemap[((y / TILE_Y) * (width / TILE_X)) + (x / TILE_X)];
					uint8_t b = tileset[(t * TILE_BYTES) + (y & 0x7)];
					printf("%s%s%s%s%s%s%s%s",	b & 0x80 ? "W" : "_",
									b & 0x40 ? "W" : "_",
									b & 0x20 ? "W" : "_",
									b & 0x10 ? "W" : "_",
									b & 0x08 ? "W" : "_",
									b & 0x04 ? "W" : "_",
									b & 0x02 ? "W" : "_",
									b & 0x01 ? "W" : "_");
				}
				printf("\n");
			}
		}
	
		free(tilemap);
		tilemap = NULL;

		if (tilesetname_str[0] == 0)
			strncpy(tilesetname_str, name_str, sizeof (tilesetname_str)-1);
		memset(name_str, 0, sizeof (name_str));
	}
	
	printf("const uint8_t %s_font8x8[%d*%d] PROGMEM __attribute__ ((aligned(256))) =\n", tilesetname_str, TILE_Y, max_tiles);
	printf("{\n");
	if (human_readable)
	{
		printf("//\t");
		for (i = 0; i < max_tiles; i++)
		{
			printf(" [ %02x=%-5.5s ]%s", i, charname(i), i == max_tiles-1 ? "\n" : " ");
		}
	}

	for (y = 0; y < TILE_Y; y++)
	{
		printf("\t");
		for (i = 0; i < max_tiles; i++)
		{
			uint8_t b = tileset[(i * TILE_BYTES) + y];
			if (human_readable)
				printf("_( %s%s%s%s%s%s%s%s ),",	b & 0x80 ? "W" : "_",
								b & 0x40 ? "W" : "_",
								b & 0x20 ? "W" : "_",
								b & 0x10 ? "W" : "_",
								b & 0x08 ? "W" : "_",
								b & 0x04 ? "W" : "_",
								b & 0x02 ? "W" : "_",
								b & 0x01 ? "W" : "_");
			else
				printf("0x%02x,", b);
		}
		printf("\n");
	}
	printf("};\n\n");
	printf("// EOF\n");
	fprintf(stderr, "Done!\n");

	return 0;
}

#endif

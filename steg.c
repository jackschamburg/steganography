#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define false 0
#define true 1

typedef unsigned char byte;
typedef unsigned char bool;

void encode(byte* dst, byte* src, long src_bytes)
{
	int r;
	long i;

	for(i=0; i<src_bytes*8; i++)
	{
		r = i%8;
		dst[i] &= 0xfe;
		dst[i] |= (src[i/8] & (1<<r))>>r;
	}
}

long decode(byte* dst, byte* src)
{
	int r;
	long i=0;
	byte b=0;

	while(true)
	{
		r = i%8;
		b |= (src[i] & 1) << r;

		if(r == 7)
		{
			if (i>7 && dst[i/8-1]==0x00 && dst[i/8]==0xff)
				return (i/8-2);
			
			dst[i/8] = b;
			b=0;
		}
		i++;
	}
}

int main(int argc, char **argv)
{
	if ( argc < 3 )
	{
		printf("USAGE: %s e input.png file-to-hide\n", argv[0]);
		printf("USAGE: %s d input.png output-filename\n", argv[0]);
		return 1;
	}

	int w,h,n;
	byte *img = stbi_load(argv[2], &w, &h, &n, 0);
	if( img == NULL ) 
	{
		printf("Could not open '%s'\n", argv[2]);
		return 1;
	}
	FILE* fImg = fopen(argv[2], "rb");
	fseek(fImg, 0, SEEK_END);
	long fImgSize = ftell(fImg);
	fclose(fImg);

	if( argv[1][0] == 'e' )
	{
		/* ENCODE MODE */
		FILE* fSecret = fopen(argv[3], "rb");

		if(fSecret != NULL)
		{
			fseek(fSecret, 0, SEEK_END);
			long fSecretSize = ftell(fSecret);
			rewind(fSecret);
			if((fSecretSize+2)*8 > fImgSize)
			{
				printf("File to hide is too large\n");
				fclose(fSecret);
				return 1;
			}

			byte* bSecret = malloc(fSecretSize+2);
			fread(bSecret, fSecretSize, 1, fSecret);
			memset(bSecret+fSecretSize, 0x00, 1);
			memset(bSecret+fSecretSize+1, 0xff, 1);

			encode(img, bSecret, fSecretSize);
			stbi_write_png("output.png", w, h, n, img, w*n);

			free(bSecret);
		}
		else
			printf("Could not open '%s'\n", argv[3]);

		fclose(fSecret);

	}
	else if ( argv[1][0] == 'd' )
	{
		/* DECODE MODE */
		byte* bOut = malloc(fImgSize/8-2);

		long nBytes = decode(bOut, img);

		printf("malloc'd: %dbytes\nnBytes in file: %dbytes\n", , nBytes);

		FILE* fOut = fopen(argv[3], "wb");
		fwrite(bOut,nBytes,1,fOut);
		fclose(fOut);

		free(bOut);
	}
	else
	{
		printf("ARG 1 must be either e|d\n");
		return 1;
	}
	
	stbi_image_free(img);

	return 0;
}

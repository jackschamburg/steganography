#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef unsigned char byte;

void encode(byte* dst, byte* src, long src_bytes)
{
	int r;
	for(long i=0; i<src_bytes*8; i++)
	{
		r = i%8;
		dst[i] &= 0xfe;
		dst[i] |= (src[i/8] & (1<<r))>>r;
	}
}

long decode(byte* dst, byte* src)
{
	long i=0, n;
	byte b=0, r;

	while(1)
	{
		r = i%8;
		n = i/8;
		b |= (src[i] & 1) << r;

		if(r == 7)
		{
			dst[n] = b;
			if (i>=32 && dst[n-3]==0x00 && dst[n-2]==0xff && dst[n-1]==0x00 && dst[n]==0xff)
				return (i/8-3);
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

			if((fSecretSize+4)*8 > w*h*n)
			{
				printf("%s is too large to hide in %s\n", argv[3], argv[2]);
				fclose(fSecret);
				stbi_image_free(img);
				return 1;
			}

			byte* bSecret = malloc(fSecretSize+4);
			fread(bSecret, fSecretSize, 1, fSecret);

			/* Add delimiters */
			memset(bSecret+fSecretSize, 0x00, 1);
			memset(bSecret+fSecretSize+1, 0xff, 1);
			memset(bSecret+fSecretSize+2, 0x00, 1);
			memset(bSecret+fSecretSize+3, 0xff, 1);

			encode(img, bSecret, fSecretSize+4);
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
		byte* bOut = malloc(fImgSize/8-16);

		long nBytes = decode(bOut, img);

		printf("malloc'd: %ldbytes\nnBytes in file: %ldbytes\n", fImgSize/8-16, nBytes);

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

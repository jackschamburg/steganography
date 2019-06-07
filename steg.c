#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef unsigned char byte;

const int delimiterSize = 5;
const byte delimiter[] = { 0x00, 0xff, 0x00, 0xff, 0xfe };

void encode(byte * const dst, const byte * const src, const long src_bytes);
long decode(byte * const dst, const byte * const src, const long src_bytes);
void printProgramUsage(const char* argv0);

int main(int argc, char **argv)
{
	/* Check the correct command line arguments are provided */
	if ( argc < 4 )
	{
		printProgramUsage(argv[0]);
		return 1;
	}

	/* Load input image into memory */
	int width, height, num_channels;
	byte *img = stbi_load(argv[2], &width, &height, &num_channels, 0);
	if ( img == NULL ) 
	{
		printf("Could not open '%s'\n", argv[2]);
		return 1;
	}

	if ( argv[1][0] == 'e' || argv[1][0] == 'E' )
	{
		/* ENCODE MODE */

		/* Open file to hide */
		FILE* fSecret = fopen(argv[3], "rb");

		/* If file exists */
		if ( fSecret != NULL )
		{
			/* Calculate the size in bytes of the secret file */
			fseek(fSecret, 0, SEEK_END);
			long fSecretSize = ftell(fSecret);
			rewind(fSecret);

			/*
			** Check if the size of the secret file + the size of the delimiter bytes
			** can be spread across each last bit of each byte in the image
			*/
			if ( (fSecretSize + delimiterSize) * 8 > width * height * num_channels )
			{
				/* Secret file is too large to hide within the input image */
				printf("%s is too large to hide in %s\n", argv[3], argv[2]);
				fclose(fSecret);
				stbi_image_free(img);
				return 1;
			}

			/* Read the contents of the secret file into memory */
			byte* bSecret = malloc(fSecretSize+delimiterSize);
			fread(bSecret, fSecretSize, 1, fSecret);

			/* Add delimiters */
			memcpy(bSecret + fSecretSize, delimiter, delimiterSize);

			/* Encode secret file data + delimiters into the image */
			encode(img, bSecret, fSecretSize + delimiterSize);

			/* Configure output filename */
			const char * const defaultOutputFilename = "output.png";
			const char * outputFilename;
			if ( argc >= 5 )
				outputFilename = argv[4];
			else
				outputFilename = defaultOutputFilename;

			/* Write image with hidden data inside to disk */
			stbi_write_png(outputFilename, width, height, num_channels, img, width * num_channels);

			free(bSecret);
		}
		/* Else if secret file does not exist on disk */
		else
			printf("Could not open '%s'\n", argv[3]);

		fclose(fSecret);

	}
	else if ( argv[1][0] == 'd' )
	{
		/* DECODE MODE */

		/* Calculate the size in bytes of the input mask image */
		FILE* fImg = fopen(argv[2], "rb");
		fseek(fImg, 0, SEEK_END);
		long fImgSize = ftell(fImg);
		fclose(fImg);

		/* Allocate memory for hidden file  */
		byte* bOut = malloc(fImgSize / 8 - delimiterSize);

		/* Decode the hidden image from the input mask image */
		long nBytes = decode(bOut, img, width * height * num_channels);

		/* Write the hidden file to disk */
		FILE* fOut = fopen(argv[3], "wb");
		fwrite(bOut, nBytes, 1, fOut);
		fclose(fOut);

		free(bOut);
	}
	else
	{
		printProgramUsage(argv[0]);
		return 1;
	}
	
	stbi_image_free(img);

	return 0;
}

/*
** Encode the src byte array into the last bit of each byte in the dst byte array.
**
** Inputs:
**			- dst is the byte array of the mask image's pixels
**			- src is the byte array of the image-to-hide's pixels
**			- src_bytes is the length of the src byte array which INCLUDES
**			  the delimiter string!
**
** Pre-conditions:
**			- the size of the dst array must be at least 8 times the size
**			  of the src array + the length of the delimiter
*/ 
void encode(byte * const dst, const byte * const src, const long src_bytes)
{
	/* Loop for i=0 to i=num_of_bits-1 in src byte array */
	for ( long i = 0; i < src_bytes * 8; i++ )
	{
		/* Clear the last bit of the mask image */
		dst[i] &= 0xfe;

		/*
		** Set the last bit of the mask image to the i'th bit 
		** from the src byte array 
		*/
		int r = i % 8;
		dst[i] |= ( src[i/8] & (1 << r) ) >> r;
	}
}

/*
** Decode the file hidden within the last bit of each byte within the 
** src byte array
**
** Inputs:
**			- dst is the byte array to write the hidden file to
**			- src is the byte array of mask image with a file hidden within
**			- src_bytes is the length of the src array in bytes
**
** Returns: - the size of the decoded hidden file in bytes
**
** Pre-conditions:
**			- the size of the dst array must be at least the size of the src
** 			  array divided by 8
*/ 
long decode(byte * const dst, const byte * const src, const long src_bytes)
{
	long n;			// stores what byte in the dst array the loop is up to
	byte b = 0;		// stores the current byte. Is reset after each byte has been written to dst
	byte r;			// stores what bit in the current byte the loop is up to

	/* Loop over every byte in the src array */
	for ( long i = 0; i < src_bytes; i++ )
	{
		/* Calculate the number of times to rotate the bit into place */
		r = i % 8;

		/* Calculate what byte in the dst array we are up to */
		n = i / 8;

		/* 
		** Set the r'th bit in the current byte to the value of the i'th src 
		** byte's least significant bit
		*/
		b |= ( src[i] & 1 ) << r;

		/* If all bits in the current byte have been set */
		if ( r == 7 )
		{
			/* Save the current byte to the n'th byte in the output array */
			dst[n] = b;

			/* Check if we have reached the delimiter string */
			if (i >= delimiterSize * 8)
			{
				int foundDelimiter = 1; // assume we have found delimiter
				for ( int j = 0; j < delimiterSize; j++ )
				{
					if ( dst[n - (delimiterSize - j - 1)] != delimiter[j] )
						foundDelimiter = 0; // we did not find the delimiter
				}

				if ( foundDelimiter )
					/* return the length of the output file in bytes */
					return ( i/8 - delimiterSize + 1 );
			}

			/* Reset the current byte */
			b = 0;
		}
	}

	/* Function did not return in the for-loop above! */
	/* File-ending delimiter was not reached so something went wrong here... */
	fprintf(stderr, "Error when decoding: matching delimiter not found within src mask file\n");
	return src_bytes;
}

void printProgramUsage(const char* argv0)
{
	printf("USAGE: %s e(ncrypt) mask-image.png file-to-hide [output-filename.png]\n", argv0);
	printf("USAGE: %s d(ecrypt) mask-image.png output-filename\n", argv0);
}

/*
 *
 *  Copyright (C) 1997-2005, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  dcmjpeg
 *
 *  Author:  Marco Eichelberg, Norbert Olges
 *
 *  Purpose: compression routines of the IJG JPEG library configured for 12 bits/sample. 
 *
 *  Last Update:      $Author: lpysher $
 *  Update Date:      $Date: 2006/03/01 20:15:44 $
 *  Source File:      $Source: /cvsroot/osirix/osirix/Binaries/dcmtk-source/dcmjpeg/djeijg12.cc,v $
 *  CVS/RCS Revision: $Revision: 1.1 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjpeg/djeijg2k.h"
#include "dcmtk/dcmjpeg/djcparam.h"
#include "dcmtk/ofstd/ofconsol.h"

#include <sys/types.h>
//#include <sys/sysctl.h>

#define INCLUDE_CSTDIO
#define INCLUDE_CSETJMP
#include "dcmtk/ofstd/ofstdinc.h"

// These two macros are re-defined in the IJG header files.
// We undefine them here and hope that IJG's configure has
// come to the same conclusion that we have...
#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif
#ifdef HAVE_STDDEF_H
#undef HAVE_STDDEF_H
#endif

// use 16K blocks for temporary storage of compressed JPEG data
#define IJGE12_BLOCKSIZE 16384

#include "openjpeg.h"

/**
sample error callback expecting a FILE* client object
*/
static void error_callback(const char *msg, void *a)
{
  std::cout << "Error: " << msg << std::endl;
}
/**
sample warning callback expecting a FILE* client object
*/
static void warning_callback(const char *msg, void *a)
{
  std::cout << "Warning: " << msg << std::endl;
}
/**
sample debug callback expecting no client object
*/
static void info_callback(const char *msg, void *a)
{
  //std::cout << "Info: " << msg << std::endl;
}

static inline int int_ceildivpow2(int a, int b)
{
  return (a + (1 << b) - 1) >> b;
}

OPJ_SIZE_T _write (void * p_buffer, OPJ_SIZE_T p_nb_bytes, FILE * fp)
{
  return fwrite(p_buffer, 1, p_nb_bytes, fp);
}

DJCompressJP2K::DJCompressJP2K(const DJCodecParameter& cp, EJ_Mode mode, Uint8 theQuality, Uint8 theBitsPerSample)
: DJEncoder()
, cparam(&cp)
, quality(theQuality)
, bitsPerSampleValue(theBitsPerSample)
, modeofOperation(mode)
{

}

DJCompressJP2K::~DJCompressJP2K()
{

}

template<typename T>
static void rawtoimage_fill(T *inputbuffer, int w, int h, int numcomps, opj_image_t *image, int pc)
{
  T *p = inputbuffer;
  if( pc )
    {
    for(int compno = 0; compno < numcomps; compno++)
      {
      for (int i = 0; i < w * h; i++)
        {
        /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
        image->comps[compno].data[i] = *p;
        ++p;
        }
      }
    }
  else
    {
    for (int i = 0; i < w * h; i++)
      {
      for(int compno = 0; compno < numcomps; compno++)
        {
        /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
        image->comps[compno].data[i] = *p;
        ++p;
        }
      }
    }
}

static opj_image_t * rawtoimage(char *inputbuffer, opj_cparameters_t *parameters,
  int fragment_size, int image_width, int image_height, int sample_pixel,
  int bitsallocated, int bitsstored, int sign, int quality, int pc){
  (void)quality;
  (void)fragment_size;
  int w, h;
  int numcomps;
  OPJ_COLOR_SPACE color_space;
  opj_image_cmptparm_t cmptparm[3]; /* maximum of 3 components */
  opj_image_t * image = NULL;

  assert( sample_pixel == 1 || sample_pixel == 3 );
  if( sample_pixel == 1 )
    {
    numcomps = 1;
    color_space = OPJ_CLRSPC_GRAY;
    }
  else // sample_pixel == 3
    {
    numcomps = 3;
    color_space = OPJ_CLRSPC_SRGB;
    /* Does OpenJPEg support: CLRSPC_SYCC ?? */
    }
  if( bitsallocated % 8 != 0 )
    {
    fprintf( stderr, "BitsAllocated is not 8\n" );
    return 0;
    }
  assert( bitsallocated % 8 == 0 );
  // eg. fragment_size == 63532 and 181 * 117 * 3 * 8 == 63531 ...
  assert( ((fragment_size + 1)/2 ) * 2 == ((image_height * image_width * numcomps * (bitsallocated/8) + 1)/ 2 )* 2 );
  int subsampling_dx = parameters->subsampling_dx;
  int subsampling_dy = parameters->subsampling_dy;

  // FIXME
  w = image_width;
  h = image_height;

  /* initialize image components */
  memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
  //assert( bitsallocated == 8 );
  for(int i = 0; i < numcomps; i++) {
    cmptparm[i].prec = bitsstored;
    cmptparm[i].bpp = bitsallocated;
    cmptparm[i].sgnd = sign;
    cmptparm[i].dx = subsampling_dx;
    cmptparm[i].dy = subsampling_dy;
    cmptparm[i].w = w;
    cmptparm[i].h = h;
  }

  /* create the image */
  image = opj_image_create(numcomps, &cmptparm[0], color_space);
  if(!image) {
    return NULL;
  }
  /* set image offset and reference grid */
  image->x0 = parameters->image_offset_x0;
  image->y0 = parameters->image_offset_y0;
  image->x1 = parameters->image_offset_x0 + (w - 1) * subsampling_dx + 1;
  image->y1 = parameters->image_offset_y0 + (h - 1) * subsampling_dy + 1;

  /* set image data */

  //assert( fragment_size == numcomps*w*h*(bitsallocated/8) );
  if (bitsallocated <= 8)
    {
    if( sign )
      {
      rawtoimage_fill<int8_t>((int8_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    else
      {
      rawtoimage_fill<uint8_t>((uint8_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    }
  else if (bitsallocated <= 16)
    {
    if( sign )
      {
      rawtoimage_fill<int16_t>((int16_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    else
      {
      rawtoimage_fill<uint16_t>((uint16_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    }
  else if (bitsallocated <= 32)
    {
    if( sign )
      {
      rawtoimage_fill<int32_t>((int32_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    else
      {
      rawtoimage_fill<uint32_t>((uint32_t*)inputbuffer,w,h,numcomps,image,pc);
      }
    }
  else
    {
    return NULL;
    }

  return image;
}

OFCondition DJCompressJP2K::encode( 
  Uint16 columns,
  Uint16 rows,
  EP_Interpretation colorSpace,
  Uint16 samplesPerPixel,
  Uint8 * image_buffer,
  Uint8 * & to,
  Uint32 & length,
  Uint8 pixelRepresentation,
  double minUsed, double maxUsed)
{
	return encode( columns, rows, colorSpace, samplesPerPixel, (Uint16 *) image_buffer, (Uint16 * &)to, length, 8, pixelRepresentation, minUsed, maxUsed);
}

OFCondition DJCompressJP2K::encode(
    Uint16  columns ,
    Uint16  rows ,
    EP_Interpretation  interpr ,
    Uint16  samplesPerPixel ,
    Uint16 *  image_buffer ,
    Uint8 *&  to ,
    Uint32 &  length,
	Uint8 pixelRepresentation,
	double minUsed, double maxUsed)
{
	return encode( columns, rows, interpr, samplesPerPixel, (Uint16 *) image_buffer, (Uint16 * &)to, length, 16, pixelRepresentation, minUsed, maxUsed);
}

Uint16 DJCompressJP2K::bytesPerSample() const
{
	if( bitsPerSampleValue <= 8)
		return 1;
	else
		return 2;
}

Uint16 DJCompressJP2K::bitsPerSample() const
{
	return bitsPerSampleValue;
}

#ifdef _WIN32
FILE *open_memstream(char **l_out, size_t *l_size);
int get_buffer_and_size(FILE *fp);
#endif //_WIN32

OFCondition DJCompressJP2K::encode( 
  Uint16 columns,
  Uint16 rows,
  EP_Interpretation colorSpace,
  Uint16 samplesPerPixel,
  Uint16 * image_buffer,
  Uint16 * & to,
  Uint32 & length,
  Uint8 bitsAllocated,
  Uint8 pixelRepresentation,
  double minUsed, double maxUsed)
{
	int bitsstored = bitsAllocated;
	
    if( samplesPerPixel > 1)
        bitsstored = bitsAllocated = 8;
    
	OFBool isSigned = pixelRepresentation;
	
	if( bitsAllocated >= 16)
	{
		int amplitude = maxUsed;
		
		if( minUsed < 0)
			amplitude -= minUsed;
		
		int bits = 1, value = 2;
		
		while( value < amplitude && bits <= 16)
		{
			value *= 2;
			bits++;
		}
		
		if( minUsed < 0) // K A10009536850 22.06.12
			bits++;
		
		if( bits < 9)
			bits = 9;
		
		// avoid the artifacts... switch to lossless
		if( (maxUsed >= 32000 && minUsed <= -32000) || maxUsed >= 65000 || bits > 16)
			quality = 0;
		
		if( bits > 16) bits = 16;
		
		bitsstored = bits;
	}
    opj_cparameters_t parameters;
    opj_image_t *image = NULL;
		
//    printf( "JP2K OPJ-DCMTK-Encode ");
		
    opj_set_default_encoder_parameters(&parameters);

    parameters.tcp_numlayers = 1;
    parameters.cp_disto_alloc = 1;
		
    switch( quality)
    {
    case 0: // DCMLosslessQuality
        parameters.tcp_rates[0] = 0;
        break;
            
    case 1: // DCMHighQuality
        parameters.tcp_rates[0] = 4;
        break;
            
    case 2: // DCMMediumQuality
        if( columns <= 600 || rows <= 600)
            parameters.tcp_rates[0] = 6;
        else
            parameters.tcp_rates[0] = 8;
        break;
            
    case 3: // DCMLowQuality
        parameters.tcp_rates[0] = 16;
        break;
            
    default:
        //printf( "****** warning unknown compression rate -> lossless : %d", quality);
        parameters.tcp_rates[0] = 0;
        break;
    }

    int image_width = columns;
    int image_height = rows;
    int sample_pixel = samplesPerPixel;

    if (colorSpace == EPI_Monochrome1 || colorSpace == EPI_Monochrome2)
    {

    }
    else
    {
        if( sample_pixel != 3)
            printf( "*** RGB Photometric?, but... SamplesPerPixel != 3 ?");
        sample_pixel = 3;
    }

    image = rawtoimage( (char *)image_buffer, &parameters,  static_cast<int>( columns*rows*samplesPerPixel*bitsAllocated/8),  image_width, image_height, sample_pixel, bitsAllocated, bitsstored, isSigned, quality, 0);

    if(!image) {
        fprintf(stderr, "Unable to load buffer image\n");
        return EC_Normal;
    } 
        
    parameters.cod_format = 0; /* J2K format output */
    int codestream_length;

    opj_codec_t *l_codec = 00;
    l_codec = opj_create_compress(OPJ_CODEC_J2K);
    opj_set_info_handler(l_codec, info_callback,00);
    opj_set_warning_handler(l_codec, warning_callback,00);
    opj_set_error_handler(l_codec, error_callback,00);

    /* setup the encoder parameters using the current image and using user parameters */
    opj_setup_encoder(l_codec, &parameters, image);

    char * l_out;
    size_t l_size;
    FILE * l_file = open_memstream(&l_out, &l_size);

    opj_stream_t * l_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
    if (! l_stream){
        fprintf(stderr, "failed to create stream\n");
        return EC_Normal;
    }

    opj_stream_set_user_data(l_stream, l_file, (opj_stream_free_user_data_fn)NULL);
    opj_stream_set_write_function(l_stream, (opj_stream_write_fn)_write);
    /* encode the image */
    int bSuccess = opj_start_compress(l_codec, image, l_stream);
    if (!bSuccess) {
        fprintf(stderr, "failed to encode image\n");
        return EC_Normal;
    }

    bSuccess = bSuccess && opj_encode(l_codec, l_stream);
    if (!bSuccess) {
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        fprintf(stderr, "failed to encode image 2\n");
        remove(parameters.outfile);
        return EC_Normal;
    }

    bSuccess = bSuccess && opj_end_compress(l_codec, l_stream);
    if (!bSuccess)  {
        fprintf(stderr, "failed to encode image: opj_end_compress\n");
        return EC_Normal;
    }

    /* l_file memstream must be flushed to allow accessint to l_out and l_size values */
    fflush(l_file);
#ifdef _WIN32
	get_buffer_and_size(l_file);
#endif
    fclose(l_file);
    /* copy compressed stream to "to" UINT8*  */
    to = new Uint16[l_size];
    memcpy( to, l_out, l_size);
    length = l_size;

    /* free remaining compression structures */
    free(l_out);
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    opj_image_destroy(image);
//	}
    return EC_Normal;
}

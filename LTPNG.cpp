/**
 * Lowe Technologies PNG Encoder (LTPNG)
 *
 * A PNG encoder built to the ISO/IEC 15948:2003 Portable Network Graphics Standard Rev 10 Nov 03
 * Supports only 8-bit and 16-bit truecolour PNG images with or without alpha.
 *
 * @author Rich Lowe
 * @version 1.1.0
 */
 
/**
 * Revision history:
 *
 * 1.0.0: Initial implementation supporting only 8 and 16-bit truecolour images without
 *        filtering and standard zlib compression without buffering support.
 * 1.0.1: Minor formatting and bug fixes, added static ramping and pattern functions.
 * 1.1.0: Added support for all filter types defined by the standard, inefficiency fixes.
 */

/** Header includes */
#include <iostream>
#include <fstream>
#include <cmath>
#include <zlib.h>
#include "LTPNG.h"

using namespace std;

/**
 * Create a truecolour or truecolour with alpha image of resolution width x height pixels with each pixel containing either
 * an 8 or 16-bit value in three channels (red, green, and blue) for truecolour, and four channels (red, green, blue, and alpha)
 * for truecolour with alpha.  Each value represents the amount of expression of the quality the channel represents, with 0 being 
 * the least expressed, and 255 (8-bit) or 65535 (16-bit) being the most expressed.
 */  
LTPNG::LTPNG(unsigned char depth, unsigned char type, unsigned char filter) {
	/** Initialize CRC vars */
	crc_index = 0;
	crc_allocated = 0;
	crc_table_exists = 0;
	file_size = 0;
	
	/** If 8-bit, max expression is at 0xFF (255) */
	max_val = 255;
	
	/** If 16-bit, max expression is at 0xFFFF (65535) */
	if ( depth == 16 )
		max_val = 65535;
		
	/** Record bit depth and colour type */
	bit_depth = depth;
	colour_type = type;
	filter_type = filter;
}

/** Create a PNG image of set size with provided pixel channels */
void LTPNG::create_image(ofstream &file, unsigned int pixel_width, unsigned int pixel_height, unsigned short *red, unsigned short *green, unsigned short *blue, unsigned short *alpha) {	
	image = &file;
	width = pixel_width;
	height = pixel_height;
	
	/** Calculate pixel size */
	unsigned char pixel_size = colour_type == 2 ? 3 : 4;

	if ( bit_depth == 16 )
		pixel_size *= 2;
	
	/** Set max data size for buffers */
	unsigned int data_size = height*(width*pixel_size + 1);
		
	/** Self-allocate uncompressed, filtered, and compressed IDAT data stream arrays */
	uncompressed_data = new unsigned char[data_size];
	filtered_data = new unsigned char[data_size];
	compressed_data = new unsigned char[data_size];

	unsigned int row, col;
	unsigned int uncompressed_len = 0;
	unsigned int filtered_len = 0;
	unsigned int compressed_len = 0;

	/** Loop through each pixel row in the image to load the channel data */
	for ( row = 0; row < height; row++ ) {
		/** Save the filter bit (currently 0 for unfiltered) as the first byte of the uncompressed stream */
		uncompressed_data[uncompressed_len++] = filter_type;
		filtered_data[filtered_len++] = uncompressed_data[uncompressed_len - 1];

		/** 
		 * Loop through each pixel column on this row, saving each of the channels for each pixel in the 
		 * uncompressed stream in the proper order, including alpha channels for colour_type (6)
		 */
		for ( col = 0; col < width; col++ ) {
			/** If 8-bit */
			if ( bit_depth == 8 ) {
				/** Store the raw, uncompressed 8-bit RGB(A) pixel data in the uncompressed data array */
				uncompressed_data[uncompressed_len++] = red[row*width + col];
				uncompressed_data[uncompressed_len++] = green[row*width + col];
				uncompressed_data[uncompressed_len++] = blue[row*width + col];
				
				if ( colour_type == 6 )
					uncompressed_data[uncompressed_len++] = alpha[row*width + col];

				/** Now that the uncompressed data is stored, we can go ahead and filter those bytes */
				filtered_data[filtered_len++] = filter_byte(row, col, 1);
				filtered_data[filtered_len++] = filter_byte(row, col, 2);
				filtered_data[filtered_len++] = filter_byte(row, col, 3);
				
				if ( colour_type == 6 )
					filtered_data[filtered_len++] = filter_byte(row, col, 4);
			}
			
			/** If 16-bit */
			if ( bit_depth == 16 ) {
				/** Store the raw, uncompressed 16-bit RGB(A) pixel data in the uncompressed data array */
				uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(red[row*width + col], 1);
				uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(red[row*width + col], 2);
				uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(green[row*width + col], 1);
				uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(green[row*width + col], 2);
				uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(blue[row*width + col], 1);
				uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(blue[row*width + col], 2);
				
				if ( colour_type == 6 ) {
					uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(alpha[row*width + col], 1);
					uncompressed_data[uncompressed_len++] = get_byte_from_two_bytes(alpha[row*width + col], 2);
				}
				
				/** Now that the uncompressed data is stored, we can go ahead and filter those bytes */
				filtered_data[filtered_len++] = filter_byte(row, col, 1);
				filtered_data[filtered_len++] = filter_byte(row, col, 2);
				filtered_data[filtered_len++] = filter_byte(row, col, 3);
				filtered_data[filtered_len++] = filter_byte(row, col, 4);
				filtered_data[filtered_len++] = filter_byte(row, col, 5);
				filtered_data[filtered_len++] = filter_byte(row, col, 6);
				
				if ( colour_type == 6 ) {
					filtered_data[filtered_len++] = filter_byte(row, col, 7);
					filtered_data[filtered_len++] = filter_byte(row, col, 8);
				}
			} 
		}
	}
	
	/** Compress the data using the zlib library */    
	def(filtered_data, filtered_len, compressed_data, data_size, compressed_len, Z_DEFAULT_COMPRESSION);
	
	/** Allocate space for the CRC buffer */
	allocate_crc_size(bit_depth, colour_type);
	
	/** Write the PNG file signature per section 5.2 */
	write_png_signature();

	/** Write the IHDR header chunk per 11.2.2 */
	write_header_chunk(bit_depth, colour_type, 0);
		
	/** Write the compressed data as a single IDAT chunk per 4.1 and 11.2.4 */
	write_data_chunk(compressed_data, compressed_len);

	/** Write the IEND end chunk and 11.2.5 */
	write_end_chunk();
	
	/** Clean up self-allocated memory */
	delete[] uncompressed_data;
	delete[] filtered_data;
	delete[] compressed_data;
	
	file_size = compressed_len;
	
	/** Initialize CRC vars */
	crc_index = 0;
	crc_allocated = 0;
	crc_table_exists = 0;
}

/** Write the 8-byte PNG file signature per section 5.2 */
void LTPNG::write_png_signature() {
	fwrite_8(137);
	fwrite_8(80);
	fwrite_8(78);
	fwrite_8(71);
	fwrite_8(13);
	fwrite_8(10);
	fwrite_8(26);
	fwrite_8(10);
}

/** Write the IHDR image header chunk */
void LTPNG::write_header_chunk(unsigned char bit_depth, unsigned char colour_type, unsigned char interlace_method) {	
	fwrite_32(13);				/** Write the 4-byte data length to start the header chunk */
	crc_init();					/** Reset the CRC buffer */
	fwrite_8(73);				/** Write the 4-byte chunk type per 11.2.2 */
	fwrite_8(72);
	fwrite_8(68);
	fwrite_8(82);
	fwrite_32(width);			/** Write the 4-byte image width in pixels per 11.2.2  */
	fwrite_32(height);			/** Write the 4-byte image height in pixels per 11.2.2 */
	fwrite_8(bit_depth);		/** Write the 1-byte bit depth of each pixel per 11.2.2 */
	fwrite_8(colour_type);		/** Write the 1-byte colour type per 11.2.2 */
	fwrite_8(0);				/** Write the 1-byte compression type per 11.2.2 */
	fwrite_8(0);				/** Write the 1-byte filter type per 11.2.2 */
	fwrite_8(interlace_method);	/** Write the 1-byte interlace method per 11.2.2 */
	fwrite_32(get_crc());		/** Calculate and write the 4-byte CRC value per Annex D */
}

/** Write an IDAT image data chunk */
void LTPNG::write_data_chunk(unsigned char *compressed_data, unsigned int len) {
	fwrite_32(len);				/** Write the 4-byte data length to start the data chunk */
	crc_init();					/** Reset the CRC buffer */
	fwrite_8(73);				/** Write the 4-byte chunk type per 11.2.4 */
	fwrite_8(68);
	fwrite_8(65);
	fwrite_8(84);
    	
    /** Write the compressed data per 4.1 and 11.2.4 */
    unsigned int i;
    
    for ( i = 0; i < len; i++ )
		fwrite_8(compressed_data[i]);

	fwrite_32(get_crc());		/** Calculate and write the 4-byte CRC value per Annex D */
}

/** Write the IEND image end chunk */
void LTPNG::write_end_chunk() {
	fwrite_32(0);				/** Write the 4-byte data length to start the end chunk */
	crc_init();					/** Reset the CRC buffer */
	fwrite_8(73);				/** Write the 4-byte chunk type per 11.2.5 */
	fwrite_8(69);
	fwrite_8(78);
	fwrite_8(68);
	fwrite_32(get_crc());		/** Calculate and write the 4-byte CRC value per Annex D */
}

/** Write one 8-bit unsigned int to an image file and append it to the CRC buffer */
void LTPNG::fwrite_8(unsigned char val) { 
	/** Verify we won't exceeded the CRC buffer size */ 
	if ( crc_index == crc_allocated )
		throw "LTPNG: CRC buffer out of room, was allocate_crc_size() called?";

	image->write((char *) &val, 1);
	crc_buf[crc_index++] = val;
}

/** Write one 16-bit unsigned int to an image file in big endian order and append append each byte to the CRC buffer */
void LTPNG::fwrite_16(unsigned short val) {
	/** PNG prefers big endian, so have to re-order the 32-bit unsigned int */
	fwrite_8(get_byte_from_two_bytes(val, 1));
	fwrite_8(get_byte_from_two_bytes(val, 2));
}

/** Write one 32-bit unsigned int to an image file in big endian order and append append each byte to the CRC buffer */
void LTPNG::fwrite_32(unsigned int val) {
	/** PNG prefers big endian, so have to re-order the 32-bit unsigned int */
	fwrite_8(get_byte_from_four_bytes(val, 1));
	fwrite_8(get_byte_from_four_bytes(val, 2));
	fwrite_8(get_byte_from_four_bytes(val, 3));
	fwrite_8(get_byte_from_four_bytes(val, 4));
}

/** Retrieves one 8-bit unsigned int segment from a 16-bit unsigned int value */
unsigned char LTPNG::get_byte_from_two_bytes(unsigned int val, unsigned char byte_num) {
	/** Verify a valid byte number was provided (byte 1 = bits 31..24, byte 2 = bits 23..16, byte 3 = bits 15..8, byte 4 = bits 7..0) */
	if ( byte_num < 1 || byte_num > 2 )
		throw "LTPNG: invalid byte_num passed to get_byte_from_two_bytes()";
	
	/** Define the required shift to get the desired byte in the LSB slot */
	unsigned char shift = 8 - (byte_num - 1)*8;

	/** Shift and return the LSB byte using an AND mask */
	return (val>>shift) & 0xFF;
}

/** Retrieves one 8-bit unsigned int segment from a 32-bit unsigned int value */
unsigned char LTPNG::get_byte_from_four_bytes(unsigned int val, unsigned char byte_num) {
	/** Verify a valid byte number was provided (byte 1 = bits 31..24, byte 2 = bits 23..16, byte 3 = bits 15..8, byte 4 = bits 7..0) */
	if ( byte_num < 1 || byte_num > 4 )
		throw "LTPNG: invalid byte_num passed to get_byte_from_four_bytes()";

	/** Define the required shift to get the desired byte in the LSB slot */
	unsigned char shift = 24 - (byte_num - 1)*8;

	/** Shift and return the LSB byte using an AND mask */
	return (val>>shift) & 0xFF;
}

/** Perform the filter conversion using the 5 supported filter methods */
unsigned char LTPNG::filter_byte(unsigned int row, unsigned int col, unsigned char offset) {
	unsigned char x, a, b, c;
	unsigned char pixel_size = colour_type == 2 ? 3 : 4;
	
	if ( bit_depth == 16 )
		pixel_size *= 2;
	
	x = uncompressed_data[row*(width*pixel_size + 1) + col*pixel_size + offset];
	a = col > 0 ? uncompressed_data[row*(width*pixel_size + 1) + col*pixel_size - pixel_size + offset] : 0;
	b = row > 0 ? uncompressed_data[row*(width*pixel_size + 1) + col*pixel_size - width*pixel_size + offset - 1] : 0;
	c = col > 0 && row > 0 ? uncompressed_data[row*(width*pixel_size + 1) + col*pixel_size - pixel_size - width*pixel_size + offset - 1] : 0;
	
	if ( filter_type == 0 )
		return x;
	else if ( filter_type == 1 )
		return x - a;
	else if ( filter_type == 2 )
		return x - b;
	else if ( filter_type == 3 )
		return x - floor((a + b)/2);
	else if ( filter_type == 4 )
		return x - paeth_predictor(a, b, c);
		
	throw "LTPNG::filter_byte(): Invalid filter type.";
}

/** Paeth predictor for PNG filter method 4 defined in the PNG specification */
unsigned char LTPNG::paeth_predictor(short a, short b, short c) {
	short p, pa, pb, pc;
	
	p = a + b - c;
	pa = abs(p - a);
	pb = abs(p - b);
	pc = abs(p - c);

	if ( pa <= pb && pa <= pc )
		return a;
	else if ( pb <= pc )
		return b;
	
	return c;
}

/** Make the table for a fast CRC */
void LTPNG::make_crc_table() {
	unsigned int c;
	unsigned short n, k;

	for (n = 0; n < 256; n++) {
		c = (unsigned int) n;

		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xEDB88320L ^ (c>>1);
			else
				c = c >> 1;
		}

		crc_table[n] = c;
	}
	
	crc_table_exists = 1;
}

/** Simple helper method for resetting the CRC buffer index */
void LTPNG::crc_init() {
	crc_index = 0;
}

/** Allocate enough space for the CRC buffer */
void LTPNG::allocate_crc_size(unsigned char bit_depth, unsigned char colour_type) {
	/** Calculate pixel size */
	unsigned char pixel_size = colour_type == 2 ? 3 : 4;
	
	/** Calculate max CRC length */
	unsigned int len = width*height*pixel_size*(3-16/bit_depth) + height + 4 + 1;
	
	/** If CRC buffer already allocated, free so new block can be allocated */
	if ( crc_allocated )
		delete[] crc_buf;
		
	/** Allocate new blcok */
	crc_buf = new unsigned char[len];
	
	/** Store new block length in global */
	crc_allocated = len;
}

/** Returns the CRC of the bytes buf[0..len-1] */
unsigned int LTPNG::get_crc() {
	return update_crc(0xffffffffL, crc_buf, crc_index) ^ 0xffffffffL;
}

/** 
 * Update a running CRC with the bytes buf[0..len-1]--the CRC should be initialized to 
 * all 1's, and the transmitted value is the 1's complement of the final running CRC 
 */
unsigned int LTPNG::update_crc(unsigned int crc, unsigned char *buf, unsigned int len) {
	unsigned int c = crc;
	unsigned int n;

	if ( !crc_table_exists )
		make_crc_table();
		
	for (n = 0; n < len; n++)
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c>>8);

	return c;
}

/** 
 * Compress from file source to file dest until EOF on source.
 * def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 * allocated for processing, Z_STREAM_ERROR if an invalid compression
 * level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
 * version of the library linked do not match, or Z_ERRNO if there is
 * an error reading or writing the files
 */
void LTPNG::def(unsigned char *in, unsigned int in_len, unsigned char *out, unsigned int out_len, unsigned int &written, int level) {
	int8_t ret;
    z_stream strm;
	
    /** Allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    /** Initialize the zlib deflate stream */
    ret = deflateInit(&strm, level);
    
    /** Handle any errors */
    if ( ret != Z_OK ) {
    	switch ( ret ) {
    		case Z_MEM_ERROR: throw "LTPNG::def(): not enough memory for deflateInit()";
    		case Z_STREAM_ERROR: throw "LTPNG::def(): deflateInit() received invalid compression level";
    		case Z_VERSION_ERROR: throw "LTPNG::def(): zlib library version is incompatible with the version of deflateInit() assumed";
    		default: throw "LTPNG::def(): unknown error on deflateInit()";
    	}
    	
    	exit(1);
    }

    /** Compress until end of file */
	strm.next_in = in;
	strm.avail_in = in_len;
		
	/** Set the output buffer and output buffer length */
	strm.next_out = out;
	strm.avail_out = out_len;

	/** Force full output in one go */
	ret = deflate(&strm, Z_FINISH);
	
	/** Handle any errors */
	if ( ret != Z_STREAM_END ) {
    	switch ( ret ) {
    		case Z_OK: throw "LTPNG::def(): deflate() output length insufficient to hold uncompressed data";
    		case Z_STREAM_ERROR: throw "LTPNG::def(): deflate() parameters are invalid";
    		case Z_BUF_ERROR: throw "LTPNG::def(): no progress possible, output length insufficient for deflate()";
    		default: throw "LTPNG::def(): unknown error on deflate()";
    	}
    	
    	exit(1);
    }
    
    /** Record the output bytes written */
	written = strm.total_out;

    /** Clean up the zlib stream */
    ret = deflateEnd(&strm);
    
    /** Handle any errors */
    if ( ret != Z_OK ) {
    	switch ( ret ) {
    		case Z_DATA_ERROR: throw "LTPNG::def(): deflateEnd() called prematurely, some data lost";
    		case Z_STREAM_ERROR: throw "LTPNG::def(): deflateEnd() stream state was inconsistent";
    		default: throw "LTPNG::def(): unknown error on deflateEnd()";
    	}
    	
    	exit(1);
    }
}

/**
 * Decompress from file source to file dest until stream ends or EOF.
 * inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 * allocated for processing, Z_DATA_ERROR if the deflate data is
 * invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
 * the version of the library linked do not match, or Z_ERRNO if there
 * is an error reading or writing the files
 */
void LTPNG::inf(unsigned char *in, unsigned int in_len, unsigned char *out, unsigned int out_len, unsigned int &written) {
    int8_t ret;
    z_stream strm;

    /** Allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    
    /** Initialize the zlib inflate stream */
    ret = inflateInit(&strm);
    
    /** Handle any errors */
    if ( ret != Z_OK ) {
    	switch ( ret ) {
    		case Z_MEM_ERROR: throw "LTPNG::inf() not enough memory for inflateInit()";
    		case Z_STREAM_ERROR: throw "LTPNG::inf() inflateInit() parameters are invalid";
    		case Z_VERSION_ERROR: throw "LTPNG::inf() zlib library version is incompatible with the version of inflateInit() assumed";
    		default: throw "LTPNG::inf() unknown error on inflateInit()";
    	}
    	
    	exit(1);
    }

	unsigned short i = 0;
	
    /** Decompress until deflate stream ends or end of file */
	strm.next_in = in;
	strm.avail_in = in_len;

	/** Run inflate() on input until output buffer not full */
	strm.next_out = out;
	strm.avail_out = out_len;

	/** Force full output in one go */
	ret = inflate(&strm, Z_FINISH);
	
	/** Handle any errors */
	if ( ret != Z_STREAM_END ) {
    	switch ( ret ) {
    		case Z_OK: throw "LTPNG::inf() inflate() output length insufficient to hold uncompressed data";
    		case Z_NEED_DICT: throw "LTPNG::inf() inflate() requires preset dictionary for decompression";
    		case Z_DATA_ERROR: throw "LTPNG::inf() inflate() input data is corrupted";
    		case Z_MEM_ERROR: throw "LTPNG::inf() not enough memory for inflate()";
    		case Z_STREAM_ERROR: throw "LTPNG::inf() inflate() parameters are invalid";
    		case Z_BUF_ERROR: throw "LTPNG::inf() no progress possible, output length insufficient for inflate()";
    		case Z_VERSION_ERROR: throw "LTPNG::inf() zlib library version is incompatible with the version of inflateInit() assumed";
    		default: throw "LTPNG::inf() unknown error on inflate()";
    	}
    	
    	exit(1);
    }
	
	/** Record the output bytes written */
	written = strm.total_out;

    /** Clean up and return */
    ret = inflateEnd(&strm);
    
    /** Handle any errors */
    if ( ret != Z_OK ) {
    	switch ( ret ) {
    		case Z_STREAM_ERROR: throw "LTPNG::inf() inflateEnd() stream state was inconsistent";
    		default: throw "LTPNG::inf() unknown error on inflateEnd()";
    	}
    	
    	exit(1);
    }
}

/** Gradient ramp function (north) */
double LTPNG::ramp_n(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return 1 - (double) row/height;
}

/** Gradient ramp function (south) */
double LTPNG::ramp_s(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return (double) row/height;
}

/** Gradient ramp function (east) */
double LTPNG::ramp_e(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return (double) col/width;
}

/** Gradient ramp function (west) */
double LTPNG::ramp_w(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return 1 - (double) col/width;
}

/** Gradient ramp function (northwest) */
double LTPNG::ramp_nw(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return (double) (height - row + width - col)/(width + height);
}

/** Gradient ramp function (northeast) */
double LTPNG::ramp_ne(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return (double) (height - row + col)/(width + height);
}

/** Gradient ramp function (southwest) */
double LTPNG::ramp_sw(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return (double) (row + width - col)/(width + height);
}

/** Gradient ramp function (southeast) */
double LTPNG::ramp_se(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return (double) (row + col)/(width + height);
}

/** Full expression function (full) */
double LTPNG::pattern_full(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return 1;
}

/** Half expression function (none) */
double LTPNG::pattern_half(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return 0.5;
}

/** Absent expression function (none) */
double LTPNG::pattern_none(unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	return 0;
}

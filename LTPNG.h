/**
 * Lowe Technologies PNG Encoder (LTPNG)
 *
 * A PNG encoder built to the ISO/IEC 15948:2003 Portable Network Graphics Standard Rev 10 Nov 03
 * Supports only 8-bit and 16-bit truecolour PNG images with or without alpha and no filtering.
 *
 * @author Rich Lowe
 * @version See CPP for revision and revision history
 */
 
using namespace std;

class LTPNG {
	public:
		/** Public properties */
		unsigned int file_size;
		unsigned int max_val;
		unsigned char bit_depth;
		unsigned char colour_type;
		unsigned char filter_type;
		unsigned int width;
		unsigned int height;
		ofstream *image;
		
		/** Constructor declaration */
		LTPNG(unsigned char, unsigned char, unsigned char);
		
		/** Main function declaration */
		void create_image(ofstream &, unsigned int, unsigned int, unsigned short *, unsigned short *, unsigned short *, unsigned short *);
		
		/** Channel map/ramp function declarations */
		static double ramp_n(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_s(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_e(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_w(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_nw(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_ne(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_sw(unsigned int, unsigned int, unsigned int, unsigned int);
		static double ramp_se(unsigned int, unsigned int, unsigned int, unsigned int);
		static double pattern_full(unsigned int, unsigned int, unsigned int, unsigned int);
		static double pattern_half(unsigned int, unsigned int, unsigned int, unsigned int);
		static double pattern_none(unsigned int, unsigned int, unsigned int, unsigned int);

	protected:
		/** Image data pointers */
		unsigned char *uncompressed_data;
		unsigned char *filtered_data;
		unsigned char *compressed_data;
		
		/** Buffer and index counter for holding CRC data */
		unsigned char *crc_buf;
		unsigned int crc_index;
		unsigned int crc_allocated;
		
		/** Table of CRCs of all 8-bit messages. */
		unsigned int crc_table[256];
		unsigned char crc_table_exists;
		
		/** PNG formatting helpers */
		void write_png_signature();
		void write_header_chunk(unsigned char, unsigned char, unsigned char);
		void write_data_chunk(unsigned char *, unsigned int);
		void write_end_chunk();
		
		/** Filter function declarations */
		unsigned char filter_byte(unsigned int, unsigned int, unsigned char);
		unsigned char paeth_predictor(short, short, short);
		
		/** CRC function declarations */
		void make_crc_table();
		void crc_init();
		void allocate_crc_size(unsigned char, unsigned char);
		unsigned int get_crc();
		unsigned int update_crc(unsigned int, unsigned char *, unsigned int);
		
		/** File I/O helper declarations */
		void fwrite_8(unsigned char);
		void fwrite_16(unsigned short);
		void fwrite_32(unsigned int);
		unsigned char get_byte_from_two_bytes(unsigned int, unsigned char);
		unsigned char get_byte_from_four_bytes(unsigned int, unsigned char);

		/** zLib function declarations */
		void def(unsigned char *, unsigned int, unsigned char *, unsigned int, unsigned int &, int);
		void inf(unsigned char *, unsigned int, unsigned char *, unsigned int, unsigned int &);
};

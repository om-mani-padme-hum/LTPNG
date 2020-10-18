/**
 * PNG Gradient
 *
 * A simple PNG file format command-line driven gradient maker.
 *
 * @author Rich Lowe
 */

/** Header includes */
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "LTPNG.h"

using namespace std;

/** Primary function declarations */
void create_gradient(string, unsigned int, unsigned int, unsigned char, unsigned char, string, string, string, string, unsigned char);
double get_pattern(string, unsigned int, unsigned int, unsigned int, unsigned int);
bool valid_pattern(string);
void usage();

/** Beginning of program */
int main(int argc, char **argv) {
	string filename;
	string red_pattern;
	string green_pattern;
	string blue_pattern;
	string alpha_pattern;
	int width = 0;
	int height = 0;
	int bit_depth = 8;
	int colour_type = 2;
	int filter_type = 4;
	int c;

	opterr = 0;

	/** Look for option switches */
	while ( (c = getopt(argc, argv, "f:d:p:a:w:h:r:g:b:t:")) != -1 ) {
		switch ( c ) {
			case 'f': filename = string(optarg); break;
			case 'd': bit_depth = atoi(optarg); break;
			case 'a': alpha_pattern = string(optarg); colour_type = 6; break;
			case 'w': width = atoi(optarg); break;
			case 'h': height = atoi(optarg); break;
			case 'r': red_pattern = string(optarg); break;
			case 'g': green_pattern = string(optarg); break;
			case 'b': blue_pattern = string(optarg); break;
			case 't': filter_type = atoi(optarg); break;
			case '?':
				if ( optopt == 'f' || optopt == 'd' || optopt == 'w' || optopt == 'h' || optopt == 'r' || optopt == 'g' || optopt == 'b' || optopt == 'b' )
					cout<<"png_gradient: Option -"<<static_cast<char>(optopt)<<" requires an argument."<<endl;
				else
					cout<<"png_gradient: Unknown option `-"<<static_cast<char>(optopt)<<"'."<<endl;
				return 1;
			default: abort();
		}
	}

	/** Verify width and height entered */
	if ( width <= 0 || height <= 0 ) {
		cout<<"png_gradient: please specify a valid width and height of the image."<<endl<<endl;
		usage();
		return 1;
	}

	/** Check for valid bit depth */
	if ( bit_depth != 8 && bit_depth != 16 ) {
		cout<<"png_gradient: only 8 and 16-bit depths are allowed."<<endl<<endl;
		usage();
		return 1;
	}

	/** Check for valid filter type */
	if ( filter_type < 0 || filter_type > 4 ) {
		cout<<"png_gradient: invalid filter type, only methods 0-4 are allowed."<<endl<<endl;
		usage();
		return 1;
	}

	/** Make sure filename was provided */
	if ( filename.length() <= 0 ) {
		cout<<"png_gradient: please specify a valid filename for the image."<<endl<<endl;
		usage();
		return 1;
	}

	/** Verify valid patterns are used */
	if ( !valid_pattern(red_pattern) || !valid_pattern(green_pattern) || !valid_pattern(blue_pattern) || (colour_type == 6 && !valid_pattern(alpha_pattern)) ) {
		cout<<"png_gradient: invalid pattern specified."<<endl<<endl;
		usage();
		return 1;
	}

	/** Try to create the gradient, report any errors */
	try {
		create_gradient(filename, width, height, bit_depth, colour_type, red_pattern, green_pattern, blue_pattern, alpha_pattern, filter_type);
	} catch ( const char *error ) {
		cout<<error<<endl;
		return 1;
	}

    return 0;
}

/** Create an example truecolour image with a gradient */
void create_gradient(string filename, unsigned int width, unsigned int height, unsigned char bit_depth, unsigned char colour_type, string red_pattern, string green_pattern, string blue_pattern, string alpha_pattern, unsigned char filter_type) {
	/** Self-allocate uncompressed reference channel arrays */
	unsigned short *red = new unsigned short[height*width];
	unsigned short *green = new unsigned short[height*width];
	unsigned short *blue = new unsigned short[height*width];
	unsigned short *alpha;

	/** If with alpha, allocate alpha array */
	if ( colour_type == 6 )
		alpha = new unsigned short[height*width];

	unsigned int row, col;
	
	/** Instantiate the image with the bit depth, colour type, and filter type */
	LTPNG image(bit_depth, colour_type, filter_type);

	/** Load the reference channel arrays with test pixels */
	for ( row = 0; row < height; row++ ) {
		for ( col = 0; col < width; col++ ) { /** r = s, g = se, b = nw is nice */
			red[row*width + col] = image.max_val*get_pattern(red_pattern, row, col, width, height);
			green[row*width + col] = image.max_val*get_pattern(green_pattern, row, col, width, height);
			blue[row*width + col] = image.max_val*get_pattern(blue_pattern, row, col, width, height);

			if ( colour_type == 6 )
				alpha[row*width + col] = image.max_val*get_pattern(alpha_pattern, row, col, width, height);
		}
	}

	/** Calculate pixel size */
	unsigned char pixel_size = colour_type == 2 ? 3 : 4;

	cout<<"Creating new "<<static_cast<unsigned int>(bit_depth)<<"-bit truecolour image";

	if ( colour_type == 6 )
		cout<<" with alpha";

	cout<<"..."<<endl;
	cout<<" Red pixel pattern: "<<red_pattern<<endl;
	cout<<" Green pixel pattern: "<<green_pattern<<endl;
	cout<<" Blue pixel pattern: "<<blue_pattern<<endl;

	if ( colour_type == 6 )
		cout<<" Alpha pixel pattern: "<<alpha_pattern<<endl;

	cout<<" Number of pixels/channel size: "<<(height*width)<<endl;
	cout<<" Pixel width: "<<width<<endl;
	cout<<" Pixel height: "<<height<<endl;
	cout<<" Pixel size: "<<(pixel_size*(3-16/bit_depth))<<endl;
	cout<<" Scan line size: "<<(width*pixel_size*(3-16/bit_depth) + 1)<<endl;
	cout<<" Total uncompressed image data size: "<<(width*height*pixel_size*(3-16/bit_depth) + height + 1)<<endl;

	/** Image file */
	ofstream file(filename, ios::binary);

	/** Pass file and create image */
	image.create_image(file, width, height, red, green, blue, alpha);

	/** Close the image file */
	file.close();

	cout<<" Total compressed image data size: "<<image.file_size<<endl<<endl;

	cout<<"Done!"<<endl;

	/** Clean up self-allocated memory */
	delete[] red;
	delete[] green;
	delete[] blue;

	/** Clean up self-allocated memory if with alpha */
	if ( colour_type == 6 )
		delete[] alpha;
}

/** Return channel pattern multiplier for pixel */
double get_pattern(string pattern, unsigned int row, unsigned int col, unsigned int width, unsigned int height) {
	if ( !pattern.compare("n") ) return LTPNG::ramp_n(row, col, width, height);
	else if ( !pattern.compare("e") ) return LTPNG::ramp_e(row, col, width, height);
	else if ( !pattern.compare("s") ) return LTPNG::ramp_s(row, col, width, height);
	else if ( !pattern.compare("w") ) return LTPNG::ramp_w(row, col, width, height);
	else if ( !pattern.compare("nw") ) return LTPNG::ramp_nw(row, col, width, height);
	else if ( !pattern.compare("ne") ) return LTPNG::ramp_ne(row, col, width, height);
	else if ( !pattern.compare("se") ) return LTPNG::ramp_se(row, col, width, height);
	else if ( !pattern.compare("sw") ) return LTPNG::ramp_sw(row, col, width, height);
	else if ( !pattern.compare("full") ) return LTPNG::pattern_full(row, col, width, height);
	else if ( !pattern.compare("half") ) return LTPNG::pattern_half(row, col, width, height);
	else if ( !pattern.compare("none") ) return LTPNG::pattern_none(row, col, width, height);

	throw "png_gradient: Invalid pattern made it to get_pattern().";
}

/** Validate channel pattern */
bool valid_pattern(string pattern) {
	if ( !pattern.compare("n") ) return true;
	else if ( !pattern.compare("s") ) return true;
	else if ( !pattern.compare("e") ) return true;
	else if ( !pattern.compare("w") ) return true;
	else if ( !pattern.compare("nw") ) return true;
	else if ( !pattern.compare("ne") ) return true;
	else if ( !pattern.compare("se") ) return true;
	else if ( !pattern.compare("sw") ) return true;
	else if ( !pattern.compare("full") ) return true;
	else if ( !pattern.compare("half") ) return true;
	else if ( !pattern.compare("none") ) return true;

	return false;
}

/** Print usage instructions */
void usage() {
	cout<<"Usage: png_gradient [options]"<<endl<<endl;
	cout<<"  -f FILENAME   Desired filename of the generated PNG image"<<endl;
	cout<<"  -w WIDTH      Specifies the width of image in pixels"<<endl;
	cout<<"  -h HEIGHT     Specifies the height of image in pixels"<<endl;
	cout<<"  -d DEPTH      Can be 8 or 16-bit pixel channel sizes [optional]"<<endl;
	cout<<"  -t FILTER     Can be 0 = None, 1 = Sub, 2 = Up, 3 = Average, 4 = Paeth [optional]"<<endl;
	cout<<"  -r PATTERN    Red pattern"<<endl;
	cout<<"  -g PATTERN    Green pattern"<<endl;
	cout<<"  -b PATTERN    Blue pattern"<<endl;
	cout<<"  -a PATTERN    Alpha pattern [optional]"<<endl<<endl;
	cout<<"Valid patterns:"<<endl;
	cout<<"  n             Increase expression from south to north"<<endl;
	cout<<"  e             Increase expression from west to east"<<endl;
	cout<<"  s             Increase expression from north to south"<<endl;
	cout<<"  w             Increase expression from east to west"<<endl;
	cout<<"  nw            Increase expression from southeast to northwest"<<endl;
	cout<<"  ne            Increase expression from southwest to northeast"<<endl;
	cout<<"  se            Increase expression from northwest to southeast"<<endl;
	cout<<"  sw            Increase expression from northeast to southwest"<<endl;
	cout<<"  full          Constant full expression"<<endl;
	cout<<"  half          Constant half expression"<<endl;
	cout<<"  none          Constant absent expression"<<endl<<endl;
}

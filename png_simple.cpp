/**
 * Simplest PNG Gradient
 *
 * A very simple LTPNG example.
 *
 * @author Rich Lowe
 */
 
/** Header includes */
#include <iostream>
#include <fstream>
#include "LTPNG.h"

using namespace std;

/** Beginning of program */
int main(int argc, char **argv) {
	unsigned short red[1280*800];
	unsigned short green[1280*800];
	unsigned short blue[1280*800];
	unsigned short *alpha;
	unsigned int row, col;
	unsigned short imprint_row, imprint_col;
	
	/** Instantiate the image with the bit depth and colour type */
	LTPNG image(16, 2, 4);
	
	/** Load the reference channel arrays with pixels */
	for ( row = 0; row < 800; row++ ) {		
		for ( col = 0; col < 1280; col++ ) { /** r = s, g = se, b = nw is nice */
			red[row*1280 + col] = image.max_val*LTPNG::ramp_s(row, col, 1280, 800);
			green[row*1280 + col] = image.max_val*LTPNG::ramp_se(row, col, 1280, 800);
			blue[row*1280 + col] = image.max_val*LTPNG::ramp_nw(row, col, 1280, 800);
		}
	}
	
	/** Image file */
	ofstream file("test.png", ios::binary);
	
	image.create_image(file, 1280, 800, red, green, blue, alpha);
		
	/** Close the image file */
	file.close();
	
	cout<<"Done! Total compressed image data size: "<<image.file_size<<" bytes"<<endl<<endl;
	
    return 0;
}
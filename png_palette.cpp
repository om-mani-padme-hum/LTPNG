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
#include <cmath>
#include "LTPNG.h"

using namespace std;

/** Beginning of program */
int main(int argc, char **argv) {
	unsigned short red[512*512];
	unsigned short green[512*512];
	unsigned short blue[512*512];
	unsigned short *alpha;
	unsigned int r = 0, g = 0, b = 0, row = 0, col = 0;
	unsigned short imprint_row, imprint_col;
    double theta, magnitude;
	
	/** Instantiate the image with the bit depth and colour type */
	LTPNG image(16, 2, 4);
	
	/** Load the reference channel arrays with pixels */
	for ( row = 0; row < 512; row++ ) {
        for ( r = 0; r < 256; r += 32 ) {
            for ( g = 0; g < 256; g += 32 ) {
                for ( b = 0; b < 256; b += 32 ) {
                    magnitude = row / 511.0;

                    red[row*512 + col] = image.max_val * magnitude * r / 255;
                    green[row*512 + col] = image.max_val * magnitude * g / 255;
                    blue[row*512 + col] = image.max_val * magnitude * b / 255;

                    col++;
                }
            }
		}
      
        col = 0;
	}
	
	/** Image file */
	ofstream file("test_palette.png", ios::binary);
	
	image.create_image(file, 512, 512, red, green, blue, alpha);
		
	/** Close the image file */
	file.close();
	
	cout<<"Done! Total compressed image data size: "<<image.file_size<<" bytes"<<endl<<endl;
	
    return 0;
}
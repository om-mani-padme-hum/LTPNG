all:
	g++ -lz -o png_gradient LTPNG.cpp png_gradient.cpp
	g++ -lz -o png_simple LTPNG.cpp png_simple.cpp
	g++ -lz -o png_imprint LTPNG.cpp png_imprint.cpp


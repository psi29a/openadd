#ifndef BITMAP_HPP
#define BITMAP_HPP

#define BMPHEADEROFFSET 14

#include <stdint.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <SDL2/SDL.h>

namespace GRAPHICS {

/*
 struct BMP
 {
 uint16_t fileSize;
 uint16_t unknown1;
 uint16_t numberOfSubImages;
 uint16_t unknown3;
 uint16_t unknown4;
 uint16_t width;
 uint16_t height;
 uint8_t compressedData[0];
 };
 */

struct SubBitmap {
	uint16_t width;
	uint16_t height;
	std::vector<uint8_t> subBitmap;
};

class Bitmap {
public:
	Bitmap(const std::vector<uint8_t> &vec);
	virtual ~Bitmap();
	const uint8_t& operator[](uint16_t index);
	uint16_t getWidth(uint16_t index) ;
	uint16_t getHeight(uint16_t index) ;
	uint16_t getNumberOfBitmaps();
private:
	uint16_t mNumSubBitmaps;
	std::map< uint16_t, SubBitmap > mSubBitmaps;
};

}
#endif //BITMAP_HPP
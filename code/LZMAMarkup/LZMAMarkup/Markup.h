#pragma once

#include "LZMA.h"

namespace markup
{
	extern string renderType;
	extern string renderMode;
}

void readMarkupParameters(string path);

void readCharSizes(string path);
void setOutputCodes();


void decompressAndMarkText(string path, uint& textLen, uchar*& text, uchar*& marks);

void decompressAndEntropyMarkText(string path, uint& textLen, uchar*& text, uchar*& marks, uchar*& colorTypes);


void createHTMLPages(string folderPath, uint textLen, uchar* text, uchar* marks);

void createHTMLPages16(string folderPath, uint textLen, uchar* text, uchar* marks);

void createHTMLPagesEntropy(string folderPath, uint textLen, uchar* text, uchar* marks, uchar* colorTypes);

void createHTMLPagesEntropy16(string folderPath, uint textLen, uchar* text, uchar* colorTypes);
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

void decompressAndCompareMarkText(string path1, string path2, uint& textLen, uchar*& text, 
	uchar*& marks, uchar*& colorTypes);

void createHTMLPages(string folderPath, uint textLen, uchar* text, uchar* marks);

void createHTMLPages16(string folderPath, uint textLen, uchar* text, uchar* marks);

void createHTMLPagesEntropy(string folderPath, uint textLen, uchar* text, uchar* marks, uchar* colorTypes);

void createHTMLPagesEntropy16(string folderPath, uint textLen, uchar* text, uchar* colorTypes);

void createHTMLPagesCompare(string folderPath, uint textLen, uchar* text, uchar* marks, uchar* colorTypes);

void createHTMLPagesCompare16(string folderPath, uint textLen, uchar* text, uchar* colorTypes);
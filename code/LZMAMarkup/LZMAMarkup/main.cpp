#include "Markup.h"

/*
* parameters:
* lzma compressed file
* folder with html files, default = ".\" (ensure folder already exist)
* file with characters size information from web console, default = "CharSizeConsole.log"
* file with rendering parameters like colors, default = "Settings.txt"
*/

void show()
{
	cout << "parameters:\n";
	cout << "lzma compressed file\n";
	cout << "folder with html files, default = \".\\\" (ensure folder already exist)\n";
	cout << "file with characters size information from web console, default = \"CharSizeConsole.log\"\n";
	cout << "file with rendering parameters like colors, default = \"Settings.txt\"\n";
}

int main(int argc, char** argv)
{
	string lzmaPath;
	string folderPath = ".\\";
	string charSizesPath = "CharSizeConsole.log";
	string settingsPath = "Settings.txt";

	if (argc < 2 || argc > 5)
	{
		show();

		return 1;
	}

	lzmaPath = string(argv[1]);
	if (argc >= 3)
	{
		folderPath = string(argv[2]);
	}
	if (argc >= 4)
	{
		charSizesPath = string(argv[3]);
	}
	if (argc >= 5)
	{
		settingsPath = string(argv[4]);
	}

	readMarkupParameters(settingsPath);
	readCharSizes(charSizesPath);
	setOutputCodes();

	if (markup::renderMode == "blocks")
	{
		uint textLen;
		uchar* text;
		uchar* marks;
		decompressAndMarkText(lzmaPath, textLen, text, marks);

		if (markup::renderType == "general")
		{
			createHTMLPages(folderPath, textLen, text, marks);
		}
		else if (markup::renderType == "hexadecimal")
		{
			createHTMLPages16(folderPath, textLen, text, marks);
		}
		else
		{
			exit(7);
		}

		delete[] text;
		delete[] marks;
	}
	else if (markup::renderMode == "entropy")
	{
		uint textLen;
		uchar* text;
		uchar* marks;
		uchar* colorTypes;

		decompressAndEntropyMarkText(lzmaPath, textLen, text, marks, colorTypes);
		if (markup::renderType == "general")
		{
			createHTMLPagesEntropy(folderPath, textLen, text, marks, colorTypes);
		}
		else if (markup::renderType == "hexadecimal")
		{
			createHTMLPagesEntropy16(folderPath, textLen, text, colorTypes);
		}
		else
		{
			exit(8);
		}

		delete[] text;
		delete[] marks;
		delete[] colorTypes;
	}
	else
	{
		exit(9);
	}

	return 0;
}
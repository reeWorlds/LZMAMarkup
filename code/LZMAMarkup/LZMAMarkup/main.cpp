#include "Markup.h"

void show()
{
	cout << "parameters:\n";
	cout << "\"s\" for markup of single file or \"d\" for comparison markup of 2 files\n";
	cout << "lzma compressed file\n";
	cout << "second lzma compressed file if \"d\" option is choosen or skip this parameter otherwise\n";
	cout << "folder with html files, default = \".\\\" (ensure folder already exist)\n";
	cout << "file with characters size information from web console, default = \"CharSizeConsole.log\"\n";
	cout << "file with rendering parameters like colors, default = \"Settings.txt\"\n";
}

int main(int argc, char** argv)
{
	string lzmaPath;
	string lzmaPath2; // optional
	string folderPath = ".\\";
	string charSizesPath = "CharSizeConsole.log";
	string settingsPath = "Settings.txt";

	if (argc < 2)
	{
		show();

		return -1;
	}

	if (argv[1][0] == 's')
	{
		if (argc < 3 || argc > 6)
		{
			show();

			return 1;
		}

		lzmaPath = string(argv[2]);
		if (argc >= 4)
		{
			folderPath = string(argv[3]);
		}
		if (argc >= 5)
		{
			charSizesPath = string(argv[4]);
		}
		if (argc >= 6)
		{
			settingsPath = string(argv[5]);
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
	}
	else if (argv[1][0] == 'd')
	{
		if (argc < 4 || argc > 7)
		{
			show();

			return 1;
		}

		lzmaPath = string(argv[2]);
		lzmaPath2 = string(argv[3]);
		if (argc >= 5)
		{
			folderPath = string(argv[4]);
		}
		if (argc >= 6)
		{
			charSizesPath = string(argv[5]);
		}
		if (argc >= 7)
		{
			settingsPath = string(argv[6]);
		}

		readMarkupParameters(settingsPath);
		readCharSizes(charSizesPath);
		setOutputCodes();

		uint textLen;
		uchar* text;

		uchar* marks;
		uchar* colorTypes;

		decompressAndCompareMarkText(lzmaPath, lzmaPath2, textLen, text, marks, colorTypes);
		if (markup::renderType == "general")
		{
			createHTMLPagesCompare(folderPath, textLen, text, marks, colorTypes);
		}
		else if (markup::renderType == "hexadecimal")
		{
			createHTMLPagesCompare16(folderPath, textLen, text, colorTypes);
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
		show();
	}

	return 0;
}
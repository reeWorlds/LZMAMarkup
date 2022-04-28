#include "Markup.h"

namespace markup
{
	// "general" renders visible characters as is and invisible as '\???', not width-fixed
	// "hexadecimal" renders each character as 2 symbols 0-9-A-F, width-fixed
	string renderType;

	// "blocks" renders background depending on what was encoded, like literal, match, kind of repeat
	// "entropy" renders background depending on number of bits used to encode symbol
	string renderMode;

	string textFont; // e. g. "\"24px_Arial_Bold\"", '_' will be replaced with spaces

	int maxCharactersPerLine; // e. g. 100
	int maxLinesPerPage; // e. g. 1000

	int ignoreEndLine; // if != 0, line spliting ignores '\n'

	int horisontalBlockShiftLeft; // pixels multiplied by 10, e. g 20
	int horisontalBlockShiftRight; // pixels multiplied by 10, e. g. 20

	int verticalBlockShiftUp; // pixels, e. g. 2
	int verticalBlockShiftDown; // pixels e. g. 2

	// color examples: black, red, #F4D03F, #33FFD2
	string literalColor[2]; // one encoded literal
	string matchColor[2]; // match encoded as length + offset
	string shortRepeatColor[2]; // repeat one literal coressponding to last offset
	string longRepeat0Color[2]; // repeat match coressponding to last offset
	string longRepeat1Color[2]; // repeat match coressponding to second last offset
	string longRepeat2Color[2]; // repeat match coressponding to third last offset
	string longRepeat3Color[2]; // repeat match coressponding to fourth last offset
	string textColor; // color of text, e. g. black

	int entropySaturation; // hsl(,*,), from 0 to 100
	int entropyLightness; // hsl(,,*), from 0 to 100
}

int charWidth[256]; // multiplied by 10 (for 0.1 pixel accuracy)
int charMaxHeightAscent, charMaxHeightDescent, charMaxFullHeight;
int charWidth16, charWidth16_2, charHeight16;

int outputCodesWidth[256]; // width of each code = sum of widths of each cahracter
// e. g. width(9) == width("\\t") = charWidth['\\'] + charWidth['t']
string outputCodes[256]; // representation for each character, e. g. g for 103 or \t for 9
string outputCodesString[256]; // string outputed to *.js file, e. g. "\\157" for 157 or "\\t" for 9
string outputCodes16[256]; // representtion of each character in 

void readMarkupParameters(string path)
{
	using namespace markup;

	string dummy;

	ifstream in(path);

	if (!in.is_open())
	{
		exit(3);
	}

	in >> dummy >> renderType;
	in >> dummy >> renderMode;

	in >> dummy >> textFont;
	for (auto& character : textFont)
	{
		if (character == '_')
		{
			character = ' ';
		}
	}

	in >> dummy >> maxCharactersPerLine;
	in >> dummy >> maxLinesPerPage;

	in >> dummy >> ignoreEndLine;

	in >> dummy >> horisontalBlockShiftLeft;
	in >> dummy >> horisontalBlockShiftRight;

	in >> dummy >> verticalBlockShiftUp;
	in >> dummy >> verticalBlockShiftDown;

	in >> dummy >> literalColor[0] >> dummy >> literalColor[1];
	in >> dummy >> matchColor[0] >> dummy >> matchColor[1];
	in >> dummy >> shortRepeatColor[0] >> dummy >> shortRepeatColor[1];
	in >> dummy >> longRepeat0Color[0] >> dummy >> longRepeat0Color[1];
	in >> dummy >> longRepeat1Color[0] >> dummy >> longRepeat1Color[1];
	in >> dummy >> longRepeat2Color[0] >> dummy >> longRepeat2Color[1];
	in >> dummy >> longRepeat3Color[0] >> dummy >> longRepeat3Color[1];
	in >> dummy >> textColor;

	in >> dummy >> entropySaturation;
	in >> dummy >> entropyLightness;

	in.close();
}

void readCharSizes(string path)
{
	for (int i = 0; i < 256; i++)
	{
		charWidth[i] = -BIGNUM;
	}

	uchar base16Characters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	uchar isHexadecimal[256];
	for (auto& i : isHexadecimal)
	{
		i = 0;
	}
	for (auto& character : base16Characters)
	{
		isHexadecimal[character] = 1;
	}

	string lineInfo;
	uchar character;
	double width;
	int heightAscent, heightDescent, _width;

	ifstream in(path);

	if (!in.is_open())
	{
		exit(2);
	}

	while (in >> lineInfo >> character >> width >> heightAscent >> heightDescent)
	{
		_width = int(width * 10) + 1;

		charMaxHeightAscent = max(charMaxHeightAscent, heightAscent);
		charMaxHeightDescent = max(charMaxHeightDescent, heightDescent);

		charMaxFullHeight = max(charMaxFullHeight, heightAscent + heightDescent);

		charWidth[character] = _width;

		if (isHexadecimal[character] == 1)
		{
			charWidth16 = max(charWidth16, _width);
			charWidth16_2 = charWidth16 * 2;
			charHeight16 = max(charHeight16, heightAscent);
		}
	}
	charWidth[(uchar)'`'] = charWidth['-'];

	in.close();
}

void setOutputCodes()
{
	for (int i = 0; i < 256; i++)
	{
		if (charWidth[i] != -BIGNUM)
		{
			outputCodes[i] = string(1, char(i));
		}
	}
	for (int i = 127; i < 256; i++)
	{
		outputCodes[i] = "\\???";
		outputCodes[i][1] = i / 100 + '0';
		outputCodes[i][2] = (i / 10) % 10 + '0';
		outputCodes[i][3] = i % 10 + '0';
	}
	pair<char, string> extraOutputCodes[] = { {0, "\\0"}, {1, "\\x1"}, {2, "\\x2"}, {3, "\\x3"}, {4, "\\x4"}, 
		{5, "\\x5"}, {6, "\\x6"}, {7, "\\a"}, {8, "\\b"}, {9, "\\t"}, {10, "\\n"}, {11, "\\v"}, {12, "\\f"},
		{13, "\\r"}, {14, "\\xe"}, {15, "\\xf"}, {16, "\\x10"}, {17, "\\x11"}, {18, "\\x12"}, {19, "\\x13"},
		{20, "\\x14"}, {21, "\\x15"}, {22, "\\x16"}, {23, "\\x17"}, {24, "\\x18"}, {25, "\\x19"}, {26, "\\x1a"},
		{27, "\\x1b"}, {28, "\\x1c"}, {29, "\\x1d"}, {30, "\\x1e"}, {31, "\\x1f"}, {32, "`"}, {(int)'`', "\\x96"}};
	for (auto& p : extraOutputCodes)
	{
		outputCodes[p.first] = p.second;
	}

	for (int i = 0; i < 256; i++)
	{
		for (auto& character : outputCodes[i])
		{
			outputCodesWidth[i] += charWidth[character];

			if (character == '\\')
			{
				outputCodesString[i] += "\\\\";
			}
			else if (character == '\'')
			{
				outputCodesString[i] += "\\\'";
			}
			else if (character == '\"')
			{
				outputCodesString[i] += "\\\"";
			}
			else
			{
				outputCodesString[i] += character;
			}
		}
	}

	uchar base16Characters[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	for (int i = 0; i < 256; i++)
	{
		outputCodes16[i] = "??";
		outputCodes16[i][0] = base16Characters[i / 16];
		outputCodes16[i][1] = base16Characters[i % 16];
	}
}

void decompressAndMarkText(string path, uint& textLen, uchar*& text, uchar*& marks)
{
	FILE* in = fopen(path.c_str(), "rb");

	if (in == NULL)
	{
		exit(4);
	}

	LZMA* lzma = new LZMA(in);

	lzma->decodeText(textLen, text, marks);

	delete lzma;

	fclose(in);
}

void decompressAndEntropyMarkText(string path, uint& textLen, uchar*& text, uchar*& marks, uchar*& colorTypes)
{
	FILE* in = fopen(path.c_str(), "rb");

	if (in == NULL)
	{
		exit(4);
	}

	LZMA* lzma = new LZMA(in);

	lzma->entropydecodeText(textLen, text, marks, colorTypes);

	delete lzma;

	fclose(in);
}

#define start first
#define end second

void separateTextIntoLines(uint textLen, uchar* text, vector<vector<pair<int, int> > >& pages)
{
	using namespace markup;

	// separate text into pages and lines
	int leftPosition = 0;
	pages.push_back(vector <pair<int, int> >());
	for (int i = 0; i < textLen; i++)
	{
		if (ignoreEndLine == 0 && text[i] == '\n')
		{
			pages.back().push_back({ leftPosition, i });
			leftPosition = i + 1;
		}
		else
		{
			if (i - leftPosition + 1 == maxCharactersPerLine)
			{
				pages.back().push_back({ leftPosition, i });
				leftPosition = i + 1;
			}
		}

		if (pages.back().size() == maxLinesPerPage)
		{
			pages.push_back(vector <pair<int, int> >());
		}
	}
	if (pages.back().size() == 0)
	{
		pages.back().pop_back();
	}
}

void findWidthHeight(vector<pair<int, int> >& page, uchar* text, uchar* marks, int& pageWidth, int& pageHeight)
{
	using namespace markup;

	int horisontalBlockShift = horisontalBlockShiftLeft + horisontalBlockShiftRight;

	for (auto& line : page)
	{
		int myWidth = 0;
		int countBlocks = 1;

		for (int i = line.start; i <= line.end; i++)
		{
			myWidth += outputCodesWidth[text[i]];
			
			if (i != line.start && marks[i - 1] != marks[i])
			{
				countBlocks++;
			}
		}

		// remember width is multiplied by 10
		pageWidth = max(pageWidth, (myWidth + countBlocks * horisontalBlockShift) / 10 + 1);
	}

	pageHeight = page.size() * (charMaxFullHeight + verticalBlockShiftUp + verticalBlockShiftDown);
}

void writeHTMLPage(string path, int pageWidth, int pageHeight, int pageNumber, int totalPages)
{
	ofstream out(path);

	if (!out.is_open())
	{
		exit(5);
	}

	set <int> linkedPages;

	if (pageNumber != 1) { linkedPages.insert(1); }
	if (pageNumber >= 101) { linkedPages.insert(pageNumber - 100); }
	if (pageNumber >= 11) { linkedPages.insert(pageNumber - 10); }
	if (pageNumber >= 4) { linkedPages.insert(pageNumber - 3); }
	if (pageNumber >= 3) { linkedPages.insert(pageNumber - 2); }
	if (pageNumber >= 2) { linkedPages.insert(pageNumber - 1); }
	if (pageNumber + 1 <= totalPages) { linkedPages.insert(pageNumber + 1); }
	if (pageNumber + 2 <= totalPages) { linkedPages.insert(pageNumber + 2); }
	if (pageNumber + 3 <= totalPages) { linkedPages.insert(pageNumber + 3); }
	if (pageNumber + 10 <= totalPages) { linkedPages.insert(pageNumber + 10); }
	if (pageNumber + 100 <= totalPages) { linkedPages.insert(pageNumber + 100); }
	if (pageNumber != totalPages) { linkedPages.insert(totalPages); }

	out << "<!DOCTYPE html>\n";
	out << "<html>\n";
	out << "<body>\n\n";

	out << "<h1>Page " << pageNumber << "</h1>\n";

	for (auto index : linkedPages)
	{
		out << "<p><a href = \"page" << index << ".html\">Page " << index << "</a></p>\n";
	}
	out << "\n";

	out << "<div style=\"font-size:125%; color:" <<  markup::textColor << "\">\n";
	out << "<div style=\"background-color:" << markup::literalColor[0] << "; \">Literal</div>\n";
	out << "<div style=\"background-color:" << markup::literalColor[1] << "; \">Literal</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<div style=\"background-color:" << markup::matchColor[0] << "; \">Match</div>\n";
	out << "<div style=\"background-color:" << markup::matchColor[1] << "; \">Match</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<div style=\"background-color:" << markup::shortRepeatColor[0] << "; \">ShortRepeat</div>\n";
	out << "<div style=\"background-color:" << markup::shortRepeatColor[1] << "; \">ShortRepeat</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat0Color[0] << "; \">longRepeat0</div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat0Color[1] << "; \">longRepeat0</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat1Color[0] << "; \">longRepeat1</div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat1Color[1] << "; \">longRepeat1</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat2Color[0] << "; \">longRepeat2</div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat2Color[1] << "; \">longRepeat2</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat3Color[0] << "; \">longRepeat3</div>\n";
	out << "<div style=\"background-color:" << markup::longRepeat3Color[1] << "; \">longRepeat3</div>\n";
	out << "<div style=\"font-size:25%;\"><br></div>\n";
	out << "<\div>";
	
	out << "<canvas id = \"canvas\" width = \"" << pageWidth << "px\" height = \""
		<< pageHeight << "px\"></canvas>\n\n";

	out << "<script src = \"script" << pageNumber << ".js\"></script>\n";
	out << "<script>\n";
	out << "\tdraw();\n";
	out << "</script>\n\n";

	out << "</body>\n";
	out << "</html>\n";

	out.close();
}

string splitDigit(int n)
{
	string res = to_string(n / 10);
	res += ".";
	res += char((n % 10) + '0');

	return res;
}

void writeDrawScript(string path, vector <pair<int, int> >& page, uchar* text, uchar* marks)
{
	using namespace markup;

	const int totalBlockTypes = 14;

	ofstream out(path);

	if (!out.is_open())
	{
		exit(6);
	}

	tuple<string, int, int> tmpTextTuple;
	vector <tuple<string, int, int> > textsToDraw; // {text, x, y}

	tuple<int, int, int, int> tmpBlockTuple;
	vector <tuple<int, int, int, int> > blocksCoordinateToDraw[totalBlockTypes]; // {x, y, width, height}
	
	string blocksColorToDraw[totalBlockTypes];

	for (int lineI = 0; lineI < page.size(); lineI++)
	{
		pair<int, int> line = page[lineI];

		get<1>(tmpBlockTuple) = lineI * (charMaxFullHeight + verticalBlockShiftUp + verticalBlockShiftDown);
		get<3>(tmpBlockTuple) = charMaxFullHeight + verticalBlockShiftUp + verticalBlockShiftDown;
		get<2>(tmpTextTuple) = get<1>(tmpBlockTuple) + verticalBlockShiftUp + charMaxHeightAscent;

		int x = 0, blockTextWidth = 0;
		
		string blockText = outputCodesString[text[line.start]];
		blockTextWidth += outputCodesWidth[text[line.start]];
		
		for (int i = line.start + 1; i <= line.end; i++)
		{
			if (marks[i - 1] != marks[i])
			{
				get<0>(tmpTextTuple) = blockText;
				get<1>(tmpTextTuple) = x + horisontalBlockShiftLeft;
				textsToDraw.push_back(tmpTextTuple);

				get<0>(tmpBlockTuple) = x;
				get<2>(tmpBlockTuple) = blockTextWidth + horisontalBlockShiftLeft + horisontalBlockShiftRight;
				blocksCoordinateToDraw[marks[i - 1]].push_back(tmpBlockTuple);

				x += blockTextWidth + horisontalBlockShiftLeft + horisontalBlockShiftRight;
				blockText = outputCodesString[text[i]];
				blockTextWidth = outputCodesWidth[text[i]];
			}
			else
			{
				blockText += outputCodesString[text[i]];
				blockTextWidth += outputCodesWidth[text[i]];
			}
		}
		get<0>(tmpTextTuple) = blockText;
		get<1>(tmpTextTuple) = x + horisontalBlockShiftLeft;
		textsToDraw.push_back(tmpTextTuple);

		get<0>(tmpBlockTuple) = x;
		get<2>(tmpBlockTuple) = blockTextWidth + horisontalBlockShiftLeft + horisontalBlockShiftRight;
		blocksCoordinateToDraw[marks[line.end]].push_back(tmpBlockTuple);
	}

	blocksColorToDraw[0] = literalColor[0]; blocksColorToDraw[1] = literalColor[1];
	blocksColorToDraw[2] = matchColor[0]; blocksColorToDraw[3] = matchColor[1];
	blocksColorToDraw[4] = shortRepeatColor[0]; blocksColorToDraw[5] = shortRepeatColor[1];
	blocksColorToDraw[6] = longRepeat0Color[0]; blocksColorToDraw[7] = longRepeat0Color[1];
	blocksColorToDraw[8] = longRepeat1Color[0]; blocksColorToDraw[9] = longRepeat1Color[1];
	blocksColorToDraw[10] = longRepeat2Color[0]; blocksColorToDraw[11] = longRepeat2Color[1];
	blocksColorToDraw[12] = longRepeat3Color[0]; blocksColorToDraw[13] = longRepeat3Color[1];

	out << "function draw()\n";
	out << "{\n";
	out << "\tlet canvas = document.getElementById(\"canvas\");\n";
	out << "\tlet ctx = canvas.getContext(\"2d\");\n";
	out << "\tctx.font = " << textFont << "; \n\n";

	out << "\tfunction t(s, x, y) {ctx.fillText(s, x, y);}\n";
	out << "\tfunction r(x, y, w, h) {ctx.fillRect(x, y, w, h);}\n";
	out << "\tfunction s(c) {ctx.fillStyle = c;}\n\n";

	for (int blockTypeI = 0; blockTypeI < totalBlockTypes; blockTypeI++)
	{
		out << "\ts(\"" << blocksColorToDraw[blockTypeI] << "\");\n";

		for (auto& blockTuple : blocksCoordinateToDraw[blockTypeI])
		{
			out << "\tr(" << splitDigit(get<0>(blockTuple)) << "," << get<1>(blockTuple)
				<< "," << splitDigit(get<2>(blockTuple)) << "," << get<3>(blockTuple) << ")\n";
		}

		out << "\n";
	}

	out << "\ts(\"" << textColor << "\");\n";
	for (auto& textTuple : textsToDraw)
	{
		out << "\tt(\'" << get<0>(textTuple) << "\'," << splitDigit(get<1>(textTuple)) 
			<< "," << get<2>(textTuple) << ")\n";
	}

	out << "}";

	out.close();
}

void createHTMLPages(string folderPath, uint textLen, uchar* text, uchar* marks)
{
	// Full text consist of pages, page consist of lines, line consist of characters text[line.start : line.end]
	vector <vector <pair<int, int> > > pages;

	separateTextIntoLines(textLen, text, pages);

	// process each page separately 
	for (int pageI = 1; pageI <= pages.size(); pageI++)
	{
		vector<pair<int, int> >& page = pages[pageI - 1];

		int pageWidth = 0, pageHeight = 0;
		findWidthHeight(page, text, marks, pageWidth, pageHeight);

		writeHTMLPage(folderPath + "page" + to_string(pageI) + ".html", pageWidth, pageHeight, pageI, pages.size());

		writeDrawScript(folderPath + "script" + to_string(pageI) + ".js", page, text, marks);
	}
}

void findWidthHeight16(vector<pair<int, int> >& page, uchar* text, int& pageWidth, int& pageHeight)
{
	using namespace markup;

	int horisontalBlockShift = horisontalBlockShiftLeft + horisontalBlockShiftRight;

	for (auto& line : page)
	{
		int myWidth = (line.end - line.start + 1) * (charWidth16_2 + horisontalBlockShift);

		// remember width is multiplied by 10
		pageWidth = max(pageWidth, myWidth / 10 + 1);
	}

	pageHeight = page.size() * (charHeight16 + verticalBlockShiftUp + verticalBlockShiftDown);
}

void writeDrawScript16(string path, vector <pair<int, int> >& page, uchar* text, uchar* marks)
{
	using namespace markup;

	const int totalBlockTypes = 14;

	ofstream out(path);

	if (!out.is_open())
	{
		exit(6);
	}

	tuple<string, int, int> tmpTextTuple;
	vector <tuple<string, int, int> > textsToDraw; // {text, x, y}

	tuple<int, int> tmpBlockTuple;
	vector <tuple<int, int> > blocksCoordinateToDraw[totalBlockTypes]; // {x, y, width, height}

	string blocksColorToDraw[totalBlockTypes];

	int blockWidth = charWidth16_2 + horisontalBlockShiftLeft + horisontalBlockShiftRight;
	int blockHeight = charHeight16 + verticalBlockShiftUp + verticalBlockShiftDown;

	for (int lineI = 0; lineI < page.size(); lineI++)
	{
		pair<int, int> line = page[lineI];

		get<1>(tmpBlockTuple) = lineI * blockHeight;
		get<2>(tmpTextTuple) = get<1>(tmpBlockTuple) + verticalBlockShiftUp + charMaxHeightAscent;

		for (int i = line.start; i <= line.end; i++)
		{
			get<0>(tmpTextTuple) = outputCodes16[text[i]];
			get<1>(tmpTextTuple) = (i - line.start) * blockWidth + horisontalBlockShiftLeft;
			textsToDraw.push_back(tmpTextTuple);

			get<0>(tmpBlockTuple) = (i - line.start) * blockWidth;
			blocksCoordinateToDraw[marks[i]].push_back(tmpBlockTuple);
		}
	}

	blocksColorToDraw[0] = literalColor[0]; blocksColorToDraw[1] = literalColor[1];
	blocksColorToDraw[2] = matchColor[0]; blocksColorToDraw[3] = matchColor[1];
	blocksColorToDraw[4] = shortRepeatColor[0]; blocksColorToDraw[5] = shortRepeatColor[1];
	blocksColorToDraw[6] = longRepeat0Color[0]; blocksColorToDraw[7] = longRepeat0Color[1];
	blocksColorToDraw[8] = longRepeat1Color[0]; blocksColorToDraw[9] = longRepeat1Color[1];
	blocksColorToDraw[10] = longRepeat2Color[0]; blocksColorToDraw[11] = longRepeat2Color[1];
	blocksColorToDraw[12] = longRepeat3Color[0]; blocksColorToDraw[13] = longRepeat3Color[1];

	out << "function draw()\n";
	out << "{\n";
	out << "\tlet canvas = document.getElementById(\"canvas\");\n";
	out << "\tlet ctx = canvas.getContext(\"2d\");\n";
	out << "\tctx.font = " << textFont << "; \n\n";

	out << "\tfunction t(s, x, y) {ctx.fillText(s, x, y);}\n";
	out << "\tfunction r(x, y) {ctx.fillRect(x, y, " << splitDigit(blockWidth) << ", " << blockHeight << ");}\n";
	out << "\tfunction s(c) {ctx.fillStyle = c;}\n\n";

	for (int blockTypeI = 0; blockTypeI < totalBlockTypes; blockTypeI++)
	{
		out << "\ts(\"" << blocksColorToDraw[blockTypeI] << "\");\n";

		for (auto& blockTuple : blocksCoordinateToDraw[blockTypeI])
		{
			out << "\tr(" << splitDigit(get<0>(blockTuple)) << "," << get<1>(blockTuple) << ")\n";
		}

		out << "\n";
	}

	out << "\ts(\"" << textColor << "\");\n";
	for (auto& textTuple : textsToDraw)
	{
		out << "\tt(\'" << get<0>(textTuple) << "\'," << splitDigit(get<1>(textTuple))
			<< "," << get<2>(textTuple) << ")\n";
	}

	out << "}";

	out.close();
}

void createHTMLPages16(string folderPath, uint textLen, uchar* text, uchar* marks)
{
	// Full text consist of pages, page consist of lines, line consist of characters text[line.start : line.end]
	vector <vector <pair<int, int> > > pages;

	separateTextIntoLines(textLen, text, pages);

	// process each page separately 
	for (int pageI = 1; pageI <= pages.size(); pageI++)
	{
		vector<pair<int, int> >& page = pages[pageI - 1];

		int pageWidth = 0, pageHeight = 0;
		findWidthHeight16(page, text, pageWidth, pageHeight);

		writeHTMLPage(folderPath + "page" + to_string(pageI) + ".html", pageWidth, pageHeight, pageI, pages.size());

		writeDrawScript16(folderPath + "script" + to_string(pageI) + ".js", page, text, marks);
	}
}

void writeHTMLPageEntropy(string path, int pageWidth, int pageHeight, int pageNumber, int totalPages)
{
	ofstream out(path);

	if (!out.is_open())
	{
		exit(5);
	}

	set <int> linkedPages;

	if (pageNumber != 1) { linkedPages.insert(1); }
	if (pageNumber >= 101) { linkedPages.insert(pageNumber - 100); }
	if (pageNumber >= 11) { linkedPages.insert(pageNumber - 10); }
	if (pageNumber >= 4) { linkedPages.insert(pageNumber - 3); }
	if (pageNumber >= 3) { linkedPages.insert(pageNumber - 2); }
	if (pageNumber >= 2) { linkedPages.insert(pageNumber - 1); }
	if (pageNumber + 1 <= totalPages) { linkedPages.insert(pageNumber + 1); }
	if (pageNumber + 2 <= totalPages) { linkedPages.insert(pageNumber + 2); }
	if (pageNumber + 3 <= totalPages) { linkedPages.insert(pageNumber + 3); }
	if (pageNumber + 10 <= totalPages) { linkedPages.insert(pageNumber + 10); }
	if (pageNumber + 100 <= totalPages) { linkedPages.insert(pageNumber + 100); }
	if (pageNumber != totalPages) { linkedPages.insert(totalPages); }

	out << "<!DOCTYPE html>\n";
	out << "<html>\n";
	out << "<body>\n\n";

	out << "<h1>Page " << pageNumber << "</h1>\n";

	for (auto index : linkedPages)
	{
		out << "<p><a href = \"page" << index << ".html\">Page " << index << "</a></p>\n";
	}
	out << "\n";

	out << "<canvas id = \"canvas2\" width = \"1400px\" height = \"50px\"></canvas>\n\n";
	out << "<script src = \"script_2.js\"></script>\n";
	out << "<script>\n";
	out << "\tdraw2();\n";
	out << "</script>\n\n";

	out << "<canvas id = \"canvas\" width = \"" << pageWidth << "px\" height = \""
		<< pageHeight << "px\"></canvas>\n\n";

	out << "<script src = \"script" << pageNumber << ".js\"></script>\n";
	out << "<script>\n";
	out << "\tdraw();\n";
	out << "</script>\n\n";

	out << "</body>\n";
	out << "</html>\n";

	out.close();
}

void writeDraw2ScriptEntropy(string path)
{
	using namespace markup;

	ofstream out(path);

	out << "function draw2()\n";
	out << "{\n";
	out << "\tlet canvas = document.getElementById(\"canvas2\");\n";
	out << "\tlet ctx = canvas.getContext(\"2d\");\n";
	out << "\tctx.font = \"34px Arial Bold\";\n\n";

	out << "\tfunction ft(s, x, y) {ctx.fillText(s, x, y);}\n";
	out << "\tfunction fr(x, y, w, h) {ctx.fillRect(x, y, w, h);}\n\n";
	out << "\tfunction fs(c) {ctx.fillStyle = c;}\n\n";

	int i = 0;
	for (double e = 0.00001; e < 16.0; e += 0.0125)
	{
		int h = scaleEntropyToHue(e);

		out << "\tfs(\"hsl(" << h << "," << entropySaturation << "%, " << entropyLightness << "%)\");\n";
		out << "\tfr(" << i << ",0,2,50);\n";
		i++;
	}
	i = 0;
	out << "\tfs(\"black\");\n";
	for (double e = 0.0; e < 16.0; e += 0.0125)
	{
		if (i % 80 == 0)
		{
			int bits = i / 80;
			out << "\tft(\"" << bits << "\"," << i << ",30);\n";
		}
		i++;
	}

	out << "}";
}

void writeDrawScriptEntropy(string path, vector <pair<int, int> >& page, uchar* text, uchar* marks,
	uchar* colorTypes)
{
	using namespace markup;

	const int totalBlockTypes = 251;

	ofstream out(path);

	if (!out.is_open())
	{
		exit(6);
	}

	tuple<string, int, int> tmpTextTuple;
	vector <tuple<string, int, int> > textsToDraw; // {text, x, y}

	tuple<int, int, int, int> tmpBlockTuple;
	vector <tuple<int, int, int, int> > blocksCoordinateToDraw[totalBlockTypes]; // {x, y, width, height}

	for (int lineI = 0; lineI < page.size(); lineI++)
	{
		pair<int, int> line = page[lineI];

		get<1>(tmpBlockTuple) = lineI * (charMaxFullHeight + verticalBlockShiftUp + verticalBlockShiftDown);
		get<3>(tmpBlockTuple) = charMaxFullHeight + verticalBlockShiftUp + verticalBlockShiftDown;
		get<2>(tmpTextTuple) = get<1>(tmpBlockTuple) + verticalBlockShiftUp + charMaxHeightAscent;

		int x = 0, blockTextWidth = 0;

		string blockText = outputCodesString[text[line.start]];
		blockTextWidth += outputCodesWidth[text[line.start]];

		for (int i = line.start + 1; i <= line.end; i++)
		{
			if (marks[i - 1] != marks[i])
			{
				get<0>(tmpTextTuple) = blockText;
				get<1>(tmpTextTuple) = x + horisontalBlockShiftLeft;
				textsToDraw.push_back(tmpTextTuple);

				get<0>(tmpBlockTuple) = x;
				get<2>(tmpBlockTuple) = blockTextWidth + horisontalBlockShiftLeft + horisontalBlockShiftRight + 10;
				blocksCoordinateToDraw[colorTypes[i - 1]].push_back(tmpBlockTuple);

				x += blockTextWidth + horisontalBlockShiftLeft + horisontalBlockShiftRight;
				blockText = outputCodesString[text[i]];
				blockTextWidth = outputCodesWidth[text[i]];
			}
			else
			{
				blockText += outputCodesString[text[i]];
				blockTextWidth += outputCodesWidth[text[i]];
			}
		}
		get<0>(tmpTextTuple) = blockText;
		get<1>(tmpTextTuple) = x + horisontalBlockShiftLeft;
		textsToDraw.push_back(tmpTextTuple);

		get<0>(tmpBlockTuple) = x;
		get<2>(tmpBlockTuple) = blockTextWidth + horisontalBlockShiftLeft + horisontalBlockShiftRight;
		blocksCoordinateToDraw[colorTypes[line.end]].push_back(tmpBlockTuple);
	}

	out << "function draw()\n";
	out << "{\n";
	out << "\tlet canvas = document.getElementById(\"canvas\");\n";
	out << "\tlet ctx = canvas.getContext(\"2d\");\n";
	out << "\tctx.font = " << textFont << "; \n\n";

	out << "\tfunction t(s, x, y) {ctx.fillText(s, x, y);}\n";
	out << "\tfunction r(x, y, w, h) {ctx.fillRect(x, y, w, h);}\n";
	out << "\tfunction s(c) {ctx.fillStyle = c;}\n\n";

	for (int colI = 0; colI < totalBlockTypes; colI++)
	{
		string myColor = "hsl(" + to_string(colI) + ", " + to_string(entropySaturation) + "%, " +
			to_string(entropyLightness) + "%)";
		out << "\ts(\"" << myColor << "\");\n";

		for (auto& blockTuple : blocksCoordinateToDraw[colI])
		{
			out << "\tr(" << splitDigit(get<0>(blockTuple)) << "," << get<1>(blockTuple)
				<< "," << splitDigit(get<2>(blockTuple)) << "," << get<3>(blockTuple) << ")\n";
		}

		out << "\n";
	}

	out << "\tctx.fillStyle = " << "\"" << textColor << "\"" << ";\n\n";
	for (auto& textTuple : textsToDraw)
	{
		out << "\tt(\'" << get<0>(textTuple) << "\'," << splitDigit(get<1>(textTuple))
			<< "," << get<2>(textTuple) << ")\n";
	}

	out << "}";

	out.close();
}

void createHTMLPagesEntropy(string folderPath, uint textLen, uchar* text, uchar* marks, uchar* colorTypes)
{
	// Full text consist of pages, page consist of lines, line consist of characters text[line.start : line.end]
	vector <vector <pair<int, int> > > pages;

	separateTextIntoLines(textLen, text, pages);

	writeDraw2ScriptEntropy(folderPath + "script_2.js");

	// process each page separately 
	for (int pageI = 1; pageI <= pages.size(); pageI++)
	{
		vector<pair<int, int> >& page = pages[pageI - 1];

		int pageWidth = 0, pageHeight = 0;
		findWidthHeight(page, text, marks, pageWidth, pageHeight);

		writeHTMLPageEntropy(folderPath + "page" + to_string(pageI) + ".html", pageWidth, pageHeight,
			pageI, pages.size());

		writeDrawScriptEntropy(folderPath + "script" + to_string(pageI) + ".js", page, text, marks, colorTypes);
	}
}

void writeDrawScriptEntropy16(string path, vector <pair<int, int> >& page, uchar* text, uchar* colorTypes)
{
	using namespace markup;

	const int totalBlockTypes = 251;

	ofstream out(path);

	if (!out.is_open())
	{
		exit(6);
	}

	tuple<string, int, int> tmpTextTuple;
	vector <tuple<string, int, int> > textsToDraw; // {text, x, y}

	tuple<int, int> tmpBlockTuple;
	vector <tuple<int, int> > blocksCoordinateToDraw[totalBlockTypes]; // {x, y, width, height}

	int blockWidth = charWidth16_2 + horisontalBlockShiftLeft + horisontalBlockShiftRight;
	int blockHeight = charHeight16 + verticalBlockShiftUp + verticalBlockShiftDown;

	for (int lineI = 0; lineI < page.size(); lineI++)
	{
		pair<int, int> line = page[lineI];

		get<1>(tmpBlockTuple) = lineI * blockHeight;
		get<2>(tmpTextTuple) = get<1>(tmpBlockTuple) + verticalBlockShiftUp + charMaxHeightAscent;

		for (int i = line.start; i <= line.end; i++)
		{
			get<0>(tmpTextTuple) = outputCodes16[text[i]];
			get<1>(tmpTextTuple) = (i - line.start) * blockWidth + horisontalBlockShiftLeft;
			textsToDraw.push_back(tmpTextTuple);

			get<0>(tmpBlockTuple) = (i - line.start) * blockWidth;
			blocksCoordinateToDraw[colorTypes[i]].push_back(tmpBlockTuple);
		}
	}

	out << "function draw()\n";
	out << "{\n";
	out << "\tlet canvas = document.getElementById(\"canvas\");\n";
	out << "\tlet ctx = canvas.getContext(\"2d\");\n";
	out << "\tctx.font = " << textFont << "; \n\n";

	out << "\tfunction t(s, x, y) {ctx.fillText(s, x, y);}\n";
	out << "\tfunction r(x, y) {ctx.fillRect(x, y, " << splitDigit(blockWidth + 10) << ", " << blockHeight << ");}\n";
	out << "\tfunction s(c) {ctx.fillStyle = c;}\n\n";

	for (int colI = 0; colI < totalBlockTypes; colI++)
	{
		string myColor = "hsl(" + to_string(colI) + ", " + to_string(entropySaturation) + "%, " +
			to_string(entropyLightness) + "%)";
		out << "\ts(\"" << myColor << "\");\n";

		for (auto& blockTuple : blocksCoordinateToDraw[colI])
		{
			out << "\tr(" << splitDigit(get<0>(blockTuple)) << "," << get<1>(blockTuple) << ")\n";
		}

		out << "\n";
	}

	out << "\tctx.fillStyle = " << "\"" << textColor << "\"" << ";\n\n";
	for (auto& textTuple : textsToDraw)
	{
		out << "\tt(\'" << get<0>(textTuple) << "\'," << splitDigit(get<1>(textTuple))
			<< "," << get<2>(textTuple) << ")\n";
	}

	out << "}";

	out.close();
}

void createHTMLPagesEntropy16(string folderPath, uint textLen, uchar* text, uchar* colorTypes)
{
	// Full text consist of pages, page consist of lines, line consist of characters text[line.start : line.end]
	vector <vector <pair<int, int> > > pages;

	separateTextIntoLines(textLen, text, pages);

	writeDraw2ScriptEntropy(folderPath + "script_2.js");

	// process each page separately 
	for (int pageI = 1; pageI <= pages.size(); pageI++)
	{
		vector<pair<int, int> >& page = pages[pageI - 1];

		int pageWidth = 0, pageHeight = 0;
		findWidthHeight16(page, text, pageWidth, pageHeight);

		writeHTMLPageEntropy(folderPath + "page" + to_string(pageI) + ".html", pageWidth, pageHeight,
			pageI, pages.size());

		writeDrawScriptEntropy16(folderPath + "script" + to_string(pageI) + ".js", page, text, colorTypes);
	}
}
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

using uchar = unsigned char;

string font;

vector <uchar> vc; // visibleCharacters

// create list of characters that can be rendered as they are
void fillVisibleCharacters()
{
	for (uchar c = 'a'; c <= 'z'; c++)
	{
		vc.push_back(c);
	}
	for (uchar c = 'A'; c <= 'Z'; c++)
	{
		vc.push_back(c);
	}
	for (uchar c = '0'; c <= '9'; c++)
	{
		vc.push_back(c);
	}
	uchar arr[33] = "~!@#$%^&*()_+'\";:?-=\\/|<>.,{}[]";

	for (auto c : arr)
	{
		if (c != '\0')
		{
			vc.push_back(c);
		}
	}
}

// create *.html file that executes *.js script
void createHTMLForLens()
{
	ofstream out("page.html");

	out << "<!DOCTYPE html>\n";
	out << "<html>\n";
	out << "<body>\n\n";

	out << "<canvas id = \"canvas\" width = \"10px\" height = \"10px\"></canvas>\n\n";

	out << "<script src = \"script.js\"></script>\n";
	out << "<script>\n";
	out << "\tdraw();\n";
	out << "</script>\n\n";

	out << "</body>\n";
	out << "</html>\n";

	out.close();
}

// create *.js file that determines size of each character rendered on html image canvas
void createJSForLens()
{
	ofstream out("script.js");

	out << "function draw()\n";
	out << "{\n";
	out << "\tlet canvas = document.getElementById(\"canvas\");\n";
	out << "\tlet ctx = canvas.getContext(\"2d\");\n";
	out << "\tctx.font = " << font << ";\n\n";

	out << "\tlet text = \"temp_text\"\n";
	out << "\tlet metrics = ctx.measureText(text)\n";
	out << "\tlet w = ctx.measureText(text).width\n";
	out << "\tlet h = metrics.actualBoundingBoxAscent + metrics.actualBoundingBoxDescent\n";

	for (auto c : vc)
	{
		out << "\n";

		string s = string(1, c);
		if (c == '\"')
		{
			s = "\\\"";
		}
		else if (c == '\\')
		{
			s = "\\\\";
		}

		out << "\ttext = \"" << s << "\"\n";

		out << "\tmetrics = ctx.measureText(text)\n";
		out << "\tw = ctx.measureText(text).width\n";
		out << "\th1 = metrics.actualBoundingBoxAscent\n";
		out << "\th2 = metrics.actualBoundingBoxDescent\n";

		out << "\tconsole.log(\"" << s << "\" + \" \" + w + \" \" + h1 + \" \" + h2);\n";
	}

	out << "}";
}

int main()
{
	cout << "Input font name e. g. \"24px_Arial_Bold\" (with quotes) or . to use default font\n";
	cin >> font;
	if (font == ".")
	{
		font = "\"24px_Arial_Bold\"";
	}
	for (auto& character : font)
	{
		if (character == '_')
		{
			character = ' ';
		}
	}

	fillVisibleCharacters();
	cout << "total " << vc.size() << " visible characters\n";

	createHTMLForLens();
	createJSForLens();
	cout << "done\n\n";

	cout << "Open page.html -> right click -> inspect -> console -> right click -> Save as.. CharSizeConsole.log\n";

	return 0;
}
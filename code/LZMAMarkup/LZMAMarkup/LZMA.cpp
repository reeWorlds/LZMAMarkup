#include "LZMA.h"

int scaleEntropyToHue(double e)
{
	e = max(min(16.0, e), 0.0);

	if (e < 4.0)
	{
		return 250 - 120 * e / 4.0;
	}
	else if (e < 8.0)
	{
		return 130 - 70 * (e - 4.0) / 4.0;
	}
	else if (e < 12.0)
	{
		return 60 - 40 * (e - 8.0) / 4.0;
	}
	else
	{
		return 20 - 19.99 * (e - 12.0) / 4.0;
	}
}

int scaleEntropyToHue2(float e)
{
	int mult = e < 1e-10 ? -1 : 1;
	int val;

	e = abs(e);

	if (e >= 10.0)
	{
		val = 125;
	}
	else if (e >= 5.0)
	{
		val = 95 + 5.9999 * (e - 5.0);
	}
	else if (e > 3.0)
	{
		val = 75 + 10 * (e - 3.0);
	}
	else if (e > 2.0)
	{
		val = 60 + 16 * (e - 2.0);
	}
	else if (e > 1.0)
	{
		val = 35 + 25 * (e - 1.0);
	}
	else
	{
		val = 35 * e;
	}

	return 125 + mult * val;
}

LZMA::RC::RC(FILE* _in)
{
	in = _in;
}

uchar LZMA::RC::get()
{
	return getc(in);
}

void LZMA::RC::rc_Init()
{
	code = (get() << 24) | (get() << 16) | (get() << 8) | (get());
	range = 0xFFFFFFFF;
}

void LZMA::RC::rc_Renorm()
{
	if (range < sTOP)
	{
		range <<= 8;
		code = (code << 8) | get();
	}
}

uint LZMA::RC::rc_Bits(uint l)
{
	uint x = 0;
	do
	{
		rc_Renorm();
		range &= ~1;

		uint rnew = (range >> 1) * 1;
		uint bit = code >= rnew;

		range = bit ? code -= rnew, range - rnew : rnew;

		x += x + bit;
	} while (--l != 0);

	return x;
}

uint LZMA::RC::rc_Decode(uint P)
{
	rc_Renorm();

	uint rnew = (range >> SCALElog) * P;

	uint bit = code >= rnew;

	range = bit ? code -= rnew, range - rnew : rnew;

	return bit;
}

uint LZMA::RC::rc_Encode(uint P, uint bit)
{
	rc_Renorm();

	uint rnew = (range >> SCALElog) * P;

	range = bit ? lowc += rnew, range - rnew : rnew;

	return bit;
}

LZMA::LZMA(FILE* f) : rc(f), f(f)
{
	
}

void LZMA::initCounters()
{
	for (uint i = 0; i < kNumStates; i++)
	{
		for (uint j = 0; j < (1 << kNumPosBitsMax); j++)
		{
			c_IsMatch[i][j] = RC::hSCALE;
		}
	}

	for (uint i = 0; i < kNumStates; i++)
	{
		c_IsRep[i] = RC::hSCALE;
		c_IsRepG0[i] = RC::hSCALE;
		c_IsRepG1[i] = RC::hSCALE;
		c_IsRepG2[i] = RC::hSCALE;

		for (uint j = 0; j < (1 << kNumPosBitsMax); j++)
		{
			c_IsRep0Long[i][j] = RC::hSCALE;
		}
	}

	c_LenChoice[0] = c_LenChoice[1] = RC::hSCALE;
	c_LenChoice2[0] = c_LenChoice2[1] = RC::hSCALE;

	for (uint i1 = 0; i1 < (1 << kNumLPosBitsMax); i1++)
	{
		for (uint i2 = 0; i2 < 256; i2++)
		{
			for (uint i3 = 0; i3 < 3; i3++)
			{
				for (uint i4 = 0; i4 < 256; i4++)
				{
					c_Literal[i1][i2][i3][i4] = RC::hSCALE;
				}
			}
		}
	}

	for (uint i = 0; i < 2; i++)
	{
		for (uint j = 0; j < kNumPosStatesMax; j++)
		{
			for (uint l = 0; l < (1 << kLenNumLowBits); l++)
			{
				c_LenLow[i][j][l] = RC::hSCALE;
				c_LenMid[i][j][l] = RC::hSCALE;
			}
		}
	}

	for (uint i = 0; i < (1 << kLenNumHighBits); i++)
	{
		c_LenHigh[0][i] = c_LenHigh[1][i] = RC::hSCALE;
	}

	for (uint i = 0; i < kNumLenToPosStates; i++)
	{
		for (uint j = 0; j < (1 << kNumPosSlotBits); j++)
		{
			c_PosSlot[i][j] = RC::hSCALE;
		}
	}

	for (uint i = 0; i < kNumFullDistances - kEndPosModelIndex; i++)
	{
		c_SpecPos[i] = RC::hSCALE;
	}

	for (uint i = 0; i < (1 << kNumAlignBits); i++)
	{
		c_Align[i] = RC::hSCALE;
	}
}

void LZMA::updateCounter(short& c, uint bit)
{
	c += bit ? -(c >> 5) : ((RC::SCALE - c) >> 5);
}

int LZMA::decode_bit(short& c)
{
	int bit = rc.rc_Decode(c);
	
	updateCounter(c, bit);

	return bit;
}

#define rep0Pos() (dictPos - rep0) + ((dictPos < rep0) ? dictSize : 0)
#define symStore(symbol) {	text[filePos] = dict[dictPos] = symbol; \
							marks[filePos] = markId; \
							if(++dictPos == dictSize) {dictPos=0;} \
							filePos++; }

void LZMA::decodeText(uint& textLen, uchar*& text, uchar*& marks)
{
	initCounters();

	ull fLen;

	// read lc/lp/pb
	if (true)
	{
		uchar token;
		fread(&token, sizeof(uchar), 1, f);

		lc = token % 9;
		lp = token / 9 % 5;
		pb = token / 9 / 5;
	}

	// read dict size
	fread(&dictSize, sizeof(dictSize), 1, f);
	// read decompressed file length
	fread(&fLen, sizeof(fLen), 1, f);
	rc.get(); // zero byte

	textLen = fLen;
	text = new uchar[textLen];
	marks = new uchar[textLen];

	dict = new uchar[dictSize];

	rc.rc_Init();

	uint i;
	uint state = 0, rep0 = 1, rep1 = 1, rep2 = 1, rep3 = 1;
	uint dictPos = 0;
	uint pbMask = (1 << pb) - 1, lpMask = (1 << lp) - 1, lc8 = 8 - lc;
	uint id, val, symbol = 0, iLen, dist, pos, len, cps;
	short* clen;

	uchar markId;
	uchar literalFlag = 0, matchFlag = 0, shortRepeatFlag = 0, longRepeat0Flag = 0,
		longRepeat1Flag = 0, longRepeat2Flag = 0, longRepeat3Flag = 0;

	// create reverse table for 5-bit numbers
	for (i = 0; i < 32; i++)
	{
		rbit5[i] = ((i * 0x0802 & 0x22110) | (i * 0x8020 & 0x88440)) * 0x10101 >> 16 + 3;
	}

	for (ull filePos = 0; filePos < fLen; state = statemap[state][id])
	{
		uint posState = filePos & pbMask;
		uint pSymbol = uchar(symbol);

		if (decode_bit(c_IsMatch[state][posState]) == 0)
		{
			id = id_lit;
			markId = mark_literal1 + literalFlag;
			literalFlag ^= 1;
		}
		else if (decode_bit(c_IsRep[state]) == 0)
		{
			id = id_match;
			markId = mark_match1 + matchFlag;
			matchFlag ^= 1;
		}
		else if (decode_bit(c_IsRepG0[state]) == 0)
		{
			if (decode_bit(c_IsRep0Long[state][posState]) == 0)
			{
				id = id_litr0;
				markId = mark_shortRepeat1 + shortRepeatFlag;
				shortRepeatFlag ^= 1;
			}
			else
			{
				id = id_r0;
				markId = mark_longRepeat01 + longRepeat0Flag;
				longRepeat0Flag ^= 1;
			}
		}
		else
		{
			if (decode_bit(c_IsRepG1[state]) == 0)
			{
				id = id_r1;
				markId = mark_longRepeat11 + longRepeat1Flag;
				longRepeat1Flag ^= 1;
			}
			else
			{
				if (decode_bit(c_IsRepG2[state]))
				{
					id = id_r3;
					markId = mark_longRepeat21 + longRepeat2Flag;
					longRepeat2Flag ^= 1;
				}
				else
				{
					id = id_r2;
					markId = mark_longRepeat31 + longRepeat3Flag;
					longRepeat3Flag ^= 1;
				}
			}
		}

		if (id == id_lit || id == id_litr0)
		{
			if (id == id_litr0)
			{
				symbol = dict[rep0Pos()];
			}
			else
			{
				short (&c)[3][256] = c_Literal[filePos & lpMask][pSymbol >> lc8];

				if (state >= kNumLitStates)
				{
					uint matchbyte = 0x100 + dict[rep0Pos()];

					for (symbol = 1; symbol < 0x100; )
					{
						uint mbprefix = (matchbyte <<= 1) >> 8;
						symbol += symbol + decode_bit(c[1 + (mbprefix & 1)][symbol]);

						if (mbprefix != symbol)
						{
							break;
						}
					}
				}
				else
				{
					symbol = 1;
				}

				for (; symbol < 0x100;)
				{
					symbol += symbol + decode_bit(c[0][symbol]);
				}
			}

			symStore(symbol);
		}
		else
		{
			uint fRep = (id != id_match);

			if (fRep)
			{
				if (id != id_r0)
				{
					dist = rep1;

					if (id != id_r1)
					{
						dist = rep2;
						if (id == id_r3)
						{
							dist = rep3;
							rep3 = rep2;
						}
						rep2 = rep1;
					}

					rep1 = rep0;
					rep0 = dist;
				}
			}

			if (decode_bit(c_LenChoice[fRep]) == 0)
			{
				iLen = 0;
			}
			else
			{
				if (decode_bit(c_LenChoice2[fRep]) == 0)
				{
					iLen = 1;
				}
				else
				{
					iLen = 2;
				}
			}

			uint limit, offset;
			if (iLen == 0)
			{
				clen = &c_LenLow[fRep][posState][0];
				offset = 0; limit = (1 << kLenNumLowBits);
			}
			else
			{
				if (iLen == 1)
				{
					clen = &c_LenMid[fRep][posState][0];
					offset = kLenNumLowSymbols; limit = (1 << kLenNumMidBits);
				}
				else
				{
					clen = &c_LenHigh[fRep][0];
					offset = kLenNumLowSymbols + kLenNumMidSymbols; limit = (1 << kLenNumHighBits);
				}
			}

			for (len = 1; len < limit;)
			{
				len += len + decode_bit(clen[len]);
			}
			len -= limit;
			len += offset;

			if (fRep == 0)
			{
				short (&cpos)[1 << kNumPosSlotBits] = c_PosSlot[len < kNumLenToPosStates ? len : kNumLenToPosStates - 1];

				for (dist = 1; dist < 64;)
				{
					dist += dist + decode_bit(cpos[dist]);
				}
				dist -= 64;

				if (dist >= kStartPosModelIndex)
				{
					uint posSlot = dist;
					int numDirectBits = (dist >> 1) - 1;
					dist = (2 | (dist & 1));

					if (posSlot < kEndPosModelIndex)
					{
						dist <<= numDirectBits;
						uint limit = 1 << numDirectBits;
						for (i = 1; i < limit;)
						{
							i += i + decode_bit(c_SpecPos[dist - posSlot - 1 + i]);
						}
						dist += rbit5[(i - limit) << (5 - numDirectBits)];
					}
					else
					{
						numDirectBits -= kNumAlignBits;
						(dist <<= numDirectBits) += rc.rc_Bits(numDirectBits);
						for (i = 1; i < 16; )
						{
							i += i + decode_bit(c_Align[i]);
						}
						(dist <<= kNumAlignBits) += rbit5[(i - 16) << 1];
						
						if (dist == 0xFFFFFFFFU)
						{
							break;
						}
					}
				}

				rep3 = rep2;
				rep2 = rep1;
				rep1 = rep0;
				rep0 = dist + 1;
			}

			for (pos = rep0Pos(), len += kMatchMinLen; len > 0; len--)
			{
				symbol = dict[pos];
				symStore(symbol);

				if (++pos == dictSize)
				{
					pos = 0;
				}
			}
		}
	}

	delete[] dict;
}

int LZMA::entropyDecode_bit(short& c, double& totalBits)
{
	int bit = rc.rc_Decode(c);
	if (bit == 0)
	{
		totalBits -= log2(double(c) / RC::SCALE);
	}
	else
	{
		totalBits -= log2(1.0 - double(c) / RC::SCALE);
	}

	updateCounter(c, bit);

	return bit;
}

#define symStore2(symbol) {	text[filePos] = dict[dictPos] = symbol; \
							colorTypes[filePos] = bitMark; \
							marks[filePos] = markId; \
							if(++dictPos == dictSize) {dictPos=0;} \
							filePos++; }

void LZMA::entropydecodeText(uint& textLen, uchar*& text, uchar*& marks, uchar*& colorTypes)
{
	initCounters();

	ull fLen;

	// read lc/lp/pb
	if (true)
	{
		uchar token;
		fread(&token, sizeof(uchar), 1, f);

		lc = token % 9;
		lp = token / 9 % 5;
		pb = token / 9 / 5;
	}

	// read dict size
	fread(&dictSize, sizeof(dictSize), 1, f);
	// read decompressed file length
	fread(&fLen, sizeof(fLen), 1, f);
	rc.get(); // zero byte

	textLen = fLen;
	text = new uchar[textLen];
	marks = new uchar[textLen];
	colorTypes = new uchar[textLen];

	dict = new uchar[dictSize];

	rc.rc_Init();

	uint i;
	uint state = 0, rep0 = 1, rep1 = 1, rep2 = 1, rep3 = 1;
	uint dictPos = 0;
	uint pbMask = (1 << pb) - 1, lpMask = (1 << lp) - 1, lc8 = 8 - lc;
	uint id, val, symbol = 0, iLen, dist, pos, len, cps;
	short* clen;

	// create reverse table for 5-bit numbers
	for (i = 0; i < 32; i++)
	{
		rbit5[i] = ((i * 0x0802 & 0x22110) | (i * 0x8020 & 0x88440)) * 0x10101 >> 16 + 3;
	}

	uchar markId;
	uchar literalFlag = 0, matchFlag = 0, shortRepeatFlag = 0, longRepeat0Flag = 0,
		longRepeat1Flag = 0, longRepeat2Flag = 0, longRepeat3Flag = 0;

	double usedBits;
	uchar bitMark;

	for (ull filePos = 0; filePos < fLen; state = statemap[state][id])
	{
		usedBits = 0.0;
		uint posState = filePos & pbMask;
		uint pSymbol = uchar(symbol);

		if (entropyDecode_bit(c_IsMatch[state][posState], usedBits) == 0)
		{
			id = id_lit;
			markId = mark_literal1 + literalFlag;
			literalFlag ^= 1;
		}
		else if (entropyDecode_bit(c_IsRep[state], usedBits) == 0)
		{
			id = id_match;
			markId = mark_match1 + matchFlag;
			matchFlag ^= 1;
		}
		else if (entropyDecode_bit(c_IsRepG0[state], usedBits) == 0)
		{
			if (entropyDecode_bit(c_IsRep0Long[state][posState], usedBits) == 0)
			{
				id = id_litr0;
				markId = mark_shortRepeat1 + shortRepeatFlag;
				shortRepeatFlag ^= 1;
			}
			else
			{
				id = id_r0;
				markId = mark_longRepeat01 + longRepeat0Flag;
				longRepeat0Flag ^= 1;
			}
		}
		else
		{
			if (entropyDecode_bit(c_IsRepG1[state], usedBits) == 0)
			{
				id = id_r1;
				markId = mark_longRepeat11 + longRepeat1Flag;
				longRepeat1Flag ^= 1;
			}
			else
			{
				if (entropyDecode_bit(c_IsRepG2[state], usedBits))
				{
					id = id_r3;
					markId = mark_longRepeat21 + longRepeat2Flag;
					longRepeat2Flag ^= 1;
				}
				else
				{
					id = id_r2;
					markId = mark_longRepeat31 + longRepeat3Flag;
					longRepeat3Flag ^= 1;
				}
			}
		}

		if (id == id_lit || id == id_litr0)
		{
			if (id == id_litr0)
			{
				symbol = dict[rep0Pos()];
			}
			else
			{
				short(&c)[3][256] = c_Literal[filePos & lpMask][pSymbol >> lc8];

				if (state >= kNumLitStates)
				{
					uint matchbyte = 0x100 + dict[rep0Pos()];

					for (symbol = 1; symbol < 0x100; )
					{
						uint mbprefix = (matchbyte <<= 1) >> 8;
						symbol += symbol + entropyDecode_bit(c[1 + (mbprefix & 1)][symbol], usedBits);

						if (mbprefix != symbol)
						{
							break;
						}
					}
				} 
				else
				{
					symbol = 1;
				}

				for (; symbol < 0x100;)
				{
					symbol += symbol + entropyDecode_bit(c[0][symbol], usedBits);
				}
			}

			bitMark = scaleEntropyToHue(usedBits);
			symStore2(symbol);
		}
		else
		{
			uint fRep = (id != id_match);

			if (fRep)
			{
				if (id != id_r0)
				{
					dist = rep1;

					if (id != id_r1)
					{
						dist = rep2;
						if (id == id_r3)
						{
							dist = rep3;
							rep3 = rep2;
						}
						rep2 = rep1;
					}

					rep1 = rep0;
					rep0 = dist;
				}
			}

			if (entropyDecode_bit(c_LenChoice[fRep], usedBits) == 0)
			{
				iLen = 0;
			}
			else
			{
				if (entropyDecode_bit(c_LenChoice2[fRep], usedBits) == 0)
				{
					iLen = 1;
				}
				else
				{
					iLen = 2;
				}
			}

			uint limit, offset;
			if (iLen == 0)
			{
				clen = &c_LenLow[fRep][posState][0];
				offset = 0; limit = (1 << kLenNumLowBits);
			}
			else
			{
				if (iLen == 1)
				{
					clen = &c_LenMid[fRep][posState][0];
					offset = kLenNumLowSymbols; limit = (1 << kLenNumMidBits);
				}
				else
				{
					clen = &c_LenHigh[fRep][0];
					offset = kLenNumLowSymbols + kLenNumMidSymbols; limit = (1 << kLenNumHighBits);
				}
			}

			for (len = 1; len < limit;)
			{
				len += len + entropyDecode_bit(clen[len], usedBits);
			}
			len -= limit;
			len += offset;

			if (fRep == 0)
			{
				short(&cpos)[1 << kNumPosSlotBits] = c_PosSlot[len < kNumLenToPosStates ? len : kNumLenToPosStates - 1];

				for (dist = 1; dist < 64;)
				{
					dist += dist + entropyDecode_bit(cpos[dist], usedBits);
				}
				dist -= 64;

				if (dist >= kStartPosModelIndex)
				{
					uint posSlot = dist;
					int numDirectBits = (dist >> 1) - 1;
					dist = (2 | (dist & 1));

					if (posSlot < kEndPosModelIndex)
					{
						dist <<= numDirectBits;
						uint limit = 1 << numDirectBits;
						for (i = 1; i < limit;)
						{
							i += i + entropyDecode_bit(c_SpecPos[dist - posSlot - 1 + i], usedBits);
						}
						dist += rbit5[(i - limit) << (5 - numDirectBits)];
					}
					else
					{
						numDirectBits -= kNumAlignBits;
						usedBits += numDirectBits;
						(dist <<= numDirectBits) += rc.rc_Bits(numDirectBits);
						for (i = 1; i < 16; )
						{
							i += i + entropyDecode_bit(c_Align[i], usedBits);
						}
						(dist <<= kNumAlignBits) += rbit5[(i - 16) << 1];

						if (dist == 0xFFFFFFFFU)
						{
							break;
						}
					}
				}

				rep3 = rep2;
				rep2 = rep1;
				rep1 = rep0;
				rep0 = dist + 1;
			}

			usedBits /= (len + kMatchMinLen);
			bitMark = scaleEntropyToHue(usedBits);

			for (pos = rep0Pos(), len += kMatchMinLen; len > 0; len--)
			{
				symbol = dict[pos];
				symStore2(symbol);

				if (++pos == dictSize)
				{
					pos = 0;
				}
			}
		}
	}

	delete[] dict;
}

#define symStore3(symbol) {	text[filePos] = dict[dictPos] = symbol; \
							ent[filePos] = usedBits; \
							marks[filePos] = markId; \
							if(++dictPos == dictSize) {dictPos=0;} \
							filePos++; }

void LZMA::compareDecodeText(uint& textLen, uchar*& text, uchar*& marks, float*& ent)
{
	initCounters();

	ull fLen;

	// read lc/lp/pb
	if (true)
	{
		uchar token;
		fread(&token, sizeof(uchar), 1, f);

		lc = token % 9;
		lp = token / 9 % 5;
		pb = token / 9 / 5;
	}

	// read dict size
	fread(&dictSize, sizeof(dictSize), 1, f);
	// read decompressed file length
	fread(&fLen, sizeof(fLen), 1, f);
	rc.get(); // zero byte

	textLen = fLen;
	text = new uchar[textLen];
	marks = new uchar[textLen];
	ent = new float[textLen];

	dict = new uchar[dictSize];

	rc.rc_Init();

	uint i;
	uint state = 0, rep0 = 1, rep1 = 1, rep2 = 1, rep3 = 1;
	uint dictPos = 0;
	uint pbMask = (1 << pb) - 1, lpMask = (1 << lp) - 1, lc8 = 8 - lc;
	uint id, val, symbol = 0, iLen, dist, pos, len, cps;
	short* clen;

	// create reverse table for 5-bit numbers
	for (i = 0; i < 32; i++)
	{
		rbit5[i] = ((i * 0x0802 & 0x22110) | (i * 0x8020 & 0x88440)) * 0x10101 >> 16 + 3;
	}

	uchar markId;
	uchar literalFlag = 0, matchFlag = 0, shortRepeatFlag = 0, longRepeat0Flag = 0,
		longRepeat1Flag = 0, longRepeat2Flag = 0, longRepeat3Flag = 0;

	double usedBits;

	for (ull filePos = 0; filePos < fLen; state = statemap[state][id])
	{
		usedBits = 0.0;
		uint posState = filePos & pbMask;
		uint pSymbol = uchar(symbol);

		if (entropyDecode_bit(c_IsMatch[state][posState], usedBits) == 0)
		{
			id = id_lit;
			markId = mark_literal1 + literalFlag;
			literalFlag ^= 1;
		}
		else if (entropyDecode_bit(c_IsRep[state], usedBits) == 0)
		{
			id = id_match;
			markId = mark_match1 + matchFlag;
			matchFlag ^= 1;
		}
		else if (entropyDecode_bit(c_IsRepG0[state], usedBits) == 0)
		{
			if (entropyDecode_bit(c_IsRep0Long[state][posState], usedBits) == 0)
			{
				id = id_litr0;
				markId = mark_shortRepeat1 + shortRepeatFlag;
				shortRepeatFlag ^= 1;
			}
			else
			{
				id = id_r0;
				markId = mark_longRepeat01 + longRepeat0Flag;
				longRepeat0Flag ^= 1;
			}
		}
		else
		{
			if (entropyDecode_bit(c_IsRepG1[state], usedBits) == 0)
			{
				id = id_r1;
				markId = mark_longRepeat11 + longRepeat1Flag;
				longRepeat1Flag ^= 1;
			}
			else
			{
				if (entropyDecode_bit(c_IsRepG2[state], usedBits))
				{
					id = id_r3;
					markId = mark_longRepeat21 + longRepeat2Flag;
					longRepeat2Flag ^= 1;
				}
				else
				{
					id = id_r2;
					markId = mark_longRepeat31 + longRepeat3Flag;
					longRepeat3Flag ^= 1;
				}
			}
		}

		if (id == id_lit || id == id_litr0)
		{
			if (id == id_litr0)
			{
				symbol = dict[rep0Pos()];
			}
			else
			{
				short(&c)[3][256] = c_Literal[filePos & lpMask][pSymbol >> lc8];

				if (state >= kNumLitStates)
				{
					uint matchbyte = 0x100 + dict[rep0Pos()];

					for (symbol = 1; symbol < 0x100; )
					{
						uint mbprefix = (matchbyte <<= 1) >> 8;
						symbol += symbol + entropyDecode_bit(c[1 + (mbprefix & 1)][symbol], usedBits);

						if (mbprefix != symbol)
						{
							break;
						}
					}
				}
				else
				{
					symbol = 1;
				}

				for (; symbol < 0x100;)
				{
					symbol += symbol + entropyDecode_bit(c[0][symbol], usedBits);
				}
			}

			symStore3(symbol);
		}
		else
		{
			uint fRep = (id != id_match);

			if (fRep)
			{
				if (id != id_r0)
				{
					dist = rep1;

					if (id != id_r1)
					{
						dist = rep2;
						if (id == id_r3)
						{
							dist = rep3;
							rep3 = rep2;
						}
						rep2 = rep1;
					}

					rep1 = rep0;
					rep0 = dist;
				}
			}

			if (entropyDecode_bit(c_LenChoice[fRep], usedBits) == 0)
			{
				iLen = 0;
			}
			else
			{
				if (entropyDecode_bit(c_LenChoice2[fRep], usedBits) == 0)
				{
					iLen = 1;
				}
				else
				{
					iLen = 2;
				}
			}

			uint limit, offset;
			if (iLen == 0)
			{
				clen = &c_LenLow[fRep][posState][0];
				offset = 0; limit = (1 << kLenNumLowBits);
			}
			else
			{
				if (iLen == 1)
				{
					clen = &c_LenMid[fRep][posState][0];
					offset = kLenNumLowSymbols; limit = (1 << kLenNumMidBits);
				}
				else
				{
					clen = &c_LenHigh[fRep][0];
					offset = kLenNumLowSymbols + kLenNumMidSymbols; limit = (1 << kLenNumHighBits);
				}
			}

			for (len = 1; len < limit;)
			{
				len += len + entropyDecode_bit(clen[len], usedBits);
			}
			len -= limit;
			len += offset;

			if (fRep == 0)
			{
				short(&cpos)[1 << kNumPosSlotBits] = c_PosSlot[len < kNumLenToPosStates ? len : kNumLenToPosStates - 1];

				for (dist = 1; dist < 64;)
				{
					dist += dist + entropyDecode_bit(cpos[dist], usedBits);
				}
				dist -= 64;

				if (dist >= kStartPosModelIndex)
				{
					uint posSlot = dist;
					int numDirectBits = (dist >> 1) - 1;
					dist = (2 | (dist & 1));

					if (posSlot < kEndPosModelIndex)
					{
						dist <<= numDirectBits;
						uint limit = 1 << numDirectBits;
						for (i = 1; i < limit;)
						{
							i += i + entropyDecode_bit(c_SpecPos[dist - posSlot - 1 + i], usedBits);
						}
						dist += rbit5[(i - limit) << (5 - numDirectBits)];
					}
					else
					{
						numDirectBits -= kNumAlignBits;
						usedBits += numDirectBits;
						(dist <<= numDirectBits) += rc.rc_Bits(numDirectBits);
						for (i = 1; i < 16; )
						{
							i += i + entropyDecode_bit(c_Align[i], usedBits);
						}
						(dist <<= kNumAlignBits) += rbit5[(i - 16) << 1];

						if (dist == 0xFFFFFFFFU)
						{
							break;
						}
					}
				}

				rep3 = rep2;
				rep2 = rep1;
				rep1 = rep0;
				rep0 = dist + 1;
			}

			usedBits /= (len + kMatchMinLen);

			for (pos = rep0Pos(), len += kMatchMinLen; len > 0; len--)
			{
				symbol = dict[pos];
				symStore3(symbol);

				if (++pos == dictSize)
				{
					pos = 0;
				}
			}
		}
	}

	delete[] dict;
}
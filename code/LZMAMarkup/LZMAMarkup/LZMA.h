#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <tuple>
using namespace std;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ll = long long;
using ull = unsigned long long;

#define BIGNUM (1000000007)

enum mark : uchar
{
	mark_literal1 = 0,
	mark_literal2 = 1,

	mark_match1 = 2,
	mark_match2 = 3,

	mark_shortRepeat1 = 4,
	mark_shortRepeat2 = 5,

	mark_longRepeat01 = 6,
	mark_longRepeat02 = 7,
	mark_longRepeat11 = 8,
	mark_longRepeat12 = 9,
	mark_longRepeat21 = 10,
	mark_longRepeat22 = 11,
	mark_longRepeat31 = 12,
	mark_longRepeat32 = 13,
};

int scaleEntropyToHue(double e);

class LZMA
{
	class RC
	{
	public:

		static const int SCALElog = 11;
		static const int SCALE = 1 << SCALElog;
		static const int hSCALE = SCALE / 2;
		static const int mSCALE = SCALE - 1;
		static const int eSCALE = 16 * mSCALE;

		enum
		{
			NUM = 4,
			sTOP = 0x01000000U,
			Thres = 0xFF000000U,
		};

		uint range, code, FFNum, Cache;
		ull lowc;

		FILE* in;

		RC(FILE* _in);
		uchar get();
		void rc_Init();
		void rc_Renorm();
		uint rc_Bits(uint l);
		uint rc_Decode(uint P);
		uint rc_Encode(uint P, uint bit);
	};

	void initCounters();
	void updateCounter(short& c, uint bit);

	FILE *f;

	RC rc;

	uchar* dict;
	uint lc, lp, pb, dictSize;
	uchar rbit5[32];

	const uchar statemap[12][7] = {
	{  7,  0,  8,  9,  8,  8,  8, },
	{  7,  0,  8,  9,  8,  8,  8, },
	{  7,  0,  8,  9,  8,  8,  8, },
	{  7,  0,  8,  9,  8,  8,  8, },
	{  7,  1,  8,  9,  8,  8,  8, },
	{  7,  2,  8,  9,  8,  8,  8, },
	{  7,  3,  8,  9,  8,  8,  8, },
	{ 10,  4, 11, 11, 11, 11, 11, },
	{ 10,  5, 11, 11, 11, 11, 11, },
	{ 10,  6, 11, 11, 11, 11, 11, },
	{ 10,  4, 11, 11, 11, 11, 11, },
	{ 10,  5, 11, 11, 11, 11, 11, },
	};

	enum
	{
		kNumLPosBitsMax = 4,
		kNumPosBitsMax = 4, kNumPosStatesMax = (1 << kNumPosBitsMax),
		kLenNumLowBits = 3, kLenNumLowSymbols = (1 << kLenNumLowBits),
		kLenNumMidBits = 3, kLenNumMidSymbols = (1 << kLenNumMidBits),
		kLenNumHighBits = 8, kLenNumHighSymbols = (1 << kLenNumHighBits),
		kStartPosModelIndex = 4, kEndPosModelIndex = 14,
		kNumFullDistances = (1 << (kEndPosModelIndex >> 1)),
		kNumPosSlotBits = 6, kNumLenToPosStates = 4,
		kNumAlignBits = 4, kAlignTableSize = (1 << kNumAlignBits),
		kMatchMinLen = 2, kNumLitStates = 7, kNumStates = 12,
		id_match = 0, id_lit, id_r0, id_litr0, id_r1, id_r2, id_r3
	};

	short c_IsMatch[kNumStates][1 << kNumPosBitsMax];
	short c_IsRep[kNumStates];
	short c_IsRepG0[kNumStates];
	short c_IsRep0Long[kNumStates][1 << kNumPosBitsMax];
	short c_IsRepG1[kNumStates];
	short c_IsRepG2[kNumStates];
	short c_LenChoice[2];
	short c_LenChoice2[2];
	short c_Literal[1 << kNumLPosBitsMax][256][3][256];
	short c_LenLow[2][kNumPosStatesMax][1 << kLenNumLowBits];
	short c_LenMid[2][kNumPosStatesMax][1 << kLenNumMidBits];
	short c_LenHigh[2][1 << kLenNumHighBits];
	short c_PosSlot[kNumLenToPosStates][1 << kNumPosSlotBits];
	short c_SpecPos[kNumFullDistances - kEndPosModelIndex];
	short c_Align[1 << kNumAlignBits];

	int decode_bit(short& c);
	int entropyDecode_bit(short& c, double& totalBits);

public:

	LZMA(FILE* f);

	void decodeText(uint& textLen, uchar*& text, uchar*& marks);

	void entropydecodeText(uint& textLen, uchar*& text, uchar*& marks, uchar*& colorTypes);
};
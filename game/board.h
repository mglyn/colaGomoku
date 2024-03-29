﻿#pragma once

#include <iostream>
#include <queue>
#include <cassert>
#include <algorithm>
#include <bitset>

#include "../common/common.h"
#include "../genEval/genEval.h"
#include "pos.h"

constexpr int CAND_RANGE = 4;

struct Range {
	int x1 = 15, y1 = 15, x2 = -1, y2 = -1;

	void update(Pos pos, int length) {
		int x = pos.x(), y = pos.y();
		x1 = (std::min)(x1, x - CAND_RANGE);
		y1 = (std::min)(y1, y - CAND_RANGE);
		x2 = (std::max)(x2, x + CAND_RANGE);
		y2 = (std::max)(y2, y + CAND_RANGE);
		x1 = (std::max)(0, x1);
		y1 = (std::max)(0, y1);
		x2 = (std::min)(x2, length - 1);
		y2 = (std::min)(y2, length - 1);
	}

	int area() const { return (x2 - x1 + 1) * (y2 - y1 + 1); }
};

struct BoardInfo {
	int valueP1 = 0;
	std::array<std::array<int, 2>, FTYPE_NUM> cntT = {};
	Pos T4cost = NULLPOS;
	Pos lastMove = NULLPOS;
	Range range;
};

struct Unit {
	Eval::Line2 line2[4];
	uint8_t fType[2];
	uint8_t fValue[2];
};

class Board {
	int _length;
	int _cntMove;
	Piece _self;
	uint64_t hash;

	std::vector<BoardInfo> info;

	uint64_t codeLR[BOARD_LENGTH];				// L->R 低位->高位
	uint64_t codeUD[BOARD_LENGTH];				// U->D 
	uint64_t codeMain[2 * BOARD_LENGTH - 1];	// x = y
	uint64_t codeVice[2 * BOARD_LENGTH - 1];	// x = -y 
	void or2Bits(PieceCode p, Pos pos);
	void xor2Bits(PieceCode p, Pos pos);
	void and2Bits(PieceCode p, Pos pos);

	std::array<Piece, BOARD_SIZE> content;
	std::array<uint8_t, BOARD_SIZE> _cand;
	std::array<Unit, BOARD_SIZE> units;
	std::vector<std::array<Unit, 8 * Eval::HALF_LINE_LEN>> evalCache;

public:
	Board() { reset(); }

	bool notin(Pos pos) const {
		int x = pos.x(), y = pos.y();
		return x >= _length || y >= _length || x < 0 || y < 0;
	}
	int length() const { return _length; }
	int cntMove() const { return _cntMove; }

	Piece operator[](Pos pos) { return content[pos]; }
	FType type(Piece p, Pos pos) { return (FType)units[pos].fType[p]; }
	int value(Piece p, Pos pos) { return units[pos].fValue[p]; }
	bool cand(Pos pos) { return _cand[pos]; }

	Piece self() const { return _self; }
	int64_t hashKey() const { return hash; }
	Pos cost() const { return info[_cntMove].T4cost; }
	int cntFT(FType t, Piece p) const { return info[_cntMove].cntT[t][p]; }
	Range& candRange() { return info[_cntMove].range; }

	int staticEval() {
		return lastEval(0);
	}
	int lastEval(int r) {
		int id = (std::max)(1, _cntMove - r);

		int basicVal = (info[id].valueP1 + info[id - 1].valueP1) / 2;

		int threatVal = Eval::threatP1Eval(info[id].cntT);

		int val = basicVal + threatVal;
		return _self == P1 ? r & 1 ? -val : val : r & 1 ? val : -val;
	}
	Pos lastMove(int r) const { 
		int id = (std::max)(0, _cntMove - r + 1); 
		return info[id].lastMove; 
	}
	BoardInfo& bdnf() { return info[_cntMove]; }

	void reset(int len = 15);
	void update(Pos cd);
	void undo();
	Pos findPattern(Piece p, FType pat);

	void sssw(Piece p) { _self = p; }
};


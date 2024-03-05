#pragma once

#include "../common/common.h"
#include "../game/board.h"

enum GenMoveMod {
	G_threat,
	G_quiet,
	G_pseudoVCF,
	G_VCF,
	G_all
};

enum PickerMod {
	P_main,
	P_VCF
};

struct Move {
	Pos pos;
	int val;
	Move(Pos pos, int val) :pos(pos), val(val) {}
	bool operator < (const Move& a)const {
		return val < a.val;
	}
	bool operator > (const Move& a)const {
		return val > a.val;
	}
};

class MovePicker {
	Board& bd;
	int dep;

	Pos ttMove;
	int stage;
	std::vector<Move> moves;
	int cur;
public:
	MovePicker(PickerMod mod, Board &bd, Pos ttMove, int dep = 0);
	Pos nextMove(bool skipQuiets = false);
};

template<GenMoveMod mod>
void genMove(Board& bd, std::vector<Move>& moves);
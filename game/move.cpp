#include "move.h"
#include "../genEval/genEval.h"
#include "../test/test.h"
#include "../search/search.h"

enum MoveStage {

	M_main_tt,
	M_threat_init,
	M_threat,
	M_quiet_init,
	M_quiet,

	M_VCF_tt,
	M_VCF_init,
	M_VCF,

	M_all
};

MovePicker::MovePicker(PickerMod mod, Board& bd, Pos ttMove, int dep) :
	bd(bd),
	ttMove(ttMove),
	dep(dep),
	cur(0) {
	stage = (mod == P_main ? M_main_tt : M_VCF_tt) + (ttMove == NULLPOS);
	moves.reserve(bd.candRange().area() - bd.cntMove());
}

Pos MovePicker::nextMove(bool skipQuiets) {
	switch (stage) {
	
	case M_main_tt:
	case M_VCF_tt:

		stage++;
		return ttMove;

	case M_threat_init:
		
		genMove<G_threat>(bd, moves);
		stage++;
		[[fallthrough]];

	case M_threat:

		for (; cur < moves.size(); cur++) {
			std::swap(*std::max_element(moves.begin() + cur, moves.end()), moves[cur]);

			if (moves[cur].pos != ttMove) return moves[cur++].pos;
		}

		stage++;
		[[fallthrough]];

	case M_quiet_init:

		if (!skipQuiets) {
			genMove<G_quiet>(bd, moves);

			for (int i = cur; i < moves.size();) {

				//skip trivial move
				while (i < moves.size() && 
					dep <= 4 && moves[i].val < 5) {
					moves[i] = moves.back();
					moves.pop_back();
				}

				if (i < moves.size()) {
					int j = i;
					Move tmp = moves[i];
					for (; j >= cur + 1 && moves[j - 1] < tmp; j--) {
						std::swap(moves[j], moves[j - 1]);
					}
					moves[j] = tmp;
					i++;
				}
			}
				
		}

		stage++;
		[[fallthrough]];

	case M_quiet:

		for (; cur < moves.size(); cur++) {
			if (moves[cur].pos != ttMove) return moves[cur++].pos;
		}

		return NULLPOS;

	case M_VCF_init:

		genMove<G_pseudoVCF>(bd, moves);
		stage++;
		[[fallthrough]];

	case M_VCF:

		for (; cur < moves.size(); cur++) {
			std::swap(*std::max_element(moves.begin() + cur, moves.end()), moves[cur]);

			if (moves[cur].pos != ttMove) return moves[cur++].pos;
		}

		return NULLPOS;
	}
	assert(0);
	return 0;
}

template<GenMoveMod mod>
void genMove(Board& bd, std::vector<Move>& moves) {
	Piece self = bd.self(), oppo = ~self;

	if constexpr (mod == G_threat) {
		if (bd.cntFT(TH4, oppo)) {				//对方有成双4
			for (int i = bd.candRange().x1; i <= bd.candRange().x2; i++) {
				for (int j = bd.candRange().y1; j <= bd.candRange().y2; j++) {
					Pos pos(i, j);
					if (bd[pos] == Empty && bd.cand(pos) &&
						(bd.type(self, pos) >= T4 || bd.type(oppo, pos) >= T4)) {
						moves.emplace_back(pos, 2 * bd.value(self, pos) + bd.value(oppo, pos));
					}
				}
			}
		}
		else {
			for (int i = bd.candRange().x1; i <= bd.candRange().x2; i++) {
				for (int j = bd.candRange().y1; j <= bd.candRange().y2; j++) {
					Pos pos(i, j);
					if (bd[pos] == Empty && bd.cand(pos) &&
						(bd.type(self, pos) >= TH3 || bd.type(oppo, pos) >= TH3))
						moves.emplace_back(pos, 2 * bd.value(self, pos) + bd.value(oppo, pos));
				}
			}
		}
	}
	if constexpr (mod == G_quiet) {
		for (int i = bd.candRange().x1; i <= bd.candRange().x2; i++) {
			for (int j = bd.candRange().y1; j <= bd.candRange().y2; j++) {
				Pos pos(i, j);
				if (bd[pos] == Empty && bd.cand(pos) &&
					bd.type(self, pos) == TNone && bd.type(oppo, pos) == TNone)
					moves.emplace_back(pos, 2 * bd.value(self, pos) + bd.value(oppo, pos));
			}
		}
	}
	if constexpr (mod == G_VCF) {
		assert(0);
	}
	if constexpr (mod == G_pseudoVCF) {
		Pos pos = bd.lastMove(1);
		constexpr int len = arrLen(EXPAND_S2L4);
		for (int i = 0; i < len; i++) {
			Pos npos = pos + EXPAND_S2L4[i];
			if (bd[npos] == Empty &&
				bd.type(self, npos) >= T4)
				moves.emplace_back(npos, 2 * bd.value(self, npos) + bd.value(oppo, npos));
		}
	}
	if constexpr (mod == G_all) {
		for (int i = bd.candRange().x1; i <= bd.candRange().x2; i++) {
			for (int j = bd.candRange().y1; j <= bd.candRange().y2; j++) {
				Pos pos(i, j);
				if (bd[pos] == Empty && bd.cand(pos))
					moves.emplace_back(pos, 2 * bd.value(self, pos) + bd.value(oppo, pos));
			}
		}
	}
}

template void genMove<G_threat>(Board& bd, std::vector<Move>& moves);
template void genMove<G_quiet>(Board& bd, std::vector<Move>& moves);
template void genMove<G_pseudoVCF>(Board& bd, std::vector<Move>& moves);
template void genMove<G_VCF>(Board& bd, std::vector<Move>& moves);
template void genMove<G_all>(Board& bd, std::vector<Move>& moves);
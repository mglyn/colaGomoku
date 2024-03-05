#pragma once

#include "../common/common.h"
#include "../game/board.h"
#include "../game/move.h"
#include "time.h"

namespace Search {

	constexpr int MAX_PLY = 128;
	constexpr int END_DEP = 127;

	constexpr int win(int ply) { return WIN_MAX - ply; }

	constexpr int loss(int ply) { return -WIN_MAX + ply; }

	enum NType {
		NonPV,
		PV,
		Root
	};

	struct Stack {
		int ply;
		Pos* pv;
		Pos move;
		Pos excludedMove;
		int eval;
		bool ttpv;
		FType moveFT[2];
	};

	struct RootMove {
		Pos pos = NULLPOS;
		int val = -VAL_INF;
		int lastVal = -VAL_INF;
		int avg = -VAL_INF;
		std::vector<Pos> pv;
		RootMove(Pos pos, int val, int lastVal) :pos(pos), val(val), lastVal(lastVal) {}
	};
	extern std::vector<RootMove> rootMoves;

	class Worker {
		Timer timer;
		Board& bd;
		Stack ss;

		//std::vector<RootMove> rootMoves;
		int pvIdx = 0;
		void iterative_deepening();
		int search(NType NT, Stack* ss, int alpha, int beta, int dep, bool cutNode);
		int VCFSearch(NType NT, Stack* ss, int alpha, int beta, int dep);
		int quickEnd(Stack* ss);
	public:
		Worker(Board& bd) : bd(bd) {}
		void start();
	};
}


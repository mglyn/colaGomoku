#include <iostream>

#include "search.h"
#include "tt.h"
#include "../test/test.h"
#include "../genEval/genEval.h"

namespace Search {
	std::vector<RootMove> rootMoves;

	std::array<int, std::max(MAX_MOVE + 1, MAX_PLY + 10)> _20log;

	const auto init = []() {
		for (int i = 1; i < std::max(MAX_MOVE + 1, MAX_PLY + 10); i++) {
			_20log[i] = 20. * log(i);
		}
		return true; }();

	void updatePV(Pos* pv, Pos pos, Pos* childPV) {
		for (*pv++ = pos; childPV && *childPV;) 
			*pv++ = *childPV++;
		*pv = NULLPOS;
	}

	int Worker::quickEnd(Stack* ss) {
		Piece self = bd.self(), oppo = ~self;

		if (bd.cntFT(T5, self)) return win(ss->ply);

		int oppo5 = bd.cntFT(T5, oppo);
		if (oppo5) {
			return oppo5 > 1 ? loss(ss->ply + 1) : 0;
		}

		if (bd.cntFT(TH4, self))return win(ss->ply + 2);

		//if (bd.cntFT(T4H3, self))return win(ss->ply + 4);

		return 0;
	}

	int Worker::VCFSearch(NType NT, Stack* ss, int alpha, int beta, float dep) {

		vcfnode++;

		bool pvNode = NT != NonPV;
		Pos pv[MAX_PLY + 1];

		Piece self = bd.self(), oppo = ~self;
		int oppoT5 = bd.cntFT(T5, oppo);
		int val;

		assert(-WIN_MAX <= alpha && alpha < beta && beta <= WIN_MAX);
		assert(pvNode || (alpha == beta - 1));

		//检查平局
		if (bd.candRange().area() - bd.cntMove() == 0) return 0;

		//max ply
		if (ss->ply > MAX_PLY)return bd.staticEval();

		//检查赢/输棋
		if (int q = quickEnd(ss); q) return q;

		//mate distance pruning
		alpha = std::max(loss(ss->ply), alpha);
		beta = std::min(win(ss->ply + 1), beta);
		if (alpha >= beta) return alpha;

		//T5跳过节点
		if (oppoT5) {
			Pos pos = bd.cost();

			bd.update(pos);

			if (pvNode) {
				pv[0] = NULLPOS;
				(ss + 1)->pv = pv;

				val = -VCFDefend(PV, ss + 1, -beta, -alpha, dep);
			}
			else val = -VCFDefend(NonPV, ss + 1, -beta, -alpha, dep);

			bd.undo();

			if (pvNode) updatePV(ss->pv, pos, (ss + 1)->pv);

			return val;
		}
		
		int bestVal = -VAL_INF;
		Pos bestMove = NULLPOS;

		uint64_t key = bd.hashKey();
		HashEntry* tte;
		int ttVal = -VAL_INF;
		Pos ttMove = NULLPOS;
		int ttDep = -VAL_INF;
		int ttpv = false;
		HashType ttBound = B_Initial;

		//访问置换表
		bool ttHit = tt.probe(key, tte);
		if (ttHit) {
			ttVal = tte->value(ss->ply);
			ttMove = tte->movePos();
			ttDep = tte->depth();
			ttpv = tte->isPV();
			ttBound = tte->type();
		}

		//使用置换表中的值截断
		if (ttDep >= dep && ttVal != -VAL_INF) {
			if (ttBound & B_Lower)
				alpha = std::max(alpha, ttVal);
			if (ttBound & B_Upper)
				beta = std::min(beta, ttVal);
			if (alpha >= beta) {
				vcfhcut++;
				return ttVal;
			}
		}

		bestVal = bd.staticEval();
		if (bestVal >= beta) {
			return bestVal;
		}

		if (pvNode) alpha = std::max(alpha, bestVal);

		MovePicker mp(bd, NULLPOS, G_pseudoVCF);
		for (Pos pos; pos = mp.nextMove(); ) {

			bd.update(pos);

			if (pvNode) {
				pv[0] = NULLPOS;
				(ss + 1)->pv = pv;
			}

			val = -VCFDefend(NT, ss + 1, -beta, -alpha, dep - 1);

			bd.undo();

			if (timer.time_out())return 0;

			if (val > bestVal) {
				bestVal = val;

				if (val > alpha) {
					bestMove = pos;

					// Update pv even in fail-high case
					if (pvNode) updatePV(ss->pv, pos, (ss + 1)->pv);

					if (pvNode && val < beta)  // Update alpha
						alpha = val;
					else {
						vcfbcut++;
						break;
					} 
				}
			}
		}

		HashType bound = bestVal >= beta ? B_Lower :
			pvNode && bestMove ? B_Exact : B_Lower;
		tte->store(key, bestMove, bestVal, dep, ss->ply, bound);

		return bestVal;
	}

	int Worker::VCFDefend(NType NT, Stack* ss, int alpha, int beta, float dep) {

		defnode++;

		bool pvNode = NT != NonPV;
		Pos pv[MAX_PLY + 1];

		Piece self = bd.self(), oppo = ~self;

		if (!bd.cntFT(T5, oppo))return bd.staticEval();

		Pos pos = bd.cost();

		bd.update(pos);

		if (pvNode) {
			pv[0] = NULLPOS;
			(ss + 1)->pv = pv;
		}

		int val = -VCFSearch(NT, ss + 1, -beta, -alpha, dep);

		bd.undo();

		if (pvNode) updatePV(ss->pv, pos, (ss + 1)->pv);

		return val;
	}

	int Worker::search(NType NT, Stack* ss, int alpha, int beta, float dep, bool cutNode) {

		bool pvNode = NT != NonPV;
		bool rootNode = NT == Root;
		Pos pv[MAX_PLY + 1];

		Piece self = bd.self(), oppo = ~self;
		int oppoT5 = bd.cntFT(T5, oppo);
		int oppoTH4 = bd.cntFT(TH4, oppo) + bd.cntFT(T4H3, oppo);
		int val;

		//深度小于零搜索VCF
		if (dep <= 0) {
			return bd.staticEval();
			return oppoT5 ? VCFDefend(NT, ss, alpha, beta, 0) :
				VCFSearch(NT, ss, alpha, beta, 0);
		}
			

		assert(-WIN_MAX <= alpha && alpha < beta && beta <= WIN_MAX);
		assert(pvNode || (alpha == beta - 1));

		node++;

		if (!rootNode) {
			//检查平局
			if (bd.candRange().area() - bd.cntMove() == 0) return 0;

			//max ply
			if (ss->ply > MAX_PLY)return bd.staticEval();

			//检查赢/输棋
			if (val = quickEnd(ss)) return val;

			//mate distance pruning
			alpha = std::max(loss(ss->ply), alpha);
			beta = std::min(win(ss->ply + 1), beta);
			if (alpha >= beta) return alpha;
		}

		//T5跳过节点
		if (oppoT5) {
			Pos pos = bd.cost();

			bd.update(pos);

			if (pvNode) {
				pv[0] = NULLPOS;
				(ss + 1)->pv = pv;

				val = -search(PV, ss + 1, -beta, -alpha, dep, false);
			}
			else val = -search(NonPV, ss + 1, -beta, -alpha, dep, !cutNode);

			bd.undo();

			if (pvNode && !rootNode) updatePV(ss->pv, pos, (ss + 1)->pv);

			return val;
		}

		int bestVal = -VAL_INF;
		Pos bestMove = NULLPOS;
		int moveCnt = 0;

		uint64_t key = bd.hashKey();
		int ttVal = -VAL_INF;
		Pos ttMove = NULLPOS;
		int ttDep = -VAL_INF;
		int ttpv = false;
		HashType ttBound = B_Initial;
		HashEntry* tte = nullptr;

		//估值
		int eval = rootNode ? -VAL_INF : bd.staticEval();
		bool improving = eval - bd.lastEval(2) > 0;

		//访问置换表
		bool ttHit = tt.probe(key, tte);
		if (ttHit) {
			ttVal = tte->value(ss->ply);
			ttMove = tte->movePos();
			ttDep = tte->depth();
			ttpv = tte->isPV();
			ttBound = tte->type();
		}

		if (!pvNode) {
			if (ttDep >= dep &&
				ttVal < WIN_CRITICAL && ttVal > -WIN_CRITICAL &&
				(ttVal >= beta ? (ttBound & B_Lower) : (ttBound & B_Upper))) {
				hcut++;
				return ttVal;
			}

			//razoring
			if (dep <= 4 && eval + 50 * dep * dep + 30 < alpha) {
				razor[(int)dep]++;
				return eval;
				return VCFSearch(NT, ss, alpha, alpha + 1, 0);
			}

			//futility pruning
			if (!oppoTH4 && dep <= 4 && beta > -WIN_CRITICAL &&
				eval - ((150 - 10 * improving - 10 * (!ttHit && cutNode)) * dep) > beta) {
				futility[(int)dep]++;
				return eval;
			}
		}

		//内部迭代加深  
		if (!rootNode && pvNode && !ttMove) 
			dep -= 2;

		if (dep <= 0) {
			return bd.staticEval();
			return oppoT5 ? VCFDefend(NT, ss, alpha, beta, 0) :
				VCFSearch(NT, ss, alpha, beta, 0);
		}

		if (dep >= 9 && cutNode && !ttMove)
			dep -= 2;

		//尝试所有着法直到产生beta截断
		MovePicker mp(bd, NULLPOS, G_candidates);
		bool skipQuietMove = false;

		for (Pos pos; pos = mp.nextMove(skipQuietMove); ) {

			if (rootNode && !std::count_if(rootMoves.begin() + pvIdx, rootMoves.end(), [&pos](auto& a) {return pos == a.pos;}))
				continue;

			moveCnt++;

			bool dispersed = Pos::dis1(bd.lastMove(1), pos) > 4 && Pos::dis1(bd.lastMove(2), pos) > 4;
			bool trivial = bd.value(self, pos) < Eval::TRIVIAL_CRITICAL && bd.value(oppo, pos) < Eval::TRIVIAL_CRITICAL;

			//pruning at shallow depth
			if (!rootNode && bestVal > -WIN_CRITICAL) {
				//moveCount pruning
				if (!skipQuietMove && moveCnt > (improving ? 1.f : 0.7f) * dep * dep + 3) {
					tttt[1]++;
					skipQuietMove = true;
				}
					
				//prune the dispersed move
				if (dispersed && moveCnt > dep * dep + 3) {
					tttt[2]++;
					continue;
				}
				if (dispersed && oppoTH4 && dep <= 4) {
					tttt[3]++;
					continue;
				}
				if (trivial && dep <= 4) {
					tttt[4]++;
					break;
				}
				
			}

			int extension = 0;

			extension += oppoTH4;

			int newDep = dep + extension - 2;

			bd.update(pos);

			//late move reduction
			bool fullDep;
			int reduction = (_20log[dep] * _20log[moveCnt] + 480 * (!improving)) / 2080;
			if (newDep - reduction > 0 && moveCnt > 1 + rootNode) {
				val = -search(NonPV, ss + 1, -alpha - 1, -alpha, newDep - reduction, true);
				fullDep = val > alpha;
			}
			else fullDep = !pvNode || moveCnt > 1;

			//full depth seach
			if (fullDep) {
				val = -search(NonPV, ss + 1, -alpha - 1, -alpha, newDep, !cutNode);
			}

			if (pvNode && (moveCnt == 1 || val > alpha)) {

				pv[0] = NULLPOS;
				(ss + 1)->pv = pv;

				val = -search(PV, ss + 1, -beta, -alpha, newDep, false);
			}

			bd.undo();

			if (timer.time_out()) return 0;

			if (rootNode) {
				RootMove& rm = *std::find_if(rootMoves.begin(), rootMoves.end(), [&pos](auto& a) {return a.pos == pos; });

				rm.avg = rm.avg == -VAL_INF ? val : (2 * val + rm.avg) / 3;

				if (moveCnt == 1 || val > alpha) {

					rm.val = val;

					rm.pv.clear();
					for (Pos* p = (ss + 1)->pv; *p; p++)
						rm.pv.push_back(*p);
				}
				else rm.val = -VAL_INF;
			}

			if (val > bestVal) {
				bestVal = val;

				if (val > alpha) {
					bestMove = pos;

					// Update pv even in fail-high case
					if (pvNode && !rootNode) updatePV(ss->pv, pos, (ss + 1)->pv);

					if (pvNode && val < beta)  // Update alpha
						alpha = val;
					else {
						bcut++;
						break;
					}
				}
			}
		}

		HashType bound = bestVal >= beta ? B_Lower :
			pvNode && bestMove ? B_Exact : B_Lower;
		tte->store(key, bestMove, bestVal, dep, ss->ply, bound);

		return bestVal;
	}

	void Worker::iterative_deepening() {
		
		int bestVal = -VAL_INF;
		std::vector<Move> moves;
		genMove(bd, moves, G_candidates);
		int multiPV = 1;
		bool stop = false;

		Stack stack[MAX_PLY + 10] = {};
		Stack* ss = stack + 5;

		for (int i = 0; i < MAX_PLY + 5; i++) 
			(ss + i)->ply = i;

		for (auto& move : moves) {
			rootMoves.emplace_back(move.pos, -VAL_INF, -VAL_INF);
		}
		auto cmpRootMove = [](const RootMove& a, const RootMove& b) {
			return a.val == b.val ? a.lastVal > b.lastVal : a.val > b.val;
		};
		multiPV = std::min(multiPV, (int)rootMoves.size());
		

		for (int dep = 5; !stop && dep <= MAX_PLY; dep++) {

			for (auto& rm : rootMoves)
				rm.lastVal = rm.val;

			for (pvIdx = 0; !stop && pvIdx < multiPV; pvIdx++) {

				//aspiration search
				auto& rm = rootMoves[pvIdx];
				int delta = rm.avg * rm.avg / 2000 + 5;
				int alpha = std::max(rm.avg - delta, -WIN_MAX);
				int beta = std::min(rm.avg + delta, WIN_MAX);

				while (true) {
					std::cout << "dep: " << dep << " alpha: " << alpha << " beta:" << beta << "\n";
					bestVal = search(Root, ss, alpha, beta, dep, false);
					std::cout << " val: " << bestVal << "\n";

					std::stable_sort(rootMoves.begin() + pvIdx, rootMoves.end(), cmpRootMove);

					if (timer.time_out() || abs(bestVal) >= WIN_CRITICAL) {
						stop = true;
						break;
					}

					if (bestVal <= alpha) {
						beta = (alpha + beta) / 2;
						alpha = std::max(bestVal - delta, -WIN_MAX);
					}
					else if (bestVal >= beta) {
						beta = std::min(bestVal + delta, WIN_MAX);
					}
					else break;

					delta += delta / 3;
				}

				std::stable_sort(rootMoves.begin(), rootMoves.begin() + pvIdx + 1, cmpRootMove);
			}
		}
	}

	void Worker::start() {
		rootMoves.clear();
		

		if (bd.cntMove() == 0) {
			rootMoves.push_back(RootMove({ 7,7 }, 0, 0));
			return;
		}
		if (bd.cntFT(T5, bd.self())) {
			std::vector<Move> moves;
			genMove(bd, moves, G_legal);
			Pos pos = find_if(moves.begin(), moves.end(), [this](auto& a) {return bd.type(bd.self(), a.pos) == T5; })->pos;
			rootMoves.push_back(RootMove(pos, 0, 0));
			return;
		}
		iterative_deepening();
	}
}


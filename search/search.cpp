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

		if (bd.cntFT(TDH3, self) + bd.cntFT(T4H3, self) && !bd.cntFT(T4, oppo)) {
			te[TDH3T4H3wincheck]++;
			return win(ss->ply + 4);
		}

		return 0;
	}

	int Worker::VCFSearch(NType NT, Stack* ss, int alpha, int beta, int dep) {

		te[vcfnode]++;

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
			Pos move = bd.cost();

			bd.update(move);
			tt.prefetch(bd.hashKey());

			if (pvNode) {
				pv[0] = NULLPOS;
				(ss + 1)->pv = pv;

				val = -VCFSearch(PV, ss + 1, -beta, -alpha, dep);
			}
			else val = -VCFSearch(NonPV, ss + 1, -beta, -alpha, dep);

			bd.undo();

			if (pvNode) updatePV(ss->pv, move, (ss + 1)->pv);

			return val;
		}
		
		int bestVal = -VAL_INF;
		Pos bestMove = NULLPOS;

		uint64_t key = bd.hashKey();
		HashEntry* tte;
		int ttVal = -VAL_INF;
		Pos ttMove = NULLPOS;
		int ttDep = -VAL_INF;
		bool pvHit = false;
		HashType ttBound = B_Initial;

		//访问置换表
		bool ttHit = tt.probe(key, tte);
		if (ttHit) {
			ttVal = tte->value(ss->ply);

			ttMove = tte->movePos();
			if (ttMove != NULLPOS && bd[tte->movePos()] != Empty) {
				ttMove = NULLPOS;
				te[tothasherror]++;
			}

			ttDep = tte->depth();
			pvHit = tte->isPV();
			ttBound = tte->type();
		}

		//使用置换表中的值截断
		if (ttDep >= dep && ttVal != -VAL_INF) {
			if (ttBound & B_Lower)
				alpha = std::max(alpha, ttVal);
			if (ttBound & B_Upper)
				beta = std::min(beta, ttVal);
			if (alpha >= beta) {
				te[vcfTTcutoff]++;
				return ttVal;
			}
		}

		bestVal = bd.staticEval();
		if (bestVal >= beta) {
			return bestVal;
		}

		if (pvNode) alpha = std::max(alpha, bestVal);

		MovePicker mp(P_VCF, bd, NULLPOS);
		for (Pos move; move = mp.nextMove(); ) {

			bd.update(move);

			if (pvNode) {
				pv[0] = NULLPOS;
				(ss + 1)->pv = pv;
			}

			val = -VCFSearch(NT, ss + 1, -beta, -alpha, dep - 2);

			bd.undo();

			if (timer.time_out())return 0;

			if (val > bestVal) {
				bestVal = val;

				if (val > alpha) {
					bestMove = move;

					// Update pv even in fail-high case
					if (pvNode) updatePV(ss->pv, move, (ss + 1)->pv);

					if (pvNode && val < beta)  // Update alpha
						alpha = val;
					else {
						te[vcfbetacutoff]++;
						break;
					} 
				}
			}
		}

		HashType bound = bestVal >= beta ? B_Lower : B_Upper;
		tte->store(key, pvHit, bestMove, bestVal, dep, ss->ply, bound);

		return bestVal;
	}

	int Worker::search(NType NT, Stack* ss, int alpha, int beta, int dep, bool cutNode) {

		bool pvNode = NT != NonPV;
		bool rootNode = NT == Root;
		Pos pv[MAX_PLY + 1];

		Piece self = bd.self(), oppo = ~self;
		int oppoT5 = bd.cntFT(T5, oppo);
		int oppoTH4 = bd.cntFT(TH4, oppo);
		int val;

		//深度小于零搜索VCF
		if (dep <= 0)
			return VCFSearch(NT, ss, alpha, beta, 0);

		assert(-WIN_MAX <= alpha && alpha < beta && beta <= WIN_MAX);
		assert(pvNode || (alpha == beta - 1));

		te[node]++;

		if (!rootNode) {
			//检查平局
			if (bd.candRange().area() - bd.cntMove() == 0) 
				return 0;

			//max ply
			if (ss->ply > MAX_PLY)
				return bd.staticEval();

			//检查赢/输棋
			if (val = quickEnd(ss)) 
				return val;

			//mate distance pruning
			alpha = std::max(loss(ss->ply), alpha);
			beta = std::min(win(ss->ply + 1), beta);
			if (alpha >= beta) 
				return alpha;

			//T5跳过节点
			if (oppoT5) {
				Pos move = bd.cost();

				bd.update(move);
				tt.prefetch(bd.hashKey());

				if (pvNode) {
					pv[0] = NULLPOS;
					(ss + 1)->pv = pv;

					val = -search(PV, ss + 1, -beta, -alpha, dep, false);
				}
				else val = -search(NonPV, ss + 1, -beta, -alpha, dep, !cutNode);

				bd.undo();

				if (pvNode && !rootNode) 
					updatePV(ss->pv, move, (ss + 1)->pv);

				return val;
			}
		}

		int bestVal = -VAL_INF;
		Pos bestMove = NULLPOS;
		int moveCnt = 0;

		uint64_t key = bd.hashKey();
		int ttVal = -VAL_INF;
		Pos ttMove = NULLPOS;
		int ttDep = -VAL_INF;
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
			if (ttMove != NULLPOS && bd[tte->movePos()] != Empty) {
				ttMove = NULLPOS;
				te[tothasherror]++;
			}
			ttDep = tte->depth();
			ttBound = tte->type();

			if (!ss->excludedMove) 
				ss->ttpv = pvNode || tte->isPV();
		}

		if (!pvNode && !ss->excludedMove && ttDep > dep &&
			std::abs(ttVal) < WIN_CRITICAL &&
			(ttVal >= beta ? (ttBound & B_Lower) : (ttBound & B_Upper))) {
			te[TTcutoff]++;
			return ttVal;
		}

		//razoring
		if (dep <= 4 && eval + 36 * dep * dep + 55 < alpha) {
			te[razor + dep]++;
			return VCFSearch(NonPV, ss, alpha, alpha + 1, 0);
		}

		//futility pruning
		if (!ss->ttpv && !oppoTH4 && beta > -WIN_CRITICAL &&
			eval - ((30 - 4 * improving - 4 * (!ttHit && cutNode)) * (0.5 * dep * dep + dep) + 70) > beta) {
			te[futility + dep]++;
			return eval;
		}
		

		//内部迭代加深  
		if (!rootNode && pvNode && !ttMove) 
			dep -= 3;

		if (dep <= 0) 
			return VCFSearch(NT, ss, alpha, beta, 0);

		if (dep >= 9 && cutNode && !ttMove)
			dep -= 3;

		//尝试所有着法直到产生beta截断
		MovePicker mp(P_main, bd, ttMove);
		bool skipQuietMove = false;

		for (Pos move; move = mp.nextMove(skipQuietMove); ) {

			if (rootNode && !std::count_if(rootMoves.begin() + pvIdx, rootMoves.end(), [&move](auto& a) {return move == a.pos;}))
				continue;

			if (move == ss->excludedMove)continue;

			assert(bd[move] == Empty);

			moveCnt++;
			ss->moveFT[P1] = bd.type(P1, move);
			ss->moveFT[P2] = bd.type(P2, move);

			int disSelf = Pos::dis1(bd.lastMove(1), move);
			int disOppo = Pos::dis1(bd.lastMove(2), move);
			bool dispersed = disSelf > 4 && disOppo > 4;

			//pruning at shallow depth
			if (!rootNode && bestVal > -WIN_CRITICAL) {
				//moveCount pruning
				if (!skipQuietMove && moveCnt > (improving ? 0.8f : .6f) * dep * dep + 10) {
					te[moveCntpruning]++;
					skipQuietMove = true;
				}
					
				//prune the dispersed move
				if (dispersed && oppoTH4 && ss->moveFT[oppo] < T4 && dep <= 4) {
					te[dispersedT]++;
					continue;
				}
			}

			//extentions
			int extension = 0;

			//extent when oppo threats
			if(oppoTH4)
				extension += 1;

			// Singular extension search. If all moves but one fail low on a
			// search of (alpha-s, beta-s), and just one fails high on (alpha, beta),
			// then that move is singular and should be extended. To verify this we do
			// a reduced search on the position excluding the ttMove and if the result
			// is lower than ttValue minus a margin, then we will extend the ttMove.
			if (!rootNode && move == ttMove && !ss->excludedMove && 
				dep >= 8 + 2 * ss->ttpv &&
				std::abs(ttVal) < WIN_CRITICAL &&
				(ttBound & B_Lower) &&
				ttDep >= dep - 3) {
				
				int singularBeta = ttVal - (3 + (ss->ttpv && !pvNode)) * dep;
				int singularDep = dep / 2;

				ss->excludedMove = move;
				val = search(NonPV, ss, singularBeta - 1, singularBeta, singularDep, cutNode);
				ss->excludedMove = NULLPOS;

				if (val < singularBeta) {
					extension += 3;
				}
				// Multi-cut pruning
				// Our ttMove is assumed to fail high based on the bound of the TT entry,
				// and if after excluding the ttMove with a reduced search we fail high over the original beta,
				// we assume this expected cut-node is not singular (multiple moves fail high),
				// and we can prune the whole subtree by returning a softbound.
				else if (singularBeta >= beta)
					return singularBeta;
				// Negative extensions
				// If other moves failed high over (ttValue - margin) without the ttMove on a reduced search,
				// but we cannot do multi-cut because (ttValue - margin) is lower than the original beta,
				// we do not know if the ttMove is singular or can do a multi-cut,
				// so we reduce the ttMove in favor of other moves based on some conditions:
				else if (ttVal >= beta)
					extension -= 2;
			}

			int newDep = dep + extension - 2;

			bd.update(move);
			if (ss->moveFT[self] != T4)
				tt.prefetch(bd.hashKey());
			
			//late move reduction
			int reduction = (_20log[dep] * _20log[moveCnt] + 600 * !improving) / 1400;

			//Decrease reduction if position is or has been on the PV
			if (ss->ttpv)
				reduction -= 2;

			//Increase reduction for cut nodes
			if (cutNode)
				reduction += 2;

			//Increase reduction for useless defend move 
			if (oppoTH4 && ss->moveFT[self] < T4)
				reduction += disSelf > 4 + 2 * (disOppo > 4);

			//Decrease reduction for continous attack
			if (!oppoTH4 && (ss - 2)->moveFT[self] >= TH3 && ss->moveFT[self] >= TH3)
				reduction -= 1;

			bool fullDep;
			if (reduction > 0 && 
				newDep > reduction &&
				moveCnt > 1 + rootNode) {
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
				RootMove& rm = *std::find_if(rootMoves.begin(), rootMoves.end(), [&move](auto& a) {return a.pos == move; });

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
					bestMove = move;

					// Update pv even in fail-high case
					if (pvNode && !rootNode) updatePV(ss->pv, move, (ss + 1)->pv);

					if (pvNode && val < beta)  // Update alpha
						alpha = val;
					else {
						te[betacutoff]++;
						break;
					}
				}
			}
		}

		HashType bound = bestVal >= beta ? B_Lower :
			pvNode && bestMove ? B_Exact : B_Upper;
		tte->store(key, ss->ttpv, bestMove, bestVal, dep, ss->ply, bound);

		return bestVal;
	}

	void Worker::iterative_deepening() {
		
		int bestVal = -VAL_INF;
		std::vector<Move> moves;
		genMove<G_all>(bd, moves);
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
		

		for (int dep = 5, endDep = VAL_INF; !stop && dep <= std::min(END_DEP, endDep); dep++) {

			for (auto& rm : rootMoves)
				rm.lastVal = rm.val;

			for (pvIdx = 0; !stop && pvIdx < multiPV; pvIdx++) {

				//aspiration search
				auto& rm = rootMoves[pvIdx];
				int delta = std::min(std::abs(rm.avg) / 14 + 11, VAL_INF);
				int alpha = std::max(rm.avg - delta, -WIN_MAX);
				int beta = std::min(rm.avg + delta, WIN_MAX);
				int failHighCnt = 0;

				while (true) {
					
					bestVal = search(Root, ss, alpha, beta, dep - failHighCnt / 4, false);
					printf("d: %d   ab: [%d, %d]   v: %d\n", dep, alpha, beta, bestVal);

					std::stable_sort(rootMoves.begin() + pvIdx, rootMoves.end(), cmpRootMove);
					
					if (timer.time_out()) {
						stop = true;
						break;
					}
					//After finding the checkmate, search slightly deeper in an attempt to find a better solution
					if (bestVal >= WIN_CRITICAL && endDep == VAL_INF)
						endDep = dep + 3;

					if (bestVal <= alpha) {
						beta = (alpha + beta) / 2;
						alpha = std::max(bestVal - delta, -WIN_MAX);
						failHighCnt = 0;
					}
					else if (bestVal >= beta) {
						beta = std::min(bestVal + delta, WIN_MAX);
						failHighCnt++;
					}
					else break;
					
					delta = std::min(delta * 4 / 3, VAL_INF);

					assert(-WIN_MAX <= alpha && alpha < beta && beta <= WIN_MAX);
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
			genMove<G_all>(bd, moves);
			Pos pos = find_if(moves.begin(), moves.end(), [this](auto& a) {return bd.type(bd.self(), a.pos) == T5; })->pos;
			rootMoves.push_back(RootMove(pos, 0, 0));
			return;
		}
		iterative_deepening();
	}
}


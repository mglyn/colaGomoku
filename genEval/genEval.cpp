#include <cassert>
#include <iostream>

#include "genEval.h"
#include "data.h"

namespace Eval {
	Line2 codeToLineID2[NUMCODE];
	Flower lineToFlower[NUMLINE4DIR];
	uint8_t threatStateToP1Eval[1 << 2 * (FTYPE_NUM - 1)];

	Line2 decode1(uint64_t code) {
		return codeToLineID2[(code >> 2) & 0b1111111100000000 | code & 0b11111111];
	}
	Flower decode2(int l1, int l2, int l3, int l4) { 
		return lineToFlower[(l1 << 12) + (l2 << 8) + (l3 << 4) + l4]; 
	}

	int getThreatStat(std::array<std::array<int, 2>, FTYPE_NUM>& cntT) {
		int threatStat = 0;
		for (int i = 1; i < FTYPE_NUM; i++) {
			threatStat |= int(bool(cntT[i][P1])) << 2 * i - 2;
			threatStat |= int(bool(cntT[i][P2])) << 2 * i - 1;
		}
		return threatStat;
	}

	int threatP1Eval(std::array<std::array<int, 2>, FTYPE_NUM>& cntT) {
		return threatStateToP1Eval[getThreatStat(cntT)];
	}

	void ACautomation::insertPattern(const std::vector<int>& pattern, int lineID) {
		int now = root;
		for (int c : pattern) {
			if (nodes[now].edge[c] != 0)
				now = nodes[now].edge[c];
			else {
				nodes[now].edge[c] = ++cnt_node;
				nodes[nodes[now].edge[c]].father = now;
				now = nodes[now].edge[c];
			}
		}
		nodes[now].lineType = lineID;
	}

	void ACautomation::calc_fail() {
			nodes[root].fail = 0;
			std::queue<int> q;
			for (int i = 0; i < 4; i++) {
				if (nodes[root].edge[i] != 0) {
					nodes[nodes[root].edge[i]].fail = root;
					q.push(nodes[root].edge[i]);
				}
			}
			while (!q.empty()) {
				int fa = q.front();
				int fafail = nodes[fa].fail;
				q.pop();
				for (int i = 0; i < 4; i++) {
					int now = nodes[fa].edge[i];
					if (now == 0) continue;
					if (nodes[fafail].edge[i] != 0) {
						nodes[now].fail = nodes[fafail].edge[i];
					}
					else nodes[now].fail = root;
					q.push(now);
				}
			}
		}
	
	ACautomation::ACautomation() {
		memset(nodes, 0, sizeof(nodes));
		root = cnt_node = 1;
	}

	int ACautomation::query(std::vector<int>& arr) const {
		int lineType = 0;
		for (int i = 0, now = root; i < 9; i++) { 
			int ch = arr[i];
			while (nodes[now].edge[ch] == 0 && nodes[now].fail != 0) //不能匹配时能跳则跳fail
				now = nodes[now].fail; 
			if (nodes[now].edge[ch] != 0) {        //能匹配时记录答案
				now = nodes[now].edge[ch];
				lineType = std::max(lineType, nodes[now].lineType);
			}
		}
		return lineType;
	}
	
	static void Init() {
		ACautomation ac;

		assert(arrLen(lineID) == patterns.size());
		for (int i = 0; i < patterns.size(); i++) {

			auto pattern = patterns[i];
			ac.insertPattern(pattern, lineID[i]);
			
			for (int& c : pattern) 
				if (c == C_P1)
					c = C_P2;
			ac.insertPattern(pattern, lineID[i]);
		}
		ac.calc_fail();

		//枚举code
		for (int code = 0; code < NUMCODE; code++) {

			auto simulate = [](int code) {
				std::vector<int> arr(9, 0);
				for (int i = 0; i <= 3; i++)
					arr[i] = (code >> 2 * i) & 0b11;
				for (int i = 4; i <= 7; i++)
					arr[i + 1] = (code >> 2 * i) & 0b11;
				return arr;
				};
			std::vector<int> arr = simulate(code);

			arr[4] = 1;
			int typeP1 = ac.query(arr);

			arr[4] = 2;
			int typeP2 = ac.query(arr);

			codeToLineID2[code].lineP1 = typeP1;
			codeToLineID2[code].lineP2 = typeP2;
		}

		//枚举line
		for (int a = 0; a < LINE_NUM; a++) {
			for (int b = a; b < LINE_NUM; b++) {
				for (int c = b; c < LINE_NUM; c++) {
					for (int d = c; d < LINE_NUM; d++) {

						int p[4] = { a,b,c,d };

						Flower f;
			
						f.value = eval[a] + eval[b] + eval[c] + eval[d];

						int n[20] = {};
						n[a]++, n[b]++, n[c]++, n[d]++;
						//4:活三 5:冲四 6:活四 7:连五 8:禁手

						int ft = TNone;

						if (n[10] || n[11]) ft = TH3;
						if (n[10] + n[11] >= 2) ft = TDH3;
						if ((ft == TH3 || ft == TDH3) && n[12]) ft = T4H3;
						else if (n[12])ft = T4;
						if (n[13] || n[12] >= 2)ft = TH4;
						if (n[14])ft = T5;
						f.type = (FType)ft;

						do {
							lineToFlower[(p[0] << 12) + (p[1] << 8) + (p[2] << 4) + p[3]] = f;
						} while (std::next_permutation(p, p + 4));
					}
				}
			}
		}

		//枚举threatStat
		for (int threatStat = 0; threatStat < 1 << 2 * (FTYPE_NUM - 1); threatStat++) {

			bool hasT[FTYPE_NUM][2] = {};


			for (int i = 1; i < FTYPE_NUM; i++) {
				hasT[i][P1] = threatStat & (1 << 2 * i - 2);
				hasT[i][P2] = threatStat & (1 << 2 * i - 1);
			}

			int evalP1 = 0;
			int evalP2 = 0;

			if (hasT[TH3][P1])evalP1 +=  14;
			if (hasT[TDH3][P1])evalP1 += 34;
			if (hasT[T4][P1])evalP1 +=   17;
			if (hasT[T4H3][P1])evalP1 += 39;
			if (hasT[TH4][P1])evalP1 +=  49;
			if (hasT[T5][P1])evalP1 +=   59;

			if (hasT[TH3][P2])evalP2 +=  14;
			if (hasT[TDH3][P2])evalP2 += 34;
			if (hasT[T4][P2])evalP2 +=   17;
			if (hasT[T4H3][P2])evalP2 += 39;
			if (hasT[TH4][P2])evalP2 +=  49;
			if (hasT[T5][P2])evalP2 +=   59;

			threatStateToP1Eval[threatStat] = evalP1 - evalP2;
		}

	}

	const auto init = []() {
		Init();
		return true;
	}();
}


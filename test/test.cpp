#include <string>
#include <iostream>
#include <algorithm>
#include <vector>

#include "test.h"

double te[128];

void PrintTest() {
	std::cout << "node: " << te[node] / 1000.f / 0.990 << " k nodes/s\n";
	std::cout << "vcfnode: " << te[vcfnode] / 1000.f / 0.990 << " k nodes/s\n";
	std::cout << "totnode: " << (te[vcfnode] + te[node]) / 1000.f / 0.990 << " k nodes/s\n";
	te[node] = te[vcfnode] = 0;
	std::cout << "TT cutoff: " << te[TTcutoff] << "\n";
	te[TTcutoff] = 0;
	std::cout << "beta cutoff: " << te[betacutoff] << "\n";
	te[betacutoff] = 0;
	std::cout << "vcf TT cutoff: " << te[vcfTTcutoff] << "\n";
	te[vcfTTcutoff] = 0;
	std::cout << "vcf beta cutoff: " << te[vcfbetacutoff] << "\n";
	te[vcfbetacutoff] = 0;
	std::cout << "moveCnt pruning: " << te[moveCntpruning] << "\n";
	te[moveCntpruning] = 0;
	std::cout << "dispersed T: " << te[dispersedT] << "\n";
	te[dispersedT] = 0;
	std::cout << "TDH3 T4H3 win check: " << te[TDH3T4H3wincheck] << "\n";
	te[TDH3T4H3wincheck] = 0;
	std::cout << "tot hash error: " << te[tothasherror] << "\n";

	std::cout << "razor:\n";
	for (int i = 0; i < 32; i++) {
		std::cout << te[razor + i] << " ";
		te[razor + i] = 0;
	}
	std::cout << "\n";

	std::cout << "futility:\n";
	for (int i = 0; i < 32; i++) {
		std::cout << te[futility + i] << " ";
		te[futility + i] = 0;
	}
	std::cout << "\n";
}
//std::cout << "up\n";
//std::cout << " now:" << pos.first << " " << pos.second << "\n";
//for (int ii = 0; ii < 15; ii++) {
//	for (int jj = 0; jj < 15; jj++) {
//		if (board[ii][jj] == P1)
//			std::cout << "* ";
//		else if (board[ii][jj] == P2)
//			std::cout << "# ";
//		else std::cout << "- ";
//	}
//	std::cout << "\n";
//}


	/*void get1(int x, int y) {                   //debug
		std::cout << (codeLR[x] >> (2 * y) & 3);
	}
	void get2(int x, int y) {
		std::cout << (codeUD[y] >> (2 * x) & 3);
	}
	void get3(int x, int y) {
		std::cout << (codeMain[x - y + BOARD_LENGTH - 1] >> (2 * y) & 3);
	}
	void get4(int x, int y) {
		std::cout << (codeVice[x + y] >> (2 * y) & 3);
	}*/
#pragma once
#include <unordered_map>

enum Test {
	node,
	vcfnode,
	TTcutoff,
	betacutoff,
	vcfTTcutoff,
	vcfbetacutoff,
	moveCntpruning,
	dispersedT,
	TDH3T4H3wincheck,
	tothasherror,
	razor,
	futility = 60
};

extern double te[128];
void PrintTest();

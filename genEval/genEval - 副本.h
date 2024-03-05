#pragma once

#include <vector>
#include <queue>

#include "../common/common.h"

namespace Eval{
	constexpr int NUMCODE = 1 << 16;
	constexpr int NUMLINE4DIR = 1 << 16;

	struct Line2 {
		uint8_t lineP1 : 4;
		uint8_t lineP2 : 4;
	};

	struct Flower {
		uint8_t value;
		uint8_t type;
	};

	Line2 decode1(uint64_t code, int shift);
	Flower decode2(int l1, int l2, int l3, int l4);

	struct trie_node {
		int father;
		int fail;
		int edge[4];
		int lineType;
	};

	class ACautomation {      //AC自动机多串匹配
		trie_node nodes[256];
		int root;
		int cnt_node;
		
	public:
		void insertPattern(const std::vector<int>& pattern, int lineID);
		void calc_fail();
		ACautomation();
		int query(std::vector<int>& arr) const;
	};
}


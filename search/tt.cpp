#include <random>
#include <iostream>

#include "tt.h"

namespace Search {
	uint64_t Zobrist[2][BOARD_SIZE];

	static void Init1() {
		std::mt19937_64 gen;
		std::uniform_int_distribution<uint64_t> dis;

		for (int c = 0; c < 2; c++) {
			for (int i = 0; i < BOARD_SIZE; i++) {
				Zobrist[c][i] = dis(gen);
			}
		}
	}

	auto init = []() {
		Init1();
		return true;
		}();

		void HashEntry::store(uint64_t hash, bool pv, Pos best, int val, int dep, int step, HashType type) {
			uint16_t key16 = static_cast<uint16_t>(hash);

			if (_hashLow16 != key16 || dep >= _dep || type == B_Exact) {
				_PV = pv;
				if (val >= WIN_CRITICAL) val += step;
				else if (val <= -WIN_CRITICAL) val -= step;
				_value = val;
				_dep = dep;
				_gen = tt.generation();
				_type = type;
				_movePos = best;
				_hashLow16 = key16;
			}
		}

		HashTable::HashTable(int maxByteSize) {
			numClusters = maxByteSize / sizeof(Cluster);
			clusters = new Cluster[numClusters];
			_generation = 0;
		}

		HashTable::~HashTable() {
			delete[] clusters;
		}

		void HashTable::clear() {
			for (int i = 0; i < numClusters; i++) 
				clusters[i].Clear();
			_generation = 0;
		}

		bool HashTable::probe(uint64_t hashKey, HashEntry*& tte) {

			HashEntry* entry = firstEntry(hashKey);
			uint16_t key16 = static_cast<uint16_t>(hashKey);

			for (int i = 0; i < ClusterSize; i++) {
				if (entry[i].key() == key16) {
					entry[i].setGeneration(_generation);
					tte = entry;
					return true;
				}
				if (entry[i].type() == B_Initial) {
					entry[i].setGeneration(_generation);
					tte = entry;
					return false;
				}
			}

			HashEntry* replace = entry;
			for (int i = 1; i < ClusterSize; i++) {
				if (replace->depth() - (32 + _generation - replace->genaration()) >
					entry[i].depth() - (32 + _generation - entry[i].genaration()))
					replace = entry + i;
			}
			tte = replace;
			return false;
		}

		HashTable tt(TTBYTESIZE);
}


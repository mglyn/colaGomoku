#include <bits/stdc++.h>
#include <random>
#include <chrono>
using namespace std;

#define TEST2

#ifdef TEST
int cnt, cntt;
int tcut, hcut, bcut, rewrite;
double pacut[10], pbcut[10], cntex, totex;
#endif

enum Piece :char {
	Empty = 2,
	P1 = 0,
	P2 = 1,
};
Piece Opp(Piece piece) { return static_cast<Piece>(piece ^ 1); }

constexpr int winCritical = 20000;
constexpr int winMax = 30000;
constexpr int d4[4][2] = { {0,1},{1,0},{1,1},{1,-1} };
constexpr pair<int, int> nullCoord = make_pair(15, 15);
constexpr int MaxTime = 990;

unsigned short fscore[2380];
enum enum_ftype :int {
	NORMAL = 1, T4 = 2, TH4 = 3, T5 = 4
};
unsigned char ftype[2380];
unsigned short flower[16][16][16][16];
unsigned char decode[256][256];
unsigned long long pieceHash[16][16][2];

namespace Data {
	const int pattern_num = 37;
	const int type_num = 14;
	const int eval[type_num] = {
		0,
		2,4,5,			5,7,	//2
		14,17,20,		30,38,	//3
		60,					100,	//4
		100,						//5
	};
	const int idx[pattern_num] = {
		1,1,1,2,2,2,3,3,3,3,			4,4,4,5,5,5,//2
		6,6,6,6,6,7,7,8,8,8,			9,9,10,10,//3
		11,11,11,11,11,					12,			//4
		13,											//5
	};
	const std::vector<int> pattern[pattern_num] = {
	{1,0,0,0,1},
	{0,1,0,0,1},
	{1,0,0,1,0},
	{0,0,1,0,1},
	{0,1,0,1,0},
	{1,0,1,0,0},
	{0,0,0,1,1},
	{0,0,1,1,0},
	{0,1,1,0,0},
	{1,1,0,0,0},//2
	{ 0,1,0,0,1,0 },
	{ 0,0,1,0,1,0 },
	{ 0,1,0,1,0,0 },
	{ 0,1,1,0,0,0 },
	{ 0,0,1,1,0,0 },
	{ 0,0,0,1,1,0 },//2

	{1,0,1,0,1},
	{1,0,0,1,1},
	{1,1,0,0,1},
	{0,1,0,1,1},
	{1,1,0,1,0},
	{0,1,1,0,1},
	{1,0,1,1,0},
	{0,0,1,1,1},
	{0,1,1,1,0},
	{1,1,1,0,0},//3
	{ 0,1,0,1,1,0 },
	{ 0,1,1,0,1,0 },
	{ 0,0,1,1,1,0 },
	{ 0,1,1,1,0,0 },//3

	{1,1,1,1,0},
	{1,1,1,0,1},
	{1,1,0,1,1},
	{1,0,1,1,1},
	{0,1,1,1,1},//4
	{ 0,1,1,1,1,0 },//4

	{1,1,1,1,1},//5
	};
}

struct ACautomation {
	struct trie_node {
		unsigned char id = 0;
		int father = 0;
		int fail = 0;
		int edge[4] = {};
	}nodes[68];
	int root, cnt_node;
	void build_trie() {
		root = ++cnt_node;
		for (int i = 0; i < Data::pattern_num; i++) {
			int now = root;
			for (int c : Data::pattern[i]) {
				if (nodes[now].edge[c] != 0)
					now = nodes[now].edge[c];
				else {
					nodes[now].edge[c] = ++cnt_node;
					nodes[nodes[now].edge[c]].father = now;
					now = nodes[now].edge[c];
				}
			}
			nodes[now].id = Data::idx[i];
		}
	}
	void get_fail() {
		nodes[root].fail = 0;
		queue<int> q;
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
				if (now == 0)
					continue;
				if (nodes[fafail].edge[i] != 0) {
					nodes[now].fail = nodes[fafail].edge[i];
					nodes[now].id = max(nodes[nodes[now].fail].id, nodes[now].id);
				}
				else
					nodes[now].fail = root;
				q.push(now);
			}
		}
	}
	ACautomation() {
		memset(nodes, 0, sizeof(nodes));
		root = cnt_node = 0;
		build_trie();
		get_fail();
	}
	static void EvaluatorInit() {
		ACautomation* ac = new ACautomation;
		for (int code1 = 0; code1 < 256; code1++) {
			for (int code2 = 0; code2 < 256; code2++) {
				int d[9] = {};
				int now = ac->root;
				for (int i = 0; i < 4; i++) {
					int k1 = code1 & (1 << i);
					int k2 = code2 & (1 << i);
					d[i] = k1 ? (k2 ? 2 : 1) : (k2 ? 2 : 0);
				}
				d[4] = 1;
				for (int i = 4; i < 8; i++) {
					int k1 = code1 & (1 << i);
					int k2 = code2 & (1 << i);
					d[i + 1] = k1 ? (k2 ? 2 : 1) : (k2 ? 2 : 0);
				}
				for (int i = 0; i < 9; i++) {
					int ch = d[i];
					while (ac->nodes[now].edge[ch] == 0 && ac->nodes[now].fail != 0) now = ac->nodes[now].fail;
					if (ac->nodes[now].edge[ch] != 0) now = ac->nodes[now].edge[ch];
					else continue;
					if (decode[code1][code2] < ac->nodes[now].id) {
						decode[code1][code2] = ac->nodes[now].id;
					}
				}
			}
		}
		delete ac;
		int idx = 0;
		for (int a = 0; a < Data::type_num; a++) {
			for (int b = a; b < Data::type_num; b++) {
				for (int c = b; c < Data::type_num; c++) {
					for (int d = c; d < Data::type_num; d++, idx++) {
						int p[4] = { a,b,c,d };
						do {
							flower[p[0]][p[1]][p[2]][p[3]] = idx;
						} while (next_permutation(p, p + 4));

						int score = Data::eval[a] + Data::eval[b] + Data::eval[c] + Data::eval[d];
						fscore[idx] = int(pow(score, 1.15) * 0.75);

						int n[20] = {};
						n[a]++, n[b]++, n[c]++, n[d]++;
						//4:活三 5:冲四 6:活四 7:连五 8:禁手

						int ft = NORMAL;
						if (n[11])ft = T4;
						if (n[12] || n[11] >= 2)ft = TH4;
						if (n[13])ft = T5;
						ftype[idx] = ft;
					}
				}
			}
		}
	}
};

enum HashType :int {
	Initial = 0, PV, Alpha, Beta
};

#pragma pack(push, 2)
struct HashEntry {
	unsigned int _hashKeyHigh32;
	short _score;
	unsigned char _dep;
	unsigned char _genType;
	unsigned char _best;
	HashEntry() {
		_hashKeyHigh32 = _score = _dep = _genType = _best = 0;
	}
	inline HashType Type() { return static_cast<HashType>(_genType & 3); }
	inline unsigned char Gen() { return _genType & 0xFC; }
	inline pair<int, int> Best() { return make_pair(static_cast<int>(_best >> 4), static_cast<int>(_best & 15)); }
	inline void SaveGen(unsigned char gen) { _genType = gen | Type(); }
	inline int Score(int step) {
		if (_score >= winCritical) {
			return _score - step;
		}
		else if (_score <= -winCritical) {
			return _score + step;
		}
		return _score;
	}
	inline bool IsValid(int alpha, int beta, int dep, int step) {
		bool mate = false;
		int val = _score;
		if (val >= winCritical) {
			val -= step;
			mate = true;
		}
		else if (val <= -winCritical) {
			val += step;
			mate = true;
		}
		if (_dep >= dep || mate) {
			HashType type = Type();
			return	(type == PV) || (type == Alpha && val <= alpha) || (type == Beta && val >= beta);
		}
		return false;
	}
	inline void Store(unsigned long long hashKey, pair<int, int> best, int score, int dep, int step, unsigned char gen, HashType type) {
		unsigned int key32 = static_cast<unsigned int>(hashKey >> 32);
		if (_hashKeyHigh32 == key32 && dep < _dep)return;
#ifdef TEST
		if (_hashKeyHigh32 != key32 && _hashKeyHigh32 != 0)rewrite++;
#endif
		if (score >= winCritical) score += step;
		else if (score <= -winCritical) score -= step;
		_score = static_cast<short>(score);
		_dep = static_cast<unsigned char>(dep);
		_genType = gen | type;
		_best = static_cast<unsigned char>(best.first << 4);
		_best |= static_cast<unsigned char>(best.second);
		_hashKeyHigh32 = key32;
	}
	inline void Clear() {
		_score = _dep = _genType = _best = 0;
	}
};
#pragma pack(pop)

struct HashTable {
	static constexpr int HashTableSize = 1 << 23;
	static constexpr int Mask = HashTableSize - 1;
	static constexpr int ClusterSize = 3;
	struct Cluster {
		HashEntry hashEntry[ClusterSize];
		inline HashEntry* Entry() { return hashEntry; }
		inline void Clear() { for (int i = 0; i < ClusterSize; i++) hashEntry[i].Clear(); }
	};
	Cluster* hashTable;
	unsigned char generation;
	HashTable() {
		hashTable = new Cluster[HashTableSize];
		generation = 0;
		random_device rd;
		mt19937_64 gen(rd());
		uniform_int_distribution<unsigned long long> dis;
		for (int i = 0; i < 15; i++) {
			for (int j = 0; j < 15; j++) {
				for (int c = 0; c < 2; c++) {
					pieceHash[i][j][c] = dis(gen);
				}
			}
		}
	}
	~HashTable() {
		delete[] hashTable;
	}
	void Clear() {
		for (int i = 0; i < HashTableSize; i++) {
			hashTable[i].Clear();
		}
		generation = 0;
	}
	void NewGen() { generation += 4; }
	bool Probe(unsigned long long hashKey, HashEntry*& tte) {
		unsigned int key32 = static_cast<unsigned int>(hashKey >> 32);
		HashEntry* entry = hashTable[hashKey & Mask].Entry();
		for (int i = 0; i < ClusterSize; i++, entry++) {
			if (entry->_hashKeyHigh32 == key32) {
				if (entry->Gen() != generation) entry->SaveGen(generation);
				tte = entry;
				return true;
			}
			else if (entry->Type() == Initial) {
				tte = entry;
			}
			else if (!tte || (tte->Type() != Initial
				&& tte->_dep - 2 * (259 + generation - tte->Gen()) > entry->_dep - 2 * (259 + generation - entry->Gen()))) {
				tte = entry;
			}
		}
		return false;
	}
} tt;

struct Info {
	struct Area {
		int x1, y1, x2, y2;
		inline void Update(int x, int y) {
			x1 = min(x1, x - 2);
			y1 = min(y1, y - 2);
			x2 = max(x2, x + 2);
			y2 = max(y2, y + 2);
			x1 = max(0, x1);
			y1 = max(0, y1);
			x2 = min(x2, 14);
			y2 = min(y2, 14);
		}
		inline int area() {
			return (x2 - x1) * (y2 - y1);
		}
	} range;
	Piece self, oppo;
	int cntT[5][2];
	int evaluation[2];
	pair<int, int> T4cost;
	unsigned long long hashKey;
	Info() { Reset(); }
	void Reset() {
		self = P1;
		oppo = P2;
		hashKey = 1ULL << 63;
		range.x1 = range.y1 = 15;
		range.x2 = range.y2 = -1;
		for (int c = 0; c < 2; c++) {
			for (int i = 0; i < 5; i++) cntT[i][c] = 0;
			cntT[NORMAL][c] = 225;
			evaluation[c] = 0;
		}
		T4cost = nullCoord;
	}
};

struct Stack {
	static constexpr int StackSize = 256;
	pair<int, int> pos[StackSize];
	Info inf[StackSize];
	int cnts;
	Stack() {
		Reset();
	}
	void Reset() {
		cnts = 0;
	}
	inline bool NearToLast(int k, pair<int, int> coord) {
		int i = max(0, cnts - k);
		int distance = max(abs(pos[i].first - coord.first), abs(pos[i].second - coord.second));
		if (pos[i].first == coord.first ||
			pos[i].second == coord.second ||
			abs(pos[i].first - coord.first) == abs(pos[i].second - coord.second)) {
			return distance <= 4;
		}
		else return distance <= 2;
	}
	inline int LastEval() {
		int i = max(0, cnts - 1);
		return inf[i].evaluation[inf[i].self] - inf[i].evaluation[inf[i].oppo];
	}
};

struct Board {
	Piece board[16][16];
	Info nf;
	Stack ss;
	int cntPiece;
	struct eval {
		unsigned char code[4][2];
		unsigned char type[4][2];
		unsigned char ftype[2];
		unsigned short fscore[2];
	}unit[16][16];
	Board() { Reset(); }
	void Reset() {
		ss.Reset();
		nf.Reset();
		cntPiece = 0;
		for (int x = 0; x < 15; x++) {//初始化code和棋盘
			for (int y = 0; y < 15; y++) {
				board[x][y] = Empty;
				unit[x][y].ftype[P1] = unit[x][y].ftype[P2] = NORMAL;
				unit[x][y].fscore[P1] = unit[x][y].fscore[P2] = 0;
				for (int i = 0; i < 4; i++) {
					unit[x][y].code[i][P1] = unit[x][y].code[i][P2] = 0;
					unit[x][y].type[i][P1] = unit[x][y].type[i][P2] = 0;
				}
			}
		}
		for (int x = 0; x < 15; x++) {//初始化code
			for (int y = 0; y < 15; y++) {
				for (int i = 0; i < 4; i++) {
					for (int k = 1 << 4, nx = x - d4[i][0], ny = y - d4[i][1]; k <= (1 << 7); nx -= d4[i][0], ny -= d4[i][1], k <<= 1) {
						if (Notin(nx, ny))unit[x][y].code[i][P1] |= k, unit[x][y].code[i][P2] |= k;
					}
					for (int k = 1 << 3, nx = x + d4[i][0], ny = y + d4[i][1]; k >= 1; nx += d4[i][0], ny += d4[i][1], k >>= 1) {
						if (Notin(nx, ny))unit[x][y].code[i][P1] |= k, unit[x][y].code[i][P2] |= k;
					}
				}
			}
		}
	}
	inline bool Notin(int x, int y) { return x >= 15 || y >= 15 || x < 0 || y < 0; }
	void Update(pair<int, int> cd) {
		assert(ss.cnts < 255);
		ss.pos[ss.cnts] = cd;
		ss.inf[ss.cnts] = nf;
		ss.cnts++;

		int x = cd.first, y = cd.second;
		board[x][y] = nf.self;
		nf.hashKey ^= pieceHash[x][y][nf.self];

		nf.evaluation[P1] -= unit[x][y].fscore[P1];
		nf.evaluation[P2] -= unit[x][y].fscore[P2];
		nf.cntT[unit[x][y].ftype[P1]][P1]--;
		nf.cntT[unit[x][y].ftype[P2]][P2]--;

		for (int i = 0; i < 4; i++) {
			for (int k = 1 << 3, nx = x - d4[i][0], ny = y - d4[i][1]; k >= 1; nx -= d4[i][0], ny -= d4[i][1], k >>= 1) {
				if (Notin(nx, ny))break;
				if (board[nx][ny] == Empty) {
					nf.evaluation[P1] -= unit[nx][ny].fscore[P1];
					nf.evaluation[P2] -= unit[nx][ny].fscore[P2];
					nf.cntT[unit[nx][ny].ftype[P1]][P1]--;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]--;

					unit[nx][ny].code[i][nf.self] |= k;
					unit[nx][ny].type[i][P1] = decode[unit[nx][ny].code[i][P1]][unit[nx][ny].code[i][P2]];
					unit[nx][ny].type[i][P2] = decode[unit[nx][ny].code[i][P2]][unit[nx][ny].code[i][P1]];
					int flower1 = flower[unit[nx][ny].type[0][P1]][unit[nx][ny].type[1][P1]][unit[nx][ny].type[2][P1]][unit[nx][ny].type[3][P1]];
					int flower2 = flower[unit[nx][ny].type[0][P2]][unit[nx][ny].type[1][P2]][unit[nx][ny].type[2][P2]][unit[nx][ny].type[3][P2]];
					unit[nx][ny].ftype[P1] = ftype[flower1];
					unit[nx][ny].ftype[P2] = ftype[flower2];
					unit[nx][ny].fscore[P1] = fscore[flower1];
					unit[nx][ny].fscore[P2] = fscore[flower2];

					nf.evaluation[P1] += unit[nx][ny].fscore[P1];
					nf.evaluation[P2] += unit[nx][ny].fscore[P2];
					nf.cntT[unit[nx][ny].ftype[P1]][P1]++;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]++;

					if (unit[nx][ny].ftype[P1] == T5 || unit[nx][ny].ftype[P2] == T5) nf.T4cost = make_pair(nx, ny);
				}
			}
			for (int k = 1 << 4, nx = x + d4[i][0], ny = y + d4[i][1]; k <= (1 << 7); nx += d4[i][0], ny += d4[i][1], k <<= 1) {
				if (Notin(nx, ny))break;
				if (board[nx][ny] == Empty) {
					nf.evaluation[P1] -= unit[nx][ny].fscore[P1];
					nf.evaluation[P2] -= unit[nx][ny].fscore[P2];
					nf.cntT[unit[nx][ny].ftype[P1]][P1]--;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]--;

					unit[nx][ny].code[i][nf.self] |= k;
					unit[nx][ny].type[i][P1] = decode[unit[nx][ny].code[i][P1]][unit[nx][ny].code[i][P2]];
					unit[nx][ny].type[i][P2] = decode[unit[nx][ny].code[i][P2]][unit[nx][ny].code[i][P1]];
					int flower1 = flower[unit[nx][ny].type[0][P1]][unit[nx][ny].type[1][P1]][unit[nx][ny].type[2][P1]][unit[nx][ny].type[3][P1]];
					int flower2 = flower[unit[nx][ny].type[0][P2]][unit[nx][ny].type[1][P2]][unit[nx][ny].type[2][P2]][unit[nx][ny].type[3][P2]];
					unit[nx][ny].ftype[P1] = ftype[flower1];
					unit[nx][ny].ftype[P2] = ftype[flower2];
					unit[nx][ny].fscore[P1] = fscore[flower1];
					unit[nx][ny].fscore[P2] = fscore[flower2];

					nf.evaluation[P1] += unit[nx][ny].fscore[P1];
					nf.evaluation[P2] += unit[nx][ny].fscore[P2];
					nf.cntT[unit[nx][ny].ftype[P1]][P1]++;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]++;

					if (unit[nx][ny].ftype[P1] == T5 || unit[nx][ny].ftype[P2] == T5) nf.T4cost = make_pair(nx, ny);
				}
			}
		}
		cntPiece++;
		nf.range.Update(x, y);
		swap(nf.self, nf.oppo);
	}
	void UpdateVC(pair<int, int> cd) {
		assert(ss.cnts < 255);
		ss.pos[ss.cnts] = cd;
		ss.inf[ss.cnts] = nf;
		ss.cnts++;

		int x = cd.first, y = cd.second;
		board[x][y] = nf.self;
		nf.hashKey ^= pieceHash[x][y][nf.self];

		nf.cntT[unit[x][y].ftype[P1]][P1]--;
		nf.cntT[unit[x][y].ftype[P2]][P2]--;

		for (int i = 0; i < 4; i++) {
			for (int k = 1 << 3, nx = x - d4[i][0], ny = y - d4[i][1]; k >= 1; nx -= d4[i][0], ny -= d4[i][1], k >>= 1) {
				if (Notin(nx, ny))break;
				if (board[nx][ny] == Empty) {
					nf.cntT[unit[nx][ny].ftype[P1]][P1]--;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]--;

					unit[nx][ny].code[i][nf.self] |= k;
					unit[nx][ny].type[i][P1] = decode[unit[nx][ny].code[i][P1]][unit[nx][ny].code[i][P2]];
					unit[nx][ny].type[i][P2] = decode[unit[nx][ny].code[i][P2]][unit[nx][ny].code[i][P1]];
					int flower1 = flower[unit[nx][ny].type[0][P1]][unit[nx][ny].type[1][P1]][unit[nx][ny].type[2][P1]][unit[nx][ny].type[3][P1]];
					int flower2 = flower[unit[nx][ny].type[0][P2]][unit[nx][ny].type[1][P2]][unit[nx][ny].type[2][P2]][unit[nx][ny].type[3][P2]];
					unit[nx][ny].ftype[P1] = ftype[flower1];
					unit[nx][ny].ftype[P2] = ftype[flower2];

					nf.cntT[unit[nx][ny].ftype[P1]][P1]++;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]++;

					if (unit[nx][ny].ftype[P1] == T5 || unit[nx][ny].ftype[P2] == T5) nf.T4cost = make_pair(nx, ny);
				}
			}
			for (int k = 1 << 4, nx = x + d4[i][0], ny = y + d4[i][1]; k <= (1 << 7); nx += d4[i][0], ny += d4[i][1], k <<= 1) {
				if (Notin(nx, ny))break;
				if (board[nx][ny] == Empty) {
					nf.cntT[unit[nx][ny].ftype[P1]][P1]--;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]--;

					unit[nx][ny].code[i][nf.self] |= k;
					unit[nx][ny].type[i][P1] = decode[unit[nx][ny].code[i][P1]][unit[nx][ny].code[i][P2]];
					unit[nx][ny].type[i][P2] = decode[unit[nx][ny].code[i][P2]][unit[nx][ny].code[i][P1]];
					int flower1 = flower[unit[nx][ny].type[0][P1]][unit[nx][ny].type[1][P1]][unit[nx][ny].type[2][P1]][unit[nx][ny].type[3][P1]];
					int flower2 = flower[unit[nx][ny].type[0][P2]][unit[nx][ny].type[1][P2]][unit[nx][ny].type[2][P2]][unit[nx][ny].type[3][P2]];
					unit[nx][ny].ftype[P1] = ftype[flower1];
					unit[nx][ny].ftype[P2] = ftype[flower2];

					nf.cntT[unit[nx][ny].ftype[P1]][P1]++;
					nf.cntT[unit[nx][ny].ftype[P2]][P2]++;

					if (unit[nx][ny].ftype[P1] == T5 || unit[nx][ny].ftype[P2] == T5) nf.T4cost = make_pair(nx, ny);
				}
			}
		}
		cntPiece++;
		swap(nf.self, nf.oppo);
	}
	void Undo() {
		assert(ss.cnts >= 1);
		int x = ss.pos[ss.cnts - 1].first, y = ss.pos[ss.cnts - 1].second;
		nf = ss.inf[ss.cnts - 1];
		ss.cnts--;
		board[x][y] = Empty;
		for (int i = 0; i < 4; i++) {
			for (int k = 1 << 3, nx = x - d4[i][0], ny = y - d4[i][1]; k >= 1; nx -= d4[i][0], ny -= d4[i][1], k >>= 1) {
				if (Notin(nx, ny))break;
				if (board[nx][ny] == Empty) {
					unit[nx][ny].code[i][nf.self] ^= k;
					unit[nx][ny].type[i][P1] = decode[unit[nx][ny].code[i][P1]][unit[nx][ny].code[i][P2]];
					unit[nx][ny].type[i][P2] = decode[unit[nx][ny].code[i][P2]][unit[nx][ny].code[i][P1]];
					int flower1 = flower[unit[nx][ny].type[0][P1]][unit[nx][ny].type[1][P1]][unit[nx][ny].type[2][P1]][unit[nx][ny].type[3][P1]];
					int flower2 = flower[unit[nx][ny].type[0][P2]][unit[nx][ny].type[1][P2]][unit[nx][ny].type[2][P2]][unit[nx][ny].type[3][P2]];
					unit[nx][ny].ftype[P1] = ftype[flower1];
					unit[nx][ny].ftype[P2] = ftype[flower2];
					unit[nx][ny].fscore[P1] = fscore[flower1];
					unit[nx][ny].fscore[P2] = fscore[flower2];
				}
			}
			for (int k = 1 << 4, nx = x + d4[i][0], ny = y + d4[i][1]; k <= (1 << 7); nx += d4[i][0], ny += d4[i][1], k <<= 1) {
				if (Notin(nx, ny))break;
				if (board[nx][ny] == Empty) {
					unit[nx][ny].code[i][nf.self] ^= k;
					unit[nx][ny].type[i][P1] = decode[unit[nx][ny].code[i][P1]][unit[nx][ny].code[i][P2]];
					unit[nx][ny].type[i][P2] = decode[unit[nx][ny].code[i][P2]][unit[nx][ny].code[i][P1]];
					int flower1 = flower[unit[nx][ny].type[0][P1]][unit[nx][ny].type[1][P1]][unit[nx][ny].type[2][P1]][unit[nx][ny].type[3][P1]];
					int flower2 = flower[unit[nx][ny].type[0][P2]][unit[nx][ny].type[1][P2]][unit[nx][ny].type[2][P2]][unit[nx][ny].type[3][P2]];
					unit[nx][ny].ftype[P1] = ftype[flower1];
					unit[nx][ny].ftype[P2] = ftype[flower2];
					unit[nx][ny].fscore[P1] = fscore[flower1];
					unit[nx][ny].fscore[P2] = fscore[flower2];
				}
			}
		}
		cntPiece--;
	}
	inline int StaticEval() { return (nf.evaluation[nf.self] - nf.evaluation[nf.oppo] - ss.LastEval()) / 2; }
};

struct MoveSeq {
	const int MaxMovePredict = 128;
	Board& bd;
	enum MoveState :int {
		HashMove,
		Generate,
		Normal
	} state;
	vector<pair<pair<int, int>, int>> moves;
	pair<int, int> hashCoord;
	int iter;
	MoveSeq(Board& bd, pair<int, int> hashCoord) : bd(bd), hashCoord(hashCoord) {
		moves.reserve(MaxMovePredict);
		iter = 0;
		if (hashCoord != nullCoord)state = HashMove;
		else state = Generate;
	}
	void GenMove() {
		if (bd.nf.cntT[TH4][bd.nf.oppo]) {  //对方有双4点
			for (int i = bd.nf.range.x1; i <= bd.nf.range.x2; i++) {
				for (int j = bd.nf.range.y1; j <= bd.nf.range.y2; j++) {
					if (bd.board[i][j] == Empty)
						if (bd.unit[i][j].ftype[bd.nf.self] >= T4 || bd.unit[i][j].ftype[bd.nf.oppo] >= T4)//自己冲4或防双4点,连5点
							moves.push_back(make_pair(make_pair(i, j), 2 * bd.unit[i][j].fscore[bd.nf.self] + bd.unit[i][j].fscore[bd.nf.oppo]));
				}
			}
		}
		else {
			for (int i = bd.nf.range.x1; i <= bd.nf.range.x2; i++) {
				for (int j = bd.nf.range.y1; j <= bd.nf.range.y2; j++) {
					if (bd.board[i][j] == Empty)
						moves.push_back(make_pair(make_pair(i, j), 2 * bd.unit[i][j].fscore[bd.nf.self] + bd.unit[i][j].fscore[bd.nf.oppo]));
				}
			}
		}
	}
	bool NextMove(pair<int, int>& move) {
		if (state == HashMove) {
			state = Generate;
			move = hashCoord;
			return true;
		}
		else if (state == Generate) {
			state = Normal;
			GenMove();
			return NextMove(move);
		}
		if (iter < moves.size()) {
			int maxScore = moves[iter].second;
			int maxPos = iter;
			for (int i = iter + 1; i < moves.size(); i++) {
				if (moves[i].second > maxScore) {
					maxScore = moves[i].second;
					maxPos = i;
				}
			}
			swap(moves[maxPos], moves[iter]);
			move = moves[iter++].first;
			if (move == hashCoord) return NextMove(move);
			else return true;
		}
		else return false;
	}
};

struct Clock {
	const int Cycle = 64;
	int cntCheckClock;
	bool timeOut;
	chrono::time_point<chrono::steady_clock> start;
public:
	Clock() {
		cntCheckClock = 0;
		timeOut = false;
		start = chrono::steady_clock::now();
	}
	int time_out() {
		if (timeOut) return true;
		cntCheckClock++;
		if (cntCheckClock == Cycle) {
			cntCheckClock = 0;
			using namespace chrono;
			return timeOut = duration_cast<milliseconds>(steady_clock::now() - start).count() > MaxTime;
		}
		return false;
	}
};

struct Search {
	static constexpr int MaxExpand = 36;
	static constexpr int MaxDep = 63;
	static constexpr int IIDMinDep = 7;
	static constexpr int HPDep = 4;
	static constexpr int HAPCritical[6] = { 0,100,160,250,370,520 };
	static constexpr int HBPCritical[6] = { 0,140,200,290,410,560 };

	Board bd;
	Clock timer;

	Search() {}

	Search(const vector<pair<int, int>>& moveSeq) {
		for (pair<int, int> move : moveSeq) {
			bd.Update(move);
		}
	}

	pair<int, int>GenMovePattern(Piece p, enum_ftype pat) {
		for (int i = bd.nf.range.x1; i <= bd.nf.range.x2; i++) {
			for (int j = bd.nf.range.y1; j <= bd.nf.range.y2; j++) {
				if (bd.board[i][j] != Empty)continue;
				if (bd.unit[i][j].ftype[p] >= pat)return make_pair(i, j);
			}
		}
		assert(0);
		return nullCoord;
	}

	int Root(int alpha, int beta, float dep, int step, pair<int, int>& decision) {
#ifdef TEST
		cnt++;
#endif
		HashEntry* hashEntry = nullptr;
		pair<int, int> hashMove = nullCoord;
		if (tt.Probe(bd.nf.hashKey, hashEntry)) hashMove = hashEntry->Best();
		MoveSeq moves(bd, hashMove);
		int val = 0;
		int bestVal = -winMax;
		pair<int, int> bestMove = nullCoord;
		int expand = 0;

		float r = bd.nf.cntT[TH4][bd.nf.oppo] ?
			logf(max(bd.nf.cntT[T4][bd.nf.oppo] + bd.nf.cntT[TH4][bd.nf.oppo] + bd.nf.cntT[T4][bd.nf.self], 1)) / 1.3 :
			logf(max(bd.nf.range.area() - bd.cntPiece, 1)) / 1.3f;

		for (pair<int, int> move; moves.NextMove(move); expand++) {
			if (bd.unit[move.first][move.second].ftype[bd.nf.self] == NORMAL) {
				if (bestVal > -winCritical) {
					if (expand > MaxExpand) break;
				}
			}
			bd.Update(move);
			val = -NegaMax(-beta, -alpha, dep - r, step + 1, true);
			bd.Undo();

			if (timer.time_out()) {
				decision = bestMove;
				return bestVal;
			}

			if (val > bestVal) {
				bestVal = val;
				bestMove = move;
				if (val > alpha) {
					alpha = val;
				}
			}
		}

		hashEntry->Store(bd.nf.hashKey, bestMove, bestVal, dep, step, tt.generation, PV);
#ifdef TEST
		cout << alpha << "\n";
#endif
		decision = bestMove;
		return bestVal;
	}

	int NegaMax(int alpha, int beta, float dep, int step, bool PVsearch) {
#ifdef TEST
		cnt++;
#endif
		alpha = max(-winMax + step, alpha);    //杀棋裁剪
		beta = min(winMax - step - 1, beta);
		if (alpha >= beta) return alpha;

		if (bd.cntPiece == 225)return 0;

		if (bd.nf.cntT[T5][bd.nf.self]) return winMax - step;
		if (bd.nf.cntT[T5][bd.nf.oppo]) {
			bd.Update(bd.nf.T4cost);
			int val = -NegaMax(-beta, -alpha, dep, step + 1, PVsearch);
			bd.Undo();
			return val;
		}
		if (bd.nf.cntT[TH4][bd.nf.self])return winMax - step - 2;

		int Eval = bd.StaticEval();

		if (dep <= 0 && Eval >= beta)return Eval;

		HashEntry* hashEntry = nullptr;
		bool hashHit = tt.Probe(bd.nf.hashKey, hashEntry);

		if (hashHit && hashEntry->IsValid(alpha, beta, PVsearch ? ceilf(dep) : roundf(dep), step)) {		//置换表命中且符合截断条件
#ifdef TEST
			hcut++;
#endif
			return hashEntry->Score(step);
		}

		if (dep <= 0) {
			if (!hashHit) {
				int q = QsearchRoot(step);
				if (q) {
#ifdef TEST
					tcut++;
#endif
					hashEntry->Store(bd.nf.hashKey, nullCoord, q, 0, step, tt.generation, PV);
					return q;
				}
				else hashEntry->Store(bd.nf.hashKey, nullCoord, Eval, 0, step, tt.generation, PV);
			}
			return Eval;
		}

		pair<int, int> hashMove = nullCoord;
		if (hashHit) {			//置换表启发
#ifdef TEST
			hcut++;
#endif
			hashMove = hashEntry->Best();
			Eval = hashEntry->Score(step);
		}

		if (dep <= HPDep) {
			if (Eval - HBPCritical[(int)floorf(dep)] >= beta) {
#ifdef TEST
				pbcut[(int)floorf(dep)]++;
#endif
				return Eval;
			}
			if (!PVsearch && Eval + HAPCritical[(int)floorf(dep)] <= alpha) {
#ifdef TEST
				pacut[(int)floorf(dep)]++;
#endif
				return Eval;
			}
		}

		if (!hashHit) {                                  //置换表不命中时             
			if (dep >= IIDMinDep && PVsearch) {              //内部迭代加深找PV
				HashEntry* hashEntryIID = nullptr;
				NegaMax(alpha, beta, dep * 2 / 3, step, false);
				bool hashHitIID = tt.Probe(bd.nf.hashKey, hashEntryIID);
				if (hashHitIID) hashMove = hashEntryIID->Best();
			}
		}

		MoveSeq moves(bd, hashMove);
		int val = 0;
		int bestVal = -winMax;
		pair<int, int> bestMove = nullCoord;
		HashType hashType = Alpha;
		int expand = 0;
		int Expand = max(12, MaxExpand - 2 * step);

		float r = bd.nf.cntT[TH4][bd.nf.oppo] ?
			logf(max(bd.nf.cntT[T4][bd.nf.oppo] + bd.nf.cntT[TH4][bd.nf.oppo] + bd.nf.cntT[T4][bd.nf.self], 1)) / 1.3 :
			logf(max(bd.nf.range.area() - bd.cntPiece, 1)) / 1.3;

#ifdef TEST
		cntex++;
#endif
		for (pair<int, int> move; moves.NextMove(move); expand++) {
#ifdef TEST
			totex++;
#endif
			if (bd.unit[move.first][move.second].ftype[bd.nf.self] == NORMAL) {
				if (bestVal > -winCritical) {
					if (expand > Expand) break;
					if (!PVsearch) {
						if (dep <= 4) {
							if (!bd.ss.NearToLast(1, move)) {
								if (!bd.ss.NearToLast(2, move)) {
									if (!bd.ss.NearToLast(3, move)) {
										if (expand > 5 + dep) continue;
									}
									else if (expand > 9 + dep) continue;
								}
								else if (expand > 13 + dep) continue;
							}
						}
					}
				}
				else {
					if (expand > MaxExpand && !bd.ss.NearToLast(1, move) && !bd.ss.NearToLast(3, move))continue;
				}
			}

			bd.Update(move);

			if (!PVsearch || expand > 0) {
				val = -NegaMax(-alpha - 1, -alpha, dep - r, step + 1, false);
			}

			if (PVsearch) {
				if (expand == 0 || (val > alpha && val < beta)) {
					val = -NegaMax(-beta, -alpha, dep - r, step + 1, true);
				}
			}

			bd.Undo();

			if (timer.time_out()) return 0; //超时作废

			if (val > bestVal) {
				bestVal = val;
				bestMove = move;
				if (val >= beta) {
					hashType = Beta;
#ifdef TEST
					bcut++;
#endif
					break;
				}
				if (val > alpha) {
					hashType = PV;
					alpha = val;
				}
			}
		}

		hashEntry->Store(bd.nf.hashKey, bestMove, bestVal, dep, step, tt.generation, hashType);

		return bestVal;
	}

	inline int TryExpandQsearch(int nx, int ny, int step) {
		if (bd.board[nx][ny] == Empty && bd.unit[nx][ny].ftype[bd.nf.self] >= T4) {
			bd.UpdateVC(make_pair(nx, ny));
			bd.UpdateVC(bd.nf.T4cost);
			int qw = Qsearch(nx, ny, step + 2);
			bd.Undo();
			bd.Undo();
			return qw;
		}
		return 0;
	}

	int Qsearch(int x, int y, int step) {
#ifdef TEST
		cntt++;
#endif
		if (bd.nf.cntT[T5][bd.nf.self]) return winMax - step;
		if (bd.nf.cntT[T5][bd.nf.oppo]) {
			if (bd.nf.cntT[T5][bd.nf.oppo] >= 2)return 0;
			return TryExpandQsearch(bd.nf.T4cost.first, bd.nf.T4cost.second, step);
		}
		if (bd.nf.cntT[TH4][bd.nf.self])return winMax - step - 2;
		for (int i = 0; i < 4; i++) {
			for (int k = 0, nx = x - d4[i][0], ny = y - d4[i][1]; k < 4; nx -= d4[i][0], ny -= d4[i][1], k++) {
				if (bd.Notin(nx, ny) || bd.board[nx][ny] == bd.nf.oppo)break;
				int q = TryExpandQsearch(nx, ny, step);
				if (q)return q;
			}
			for (int k = 0, nx = x + d4[i][0], ny = y + d4[i][1]; k < 4; nx += d4[i][0], ny += d4[i][1], k++) {
				if (bd.Notin(nx, ny) || bd.board[nx][ny] == bd.nf.oppo)break;
				int q = TryExpandQsearch(nx, ny, step);
				if (q)return q;
			}
		}
		return 0;
	}

	int QsearchRoot(int step) {
		if (!bd.nf.cntT[T4][bd.nf.self])return 0;
		for (int nx = bd.nf.range.x1; nx <= bd.nf.range.x2; nx++) {
			for (int ny = bd.nf.range.y1; ny <= bd.nf.range.y2; ny++) {
				int q = TryExpandQsearch(nx, ny, step);
				if (q)return q;
			}
		}
		return 0;
	}

	pair<int, int> bot_decision() {
		if (bd.cntPiece == 0) {
			return make_pair(7, 7);
		}
		if (bd.nf.cntT[T5][bd.nf.self]) {
			return GenMovePattern(bd.nf.self, T5);
		}
		if (bd.cntPiece == 225) {
			return nullCoord;
		}
		tt.NewGen();
		pair<int, int> ans = nullCoord;
		int bestVal = -winMax;
		for (int dep = 3; dep <= MaxDep; dep++) {
#ifdef TEST
			cout << "nowmaxdep:" << dep << "\n";
#endif
			pair<int, int> decision = nullCoord;
			int val = Root(-winMax, winMax, dep, 0, decision);
			if (timer.time_out() || abs(val) > winCritical) {
				if (val > bestVal) {
					ans = decision;
					bestVal = val;
				}
				break;
			}
			else {
				ans = decision;
				bestVal = val;
			}
		}
		return ans;
	}
};

#ifdef TEST
#include<graphics.h>
#include<ctime>
#include <conio.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
class button {
	wchar_t text[30] = {};
public:
	void set_text(wchar_t s[], int size) {
		memcpy(text, s, size);
	}
	bool isin = 0, isclick = 0;
	int mx = 0, my = 0, mw = 0, mh = 0, mtype = 0;
	void set(int x, int y, int w, int h, int type) {
		mx = x, my = y, mw = w, mh = h, mtype = type;
	}
	bool update(ExMessage& msg) {
		bool up = 0;
		isin = msg.x >= mx && msg.x <= mx + mw && msg.y >= my && msg.y <= my + mh;
		if (isin && msg.message == WM_LBUTTONDOWN) isclick = 1;
		if (!isin) isclick = 0;
		int oldone = isclick;
		if (isin && msg.message == WM_LBUTTONUP) {
			isclick = 0;
			up = oldone && !isclick;
		}
		return up;
	}
	bool show(ExMessage& msg) {
		bool play = !isclick;
		bool up = update(msg);
		setlinecolor(BLACK);
		setlinestyle(PS_SOLID, 1);
		if (mtype == 1) {
			Gdiplus::Graphics Graphics(GetImageHDC());
			Gdiplus::Pen Pen_1(Gdiplus::Color(160, 180, 210), 2.f);
			Gdiplus::Pen Pen_2(Gdiplus::Color(205, 225, 235), 2.f);// 设置绘图质量为高质量
			Graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeHighQuality);
			if (isin && !isclick) {
				Graphics.DrawEllipse(&Pen_1, Gdiplus::Rect{ mx - 1, my - 1, mw + 2,mw + 2 });
			}
			else if (isclick) {
				Graphics.DrawEllipse(&Pen_2, Gdiplus::Rect{ mx - 1, my - 1, mw + 2,mw + 2 });
			}
		}
		else if (mtype == 2) {
			setlinestyle(PS_NULL);
			if (isin && !isclick) {
				setfillcolor(RGB(255, 254, 215));
				fillroundrect(mx, my, mx + mw + 1, my + mh + 1, 10, 10);
			}
			else if (isclick) {
				setfillcolor(RGB(255, 255, 235));
				fillroundrect(mx, my, mx + mw + 1, my + mh + 1, 10, 10);
			}
		}
		else {
			setlinestyle(PS_NULL);
			if (isin && !isclick) {
				setfillcolor(RGB(148, 110, 20));
				fillroundrect(mx, my, mx + mw + 1, my + mh + 1, 10, 10);
				setfillcolor(RGB(200, 180, 120));
				fillroundrect(mx + 1, my + 1, mx + mw - 1, my + mh - 1, 10, 10);
			}
			else if (isclick) {
				int shift = 3;
				setfillcolor(RGB(148, 110, 20));
				fillroundrect(mx + 2 * shift, my + shift, (mx + mw) - 2 * shift, (my + mh) - shift, 10, 10);
				setfillcolor(RGB(170, 150, 100));
				fillroundrect(mx + 2 * shift + 1, my + shift + 1, mx + mw - 2 * shift - 1, my + mh - shift - 1, 10, 10);
			}
			else {
				setfillcolor(RGB(240, 220, 170));
				fillroundrect(mx + 1, my + 1, mx + mw - 1, my + mh - 1, 10, 10);
			}
			showTextInMiddle(20, 300);
		}
		return up;
	}
	void showText(wchar_t text[], int dx, int dy, int size, int thick) {
		LOGFONT f;
		gettextstyle(&f);
		f.lfHeight = size;
		f.lfWeight = thick;
		settextstyle(&f);
		outtextxy(dx + mx, dy + my, text);
	}
	void showTextInMiddle(int size = 20, int thick = 2000) {
		settextcolor(BLACK);
		settextstyle(size, 0, L"微软雅黑");
		int width1 = mx + (mw - textwidth(text)) / 2;
		int height1 = my + (mh - textheight(text)) / 2;
		outtextxy(width1, height1, text);
	}
};
class GUI {
	static constexpr int width = 750;
	static constexpr int height = 800;
	static constexpr int row = 15;
	static constexpr int col = 15;
	static constexpr int grid_size = 36;
	static constexpr int start_x = (width - grid_size * (col - 1)) / 2;
	static constexpr int start_y = -height * 0.03 + (height - grid_size * (row - 1)) / 2;
	bool five;
	int nowplay;
	vector<pair<int, int>> moveSeq;
	Board bd;
	button btnplay[5];
	button infoo;
	button btnGrid[15][15];
	void draw_state() {
		settextcolor(RGB(0, 0, 0));
		settextstyle(23, 0, L"微软雅黑");
		outtextxy(width / 2 - textwidth(L"TEST") / 2, height * 0.054, L"TEST");
	}
	void draw_board() {
		Gdiplus::Graphics Graphics(GetImageHDC());
		Gdiplus::Pen Pen_1(Gdiplus::Color(0, 0, 0), 1.f);
		Gdiplus::Pen Pen_2(Gdiplus::Color(0, 0, 0), 2.f);
		// 设置绘图质量为高质量
		Graphics.SetSmoothingMode(Gdiplus::SmoothingMode::
			SmoothingModeHighQuality);
		//绘制棋盘位置标号
		setlinestyle(PS_SOLID, 1);
		settextcolor(BLACK);
		settextstyle(20, 0, L"微软雅黑");
		wchar_t c[3] = L"";
		for (int i = 1; i <= 9; i++) {
			swprintf_s(c, 3, L"%d", i);
			::outtextxy(start_x + (i - 1) * grid_size - 4, start_y - grid_size, c);
		}
		for (int i = 10; i <= 15; i++) {
			swprintf_s(c, 3, L"%d", i);
			::outtextxy(start_x + (i - 1) * grid_size - 8, start_y - grid_size, c);
		}
		for (int i = 1; i <= 9; i++) {
			swprintf_s(c, 3, L"%c", i + 'A' - 1);
			::outtextxy(start_x - grid_size + 5, start_y + (i - 1) * grid_size - 12, c);
		}
		for (int i = 10; i <= 15; i++) {
			swprintf_s(c, 3, L"%c", i + 'A' - 1);
			::outtextxy(start_x - grid_size + 5, start_y + (i - 1) * grid_size - 12, c);
		}
		//画线
		for (int i = 0; i < row; i++) {
			Graphics.DrawLine(&Pen_1, start_x, start_y + i * grid_size, start_x + (col - 1) * grid_size, start_y + i * grid_size);
		}
		for (int i = 0; i < col; i++) {
			Graphics.DrawLine(&Pen_1, start_x + i * grid_size, start_y, start_x + i * grid_size, start_y + (row - 1) * grid_size);
		}
		//外圈线加粗
		Graphics.DrawLine(&Pen_2, start_x, start_y, start_x + (col - 1) * grid_size, start_y);
		Graphics.DrawLine(&Pen_2, start_x, start_y + 14 * grid_size, start_x + (col - 1) * grid_size, start_y + 14 * grid_size);
		Graphics.DrawLine(&Pen_2, start_x, start_y, start_x, start_y + (row - 1) * grid_size);
		Graphics.DrawLine(&Pen_2, start_x + 14 * grid_size, start_y, start_x + 14 * grid_size, start_y + (row - 1) * grid_size);
		//绘制黑点
		setfillcolor(BLACK);
		solidcircle(start_x + 7 * grid_size, start_y + 7 * grid_size, 2);
		solidcircle(start_x + 3 * grid_size, start_y + 3 * grid_size, 2);
		solidcircle(start_x + 11 * grid_size, start_y + 11 * grid_size, 2);
		solidcircle(start_x + 3 * grid_size, start_y + 11 * grid_size, 2);
		solidcircle(start_x + 11 * grid_size, start_y + 3 * grid_size, 2);
	}
	void draw_piece(bool sig) {
		Gdiplus::Graphics Graphics(GetImageHDC());
		Gdiplus::Pen Pen_W(Gdiplus::Color(235, 235, 235), 17.f);
		Gdiplus::Pen Pen_w(Gdiplus::Color(255, 255, 255), 16.f);
		Gdiplus::Pen Pen_B(Gdiplus::Color(80, 80, 80), 17.f);
		Gdiplus::Pen Pen_b(Gdiplus::Color(0, 0, 0), 16.f);
		// 设置绘图质量为高质量
		Graphics.SetSmoothingMode(Gdiplus::SmoothingMode::
			SmoothingModeHighQuality);
		settextstyle(20, 0, L"微软雅黑");
		for (int i = 0; i < moveSeq.size(); i++) {
			int y = moveSeq[i].first, x = moveSeq[i].second;
			//if(0){
			if (i & 1) {
				int r = grid_size - 18, stx = start_x + x * grid_size - r / 2 - 1, sty = start_y + y * grid_size - r / 2 - 1;
				Graphics.DrawEllipse(&Pen_W, Gdiplus::Rect{ stx, sty, r, r });//绘制棋子外圈
				r = grid_size - 20, stx = start_x + x * grid_size - r / 2 - 1, sty = start_y + y * grid_size - r / 2 - 1;
				Graphics.DrawEllipse(&Pen_w, Gdiplus::Rect{ stx, sty, r, r });//绘制棋子内圈
				settextcolor(BLACK);
			}
			else {
				int r = grid_size - 18, stx = start_x + x * grid_size - r / 2 - 1, sty = start_y + y * grid_size - r / 2 - 1;
				Graphics.DrawEllipse(&Pen_B, Gdiplus::Rect{ stx, sty, r, r });//绘制棋子外圈
				r = grid_size - 20, stx = start_x + x * grid_size - r / 2 - 1, sty = start_y + y * grid_size - r / 2 - 1;
				Graphics.DrawEllipse(&Pen_b, Gdiplus::Rect{ stx, sty, r, r });//绘制棋子内圈
				settextcolor(WHITE);
			}
			if (sig) {//棋子标号
				TCHAR s[5];
				_stprintf_s(s, _T("%d"), i + 1);
				//标号居中
				int width = start_x + x * grid_size - grid_size / 2 - 1 + (grid_size - textwidth(s)) / 2;
				int height = start_y + y * grid_size - grid_size / 2 - 1 + (grid_size - textheight(s)) / 2;
				if (i == moveSeq.size() - 1) settextcolor(RGB(240, 85, 60));
				outtextxy(width, height, s);
			}
		}
	}
	void Update(int x, int y) {
		moveSeq.push_back(make_pair(x, y));
		bd.Update(make_pair(x, y));
		five = findfive(nowplay);
		nowplay = nowplay ^ 1;
	}
	void Undo() {
		if (moveSeq.size() == 0)return;
		bd.Undo();
		moveSeq.pop_back();
		five = false;
		nowplay = nowplay ^ 1;
	}
	void Reset() {
		moveSeq.clear();
		bd.Reset();
		five = false;
		nowplay = P1;
	}
	inline bool Isin(int x, int y) { return x < 15 && y < 15 && x >= 0 && y >= 0; }
	bool findfive(int piece) {
		if (moveSeq.size() < 9)return false;
		int x = moveSeq.back().first, y = moveSeq.back().second;
		for (int i = 0; i < 4; i++) {
			int combo = 1;
			for (int k = 1; ; k++) {
				if (Isin(x + k * d4[i][0], y + k * d4[i][1]) && bd.board[x + k * d4[i][0]][y + k * d4[i][1]] == piece) combo++;
				else break;
				if (combo == 5) return true;
			}
			for (int k = 1; ; k++) {
				if (Isin(x - k * d4[i][0], y - k * d4[i][1]) && bd.board[x - k * d4[i][0]][y - k * d4[i][1]] == piece) combo++;
				else break;
				if (combo == 5) return true;
			}
		}
		return false;
	}
	inline void draw_info() {
		settextcolor(RGB(0, 0, 0));
		settextstyle(20, 0, L"微软雅黑");
		if (five) {
			nowplay & 1 ?
				outtextxy(start_x + 6 * grid_size + 8, start_y + 14.5 * grid_size + 5, L"黑子获胜") :
				outtextxy(start_x + 6 * grid_size + 8, start_y + 14.5 * grid_size + 5, L"白子获胜");
		}
		if (!moveSeq.empty()) {
			wchar_t latest[32];
			char ch = moveSeq[moveSeq.size() - 1].first + 'A'; int num = moveSeq[moveSeq.size() - 1].second + 1;
			swprintf(latest, 10, L"上一步棋: %c%d ", ch, num);
			outtextxy(start_x + grid_size + 6, start_y + 14.5 * grid_size + 5, latest);
		}
	}
public:
	GUI() {
		initgraph(width, height, SHOWCONSOLE);
		setbkcolor(RGB(255, 248, 196));
		LOGFONT f;
		gettextstyle(&f);
		_tcscpy_s(f.lfFaceName, _T("微软雅黑"));
		f.lfQuality = ANTIALIASED_QUALITY;
		settextstyle(&f);
		setbkmode(TRANSPARENT);
		cleardevice();
		Gdiplus::GdiplusStartupInput Input;
		ULONG_PTR Token;
		Gdiplus::GdiplusStartup(&Token, &Input, NULL);
		nowplay = P1;
		five = false;
		wchar_t strplay[5][30] = { L"",L"play",L"撤销",L"重置",L"" };
		int btnw = 90, btnh = 30;
		for (int i = 0; i < 5; i++) {
			btnplay[i].set(width / 2 - 5 * (btnw + 4) / 2 + i * (btnw + 4), height * 0.88, btnw, btnh, 3);
			btnplay[i].set_text(strplay[i], 30);
		}
		infoo.set(width / 2 - 14 * grid_size / 2, start_y + 14.5 * grid_size, 14 * grid_size, btnh, 2);
		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				btnGrid[i][j].set(start_x + i * grid_size - grid_size / 2, start_y + j * grid_size - grid_size / 2, grid_size - 2, grid_size - 2, 1);
			}
		}
	}
	void Run() {
		ExMessage msg;
		while (1) {
			Sleep(5);
			BeginBatchDraw();
			cleardevice();
			draw_board();
			draw_state();
			int op = -1;

			::peekmessage(&msg, EM_MOUSE);
			int y = (msg.x - start_x + grid_size / 2) / grid_size;
			int x = (msg.y - start_y + grid_size / 2) / grid_size;
			if (Isin(x, y))btnGrid[y][x].show(msg);

			if (!five) {
				if (msg.message == WM_LBUTTONDOWN) {
					if (Isin(x, y) && bd.board[x][y] == Empty) {
						Update(x, y);
						/*cout << nf.evaluation[P1] << '\n';
						for (int i = 0; i < 15; i++) {
							for (int j = 0; j < 15; j++) {
								if (board[i][j] == Empty)
									cout << bd.unit[i][j].fscore[P1] << "\t";
								else cout << "-\t";
							}
							cout << "\n";
						}*/
						/*for (int j = 0; j < 5; j++)
							cout << nf.cntT[j][0] << " ";
						cout << "\n";
						for (int j = 0; j < 5; j++)
							cout << nf.cntT[j][1] << " ";
						cout << "\n";*/
					}
				}
			}
			infoo.show(msg);

			draw_info();

			draw_piece(true);
			for (int i = 0; i < 5; i++) {
				::peekmessage(&msg, EM_MOUSE);
				if (btnplay[i].show(msg)) {
					op = i;
				}
			}

			if (op == 0) {


			}
			else if (op == 1) {//AI落子
				Search sh(moveSeq);
				pair<int, int> p = sh.bot_decision();
				Update(p.first, p.second);
#ifdef TEST
				cout << "tot: " << cnt << "\n";
				cnt = 0;
				cout << "tott: " << cntt << "\n";
				cntt = 0;
				cout << "TSS截断: " << tcut << "\n";
				tcut = 0;
				cout << "HASH截断: " << hcut << "\n";
				hcut = 0;
				cout << "BETA截断: " << bcut << "\n";
				bcut = 0;
				cout << "平均扩展节点数; " << totex / max(cntex, 1.0) << "\n";
				cout << "启发式截断:\n";
				for (int i = 0; i < 10; i++) {
					cout << pacut[i] << " ";
					pacut[i] = 0;
				}
				cout << '\n';
				for (int i = 0; i < 10; i++) {
					cout << pbcut[i] << " ";
					pbcut[i] = 0;
				}
				cout << '\n';
				cout << "HASH覆盖: " << rewrite << "\n";
				rewrite = 0;
				cout << "\n";
				cout << "T4:" << bd.nf.cntT[T4][P1] << "\n";
				cout << "TH4:" << bd.nf.cntT[TH4][P1] << "\n";
				cout << "T5:" << bd.nf.cntT[T5][P1] << "\n";
				cout << "\n";
#endif	
				msg.message = 0;
			}
			else if (op == 2) {//撤销
				Undo();
				msg.message = 0;
			}
			else if (op == 3) {//重置
				Reset();
				msg.message = 0;
			}
			else if (op == 4) {
				msg.message = 0;
			}
			EndBatchDraw();
		}
	}
};

signed main() {
	ACautomation::EvaluatorInit();
	GUI* draw = new GUI;
	draw->Run();
	delete draw;
}
#endif

#ifdef TEST2
signed main() {
	ACautomation::EvaluatorInit();
	vector<pair<int, int>> vec;
	int n; cin >> n;
	while (1) {
		int x, y;
		cin >> x >> y;
		if (x != -1) {
			vec.push_back(make_pair(x, y));
		}
		Search s(vec);
		pair<int, int> p = s.bot_decision();
		vec.push_back(p);
		printf("%d %d\n", p.first, p.second);
		printf(">>>BOTZONE_REQUEST_KEEP_RUNNING<<<\n");
		cout << flush;
	}
}
#endif

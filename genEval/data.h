﻿#pragma once

#include <vector>

namespace Eval {
	constexpr int LINE_NUM = 15;

	const uint8_t eval[LINE_NUM] = {
		0,
		2,3,4,			4,5,6,	//2
		16,18,20,		30,34,	//3
		45,				60,	//4
		100,
	};
	const int lineID[] = {
		1,1,1,2,2,2,3,3,3,3,			4,5,5,6,6,6,//2
		7,7,7,7,7,8,8,9,9,9,			10,10,11,11,//3
		12,12,12,12,12,					13,			//4
		14,											//5
	};
	const std::vector<std::vector<int>> patterns = {
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
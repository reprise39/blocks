#pragma once
#include <vector>
#include <utility>
#include <set>
#include <map>
#include <algorithm>
#include <climits>
#include "mino.hpp"
 
// ========================================
// ピースの回転・反転・初期化し、全パターンを事前に列挙しておく関数群
// initPieces() で生成、以降は読み取り専用
// ========================================
 
std::vector<std::vector<MinoPattern>> ALL_PIECES;
 
// -----------------------------------------------
// 置きにくさを定義する（評価関数用）
// -----------------------------------------------
inline constexpr int HARDNESS_BY_PIECE_ID[NUM_PIECES] = 
{
	// A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U
	   1, 2, 3, 3, 4, 4, 4, 3, 4, 5, 6, 6, 5, 6, 6, 7, 7, 8, 7, 7, 7
};

// -----------------------------------------------
// 最小y,xが0になるようシフト＆ソート
// -----------------------------------------------
std::vector<std::pair<int,int>> normalize(std::vector<std::pair<int,int>> piece)
{
	int min_y = INT_MAX;
	int min_x = INT_MAX;
	for (auto [y, x] : piece)
	{
		min_y = std::min(min_y, y);
		min_x = std::min(min_x, x);
	}
	for (auto& [y, x] : piece)
	{
		y -= min_y;
		x -= min_x;
	}
	std::sort(piece.begin(), piece.end());
	return piece;
}
 
// -----------------------------------------------
// rotate:90度回転: (y, x) → (x, -y)
// -----------------------------------------------
std::vector<std::pair<int,int>> rotate90(const std::vector<std::pair<int,int>>& piece)
{
	std::vector<std::pair<int,int>> result;
	for (auto [y, x] : piece)
		result.push_back({x, -y});
	return normalize(result);
}
 
// -----------------------------------------------
// filp:反転: (y, x) → (y, -x)
// -----------------------------------------------
std::vector<std::pair<int,int>> flip(const std::vector<std::pair<int,int>>& piece)
{
	std::vector<std::pair<int,int>> result;
	for (auto [y, x] : piece)
		result.push_back({y, -x});
	return normalize(result);
}
 
// -----------------------------------------------
// 全ミノの全回転・反転パターンを生成
// 重複は消去できているはず
// -----------------------------------------------
void initPieces()
{
	ALL_PIECES.resize(NUM_PIECES);
	for (int id = 0; id < NUM_PIECES; id++)
	{
		const int hardness = HARDNESS_BY_PIECE_ID[id];

		std::map<std::vector<std::pair<int,int>>,MinoPattern> unique_patterns;
		auto piece = BASE_PIECES[id];
 
		for(int r = 0 ; r < 4 ; r++)
		{
			for(int f = 0; f < 2; f++)
			{
				auto normalized = normalize(piece);
				if(!unique_patterns.count(normalized))
				{
					unique_patterns[normalized] =
					{
						id,
						hardness,
						(2*r)+f,
						r,
						f,
						normalized
					};
				}
				piece = flip(piece);
			}
			piece = rotate90(piece);
		}

 		std::vector<MinoPattern> patterns;
		for(auto& pair : unique_patterns) {
			patterns.push_back(pair.second);
		}
		ALL_PIECES[id] = patterns;
	}
}

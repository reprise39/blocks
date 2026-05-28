#pragma once
#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <utility>
#include <random>
#include <assert.h>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <functional>
#include <queue>
#include <set>

constexpr const int H = 14;  // 盤面高さ
constexpr const int W = 14;  // 盤面幅

#include "mino.hpp"
#include "mino_utils.hpp"
#include "timekeeper.hpp"
#include "blocks_evaluate.hpp"

struct Action
{
	const MinoPattern* mino_ptr;
	int x;
	int y;
	
	std::string toString() const
	{
		if (mino_ptr == nullptr) return "X000";
		char p = 'A' + mino_ptr->piece_id;
		char r = '0' + mino_ptr->num;
		char hx = (x + 1) < 10 ? '0' + (x + 1) : 'A' + (x + 1 - 10);
		char hy = (y + 1) < 10 ? '0' + (y + 1) : 'A' + (y + 1 - 10);
		return std::string{p, r, hx, hy};
	}
};

class BlocksState
{
public:
	int board[H][W];
	int turn_;
	bool is_passed[2];
	int my_player_number;
	
	struct Player
	{
		bool have_mino[21];
		int game_score_;
		Player() : game_score_(0)
		{
			for (int i = 0; i < 21; i++) have_mino[i] = true;
		}
	};
	std::vector<Player> Players_;

	BlocksState(int my_player)
	{
		for(int y=0;y<H;y++) for(int x=0;x<W;x++) board[y][x] = 0;
		turn_ = 0;
		is_passed[0] = false;
		is_passed[1] = false;
		my_player_number = my_player;
		Players_.resize(2);
	}

	void parseBoard(const std::vector<std::string>& lines)
	{
		for(int y = 0; y < H ; y++)
		{
			for(int x = 0; x < W; x++)
			{
				char c = lines[y][x];
				if (c == 'o') board[y][x] = 1;
				else if (c == 'x') board[y][x] = 2;
				else board[y][x] = 0;
			}
		}
		// スコア計算
		Players_[0].game_score_ = 0;
		Players_[1].game_score_ = 0;
		for(int y=0;y<H;y++) 
		{
			for(int x=0;x<W;x++)
			{
				if(board[y][x] == 1) Players_[0].game_score_++;
				if(board[y][x] == 2) Players_[1].game_score_++;
			}
		}
	}
	
int currentPlayer() const
	{
		// 偶数ターンは自分、奇数ターンは相手
		if (turn_ % 2 == 0) return my_player_number;
		return (my_player_number == 1) ? 2 : 1;
	}

	bool canPlace(int pid, const MinoPattern& mino, int ox, int oy, bool isFirst) const
	{
		bool on_start = false;
		bool corner_ok = false;
		int player_id = currentPlayer();
		if(isFirst)
		{
			bool start_ok = false;
			int start_x = (player_id == 1) ? 4 : 9;
			int start_y = (player_id == 1) ? 4 : 9;
			for(auto& cell : mino.cells)
			{
				if(oy + cell.first == start_y && ox + cell.second == start_x) start_ok = true; //配列外に行くことはない
			}
			return start_ok;
		}

		for(auto cell : mino.cells)
		{
			int y = oy + cell.first;
			int x = ox + cell.second;
			if(x < 0 || x >= W || y < 0 || y >= H) return false;
			if(board[y][x] != 0) return false;
			
			// 辺の接触チェック(NG)
			int dy[] = {-1, 1, 0, 0};
			int dx[] = {0, 0, -1, 1};
			for(int i = 0; i < 4 ; i++)
			{
				int ny = y + dy[i];
				int nx = x + dx[i];
				if(ny >= 0 && ny < H && nx  >=0 && nx < W)
				{
					if(board[ny][nx] == player_id) return false;
				}
			}
			
			// 角の接触チェック(OK)
			int dcy[] = {-1, -1, 1, 1};
			int dcx[] = {-1, 1, -1, 1};
			for(int i = 0 ; i < 4 ; i++)
			{
				int ny = y + dcy[i];
				int nx = x + dcx[i];
				if(ny >= 0 && ny < H && nx >= 0 && nx < W)
				{
					if(board[ny][nx] == player_id) corner_ok = true;
				}
			}
		}
		return corner_ok;
	}

	std::vector<Action> legalActions() const
	{
		std::vector<Action> actions;
		int p_idx = currentPlayer() - 1;
		bool isFirst = (Players_[p_idx].game_score_ == 0);
		
		for(int id = 0; id < 21; id++)
		{
			if(!Players_[p_idx].have_mino[id]) continue;
			for(int num=0; num < ALL_PIECES[id].size(); num++)
			{
				auto& mino = ALL_PIECES[id][num];
				for (int y=0; y < H; y++)
				{
					for(int x=0; x < W; x++)
					{
						if(canPlace(id, mino, x, y, isFirst))
						{
							actions.push_back({&ALL_PIECES[id][num], x, y});
						}
					}
				}
			}
		}
		if (actions.empty()) actions.push_back({nullptr, 0, 0});
		return actions;
	}

void advance(const Action& action)
	{
		int p_idx = currentPlayer() - 1;
		if (action.mino_ptr == nullptr) is_passed[p_idx] = true;
		else
		{
			for(auto cell : action.mino_ptr->cells)
			{
				int y = action.y + cell.first;
				int x = action.x + cell.second;
				board[y][x] = currentPlayer();
			}
			Players_[p_idx].have_mino[action.mino_ptr->piece_id] = false;
			Players_[p_idx].game_score_ += action.mino_ptr->cells.size();
		}
		turn_++;
	}
	
	// 評価部分
	int getEvaluateScore(int root_player_id) const
	{
		const int self_id = root_player_id; // 常に自分の視点で評価する
		const int opp_id  = (self_id == 1 ? 2 : 1);
	
		int filled = 0;
		for (int y = 0; y < H; y++)
		{
			for (int x = 0; x < W; x++)
			{
				if (board[y][x] != 0) filled++;
			}
		}
		const double time_factor = static_cast<double>(filled) / static_cast<double>(H * W);
		
		return evaluate
		(
			board,
			self_id,
			opp_id,
			Players_[self_id - 1].have_mino,
			Players_[opp_id - 1].have_mino,
			time_factor
		);
	}
};

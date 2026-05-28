#include <cstdlib>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>

#include "mino.hpp"
#include "mino_utils.hpp"
#include "timekeeper.hpp"
#include "blocksstate.hpp"

namespace
{
	constexpr int SEARCH_DEPTH = 3; // 自分(0) -> 相手(1) -> 自分(2)
	constexpr int BEAM_WIDTH = 10;  // N=10
	constexpr int TIME_LIMIT_MS = 9000; // 9秒で打ち切る

	constexpr int WIN_SCORE = std::numeric_limits<int>::max() / 4;
	constexpr int INF = std::numeric_limits<int>::max() / 2;

	int configuredTimeLimitMs()
	{
		const char *value = std::getenv("MYGAME_TIME_LIMIT_MS");
		if (value == nullptr || value[0] == '\0')
		{
			return TIME_LIMIT_MS;
		}
		return std::max(1, std::atoi(value));
	}

	bool isTerminalState(const BlocksState &state)
	{
		return state.is_passed[0] && state.is_passed[1];
	}

	// ある盤面から合法手をすべて生成し、評価値が高い（または低い）上位N手だけを返すヘルパー
	struct ChildNode {
		Action act;
		BlocksState state;
		int eval_score;
	};

	std::vector<ChildNode> getTopNChildren(const BlocksState& state, int root_player, bool is_maximizing, int N, TimeKeeper& timer) {
		auto actions = state.legalActions();
		std::vector<ChildNode> children;
		
		for (const auto& act : actions) {
			if (timer.isTimeOver()) break;
			BlocksState next_state = state; // 盤面コピー
			next_state.advance(act);
			children.push_back({act, next_state, next_state.getEvaluateScore(root_player)});
		}

		// is_maximizingがtrue（自分の手番）ならスコアが高い順、false（相手の手番）なら低い順にソート
		if (is_maximizing) {
			std::sort(children.begin(), children.end(), [](const ChildNode& a, const ChildNode& b) { return a.eval_score > b.eval_score; });
		} else {
			std::sort(children.begin(), children.end(), [](const ChildNode& a, const ChildNode& b) { return a.eval_score < b.eval_score; });
		}

		// 上位N個に絞る
		if (children.size() > static_cast<size_t>(N))
		{
			children.erase(children.begin() + N, children.end());
		}
		return children;
	}

	// 再帰的な上位N手限定ミニマックス探索
	int minimax(BlocksState state, int depth, bool is_maximizing, int root_player, TimeKeeper& timer, int N) {
		if (depth == 0 || timer.isTimeOver() || isTerminalState(state)) {
			return state.getEvaluateScore(root_player);
		}

		auto top_children = getTopNChildren(state, root_player, is_maximizing, N, timer);
		if (top_children.empty()) {
			return state.getEvaluateScore(root_player);
		}

		if (is_maximizing) { // 自分のターン（Maxノード）
			int max_eval = -INF;
			for (const auto& child : top_children) {
				int eval = minimax(child.state, depth - 1, false, root_player, timer, N);
				max_eval = std::max(max_eval, eval);
			}
			return max_eval;
		} else { // 相手のターン（Minノード）
			int min_eval = INF;
			for (const auto& child : top_children) {
				int eval = minimax(child.state, depth - 1, true, root_player, timer, N);
				min_eval = std::min(min_eval, eval);
			}
			return min_eval;
		}
	}

	Action beamMinimaxSearch(const BlocksState &root_state)
	{
		int root_player = root_state.my_player_number;
		auto root_actions = root_state.legalActions();
		if (root_actions.empty()) return Action{nullptr, 0, 0};
		if (root_actions.size() == 1 && root_actions[0].mino_ptr == nullptr) return root_actions[0];

		TimeKeeper timer(configuredTimeLimitMs());

		// 最初の1手目（ルート）も上位N手に絞る
		auto top_children = getTopNChildren(root_state, root_player, true, BEAM_WIDTH, timer);
		if (top_children.empty()) return root_actions[0];

		Action best_action = top_children[0].act;
		int best_score = -INF;

		// 絞った上位N手それぞれについて、ミニマックス探索を回す
		for (const auto& child : top_children) {
			if (timer.isTimeOver()) break;
			
			// ルート直下は相手のターンになるので is_maximizing = false
			int score = minimax(child.state, SEARCH_DEPTH - 1, false, root_player, timer, BEAM_WIDTH);
			
			if (score > best_score) {
				best_score = score;
				best_action = child.act;
			}
		}

		return best_action;
	}
} // namespace

int main()
{
	initPieces();

	int player_number;
	if (!(std::cin >> player_number)) return 0;

	BlocksState state(player_number);
	std::vector<std::string> lines(H);

	while (true)
	{
		bool input_failed = false;
		for (int i = 0; i < H; i++)
		{
			if (!(std::cin >> lines[i]))
			{
				input_failed = true;
				break;
			}
		}
		if (input_failed) break;

		state.parseBoard(lines);
		Action act = beamMinimaxSearch(state);
		std::cout << act.toString() << std::endl;
		
		if (act.mino_ptr == nullptr) break;
		state.advance(act);
	}

	return 0;
}
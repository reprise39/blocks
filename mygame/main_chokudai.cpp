#include <cstdlib>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <vector>

#include "mino.hpp"
#include "mino_utils.hpp"
#include "timekeeper.hpp"
#include "blocksstate.cpp"

namespace
{
	constexpr int SEARCH_DEPTH = 3;
	constexpr int SEARCH_WIDTH = 10;
	constexpr int TIME_LIMIT_MS = 8000;

	int configuredTimeLimitMs()
	{
		const char *value = std::getenv("MYGAME_TIME_LIMIT_MS");
		if (value == nullptr || value[0] == '\0')
		{
			return TIME_LIMIT_MS;
		}
		return std::max(1, std::atoi(value));
	}

	struct Node
	{
		BlocksState state;
		Action first_action;
		int score;
		int depth;
		bool has_first_action;
		bool terminal;

		bool operator<(const Node &other) const
		{
			return score < other.score;
		}
	};

	constexpr int WIN_SCORE = std::numeric_limits<int>::max() / 4;

	bool isTerminalState(const BlocksState &state)
	{
		return state.is_passed[0] && state.is_passed[1];
	}

	int terminalRootScore(const BlocksState &state)
	{
		const int self_score = state.Players_[0].game_score_;
		const int opp_score = state.Players_[1].game_score_;
		if (self_score > opp_score)
		{
			return WIN_SCORE;
		}
		if (self_score < opp_score)
		{
			return -WIN_SCORE;
		}
		return 0;
	}

	int rootPerspectiveScore(const Node &node)
	{
		if (node.terminal)
		{
			return node.score;
		}
		if (node.depth % 2 == 0)
		{
			return node.score;
		}
		return -node.score;
	}

	Node makeChild(const Node &parent, const Action &action)
	{
		Node child{parent.state, parent.first_action, parent.score, parent.depth + 1, parent.has_first_action, false};
		child.state.advance(action);
		child.terminal = isTerminalState(child.state);
		child.score = child.terminal ? terminalRootScore(child.state) : child.state.getEvaluateScore();
		if (!parent.has_first_action)
		{
			child.first_action = action;
			child.has_first_action = true;
		}
		return child;
	}

	Action chokudaiSearch(const BlocksState &root_state)
	{
		auto root_actions = root_state.legalActions();
		if (root_actions.empty())
		{
			return Action{nullptr, 0, 0};
		}
		if (root_actions.size() == 1 && root_actions[0].mino_ptr == nullptr)
		{
			return root_actions[0];
		}

		Action fallback = root_actions[0];
		Action best_action = fallback;
		int best_score = std::numeric_limits<int>::min();

		std::vector<std::priority_queue<Node>> beams(SEARCH_DEPTH + 1);
		Node root{root_state, fallback, root_state.getEvaluateScore(), 0, false, isTerminalState(root_state)};
		if (root.terminal)
		{
			root.score = terminalRootScore(root.state);
		}
		beams[0].push(root);

		TimeKeeper timer(configuredTimeLimitMs());

		while (!timer.isTimeOver())
		{
			bool expanded_any = false;
			for (int depth = 0; depth < SEARCH_DEPTH && !timer.isTimeOver(); ++depth)
			{
				for (int i = 0; i < SEARCH_WIDTH && !timer.isTimeOver(); ++i)
				{
					if (beams[depth].empty())
					{
						break;
					}

					Node current = beams[depth].top();
					beams[depth].pop();
					expanded_any = true;

					const int current_root_score = rootPerspectiveScore(current);
					if (current.has_first_action && current_root_score > best_score)
					{
						best_score = current_root_score;
						best_action = current.first_action;
					}

					if (depth == SEARCH_DEPTH)
					{
						continue;
					}

					auto actions = current.state.legalActions();
					if (actions.size() == 1 && actions[0].mino_ptr == nullptr)
					{
						continue;
					}

					for (const auto &action : actions)
					{
						Node child = makeChild(current, action);
						beams[depth + 1].push(std::move(child));
					}
				}
			}

			if (!expanded_any)
			{
				break;
			}
		}

		return best_action;
	}
} // namespace

int main()
{
	initPieces();

	int player_number;
	if (!(std::cin >> player_number))
	{
		return 0;
	}

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
		if (input_failed)
		{
			break;
		}

		state.parseBoard(lines);
		Action act = chokudaiSearch(state);
		std::cout << act.toString() << std::endl;
		if (act.mino_ptr == nullptr)
		{
			break;
		}
		state.advance(act);
	}

	return 0;
}
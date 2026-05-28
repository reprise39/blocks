#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include "mino.hpp"
#include "mino_utils.hpp"
#include "timekeeper.hpp"
#include "blocksstate.cpp"

bool shouldProfileSearch()
{
	const char *value = std::getenv("MYGAME_PROFILE_SEARCH");
	return value != nullptr && value[0] != '\0' && value[0] != '0';
}

Action greedyAction(const BlocksState &state, bool profile = false)
{
	TimeKeeper timer(0);
	auto actions = state.legalActions();
	if (actions.size() == 1 && actions[0].mino_ptr == nullptr)
	{
		if (profile)
		{
			std::cerr << "[profile] greedyAction us=" << timer.elapsedMicroseconds() << " candidates=" << actions.size() << std::endl;
		}
		return actions[0];
	}

	int best_score = -1000000;
	Action best_action = actions[0];

	for (const auto &action : actions)
	{
		BlocksState next_state = state;
		next_state.advance(action);
		int score = next_state.getEvaluateScore();
		if (score > best_score)
		{
			best_score = score;
			best_action = action;
		}
	}
	if (profile)
	{
		std::cerr << "[profile] greedyAction us=" << timer.elapsedMicroseconds() << " candidates=" << actions.size() << std::endl;
	}
	return best_action;
}

#ifdef MYGAME_ALT_PROFILE
Action greedyActionAlt(const BlocksState &state, bool profile = false)
{
	TimeKeeper timer(0);
	auto actions = state.legalActions();
	if (actions.size() == 1 && actions[0].mino_ptr == nullptr)
	{
		if (profile)
		{
			std::cerr << "[profile] greedyActionAlt us=" << timer.elapsedMicroseconds() << " candidates=" << actions.size() << std::endl;
		}
		return actions[0];
	}

	int best_score = -1000000;
	Action best_action = actions[0];

	for (const auto &action : actions)
	{
		BlocksState next_state = state;
		next_state.advance(action);
		int score = next_state.getEvaluateScore();
		if (action.mino_ptr != nullptr)
		{
			score += action.mino_ptr->hardness * 2;
		}
		if (score > best_score)
		{
			best_score = score;
			best_action = action;
		}
	}
	if (profile)
	{
		std::cerr << "[profile] greedyActionAlt us=" << timer.elapsedMicroseconds() << " candidates=" << actions.size() << std::endl;
	}
	return best_action;
}
#endif

int main()
{
	// std::ios_base::sync_with_stdio(false);
	// std::cin.tie(nullptr);

	initPieces();

	int player_number; // 先攻(1) または 後攻(2)
	if (!(std::cin >> player_number))
		return 0;

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
			break;
		state.parseBoard(lines);
		const bool profile_search = shouldProfileSearch();

		// 自分の思考ターン
#ifdef MYGAME_ALT_PROFILE
		Action act = greedyActionAlt(state, profile_search);
#else
		Action act = greedyAction(state, profile_search);
#endif
		std::cout << act.toString() << std::endl;
		if (act.mino_ptr == nullptr)
			break;
		state.advance(act);
	}
	return 0;
}
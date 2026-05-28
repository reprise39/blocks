CXX := g++
CXXFLAGS := -O2 -std=c++17
CPP_SOURCE_DIR := ./cpp_source
MINIMAX_DIR := $(CPP_SOURCE_DIR)/minimax_beam
ONE_GREEDY_DIR := $(CPP_SOURCE_DIR)/one_greedy
COMMON_HEADERS := blocksstate.hpp mino.hpp mino_utils.hpp timekeeper.hpp

minimax_1: $(MINIMAX_DIR)/main_minimax.cpp $(addprefix $(MINIMAX_DIR)/,$(COMMON_HEADERS))
	$(CXX) $(CXXFLAGS) -o blocksstate_minimax_1 $(MINIMAX_DIR)/main_minimax.cpp

one_greedy_1: $(ONE_GREEDY_DIR)/main.cpp $(addprefix $(ONE_GREEDY_DIR)/,$(COMMON_HEADERS))
	$(CXX) $(CXXFLAGS) -o blocksstate_one_greedy_1 $(ONE_GREEDY_DIR)/main.cpp
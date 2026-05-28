#pragma once

#include <string>
#include <vector>
#include <queue>
#include <cmath>

#include "mino_utils.hpp"

void countBlocks(const int board[H][W], int self_id, int opp_id, int& out_self, int& out_opp)
{
    out_self = 0;
    out_opp = 0;
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            if (board[y][x] == self_id)  out_self++;
            else if (board[y][x] == opp_id) out_opp++;
        }
    }
}

double interpolateWeight(double start_val, double end_val, double time_factor)
{
    return start_val * (1.0 - time_factor) + end_val * time_factor;
}

// 1.埋めたマスの数（勝敗基準そのまま）
int evalPlacedCells(const int board[H][W], int self_id, int opp_id)
{
    int self_cnt, opp_cnt;
    countBlocks(board, self_id, opp_id, self_cnt, opp_cnt);
    return self_cnt - opp_cnt;
}

// 2.生きている角部分の数（ずっと大事そう）
int evalCornerPotential(const int board[H][W], int self_id, int opp_id)
{
    int self_potential = 0;
    int opp_potential  = 0;

    const int dcy[4] = {-1, -1, 1, 1};
    const int dcx[4] = {-1, 1, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};
    const int dx[4] = {0, 0, -1, 1};

    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            if (board[y][x] != 0) continue;
            bool self_diag_ok = false;
            bool opp_diag_ok  = false;
            bool self_edge_ng = false;
            bool opp_edge_ng  = false;

            for (int k = 0; k < 4; k++)
            {
                const int ny = y + dcy[k];
                const int nx = x + dcx[k];
                if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
                const int val = board[ny][nx];
                if (val == self_id) self_diag_ok = true;
                if (val == opp_id)  opp_diag_ok  = true;
            }

            for (int k = 0; k < 4; k++)
            {
                const int ny = y + dy[k];
                const int nx = x + dx[k];
                if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
                const int val = board[ny][nx];
                if (val == self_id) self_edge_ng = true;
                if (val == opp_id)  opp_edge_ng  = true;
            }

            if (self_diag_ok && !self_edge_ng) self_potential++;
            if (opp_diag_ok && !opp_edge_ng)   opp_potential++;
        }
    }
    return self_potential - opp_potential;
}

// 3.角の隣接空間が広いとよい（２番の拡張ではあるかも、ずっと大事そう）
int evalCornerMobilityBFS(const int board[H][W], int self_id, int opp_id)
{
    const int dy4[4] = {-1, 1, 0, 0};
    const int dx4[4] = {0, 0, -1, 1};
    const int dcy[4] = {-1, -1, 1, 1};
    const int dcx[4] = {-1, 1, -1, 1};
    const int DEPTH_LIMIT = 4;

    auto is_legal_corner = [&](int y, int x, int pid) {
        if (board[y][x] != 0) return false;
        bool diagonal = false;
        bool edge_touch = false;
        for (int k = 0; k < 4; k++)
        {
            const int ny = y + dcy[k];
            const int nx = x + dcx[k];
            if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
            if (board[ny][nx] == pid) diagonal = true;
        }
        for (int k = 0; k < 4; k++)
        {
            const int ny = y + dy4[k];
            const int nx = x + dx4[k];
            if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
            if (board[ny][nx] == pid) edge_touch = true;
        }
        return diagonal && !edge_touch;
    };

    auto mobility_score = [&](int pid) {
        std::vector<std::vector<int>> dist(H, std::vector<int>(W, -1));

        struct Node { int y, x, d; };
        std::queue<Node> q;

        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                if (!is_legal_corner(y, x, pid)) continue;
                if (dist[y][x] >= 0) continue;
                dist[y][x] = 0;
                q.push({y, x, 0});
            }
        }

        int total_score = 0;
        while (!q.empty())
        {
            const Node cur = q.front();
            q.pop();
            const int y = cur.y;
            const int x = cur.x;
            const int d = cur.d;

            total_score += 3;

            int free_neighbor = 0;
            for (int dir = 0; dir < 4; dir++)
            {
                const int ny = y + dy4[dir];
                const int nx = x + dx4[dir];
                if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
                if (board[ny][nx] == 0) free_neighbor++;
            }
            total_score += free_neighbor;
            if (free_neighbor <= 1) total_score -= 2;

            if (d >= DEPTH_LIMIT) continue;

            for (int dir = 0; dir < 4; dir++)
            {
                const int ny = y + dy4[dir];
                const int nx = x + dx4[dir];
                if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
                if (board[ny][nx] != 0) continue;
                if (dist[ny][nx] >= 0) continue;
                dist[ny][nx] = d + 1;
                q.push({ny, nx, d + 1});
            }
        }
        return total_score;
    };

    return mobility_score(self_id) - mobility_score(opp_id);
}

// 4.難しいピースを置くといい（終盤減衰したほうがよさそうか）
int evalHardPiecesUsed(const bool self_have[21], const bool opp_have[21])
{
    int self_used_hardness = 0;
    int opp_used_hardness  = 0;
    for (int i = 0; i < 21; i++)
    {
        const int hard = HARDNESS_BY_PIECE_ID[i];
        if (!self_have[i]) self_used_hardness += hard;
        if (!opp_have[i])  opp_used_hardness  += hard;
    }
    return self_used_hardness - opp_used_hardness;
}

// 5の補助関数
int calcPlayerSpread(const int board[H][W], int pid)
{
    int count = 0;
    int sum_y = 0;
    int sum_x = 0;

    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            if (board[y][x] == pid)
            {
                sum_y += y;
                sum_x += x;
                count++;
            }
        }
    }
    if (count == 0) return 0;

    int mean_y_10 = (sum_y * 10) / count;
    int mean_x_10 = (sum_x * 10) / count;

    int total_variance = 0;
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            if (board[y][x] == pid)
            {
                int dy_10 = (y * 10) - mean_y_10;
                int dx_10 = (x * 10) - mean_x_10;
                total_variance += (dy_10 * dy_10 + dx_10 * dx_10);
            }
        }
    }
    return (total_variance / count) / 10;
}

// 5.おいてあるミノの広がりを得点にする（序盤～中盤にかけて想定）
int evalSpreadScore(const int board[H][W], int self_id, int opp_id)
{
    int self_spread = calcPlayerSpread(board, self_id);
    int opp_spread  = calcPlayerSpread(board, opp_id);

    return (self_spread - opp_spread);
}

// 全５項目の和
int evaluate
(
    const int board[H][W],
    int self_id,
    int opp_id,
    const bool self_have[21],
    const bool opp_have[21],
    double time_factor
)
{
    const double w_cells      = interpolateWeight(0.5, 1.5, time_factor); //｛ 1番の要素の点数：0 ~ 89 ｝
    const double w_corner_pot = interpolateWeight(1.2, 1.2, time_factor); //｛ 2番の要素の点数：0 ~ 50くらい？ ｝
    const double w_corner_mob = interpolateWeight(1.0, 1.0, time_factor); //｛ 3番の要素の点数：0 ~ 500くらい？ ｝
    const double w_hard_pieces= interpolateWeight(0.8, 0.5, time_factor); //｛ 4番の要素の点数：0 ~ 100くらい？ ｝
    const double w_edge_pen   = interpolateWeight(1.5, 0.4, time_factor); //｛ 5番の要素の点数：0 ~ 100くらい？？？？ ｝

    const double score_cells   = evalPlacedCells(board, self_id, opp_id) * w_cells;
    const double score_corn_po = evalCornerPotential(board, self_id, opp_id) * w_corner_pot;
    const double score_corn_mo = evalCornerMobilityBFS(board, self_id, opp_id) * w_corner_mob;
    const double score_hard    = evalHardPiecesUsed(self_have, opp_have) * w_hard_pieces;
    const double score_edge    = evalSpreadScore(board, self_id, opp_id) * w_edge_pen;

    return static_cast<int>(score_cells + score_corn_po + score_corn_mo + score_hard + score_edge);
}

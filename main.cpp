#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>

/**
 * A mancala board is represented as an array of 14 "pits".
 *
 *        13
 *      00  12
 *      01  11
 *      02  10
 *      03  09
 *      04  08
 *      05  07
 *        06
 */
using board_type = std::array<std::uint8_t, 14>;

/**
 * The left side (0 to 5) are AlphaCala's, and 6 is its store.
 * The right side (7 to 12) are the opponents's, and 13 is their store.
 */
const std::array<int, 6> AC_PITS = {5, 4, 3, 2, 1, 0};
const std::array<int, 6> OPP_PITS = {12, 11, 10, 9, 8, 7};

/**
 * @brief Outputs a mancala board in a human-friendly format
 *
 * @param[in] board The board to print
 */
void print_board(const board_type &board) {
    auto print_pit = [&](int index, int num_spaces = 0) {
        for (int i = 0; i < num_spaces; ++i) {
            std::cout << ' ';
        }

        // Must cast to int because by default C++ prints std::uint8_t as char
        std::cout << std::setw(2) << std::setfill('0')
                  << static_cast<int>(board[index]);
    };

    print_pit(13, 2);
    std::cout << std::endl;

    for (int i = 0; i <= 5; ++i) {
        print_pit(i);
        print_pit(12 - i, 2);
        std::cout << std::endl;
    }

    print_pit(6, 2);
    std::cout << std::endl;
}

/**
 * @brief Calculates the next pit to drop a seed into, making sure to skip the
 * store of the opposing player
 *
 * @param[in] pit_index The index of the current pit
 * @param[in] is_ac_turn Whether it is AlphaCala's turn
 * @return The index of the next pit
 */
int next_pit_index(int pit_index, bool is_ac_turn) {
    if (pit_index == 12 && is_ac_turn) {
        return 0;
    }
    if (pit_index == 5 && !is_ac_turn) {
        return 7;
    }
    return (pit_index + 1) % 14;
}

/**
 * @brief Plays a move on a mancala board, mutating it
 *
 * @param[in,out] board The board to play the move on
 * @param[in] pit_index The index of the pit to play
 * @return True when the player gets to take another turn, and false otherwise
 */
bool play_move(board_type &board, int pit_index) {
    const auto is_ac_move = (pit_index <= 5);

    auto num_seeds = board[pit_index];
    board[pit_index] = 0;

    while (num_seeds > 1) {
        pit_index = next_pit_index(pit_index, is_ac_move);
        ++board[pit_index];
        --num_seeds;
    }

    pit_index = next_pit_index(pit_index, is_ac_move);

    // Handle last seed landing in a store
    if (pit_index == 6 || pit_index == 13) {
        ++board[pit_index];
        return true;
    }

    // Handle "stealing" when last seed lands facing a non-empty pit
    if (board[pit_index] == 0) {
        const auto end_on_ac_side = (pit_index <= 5);
        const auto facing_index = 12 - pit_index;

        if (board[facing_index] > 0 && end_on_ac_side == is_ac_move) {
            const auto store_index = (is_ac_move ? 6 : 13);

            board[store_index] += 1 + board[facing_index];
            board[facing_index] = 0;

            return false;
        }
    }

    ++board[pit_index];
    return false;
}

/**
 * @brief Uses the minimax algorithm to evaluate a mancala board position and
 * find the best move
 *
 * @param[in] board The current mancala board
 * @param[in] is_ac_turn Whether it is AlphaCala's turn
 * @param[in] depth How many moves to make in the search tree
 * @param[out] move_out Will be set to the best found move if it is not nullptr
 * @param[in] alpha For alpha-beta pruning optimization
 * @param[in] beta For alpha-beta pruning optimization
 * @return An integer for the evaluation of the position, where positive means
 * AlphaCala is winning and negative means the opponent is winning
 *
 */
int minimax(const board_type &board, bool is_ac_turn, int depth, int *move_out,
            int alpha = -99, int beta = 99) {
    // Evaluation is simply the difference between the players' stores
    if (depth == 0) {
        return board[6] - board[13];
    }

    auto best_eval = is_ac_turn ? -99 : 99;
    auto best_move = -1;

    if (is_ac_turn) {
        for (const auto i : AC_PITS) {
            if (board[i] == 0) continue;

            auto board_copy = board;
            bool go_again = play_move(board_copy, i);
            auto tmp_eval =
                minimax(board_copy, go_again, depth - 1, nullptr, alpha, beta);

            if (tmp_eval > best_eval) {
                best_eval = tmp_eval;
                best_move = i;
            }

            alpha = std::max(alpha, tmp_eval);
            if (beta <= alpha) break;
        }
    } else {
        for (const auto i : OPP_PITS) {
            if (board[i] == 0) continue;

            auto board_copy = board;
            bool go_again = play_move(board_copy, i);
            auto tmp_eval =
                minimax(board_copy, !go_again, depth - 1, nullptr, alpha, beta);

            if (tmp_eval < best_eval) {
                best_eval = tmp_eval;
                best_move = i;
            }

            beta = std::min(beta, tmp_eval);
            if (beta <= alpha) break;
        }
    }

    if (move_out != nullptr) {
        *move_out = best_move;
    }

    // If there were no possible moves, add remaining seeds to store and
    // end the game immediately (returning the evaluation)
    if (best_move == -1) {
        auto eval = board[6] - board[13];

        if (is_ac_turn) {
            for (const auto i : OPP_PITS) {
                eval -= board[i];
            }
        } else {
            for (const auto i : AC_PITS) {
                eval += board[i];
            }
        }
        return eval;
    }

    return best_eval;
}

int main() {
    const auto start_seeds = 4;
    const auto depth = 20;

    std::cout << "Is AlphaCala playing first (y/n)? ";

    char choice;
    std::cin >> choice;
    bool is_ac_turn = (choice == 'y');

    // Init board
    board_type board;
    for (const auto i : AC_PITS) {
        board[i] = start_seeds;
    }
    for (const auto i : OPP_PITS) {
        board[i] = start_seeds;
    }
    board[6] = 0;
    board[13] = 0;

    // Game loop
    while (true) {
        print_board(board);

        bool go_again;

        if (is_ac_turn) {
            int ac_move;
            auto eval = minimax(board, true, depth, &ac_move);

            if (ac_move == -1) break;

            std::cout << "AlphaCala plays " << ac_move << " (eval = " << eval
                      << ")" << std::endl;
            go_again = play_move(board, ac_move);
        } else {
            std::cout << "Opponent move row (0-5): ";
            int row;
            std::cin >> row;
            go_again = play_move(board, 12 - row);
        }

        if (!go_again) {
            is_ac_turn = !is_ac_turn;
        }
    }

    return 0;
}

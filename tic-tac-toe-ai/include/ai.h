/**
 * @file ai.h
 * @brief AI interface for Tic-Tac-Toe using Monte Carlo Tree Search (MCTS).
 */

#ifndef AI_H
#define AI_H

#include "board.h"
#include "tree.h"

/*============================================================================*/
/*                           FUNCTION PROTOTYPES                              */
/*============================================================================*/

/**
 * @brief Sets the AI difficulty level.
 * @param level The desired difficulty level (AI_LEVEL_EASY, AI_LEVEL_MEDIUM,
 *              or AI_LEVEL_HARD).
 */
void AI_SetLevel(AILevel level);

/**
 * @brief Gets the current AI difficulty level.
 * @return The current AILevel value.
 */
AILevel AI_GetLevel(void);

/**
 * @brief Chooses the best move for the current board state using MCTS.
 * @param board Pointer to the current board state.
 * @return The move index (0-8), or -1 if no valid move exists.
 */
int AI_ChooseMove(const Board* board);

/**
 * @brief Returns a random valid move from the available moves.
 * @param board Pointer to the current board state.
 * @return A random valid move index (0-8), or -1 if no valid move exists.
 */
int GetRandomMove(const Board* board);

#endif /* AI_H */

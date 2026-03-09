/**
 * @file board.c
 * @brief Game board implementation for Tic-Tac-Toe.
 */

#include "board.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*============================================================================*/
/*                           LOCAL FUNCTIONS                                  */
/*============================================================================*/

static void SeedRngOnce(void) {
	/**
	 * Seeds the random number generator exactly once.
	 * Uses a static flag to ensure srand() is only called on first invocation.
	 */
	static int seeded = 0;

	if (!seeded) {
		srand((unsigned int)time(NULL));
		seeded = 1;
	}
}

/*============================================================================*/
/*                             UTILS FUNCTIONS                                */
/*============================================================================*/

void InitBoard(Board* board) {
	/**
	 * Initializes a new game board by calling ResetBoard.
	 * This is an alias for ResetBoard for semantic clarity.
	 */
	ResetBoard(board);
}

void ResetBoard(Board* board) {
	/**
	 * Resets the board to its initial state:
	 *   - All cells set to PLAYER_NONE
	 *   - Randomly selects starting player (X or O)
	 *   - Sets game state to GAME_PLAYING
	 */
	SeedRngOnce();

	for (int i = 0; i < 9; i++) {
		board->cells[i] = PLAYER_NONE;
	}

	board->current_turn = (rand() % 2 == 0) ? PLAYER_X : PLAYER_O;
	board->state = GAME_PLAYING;
}

int IsMoveValid(const Board* board, int index) {
	/**
	 * Validates a move at the given index.
	 * A move is valid if:
	 *   - Index is within bounds (0-8)
	 *   - Game is still in progress
	 *   - The cell is empty
	 */
	if (index < 0 || index >= 9) {
		return 0;
	}

	if (board->state != GAME_PLAYING) {
		return 0;
	}

	return board->cells[index] == PLAYER_NONE;
}

void MakeMove(Board* board, int index) {
	/**
	 * Places the current player's mark at the given index.
	 * After placing the mark:
	 *   - Checks for game end condition
	 *   - Switches to the other player's turn if game continues
	 * Does nothing if the move is invalid.
	 */
	if (!IsMoveValid(board, index)) {
		return;
	}

	board->cells[index] = board->current_turn;
	board->state = CheckGameResult(board);

	if (board->state == GAME_PLAYING) {
		board->current_turn =
			(board->current_turn == PLAYER_X) ? PLAYER_O : PLAYER_X;
	}
}

GameResult CheckGameResult(const Board* board) {
	/**
	 * Checks if the game has ended and determines the result.
	 * Checks all 8 winning lines (3 rows, 3 columns, 2 diagonals).
	 * Returns:
	 *   - GAME_WIN_X if Player X has won
	 *   - GAME_WIN_O if Player O has won
	 *   - GAME_DRAW if board is full with no winner
	 *   - GAME_PLAYING if game is still in progress
	 */
	const int lines[8][3] = {
		{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, /* Rows */
		{0, 3, 6}, {1, 4, 7}, {2, 5, 8}, /* Columns */
		{0, 4, 8}, {2, 4, 6}			 /* Diagonals */
	};

	for (int i = 0; i < 8; i++) {
		Player p = board->cells[lines[i][0]];

		if (p != PLAYER_NONE && p == board->cells[lines[i][1]] &&
			p == board->cells[lines[i][2]]) {
			return (p == PLAYER_X) ? GAME_WIN_X : GAME_WIN_O;
		}
	}

	int empty_count = 0;

	for (int i = 0; i < 9; i++) {
		if (board->cells[i] == PLAYER_NONE) {
			empty_count++;
		}
	}

	if (empty_count == 0) {
		return GAME_DRAW;
	}

	return GAME_PLAYING;
}

/**
 * @file board.h
 * @brief Game board structures and functions for Tic-Tac-Toe.
 */

#ifndef BOARD_H
#define BOARD_H

/*============================================================================*/
/*                           ENUMERATIONS                                     */
/*============================================================================*/

/**
 * @brief Represents a player or empty cell on the board.
 */
typedef enum {
	PLAYER_NONE = 0, /**< Empty cell */
	PLAYER_X = 1,	 /**< Player X (human) */
	PLAYER_O = 2	 /**< Player O (AI) */
} Player;

/**
 * @brief Represents the current state of the game.
 */
typedef enum {
	GAME_PLAYING = 0, /**< Game is in progress */
	GAME_DRAW,		  /**< Game ended in a draw */
	GAME_WIN_X,		  /**< Player X won */
	GAME_WIN_O		  /**< Player O won */
} GameResult;

/*============================================================================*/
/*                           STRUCTURES                                       */
/*============================================================================*/

/**
 * @brief Represents the game board state.
 */
typedef struct {
	Player cells[9];	 /**< Array of 9 cells (3x3 board) */
	Player current_turn; /**< Which player's turn it is */
	GameResult state;	 /**< Current game state */
} Board;

/*============================================================================*/
/*                           FUNCTION PROTOTYPES                              */
/*============================================================================*/

/**
 * @brief Initializes a new game board.
 * @param board Pointer to the board to initialize.
 */
void InitBoard(Board* board);

/**
 * @brief Resets the board to its initial state.
 * @param board Pointer to the board to reset.
 */
void ResetBoard(Board* board);

/**
 * @brief Checks if a move is valid at the given index.
 * @param board Pointer to the current board state.
 * @param index The cell index to check (0-8).
 * @return 1 if the move is valid, 0 otherwise.
 */
int IsMoveValid(const Board* board, int index);

/**
 * @brief Makes a move at the given index for the current player.
 * @param board Pointer to the board to modify.
 * @param index The cell index where to place the move (0-8).
 */
void MakeMove(Board* board, int index);

/**
 * @brief Checks the current game result.
 * @param board Pointer to the board to check.
 * @return The current GameResult (GAME_PLAYING, GAME_DRAW, GAME_WIN_X,
 *         or GAME_WIN_O).
 */
GameResult CheckGameResult(const Board* board);

#endif /* BOARD_H */

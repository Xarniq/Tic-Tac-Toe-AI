/**
 * @file ai.c
 * @brief MCTS (Monte Carlo Tree Search) AI implementation for Tic-Tac-Toe.
 */

#include "ai.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================*/
/*                           GLOBAL VARIABLES                                 */
/*============================================================================*/

/**
 * @brief Global variable holding the current MCTS iteration count.
 */
int g_mcts_iterations = AI_LEVEL_MEDIUM;

/*============================================================================*/
/*                           LEVEL MANAGEMENT                                 */
/*============================================================================*/

void AI_SetLevel(AILevel level) {
	/**
	 * Sets the number of MCTS iterations based on difficulty level.
	 */
	g_mcts_iterations = (int)level;
}

AILevel AI_GetLevel(void) {
	/**
	 * Returns the current AI difficulty level.
	 */
	return (AILevel)g_mcts_iterations;
}

/*============================================================================*/
/*                           HELPER FUNCTIONS                                 */
/*============================================================================*/

int GetRandomMove(const Board* board) {
	/**
	 * Returns a random valid move, or -1 if none available.
	 */
	int valid_moves[9];
	int count = 0;

	for (int i = 0; i < 9; i++) {
		if (board->cells[i] == PLAYER_NONE) {
			valid_moves[count++] = i;
		}
	}

	if (count == 0) {
		return -1;
	}

	return valid_moves[rand() % count];
}

static int GetLegalMoves(const Board* board, int* moves) {
	/**
	 * Fills moves array with legal move indices. Returns count.
	 */
	int count = 0;

	for (int i = 0; i < 9; i++) {
		if (board->cells[i] == PLAYER_NONE) {
			moves[count++] = i;
		}
	}

	return count;
}

static int IsTerminal(const Board* board) {
	/**
	 * Returns 1 if game is over (win or draw), 0 otherwise.
	 */
	return board->state != GAME_PLAYING;
}

static Board CopyBoard(const Board* src) {
	/**
	 * Creates an independent deep copy of the board.
	 */
	Board copy;
	memcpy(&copy, src, sizeof(Board));
	return copy;
}

/*============================================================================*/
/*                           MCTS NODE OPERATIONS                             */
/*============================================================================*/

static Node* CreateMCTSNode(const Board* board, int move, Player who_moved,
							Node* parent) {
	/**
	 * Creates a new MCTS node storing the full board state.
	 */
	Node* node = (Node*)malloc(sizeof(Node));
	if (!node) return NULL;

	memcpy(node->cells, board->cells, sizeof(Player) * 9);
	node->move_index = move;
	node->player_who_moved = who_moved;
	node->visits = 0;
	node->score = 0.0;
	node->parent = parent;
	node->num_children = 0;
	node->possible_moves_left = -1;

	for (int i = 0; i < 9; i++) {
		node->children[i] = NULL;
	}

	return node;
}

static void FreeMCTSTree(Node* root) {
	/**
	 * Recursively frees the entire tree.
	 */
	if (!root) return;

	for (int i = 0; i < 9; i++) {
		if (root->children[i]) {
			FreeMCTSTree(root->children[i]);
		}
	}
	free(root);
}

/*============================================================================*/
/*                           MCTS CORE ALGORITHM                              */
/*============================================================================*/

static double CalculateUCB1(const Node* node, int parent_visits) {
	/**
	 * UCB1 = win_rate + C * sqrt(ln(N_parent) / N_child)
	 * For unvisited nodes, return infinity to prioritize exploration.
	 */
	if (node->visits == 0) {
		return 1e9;
	}

	double exploitation = node->score / (double)node->visits;
	double exploration = UCB_EXPLORATION *
		sqrt(log((double)parent_visits) / (double)node->visits);

	return exploitation + exploration;
}

static Node* Select(Node* node, Board* board) {
	/**
	 * SELECTION: Traverse from root to a leaf using UCB1.
	 * Stops at:
	 *   - A terminal node (game over)
	 *   - A node with unexpanded children
	 * Updates board state as we descend.
	 */
	while (!IsTerminal(board)) {
		int moves[9];
		int num_moves = GetLegalMoves(board, moves);

		if (num_moves == 0) {
			break;
		}

		/* Check if there are unexpanded moves */
		int has_unexpanded = 0;
		for (int i = 0; i < num_moves; i++) {
			if (node->children[moves[i]] == NULL) {
				has_unexpanded = 1;
				break;
			}
		}

		/* If there are unexpanded moves, stop here for expansion */
		if (has_unexpanded) {
			return node;
		}

		/* All children expanded: select best child via UCB1 */
		Node* best_child = NULL;
		double best_ucb = -1e9;
		int best_move = -1;

		for (int i = 0; i < num_moves; i++) {
			int move = moves[i];
			Node* child = node->children[move];
			if (child) {
				double ucb = CalculateUCB1(child, node->visits);
				if (ucb > best_ucb) {
					best_ucb = ucb;
					best_child = child;
					best_move = move;
				}
			}
		}

		if (!best_child) {
			break;
		}

		/* Descend to best child and update board */
		MakeMove(board, best_move);
		node = best_child;
	}

	return node;
}

static Node* Expand(Node* node, Board* board) {
	/**
	 * EXPANSION: Add ONE new child node for an unexpanded move.
	 * Returns the new child, or the node itself if terminal/fully expanded.
	 */
	if (IsTerminal(board)) {
		return node;
	}

	int moves[9];
	int num_moves = GetLegalMoves(board, moves);

	/* Find first unexpanded move */
	for (int i = 0; i < num_moves; i++) {
		int move = moves[i];
		if (node->children[move] == NULL) {
			/* Create child with the new state */
			Player who_plays = board->current_turn;
			Board child_board = CopyBoard(board);
			MakeMove(&child_board, move);

			Node* child = CreateMCTSNode(&child_board, move, who_plays, node);
			if (!child) return node;

			node->children[move] = child;
			node->num_children++;

			/* Update board to match child state */
			*board = child_board;

			return child;
		}
	}

	return node;
}

static GameResult Simulate(Board* board) {
	/**
	 * SIMULATION (Rollout): Play random moves until game ends.
	 * Uses a deep copy to avoid corrupting the original state.
	 */
	Board sim = CopyBoard(board);

	while (!IsTerminal(&sim)) {
		int moves[9];
		int count = GetLegalMoves(&sim, moves);

		if (count == 0) break;

		int random_move = moves[rand() % count];
		MakeMove(&sim, random_move);
	}

	return sim.state;
}

static void Backpropagate(Node* node, GameResult result) {
	/**
	 * BACKPROPAGATION: Update statistics from leaf to root.
	 *
	 * CRITICAL: Score is from the perspective of the player who made
	 * the move to reach this node (player_who_moved).
	 *
	 * If player_who_moved wins -> +1 for this node
	 * If player_who_moved loses -> -1 for this node
	 * Draw -> 0
	 */
	while (node != NULL) {
		node->visits++;

		/* Root node has no player_who_moved */
		if (node->player_who_moved != PLAYER_NONE) {
			if ((result == GAME_WIN_X && node->player_who_moved == PLAYER_X) ||
				(result == GAME_WIN_O && node->player_who_moved == PLAYER_O)) {
				/* The player who made this move won */
				node->score += 1.0;
			} else if ((result == GAME_WIN_X && node->player_who_moved == PLAYER_O) ||
					   (result == GAME_WIN_O && node->player_who_moved == PLAYER_X)) {
				/* The player who made this move lost */
				node->score -= 1.0;
			}
			/* Draw: score += 0 (no change) */
		}

		node = node->parent;
	}
}

static int MCTS_Search(const Board* root_state) {
	/**
	 * Main MCTS search loop.
	 * Returns the best move (most visited child of root).
	 */

	/* Create root node - no move led here, no player */
	Node* root = CreateMCTSNode(root_state, -1, PLAYER_NONE, NULL);
	if (!root) return GetRandomMove(root_state);

	for (int iter = 0; iter < g_mcts_iterations; iter++) {
		/* Start with fresh copy of root state */
		Board current_board = CopyBoard(root_state);
		Node* node = root;

		/* 1. SELECTION */
		node = Select(node, &current_board);

		/* 2. EXPANSION (if not terminal) */
		if (!IsTerminal(&current_board)) {
			node = Expand(node, &current_board);
		}

		/* 3. SIMULATION */
		GameResult result = Simulate(&current_board);

		/* 4. BACKPROPAGATION */
		Backpropagate(node, result);
	}

	/* Choose move with most visits (most robust) */
	int best_move = -1;
	int max_visits = -1;

	for (int i = 0; i < 9; i++) {
		if (root->children[i] != NULL) {
			Node* c = root->children[i];

			if (c->visits > max_visits) {
				max_visits = c->visits;
				best_move = i;
			}
		}
	}

	FreeMCTSTree(root);

	return best_move;
}

/*============================================================================*/
/*                           PUBLIC INTERFACE                                 */
/*============================================================================*/

int AI_ChooseMove(const Board* board) {
	/**
	 * Public interface for AI move selection using MCTS.
	 */
	if (board->state != GAME_PLAYING) {
		return -1;
	}

	int best_move = MCTS_Search(board);

	if (best_move < 0 || !IsMoveValid(board, best_move)) {
		return GetRandomMove(board);
	}

	return best_move;
}

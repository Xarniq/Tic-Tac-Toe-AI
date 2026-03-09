/**
 * @file tree.h
 * @brief MCTS tree structure and node management for Tic-Tac-Toe AI.
 */

#ifndef TREE_H
#define TREE_H

#include "board.h"

/*============================================================================*/
/*                           MCTS CONFIGURATION                               */
/*============================================================================*/

/**
 * @brief UCB1 exploration constant (sqrt(2)).
 *
 * Controls the balance between exploitation and exploration in the UCB1
 * formula. Higher values favor exploration of less-visited nodes.
 */
#define UCB_EXPLORATION 1.41

/**
 * @brief AI difficulty levels based on MCTS iteration count.
 *
 * Higher iteration counts result in stronger play but longer computation time.
 */
typedef enum {
	AI_LEVEL_EASY = 10,	 /**< Weak play */
	AI_LEVEL_MEDIUM = 100,  /**< Moderate strength */
	AI_LEVEL_HARD = 5000	 /**< Near-optimal play, slower computation */
} AILevel;

/**
 * @brief Global variable holding the current AI difficulty level.
 *
 * Defaults to AI_LEVEL_MEDIUM. Can be changed via AI_SetLevel().
 */
extern int g_mcts_iterations;

/*============================================================================*/
/*                             NODE STRUCTURE                                 */
/*============================================================================*/

/**
 * @brief Represents a node in the MCTS game tree.
 *
 * Each node stores the board state, visit statistics, and links to parent
 * and child nodes. Used during the MCTS search to track simulation results.
 */
typedef struct Node {
	Player cells[9]; /**< Board state at this node */
	int move_index;	 /**< Move played to reach this node (0-8, -1 for root) */
	Player player_who_moved; /**< Player who made the move to reach this node */

	int visits;	  /**< Number of times this node was visited (N) */
	double score; /**< Cumulative score from AI's perspective (W) */

	struct Node* parent;	  /**< Pointer to the parent node */
	struct Node* children[9]; /**< Child nodes indexed by move (0-8) */
	int num_children;		  /**< Current number of expanded children */
	int possible_moves_left;  /**< Unexpanded moves count (-1 = not computed) */
} Node;

/*============================================================================*/
/*                           FUNCTION PROTOTYPES                              */
/*============================================================================*/

/**
 * @brief Creates a new tree node with the given board state.
 * @param cells Array of 9 Player values representing the board.
 * @return Pointer to the newly allocated node, or NULL on failure.
 */
Node* CreateNode(const Player cells[9]);

/**
 * @brief Creates a new MCTS node with full context.
 * @param cells Array of 9 Player values representing the board.
 * @param move The move index that led to this node (0-8, -1 for root).
 * @param player The player who made the move.
 * @param parent Pointer to the parent node (NULL for root).
 * @return Pointer to the newly allocated node, or NULL on failure.
 */
Node* CreateNodeMCTS(const Player cells[9], int move, Player player,
					 struct Node* parent);

/**
 * @brief Recursively frees all nodes in a tree.
 * @param root The root node of the tree to free.
 */
void FreeTree(Node* root);

/**
 * @brief Gets the child node for a specific move.
 * @param parent The parent node.
 * @param move The move index (0-8).
 * @return Pointer to the child node, or NULL if not exists.
 */
Node* GetChild(Node* parent, int move);

/**
 * @brief Adds a child node for a specific move.
 * @param parent The parent node.
 * @param move The move index (0-8).
 * @param cells The board state after the move.
 * @return Pointer to the new or existing child node.
 */
Node* AddChild(Node* parent, int move, const Player cells[9]);

/**
 * @brief Adds a child node with full MCTS context.
 * @param parent The parent node.
 * @param move The move index (0-8).
 * @param cells The board state after the move.
 * @param player The player who made the move.
 * @return Pointer to the new or existing child node.
 */
Node* AddChildMCTS(Node* parent, int move, const Player cells[9],
				   Player player);

#endif /* TREE_H */

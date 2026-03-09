/**
 * @file tree.c
 * @brief MCTS tree node management implementation.
 */

#include "tree.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================*/
/*                           NODE CREATION                                    */
/*============================================================================*/

Node* CreateNode(const Player cells[9]) {
	/**
	 * Allocates and initializes a new tree node with default values.
	 * Copies the board state into the node and sets all children to NULL.
	 * Returns NULL if memory allocation fails.
	 */
	Node* node = (Node*)malloc(sizeof(Node));

	if (node) {
		memcpy(node->cells, cells, sizeof(Player) * 9);
		node->move_index = -1;
		node->player_who_moved = PLAYER_NONE;
		node->visits = 0;
		node->score = 0.0;
		node->parent = NULL;
		node->num_children = 0;
		node->possible_moves_left = -1;

		for (int i = 0; i < 9; i++) {
			node->children[i] = NULL;
		}
	}

	return node;
}

Node* CreateNodeMCTS(const Player cells[9], int move, Player player,
					 Node* parent) {
	/**
	 * Allocates and initializes a new MCTS node with full context.
	 * Sets the move that led to this node, the player who made it,
	 * and links to the parent node.
	 * Returns NULL if memory allocation fails.
	 */
	Node* node = (Node*)malloc(sizeof(Node));

	if (node) {
		memcpy(node->cells, cells, sizeof(Player) * 9);
		node->move_index = move;
		node->player_who_moved = player;
		node->visits = 0;
		node->score = 0.0;
		node->parent = parent;
		node->num_children = 0;
		node->possible_moves_left = -1;

		for (int i = 0; i < 9; i++) {
			node->children[i] = NULL;
		}
	}

	return node;
}

/*============================================================================*/
/*                           TREE MANAGEMENT                                  */
/*============================================================================*/

void FreeTree(Node* root) {
	/**
	 * Recursively frees all nodes in the tree starting from root.
	 * Traverses all children and frees them before freeing the parent.
	 * Safe to call with NULL pointer.
	 */
	if (!root) {
		return;
	}

	for (int i = 0; i < 9; i++) {
		if (root->children[i]) {
			FreeTree(root->children[i]);
		}
	}

	free(root);
}

/*============================================================================*/
/*                           CHILD MANAGEMENT                                 */
/*============================================================================*/

Node* GetChild(Node* parent, int move) {
	/**
	 * Returns the child node corresponding to the given move index.
	 * Returns NULL if parent is NULL, move is out of range, or
	 * the child does not exist.
	 */
	if (!parent || move < 0 || move >= 9) {
		return NULL;
	}

	return parent->children[move];
}

Node* AddChild(Node* parent, int move, const Player cells[9]) {
	/**
	 * Adds a new child node at the specified move index.
	 * If a child already exists at that index, returns the existing child.
	 * Sets up parent-child relationship and increments child count.
	 * Returns NULL on invalid input or allocation failure.
	 */
	if (!parent || move < 0 || move >= 9) {
		return NULL;
	}

	if (parent->children[move]) {
		return parent->children[move];
	}

	Node* child = CreateNode(cells);

	if (child) {
		child->parent = parent;
		child->move_index = move;
		parent->children[move] = child;
		parent->num_children++;
	}

	return child;
}

Node* AddChildMCTS(Node* parent, int move, const Player cells[9],
				   Player player) {
	/**
	 * Adds a new MCTS child node with full context information.
	 * If a child already exists at that index, returns the existing child.
	 * Records which player made the move that led to this state.
	 * Returns NULL on invalid input or allocation failure.
	 */
	if (!parent || move < 0 || move >= 9) {
		return NULL;
	}

	if (parent->children[move]) {
		return parent->children[move];
	}

	Node* child = CreateNodeMCTS(cells, move, player, parent);

	if (child) {
		parent->children[move] = child;
		parent->num_children++;
	}

	return child;
}

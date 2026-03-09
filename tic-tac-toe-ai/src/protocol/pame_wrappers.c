#include "pame_wrappers.h"
#include <arpa/inet.h>
#include <string.h>

/**
 * @brief Send a general acknowledgment message.
 *
 * Construct and send an PAME general ACK packet containing a single
 * acknowledgment byte.
 *
 * @param fd   File descriptor (socket) to send the packet on.
 * @param ok   ACK value (e.g., PAME_MESSAGE_ACK_OK or PAME_MESSAGE_ACK_NOK).
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 *
 * @note The function wraps the ACK value in an PAME_PayloadGeneralACK and
 *       sends it using PAME_MSG_GENERAL_ACK message type.
 */
int pame_helper_send_ack(int fd, uint8_t ok) {
	PAME_PayloadGeneralACK p = {.ack_type = ok};
	return pame_send_packet(fd, PAME_MSG_GENERAL_ACK, &p, 1);
}

/**
 * @brief Send a connect request with a username.
 *
 * Populate an PAME_PayloadConnect structure with the provided username
 * (NUL-terminated/truncated to fit) and send an PAME_MSG_CONNECT packet.
 *
 * @param fd        File descriptor (socket) to send the packet on.
 * @param username  Pointer to a NUL-terminated username string. If NULL,
 *                  an empty username is sent.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 *
 * @note The username is copied using strncpy and the payload is zeroed
 *       beforehand to ensure any unused bytes are NULs.
 */
int pame_helper_send_connect(int fd, const char* username) {
	PAME_PayloadConnect p;
	memset(&p, 0, sizeof p);
	if (username) {
		strncpy((char*)p.username, username, sizeof p.username - 1);
	}
	return pame_send_packet(fd, PAME_MSG_CONNECT, &p, sizeof p);
}

/**
 * @brief Send a game-join request.
 *
 * Send an PAME_MSG_GAME_JOIN packet. The packet has no payload.
 *
 * @param fd  File descriptor (socket) to send the packet on.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_game_join(int fd) {
	// Payload vide (si struct vide)
	return pame_send_packet(fd, PAME_MSG_GAME_JOIN, NULL, 0);
}

/**
 * @brief Notify the peer that the sender is waiting for a game.
 *
 * Send an PAME_MSG_GAME_WAIT packet. The packet has no payload.
 *
 * @param fd  File descriptor (socket) to send the packet on.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_game_wait(int fd) {
	return pame_send_packet(fd, PAME_MSG_GAME_WAIT, NULL, 0);
}

/**
 * @brief Notify peers that a game is starting or stopping.
 *
 * Send an PAME_MSG_GAME_START packet containing a single byte indicating
 * whether the game is starting.
 *
 * @param fd           File descriptor (socket) to send the packet on.
 * @param is_starting  Non-zero if the game is starting, zero otherwise.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_game_start(int fd, uint8_t is_starting) {
	PAME_PayloadGameStart p = {.is_starting = is_starting};
	return pame_send_packet(fd, PAME_MSG_GAME_START, &p, 1);
}

/**
 * @brief Inform the peer whether it is their turn to play.
 *
 * Send an PAME_MSG_PLAY packet containing a single byte indicating the
 * play type / whether it is the recipient's turn.
 *
 * @param fd         File descriptor (socket) to send the packet on.
 * @param your_turn  Value indicating play type / turn ownership.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_play(int fd, uint8_t your_turn) {
	PAME_PayloadPlay p = {.play_type = your_turn};
	return pame_send_packet(fd, PAME_MSG_PLAY, &p, 1);
}

/**
 * @brief Send an illegal-move notification.
 *
 * Send an PAME_MSG_ILLEGAL_PLAY packet with a general NOK ACK payload to
 * indicate that the last attempted move was illegal.
 *
 * @param fd  File descriptor (socket) to send the packet on.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_illegal(int fd) {
	PAME_PayloadGeneralACK p = {.ack_type = PAME_MESSAGE_ACK_NOK};
	return pame_send_packet(fd, PAME_MSG_ILLEGAL_PLAY, &p, 1);
}

/**
 * @brief Send a move action describing a Tic-Tac-Toe placement.
 *
 * Build and send an PAME_MSG_MOVE packet containing the selected board cell.
 *
 * @param fd          File descriptor (socket) to send the packet on.
 * @param cell_index  Target cell index (0-8).
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_move(int fd, uint8_t cell_index) {
	PAME_PayloadMove m = {.cell_index = cell_index};
	return pame_send_packet(fd, PAME_MSG_MOVE, &m, sizeof m);
}

/**
 * @brief Send an arbitrary game state payload.
 *
 * Forward the provided raw payload as the payload of an PAME_MSG_GAME_STATE
 * packet. This function does not interpret or validate the payload contents.
 *
 * @param fd     File descriptor (socket) to send the packet on.
 * @param payload Pointer to the raw payload data to send (may be NULL if
 * len==0).
 * @param len     Length in bytes of payload.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 *
 * @note The caller is responsible for ensuring the payload format and length
 *       conform to the PAME game-state specification.
 */
int pame_helper_send_game_state(int fd, const void* payload, uint16_t len) {
	return pame_send_packet(fd, PAME_MSG_GAME_STATE, payload, len);
}

/**
 * @brief Notify peers about the end of a game and whether the sender won.
 *
 * Send an PAME_MSG_GAME_END packet containing a single byte indicating
 * whether the sender has won the game.
 *
 * @param fd      File descriptor (socket) to send the packet on.
 * @param result  Non-zero if the sender won, zero otherwise.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_game_end(int fd, uint8_t result) {
	PAME_PayloadGameEnd p = {.has_won = result};
	return pame_send_packet(fd, PAME_MSG_GAME_END, &p, 1);
}

/**
 * @brief Send AI difficulty setting to the server.
 *
 * Send an PAME_MSG_SET_DIFFICULTY packet containing a single byte
 * indicating the desired difficulty level.
 *
 * @param fd    File descriptor (socket) to send the packet on.
 * @param level Difficulty level: 1=Easy, 2=Medium, 3=Hard.
 *
 * @return The value returned by pame_send_packet (propagates success/error).
 */
int pame_helper_send_difficulty(int fd, uint8_t level) {
	PAME_PayloadSetDifficulty p = {.level = level};
	return pame_send_packet(fd, PAME_MSG_SET_DIFFICULTY, &p, sizeof p);
}

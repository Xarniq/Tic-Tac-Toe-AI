/**
 * @file wrappers.h
 * @brief High-level helper functions to build and send PAME protocol messages.
 *
 * These wrappers provide convenient routines to serialize and transmit PAME
 * messages over a connected file descriptor (socket or pipe). They hide the
 * details of message framing and payload encoding used by the PAME high-level
 * layer.
 *
 * General behavior:
 * - All functions send a single PAME message corresponding to their purpose.
 * - On success the function returns a non-negative value (typically 0). On
 *   failure they return a negative value.
 * - Caller is responsible for providing a valid, connected file descriptor.
 * - Constants referenced in parameters (e.g. PAME_MESSAGE_ACK_OK,
 *   PAME_PLAY_YOUR_TURN, PAME_END_GAME_WIN) are defined in pame_high_level.h.
 */

#pragma once
#include <stdint.h>
#include "pame_high_level.h"

/**
 * @brief Send an ACK/NACK message.
 *
 * @param fd    File descriptor to send the message on (e.g. socket).
 * @param ok    Acknowledgement value: PAME_MESSAGE_ACK_OK or PAME_MESSAGE_ACK_NOK.
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_ack(int fd, uint8_t ok); // ok = PAME_MESSAGE_ACK_OK / PAME_MESSAGE_ACK_NOK

/**
 * @brief Send a connect request with a username.
 *
 * This message is used to announce or register the client identity to the peer.
 *
 * @param fd        File descriptor to send the message on.
 * @param username  NULL-terminated username string. Must not be NULL.
 *
 * @return Non-negative on success, negative on error.
 */

int pame_helper_send_connect(int fd, const char *username);

/**
 * @brief Notify the peer that the client is joining a game.
 *
 * Sends a game-join request/message; typically used to indicate the client wants
 * to participate in matchmaking or join a lobby.
 *
 * @param fd    File descriptor to send the message on.
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_game_join(int fd);

/**
 * @brief Notify the peer that the client is waiting for a game.
 *
 * Use this to indicate a waiting/lobby state.
 *
 * @param fd    File descriptor to send the message on.
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_game_wait(int fd);

/**
 * @brief Send a game start notification.
 *
 * Informs the peer whether the game is starting and (optionally) which side or
 * turn state applies.
 *
 * @param fd           File descriptor to send the message on.
 * @param is_starting  PAME_PLAY_YOUR_TURN if this client starts / prepares to play,
 *                     otherwise NOT_YOUR_TURN (or similar defined constant).
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_game_start(int fd, uint8_t is_starting);        // PAME_PLAY_YOUR_TURN / NOT_YOUR_TURN

/**
 * @brief Inform the peer about play/turn state.
 *
 * Use to indicate whether it is the recipient's turn or the sender's turn.
 *
 * @param fd         File descriptor to send the message on.
 * @param your_turn  PAME_PLAY_YOUR_TURN if it is the recipient's turn, otherwise
 *                   NOT_YOUR_TURN (or the appropriate constant).
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_play(int fd, uint8_t your_turn);                // PAME_PLAY_YOUR_TURN / NOT_YOUR_TURN

/**
 * @brief Send an illegal-move or protocol-illegal notification.
 *
 * Indicates that an illegal action or protocol violation occurred.
 *
 * @param fd    File descriptor to send the message on.
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_illegal(int fd);

/**
 * @brief Send a move action.
 *
 * Encodes and transmits the cell index selected by the local player.
 *
 * @param fd          File descriptor to send the message on.
 * @param cell_index  Target cell index (0-8).
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_move(int fd, uint8_t cell_index);

/**
 * @brief Send current game state.
 *
 * Transmits an arbitrary game-state payload. If there is no state to send,
 * the caller may pass payload = NULL and len = 0.
 *
 * @param fd     File descriptor to send the message on.
 * @param payload Pointer to state data to send. May be NULL when len == 0.
 * @param len     Length in bytes of the payload.
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_game_state(int fd, const void *payload, uint16_t len); // si état vide: payload=NULL,len=0

/**
 * @brief Send game end/result notification.
 *
 * Informs the peer of the game outcome (win/lose/tie etc.) using defined
 * PAME end-game constants.
 *
 * @param fd      File descriptor to send the message on.
 * @param result  Result code (e.g. PAME_END_GAME_WIN or PAME_END_GAME_LOSE).
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_game_end(int fd, uint8_t result);               // PAME_END_GAME_WIN / LOSE

/**
 * @brief Send AI difficulty setting.
 *
 * Transmits the desired AI difficulty level to the server.
 *
 * @param fd    File descriptor to send the message on.
 * @param level Difficulty level: 1=Easy, 2=Medium, 3=Hard.
 *
 * @return Non-negative on success, negative on error.
 */
int pame_helper_send_difficulty(int fd, uint8_t level);
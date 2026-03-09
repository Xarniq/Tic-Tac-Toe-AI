#ifndef PAME_MESSAGE_H
#define PAME_MESSAGE_H

#include <stdint.h>
#include "pame_protocol.h"

/**
 * @file pame_message.h
 * @brief High-level container for decoded PAME messages.
 */

/**
 * @brief Decoded PAME message routed to the gameplay layer.
 */
typedef struct PAME_Message {
    PAME_MessageType type; /**< One of @ref PAME_MessageType describing @ref data. */

    /**
     * @brief Variant view of the message payload.
     */
    union {
        PAME_PayloadGeneralACK general_ack; /**< Payload for @ref PAME_MSG_GENERAL_ACK. */
        PAME_PayloadConnect connect;        /**< Payload for @ref PAME_MSG_CONNECT. */
        PAME_PayloadGameJoin game_join;     /**< Payload for @ref PAME_MSG_GAME_JOIN. */
        PAME_PayloadGameStart game_start;   /**< Payload for @ref PAME_MSG_GAME_START. */
        PAME_PayloadGameEnd game_end;       /**< Payload for @ref PAME_MSG_GAME_END. */
        PAME_PayloadGameWait game_wait;     /**< Payload for @ref PAME_MSG_GAME_WAIT. */
        PAME_PayloadGameState game_state;   /**< Payload for @ref PAME_MSG_GAME_STATE. */
        PAME_PayloadMove move;              /**< Payload for @ref PAME_MSG_MOVE. */
        PAME_PayloadPlay play;              /**< Payload for @ref PAME_MSG_PLAY. */
        PAME_PayloadIllegalPlay illegal_play; /**< Payload for @ref PAME_MSG_ILLEGAL_PLAY. */
        PAME_PayloadSetDifficulty set_difficulty; /**< Payload for @ref PAME_MSG_SET_DIFFICULTY. */
    } data;

} PAME_Message;

#endif /* PAME_MESSAGE_H */

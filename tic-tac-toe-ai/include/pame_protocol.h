#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "pame_constants.h"

#define PAME_BOARD_CELL_COUNT 9

typedef enum {
    PAME_MESSAGE_ACK_OK = 0x00,
    PAME_MESSAGE_ACK_NOK = 0x01
} PAME_AckType;

typedef enum {
    PAME_PLAY_NOT_YOUR_TURN = 0x00,
    PAME_PLAY_YOUR_TURN = 0x01
} PAME_PlayType;

typedef enum {
    PAME_END_GAME_NONE = 0x00,
    PAME_END_GAME_WIN = 0x01,
    PAME_END_GAME_LOSE = 0x02,
    PAME_END_GAME_DRAW = 0x03
} PAME_EndGameType;

typedef enum {
    PAME_MSG_GENERAL_ACK = 0x01, /**< Generic acknowledgement. */
    PAME_MSG_CONNECT = 0x02,     /**< Initial client handshake carrying identity. */
    PAME_MSG_GAME_JOIN = 0x03,   /**< Join matchmaking queue or lobby. */
    PAME_MSG_GAME_START = 0x04,  /**< Game start notification. */
    PAME_MSG_GAME_END = 0x05,    /**< Game end notification. */
    PAME_MSG_GAME_WAIT = 0x06,   /**< Waiting state prior to match start. */
    PAME_MSG_GAME_STATE = 0x07,  /**< Serialized snapshot of the match state. */
    PAME_MSG_MOVE = 0x08,        /**< Player move command. */
    PAME_MSG_PLAY = 0x09,        /**< Turn notification. */
    PAME_MSG_ILLEGAL_PLAY = 0x0A,/**< Invalid move notification. */
    PAME_MSG_SET_DIFFICULTY = 0x0B, /**< AI difficulty setting (1=Easy, 2=Medium, 3=Hard). */
} PAME_MessageType;

#pragma pack(push, 1)
typedef struct {
    uint8_t magic_byte[2]; /**< Synchronization bytes: @ref PAME_MAGIC_BYTE_0, @ref PAME_MAGIC_BYTE_1. */
    uint8_t version;       /**< Protocol version, see @ref PAME_PROTO_VERSION. */
    uint8_t code;          /**< Message code, one of @ref PAME_MessageType. */
    uint16_t size;         /**< Payload size in bytes (network byte order). */
} PAME_PacketHeader;

/**
 * @brief Generic one-byte payload used for tests and very small messages.
 */
typedef struct {
    uint8_t payload; /**< Single-byte payload. */
} PAME_GeneralPacket;

/**
 * @brief Payload for @ref PAME_MSG_GENERAL_ACK.
 */
typedef struct {
    uint8_t ack_type; /**< Value from @ref PAME_GeneralACKTypes. */
} PAME_PayloadGeneralACK;

/**
 * @brief Payload for @ref PAME_MSG_CONNECT.
 */
typedef struct {
    uint8_t username[MAX_PLAYER_USERNAME_SIZE]; /**< Raw username (not null-terminated on the wire). */
} PAME_PayloadConnect;

/**
 * @brief Payload for @ref PAME_MSG_GAME_JOIN.
 */
typedef struct {
    /* Intentionally empty; keep payload-less semantic. */
} PAME_PayloadGameJoin;

/**
 * @brief Payload for @ref PAME_MSG_GAME_START.
 */
typedef struct {
    uint8_t is_starting; /**< Value from @ref PAME_PlayType indicating turn order. */
} PAME_PayloadGameStart;

/**
 * @brief Payload for @ref PAME_MSG_GAME_END.
 */
typedef struct {
    uint8_t has_won; /**< Value from @ref PAME_EndGameType describing the outcome. */
} PAME_PayloadGameEnd;

/**
 * @brief Payload for @ref PAME_MSG_GAME_WAIT.
 */
typedef struct {
    /* Intentionally empty; keep payload-less semantic. */
} PAME_PayloadGameWait;

/**
 * @brief Payload for @ref PAME_MSG_GAME_STATE.
 */
typedef struct {
    uint8_t board_cells[PAME_BOARD_CELL_COUNT]; /**< Flattened 3x3 board encoded with @ref Player values. */
    uint8_t current_turn; /**< Active player encoded as @ref Player. */
    uint8_t state; /**< Current @ref GameResult value. */
} PAME_PayloadGameState;

/**
 * @brief Payload for @ref PAME_MSG_MOVE.
 */
typedef struct {
    uint8_t cell_index; /**< Selected board cell (0-8). */
} PAME_PayloadMove;

/**
 * @brief Payload for @ref PAME_MSG_PLAY.
 */
typedef struct {
    uint8_t play_type; /**< Value from @ref PAME_PlayType indicating turn ownership. */
} PAME_PayloadPlay;

/**
 * @brief Payload for @ref PAME_MSG_ILLEGAL_PLAY
 */
typedef struct {
    /* Intentionally empty; */
} PAME_PayloadIllegalPlay;

/**
 * @brief Payload for @ref PAME_MSG_SET_DIFFICULTY
 */
typedef struct {
    uint8_t level; /**< Difficulty level: 1=Easy, 2=Medium, 3=Hard */
} PAME_PayloadSetDifficulty;

#pragma pack(pop)

#endif
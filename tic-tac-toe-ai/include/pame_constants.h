#ifndef PAME_CONSTANTS_H
#define PAME_CONSTANTS_H

/**
 * @file constants.h
 * @brief Compile-time constants shared by the PAME protocol implementation.
 */

/**
 * @brief Name of the transport protocol used with @c getprotobyname().
 */
#define PAME_BASE_TRANSPORT_PROTOCOL "TCP"

/**
 * @brief Maximum payload size accepted by the protocol (bytes).
 */
#define PAME_MAX_PAYLOAD_SIZE 1024

/**
 * @brief Maximum number of characters stored for a username on the wire.
 */
#define MAX_PLAYER_USERNAME_SIZE 16

/**
 * @brief First synchronization byte that prefixes all PAME packets.
 */
#define PAME_MAGIC_BYTE_0 'E'

/**
 * @brief Second synchronization byte that prefixes all PAME packets.
 */
#define PAME_MAGIC_BYTE_1 'M'

/**
 * @brief Current protocol version encoded in PAME packet headers.
 */
#define PAME_PROTO_VERSION 1

#endif
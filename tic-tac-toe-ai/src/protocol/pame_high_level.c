#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "pame_constants.h"
#include "pame_low_level.h"
#include "pame_message.h"

int pame_send_packet(int sock, uint8_t msg_type, const void* payload,
					 uint16_t payload_len) {
	if (payload_len > 0 && payload == NULL) {
		return -1;
	}

	/* Craft packet header */
	PAME_PacketHeader packet_header;
	packet_header.version = PAME_PROTO_VERSION;
	packet_header.magic_byte[0] = PAME_MAGIC_BYTE_0;
	packet_header.magic_byte[1] = PAME_MAGIC_BYTE_1;
	packet_header.code = msg_type;
	packet_header.size = htons(payload_len); /* htonl if uint32_t */

	/* Send header */
	if (pame_send_all(sock, &packet_header, sizeof(packet_header)) <= 0) {
		return -1;
	}

	/* Send payload */
	if (payload_len > 0 && payload != NULL) {
		if (pame_send_all(sock, payload, payload_len) <= 0) {
			return -1;
		}
	}

	return 0;
}

int pame_recv_packet(int sock, uint8_t* type_out, void** payload_out,
					 uint16_t* payload_len_out) {

	/* Packet header struct to store incoming header data */
	PAME_PacketHeader in_packet_header;

	/* Receive all from the socket, blocking call ! */
	ssize_t con_state =
		pame_recv_all(sock, &in_packet_header, sizeof(in_packet_header));
	if (con_state <= 0) {
		return -1;
	}

	/* Magic bytes matching test */
	if (in_packet_header.magic_byte[0] != PAME_MAGIC_BYTE_0 ||
		in_packet_header.magic_byte[1] != PAME_MAGIC_BYTE_1) {
		return -1;
	}
	/* Version test */
	if (in_packet_header.version != PAME_PROTO_VERSION) {
		return -1;
	}

	/* Big/Little-endian shenanigans */
	uint16_t payload_len = ntohs(in_packet_header.size); /* ntohl if uint32_t */

	if (payload_len > PAME_MAX_PAYLOAD_SIZE) {
		return -1;
	}

	*type_out = in_packet_header.code;
	*payload_len_out = payload_len;
	*payload_out = NULL;

	/* By default no payload */
	uint8_t* payload = NULL;
	if (payload_len > 0) {
		payload = malloc(payload_len);
		if (!payload) {
			return -1;
		}

		con_state = pame_recv_all(sock, payload, payload_len);
		if (con_state <= 0) {
			free(payload);
			return -1;
		}

		*payload_out = payload;
	}
	return 0;
}

int pame_decode_msg(uint8_t msg_type_raw, const void* payload,
					uint16_t payload_len, PAME_Message* out) {

	const uint8_t* p = payload;
	PAME_MessageType type = (PAME_MessageType)msg_type_raw;

	out->type = type;

	/* Match header code to the payload struct  */
	switch (type) {
	case PAME_MSG_GENERAL_ACK: {
		/* Payload: 1 byte -> ack_type (0/1) */
		if (payload_len != sizeof(PAME_PayloadGeneralACK)) {
			return -1;
		}
		out->data.general_ack.ack_type = p[0]; /* 0 or 1 */
		break;
	}

	case PAME_MSG_CONNECT: {
		/* Payload: fixed-size username buffer */
		if (payload_len != sizeof(PAME_PayloadConnect)) {
			return -1;
		}
		memcpy(out->data.connect.username, p, MAX_PLAYER_USERNAME_SIZE);
		break;
	}

	case PAME_MSG_MOVE: {
		/* Payload: 1 byte: selected cell index */
		if (payload_len != sizeof(PAME_PayloadMove)) {
			return -1;
		}
		out->data.move.cell_index = p[0];
		break;
	}

	case PAME_MSG_PLAY: {
		/* Payload: 1 byte: PAME_PlayType (0/1) */
		if (payload_len != sizeof(PAME_PayloadPlay)) {
			return -1;
		}
		out->data.play.play_type = p[0];
		break;
	}

	case PAME_MSG_GAME_START: {
		/* Payload: 1 byte is_starting (0/1) */
		if (payload_len != sizeof(PAME_PayloadGameStart)) {
			return -1;
		}
		out->data.game_start.is_starting = p[0]; /* PAME_PlayType logically */
		break;
	}

	case PAME_MSG_GAME_END: {
		/* Payload: 1 byte has_won (0/1) */
		if (payload_len != sizeof(PAME_PayloadGameEnd)) {
			return -1;
		}
		out->data.game_end.has_won = p[0]; /* PAME_EndGameType logically */
		break;
	}

	case PAME_MSG_GAME_WAIT: {
		/* No payload expected */
		if (payload_len != 0) {
			return -1;
		}
		/* Nothing to fill */
		break;
	}

	case PAME_MSG_GAME_STATE: {
		if (payload_len != sizeof(PAME_PayloadGameState)) {
			return -1;
		}
		memcpy(&out->data.game_state, payload, sizeof(PAME_PayloadGameState));
		break;
	}

	case PAME_MSG_GAME_JOIN: {
		/* No payload */
		if (payload_len != sizeof(PAME_PayloadGameJoin)) {
			return -1;
		}
		break;
	}

	case PAME_MSG_ILLEGAL_PLAY: {

		/* Define when you have a payload format */
		if (payload_len != sizeof(PAME_PayloadIllegalPlay)) {
			return -1; /* or handle properly */
		}
		break;
	}

	case PAME_MSG_SET_DIFFICULTY: {
		/* Payload: 1 byte level (1/2/3) */
		if (payload_len != sizeof(PAME_PayloadSetDifficulty)) {
			return -1;
		}
		out->data.set_difficulty.level = p[0];
		break;
	}

	default:
		/* Unknown type */
		return -1;
	}

	return 0;
}

int pame_encode_msg(PAME_MessageType msg_type, PAME_Message* message,
					void** out, uint16_t* out_len) {
	if (!message || !out || !out_len) {
		return -1;
	}

	uint8_t* buffer = NULL;
	uint16_t buffer_len = 0;

	/* Match message type to encode the payload struct */
	switch (msg_type) {
	case PAME_MSG_GENERAL_ACK: {
		/* Payload: 1 byte -> ack_type (0/1) */
		buffer_len = sizeof(PAME_PayloadGeneralACK);
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		buffer[0] = message->data.general_ack.ack_type;
		break;
	}

	case PAME_MSG_MOVE: {
		buffer_len = sizeof(PAME_PayloadMove);
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		buffer[0] = message->data.move.cell_index;
		break;
	}

	case PAME_MSG_PLAY: {
		buffer_len = sizeof(PAME_PayloadPlay);
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		buffer[0] = message->data.play.play_type;
		break;
	}

	case PAME_MSG_CONNECT: {
		/* Payload: username is fixed-size MAX_PLAYER_USERNAME_SIZE bytes */
		buffer_len = MAX_PLAYER_USERNAME_SIZE;
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		memcpy(buffer, message->data.connect.username,
			   MAX_PLAYER_USERNAME_SIZE);
		break;
	}

	case PAME_MSG_GAME_START: {
		/* Payload: 1 byte is_starting (0/1) */
		buffer_len = sizeof(PAME_PayloadGameStart);
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		buffer[0] = message->data.game_start.is_starting;
		break;
	}

	case PAME_MSG_GAME_END: {
		/* Payload: 1 byte has_won (0/1) */
		buffer_len = sizeof(PAME_PayloadGameEnd);
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		buffer[0] = message->data.game_end.has_won;
		break;
	}

	case PAME_MSG_GAME_WAIT: {
		/* No payload */
		buffer_len = 0;
		buffer = NULL;
		break;
	}

	case PAME_MSG_GAME_STATE: {
		buffer_len = sizeof(PAME_PayloadGameState);
		buffer = malloc(buffer_len);
		if (!buffer) {
			return -1;
		}
		memcpy(buffer, &message->data.game_state,
			   sizeof(PAME_PayloadGameState));
		break;
	}

	case PAME_MSG_GAME_JOIN: {
		/* No payload */
		buffer_len = 0;
		buffer = NULL;
		break;
	}

	case PAME_MSG_ILLEGAL_PLAY: {
		buffer_len = 0;
		buffer = NULL;
		break;
	}

	default:
		/* Unknown type */
		return -1;
	}

	*out = buffer;
	*out_len = buffer_len;
	return 0;
}

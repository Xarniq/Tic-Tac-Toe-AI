#include "network.h"
#include "pame_wrappers.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static void net_ignore_sigpipe_once(void) {
	static int done = 0;
	if (!done) {
		signal(SIGPIPE, SIG_IGN);
		done = 1;
	}
}

static void net_mark_disconnected(NetworkClient* client, const char* reason) {
	if (!client || !client->is_connected)
		return;
	if (reason)
		fprintf(stderr, "[client] disconnect: %s\n", reason);
	close(client->socket);
	client->is_connected = 0;
}

static int net_expect_ack(int socket_fd) {
	uint8_t msg_type = 0;
	void* payload = NULL;
	uint16_t payload_len = 0;

	if (pame_recv_packet(socket_fd, &msg_type, &payload, &payload_len) < 0) {
		free(payload);
		return -1;
	}

	PAME_Message ack_msg;
	if (pame_decode_msg(msg_type, payload, payload_len, &ack_msg) < 0) {
		free(payload);
		return -1;
	}
	free(payload);

	if (ack_msg.type != PAME_MSG_GENERAL_ACK) {
		return -1;
	}

	if (ack_msg.data.general_ack.ack_type != PAME_MESSAGE_ACK_OK) {
		return -1;
	}

	return 0;
}

static int net_receive_message(int socket_fd, PAME_Message* out_message) {
	uint8_t msg_type = 0;
	void* payload = NULL;
	uint16_t payload_len = 0;

	if (pame_recv_packet(socket_fd, &msg_type, &payload, &payload_len) < 0) {
		free(payload);
		return -1;
	}

	if (pame_decode_msg(msg_type, payload, payload_len, out_message) < 0) {
		free(payload);
		return -1;
	}

	free(payload);
	return 0;
}

int Net_Connect(NetworkClient* client, const char* ip, int port) {
	struct sockaddr_in serv_addr;
	memset(client, 0, sizeof(*client));
	net_ignore_sigpipe_once();

	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket < 0) {
		fprintf(stderr, "[client] socket() failed: %s\n", strerror(errno));
		return 0;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		fprintf(stderr, "[client] inet_pton failed for %s\n", ip);
		close(client->socket);
		return 0;
	}

	if (connect(client->socket, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)) < 0) {
		fprintf(stderr, "[client] connect failed: %s\n", strerror(errno));
		close(client->socket);
		return 0;
	}

	client->is_connected = 1;
	fprintf(stderr, "[client] connected to %s:%d\n", ip, port);
	return 1;
}

void Net_Disconnect(NetworkClient* client) {
	if (!client || !client->is_connected) {
		return;
	}

	close(client->socket);
	client->is_connected = 0;
}

int Net_Login(NetworkClient* client, const char* username) {
	if (!client || !client->is_connected) {
		fprintf(stderr, "[client] Net_Login called while not connected\n");
		return 0;
	}

	if (pame_helper_send_connect(client->socket, username) < 0) {
		fprintf(stderr, "[client] send CONNECT failed\n");
		net_mark_disconnected(client, "CONNECT send failure");
		return 0;
	}

	// Server does not ACK the connect message; it immediately sends
	// PAME_MSG_GAME_WAIT and expects our ACK. We must return now so the
	// caller can enter the main loop and respond to that message.
	strncpy(client->username, username, MAX_PLAYER_USERNAME_SIZE - 1);
	client->username[MAX_PLAYER_USERNAME_SIZE - 1] = '\0';
	fprintf(stderr, "[client] CONNECT sent for user '%s'\n", client->username);
	return 1;
}

int Net_RequestNewGame(NetworkClient* client) {
	if (!client || !client->is_connected) {
		fprintf(stderr, "[client] RequestNewGame while disconnected\n");
		return 0;
	}

	if (pame_helper_send_game_join(client->socket) < 0) {
		fprintf(stderr, "[client] send GAME_JOIN failed\n");
		net_mark_disconnected(client, "GAME_JOIN send failure");
		return 0;
	}

	if (net_expect_ack(client->socket) != 0) {
		net_mark_disconnected(client, "GAME_JOIN ack failure");
		return 0;
	}

	return 1;
}

int Net_SendMove(NetworkClient* client, int cell_index) {
	if (!client || !client->is_connected) {
		fprintf(stderr, "[client] SendMove while disconnected\n");
		return 0;
	}

	if (cell_index < 0 || cell_index > 8) {
		return 0;
	}

	if (pame_helper_send_move(client->socket, (uint8_t)cell_index) < 0) {
		fprintf(stderr, "[client] send MOVE failed\n");
		net_mark_disconnected(client, "MOVE send failure");
		return 0;
	}

	if (net_expect_ack(client->socket) != 0) {
		net_mark_disconnected(client, "MOVE ack failure");
		return 0;
	}

	return 1;
}

int Net_SendDifficulty(NetworkClient* client, int level) {
	if (!client || !client->is_connected) {
		fprintf(stderr, "[client] SendDifficulty while disconnected\n");
		return 0;
	}

	if (level < 1 || level > 3) {
		return 0;
	}

	if (pame_helper_send_difficulty(client->socket, (uint8_t)level) < 0) {
		fprintf(stderr, "[client] send DIFFICULTY failed\n");
		net_mark_disconnected(client, "DIFFICULTY send failure");
		return 0;
	}

	if (net_expect_ack(client->socket) != 0) {
		net_mark_disconnected(client, "DIFFICULTY ack failure");
		return 0;
	}

	return 1;
}

int Net_PollMessage(NetworkClient* client, PAME_Message* out_message) {
	if (!client || !client->is_connected || !out_message) {
		return 0;
	}

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(client->socket, &read_fds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int ready = select(client->socket + 1, &read_fds, NULL, NULL, &timeout);
	if (ready <= 0) {
		if (ready < 0 && errno != EINTR) {
			fprintf(stderr, "[client] select error: %s\n", strerror(errno));
			net_mark_disconnected(client, "select failed");
			return -1;
		}
		return 0;
	}

	if (net_receive_message(client->socket, out_message) < 0) {
		fprintf(stderr, "[client] recv/decode message failed\n");
		net_mark_disconnected(client, "recv/decode failed");
		return -1;
	}

	if (out_message->type == PAME_MSG_GENERAL_ACK) {
		return 0;
	}

	if (pame_helper_send_ack(client->socket, PAME_MESSAGE_ACK_OK) < 0) {
		fprintf(stderr, "[client] send ACK failed\n");
		net_mark_disconnected(client, "ACK send failure");
		return -1;
	}

	return 1;
}

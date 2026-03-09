/**
 * @file server.c
 * @brief Tic-Tac-Toe game server with MCTS AI opponent.
 */

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "ai.h"
#include "board.h"
#include "config.h"
#include "network.h"
#include "pame_high_level.h"
#include "pame_wrappers.h"
#include "tree.h"

static ServerConfig g_config;

/*============================================================================*/
/*                           CONSTANTS                                        */
/*============================================================================*/

#define MAX_SESSIONS 10

/*============================================================================*/
/*                           TYPE DEFINITIONS                                 */
/*============================================================================*/

/**
 * @brief Session state enumeration.
 */
typedef enum {
	SESSION_FREE,  /**< Session slot is available */
	SESSION_LOBBY, /**< Client connected, waiting for game */
	SESSION_GAME,  /**< Game in progress */
	SESSION_ENDED  /**< Game has ended */
} SessionState;

/**
 * @brief Represents a client game session.
 */
typedef struct {
	int id;									 /**< Session identifier */
	int client_socket;						 /**< Client socket descriptor */
	char username[MAX_PLAYER_USERNAME_SIZE]; /**< Player username */
	SessionState state;						 /**< Current session state */
	Board board;							 /**< Game board state */
	pthread_mutex_t lock;					 /**< Session mutex */
} GameSession;

/*============================================================================*/
/*                           GLOBAL VARIABLES                                 */
/*============================================================================*/

static GameSession sessions[MAX_SESSIONS];
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

/*============================================================================*/
/*                           SESSION MANAGEMENT                               */
/*============================================================================*/

static void InitSessions(void) {
	/**
	 * Initializes all session slots to their default state.
	 * Sets all sessions as free and initializes their mutexes.
	 */
	for (int i = 0; i < MAX_SESSIONS; i++) {
		sessions[i].id = i;
		sessions[i].state = SESSION_FREE;
		sessions[i].username[0] = '\0';
		pthread_mutex_init(&sessions[i].lock, NULL);
	}
}

static GameSession* CreateSession(int socket) {
	/**
	 * Creates a new game session for the given socket.
	 * Finds a free session slot, initializes it, and returns a pointer.
	 * Returns NULL if no free slots are available.
	 */
	pthread_mutex_lock(&global_lock);

	for (int i = 0; i < MAX_SESSIONS; i++) {
		if (sessions[i].state == SESSION_FREE) {
			sessions[i].state = SESSION_LOBBY;
			sessions[i].client_socket = socket;
			sessions[i].username[0] = '\0';
			InitBoard(&sessions[i].board);
			fprintf(stderr, "[server] session %d created for socket %d\n", i,
					socket);
			pthread_mutex_unlock(&global_lock);
			return &sessions[i];
		}
	}

	pthread_mutex_unlock(&global_lock);
	return NULL;
}

static void DestroySession(GameSession* session) {
	/**
	 * Destroys a game session and releases its resources.
	 * Closes the client socket and marks the session as free.
	 */
	if (!session) {
		return;
	}

	pthread_mutex_lock(&session->lock);
	fprintf(stderr, "[server] destroy session %d (user='%s')\n", session->id,
			session->username);
	close(session->client_socket);
	session->state = SESSION_FREE;
	session->username[0] = '\0';
	pthread_mutex_unlock(&session->lock);
}

/*============================================================================*/
/*                           PROTOCOL HELPERS                                 */
/*============================================================================*/

static int server_expect_ack(int socket_fd) {
	/**
	 * Waits for and validates an ACK message from the client.
	 * Returns 0 on success, -1 on failure.
	 */
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

static int server_recv_message(GameSession* session, PAME_Message* message) {
	/**
	 * Receives and decodes a message from the client.
	 * Sends an ACK response if the message is not an ACK itself.
	 * Returns 0 on success, 1 for ACK messages, -1 on failure.
	 */
	uint8_t msg_type = 0;
	void* payload = NULL;
	uint16_t payload_len = 0;

	if (pame_recv_packet(session->client_socket, &msg_type, &payload,
						 &payload_len) < 0) {
		fprintf(stderr, "[server][session %d] recv packet failed\n",
				session->id);
		free(payload);
		return -1;
	}

	if (pame_decode_msg(msg_type, payload, payload_len, message) < 0) {
		fprintf(stderr, "[server][session %d] decode msg %u failed\n",
				session->id, (unsigned)msg_type);
		free(payload);
		return -1;
	}

	free(payload);

	if (message->type == PAME_MSG_GENERAL_ACK) {
		return 1;
	}

	if (pame_helper_send_ack(session->client_socket, PAME_MESSAGE_ACK_OK) < 0) {
		fprintf(stderr, "[server][session %d] send ACK failed\n", session->id);
		return -1;
	}

	return 0;
}

/*============================================================================*/
/*                           MESSAGE SENDERS                                  */
/*============================================================================*/

static void fill_game_state_payload(const GameSession* session,
									PAME_PayloadGameState* payload) {
	/**
	 * Fills a game state payload with the current board state.
	 */
	for (int i = 0; i < PAME_BOARD_CELL_COUNT; i++) {
		payload->board_cells[i] = (uint8_t)session->board.cells[i];
	}

	payload->current_turn = (uint8_t)session->board.current_turn;
	payload->state = (uint8_t)session->board.state;
}

static int server_send_game_wait(GameSession* session) {
	/**
	 * Sends a GAME_WAIT message to the client.
	 */
	if (pame_helper_send_game_wait(session->client_socket) < 0) {
		fprintf(stderr, "[server][session %d] send GAME_WAIT failed\n",
				session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

static int server_send_game_start(GameSession* session, uint8_t play_flag) {
	/**
	 * Sends a GAME_START message to the client.
	 */
	if (pame_helper_send_game_start(session->client_socket, play_flag) < 0) {
		fprintf(stderr, "[server][session %d] send GAME_START failed\n",
				session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

static int server_send_play(GameSession* session, uint8_t play_type) {
	/**
	 * Sends a PLAY message to the client.
	 */
	if (pame_helper_send_play(session->client_socket, play_type) < 0) {
		fprintf(stderr, "[server][session %d] send PLAY failed\n", session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

static int server_send_game_state(GameSession* session) {
	/**
	 * Sends the current game state to the client.
	 */
	PAME_PayloadGameState payload;
	fill_game_state_payload(session, &payload);

	if (pame_helper_send_game_state(session->client_socket, &payload,
									sizeof payload) < 0) {
		fprintf(stderr, "[server][session %d] send GAME_STATE failed\n",
				session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

static int server_send_game_end(GameSession* session, uint8_t result) {
	/**
	 * Sends a GAME_END message to the client.
	 */
	if (pame_helper_send_game_end(session->client_socket, result) < 0) {
		fprintf(stderr, "[server][session %d] send GAME_END failed\n",
				session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

static int server_send_ai_move(GameSession* session, uint8_t cell) {
	/**
	 * Sends the AI's move to the client.
	 */
	if (pame_helper_send_move(session->client_socket, cell) < 0) {
		fprintf(stderr, "[server][session %d] send MOVE failed\n", session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

static int server_send_illegal(GameSession* session) {
	/**
	 * Sends an ILLEGAL move notification to the client.
	 */
	if (pame_helper_send_illegal(session->client_socket) < 0) {
		fprintf(stderr, "[server][session %d] send ILLEGAL failed\n",
				session->id);
		return -1;
	}

	return server_expect_ack(session->client_socket);
}

/*============================================================================*/
/*                           GAME LOGIC                                       */
/*============================================================================*/

static uint8_t map_game_result(GameResult result) {
	/**
	 * Maps internal GameResult to protocol result code.
	 */
	if (result == GAME_WIN_X) {
		return PAME_END_GAME_WIN;
	}

	if (result == GAME_WIN_O) {
		return PAME_END_GAME_LOSE;
	}

	if (result == GAME_DRAW) {
		return PAME_END_GAME_DRAW;
	}

	return PAME_END_GAME_NONE;
}

static int HandleGameMove(GameSession* session,
						  const PAME_PayloadMove* move_payload) {
	/**
	 * Handles a move received from the client.
	 * Validates the move, updates the board, and triggers AI response.
	 */
	if (session->board.state != GAME_PLAYING) {
		return 0;
	}

	if (session->board.current_turn != PLAYER_X) {
		return 0;
	}

	int move = move_payload->cell_index;
	fprintf(stderr, "[server][session %d] MOVE from client: %d\n", session->id,
			move);

	if (!IsMoveValid(&session->board, move)) {
		return server_send_illegal(session);
	}

	MakeMove(&session->board, move);

	if (server_send_game_state(session) < 0) {
		return -1;
	}

	if (session->board.state != GAME_PLAYING) {
		uint8_t result = map_game_result(session->board.state);
		return server_send_game_end(session, result);
	}

	if (server_send_play(session, PAME_PLAY_NOT_YOUR_TURN) < 0) {
		return -1;
	}

	int ai_move = AI_ChooseMove(&session->board);

	if (ai_move >= 0) {
		MakeMove(&session->board, ai_move);

		if (server_send_ai_move(session, (uint8_t)ai_move) < 0) {
			return -1;
		}

		if (server_send_game_state(session) < 0) {
			return -1;
		}

		if (session->board.state != GAME_PLAYING) {
			uint8_t result = map_game_result(session->board.state);
			return server_send_game_end(session, result);
		}
	}

	return server_send_play(session, PAME_PLAY_YOUR_TURN);
}

static int ServerPlayAiTurn(GameSession* session) {
	/**
	 * Executes the AI's turn and sends the move to the client.
	 */
	int ai_move = AI_ChooseMove(&session->board);

	if (ai_move < 0) {
		return 0;
	}

	MakeMove(&session->board, ai_move);

	if (server_send_ai_move(session, (uint8_t)ai_move) < 0) {
		return -1;
	}

	if (server_send_game_state(session) < 0) {
		return -1;
	}

	return 0;
}

/*============================================================================*/
/*                           CLIENT THREAD                                    */
/*============================================================================*/

static void* ClientThread(void* arg) {
	/**
	 * Main thread function for handling a client connection.
	 * Processes client messages and manages the game session.
	 */
	int socket = *(int*)arg;
	free(arg);

	GameSession* session = CreateSession(socket);

	if (!session) {
		close(socket);
		return NULL;
	}

	fprintf(stderr, "[server][session %d] client thread started\n",
			session->id);

	while (1) {
		PAME_Message message;
		int status = server_recv_message(session, &message);

		if (status < 0) {
			fprintf(stderr,
					"[server][session %d] recv error, closing session\n",
					session->id);
			break;
		}

		if (status > 0) {
			continue;
		}

		pthread_mutex_lock(&session->lock);
		int op_status = 0;

		switch (message.type) {
		case PAME_MSG_CONNECT:
			memcpy(session->username, message.data.connect.username,
				   MAX_PLAYER_USERNAME_SIZE);
			session->username[MAX_PLAYER_USERNAME_SIZE - 1] = '\0';
			fprintf(stderr, "[server][session %d] CONNECT from '%s'\n",
					session->id, session->username);
			op_status = server_send_game_wait(session);
			break;

		case PAME_MSG_GAME_JOIN:
			fprintf(stderr, "[server][session %d] GAME_JOIN\n", session->id);
			InitBoard(&session->board);
			session->state = SESSION_GAME;

			int client_starts = (session->board.current_turn == PLAYER_X);
			op_status = server_send_game_start(
				session,
				client_starts ? PAME_PLAY_YOUR_TURN : PAME_PLAY_NOT_YOUR_TURN);

			if (op_status != 0) {
				break;
			}

			op_status = server_send_play(
				session,
				client_starts ? PAME_PLAY_YOUR_TURN : PAME_PLAY_NOT_YOUR_TURN);

			if (op_status != 0) {
				break;
			}

			if (!client_starts) {
				op_status = ServerPlayAiTurn(session);

				if (op_status != 0) {
					break;
				}

				if (session->board.state != GAME_PLAYING) {
					uint8_t result = map_game_result(session->board.state);
					op_status = server_send_game_end(session, result);
					break;
				}

				op_status = server_send_play(session, PAME_PLAY_YOUR_TURN);

				if (op_status != 0) {
					break;
				}
			}

			op_status = server_send_game_state(session);
			break;

		case PAME_MSG_MOVE:
			op_status = HandleGameMove(session, &message.data.move);
			break;

		case PAME_MSG_SET_DIFFICULTY: {
			uint8_t level = message.data.set_difficulty.level;
			fprintf(stderr, "[server][session %d] SET_DIFFICULTY level=%u\n",
					session->id, level);
			pthread_mutex_lock(&global_lock);
			switch (level) {
			case 1:
				AI_SetLevel(AI_LEVEL_EASY);
				break;
			case 2:
				AI_SetLevel(AI_LEVEL_MEDIUM);
				break;
			case 3:
				AI_SetLevel(AI_LEVEL_HARD);
				break;
			default:
				AI_SetLevel(AI_LEVEL_MEDIUM);
				break;
			}
			pthread_mutex_unlock(&global_lock);
			break;
		}

		default:
			fprintf(stderr, "[server][session %d] unexpected msg type %u\n",
					session->id, (unsigned)message.type);
			break;
		}

		pthread_mutex_unlock(&session->lock);

		if (op_status < 0) {
			break;
		}
	}

	DestroySession(session);
	return NULL;
}

/*============================================================================*/
/*                           MAIN                                             */
/*============================================================================*/

int main(void) {
	/**
	 * Server entry point.
	 * Loads configuration, initializes sessions, and accepts clients.
	 * AI difficulty is set by clients via PAME_MSG_SET_DIFFICULTY.
	 */
	srand((unsigned int)time(NULL));
	InitSessions();

	Config_LoadServer("config/server.ini", &g_config);

	switch (g_config.default_difficulty) {
		case 1: AI_SetLevel(AI_LEVEL_EASY); break;
		case 3: AI_SetLevel(AI_LEVEL_HARD); break;
		default: AI_SetLevel(AI_LEVEL_MEDIUM); break;
	}

	fprintf(stderr, "[server] starting, port %d\n", g_config.port);

	int server_fd;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(g_config.port);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "[server] bind ok on port %d\n", g_config.port);

	if (listen(server_fd, 3) < 0) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "[server] listen backlog 3\n");
	printf("Server listening on port %d\n", g_config.port);

	while (1) {
		int client_socket =
			accept(server_fd, (struct sockaddr*)&address, &addrlen);

		if (client_socket < 0) {
			fprintf(stderr, "[server] accept error: %s\n", strerror(errno));
			continue;
		}

		fprintf(stderr, "[server] accepted client fd=%d\n", client_socket);

		pthread_t thread_id;
		int* sock_ptr = malloc(sizeof(int));

		if (!sock_ptr) {
			close(client_socket);
			continue;
		}

		*sock_ptr = client_socket;

		if (pthread_create(&thread_id, NULL, ClientThread, sock_ptr) != 0) {
			perror("Thread creation failed");
			free(sock_ptr);
			close(client_socket);
			continue;
		}

		pthread_detach(thread_id);
	}

	close(server_fd);
	return 0;
}

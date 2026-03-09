#ifndef NETWORK_H
#define NETWORK_H

#include "pame_message.h"
#include <stddef.h>

#define SERVER_PORT 5555
#define SERVER_IP "127.0.0.1"

typedef struct {
	int socket;
	int is_connected;
	char username[MAX_PLAYER_USERNAME_SIZE];
} NetworkClient;

int Net_Connect(NetworkClient* client, const char* ip, int port);
void Net_Disconnect(NetworkClient* client);
int Net_Login(NetworkClient* client, const char* username);
int Net_RequestNewGame(NetworkClient* client);
int Net_SendMove(NetworkClient* client, int cell_index);
int Net_SendDifficulty(NetworkClient* client, int level);
int Net_PollMessage(NetworkClient* client, PAME_Message* out_message);

#endif

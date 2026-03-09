#ifndef UI_H
#define UI_H

#include "board.h"

typedef enum {
	UI_STAGE_CONNECTING = 0,
	UI_STAGE_WAITING,
	UI_STAGE_PLAYING,
	UI_STAGE_COMPLETE
} UIStage;

typedef enum {
	LOGIN_ERROR_NONE = 0,
	LOGIN_ERROR_CONNECT,
	LOGIN_ERROR_REJECTED
} LoginError;

typedef enum {
	UI_OUTCOME_IN_PROGRESS = 0,
	UI_OUTCOME_WIN,
	UI_OUTCOME_LOSS,
	UI_OUTCOME_DRAW
} UiOutcome;

// Callback types
typedef void (*MoveCallback)(int index);
typedef void (*ResetCallback)(void);
typedef void (*ClearLearningCallback)(void);

void InitGameUI(void);
void CloseGameUI(void);
void UpdateDrawFrame(void);
void PlayGameSound(const char* soundName);
void SendGameResultToNetwork(int result);

#endif

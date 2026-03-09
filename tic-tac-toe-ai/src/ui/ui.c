/**
 * @file ui.c
 * @brief Raylib-based graphical user interface for Tic-Tac-Toe client.
 */

#include "ui.h"
#include "board.h"
#include "config.h"
#include "network.h"
#include "pame_protocol.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================*/
/*                           THEME COLORS                                     */
/*============================================================================*/

#define COLOR_BG_TOP                                                           \
	(Color) { 249, 245, 238, 255 }
#define COLOR_BG_BOTTOM                                                        \
	(Color) { 239, 246, 250, 255 }
#define COLOR_GRID                                                             \
	(Color) { 201, 212, 226, 255 }
#define COLOR_X                                                                \
	(Color) { 244, 167, 160, 255 }
#define COLOR_O                                                                \
	(Color) { 140, 207, 199, 255 }
#define COLOR_PANEL                                                            \
	(Color) { 255, 255, 255, 245 }
#define COLOR_PANEL_BORDER                                                     \
	(Color) { 236, 233, 226, 255 }
#define COLOR_ACCENT                                                           \
	(Color) { 221, 190, 248, 255 }
#define COLOR_ACCENT_HOVER                                                     \
	(Color) { 211, 180, 238, 255 }
#define COLOR_TEXT_PRIMARY                                                     \
	(Color) { 87, 92, 104, 255 }
#define COLOR_TEXT_SUBTLE                                                      \
	(Color) { 138, 146, 161, 255 }
#define COLOR_TEXT_ERROR                                                       \
	(Color) { 210, 116, 116, 255 }
#define COLOR_SHADOW                                                           \
	(Color) { 0, 0, 0, 25 }
#define COLOR_CELL_HOVER                                                       \
	(Color) { 226, 237, 245, 120 }
#define COLOR_DROPDOWN_BG                                                      \
	(Color) { 249, 247, 244, 255 }

/*============================================================================*/
/*                           TYPE DEFINITIONS                                 */
/*============================================================================*/

/**
 * @brief Global UI state structure.
 */
typedef struct {
	int screenWidth;
	int screenHeight;
	LoginError loginError;
	UIStage stage;
	UiOutcome outcome;
	int isLoggedIn;
	int isPlayerTurn;
	int pendingNewGameRequest;
	double lastConnectAttempt;
	NetworkClient netClient;
	GameResult remoteGameState;
	char usernameInput[64];
	int letterCount;
	char boardStr[10];
	int selectedDifficulty;
	int dropdownOpen;
} UiState;

/*============================================================================*/
/*                           GLOBAL VARIABLES                                 */
/*============================================================================*/

static UiState ui = {
	.screenWidth = 800,
	.screenHeight = 600,
	.loginError = LOGIN_ERROR_NONE,
	.stage = UI_STAGE_CONNECTING,
	.outcome = UI_OUTCOME_IN_PROGRESS,
	.isLoggedIn = 0,
	.isPlayerTurn = 0,
	.pendingNewGameRequest = 0,
	.lastConnectAttempt = 0.0,
	.netClient = {0},
	.remoteGameState = GAME_PLAYING,
	.usernameInput = {0},
	.letterCount = 0,
	.boardStr = "_________",
	.selectedDifficulty = 2,
	.dropdownOpen = 0,
};

static Sound sndClick;
static Sound sndWin;
static Sound sndLose;

static ClientConfig g_clientConfig;

static const char* DIFFICULTY_LABELS[] = {"Easy", "Medium", "Hard"};

/*============================================================================*/
/*                           DRAWING HELPERS                                  */
/*============================================================================*/

static void DrawSoftBackground(void) {
	/**
	 * Draws a soft gradient background with decorative circles.
	 */
	DrawRectangleGradientV(0, 0, ui.screenWidth, ui.screenHeight, COLOR_BG_TOP,
						   COLOR_BG_BOTTOM);
	DrawRectangleGradientH(0, 0, ui.screenWidth, ui.screenHeight,
						   (Color){255, 255, 255, 10},
						   (Color){230, 238, 247, 40});

	Vector2 centers[2] = {{ui.screenWidth * 0.25f, ui.screenHeight * 0.3f},
						  {ui.screenWidth * 0.75f, ui.screenHeight * 0.7f}};
	Color colors[2] = {(Color){248, 214, 214, 80}, (Color){211, 234, 228, 70}};

	for (int i = 0; i < 2; i++) {
		DrawCircleGradient(
			(int)centers[i].x, (int)centers[i].y, (float)ui.screenWidth * 0.4f,
			colors[i],
			(Color){colors[i].r, colors[i].g, colors[i].b, 0});
	}
}

static void DrawPastelPanel(Rectangle bounds, float radius) {
	/**
	 * Draws a panel with shadow and rounded corners.
	 */
	Rectangle shadow = {bounds.x, bounds.y + 6, bounds.width, bounds.height};
	DrawRectangleRounded(shadow, radius, 14, COLOR_SHADOW);
	DrawRectangleRounded(bounds, radius, 14, COLOR_PANEL);
	DrawRectangleRoundedLines(bounds, radius, 14, COLOR_PANEL_BORDER);
}

static void DrawAccentButton(Rectangle bounds, const char* label,
							 Vector2 mouse) {
	/**
	 * Draws a styled button with hover effect.
	 */
	int hover = CheckCollisionPointRec(mouse, bounds);
	Color fill = hover ? COLOR_ACCENT_HOVER : COLOR_ACCENT;

	Rectangle shadow = {bounds.x, bounds.y + 3, bounds.width, bounds.height};
	DrawRectangleRounded(shadow, 0.4f, 10, COLOR_SHADOW);
	DrawRectangleRounded(bounds, 0.4f, 10, fill);

	int txtW = MeasureText(label, 20);
	DrawText(label, (int)(bounds.x + bounds.width / 2 - txtW / 2),
			 (int)bounds.y + 12, 20, COLOR_TEXT_PRIMARY);
}

static int DrawSmallButton(Rectangle bounds, const char* label, Vector2 mouse) {
	/**
	 * Draws a small subtle button. Returns 1 if clicked.
	 */
	int hover = CheckCollisionPointRec(mouse, bounds);
	int click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	Color bg = hover ? COLOR_CELL_HOVER : (Color){255, 255, 255, 180};
	DrawRectangleRounded(bounds, 0.3f, 8, bg);
	DrawRectangleRoundedLines(bounds, 0.3f, 8, COLOR_PANEL_BORDER);

	int txtW = MeasureText(label, 16);
	DrawText(label, (int)(bounds.x + bounds.width / 2 - txtW / 2),
			 (int)bounds.y + 8, 16, COLOR_TEXT_SUBTLE);

	return (hover && click);
}

static int DrawDropdown(Rectangle bounds, int selected, int* open,
						Vector2 mouse) {
	/**
	 * Draws a dropdown selector and returns the new selected index.
	 */
	int hover = CheckCollisionPointRec(mouse, bounds);
	int click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	DrawRectangleRounded(bounds, 0.18f, 10, COLOR_DROPDOWN_BG);
	DrawRectangleRoundedLines(bounds, 0.18f, 10, COLOR_PANEL_BORDER);

	const char* current = DIFFICULTY_LABELS[selected - 1];
	DrawText(current, (int)bounds.x + 14, (int)bounds.y + 14, 20,
			 COLOR_TEXT_PRIMARY);

	DrawText("v", (int)(bounds.x + bounds.width - 24), (int)bounds.y + 14, 20,
			 COLOR_TEXT_SUBTLE);

	if (click && hover && !*open) {
		*open = 1;
		return selected;
	}

	if (*open) {
		for (int i = 0; i < 3; i++) {
			Rectangle opt = {bounds.x, bounds.y + bounds.height + i * 40,
							 bounds.width, 40};
			int optHover = CheckCollisionPointRec(mouse, opt);

			Color bg = optHover ? COLOR_CELL_HOVER : COLOR_DROPDOWN_BG;
			DrawRectangleRounded(opt, 0.1f, 8, bg);
			DrawRectangleRoundedLines(opt, 0.1f, 8, COLOR_PANEL_BORDER);
			DrawText(DIFFICULTY_LABELS[i], (int)opt.x + 14, (int)opt.y + 10, 18,
					 COLOR_TEXT_PRIMARY);

			if (click && optHover) {
				*open = 0;
				return i + 1;
			}
		}

		if (click && !hover) {
			*open = 0;
		}
	}

	return selected;
}

/*============================================================================*/
/*                           NETWORK HANDLING                                 */
/*============================================================================*/

static void AttemptLogin(void) {
	/**
	 * Attempts to connect and login to the game server.
	 */
	if (!ui.netClient.is_connected) {
		if (Net_Connect(&ui.netClient, g_clientConfig.server_ip, g_clientConfig.server_port)) {
			ui.loginError = LOGIN_ERROR_NONE;
		} else {
			ui.loginError = LOGIN_ERROR_CONNECT;
			return;
		}
	}

	if (ui.netClient.is_connected) {
		Net_SendDifficulty(&ui.netClient, ui.selectedDifficulty);

		if (Net_Login(&ui.netClient, ui.usernameInput)) {
			ui.isLoggedIn = 1;
			ui.stage = UI_STAGE_CONNECTING;
			ui.loginError = LOGIN_ERROR_NONE;
		} else {
			ui.loginError = LOGIN_ERROR_REJECTED;
		}
	}
}

static void ApplyGameStatePayload(const PAME_PayloadGameState* payload) {
	/**
	 * Updates UI state from received game state payload.
	 */
	if (!payload) {
		return;
	}

	for (int i = 0; i < PAME_BOARD_CELL_COUNT && i < 9; i++) {
		switch (payload->board_cells[i]) {
		case PLAYER_X:
			ui.boardStr[i] = 'X';
			break;
		case PLAYER_O:
			ui.boardStr[i] = 'O';
			break;
		default:
			ui.boardStr[i] = '_';
			break;
		}
	}
	ui.boardStr[9] = '\0';

	ui.isPlayerTurn = (payload->current_turn == PLAYER_X);
	ui.remoteGameState = (GameResult)payload->state;

	if (ui.remoteGameState == GAME_PLAYING) {
		ui.stage = UI_STAGE_PLAYING;
		ui.outcome = UI_OUTCOME_IN_PROGRESS;
	} else {
		ui.stage = UI_STAGE_COMPLETE;
		if (ui.remoteGameState == GAME_WIN_X) {
			ui.outcome = UI_OUTCOME_WIN;
		} else if (ui.remoteGameState == GAME_WIN_O) {
			ui.outcome = UI_OUTCOME_LOSS;
		} else {
			ui.outcome = UI_OUTCOME_DRAW;
		}
	}
}

static void RequestNewGameIfPending(void) {
	/**
	 * Sends new game request if pending and conditions are met.
	 */
	if (!ui.pendingNewGameRequest || !ui.netClient.is_connected) {
		return;
	}

	if (ui.stage != UI_STAGE_WAITING && ui.stage != UI_STAGE_COMPLETE) {
		return;
	}

	if (Net_RequestNewGame(&ui.netClient)) {
		ui.pendingNewGameRequest = 0;
		ui.stage = UI_STAGE_WAITING;
	}
}

static void AttemptReconnectIfNeeded(void) {
	/**
	 * Attempts reconnection if logged in but disconnected.
	 */
	if (!ui.isLoggedIn || ui.netClient.is_connected) {
		return;
	}

	double now = GetTime();
	if (now - ui.lastConnectAttempt < 1.0) {
		return;
	}

	ui.lastConnectAttempt = now;
	if (!Net_Connect(&ui.netClient, g_clientConfig.server_ip, g_clientConfig.server_port)) {
		ui.loginError = LOGIN_ERROR_CONNECT;
		return;
	}

	ui.loginError = LOGIN_ERROR_NONE;
	Net_SendDifficulty(&ui.netClient, ui.selectedDifficulty);

	if (Net_Login(&ui.netClient, ui.usernameInput)) {
		ui.stage = UI_STAGE_CONNECTING;
	} else {
		ui.loginError = LOGIN_ERROR_CONNECT;
		ui.netClient.is_connected = 0;
	}
}

void ProcessNetworkMessages(void) {
	/**
	 * Processes all pending network messages from the server.
	 */
	if (!ui.netClient.is_connected) {
		return;
	}

	PAME_Message message;
	while (1) {
		int status = Net_PollMessage(&ui.netClient, &message);
		if (status <= 0) {
			if (status < 0) {
				fprintf(stderr, "[ui] Net_PollMessage error; disconnecting\n");
				ui.loginError = LOGIN_ERROR_CONNECT;
				ui.netClient.is_connected = 0;
				ui.stage = UI_STAGE_CONNECTING;
			}
			break;
		}

		switch (message.type) {
		case PAME_MSG_GAME_WAIT:
			ui.isLoggedIn = 1;
			ui.stage = UI_STAGE_WAITING;
			ui.isPlayerTurn = 0;
			ui.outcome = UI_OUTCOME_IN_PROGRESS;
			ui.pendingNewGameRequest = 1;
			break;

		case PAME_MSG_GAME_START:
			memset(ui.boardStr, '_', 9);
			ui.boardStr[9] = '\0';
			ui.stage = UI_STAGE_PLAYING;
			ui.remoteGameState = GAME_PLAYING;
			ui.outcome = UI_OUTCOME_IN_PROGRESS;
			ui.isPlayerTurn =
				(message.data.game_start.is_starting == PAME_PLAY_YOUR_TURN);
			break;

		case PAME_MSG_GAME_STATE:
			ApplyGameStatePayload(&message.data.game_state);
			break;

		case PAME_MSG_PLAY:
			ui.isPlayerTurn =
				(message.data.play.play_type == PAME_PLAY_YOUR_TURN);
			break;

		case PAME_MSG_MOVE:
			PlayGameSound("click");
			break;

		case PAME_MSG_GAME_END:
			ui.stage = UI_STAGE_COMPLETE;
			switch (message.data.game_end.has_won) {
			case PAME_END_GAME_WIN:
				ui.outcome = UI_OUTCOME_WIN;
				PlayGameSound("win");
				break;
			case PAME_END_GAME_LOSE:
				ui.outcome = UI_OUTCOME_LOSS;
				PlayGameSound("lose");
				break;
			default:
				ui.outcome = UI_OUTCOME_DRAW;
				PlayGameSound("lose");
				break;
			}
			break;

		case PAME_MSG_ILLEGAL_PLAY:
			PlayGameSound("lose");
			break;

		default:
			break;
		}
	}
}

/*============================================================================*/
/*                           INITIALIZATION                                   */
/*============================================================================*/

void InitGameUI(void) {
	/**
	 * Initializes the game window, audio, and network connection.
	 * Loads client configuration from config/client.ini.
	 */
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(ui.screenWidth, ui.screenHeight, "Tic-Tac-Toe: 2D Edition");
	InitAudioDevice();
	SetTargetFPS(60);

	Config_LoadClient("config/client.ini", &g_clientConfig);

	sndClick = LoadSound("assets/sounds/click.wav");
	sndWin = LoadSound("assets/sounds/win.wav");
	sndLose = LoadSound("assets/sounds/lose.wav");

	if (Net_Connect(&ui.netClient, g_clientConfig.server_ip, g_clientConfig.server_port)) {
		printf("Connected to server.\n");
	} else {
		printf("Failed to connect to server.\n");
		ui.loginError = LOGIN_ERROR_CONNECT;
	}
}

void CloseGameUI(void) {
	/**
	 * Cleans up resources and closes the game window.
	 */
	Net_Disconnect(&ui.netClient);
	UnloadSound(sndClick);
	UnloadSound(sndWin);
	UnloadSound(sndLose);
	CloseAudioDevice();
	CloseWindow();
}

void PlayGameSound(const char* soundName) {
	/**
	 * Plays a sound effect by name.
	 */
	if (TextIsEqual(soundName, "click")) {
		PlaySound(sndClick);
	} else if (TextIsEqual(soundName, "win")) {
		PlaySound(sndWin);
	} else if (TextIsEqual(soundName, "lose")) {
		PlaySound(sndLose);
	}
}

/*============================================================================*/
/*                           LOGIN SCREEN                                     */
/*============================================================================*/

static void HandleLoginInput(void) {
	/**
	 * Handles keyboard input for the login screen.
	 */
	int key = GetCharPressed();
	while (key > 0) {
		if ((key >= 32) && (key <= 125) && (ui.letterCount < 63)) {
			ui.usernameInput[ui.letterCount] = (char)key;
			ui.usernameInput[ui.letterCount + 1] = '\0';
			ui.letterCount++;
		}
		key = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE)) {
		ui.letterCount--;
		if (ui.letterCount < 0) {
			ui.letterCount = 0;
		}
		ui.usernameInput[ui.letterCount] = '\0';
	}

	if (IsKeyPressed(KEY_ENTER)) {
		AttemptLogin();
	}
}

static void DrawLoginScreen(void) {
	/**
	 * Renders the login screen with username input and difficulty selector.
	 */
	Vector2 mouse = GetMousePosition();
	int click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	float panelW = fminf(ui.screenWidth * 0.9f, 420.0f);
	float panelH = 380.0f;
	Rectangle panel = {ui.screenWidth / 2.0f - panelW / 2.0f,
					   ui.screenHeight / 2.0f - panelH / 2.0f, panelW, panelH};

	DrawPastelPanel(panel, 0.08f);

	DrawText("Tic Tac Toe", (int)panel.x + 32, (int)panel.y + 32, 36,
			 COLOR_TEXT_PRIMARY);
	DrawText("Enter your name and select difficulty.", (int)panel.x + 32,
			 (int)panel.y + 78, 16, COLOR_TEXT_SUBTLE);

	Rectangle inputRect = {panel.x + 32, panel.y + 110, panel.width - 64, 50};
	DrawRectangleRounded(inputRect, 0.18f, 10, COLOR_DROPDOWN_BG);
	DrawRectangleRoundedLines(inputRect, 0.18f, 10, COLOR_PANEL_BORDER);

	const char* displayName =
		ui.usernameInput[0] ? ui.usernameInput : "Enter your name";
	Color displayColor =
		ui.usernameInput[0] ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SUBTLE;
	DrawText(displayName, (int)inputRect.x + 14, (int)inputRect.y + 15, 20,
			 displayColor);

	int caretBlink = ((int)(GetTime() * 2)) % 2 == 0;
	if (caretBlink && ui.usernameInput[0]) {
		int caretX = (int)inputRect.x + 14 + MeasureText(ui.usernameInput, 20);
		DrawRectangle(caretX, (int)inputRect.y + 14, 2, 22, COLOR_ACCENT);
	}

	DrawText("AI Difficulty:", (int)panel.x + 32, (int)panel.y + 175, 18,
			 COLOR_TEXT_SUBTLE);

	Rectangle dropdown = {panel.x + 32, panel.y + 200, panel.width - 64, 50};
	ui.selectedDifficulty =
		DrawDropdown(dropdown, ui.selectedDifficulty, &ui.dropdownOpen, mouse);

	if (!ui.dropdownOpen) {
		Rectangle btnPlay = {panel.x + 32, panel.y + 280, panel.width - 64, 50};
		DrawAccentButton(btnPlay, "Play", mouse);

		if (click && CheckCollisionPointRec(mouse, btnPlay)) {
			AttemptLogin();
		}
	}

	if (ui.loginError == LOGIN_ERROR_CONNECT) {
		DrawText("Connection failed - server unreachable", (int)panel.x + 32,
				 (int)panel.y + panel.height - 30, 16, COLOR_TEXT_ERROR);
	} else if (ui.loginError == LOGIN_ERROR_REJECTED) {
		DrawText("Login rejected - try another name", (int)panel.x + 32,
				 (int)panel.y + panel.height - 30, 16, COLOR_TEXT_ERROR);
	}
}

/*============================================================================*/
/*                           GAME SCREEN                                      */
/*============================================================================*/

static void HandleGameInput(float startX, float startY, float cellSize,
							float boardSize) {
	/**
	 * Handles mouse input during gameplay.
	 */
	if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
		ui.stage != UI_STAGE_PLAYING) {
		return;
	}

	Vector2 mousePos = GetMousePosition();

	if (mousePos.x < startX || mousePos.x > startX + boardSize ||
		mousePos.y < startY || mousePos.y > startY + boardSize) {
		return;
	}

	int col = (int)((mousePos.x - startX) / cellSize);
	int row = (int)((mousePos.y - startY) / cellSize);
	int index = row * 3 + col;

	if (ui.boardStr[index] == '_' && ui.isPlayerTurn) {
		if (Net_SendMove(&ui.netClient, index)) {
			ui.isPlayerTurn = 0;
			PlayGameSound("click");
		}
	}
}

static void DrawBoard(float startX, float startY, float cellSize,
					  float boardSize) {
	/**
	 * Draws the game board with grid, cells, and marks.
	 */
	Vector2 mouse = GetMousePosition();

	Rectangle boardRect = {startX, startY, boardSize, boardSize};
	Rectangle boardShadow = {boardRect.x, boardRect.y + 8, boardRect.width,
							 boardRect.height};

	DrawRectangleRounded(boardShadow, 0.12f, 12, COLOR_SHADOW);
	DrawRectangleRounded(boardRect, 0.12f, 12, COLOR_PANEL);
	DrawRectangleRoundedLines(boardRect, 0.12f, 12, COLOR_PANEL_BORDER);

	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			Rectangle cellRect = {startX + col * cellSize,
								  startY + row * cellSize, cellSize, cellSize};
			int hoverCell = CheckCollisionPointRec(mouse, cellRect);
			if (hoverCell) {
				Color hoverTint = COLOR_CELL_HOVER;
				if (ui.boardStr[row * 3 + col] != '_') {
					hoverTint.a = 70;
				}
				DrawRectangleRounded(cellRect, 0.18f, 6, hoverTint);
			}
		}
	}

	for (int i = 1; i < 3; i++) {
		DrawLineEx((Vector2){startX + i * cellSize, startY},
				   (Vector2){startX + i * cellSize, startY + boardSize}, 4,
				   COLOR_GRID);
		DrawLineEx((Vector2){startX, startY + i * cellSize},
				   (Vector2){startX + boardSize, startY + i * cellSize}, 4,
				   COLOR_GRID);
	}

	for (int i = 0; i < 9; i++) {
		int col = i % 3;
		int row = i / 3;
		float x = startX + col * cellSize + cellSize / 2;
		float y = startY + row * cellSize + cellSize / 2;
		float radius = cellSize * 0.28f;

		if (ui.boardStr[i] == 'X') {
			DrawLineEx((Vector2){x - radius, y - radius},
					   (Vector2){x + radius, y + radius}, 8, COLOR_X);
			DrawLineEx((Vector2){x + radius, y - radius},
					   (Vector2){x - radius, y + radius}, 8, COLOR_X);
		} else if (ui.boardStr[i] == 'O') {
			DrawRing((Vector2){x, y}, radius - 6, radius, 0, 360, 48, COLOR_O);
		}
	}
}

static void DrawGameScreen(void) {
	/**
	 * Renders the main game screen with board and UI elements.
	 */
	Vector2 mouse = GetMousePosition();

	/* Menu button in top-left corner */
	Rectangle btnMenu = {20.0f, 20.0f, 80.0f, 36.0f};
	if (DrawSmallButton(btnMenu, "< Menu", mouse)) {
		/* Return to login screen to change difficulty */
		ui.isLoggedIn = 0;
		ui.stage = UI_STAGE_CONNECTING;
		ui.outcome = UI_OUTCOME_IN_PROGRESS;
		ui.pendingNewGameRequest = 0;
		memset(ui.boardStr, '_', 9);
		ui.boardStr[9] = '\0';
		Net_Disconnect(&ui.netClient);
		PlayGameSound("click");
		return;
	}

	float boardSize = fminf(ui.screenWidth, ui.screenHeight) * 0.55f;
	boardSize = fminf(boardSize, 520.0f);
	boardSize = fmaxf(boardSize, 280.0f);
	float startX = ui.screenWidth / 2.0f - boardSize / 2.0f;
	float startY = ui.screenHeight / 2.0f - boardSize / 2.0f + 40.0f;

	if (startY < 140.0f) {
		startY = 140.0f;
	}
	if (startY + boardSize + 140.0f > ui.screenHeight) {
		startY = ui.screenHeight - boardSize - 140.0f;
		if (startY < 120.0f) {
			startY = 120.0f;
		}
	}

	float cellSize = boardSize / 3.0f;

	HandleGameInput(startX, startY, cellSize, boardSize);

	const char* title = "Tic Tac Toe";
	int titleWidth = MeasureText(title, 44);
	DrawText(title, ui.screenWidth / 2 - titleWidth / 2, 48, 44,
			 COLOR_TEXT_PRIMARY);

	const char* statusText = "";
	switch (ui.stage) {
	case UI_STAGE_PLAYING:
		statusText = ui.isPlayerTurn ? "Your turn" : "Waiting for AI...";
		break;
	case UI_STAGE_WAITING:
		statusText = "Waiting for a new match";
		break;
	case UI_STAGE_CONNECTING:
		statusText = "Connecting to lobby...";
		break;
	case UI_STAGE_COMPLETE:
		statusText = "Match complete";
		break;
	default:
		statusText = "";
		break;
	}
	int statusWidth = MeasureText(statusText, 24);
	DrawText(statusText, ui.screenWidth / 2 - statusWidth / 2, 104, 24,
			 COLOR_TEXT_SUBTLE);

	DrawBoard(startX, startY, cellSize, boardSize);

	Rectangle btnReset = {ui.screenWidth / 2.0f - 110.0f,
						  startY + boardSize + 30.0f, 220.0f, 52.0f};

	if (ui.stage == UI_STAGE_WAITING || ui.stage == UI_STAGE_COMPLETE) {
		DrawAccentButton(btnReset, "New Game", mouse);

		if (CheckCollisionPointRec(mouse, btnReset) &&
			IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			ui.pendingNewGameRequest = 1;
			ui.outcome = UI_OUTCOME_IN_PROGRESS;
			ui.stage = UI_STAGE_WAITING;
			PlayGameSound("click");
		}
	}

	if (ui.stage == UI_STAGE_COMPLETE) {
		const char* msg = "";
		Color c = COLOR_TEXT_PRIMARY;

		if (ui.outcome == UI_OUTCOME_WIN) {
			msg = "You win!";
			c = COLOR_X;
		} else if (ui.outcome == UI_OUTCOME_LOSS) {
			msg = "AI wins!";
			c = COLOR_O;
		} else {
			msg = "It's a draw!";
			c = COLOR_TEXT_SUBTLE;
		}

		int msgWidth = MeasureText(msg, 28);
		DrawText(msg, ui.screenWidth / 2 - msgWidth / 2, startY - 40, 28, c);
	} else {
		const char* hint = "Tap any empty cell to play";
		int hintWidth = MeasureText(hint, 20);
		DrawText(hint, ui.screenWidth / 2 - hintWidth / 2, startY - 40, 20,
				 COLOR_TEXT_SUBTLE);
	}

	char footer[96];
	const char* userLabel = ui.usernameInput[0] ? ui.usernameInput : "guest";
	const char* diffLabel = DIFFICULTY_LABELS[ui.selectedDifficulty - 1];
	snprintf(footer, sizeof(footer), "%s | AI: %s", userLabel, diffLabel);
	int footerWidth = MeasureText(footer, 18);
	DrawText(footer, ui.screenWidth - footerWidth - 32, ui.screenHeight - 44,
			 18, COLOR_TEXT_SUBTLE);
}

/*============================================================================*/
/*                           MAIN UPDATE LOOP                                 */
/*============================================================================*/

void UpdateDrawFrame(void) {
	/**
	 * Main update and render function called each frame.
	 */
	AttemptReconnectIfNeeded();
	ProcessNetworkMessages();
	RequestNewGameIfPending();

	if (IsWindowResized()) {
		ui.screenWidth = GetScreenWidth();
		ui.screenHeight = GetScreenHeight();
	}

	BeginDrawing();
	ClearBackground(COLOR_BG_TOP);
	DrawSoftBackground();

	if (!ui.isLoggedIn) {
		if (!ui.netClient.is_connected) {
			double now = GetTime();
			if (now - ui.lastConnectAttempt > 1.0) {
				if (Net_Connect(&ui.netClient, g_clientConfig.server_ip, g_clientConfig.server_port)) {
					ui.loginError = LOGIN_ERROR_NONE;
				} else {
					ui.loginError = LOGIN_ERROR_CONNECT;
				}
				ui.lastConnectAttempt = now;
			}
		}

		HandleLoginInput();
		DrawLoginScreen();
	} else {
		DrawGameScreen();
	}

	EndDrawing();
}

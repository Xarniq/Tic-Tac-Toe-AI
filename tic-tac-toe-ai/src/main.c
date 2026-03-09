#include "raylib.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
	srand(time(NULL));

	// Init
	InitGameUI();

	// Main loop
	while (!WindowShouldClose()) {
		// Draw & Update
		UpdateDrawFrame();
	}

	// Cleanup
	CloseGameUI();

	return 0;
}

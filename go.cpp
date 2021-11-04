#include "Game.h"

int main() {

	printf("\n\n\ninput 1 to play with AI,or input 0 to start selfplay\n");
    int mode = 1;
	scanf_s("%d", &mode);
	
	if (mode == 1) {
		printf("\n\ninput 0 if you want move firstly,else input 1\n");

		int start_player = 0;
		scanf_s("%d", &start_player);
		Game* game = new Game(15,start_player);
		game->start();
		delete game;
	}
	else {
		Game* game = new Game(15);
		game->selfplay();
		delete game;
	}
	return 0;
}
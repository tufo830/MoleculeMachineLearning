#pragma once
#include "Board.h"
#include "Player.h"


class Game {
public:
	Game(int n = 15, int start_player = 0) :
		n(n), board(n, start_player), human(n), robot(15,400, 5,3 ) {}

	int restart(int start_player) {
		board.init(15, start_player);
		start();
	}
	void start() {
		//test();
		int status = board.end();
		//printf("status:%d\n", status);
		while (status == -2) {
			if (board.current_player == 0) {
				int action = human.get_action(board);
				board.do_move(action);
			}
			else {
				robot.mcts.update(board.last_move);
				int action = robot.get_action(board);
				assert(board.plane[action / n][action % n] == -1);
				board.do_move(action);
				robot.mcts.update(action);
			}
			status = board.end();
		}
		if (status > -1) printf("the winner is player%d\n", status);
		else printf("tie game\n");
		return;
	}
	void test() {
		//board.do_move(0, 0);
		//board.do_move(3, 4);
		//board.do_move(0, 1);
		//board.do_move(7, 4);
		//board.do_move(0, 2);
		//board.do_move(7, 7);
		//board.do_move(0, 3);
		//board.do_move(4, 6);
		//board.do_move(0, 9);
		//board.do_move(7, 8);
		//board.do_move(8, 9);
		//board.do_move(4, 9);
		//board.do_move(1, 9);
		//board.do_move(5,6);
		//board.do_move(1,4);
		//board.do_move(4,5);
		board.do_move(9,7);
		board.do_move(9,1);
		

		board.end();
		board.display();
		PolicyValueNet net;
		double x;
		net.getprob(board,x);
		//robot.get_action(board);

		printf("%d", board.end());
	}
private:
	int n,start_player;
	Board board;
	Alphago0  robot;
	Human human;
};

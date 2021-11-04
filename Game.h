#pragma once
#include "Board.h"
#include "Player.h"


class Game {
public:
	Game(int n = 15, int start_player = 0) :
		n(n),
		board(n, start_player), 
		human(n), 
		robot(15,400,5,3) {}

	void start() {
		int status = board.end();
		board.display();
		while (status == -2) {
			if (board.current_player == 0) {
				int action = human.get_action(board);
				board.do_move(action);
			}
			else {
				robot.mcts.update(board.last_move);	      
				int move = robot.get_action(board);
				board.do_move(move);
				robot.mcts.update(move);
				printf("AI'S move is %d,%d\n", move / n, move % n);
			}
			board.display();
			status = board.end();
		}
		board.display();
		if (status == -1)
			printf("Tie\n");
		else if (status==0) 
			printf("YOU Win\n");
		else printf("AI Win\n");
		return;
	}
	void selfplay() {
		int status = board.end();
		BetaCat0 robot2(15,400,5,3);
		board.display();
		while (status == -2) {
			if (board.current_player == 0) {
				robot.mcts.update(board.last_move);
				int action = robot.get_action(board,true);
				board.do_move(action);
				robot.mcts.update(action);
			}
			else {
				robot2.mcts.update(board.last_move);
				int action = robot2.get_action(board,true);
				board.do_move(action);
				robot2.mcts.update(action);
			}
			status = board.end();
			board.display();
		}
		if (status > -1) printf("the winner is player%d\n", status);
		else printf("tie game\n");
		return;
	}
private:
	int n,start_player;
	Board board;
	BetaCat0  robot;
	Human human;
};

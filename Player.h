#pragma once
#include "MCTS.h"

class Human
{
public:
	Human(int n) :n(n) {}

	int get_action(const Board& board) {
		printf("O is human's move,X is the AI's move\n\n");
		printf("input the coordinates of your move(H,W)\n");
		int h, w;
		while (true) {
            int num=scanf_s("%d,%d", &h, &w);
			if (num != 2) printf("not coordinates,try again\n");
			else if (h < 0 || h >= n || w < 0 || w >= n)
				printf("invalid move,try again\n");
			else if (board.plane[h][w] != -1)
				printf("Thers is already a move,try another position\n");
			else return h * n + w;
		}
	}
private:
	int n;//n is the board size
};

class BetaCat0
{
	friend class Game;
public:
	BetaCat0( int n,int simulation, double cpuct,double cvloss)
		:n(n),
		simulation(simulation), 
		cpuct(cpuct), 
		mcts(n, simulation,cpuct,cvloss),
		first_n_num(12)
	{}

	int get_action(const Board& board,bool selfplay=false) {
		printf("waiting\n");
		std::vector<int> visit_num = mcts.getVisitNumber(board);

		//printf("\nnumber ************************\n");
		//for (int i = 0; i < n; i++) {
		//	for (int j = 0; j < n; j++)
		//		printf("%4d ", visit_num[i * n + j]);
		//	printf("\n\n");
		//}

		if (selfplay && board.move_seq.size() < first_n_num) {
			std::random_device rd;
			std::mt19937 engine(rd());
			std::discrete_distribution<> prob(visit_num.begin(), visit_num.end());
			return prob(engine);
		}

		int action = -1,max_visit = -1;
		for (int i = 0; i < visit_num.size(); i++) {
			if (visit_num[i] > max_visit) {
				 max_visit = visit_num[i];
				 action = i;
			}
		}
		return action;
	}
private:
	int n, first_n_num, simulation;
	double cpuct, temp;
	MCTS mcts;
};

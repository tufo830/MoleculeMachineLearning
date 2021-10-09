#pragma once
#include "MCTS.h"

class Human
{
public:
	Human(int n) :n(n) {

	}
	int get_action(const Board& board) {//n is the board width
		board.display();
		printf("Please input your move as the format: h,w  ");
		int h, w;
		scanf_s("%d,%d", &h, &w);
		assert(board.plane[h][w]==-1);
		printf("\nyour move is %d,%d\n", h, w);
		return h * n + w;
	}
private:
	int n;
};

class Alphago0
{
	friend class Game;
public:
	Alphago0( int n,int simulation, double cpuct,double cvloss)
		:simulation(simulation), cpuct(cpuct), n(n), mcts(n, simulation,cpuct,cvloss)
	{
		temp = 1;
		first_num = 11;
	}
	int get_action(const Board& board) {
		vector<int> visit_num = mcts.getVisitNumber(board);
		printf("\nnumber ************************\n");
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++)
				printf("%d ", visit_num[i * n + j]);
			printf("\n");
		}
		int action = -1, max_visit = -1;
		for (int i = 0; i < visit_num.size(); i++) {
			if (visit_num[i] > max_visit) {
				max_visit = visit_num[i];
				action = i;
			}
		}
		
		return action;
		int h = action / n, w = action % n;
		return (14 - h) * n + w;
	}
private:
	int n, first_num, simulation;
	double cpuct, temp;
	MCTS mcts;
};




class MCTSPlayer
{
	MCTSPlayer(int simulation) {

	}

};
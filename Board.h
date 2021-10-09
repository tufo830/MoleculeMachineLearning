#pragma once
#include<stdio.h>
#include <vector>
using std::vector;
class PolicyValueNet;
class Game;
class MCTS;
class Human;

class Board {
public:
	friend class PolicyValueNet;
	friend class Game;
	friend class MCTS;
	friend class Player;
	friend class Human;
	Board(int n = 15,  int start_player = 0)
		: n(n), start_player(start_player), last_move(-1), plane(n, vector<int>(n, -1)) {
		current_player = start_player;
	}
	void init(int n, int start_player) {
		this->n = n;
		this->current_player = start_player;
		this->last_move = -1;
		move_seq.clear();
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				plane[i][j] = -1;
	}
	Board& operator=(const Board& board) {
		this->n = board.n;
		this->current_player = board.current_player;
		this->last_move = board.last_move;
		this->plane = board.plane;
		this->move_seq = board.move_seq;
		this->n = board.n;
		return *this;
	}
	void do_move(int pos) {
		this->move_seq.push_back(pos);
		this->last_move = pos;
		plane[pos / n][pos % n] = current_player;
		current_player = (current_player + 1) % 2;
	}
	void do_move(int h, int w) {
		this->do_move(h * n + w);
	}
	int end()const {
		// 0 or 1 represent winner,-1 represent no winner
		for (int i = 0; i < move_seq.size(); i++) {

			int h = move_seq[i] / n, w = move_seq[i] % n, x, y, sum = 1;

			for (y = w + 1; y < n && plane[h][y] == plane[h][w]; y++)
				if (y - w >= 4) return plane[h][w];

			for (x = h + 1; x < n && plane[x][w] == plane[h][w]; x++)
				if (x - h >= 4) return plane[h][w];

			for (x = h + 1, y = w + 1; x < n && y < n && plane[x][y] == plane[h][w]; x++, y++)
				if (x - h >= 4) return plane[h][w];

			for (x = h + 1, y = w - 1; x < n && y >= 0 && plane[x][y] == plane[h][w]; x++, y--)
				if (x - h >= 4) return plane[h][w];
		}
		if (move_seq.size() == n * n) return -1;
		return -2;
	}
	void display()const {
		printf("\n");
		//for (int i = 0; i < move_seq.size(); i++)
		//	printf("\n%d ", move_seq[i]);
		for (int i = 0; i < n; i++) {
			if (i == 0) {
				printf("  ");
				for (int j = 0; j < n; j++) {
					printf("%2d ", j);
				}
				printf("\n");
			}
			printf("%d ", i);
			if (i < 10) printf(" ");
			for (int j = 0; j < n; j++) {
				if (plane[i][j] < 0) printf("*  ");
				else printf("%d  ", plane[i][j]);
			}
			printf("\n\n");
		}
	}
	~Board() {

	}
private:
	int n, start_player, last_move, current_player;
	vector<int> move_seq;
	vector<vector<int>> plane;
};
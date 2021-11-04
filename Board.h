#pragma once

#include<stdio.h>
#include <vector>

class Board {
public:
	friend class PolicyValueNet;
	friend class Game;
	friend class MCTS;
	friend class BetaCat0;
	friend class Human;
	Board(int n = 15,  int start_player = 0)
		: n(n), start_player(start_player), 
		current_player(start_player),
		last_move(-1), 
		plane(n, std::vector<int>(n, -1)) {}
	void init(int n, int start_player) {
		this->n = n;
		this->current_player = start_player;
		this->last_move = -1;
		move_seq.clear();
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				plane[i][j] = -1;
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
	int end () const{
		if (move_seq.size() < 9) return -2;//not enough moves
		int h = move_seq.back() / n, w = move_seq.back() % n, color = (current_player + 1) % 2, up, down, left, right;

		up = h-1, down = h+1,left = w - 1, right = w + 1;
		while (up >= 0 && plane[up][w] == color)  up--; 
		while (down < n && plane[down][w] == color)  down++; 
		if (down - up > 5)  return color; 
		
		while (left >= 0 && plane[h][left] == color)  left--; 
		while (right < n && plane[h][right] == color)  right++; 
		if (right - left > 5)  return color; 

		left = w - 1, up = h - 1, right = w + 1, down = h + 1;
		while (left>=0&&up>=0&&plane[up][left] == color) 
		{
			left--;
			up--;
		}
		while (right < n && down < n && plane[down][right] == color) 
		{
			right++;
			down++;
		}
		if (down - up > 5)  return color; 

		left = w - 1, up = h - 1, right = w + 1, down = h + 1;
		while (down < n && left >= 0 && plane[down][left] == color) {
			down++;
			left--;
		}
		while (up >= 0 && right < n && plane[up][right] == color) {
			up--;
			right++;
		}
		if (right - left > 5)  return color; //winner

		if (move_seq.size() == n * n)  return -1; //tie
		return -2;//not end
	}

	void display()const {
		//display the board
		printf("\n");
		for (int i = 0; i < n; i++) {
			if (i == 0) {
				printf("  ");
				for (int j = 0; j < n; j++) {
					printf("%2d  ", j);
				}
				printf("\n");
			}
			printf("%d ", i);
			if (i < 10) printf(" ");
			for (int j = 0; j < n; j++) {
				if (plane[i][j] < 0)
					printf(".   ");
				else if (plane[i][j] == 0)
					printf("O   ");
				else printf("X   ");
			}
			printf("\n\n");
		}
	}

private:
	int n, start_player, last_move, current_player;
	std::vector<int> move_seq;
	std::vector<std::vector<int>> plane;
};

